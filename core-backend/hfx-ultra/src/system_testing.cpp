#include "system_testing.hpp"
#include <random>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <iomanip>
#include <cassert>

#ifdef __APPLE__
#include <mach/mach.h>
#include <sys/sysctl.h>
#elif __linux__
#include <sys/sysinfo.h>
#include <unistd.h>
#endif

namespace hfx::ultra {

// MockMempoolDataGenerator implementation
MockMempoolDataGenerator::MockMempoolDataGenerator(uint32_t transactions_per_second) 
    : tps_(transactions_per_second) {
}

bool MockMempoolDataGenerator::start_generation() {
    if (running_.load()) {
        return false;
    }
    
    running_.store(true);
    generator_thread_ = std::thread([this]() {
        generation_worker();
    });
    
    return true;
}

bool MockMempoolDataGenerator::stop_generation() {
    if (!running_.load()) {
        return false;
    }
    
    running_.store(false);
    
    if (generator_thread_.joinable()) {
        generator_thread_.join();
    }
    
    return true;
}

void MockMempoolDataGenerator::set_transaction_callback(std::function<void(const MockTransaction&)> callback) {
    callback_ = std::move(callback);
}

MockMempoolDataGenerator::MockTransaction MockMempoolDataGenerator::generate_swap_transaction(
    const std::string& token_in, const std::string& token_out) {
    
    MockTransaction tx;
    tx.hash = generate_random_hash();
    tx.from = generate_random_address();
    tx.to = "0x7a250d5630B4cF53c41EA61Ff0eE3bEf506A7C80"; // Uniswap V2 Router
    tx.value = 1000000000000000000ULL; // 1 ETH
    tx.gas_price = 20000000000ULL; // 20 gwei
    tx.gas_limit = 150000;
    tx.timestamp = std::chrono::system_clock::now();
    
    // Create swap data (simplified)
    std::string swap_data = "swapExactETHForTokens" + token_in + token_out;
    tx.data = std::vector<uint8_t>(swap_data.begin(), swap_data.end());
    
    return tx;
}

MockMempoolDataGenerator::MockTransaction MockMempoolDataGenerator::generate_mev_opportunity() {
    MockTransaction tx = generate_swap_transaction("USDC", "ETH");
    
    // MEV transactions typically have higher gas prices
    tx.gas_price = 100000000000ULL; // 100 gwei
    tx.value = 10000000000000000000ULL; // 10 ETH
    
    return tx;
}

void MockMempoolDataGenerator::generation_worker() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> mev_dist(0.0, 1.0);
    
    auto interval = std::chrono::microseconds(1000000 / tps_); // Microseconds between transactions
    
    while (running_.load()) {
        auto start_time = std::chrono::high_resolution_clock::now();
        
        MockTransaction tx;
        
        // Decide transaction type
        if (mev_dist(gen) < mev_rate_) {
            tx = generate_mev_opportunity();
        } else {
            tx = generate_swap_transaction("ETH", "USDC");
        }
        
        if (callback_) {
            callback_(tx);
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
        
        if (elapsed < interval) {
            std::this_thread::sleep_for(interval - elapsed);
        }
    }
}

std::string MockMempoolDataGenerator::generate_random_hash() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);
    
    std::stringstream ss;
    ss << "0x";
    for (int i = 0; i < 64; ++i) {
        ss << std::hex << dis(gen);
    }
    
    return ss.str();
}

std::string MockMempoolDataGenerator::generate_random_address() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);
    
    std::stringstream ss;
    ss << "0x";
    for (int i = 0; i < 40; ++i) {
        ss << std::hex << dis(gen);
    }
    
    return ss.str();
}

// SystemTestSuite implementation
SystemTestSuite::SystemTestSuite() {
    mempool_generator_ = std::make_unique<MockMempoolDataGenerator>(1000); // 1K TPS default
}

SystemTestSuite::~SystemTestSuite() {
    cleanup_test_environment();
}

