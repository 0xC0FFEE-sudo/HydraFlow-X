/**
 * @file testing_framework.cpp
 * @brief Comprehensive testing framework implementation for HydraFlow-X
 */

#include "testing_framework.hpp"
#include <iostream>
#include <algorithm>
#include <numeric>
#include <random>
#include <filesystem>
#include <regex>
#include <fstream>
#include <thread>
#include <future>
#include <chrono>
#include <iomanip>
#include <cstring>
#include <sys/stat.h>
#include <unistd.h>

#ifdef __APPLE__
#include <mach/mach.h>
#include <sys/resource.h>
#endif

namespace hfx::testing {

// TestCase implementation
TestCase::TestCase(const std::string& name, TestCategory category, TestPriority priority)
    : name_(name), category_(category), priority_(priority) {
}

TestResult TestCase::Execute() {
    TestResult result;
    result.test_name = name_;
    result.status = TestStatus::RUNNING;
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    try {
        // Setup phase
        SetUp();
        
        // Run the actual test
        Run();
        
        // Test passed if no exceptions thrown
        result.status = TestStatus::PASSED;
        
    } catch (const AssertionException& e) {
        result.status = TestStatus::FAILED;
        result.error_message = e.what();
        result.failure_details = "Assertion failure in test: " + name_;
        
    } catch (const std::exception& e) {
        result.status = TestStatus::ERROR;
        result.error_message = e.what();
        result.failure_details = "Exception thrown in test: " + name_;
        
    } catch (...) {
        result.status = TestStatus::ERROR;
        result.error_message = "Unknown exception";
        result.failure_details = "Unknown exception thrown in test: " + name_;
    }
    
    try {
        // Cleanup phase (always run)
        TearDown();
    } catch (const std::exception& e) {
        if (result.status == TestStatus::PASSED) {
            result.status = TestStatus::ERROR;
            result.error_message = "TearDown failed: " + std::string(e.what());
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    result.execution_time = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);
    
    // Copy performance results
    for (const auto& perf_result : performance_results_) {
        result.performance_metrics[perf_result.metric_type] = perf_result.value;
    }
    
    return result;
}

void TestCase::StartPerformanceMeasurement(const std::string& metric_name) {
    perf_start_times_[metric_name] = std::chrono::high_resolution_clock::now();
}

void TestCase::EndPerformanceMeasurement(const std::string& metric_name) {
    auto end_time = std::chrono::high_resolution_clock::now();
    auto it = perf_start_times_.find(metric_name);
    
    if (it != perf_start_times_.end()) {
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - it->second);
        RecordPerformanceMetric(PerformanceMetric::LATENCY_NS, metric_name, static_cast<double>(duration.count()));
        perf_start_times_.erase(it);
    }
}

void TestCase::RecordPerformanceMetric(PerformanceMetric type, const std::string& name, double value) {
    PerformanceResult result;
    result.metric_type = type;
    result.metric_name = name;
    result.value = value;
    
    switch (type) {
        case PerformanceMetric::LATENCY_NS:
            result.unit = "nanoseconds";
            break;
        case PerformanceMetric::THROUGHPUT_OPS:
            result.unit = "operations/second";
            break;
        case PerformanceMetric::MEMORY_BYTES:
            result.unit = "bytes";
            break;
        case PerformanceMetric::CPU_PERCENT:
            result.unit = "percent";
            break;
        case PerformanceMetric::CUSTOM:
            result.unit = "custom";
            break;
    }
    
    performance_results_.push_back(result);
}

// TestSuite implementation
TestSuite::TestSuite(const std::string& name) : name_(name) {
}

void TestSuite::AddTest(std::unique_ptr<TestCase> test) {
    tests_.push_back(std::move(test));
}

void TestSuite::AddTest(std::shared_ptr<TestCase> test) {
    tests_.push_back(test);
}

std::vector<TestResult> TestSuite::RunAllTests() {
    std::vector<TestResult> results;
    
    SetUpSuite();
    
    for (auto& test : tests_) {
        try {
            TestResult result = test->Execute();
            result.suite_name = name_;
            results.push_back(result);
        } catch (const std::exception& e) {
            TestResult result;
            result.test_name = test->GetName();
            result.suite_name = name_;
            result.status = TestStatus::ERROR;
            result.error_message = "Failed to execute test: " + std::string(e.what());
            results.push_back(result);
        }
    }
    
    TearDownSuite();
    
    return results;
}

std::vector<TestResult> TestSuite::RunTestsWithTag(const std::string& tag) {
    std::vector<TestResult> results;
    
    SetUpSuite();
    
    for (auto& test : tests_) {
        const auto& tags = test->GetTags();
        if (std::find(tags.begin(), tags.end(), tag) != tags.end()) {
            TestResult result = test->Execute();
            result.suite_name = name_;
            results.push_back(result);
        }
    }
    
    TearDownSuite();
    
    return results;
}

std::vector<TestResult> TestSuite::RunTestsByCategory(TestCategory category) {
    std::vector<TestResult> results;
    
    SetUpSuite();
    
    for (auto& test : tests_) {
        if (test->GetCategory() == category) {
            TestResult result = test->Execute();
            result.suite_name = name_;
            results.push_back(result);
        }
    }
    
    TearDownSuite();
    
    return results;
}

std::vector<TestResult> TestSuite::RunTestsByPriority(TestPriority min_priority) {
    std::vector<TestResult> results;
    
    SetUpSuite();
    
    for (auto& test : tests_) {
        if (test->GetPriority() >= min_priority) {
            TestResult result = test->Execute();
            result.suite_name = name_;
            results.push_back(result);
        }
    }
    
    TearDownSuite();
    
    return results;
}

std::vector<std::string> TestSuite::GetAllTags() const {
    std::vector<std::string> all_tags;
    
    for (const auto& test : tests_) {
        const auto& tags = test->GetTags();
        all_tags.insert(all_tags.end(), tags.begin(), tags.end());
    }
    
    // Remove duplicates
    std::sort(all_tags.begin(), all_tags.end());
    all_tags.erase(std::unique(all_tags.begin(), all_tags.end()), all_tags.end());
    
    return all_tags;
}

// MockObject implementation
template<typename T>
void MockObject::ExpectCall(const std::string& method_name, const T& return_value) {
    std::lock_guard<std::mutex> lock(mock_mutex_);
    
    MockExpectation expectation;
    expectation.method_name = method_name;
    expectation.behavior = MockBehavior::RETURN_VALUE;
    expectation.return_value = return_value;
    
    expectations_.push_back(expectation);
}

void MockObject::ExpectCallWithException(const std::string& method_name, const std::exception& exception) {
    std::lock_guard<std::mutex> lock(mock_mutex_);
    
    MockExpectation expectation;
    expectation.method_name = method_name;
    expectation.behavior = MockBehavior::THROW_EXCEPTION;
    expectation.exception = std::make_exception_ptr(exception);
    
    expectations_.push_back(expectation);
}

void MockObject::ExpectCallCount(const std::string& method_name, int expected_count) {
    std::lock_guard<std::mutex> lock(mock_mutex_);
    
    // Find existing expectation or create new one
    auto it = std::find_if(expectations_.begin(), expectations_.end(),
                          [&method_name](const MockExpectation& exp) {
                              return exp.method_name == method_name;
                          });
    
    if (it != expectations_.end()) {
        it->expected_call_count = expected_count;
    } else {
        MockExpectation expectation;
        expectation.method_name = method_name;
        expectation.behavior = MockBehavior::IGNORE;
        expectation.expected_call_count = expected_count;
        expectations_.push_back(expectation);
    }
}

bool MockObject::VerifyExpectations() const {
    std::lock_guard<std::mutex> lock(mock_mutex_);
    
    for (const auto& expectation : expectations_) {
        if (expectation.expected_call_count >= 0) {
            if (expectation.actual_call_count != expectation.expected_call_count) {
                return false;
            }
        }
    }
    
    return true;
}

std::vector<std::string> MockObject::GetUnmetExpectations() const {
    std::lock_guard<std::mutex> lock(mock_mutex_);
    std::vector<std::string> unmet;
    
    for (const auto& expectation : expectations_) {
        if (expectation.expected_call_count >= 0) {
            if (expectation.actual_call_count != expectation.expected_call_count) {
                std::stringstream ss;
                ss << "Method '" << expectation.method_name 
                   << "' expected " << expectation.expected_call_count 
                   << " calls but got " << expectation.actual_call_count;
                unmet.push_back(ss.str());
            }
        }
    }
    
    return unmet;
}

void MockObject::RecordCall(const std::string& method_name) {
    std::lock_guard<std::mutex> lock(mock_mutex_);
    
    call_counts_[method_name]++;
    
    // Update expectation actual call count
    for (auto& expectation : expectations_) {
        if (expectation.method_name == method_name) {
            expectation.actual_call_count++;
        }
    }
}

int MockObject::GetCallCount(const std::string& method_name) const {
    std::lock_guard<std::mutex> lock(mock_mutex_);
    
    auto it = call_counts_.find(method_name);
    return it != call_counts_.end() ? it->second : 0;
}

// PerformanceBenchmark implementation
PerformanceBenchmark::PerformanceBenchmark(const std::string& name) : name_(name) {
}

template<typename Func>
PerformanceResult PerformanceBenchmark::MeasureLatency(Func&& func, int iterations) {
    PerformanceResult result;
    result.metric_type = PerformanceMetric::LATENCY_NS;
    result.metric_name = name_ + "_latency";
    result.unit = "nanoseconds";
    result.samples.reserve(iterations);
    
    // Warm-up
    for (int i = 0; i < std::min(10, iterations / 10); ++i) {
        func();
    }
    
    // Actual measurements
    for (int i = 0; i < iterations; ++i) {
        auto start = std::chrono::high_resolution_clock::now();
        func();
        auto end = std::chrono::high_resolution_clock::now();
        
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
        result.samples.push_back(static_cast<double>(duration.count()));
    }
    
    // Calculate statistics
    result.mean_value = CalculateMean(result.samples);
    result.median_value = CalculateMedian(result.samples);
    result.min_value = *std::min_element(result.samples.begin(), result.samples.end());
    result.max_value = *std::max_element(result.samples.begin(), result.samples.end());
    result.std_deviation = CalculateStdDeviation(result.samples, result.mean_value);
    result.percentile_95 = CalculatePercentile(result.samples, 95.0);
    result.percentile_99 = CalculatePercentile(result.samples, 99.0);
    result.value = result.mean_value;
    
    return result;
}

template<typename Func>
PerformanceResult PerformanceBenchmark::MeasureThroughput(Func&& func, std::chrono::milliseconds duration) {
    PerformanceResult result;
    result.metric_type = PerformanceMetric::THROUGHPUT_OPS;
    result.metric_name = name_ + "_throughput";
    result.unit = "operations/second";
    
    auto start_time = std::chrono::high_resolution_clock::now();
    auto end_time = start_time + duration;
    
    int operations = 0;
    while (std::chrono::high_resolution_clock::now() < end_time) {
        func();
        operations++;
    }
    
    auto actual_duration = std::chrono::high_resolution_clock::now() - start_time;
    auto duration_seconds = std::chrono::duration_cast<std::chrono::duration<double>>(actual_duration).count();
    
    result.value = operations / duration_seconds;
    result.mean_value = result.value;
    
    return result;
}

PerformanceResult PerformanceBenchmark::MeasureMemoryUsage(std::function<void()> func) {
    PerformanceResult result;
    result.metric_type = PerformanceMetric::MEMORY_BYTES;
    result.metric_name = name_ + "_memory";
    result.unit = "bytes";
    
    size_t memory_before = GetMemoryUsage();
    func();
    size_t memory_after = GetMemoryUsage();
    
    result.value = static_cast<double>(memory_after - memory_before);
    result.mean_value = result.value;
    
    return result;
}

double PerformanceBenchmark::CalculateMean(const std::vector<double>& values) {
    if (values.empty()) return 0.0;
    return std::accumulate(values.begin(), values.end(), 0.0) / values.size();
}

double PerformanceBenchmark::CalculateMedian(std::vector<double> values) {
    if (values.empty()) return 0.0;
    
    std::sort(values.begin(), values.end());
    size_t size = values.size();
    
    if (size % 2 == 0) {
        return (values[size / 2 - 1] + values[size / 2]) / 2.0;
    } else {
        return values[size / 2];
    }
}

double PerformanceBenchmark::CalculateStdDeviation(const std::vector<double>& values, double mean) {
    if (values.empty()) return 0.0;
    
    double sum_squared_diff = 0.0;
    for (double value : values) {
        double diff = value - mean;
        sum_squared_diff += diff * diff;
    }
    
    return std::sqrt(sum_squared_diff / values.size());
}

double PerformanceBenchmark::CalculatePercentile(std::vector<double> values, double percentile) {
    if (values.empty()) return 0.0;
    
    std::sort(values.begin(), values.end());
    
    double index = (percentile / 100.0) * (values.size() - 1);
    size_t lower_index = static_cast<size_t>(std::floor(index));
    size_t upper_index = static_cast<size_t>(std::ceil(index));
    
    if (lower_index == upper_index) {
        return values[lower_index];
    }
    
    double weight = index - lower_index;
    return values[lower_index] * (1.0 - weight) + values[upper_index] * weight;
}

bool PerformanceBenchmark::ComparePerformance(const PerformanceResult& baseline, 
                                             const PerformanceResult& current, 
                                             double tolerance_percent) const {
    if (baseline.value == 0.0) return true;
    
    double difference_percent = std::abs((current.value - baseline.value) / baseline.value) * 100.0;
    return difference_percent <= tolerance_percent;
}

size_t PerformanceBenchmark::GetMemoryUsage() const {
#ifdef __APPLE__
    struct mach_task_basic_info info;
    mach_msg_type_number_t size = MACH_TASK_BASIC_INFO_COUNT;
    kern_return_t kerr = task_info(mach_task_self(), MACH_TASK_BASIC_INFO, 
                                  (task_info_t)&info, &size);
    if (kerr == KERN_SUCCESS) {
        return info.resident_size;
    }
#endif
    return 0;
}

double PerformanceBenchmark::GetCpuUsage() const {
    // Simplified CPU usage measurement
    // In a real implementation, this would measure actual CPU usage
    return 0.0;
}

// TestRunner implementation
TestRunner::TestRunner() {
    random_seed_ = static_cast<uint32_t>(std::chrono::system_clock::now().time_since_epoch().count());
}

TestRunner::~TestRunner() = default;

void TestRunner::SetParallelExecution(bool enabled, int max_threads) {
    parallel_execution_ = enabled;
    max_threads_ = max_threads;
}

void TestRunner::SetOutputFormat(const std::string& format) {
    if (format == "console" || format == "xml" || format == "json") {
        output_format_ = format;
    } else {
        throw std::invalid_argument("Unsupported output format: " + format);
    }
}

void TestRunner::SetOutputFile(const std::string& filepath) {
    output_file_ = filepath;
}

void TestRunner::RegisterTestSuite(std::shared_ptr<TestSuite> suite) {
    test_suites_.push_back(suite);
}

void TestRunner::DiscoverTests(const std::string& directory_path) {
    // Simplified test discovery - in real implementation would scan for test files
    HFX_LOG_INFO("[LOG] Message");
    
    if (std::filesystem::exists(directory_path)) {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(directory_path)) {
            if (entry.path().filename().string().find("test") != std::string::npos &&
                entry.path().extension() == ".cpp") {
                HFX_LOG_INFO("[LOG] Message");
            }
        }
    }
}

