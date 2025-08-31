#pragma once

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <atomic>
#include <mutex>
#include <chrono>
#include <functional>
#include <thread>

// Include all our ultra-fast components
#include "ultra_fast_mempool.hpp"
#include "mev_shield.hpp"
#include "jito_mev_engine.hpp"
#include "smart_trading_engine.hpp"
#include "v3_tick_engine.hpp"
#include "hsm_key_manager.hpp"
#include "nats_jetstream_engine.hpp"
#include "production_database.hpp"

namespace hfx::ultra {

// Test result types
enum class TestResult {
    PASSED,
    FAILED,
    SKIPPED,
    TIMEOUT,
    ERROR
};

// Performance test categories
enum class PerformanceCategory {
    LATENCY,           // Sub-millisecond latency tests
    THROUGHPUT,        // High-volume processing tests
    MEMORY,            // Memory usage and leak tests
    CPU,               // CPU efficiency tests
    NETWORK,           // Network performance tests
    STORAGE            // Database performance tests
};

// Test configuration
struct TestConfig {
    std::string test_name;
    std::string description;
    std::chrono::seconds timeout{30};
    bool performance_test = false;
    PerformanceCategory category = PerformanceCategory::LATENCY;
    
    // Performance thresholds
    std::chrono::microseconds max_latency{1000};    // 1ms max latency
    uint64_t min_throughput_ops_per_sec = 10000;    // 10K ops/sec minimum
    uint64_t max_memory_usage_mb = 100;             // 100MB max memory
    double max_cpu_usage_percent = 50.0;            // 50% max CPU
    
    // Test data
    uint32_t test_iterations = 1000;
    uint32_t concurrent_threads = 4;
    bool enable_stress_testing = false;
    std::string test_data_file;
};

// Test execution context
struct TestContext {
    std::string test_id;
    std::chrono::system_clock::time_point start_time;
    std::chrono::system_clock::time_point end_time;
    TestResult result = TestResult::FAILED;
    std::string error_message;
    std::vector<std::string> logs;
    
    // Performance metrics
    std::chrono::microseconds avg_latency{0};
    std::chrono::microseconds max_latency{0};
    std::chrono::microseconds min_latency{std::chrono::microseconds::max()};
    uint64_t operations_completed = 0;
    uint64_t errors_encountered = 0;
    double memory_usage_mb = 0.0;
    double cpu_usage_percent = 0.0;
    
    // Custom metrics
    std::unordered_map<std::string, double> custom_metrics;
};

// Mock mempool data generator for testing
class MockMempoolDataGenerator {
public:
    struct MockTransaction {
        std::string hash;
        std::string from;
        std::string to;
        uint64_t value;
        uint64_t gas_price;
        uint64_t gas_limit;
        std::vector<uint8_t> data;
        std::chrono::system_clock::time_point timestamp;
    };
    
    explicit MockMempoolDataGenerator(uint32_t transactions_per_second = 1000);
    
    bool start_generation();
    bool stop_generation();
    
    void set_transaction_callback(std::function<void(const MockTransaction&)> callback);
    void set_realistic_patterns(bool enable);
    void set_mev_opportunities_rate(double rate); // 0.0 to 1.0
    
    // Generate specific types of transactions
    MockTransaction generate_swap_transaction(const std::string& token_in, const std::string& token_out);
    MockTransaction generate_liquidity_add_transaction(const std::string& token_a, const std::string& token_b);
    MockTransaction generate_mev_opportunity();
    
private:
    uint32_t tps_;
    std::atomic<bool> running_{false};
    std::thread generator_thread_;
    std::function<void(const MockTransaction&)> callback_;
    bool realistic_patterns_ = true;
    double mev_rate_ = 0.01; // 1% of transactions are MEV opportunities
    
    void generation_worker();
    std::string generate_random_hash();
    std::string generate_random_address();
};

// Comprehensive system test suite
class SystemTestSuite {
public:
    using TestFunction = std::function<TestResult(TestContext&)>;
    