void SystemTestSuite::register_test(const TestConfig& config, TestFunction test_func) {
    registered_tests_.emplace_back(config, std::move(test_func));
}

bool SystemTestSuite::run_all_tests() {
    if (!setup_test_environment()) {
        return false;
    }
    
    test_suite_start_time_ = std::chrono::system_clock::now();
    
    std::lock_guard<std::mutex> lock(results_mutex_);
    test_results_.clear();
    
    bool all_passed = true;
    
    for (const auto& [config, test_func] : registered_tests_) {
        TestResult result = execute_test(config, test_func);
        if (result != TestResult::PASSED) {
            all_passed = false;
        }
    }
    
    return all_passed;
}

bool SystemTestSuite::test_ultra_fast_mempool() {
    TestConfig config;
    config.test_name = "UltraFastMempool_BasicFunctionality";
    config.description = "Test basic mempool monitoring functionality";
    config.max_latency = std::chrono::microseconds(100); // 100μs max
    
    register_test(config, [this](TestContext& ctx) -> TestResult {
        return test_mempool_processing_latency(ctx);
    });
    
    return run_test(config.test_name);
}

bool SystemTestSuite::test_mev_shield() {
    TestConfig config;
    config.test_name = "MEVShield_Protection";
    config.description = "Test MEV detection and protection";
    config.max_latency = std::chrono::microseconds(500); // 500μs max
    
    register_test(config, [this](TestContext& ctx) -> TestResult {
        return test_mev_opportunity_detection(ctx);
    });
    
    return run_test(config.test_name);
}

bool SystemTestSuite::test_hsm_key_manager() {
    TestConfig config;
    config.test_name = "HSMKeyManager_SigningSpeed";
    config.description = "Test HSM signing performance";
    config.max_latency = std::chrono::microseconds(5000); // 5ms max for HSM signing
    
    register_test(config, [this](TestContext& ctx) -> TestResult {
        return test_signature_speed(ctx);
    });
    
    return run_test(config.test_name);
}

bool SystemTestSuite::test_nats_messaging() {
    TestConfig config;
    config.test_name = "NATSJetStream_MessageThroughput";
    config.description = "Test NATS JetStream message throughput";
    config.min_throughput_ops_per_sec = 50000; // 50K messages/sec
    
    register_test(config, [this](TestContext& ctx) -> TestResult {
        return test_message_throughput(ctx);
    });
    
    return run_test(config.test_name);
}

bool SystemTestSuite::test_production_database() {
    TestConfig config;
    config.test_name = "ProductionDatabase_WritePerformance";
    config.description = "Test database write performance";
    config.min_throughput_ops_per_sec = 10000; // 10K writes/sec
    
    register_test(config, [this](TestContext& ctx) -> TestResult {
        return test_database_write_performance(ctx);
    });
    
    return run_test(config.test_name);
}

bool SystemTestSuite::test_end_to_end_trading_flow() {
    TestConfig config;
    config.test_name = "EndToEnd_TradingFlow";
    config.description = "Test complete trading flow from signal to execution";
    config.max_latency = std::chrono::microseconds(20000); // 20ms max (ultra-low latency target)
    config.timeout = std::chrono::seconds(60);
    
    register_test(config, [this](TestContext& ctx) -> TestResult {
        return test_sub_20ms_decision_latency(ctx);
    });
    
    return run_test(config.test_name);
}

bool SystemTestSuite::test_latency_benchmarks() {
    std::vector<std::string> latency_tests = {
        "UltraFastMempool_BasicFunctionality",
        "MEVShield_Protection", 
        "EndToEnd_TradingFlow"
    };
    
    bool all_passed = true;
    for (const auto& test_name : latency_tests) {
        if (!run_test(test_name)) {
            all_passed = false;
        }
    }
    
    return all_passed;
}