void TestRunner::SetTestFilter(const std::string& filter_pattern) {
    test_filter_ = filter_pattern;
}

void TestRunner::SetCategoryFilter(const std::vector<TestCategory>& categories) {
    category_filter_ = categories;
}

void TestRunner::SetTagFilter(const std::vector<std::string>& tags) {
    tag_filter_ = tags;
}

void TestRunner::SetPriorityFilter(TestPriority min_priority) {
    priority_filter_ = min_priority;
}

void TestRunner::RunAllTests() {
    HFX_LOG_INFO("🚀 Starting test execution...");
    HFX_LOG_INFO("   Configuration:");
    HFX_LOG_INFO("[LOG] Message");
    HFX_LOG_INFO("[LOG] Message");
    HFX_LOG_INFO("[LOG] Message");
    HFX_LOG_INFO("[LOG] Message");
    
    all_results_.clear();
    
    for (auto& suite : test_suites_) {
        if (suite_start_callback_) {
            suite_start_callback_(suite->GetName());
        }
        
        HFX_LOG_INFO("[LOG] Message");
        
        auto suite_results = suite->RunAllTests();
        all_results_.insert(all_results_.end(), suite_results.begin(), suite_results.end());
        
        if (suite_end_callback_) {
            suite_end_callback_(suite->GetName(), suite_results);
        }
        
        // Print suite summary
        int passed = 0, failed = 0, errors = 0;
        for (const auto& result : suite_results) {
            switch (result.status) {
                case TestStatus::PASSED: passed++; break;
                case TestStatus::FAILED: failed++; break;
                case TestStatus::ERROR: errors++; break;
                default: break;
            }
        }
        
        HFX_LOG_INFO("[LOG] Message");
                  << failed << " failed, " << errors << " errors" << std::endl;
    }
    
    GenerateReport();
}