    SystemTestSuite();
    ~SystemTestSuite();
    
    // Test registration
    void register_test(const TestConfig& config, TestFunction test_func);
    void register_performance_test(const TestConfig& config, TestFunction test_func);
    
    // Test execution
    bool run_all_tests();
    bool run_test(const std::string& test_name);
    bool run_tests_by_category(PerformanceCategory category);
    
    // Component testing
    bool test_ultra_fast_mempool();
    bool test_mev_shield();
    bool test_jito_engine();
    bool test_smart_trading_integration();
    bool test_v3_tick_engine();
    bool test_hsm_key_manager();
    bool test_nats_messaging();
    bool test_production_database();
    
    // Integration testing
    bool test_end_to_end_trading_flow();
    bool test_mev_detection_and_protection();
    bool test_high_frequency_data_pipeline();
    bool test_risk_management_integration();
    
    // Performance testing
    bool test_latency_benchmarks();
    bool test_throughput_benchmarks();
    bool test_memory_usage();
    bool test_cpu_efficiency();
    bool test_concurrent_performance();
    
    // Stress testing
    bool test_system_under_load();
    bool test_error_recovery();
    bool test_failover_scenarios();
    
    // Results and reporting
    struct TestSummary {
        uint32_t total_tests = 0;
        uint32_t passed_tests = 0;
        uint32_t failed_tests = 0;
        uint32_t skipped_tests = 0;
        uint32_t timeout_tests = 0;
        std::chrono::milliseconds total_execution_time{0};
        std::vector<TestContext> failed_test_details;
    };
    
    TestSummary get_test_summary() const;
    void generate_test_report(const std::string& output_file = "") const;
    void print_performance_metrics() const;
    
    // Test environment setup
    bool setup_test_environment();
    bool cleanup_test_environment();
    
    // Mock data management
    void set_mock_mempool_data_rate(uint32_t tps);
    void enable_realistic_test_data(bool enable);
    
private:
    // Test registry
    std::vector<std::pair<TestConfig, TestFunction>> registered_tests_;
    std::vector<TestContext> test_results_;
    mutable std::mutex results_mutex_;
    
    // Test environment components
    std::unique_ptr<MockMempoolDataGenerator> mempool_generator_;
    std::unique_ptr<UltraFastMempoolMonitor> mempool_monitor_;
    std::unique_ptr<MEVShield> mev_shield_;
    std::unique_ptr<JitoMEVEngine> jito_engine_;
    std::unique_ptr<SmartTradingEngine> smart_trading_engine_;
    std::unique_ptr<V3TickEngine> v3_engine_;
    std::unique_ptr<HSMKeyManager> hsm_manager_;
    std::unique_ptr<NATSJetStreamEngine> nats_engine_;
    std::unique_ptr<ProductionDatabase> database_;
    
    // Test data and state
    std::atomic<bool> test_environment_ready_{false};
    std::chrono::system_clock::time_point test_suite_start_time_;
    
    // Helper methods
    TestResult execute_test(const TestConfig& config, TestFunction test_func);
    void log_test_result(const TestContext& context);
    std::string generate_test_id();
    
    // Performance monitoring
    void start_performance_monitoring(TestContext& context);
    void stop_performance_monitoring(TestContext& context);
    double get_memory_usage_mb() const;
    double get_cpu_usage_percent() const;
    
    // Component initialization for testing
    bool initialize_mempool_monitor();
    bool initialize_mev_components();
    bool initialize_database_components();
    bool initialize_messaging_components();
    bool initialize_key_management();
    
    // Test implementations
    TestResult test_mempool_processing_latency(TestContext& context);
    TestResult test_mev_opportunity_detection(TestContext& context);
    TestResult test_database_write_performance(TestContext& context);
    TestResult test_message_throughput(TestContext& context);
    TestResult test_signature_speed(TestContext& context);
    TestResult test_concurrent_operations(TestContext& context);
    TestResult test_memory_efficiency(TestContext& context);
    TestResult test_error_handling(TestContext& context);
    TestResult test_system_recovery(TestContext& context);
    