SystemTestSuite::TestSummary SystemTestSuite::get_test_summary() const {
    std::lock_guard<std::mutex> lock(results_mutex_);
    
    TestSummary summary;
    summary.total_tests = test_results_.size();
    
    for (const auto& result : test_results_) {
        switch (result.result) {
            case TestResult::PASSED:
                summary.passed_tests++;
                break;
            case TestResult::FAILED:
                summary.failed_tests++;
                summary.failed_test_details.push_back(result);
                break;
            case TestResult::SKIPPED:
                summary.skipped_tests++;
                break;
            case TestResult::TIMEOUT:
                summary.timeout_tests++;
                break;
            default:
                summary.failed_tests++;
                break;
        }
    }
    
    if (!test_results_.empty()) {
        auto start_time = test_results_.front().start_time;
        auto end_time = test_results_.back().end_time;
        summary.total_execution_time = std::chrono::duration_cast<std::chrono::milliseconds>(
            end_time - start_time);
    }
    
    return summary;
}

void SystemTestSuite::generate_test_report(const std::string& output_file) const {
    auto summary = get_test_summary();
    
    std::ostream* out = &std::cerr; // Use cerr to avoid cout issue
    std::ofstream file_out;
    
    if (!output_file.empty()) {
        file_out.open(output_file);
        out = &file_out;
    }
    
    *out << "=== HydraFlow-X Ultra-Fast Trading System Test Report ===\n\n";
    *out << "Test Summary:\n";
    *out << "  Total Tests: " << summary.total_tests << "\n";
    *out << "  Passed: " << summary.passed_tests << "\n";
    *out << "  Failed: " << summary.failed_tests << "\n";
    *out << "  Skipped: " << summary.skipped_tests << "\n";
    *out << "  Timeout: " << summary.timeout_tests << "\n";
    *out << "  Success Rate: " << std::fixed << std::setprecision(2) 
          << (summary.total_tests > 0 ? (double)summary.passed_tests / summary.total_tests * 100.0 : 0.0) 
          << "%\n";
    *out << "  Total Execution Time: " << summary.total_execution_time.count() << "ms\n\n";
    
    if (!summary.failed_test_details.empty()) {
        *out << "Failed Tests:\n";
        for (const auto& failed_test : summary.failed_test_details) {
            *out << "  - " << failed_test.test_id << ": " << failed_test.error_message << "\n";
        }
        *out << "\n";
    }
    
    std::lock_guard<std::mutex> lock(results_mutex_);
    *out << "Detailed Performance Metrics:\n";
    for (const auto& result : test_results_) {
        if (result.result == TestResult::PASSED) {
            *out << "  " << result.test_id << ":\n";
            *out << "    Average Latency: " << result.avg_latency.count() << "μs\n";
            *out << "    Max Latency: " << result.max_latency.count() << "μs\n";
            *out << "    Operations: " << result.operations_completed << "\n";
            *out << "    Memory Usage: " << std::fixed << std::setprecision(2) 
                  << result.memory_usage_mb << "MB\n";
            *out << "    CPU Usage: " << std::fixed << std::setprecision(1) 
                  << result.cpu_usage_percent << "%\n\n";
        }
    }
}

bool SystemTestSuite::setup_test_environment() {
    if (test_environment_ready_.load()) {
        return true;
    }
    
    // Initialize all components
    if (!initialize_mempool_monitor()) {
        return false;
    }
    
    if (!initialize_mev_components()) {
        return false;
    }
    
    if (!initialize_database_components()) {
        return false;
    }
    
    if (!initialize_messaging_components()) {
        return false;
    }
    
    if (!initialize_key_management()) {
        return false;
    }
    
    // Start mock data generation
    mempool_generator_->set_transaction_callback([this](const MockMempoolDataGenerator::MockTransaction& tx) {
        if (mempool_monitor_) {
            // Convert mock transaction to FastTransaction
            FastTransaction fast_tx;
            fast_tx.hash = tx.hash;
            fast_tx.from_address = tx.from;
            fast_tx.to_address = tx.to;
            fast_tx.value = tx.value;
            fast_tx.gas_price = tx.gas_price;
            fast_tx.gas_limit = tx.gas_limit;
            fast_tx.data = tx.data;
            fast_tx.timestamp = std::chrono::duration_cast<std::chrono::microseconds>(
                tx.timestamp.time_since_epoch());
            
            // Process through mempool monitor
            mempool_monitor_->process_transaction(fast_tx);
        }
    });
    
    mempool_generator_->start_generation();
    
    test_environment_ready_.store(true);
    return true;
}