void TestRunner::RunFilteredTests() {
    auto filtered_tests = GetFilteredTests();
    
    HFX_LOG_INFO("[LOG] Message");
    
    if (parallel_execution_ && filtered_tests.size() > 1) {
        ExecuteTestsParallel(filtered_tests);
    } else {
        ExecuteTestsSequential(filtered_tests);
    }
    
    GenerateReport();
}

void TestRunner::RunSuite(const std::string& suite_name) {
    auto it = std::find_if(test_suites_.begin(), test_suites_.end(),
                          [&suite_name](const std::shared_ptr<TestSuite>& suite) {
                              return suite->GetName() == suite_name;
                          });
    
    if (it != test_suites_.end()) {
        HFX_LOG_INFO("[LOG] Message");
        auto results = (*it)->RunAllTests();
        all_results_.insert(all_results_.end(), results.begin(), results.end());
        GenerateReport();
    } else {
        HFX_LOG_ERROR("[ERROR] Message");
    }
}

TestRunner::TestStatistics TestRunner::GetStatistics() const {
    TestStatistics stats;
    
    for (const auto& result : all_results_) {
        stats.total_tests++;
        
        switch (result.status) {
            case TestStatus::PASSED:
                stats.passed_tests++;
                break;
            case TestStatus::FAILED:
                stats.failed_tests++;
                break;
            case TestStatus::SKIPPED:
                stats.skipped_tests++;
                break;
            case TestStatus::ERROR:
                stats.error_tests++;
                break;
            default:
                break;
        }
        
        stats.total_execution_time += std::chrono::duration_cast<std::chrono::milliseconds>(result.execution_time);
        
        // Performance statistics
        auto latency_it = result.performance_metrics.find(PerformanceMetric::LATENCY_NS);
        if (latency_it != result.performance_metrics.end()) {
            stats.avg_latency_ns = (stats.avg_latency_ns + latency_it->second) / 2.0;
            stats.max_latency_ns = std::max(stats.max_latency_ns, latency_it->second);
        }
        
        auto memory_it = result.performance_metrics.find(PerformanceMetric::MEMORY_BYTES);
        if (memory_it != result.performance_metrics.end()) {
            double memory_mb = memory_it->second / (1024.0 * 1024.0);
            stats.avg_memory_usage_mb = (stats.avg_memory_usage_mb + memory_mb) / 2.0;
            stats.max_memory_usage_mb = std::max(stats.max_memory_usage_mb, memory_mb);
        }
    }
    
    if (stats.total_tests > 0) {
        stats.success_rate = (static_cast<double>(stats.passed_tests) / stats.total_tests) * 100.0;
    }
    
    return stats;
}