    // Advanced trading engine performance tests
    TestResult test_sub_20ms_decision_latency(TestContext& context);
    TestResult test_mev_bundle_creation_speed(TestContext& context);
    TestResult test_v3_tick_walk_performance(TestContext& context);
    TestResult test_bloom_filter_efficiency(TestContext& context);
};

// Automated test runner for CI/CD
class AutomatedTestRunner {
public:
    struct TestRunConfig {
        bool run_unit_tests = true;
        bool run_integration_tests = true;
        bool run_performance_tests = true;
        bool run_stress_tests = false;
        
        // CI/CD specific settings
        std::string build_id;
        std::string git_commit_hash;
        std::string test_environment;
        bool fail_on_performance_regression = true;
        double max_performance_degradation_percent = 10.0;
    };
    
    explicit AutomatedTestRunner(const TestRunConfig& config);
    
    bool run_ci_test_suite();
    bool run_nightly_test_suite();
    bool run_regression_test_suite();
    
    // Results export for CI/CD
    bool export_junit_xml(const std::string& output_file) const;
    bool export_performance_metrics(const std::string& output_file) const;
    bool check_performance_regression() const;
    
private:
    TestRunConfig config_;
    std::unique_ptr<SystemTestSuite> test_suite_;
    
    bool setup_ci_environment();
    bool validate_test_environment();
    void generate_ci_report() const;
};

// Performance benchmark utilities
namespace test_utils {
    // Timing utilities
    class HighResolutionTimer {
    public:
        void start();
        void stop();
        std::chrono::nanoseconds elapsed() const;
        void reset();
        
    private:
        std::chrono::high_resolution_clock::time_point start_time_;
        std::chrono::high_resolution_clock::time_point end_time_;
        bool running_ = false;
    };
    
    // Memory tracking
    class MemoryTracker {
    public:
        void start_tracking();
        void stop_tracking();
        size_t get_peak_usage_bytes() const;
        size_t get_current_usage_bytes() const;
        
    private:
        size_t initial_usage_ = 0;
        size_t peak_usage_ = 0;
        std::atomic<bool> tracking_{false};
        std::thread monitoring_thread_;
        
        void monitor_memory();
    };
    
    // CPU monitoring
    class CPUMonitor {
    public:
        void start_monitoring();
        void stop_monitoring();
        double get_average_usage_percent() const;
        double get_peak_usage_percent() const;
        
    private:
        std::atomic<bool> monitoring_{false};
        std::thread monitoring_thread_;
        std::vector<double> usage_samples_;
        mutable std::mutex samples_mutex_;
        
        void monitor_cpu();
        double get_current_cpu_usage() const;
    };
    
    // Test data generators
    std::vector<uint8_t> generate_random_data(size_t size);
    std::string generate_random_string(size_t length);
    std::vector<std::string> generate_test_symbols(size_t count);
    
    // Validation utilities
    bool validate_latency_requirements(std::chrono::nanoseconds measured,
                                     std::chrono::nanoseconds required);
    bool validate_throughput_requirements(uint64_t measured_ops_per_sec,
                                        uint64_t required_ops_per_sec);
    bool validate_memory_requirements(size_t measured_bytes, size_t max_bytes);
    
    // Statistical analysis
    struct StatisticalSummary {
        double mean;
        double median;
        double std_deviation;
        double min_value;
        double max_value;
        double percentile_95;
        double percentile_99;
    };
    
    StatisticalSummary calculate_statistics(const std::vector<double>& values);
    bool is_performance_acceptable(const StatisticalSummary& current,
                                 const StatisticalSummary& baseline,
                                 double max_degradation_percent = 10.0);
}

} // namespace hfx::ultra