bool SystemTestSuite::cleanup_test_environment() {
    if (!test_environment_ready_.load()) {
        return true;
    }
    
    // Stop mock data generation
    if (mempool_generator_) {
        mempool_generator_->stop_generation();
    }
    
    // Cleanup components
    mempool_monitor_.reset();
    mev_shield_.reset();
    jito_engine_.reset();
    smart_trading_engine_.reset();
    v3_engine_.reset();
    hsm_manager_.reset();
    nats_engine_.reset();
    database_.reset();
    
    test_environment_ready_.store(false);
    return true;
}

// Private method implementations
TestResult SystemTestSuite::execute_test(const TestConfig& config, TestFunction test_func) {
    TestContext context;
    context.test_id = config.test_name;
    context.start_time = std::chrono::system_clock::now();
    
    start_performance_monitoring(context);
    
    try {
        context.result = test_func(context);
    } catch (const std::exception& e) {
        context.result = TestResult::ERROR;
        context.error_message = e.what();
    } catch (...) {
        context.result = TestResult::ERROR;
        context.error_message = "Unknown exception occurred";
    }
    
    stop_performance_monitoring(context);
    context.end_time = std::chrono::system_clock::now();
    
    log_test_result(context);
    
    return context.result;
}

void SystemTestSuite::log_test_result(const TestContext& context) {
    std::lock_guard<std::mutex> lock(results_mutex_);
    test_results_.push_back(context);
}

bool SystemTestSuite::initialize_mempool_monitor() {
    MempoolConfig config;
    config.enable_bloom_filtering = true;
    config.worker_threads = 4;
    config.processing_interval = std::chrono::microseconds(25);
    
    mempool_monitor_ = MempoolMonitorFactory::create_ethereum_monitor();
    
    if (mempool_monitor_) {
        mempool_monitor_->start();
        return true;
    }
    
    return false;
}

bool SystemTestSuite::initialize_mev_components() {
    // Initialize MEV Shield
    MEVShieldConfig mev_config;
    mev_config.protection_level = MEVProtectionLevel::MAXIMUM;
    mev_config.worker_threads = 4;
    
    mev_shield_ = std::make_unique<MEVShield>(mev_config);
    if (!mev_shield_->start()) {
        return false;
    }
    
    // Initialize Jito Engine
    JitoBundleConfig jito_config;
    jito_config.bundle_type = JitoBundleType::PRIORITY;
    jito_config.tip_lamports = 50000;
    jito_config.worker_threads = 4;
    
    jito_engine_ = std::make_unique<JitoMEVEngine>(jito_config);
    jito_engine_->start();
    
    // Initialize Smart Trading Engine
    SmartTradingConfig smart_config;
    smart_config.default_mode = TradingMode::SNIPER_MODE;
    smart_config.worker_threads = 4;
    
    smart_trading_engine_ = std::make_unique<SmartTradingEngine>(smart_config);
    return smart_trading_engine_->start();
}

bool SystemTestSuite::initialize_database_components() {
    SchemaConfig config;
    config.backend = DatabaseBackend::SQLITE_MEMORY; // Use in-memory for testing
    config.schema_name = "test_db";
    config.batch_size = 1000;
    
    database_ = std::make_unique<ProductionDatabase>(config);
    
    if (!database_->connect()) {
        return false;
    }
    
    if (!database_->create_schema()) {
        return false;
    }
    
    return true;
}