std::vector<std::shared_ptr<TestCase>> TestRunner::GetFilteredTests() const {
    std::vector<std::shared_ptr<TestCase>> filtered_tests;
    
    // This is a simplified version - would need access to individual tests in suite
    // In a real implementation, TestSuite would provide access to individual tests
    (void)test_suites_; // Suppress unused variable warning
    
    return filtered_tests;
}

void TestRunner::ExecuteTestsParallel(const std::vector<std::shared_ptr<TestCase>>& tests) {
    HFX_LOG_INFO("[LOG] Message");
    
    std::vector<std::future<TestResult>> futures;
    
    for (auto& test : tests) {
        auto future = std::async(std::launch::async, [test]() {
            return test->Execute();
        });
        futures.push_back(std::move(future));
    }
    
    for (auto& future : futures) {
        try {
            TestResult result = future.get();
            all_results_.push_back(result);
            
            if (test_end_callback_) {
                test_end_callback_(result);
            }
            
        } catch (const std::exception& e) {
            HFX_LOG_ERROR("[ERROR] Message");
        }
    }
}

void TestRunner::ExecuteTestsSequential(const std::vector<std::shared_ptr<TestCase>>& tests) {
    HFX_LOG_INFO("🔄 Executing tests sequentially");
    
    for (auto& test : tests) {
        TestContext context;
        context.test_name = test->GetName();
        context.category = test->GetCategory();
        context.priority = test->GetPriority();
        context.start_time = std::chrono::system_clock::now();
        
        if (test_start_callback_) {
            test_start_callback_(context);
        }
        
        TestResult result = test->Execute();
        all_results_.push_back(result);
        
        if (test_end_callback_) {
            test_end_callback_(result);
        }
    }
}

