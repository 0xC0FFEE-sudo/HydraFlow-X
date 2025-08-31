#pragma once

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <chrono>
#include <iostream>
#include <sstream>
#include <fstream>
#include <atomic>
#include <mutex>
#include <thread>
#include <typeinfo>
#include <exception>
#include <cassert>
#include <any>

namespace hfx::testing {

// Test result status
enum class TestStatus {
    PENDING,
    RUNNING,
    PASSED,
    FAILED,
    SKIPPED,
    ERROR
};

// Test categories for organization
enum class TestCategory {
    UNIT,
    INTEGRATION,
    PERFORMANCE,
    STRESS,
    SECURITY,
    COMPATIBILITY,
    REGRESSION
};

// Test priority levels
enum class TestPriority {
    LOW = 0,
    MEDIUM = 1,
    HIGH = 2,
    CRITICAL = 3
};

// Performance measurement types
enum class PerformanceMetric {
    LATENCY_NS,      // Nanosecond latency
    THROUGHPUT_OPS,  // Operations per second
    MEMORY_BYTES,    // Memory usage in bytes
    CPU_PERCENT,     // CPU usage percentage
    CUSTOM           // Custom metric
};

// Mock behavior types
enum class MockBehavior {
    RETURN_VALUE,    // Return specified value
    THROW_EXCEPTION, // Throw specified exception
    CALL_REAL,       // Call real implementation
    CALL_LAMBDA,     // Call custom lambda
    IGNORE           // Do nothing
};

// Forward declarations
class TestSuite;
class TestCase;
class TestRunner;
class MockObject;
class PerformanceBenchmark;

// Test execution context
struct TestContext {
    std::string test_name;
    std::string suite_name;
    TestCategory category;
    TestPriority priority;
    std::chrono::system_clock::time_point start_time;
    std::chrono::system_clock::time_point end_time;
    std::unordered_map<std::string, std::string> metadata;
    std::vector<std::string> tags;
    bool parallel_safe = true;
};

// Test result information
struct TestResult {
    std::string test_name;
    std::string suite_name;
    TestStatus status;
    std::chrono::nanoseconds execution_time;
    std::string error_message;
    std::string failure_details;
    std::vector<std::string> assertions_passed;
    std::vector<std::string> assertions_failed;
    std::unordered_map<PerformanceMetric, double> performance_metrics;
    std::unordered_map<std::string, std::string> custom_data;
    
    TestResult() : status(TestStatus::PENDING), execution_time(0) {}
};

// Performance measurement result
struct PerformanceResult {
    PerformanceMetric metric_type;
    std::string metric_name;
    double value;
    std::string unit;
    double baseline_value = 0.0;
    double threshold_value = 0.0;
    bool meets_threshold = true;
    std::vector<double> samples;
    
    // Statistical measures
    double min_value = 0.0;
    double max_value = 0.0;
    double mean_value = 0.0;
    double median_value = 0.0;
    double std_deviation = 0.0;
    double percentile_95 = 0.0;
    double percentile_99 = 0.0;
};

// Assertion helper macros and functions
class AssertionException : public std::exception {
public:
    explicit AssertionException(const std::string& message) : message_(message) {}
    const char* what() const noexcept override { return message_.c_str(); }
    
private:
    std::string message_;
};

// Base test case class
class TestCase {
public:
    explicit TestCase(const std::string& name, TestCategory category = TestCategory::UNIT,
                     TestPriority priority = TestPriority::MEDIUM);
    virtual ~TestCase() = default;
    
    // Test lifecycle methods
    virtual void SetUp() {}
    virtual void TearDown() {}
    virtual void Run() = 0;
    
    // Test execution
    TestResult Execute();
    
    // Getters
    const std::string& GetName() const { return name_; }
    TestCategory GetCategory() const { return category_; }
    TestPriority GetPriority() const { return priority_; }
    const std::vector<std::string>& GetTags() const { return tags_; }
    