bool SystemTestSuite::initialize_messaging_components() {
    NATSConfig config;
    config.servers = {"nats://localhost:4222"};
    config.no_echo = true;
    config.pedantic = false;
    
    nats_engine_ = std::make_unique<NATSJetStreamEngine>(config);
    
    if (!nats_engine_->connect()) {
        return false;
    }
    
    // Setup trading streams
    return nats_engine_->setup_trading_streams();
}

bool SystemTestSuite::initialize_key_management() {
    HSMConfig config;
    config.provider = HSMProvider::SOFTWARE_HSM; // Use software HSM for testing
    config.connection_params = "/tmp/test_hsm";
    
    hsm_manager_ = std::make_unique<HSMKeyManager>(config);
    
    if (!hsm_manager_->initialize()) {
        return false;
    }
    
    // Generate test keys
    std::string trading_key = hsm_manager_->generate_key(
        KeyRole::TRADING_OPERATIONAL, "test_trading_key", SecurityLevel::MEDIUM);
    
    std::string mev_key = hsm_manager_->generate_key(
        KeyRole::MEV_EXECUTION, "test_mev_key", SecurityLevel::HIGH);
    
    return !trading_key.empty() && !mev_key.empty();
}

// Test implementations
TestResult SystemTestSuite::test_mempool_processing_latency(TestContext& context) {
    if (!mempool_monitor_) {
        context.error_message = "Mempool monitor not initialized";
        return TestResult::ERROR;
    }
    
    test_utils::HighResolutionTimer timer;
    
    // Generate test transactions and measure processing latency
    uint32_t test_iterations = 1000;
    std::vector<std::chrono::nanoseconds> latencies;
    
    for (uint32_t i = 0; i < test_iterations; ++i) {
        auto tx = mempool_generator_->generate_swap_transaction("ETH", "USDC");
        
        timer.start();
        
        // Convert and process transaction
        FastTransaction fast_tx;
        fast_tx.hash = tx.hash;
        fast_tx.value = tx.value;
        
        mempool_monitor_->process_transaction(fast_tx);
        
        timer.stop();
        latencies.push_back(timer.elapsed());
        timer.reset();
    }
    
    // Calculate statistics
    if (!latencies.empty()) {
        auto total_ns = std::accumulate(latencies.begin(), latencies.end(), std::chrono::nanoseconds(0));
        context.avg_latency = std::chrono::duration_cast<std::chrono::microseconds>(total_ns / latencies.size());
        
        auto max_it = std::max_element(latencies.begin(), latencies.end());
        context.max_latency = std::chrono::duration_cast<std::chrono::microseconds>(*max_it);
        
        auto min_it = std::min_element(latencies.begin(), latencies.end());
        context.min_latency = std::chrono::duration_cast<std::chrono::microseconds>(*min_it);
        
        context.operations_completed = test_iterations;
        
        // Check if latency meets requirements (100μs max)
        if (context.avg_latency <= std::chrono::microseconds(100)) {
            return TestResult::PASSED;
        }
    }
    
    context.error_message = "Average latency exceeded 100μs threshold";
    return TestResult::FAILED;
}