void TestRunner::GenerateReport() {
    HFX_LOG_INFO("\n📊 Generating test report...");
    
    if (output_format_ == "console") {
        WriteConsoleReport();
    } else if (output_format_ == "xml") {
        WriteXmlReport();
    } else if (output_format_ == "json") {
        WriteJsonReport();
    }
}

void TestRunner::GeneratePerformanceReport() {
    HFX_LOG_INFO("\n⚡ Generating performance report...");
    
    std::string filename = output_file_.empty() ? "performance_report.json" : 
                          output_file_.substr(0, output_file_.find_last_of('.')) + "_performance.json";
    
    std::ofstream file(filename);
    if (!file.is_open()) {
        HFX_LOG_ERROR("[ERROR] Message");
        return;
    }
    
    auto stats = GetStatistics();
    
    file << "{\n";
    file << "  \"performance_summary\": {\n";
    file << "    \"avg_latency_ns\": " << stats.avg_latency_ns << ",\n";
    file << "    \"max_latency_ns\": " << stats.max_latency_ns << ",\n";
    file << "    \"avg_memory_usage_mb\": " << stats.avg_memory_usage_mb << ",\n";
    file << "    \"max_memory_usage_mb\": " << stats.max_memory_usage_mb << ",\n";
    file << "    \"total_execution_time_ms\": " << stats.total_execution_time.count() << "\n";
    file << "  },\n";
    
    file << "  \"performance_details\": [\n";
    for (size_t i = 0; i < all_results_.size(); ++i) {
        const auto& result = all_results_[i];
        if (!result.performance_metrics.empty()) {
            file << "    {\n";
            file << "      \"test_name\": \"" << result.test_name << "\",\n";
            file << "      \"metrics\": {\n";
            
            size_t metric_count = 0;
            for (const auto& [metric_type, value] : result.performance_metrics) {
                if (metric_count > 0) file << ",\n";
                
                std::string metric_name;
                switch (metric_type) {
                    case PerformanceMetric::LATENCY_NS: metric_name = "latency_ns"; break;
                    case PerformanceMetric::THROUGHPUT_OPS: metric_name = "throughput_ops"; break;
                    case PerformanceMetric::MEMORY_BYTES: metric_name = "memory_bytes"; break;
                    case PerformanceMetric::CPU_PERCENT: metric_name = "cpu_percent"; break;
                    default: metric_name = "custom"; break;
                }
                
                file << "        \"" << metric_name << "\": " << value;
                metric_count++;
            }
            
            file << "\n      }\n";
            file << "    }";
            
            if (i < all_results_.size() - 1) {
                // Check if there are more results with performance metrics
                bool has_more_performance = false;
                for (size_t j = i + 1; j < all_results_.size(); ++j) {
                    if (!all_results_[j].performance_metrics.empty()) {
                        has_more_performance = true;
                        break;
                    }
                }
                if (has_more_performance) file << ",";
            }
            file << "\n";
        }
    }
    file << "  ]\n";
    file << "}\n";
    
    file.close();
    HFX_LOG_INFO("[LOG] Message");
}