    // Configuration
    void AddTag(const std::string& tag) { tags_.push_back(tag); }
    void SetTimeout(std::chrono::milliseconds timeout) { timeout_ = timeout; }
    void SetParallelSafe(bool safe) { parallel_safe_ = safe; }
    void SetMetadata(const std::string& key, const std::string& value) { metadata_[key] = value; }
    
protected:
    std::string name_;
    TestCategory category_;
    TestPriority priority_;
    std::vector<std::string> tags_;
    std::unordered_map<std::string, std::string> metadata_;
    std::chrono::milliseconds timeout_{30000}; // 30 seconds default
    bool parallel_safe_ = true;
    
    // Performance measurement
    void StartPerformanceMeasurement(const std::string& metric_name);
    void EndPerformanceMeasurement(const std::string& metric_name);
    void RecordPerformanceMetric(PerformanceMetric type, const std::string& name, double value);
    
private:
    std::unordered_map<std::string, std::chrono::high_resolution_clock::time_point> perf_start_times_;
    std::vector<PerformanceResult> performance_results_;
};

// Test suite for grouping related tests
class TestSuite {
public:
    explicit TestSuite(const std::string& name);
    virtual ~TestSuite() = default;
    
    // Suite lifecycle
    virtual void SetUpSuite() {}
    virtual void TearDownSuite() {}
    
    // Test management
    void AddTest(std::unique_ptr<TestCase> test);
    void AddTest(std::shared_ptr<TestCase> test);
    
    // Execution
    std::vector<TestResult> RunAllTests();
    std::vector<TestResult> RunTestsWithTag(const std::string& tag);
    std::vector<TestResult> RunTestsByCategory(TestCategory category);
    std::vector<TestResult> RunTestsByPriority(TestPriority min_priority);
    
    // Information
    const std::string& GetName() const { return name_; }
    size_t GetTestCount() const { return tests_.size(); }
    std::vector<std::string> GetAllTags() const;
    
private:
    std::string name_;
    std::vector<std::shared_ptr<TestCase>> tests_;
};

// Mock object base class
class MockObject {
public:
    virtual ~MockObject() = default;
    
    // Expectation setting
    template<typename T>
    void ExpectCall(const std::string& method_name, const T& return_value);
    
    void ExpectCallWithException(const std::string& method_name, const std::exception& exception);
    void ExpectCallCount(const std::string& method_name, int expected_count);
    
    // Verification
    bool VerifyExpectations() const;
    std::vector<std::string> GetUnmetExpectations() const;
    
    // Call tracking
    void RecordCall(const std::string& method_name);
    int GetCallCount(const std::string& method_name) const;
    
protected:
    struct MockExpectation {
        std::string method_name;
        MockBehavior behavior;
        std::any return_value;
        std::exception_ptr exception;
        int expected_call_count = -1; // -1 means any number
        int actual_call_count = 0;
        std::function<void()> lambda;
    };
    
    std::vector<MockExpectation> expectations_;
    std::unordered_map<std::string, int> call_counts_;
    mutable std::mutex mock_mutex_;
};

// Performance benchmark utilities
class PerformanceBenchmark {
public:
    explicit PerformanceBenchmark(const std::string& name);
    
    // Benchmark execution
    template<typename Func>
    PerformanceResult MeasureLatency(Func&& func, int iterations = 1000);
    
    template<typename Func>
    PerformanceResult MeasureThroughput(Func&& func, std::chrono::milliseconds duration = std::chrono::milliseconds(1000));
    
    PerformanceResult MeasureMemoryUsage(std::function<void()> func);
    
    // Statistical analysis
    static double CalculateMean(const std::vector<double>& values);
    static double CalculateMedian(std::vector<double> values);
    static double CalculateStdDeviation(const std::vector<double>& values, double mean);
    static double CalculatePercentile(std::vector<double> values, double percentile);
    
    // Comparison and validation
    bool ComparePerformance(const PerformanceResult& baseline, const PerformanceResult& current, 
                          double tolerance_percent = 5.0) const;
    
private:
    std::string name_;
    
    size_t GetMemoryUsage() const;
    double GetCpuUsage() const;
};

// Main test runner and coordinator
class TestRunner {
public:
    TestRunner();
    ~TestRunner();
    