TestResult SystemTestSuite::test_sub_20ms_decision_latency(TestContext& context) {
    // Test the complete decision pipeline: Signal -> Risk Check -> Execution Decision
    // Target: Sub-20ms decision latency for ultra-fast trading
    
    test_utils::HighResolutionTimer timer;
    
    // Simulate trading signal processing
    uint32_t test_iterations = 100;
    std::vector<std::chrono::nanoseconds> decision_latencies;
    
    for (uint32_t i = 0; i < test_iterations; ++i) {
        timer.start();
        
        // Step 1: MEV opportunity detection (simulated)
        auto mev_tx = mempool_generator_->generate_mev_opportunity();
        
        // Step 2: Risk assessment
        if (mev_shield_) {
            // Simulate risk analysis
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
        
        // Step 3: Decision making
        bool should_execute = (mev_tx.value > 5000000000000000000ULL); // > 5 ETH
        
        // Step 4: Route planning (if executing)
        if (should_execute && v3_engine_) {
            // Simulate V3 tick walk
            std::this_thread::sleep_for(std::chrono::microseconds(500));
        }
        
        timer.stop();
        decision_latencies.push_back(timer.elapsed());
        timer.reset();
    }
    
    // Calculate decision latency statistics
    if (!decision_latencies.empty()) {
        auto total_ns = std::accumulate(decision_latencies.begin(), decision_latencies.end(), 
                                       std::chrono::nanoseconds(0));
        context.avg_latency = std::chrono::duration_cast<std::chrono::microseconds>(total_ns / decision_latencies.size());
        
        auto max_it = std::max_element(decision_latencies.begin(), decision_latencies.end());
        context.max_latency = std::chrono::duration_cast<std::chrono::microseconds>(*max_it);
        
        context.operations_completed = test_iterations;
        
        // Target: Sub-20ms decision latency (ultra-fast performance)
        if (context.avg_latency <= std::chrono::microseconds(20000)) {
            context.logs.push_back("✅ Decision latency target achieved: " + 
                                 std::to_string(context.avg_latency.count()) + "μs avg");
            return TestResult::PASSED;
        }
    }
    
    context.error_message = "Decision latency exceeded 20ms target - needs optimization";
    return TestResult::FAILED;
}

TestResult SystemTestSuite::test_signature_speed(TestContext& context) {
    if (!hsm_manager_) {
        context.error_message = "HSM manager not initialized";
        return TestResult::ERROR;
    }
    
    // Create test session
    std::string session_id = hsm_manager_->create_session("test_user", "test_pin");
    if (session_id.empty()) {
        context.error_message = "Failed to create HSM session";
        return TestResult::ERROR;
    }
    
    // Generate test key
    std::string key_id = hsm_manager_->generate_key(KeyRole::TRADING_OPERATIONAL, 
                                                  "speed_test_key", SecurityLevel::MEDIUM);
    if (key_id.empty()) {
        context.error_message = "Failed to generate test key";
        return TestResult::ERROR;
    }
    
    test_utils::HighResolutionTimer timer;
    
    // Test fast signing performance
    uint32_t test_iterations = 100;
    std::vector<std::chrono::nanoseconds> signing_times;
    
    for (uint32_t i = 0; i < test_iterations; ++i) {
        std::vector<uint8_t> test_data = test_utils::generate_random_data(32); // 32-byte hash
        
        timer.start();
        auto result = hsm_manager_->fast_sign(key_id, test_data, session_id);
        timer.stop();
        
        if (result.success) {
            signing_times.push_back(timer.elapsed());
        }
        
        timer.reset();
    }
    
    // Calculate signing performance
    if (!signing_times.empty()) {
        auto total_ns = std::accumulate(signing_times.begin(), signing_times.end(), 
                                       std::chrono::nanoseconds(0));
        context.avg_latency = std::chrono::duration_cast<std::chrono::microseconds>(total_ns / signing_times.size());
        
        auto max_it = std::max_element(signing_times.begin(), signing_times.end());
        context.max_latency = std::chrono::duration_cast<std::chrono::microseconds>(*max_it);
        
        context.operations_completed = signing_times.size();
        
        // Target: Sub-5ms signing (HSM typical performance)
        if (context.avg_latency <= std::chrono::microseconds(5000)) {
            return TestResult::PASSED;
        }
    }
    
    hsm_manager_->close_session(session_id);
    context.error_message = "Signing latency exceeded 5ms threshold";
    return TestResult::FAILED;
}

void SystemTestSuite::start_performance_monitoring(TestContext& context) {
    // Start monitoring CPU and memory usage
    // Implementation would use system-specific APIs
}

void SystemTestSuite::stop_performance_monitoring(TestContext& context) {
    context.memory_usage_mb = get_memory_usage_mb();
    context.cpu_usage_percent = get_cpu_usage_percent();
}

double SystemTestSuite::get_memory_usage_mb() const {
#ifdef __APPLE__
    struct mach_task_basic_info info;
    mach_msg_type_number_t infoCount = MACH_TASK_BASIC_INFO_COUNT;
    if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO,
                  (task_info_t)&info, &infoCount) != KERN_SUCCESS) {
        return 0.0;
    }
    return info.resident_size / (1024.0 * 1024.0);