void TestRunner::WriteConsoleReport() const {
    auto stats = GetStatistics();
    
    HFX_LOG_INFO("[LOG] Message");
    HFX_LOG_INFO("🎯 TEST EXECUTION SUMMARY");
    HFX_LOG_INFO("[LOG] Message");
    
    HFX_LOG_INFO("[LOG] Message");
    HFX_LOG_INFO("[LOG] Message");
    HFX_LOG_INFO("[LOG] Message");
    HFX_LOG_INFO("[LOG] Message");
    HFX_LOG_INFO("[LOG] Message");
    HFX_LOG_INFO("[LOG] Message");
              << stats.success_rate << "%" << std::endl;
    HFX_LOG_INFO("[LOG] Message");
    
    if (stats.avg_latency_ns > 0) {
        HFX_LOG_INFO("\n⚡ PERFORMANCE METRICS");
        HFX_LOG_INFO("[LOG] Message");
                  << stats.avg_latency_ns << "ns" << std::endl;
        HFX_LOG_INFO("[LOG] Message");
                  << stats.max_latency_ns << "ns" << std::endl;
        
        if (stats.avg_memory_usage_mb > 0) {
            HFX_LOG_INFO("[LOG] Message");
                      << stats.avg_memory_usage_mb << "MB" << std::endl;
            HFX_LOG_INFO("[LOG] Message");
                      << stats.max_memory_usage_mb << "MB" << std::endl;
        }
    }
    
    // Show failed tests
    if (stats.failed_tests > 0 || stats.error_tests > 0) {
        HFX_LOG_INFO("\n❌ FAILED TESTS");
        HFX_LOG_INFO("[LOG] Message");
        
        for (const auto& result : all_results_) {
            if (result.status == TestStatus::FAILED || result.status == TestStatus::ERROR) {
                HFX_LOG_INFO("[LOG] Message");
                HFX_LOG_INFO("[LOG] Message");
                if (!result.failure_details.empty()) {
                    HFX_LOG_INFO("[LOG] Message");
                }
                HFX_LOG_INFO("[LOG] Message");
            }
        }
    }
    
    HFX_LOG_INFO("[LOG] Message");
    
    if (stats.failed_tests == 0 && stats.error_tests == 0) {
        HFX_LOG_INFO("🎉 ALL TESTS PASSED! 🎉");
    } else {
        HFX_LOG_INFO("❌ SOME TESTS FAILED");
    }
    
    HFX_LOG_INFO("[LOG] Message");
}

void TestRunner::WriteXmlReport() const {
    std::string filename = output_file_.empty() ? "test_results.xml" : output_file_;
    std::ofstream file(filename);
    
    if (!file.is_open()) {
        HFX_LOG_ERROR("[ERROR] Message");
        return;
    }
    
    auto stats = GetStatistics();
    
    file << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    file << "<testsuites tests=\"" << stats.total_tests 
         << "\" failures=\"" << stats.failed_tests 
         << "\" errors=\"" << stats.error_tests 
         << "\" time=\"" << stats.total_execution_time.count() / 1000.0 << "\">\n";
    
    std::unordered_map<std::string, std::vector<TestResult>> suites;
    for (const auto& result : all_results_) {
        suites[result.suite_name].push_back(result);
    }
    
    for (const auto& [suite_name, suite_results] : suites) {
        file << "  <testsuite name=\"" << suite_name 
             << "\" tests=\"" << suite_results.size() << "\">\n";
        
        for (const auto& result : suite_results) {
            file << "    <testcase name=\"" << result.test_name 
                 << "\" time=\"" << result.execution_time.count() / 1000000000.0 << "\"";
            
            if (result.status == TestStatus::PASSED) {
                file << "/>\n";
            } else {
                file << ">\n";
                
                if (result.status == TestStatus::FAILED) {
                    file << "      <failure message=\"" << result.error_message 
                         << "\">" << result.failure_details << "</failure>\n";
                } else if (result.status == TestStatus::ERROR) {
                    file << "      <error message=\"" << result.error_message 
                         << "\">" << result.failure_details << "</error>\n";
                }
                
                file << "    </testcase>\n";
            }
        }
        
        file << "  </testsuite>\n";
    }
    
    file << "</testsuites>\n";
    file.close();
    
    HFX_LOG_INFO("[LOG] Message");
}