    // Configuration
    void SetParallelExecution(bool enabled, int max_threads = std::thread::hardware_concurrency());
    void SetOutputFormat(const std::string& format); // "console", "xml", "json"
    void SetOutputFile(const std::string& filepath);
    void SetVerboseMode(bool verbose) { verbose_mode_ = verbose; }
    void SetRandomSeed(uint32_t seed) { random_seed_ = seed; }
    void SetShuffleTests(bool shuffle) { shuffle_tests_ = shuffle; }
    
    // Test discovery and registration
    void RegisterTestSuite(std::shared_ptr<TestSuite> suite);
    void DiscoverTests(const std::string& directory_path);
    void LoadTestsFromFile(const std::string& config_file);
    
    // Filtering
    void SetTestFilter(const std::string& filter_pattern);
    void SetCategoryFilter(const std::vector<TestCategory>& categories);
    void SetTagFilter(const std::vector<std::string>& tags);
    void SetPriorityFilter(TestPriority min_priority);
    
    // Execution
    void RunAllTests();
    void RunFilteredTests();
    void RunSuite(const std::string& suite_name);
    void RunTest(const std::string& suite_name, const std::string& test_name);
    
    // Results and reporting
    std::vector<TestResult> GetResults() const { return all_results_; }
    void GenerateReport();
    void GeneratePerformanceReport();
    void GenerateCoverageReport();
    
    // Statistics
    struct TestStatistics {
        int total_tests = 0;
        int passed_tests = 0;
        int failed_tests = 0;
        int skipped_tests = 0;
        int error_tests = 0;
        std::chrono::milliseconds total_execution_time{0};
        double success_rate = 0.0;
        
        // Performance stats
        double avg_latency_ns = 0.0;
        double max_latency_ns = 0.0;
        double avg_memory_usage_mb = 0.0;
        double max_memory_usage_mb = 0.0;
    };
    
    TestStatistics GetStatistics() const;
    
    // Event callbacks
    using TestStartCallback = std::function<void(const TestContext&)>;
    using TestEndCallback = std::function<void(const TestResult&)>;
    using SuiteStartCallback = std::function<void(const std::string&)>;
    using SuiteEndCallback = std::function<void(const std::string&, const std::vector<TestResult>&)>;
    
    void SetTestStartCallback(TestStartCallback callback) { test_start_callback_ = callback; }
    void SetTestEndCallback(TestEndCallback callback) { test_end_callback_ = callback; }
    void SetSuiteStartCallback(SuiteStartCallback callback) { suite_start_callback_ = callback; }
    void SetSuiteEndCallback(SuiteEndCallback callback) { suite_end_callback_ = callback; }
    
private:
    std::vector<std::shared_ptr<TestSuite>> test_suites_;
    std::vector<TestResult> all_results_;
    
    // Configuration
    bool parallel_execution_ = false;
    int max_threads_ = 1;
    std::string output_format_ = "console";
    std::string output_file_;
    bool verbose_mode_ = false;
    uint32_t random_seed_ = 0;
    bool shuffle_tests_ = false;
    
    // Filtering
    std::string test_filter_;
    std::vector<TestCategory> category_filter_;
    std::vector<std::string> tag_filter_;
    TestPriority priority_filter_ = TestPriority::LOW;
    
    // Callbacks
    TestStartCallback test_start_callback_;
    TestEndCallback test_end_callback_;
    SuiteStartCallback suite_start_callback_;
    SuiteEndCallback suite_end_callback_;
    
    // Internal methods
    std::vector<std::shared_ptr<TestCase>> GetFilteredTests() const;
    void ExecuteTestsParallel(const std::vector<std::shared_ptr<TestCase>>& tests);
    void ExecuteTestsSequential(const std::vector<std::shared_ptr<TestCase>>& tests);
    void WriteConsoleReport() const;
    void WriteXmlReport() const;
    void WriteJsonReport() const;
    