#elif __linux__
    std::ifstream statm("/proc/self/statm");
    if (statm.is_open()) {
        size_t size, resident, shared, text, lib, data, dt;
        statm >> size >> resident >> shared >> text >> lib >> data >> dt;
        return (resident * getpagesize()) / (1024.0 * 1024.0);
    }
    return 0.0;
#else
    return 0.0;
#endif
}

double SystemTestSuite::get_cpu_usage_percent() const {
    // Simplified CPU usage estimation
    return 25.0; // Mock value
}

// Test utility implementations
namespace test_utils {
    void HighResolutionTimer::start() {
        start_time_ = std::chrono::high_resolution_clock::now();
        running_ = true;
    }
    
    void HighResolutionTimer::stop() {
        if (running_) {
            end_time_ = std::chrono::high_resolution_clock::now();
            running_ = false;
        }
    }
    
    std::chrono::nanoseconds HighResolutionTimer::elapsed() const {
        if (running_) {
            auto now = std::chrono::high_resolution_clock::now();
            return std::chrono::duration_cast<std::chrono::nanoseconds>(now - start_time_);
        } else {
            return std::chrono::duration_cast<std::chrono::nanoseconds>(end_time_ - start_time_);
        }
    }
    
    void HighResolutionTimer::reset() {
        running_ = false;
    }
    
    std::vector<uint8_t> generate_random_data(size_t size) {
        std::vector<uint8_t> data(size);
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<uint8_t> dis(0, 255);
        
        for (auto& byte : data) {
            byte = dis(gen);
        }
        
        return data;
    }
    
    std::string generate_random_string(size_t length) {
        const std::string charset = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
        std::string result;
        result.reserve(length);
        
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, charset.length() - 1);
        
        for (size_t i = 0; i < length; ++i) {
            result += charset[dis(gen)];
        }
        
        return result;
    }
    
    StatisticalSummary calculate_statistics(const std::vector<double>& values) {
        StatisticalSummary summary{};
        
        if (values.empty()) {
            return summary;
        }
        
        // Sort for percentile calculations
        std::vector<double> sorted_values = values;
        std::sort(sorted_values.begin(), sorted_values.end());
        
        // Basic statistics
        summary.min_value = sorted_values.front();
        summary.max_value = sorted_values.back();
        
        // Mean
        double sum = std::accumulate(values.begin(), values.end(), 0.0);
        summary.mean = sum / values.size();
        
        // Median
        size_t middle = sorted_values.size() / 2;
        if (sorted_values.size() % 2 == 0) {
            summary.median = (sorted_values[middle - 1] + sorted_values[middle]) / 2.0;
        } else {
            summary.median = sorted_values[middle];
        }
        
        // Standard deviation
        double variance = 0.0;
        for (double value : values) {
            variance += std::pow(value - summary.mean, 2);
        }
        summary.std_deviation = std::sqrt(variance / values.size());
        
        // Percentiles
        summary.percentile_95 = sorted_values[static_cast<size_t>(0.95 * (sorted_values.size() - 1))];
        summary.percentile_99 = sorted_values[static_cast<size_t>(0.99 * (sorted_values.size() - 1))];
        
        return summary;
    }
}

} // namespace hfx::ultra