void TestRunner::WriteJsonReport() const {
    std::string filename = output_file_.empty() ? "test_results.json" : output_file_;
    std::ofstream file(filename);
    
    if (!file.is_open()) {
        HFX_LOG_ERROR("[ERROR] Message");
        return;
    }
    
    auto stats = GetStatistics();
    
    file << "{\n";
    file << "  \"summary\": {\n";
    file << "    \"total_tests\": " << stats.total_tests << ",\n";
    file << "    \"passed_tests\": " << stats.passed_tests << ",\n";
    file << "    \"failed_tests\": " << stats.failed_tests << ",\n";
    file << "    \"error_tests\": " << stats.error_tests << ",\n";
    file << "    \"skipped_tests\": " << stats.skipped_tests << ",\n";
    file << "    \"success_rate\": " << stats.success_rate << ",\n";
    file << "    \"total_execution_time_ms\": " << stats.total_execution_time.count() << "\n";
    file << "  },\n";
    
    file << "  \"test_results\": [\n";
    
    for (size_t i = 0; i < all_results_.size(); ++i) {
        const auto& result = all_results_[i];
        
        file << "    {\n";
        file << "      \"test_name\": \"" << result.test_name << "\",\n";
        file << "      \"suite_name\": \"" << result.suite_name << "\",\n";
        file << "      \"status\": \"";
        
        switch (result.status) {
            case TestStatus::PASSED: file << "PASSED"; break;
            case TestStatus::FAILED: file << "FAILED"; break;
            case TestStatus::ERROR: file << "ERROR"; break;
            case TestStatus::SKIPPED: file << "SKIPPED"; break;
            default: file << "UNKNOWN"; break;
        }
        
        file << "\",\n";
        file << "      \"execution_time_ns\": " << result.execution_time.count() << ",\n";
        file << "      \"error_message\": \"" << result.error_message << "\",\n";
        file << "      \"failure_details\": \"" << result.failure_details << "\"\n";
        file << "    }";
        
        if (i < all_results_.size() - 1) {
            file << ",";
        }
        file << "\n";
    }
    
    file << "  ]\n";
    file << "}\n";
    
    file.close();
    
    HFX_LOG_INFO("[LOG] Message");
}

bool TestRunner::MatchesFilter(const TestCase& test) const {
    // Check name filter
    if (!test_filter_.empty()) {
        std::regex filter_regex(test_filter_);
        if (!std::regex_search(test.GetName(), filter_regex)) {
            return false;
        }
    }
    
    // Check category filter
    if (!category_filter_.empty()) {
        if (std::find(category_filter_.begin(), category_filter_.end(), test.GetCategory()) == category_filter_.end()) {
            return false;
        }
    }
    
    // Check tag filter
    if (!tag_filter_.empty()) {
        const auto& test_tags = test.GetTags();
        bool has_matching_tag = false;
        
        for (const auto& filter_tag : tag_filter_) {
            if (std::find(test_tags.begin(), test_tags.end(), filter_tag) != test_tags.end()) {
                has_matching_tag = true;
                break;
            }
        }
        
        if (!has_matching_tag) {
            return false;
        }
    }
    
    // Check priority filter
    if (test.GetPriority() < priority_filter_) {
        return false;
    }
    
    return true;
}

// TestRunnerFactory implementation
std::unique_ptr<TestRunner> TestRunnerFactory::CreateUnitTestRunner() {
    auto runner = std::make_unique<TestRunner>();
    runner->SetParallelExecution(true, std::thread::hardware_concurrency());
    runner->SetOutputFormat("console");
    runner->SetVerboseMode(true);
    runner->SetCategoryFilter({TestCategory::UNIT});
    return runner;
}

std::unique_ptr<TestRunner> TestRunnerFactory::CreateIntegrationTestRunner() {
    auto runner = std::make_unique<TestRunner>();
    runner->SetParallelExecution(false); // Integration tests often need to be sequential
    runner->SetOutputFormat("xml");
    runner->SetVerboseMode(true);
    runner->SetCategoryFilter({TestCategory::INTEGRATION});
    return runner;
}

std::unique_ptr<TestRunner> TestRunnerFactory::CreatePerformanceTestRunner() {
    auto runner = std::make_unique<TestRunner>();
    runner->SetParallelExecution(false); // Performance tests should run in isolation
    runner->SetOutputFormat("json");
    runner->SetVerboseMode(true);
    runner->SetCategoryFilter({TestCategory::PERFORMANCE});
    return runner;
}

std::unique_ptr<TestRunner> TestRunnerFactory::CreateContinuousIntegrationRunner() {
    auto runner = std::make_unique<TestRunner>();
    runner->SetParallelExecution(true, 4); // Limit threads for CI
    runner->SetOutputFormat("xml");
    runner->SetVerboseMode(false);
    runner->SetPriorityFilter(TestPriority::MEDIUM); // Skip low priority tests in CI
    return runner;
}

std::unique_ptr<TestRunner> TestRunnerFactory::CreateCustomRunner(bool parallel, 
                                                                 const std::string& output_format, 
                                                                 bool verbose) {
    auto runner = std::make_unique<TestRunner>();
    runner->SetParallelExecution(parallel);
    runner->SetOutputFormat(output_format);
    runner->SetVerboseMode(verbose);
    return runner;
}