    bool MatchesFilter(const TestCase& test) const;
    void ShuffleTests(std::vector<std::shared_ptr<TestCase>>& tests) const;
};

// Assertion macros and functions
#define HFX_ASSERT_TRUE(condition) \
    do { \
        if (!(condition)) { \
            throw hfx::testing::AssertionException("Assertion failed: " #condition " is not true"); \
        } \
    } while(0)

#define HFX_ASSERT_FALSE(condition) \
    do { \
        if (condition) { \
            throw hfx::testing::AssertionException("Assertion failed: " #condition " is not false"); \
        } \
    } while(0)

#define HFX_ASSERT_EQ(expected, actual) \
    do { \
        if (!((expected) == (actual))) { \
            std::stringstream ss; \
            ss << "Assertion failed: Expected " << (expected) << " but got " << (actual); \
            throw hfx::testing::AssertionException(ss.str()); \
        } \
    } while(0)

#define HFX_ASSERT_NE(expected, actual) \
    do { \
        if ((expected) == (actual)) { \
            std::stringstream ss; \
            ss << "Assertion failed: Expected not equal to " << (expected) << " but got " << (actual); \
            throw hfx::testing::AssertionException(ss.str()); \
        } \
    } while(0)

#define HFX_ASSERT_LT(val1, val2) \
    do { \
        if (!((val1) < (val2))) { \
            std::stringstream ss; \
            ss << "Assertion failed: " << (val1) << " is not less than " << (val2); \
            throw hfx::testing::AssertionException(ss.str()); \
        } \
    } while(0)

#define HFX_ASSERT_LE(val1, val2) \
    do { \
        if (!((val1) <= (val2))) { \
            std::stringstream ss; \
            ss << "Assertion failed: " << (val1) << " is not less than or equal to " << (val2); \
            throw hfx::testing::AssertionException(ss.str()); \
        } \
    } while(0)

#define HFX_ASSERT_GT(val1, val2) \
    do { \
        if (!((val1) > (val2))) { \
            std::stringstream ss; \
            ss << "Assertion failed: " << (val1) << " is not greater than " << (val2); \
            throw hfx::testing::AssertionException(ss.str()); \
        } \
    } while(0)

#define HFX_ASSERT_GE(val1, val2) \
    do { \
        if (!((val1) >= (val2))) { \
            std::stringstream ss; \
            ss << "Assertion failed: " << (val1) << " is not greater than or equal to " << (val2); \
            throw hfx::testing::AssertionException(ss.str()); \
        } \
    } while(0)

#define HFX_ASSERT_NEAR(val1, val2, tolerance) \
    do { \
        double diff = std::abs((val1) - (val2)); \
        if (diff > (tolerance)) { \
            std::stringstream ss; \
            ss << "Assertion failed: " << (val1) << " is not within " << (tolerance) << " of " << (val2) << " (diff: " << diff << ")"; \
            throw hfx::testing::AssertionException(ss.str()); \
        } \
    } while(0)

#define HFX_ASSERT_THROW(statement, exception_type) \
    do { \
        bool threw_expected = false; \
        try { \
            statement; \
        } catch (const exception_type&) { \
            threw_expected = true; \
        } catch (...) { \
            throw hfx::testing::AssertionException("Assertion failed: " #statement " threw unexpected exception type"); \
        } \
        if (!threw_expected) { \
            throw hfx::testing::AssertionException("Assertion failed: " #statement " did not throw " #exception_type); \
        } \
    } while(0)

#define HFX_ASSERT_NO_THROW(statement) \
    do { \
        try { \
            statement; \
        } catch (...) { \
            throw hfx::testing::AssertionException("Assertion failed: " #statement " threw an exception"); \
        } \
    } while(0)

#define HFX_ASSERT_STREQ(str1, str2) \
    do { \
        std::string s1(str1); \
        std::string s2(str2); \
        if (s1 != s2) { \
            std::stringstream ss; \
            ss << "Assertion failed: String \"" << s1 << "\" does not equal \"" << s2 << "\""; \
            throw hfx::testing::AssertionException(ss.str()); \
        } \
    } while(0)

#define HFX_ASSERT_STRNE(str1, str2) \
    do { \
        std::string s1(str1); \
        std::string s2(str2); \
        if (s1 == s2) { \
            std::stringstream ss; \
            ss << "Assertion failed: String \"" << s1 << "\" equals \"" << s2 << "\" but should not"; \
            throw hfx::testing::AssertionException(ss.str()); \
        } \
    } while(0)

// Performance measurement macros
#define HFX_BENCHMARK_START(name) StartPerformanceMeasurement(name)
#define HFX_BENCHMARK_END(name) EndPerformanceMeasurement(name)

#define HFX_MEASURE_LATENCY(name, code) \
    do { \
        auto start = std::chrono::high_resolution_clock::now(); \
        code; \
        auto end = std::chrono::high_resolution_clock::now(); \
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start); \
        RecordPerformanceMetric(PerformanceMetric::LATENCY_NS, name, static_cast<double>(duration.count())); \
    } while(0)

// Test registration macros
#define HFX_TEST_CLASS(test_name) \
    class test_name : public hfx::testing::TestCase { \
    public: \
        test_name() : TestCase(#test_name) {} \
        void Run() override; \
    }; \
    void test_name::Run()

#define HFX_PERFORMANCE_TEST_CLASS(test_name) \
    class test_name : public hfx::testing::TestCase { \
    public: \
        test_name() : TestCase(#test_name, hfx::testing::TestCategory::PERFORMANCE) {} \
        void Run() override; \
    }; \
    void test_name::Run()

#define HFX_INTEGRATION_TEST_CLASS(test_name) \
    class test_name : public hfx::testing::TestCase { \
    public: \
        test_name() : TestCase(#test_name, hfx::testing::TestCategory::INTEGRATION) {} \
        void Run() override; \
    }; \
    void test_name::Run()

// Factory for creating test runners with different configurations
class TestRunnerFactory {
public:
    // Pre-configured test runners
    static std::unique_ptr<TestRunner> CreateUnitTestRunner();
    static std::unique_ptr<TestRunner> CreateIntegrationTestRunner();
    static std::unique_ptr<TestRunner> CreatePerformanceTestRunner();
    static std::unique_ptr<TestRunner> CreateContinuousIntegrationRunner();
    
    // Custom configurations
    static std::unique_ptr<TestRunner> CreateCustomRunner(
        bool parallel = true, 
        const std::string& output_format = "console",
        bool verbose = false);
};

// Utility functions for test management
namespace test_utils {
    // Test discovery
    std::vector<std::string> DiscoverTestFiles(const std::string& directory, const std::string& pattern = ".*test.*\\.cpp");
    
    // Temporary file management for tests
    std::string CreateTempFile(const std::string& prefix = "hfx_test_");
    std::string CreateTempDirectory(const std::string& prefix = "hfx_test_dir_");
    void CleanupTempFile(const std::string& filepath);
    void CleanupTempDirectory(const std::string& dirpath);
    
    // Test data generation
    std::vector<uint8_t> GenerateRandomData(size_t size);
    std::string GenerateRandomString(size_t length);
    double GenerateRandomDouble(double min_val = 0.0, double max_val = 1.0);
    
    // Time utilities for tests
    void SleepFor(std::chrono::milliseconds duration);
    std::chrono::system_clock::time_point GetCurrentTime();
    bool WaitFor(std::function<bool()> condition, std::chrono::milliseconds timeout = std::chrono::milliseconds(5000));
    
    // String utilities
    std::vector<std::string> SplitString(const std::string& str, char delimiter);
    std::string TrimString(const std::string& str);
    bool StringContains(const std::string& haystack, const std::string& needle);
    
    // File utilities for testing
    bool FileExists(const std::string& filepath);
    std::string ReadFileContent(const std::string& filepath);
    void WriteFileContent(const std::string& filepath, const std::string& content);
    
    // Network utilities for testing
    bool IsPortOpen(const std::string& host, int port);
    int FindAvailablePort(int start_port = 8000, int end_port = 9000);
    
    // Memory utilities
    size_t GetCurrentMemoryUsage();
    size_t GetPeakMemoryUsage();
    
    // Process utilities
    std::string ExecuteCommand(const std::string& command);
    bool IsProcessRunning(const std::string& process_name);
}

} // namespace hfx::testing