// Utility functions implementation
namespace test_utils {
    std::vector<std::string> DiscoverTestFiles(const std::string& directory, const std::string& pattern) {
        std::vector<std::string> test_files;
        std::regex pattern_regex(pattern);
        
        if (std::filesystem::exists(directory)) {
            for (const auto& entry : std::filesystem::recursive_directory_iterator(directory)) {
                if (entry.is_regular_file()) {
                    std::string filename = entry.path().filename().string();
                    if (std::regex_search(filename, pattern_regex)) {
                        test_files.push_back(entry.path().string());
                    }
                }
            }
        }
        
        return test_files;
    }
    
    std::string CreateTempFile(const std::string& prefix) {
        std::string temp_path = std::filesystem::temp_directory_path();
        std::string filename = prefix + std::to_string(std::random_device{}()) + ".tmp";
        return temp_path + "/" + filename;
    }
    
    std::string CreateTempDirectory(const std::string& prefix) {
        std::string temp_path = std::filesystem::temp_directory_path();
        std::string dirname = prefix + std::to_string(std::random_device{}());
        std::string full_path = temp_path + "/" + dirname;
        
        std::filesystem::create_directory(full_path);
        return full_path;
    }
    
    void CleanupTempFile(const std::string& filepath) {
        std::filesystem::remove(filepath);
    }
    
    void CleanupTempDirectory(const std::string& dirpath) {
        std::filesystem::remove_all(dirpath);
    }
    
    std::vector<uint8_t> GenerateRandomData(size_t size) {
        std::vector<uint8_t> data(size);
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<uint8_t> dis(0, 255);
        
        for (auto& byte : data) {
            byte = dis(gen);
        }
        
        return data;
    }
    
    std::string GenerateRandomString(size_t length) {
        const std::string charset = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
        std::string result;
        result.reserve(length);
        
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, charset.size() - 1);
        
        for (size_t i = 0; i < length; ++i) {
            result += charset[dis(gen)];
        }
        
        return result;
    }
    
    double GenerateRandomDouble(double min_val, double max_val) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> dis(min_val, max_val);
        return dis(gen);
    }
    
    void SleepFor(std::chrono::milliseconds duration) {
        std::this_thread::sleep_for(duration);
    }
    
    std::chrono::system_clock::time_point GetCurrentTime() {
        return std::chrono::system_clock::now();
    }
    
    bool WaitFor(std::function<bool()> condition, std::chrono::milliseconds timeout) {
        auto start_time = std::chrono::steady_clock::now();
        
        while (std::chrono::steady_clock::now() - start_time < timeout) {
            if (condition()) {
                return true;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        
        return false;
    }
    
    std::vector<std::string> SplitString(const std::string& str, char delimiter) {
        std::vector<std::string> tokens;
        std::stringstream ss(str);
        std::string token;
        
        while (std::getline(ss, token, delimiter)) {
            tokens.push_back(token);
        }
        
        return tokens;
    }
    
    std::string TrimString(const std::string& str) {
        size_t start = str.find_first_not_of(" \t\n\r");
        if (start == std::string::npos) return "";
        
        size_t end = str.find_last_not_of(" \t\n\r");
        return str.substr(start, end - start + 1);
    }
    
    bool StringContains(const std::string& haystack, const std::string& needle) {
        return haystack.find(needle) != std::string::npos;
    }
    
    bool FileExists(const std::string& filepath) {
        return std::filesystem::exists(filepath);
    }
    
    std::string ReadFileContent(const std::string& filepath) {
        std::ifstream file(filepath);
        if (!file.is_open()) {
            throw std::runtime_error("Cannot open file: " + filepath);
        }
        
        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }
    
    void WriteFileContent(const std::string& filepath, const std::string& content) {
        std::ofstream file(filepath);
        if (!file.is_open()) {
            throw std::runtime_error("Cannot create file: " + filepath);
        }
        
        file << content;
    }
    
    bool IsPortOpen(const std::string& host, int port) {
        // Simplified port check - in real implementation would use socket programming
        return false;
    }
    
    int FindAvailablePort(int start_port, int end_port) {
        for (int port = start_port; port <= end_port; ++port) {
            if (!IsPortOpen("localhost", port)) {
                return port;
            }
        }
        return -1;
    }
    
    size_t GetCurrentMemoryUsage() {
#ifdef __APPLE__
        struct mach_task_basic_info info;
        mach_msg_type_number_t size = MACH_TASK_BASIC_INFO_COUNT;
        kern_return_t kerr = task_info(mach_task_self(), MACH_TASK_BASIC_INFO, 
                                      (task_info_t)&info, &size);
        if (kerr == KERN_SUCCESS) {
            return info.resident_size;
        }
#endif
        return 0;
    }
    
    size_t GetPeakMemoryUsage() {
        struct rusage usage;
        getrusage(RUSAGE_SELF, &usage);
        return usage.ru_maxrss;
    }
    
    std::string ExecuteCommand(const std::string& command) {
        // Simplified command execution - in real implementation would use popen
        return "";
    }
    
    bool IsProcessRunning(const std::string& process_name) {
        // Simplified process check - in real implementation would check process list
        return false;
    }
}

} // namespace hfx::testing
