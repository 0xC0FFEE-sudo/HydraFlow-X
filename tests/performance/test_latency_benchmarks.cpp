/**
 * @file test_latency_benchmarks.cpp
 * @brief Performance benchmarks for HydraFlow-X ultra-low latency components
 */

#include "core-backend/hfx-ultra/include/testing_framework.hpp"
#include "core-backend/hfx-ultra/include/smart_trading_engine.hpp"
#include "core-backend/hfx-ultra/include/v3_tick_engine.hpp"
#include "core-backend/hfx-ultra/include/mev_shield.hpp"
#include "core-backend/hfx-ultra/include/jito_mev_engine.hpp"
#include <memory>
#include <random>

using namespace hfx::testing;
using namespace hfx::ultra;

// Performance benchmark suite for ultra-low latency components
class LatencyBenchmarkSuite : public TestSuite {
public:
    LatencyBenchmarkSuite() : TestSuite("LatencyBenchmarks") {
        AddTest(std::make_shared<TradingEngineLatencyTest>());
        AddTest(std::make_shared<V3TickEngineLatencyTest>());
        AddTest(std::make_shared<MEVShieldLatencyTest>());
        AddTest(std::make_shared<JitoMEVLatencyTest>());
        AddTest(std::make_shared<MemoryAllocationBenchmark>());
        AddTest(std::make_shared<ThroughputBenchmark>());
    }
    
    void SetUpSuite() override {
        std::cout << "⚡ Setting up Latency Benchmark Suite" << std::endl;
        std::cout << "Target: Sub-microsecond latency for critical paths" << std::endl;
        
        // Initialize random number generator for reproducible tests
        rng_.seed(12345);
    }
    
    void TearDownSuite() override {
        std::cout << "⚡ Latency benchmarks completed" << std::endl;
    }

protected:
    static std::mt19937 rng_;

private:
    // Trading engine latency benchmarks
    class TradingEngineLatencyTest : public TestCase {
    public:
        TradingEngineLatencyTest() : TestCase("TradingEngineLatency", TestCategory::PERFORMANCE, TestPriority::CRITICAL) {
            AddTag("latency");
            AddTag("trading");
            AddTag("critical-path");
            SetTimeout(std::chrono::minutes(5));
        }
        
        void SetUp() override {
            trading_engine_ = std::make_unique<SmartTradingEngine>();
            HFX_ASSERT_TRUE(trading_engine_->initialize());
            HFX_ASSERT_TRUE(trading_engine_->start());
            
            // Add test wallet
            HFX_ASSERT_TRUE(trading_engine_->add_wallet("test_private_key_for_benchmarks"));
            
            benchmark_ = std::make_unique<PerformanceBenchmark>("trading_engine");
        }
        
        void TearDown() override {
            if (trading_engine_) {
                trading_engine_->stop();
                trading_engine_.reset();
            }
            benchmark_.reset();
        }
        
        void Run() override {
            std::cout << "🔸 Benchmarking trading engine operations..." << std::endl;
            
            // Benchmark 1: Strategy execution latency
            auto strategy_latency = benchmark_->MeasureLatency([this]() {
                // Simulate strategy decision making
                auto strategies = trading_engine_->get_active_strategies();
                
                // Mock strategy execution
                for (const auto& strategy : strategies) {
                    if (strategy.status == StrategyStatus::ACTIVE) {
                        // Simulate strategy calculation
                        volatile double result = 0.0;
                        for (int i = 0; i < 100; ++i) {
                            result += std::sin(i * 0.1) * std::cos(i * 0.2);
                        }
                    }
                }
            }, 10000);
            
            RecordPerformanceMetric(PerformanceMetric::LATENCY_NS, "strategy_execution", strategy_latency.mean_value);
            
            // Benchmark 2: Portfolio update latency
            auto portfolio_latency = benchmark_->MeasureLatency([this]() {
                auto portfolio = trading_engine_->get_portfolio_summary();
                
                // Simulate portfolio calculations
                volatile double total_value = 0.0;
                for (const auto& [symbol, position] : portfolio.positions) {
                    total_value += position.quantity * position.avg_price;
                }
            }, 10000);
            
            RecordPerformanceMetric(PerformanceMetric::LATENCY_NS, "portfolio_update", portfolio_latency.mean_value);
            
            // Benchmark 3: Wallet balance check latency
            auto wallet_latency = benchmark_->MeasureLatency([this]() {
                auto wallets = trading_engine_->get_managed_wallets();
                for (const auto& wallet : wallets) {
                    // Mock balance fetch (normally would be external call)
                    volatile double balance = 1000.0 + (rng_() % 10000) / 100.0;
                    (void)balance; // Suppress unused variable warning
                }
            }, 10000);
            
            RecordPerformanceMetric(PerformanceMetric::LATENCY_NS, "wallet_balance_check", wallet_latency.mean_value);
            
            // Performance assertions - ultra-low latency requirements
            HFX_ASSERT_LT(strategy_latency.mean_value, 50000.0); // < 50μs
            HFX_ASSERT_LT(strategy_latency.percentile_99, 100000.0); // 99th percentile < 100μs
            HFX_ASSERT_LT(portfolio_latency.mean_value, 10000.0); // < 10μs
            HFX_ASSERT_LT(wallet_latency.mean_value, 5000.0); // < 5μs
            
            std::cout << "   Strategy execution: " << std::fixed << std::setprecision(0) 
                      << strategy_latency.mean_value << "ns (99th: " << strategy_latency.percentile_99 << "ns)" << std::endl;
            std::cout << "   Portfolio update: " << portfolio_latency.mean_value << "ns" << std::endl;
            std::cout << "   Wallet balance check: " << wallet_latency.mean_value << "ns" << std::endl;
        }
        
    private:
        std::unique_ptr<SmartTradingEngine> trading_engine_;
        std::unique_ptr<PerformanceBenchmark> benchmark_;
    };
    
    // V3 tick engine performance benchmarks
    class V3TickEngineLatencyTest : public TestCase {
    public:
        V3TickEngineLatencyTest() : TestCase("V3TickEngineLatency", TestCategory::PERFORMANCE, TestPriority::HIGH) {
            AddTag("latency");
            AddTag("v3");
            AddTag("uniswap");
            SetTimeout(std::chrono::minutes(3));
        }
        
        void SetUp() override {
            v3_engine_ = std::make_unique<V3TickEngine>();
            HFX_ASSERT_TRUE(v3_engine_->initialize());
            benchmark_ = std::make_unique<PerformanceBenchmark>("v3_tick_engine");
        }
        
        void TearDown() override {
            v3_engine_.reset();
            benchmark_.reset();
        }
        
        void Run() override {
            std::cout << "🔸 Benchmarking V3 tick engine operations..." << std::endl;
            
            // Benchmark 1: Price calculation latency
            auto price_calc_latency = benchmark_->MeasureLatency([this]() {
                double price = 1500.0 + (rng_() % 1000) / 10.0; // Random price around $1500
                auto sqrt_price = v3_engine_->calculateSqrtPriceX96(price);
                volatile auto result = sqrt_price; // Prevent optimization
                (void)result;
            }, 100000);
            
            RecordPerformanceMetric(PerformanceMetric::LATENCY_NS, "price_calculation", price_calc_latency.mean_value);
            
            // Benchmark 2: Tick calculation latency
            auto tick_calc_latency = benchmark_->MeasureLatency([this]() {
                double price = 1000.0 + (rng_() % 2000);
                auto tick = v3_engine_->priceToTick(price);
                volatile auto result = tick;
                (void)result;
            }, 100000);
            
            RecordPerformanceMetric(PerformanceMetric::LATENCY_NS, "tick_calculation", tick_calc_latency.mean_value);
            
            // Benchmark 3: Swap simulation latency
            auto swap_latency = benchmark_->MeasureLatency([this]() {
                std::string pool_address = "0x88e6a0c2ddd26feeb64f039a2c41296fcb3f5640"; // USDC/ETH pool
                uint256_t amount_in = 1000; // 1000 USDC
                
                auto result = v3_engine_->simulateSwap(pool_address, amount_in, true);
                volatile bool success = result.successful;
                (void)success;
            }, 50000);
            
            RecordPerformanceMetric(PerformanceMetric::LATENCY_NS, "swap_simulation", swap_latency.mean_value);
            
            // Ultra-low latency requirements for tick calculations
            HFX_ASSERT_LT(price_calc_latency.mean_value, 1000.0); // < 1μs
            HFX_ASSERT_LT(tick_calc_latency.mean_value, 500.0); // < 500ns
            HFX_ASSERT_LT(swap_latency.mean_value, 5000.0); // < 5μs
            
            std::cout << "   Price calculation: " << std::fixed << std::setprecision(0) 
                      << price_calc_latency.mean_value << "ns" << std::endl;
            std::cout << "   Tick calculation: " << tick_calc_latency.mean_value << "ns" << std::endl;
            std::cout << "   Swap simulation: " << swap_latency.mean_value << "ns" << std::endl;
        }
        
    private:
        std::unique_ptr<V3TickEngine> v3_engine_;
        std::unique_ptr<PerformanceBenchmark> benchmark_;
    };
    
    // MEV Shield performance benchmarks
    class MEVShieldLatencyTest : public TestCase {
    public:
        MEVShieldLatencyTest() : TestCase("MEVShieldLatency", TestCategory::PERFORMANCE, TestPriority::HIGH) {
            AddTag("latency");
            AddTag("mev");
            AddTag("protection");
        }
        
        void SetUp() override {
            mev_shield_ = std::make_unique<MEVShield>();
            HFX_ASSERT_TRUE(mev_shield_->initialize());
            HFX_ASSERT_TRUE(mev_shield_->start());
            benchmark_ = std::make_unique<PerformanceBenchmark>("mev_shield");
        }
        
        void TearDown() override {
            if (mev_shield_) {
                mev_shield_->stop();
                mev_shield_.reset();
            }
            benchmark_.reset();
        }
        
        void Run() override {
            std::cout << "🔸 Benchmarking MEV shield operations..." << std::endl;
            
            // Benchmark 1: Protection request latency
            auto protection_latency = benchmark_->MeasureLatency([this]() {
                MEVProtectionRequest request;
                request.transaction_hash = "0x" + std::to_string(rng_());
                request.user_address = "0x" + std::to_string(rng_());
                request.protection_level = MEVProtectionLevel::MEDIUM;
                
                auto result = mev_shield_->request_protection(request);
                volatile bool success = result.success;
                (void)success;
            }, 10000);
            
            RecordPerformanceMetric(PerformanceMetric::LATENCY_NS, "protection_request", protection_latency.mean_value);
            
            // Benchmark 2: Status check latency
            auto status_latency = benchmark_->MeasureLatency([this]() {
                std::string tx_hash = "0x" + std::to_string(rng_());
                auto status = mev_shield_->get_protection_status(tx_hash);
                volatile bool found = (status.status != TransactionStatus::UNKNOWN);
                (void)found;
            }, 50000);
            
            RecordPerformanceMetric(PerformanceMetric::LATENCY_NS, "status_check", status_latency.mean_value);
            
            // MEV protection must be ultra-fast to be effective
            HFX_ASSERT_LT(protection_latency.mean_value, 10000.0); // < 10μs
            HFX_ASSERT_LT(status_latency.mean_value, 1000.0); // < 1μs
            
            std::cout << "   Protection request: " << std::fixed << std::setprecision(0) 
                      << protection_latency.mean_value << "ns" << std::endl;
            std::cout << "   Status check: " << status_latency.mean_value << "ns" << std::endl;
        }
        
    private:
        std::unique_ptr<MEVShield> mev_shield_;
        std::unique_ptr<PerformanceBenchmark> benchmark_;
    };
    
    // Jito MEV engine performance benchmarks
    class JitoMEVLatencyTest : public TestCase {
    public:
        JitoMEVLatencyTest() : TestCase("JitoMEVLatency", TestCategory::PERFORMANCE, TestPriority::MEDIUM) {
            AddTag("latency");
            AddTag("jito");
            AddTag("solana");
        }
        
        void SetUp() override {
            jito_engine_ = std::make_unique<JitoMEVEngine>();
            HFX_ASSERT_TRUE(jito_engine_->initialize());
            benchmark_ = std::make_unique<PerformanceBenchmark>("jito_mev");
        }
        
        void TearDown() override {
            jito_engine_.reset();
            benchmark_.reset();
        }
        
        void Run() override {
            std::cout << "🔸 Benchmarking Jito MEV operations..." << std::endl;
            
            // Benchmark 1: Bundle creation latency
            auto bundle_creation_latency = benchmark_->MeasureLatency([this]() {
                std::vector<SolanaTransaction> transactions;
                
                // Create mock transactions
                for (int i = 0; i < 5; ++i) {
                    SolanaTransaction tx;
                    tx.signature = "sig_" + std::to_string(rng_());
                    tx.from_address = "from_" + std::to_string(rng_());
                    tx.to_address = "to_" + std::to_string(rng_());
                    tx.amount = rng_() % 1000000;
                    transactions.push_back(tx);
                }
                
                auto bundle = jito_engine_->create_bundle(transactions);
                volatile bool success = !bundle.bundle_id.empty();
                (void)success;
            }, 5000);
            
            RecordPerformanceMetric(PerformanceMetric::LATENCY_NS, "bundle_creation", bundle_creation_latency.mean_value);
            
            // Benchmark 2: MEV opportunity estimation latency
            auto mev_estimation_latency = benchmark_->MeasureLatency([this]() {
                SolanaTransaction tx;
                tx.signature = "test_sig";
                tx.from_address = "test_from";
                tx.to_address = "test_to";
                tx.amount = 1000000;
                
                // Mock MEV estimation (normally complex calculation)
                volatile double estimated_value = tx.amount * 0.001; // 0.1% MEV
                (void)estimated_value;
            }, 20000);
            
            RecordPerformanceMetric(PerformanceMetric::LATENCY_NS, "mev_estimation", mev_estimation_latency.mean_value);
            
            // Solana MEV operations should be sub-millisecond
            HFX_ASSERT_LT(bundle_creation_latency.mean_value, 100000.0); // < 100μs
            HFX_ASSERT_LT(mev_estimation_latency.mean_value, 50000.0); // < 50μs
            
            std::cout << "   Bundle creation: " << std::fixed << std::setprecision(0) 
                      << bundle_creation_latency.mean_value << "ns" << std::endl;
            std::cout << "   MEV estimation: " << mev_estimation_latency.mean_value << "ns" << std::endl;
        }
        
    private:
        std::unique_ptr<JitoMEVEngine> jito_engine_;
        std::unique_ptr<PerformanceBenchmark> benchmark_;
    };
    
    // Memory allocation performance benchmark
    class MemoryAllocationBenchmark : public TestCase {
    public:
        MemoryAllocationBenchmark() : TestCase("MemoryAllocation", TestCategory::PERFORMANCE, TestPriority::MEDIUM) {
            AddTag("memory");
            AddTag("allocation");
            AddTag("performance");
        }
        
        void Run() override {
            std::cout << "🔸 Benchmarking memory allocation patterns..." << std::endl;
            
            PerformanceBenchmark benchmark("memory_allocation");
            
            // Benchmark 1: Small object allocation
            auto small_alloc_latency = benchmark.MeasureLatency([this]() {
                auto ptr = std::make_unique<double>(42.0);
                volatile double value = *ptr;
                (void)value;
            }, 100000);
            
            RecordPerformanceMetric(PerformanceMetric::LATENCY_NS, "small_allocation", small_alloc_latency.mean_value);
            
            // Benchmark 2: Vector allocation and growth
            auto vector_alloc_latency = benchmark.MeasureLatency([this]() {
                std::vector<double> vec;
                vec.reserve(1000);
                for (int i = 0; i < 100; ++i) {
                    vec.push_back(i * 3.14159);
                }
                volatile size_t size = vec.size();
                (void)size;
            }, 10000);
            
            RecordPerformanceMetric(PerformanceMetric::LATENCY_NS, "vector_allocation", vector_alloc_latency.mean_value);
            
            // Benchmark 3: String allocation
            auto string_alloc_latency = benchmark.MeasureLatency([this]() {
                std::string str = "This is a test string for allocation benchmark";
                str += std::to_string(rng_());
                volatile size_t length = str.length();
                (void)length;
            }, 50000);
            
            RecordPerformanceMetric(PerformanceMetric::LATENCY_NS, "string_allocation", string_alloc_latency.mean_value);
            
            // Memory allocations should be fast for high-frequency trading
            HFX_ASSERT_LT(small_alloc_latency.mean_value, 1000.0); // < 1μs
            HFX_ASSERT_LT(vector_alloc_latency.mean_value, 10000.0); // < 10μs
            HFX_ASSERT_LT(string_alloc_latency.mean_value, 5000.0); // < 5μs
            
            std::cout << "   Small allocation: " << std::fixed << std::setprecision(0) 
                      << small_alloc_latency.mean_value << "ns" << std::endl;
            std::cout << "   Vector allocation: " << vector_alloc_latency.mean_value << "ns" << std::endl;
            std::cout << "   String allocation: " << string_alloc_latency.mean_value << "ns" << std::endl;
        }
    };
    
    // Throughput benchmark
    class ThroughputBenchmark : public TestCase {
    public:
        ThroughputBenchmark() : TestCase("Throughput", TestCategory::PERFORMANCE, TestPriority::MEDIUM) {
            AddTag("throughput");
            AddTag("operations");
            SetTimeout(std::chrono::minutes(2));
        }
        
        void Run() override {
            std::cout << "🔸 Benchmarking system throughput..." << std::endl;
            
            PerformanceBenchmark benchmark("throughput");
            
            // Benchmark 1: Mathematical operations throughput
            auto math_throughput = benchmark.MeasureThroughput([this]() {
                volatile double result = 0.0;
                for (int i = 0; i < 1000; ++i) {
                    result += std::sin(i * 0.001) * std::cos(i * 0.002);
                }
                (void)result;
            }, std::chrono::milliseconds(1000));
            
            RecordPerformanceMetric(PerformanceMetric::THROUGHPUT_OPS, "math_operations", math_throughput.value);
            
            // Benchmark 2: Hash computation throughput
            auto hash_throughput = benchmark.MeasureThroughput([this]() {
                std::string data = "benchmark_data_" + std::to_string(rng_());
                volatile size_t hash_value = std::hash<std::string>{}(data);
                (void)hash_value;
            }, std::chrono::milliseconds(1000));
            
            RecordPerformanceMetric(PerformanceMetric::THROUGHPUT_OPS, "hash_computation", hash_throughput.value);
            
            // Benchmark 3: Memory copy throughput
            auto memory_throughput = benchmark.MeasureThroughput([this]() {
                std::vector<uint8_t> src(1024);
                std::vector<uint8_t> dst(1024);
                std::memcpy(dst.data(), src.data(), 1024);
                volatile size_t size = dst.size();
                (void)size;
            }, std::chrono::milliseconds(1000));
            
            RecordPerformanceMetric(PerformanceMetric::THROUGHPUT_OPS, "memory_copy", memory_throughput.value);
            
            // High-frequency trading requires high throughput
            HFX_ASSERT_GT(math_throughput.value, 1000.0); // > 1K ops/sec
            HFX_ASSERT_GT(hash_throughput.value, 10000.0); // > 10K ops/sec
            HFX_ASSERT_GT(memory_throughput.value, 50000.0); // > 50K ops/sec
            
            std::cout << "   Math operations: " << std::fixed << std::setprecision(0) 
                      << math_throughput.value << " ops/sec" << std::endl;
            std::cout << "   Hash computation: " << hash_throughput.value << " ops/sec" << std::endl;
            std::cout << "   Memory copy: " << memory_throughput.value << " ops/sec" << std::endl;
        }
    };
};

// Static member definition
std::mt19937 LatencyBenchmarkSuite::rng_;

// Performance test runner
int main() {
    auto test_runner = TestRunnerFactory::CreatePerformanceTestRunner();
    
    // Configure for performance testing
    test_runner->SetVerboseMode(true);
    test_runner->SetOutputFormat("json");
    test_runner->SetOutputFile("performance_benchmarks.json");
    
    // Set up performance-specific callbacks
    test_runner->SetSuiteStartCallback([](const std::string& suite_name) {
        std::cout << "\n⚡ Starting Performance Benchmark Suite: " << suite_name << std::endl;
        std::cout << "=========================================" << std::endl;
        std::cout << "🎯 Target: Ultra-low latency (sub-microsecond for critical paths)" << std::endl;
        std::cout << "🎯 Target: High throughput (>100K ops/sec for core operations)" << std::endl;
        std::cout << std::endl;
    });
    
    test_runner->SetSuiteEndCallback([](const std::string& suite_name, const std::vector<TestResult>& results) {
        std::cout << "\n⚡ Performance Benchmark Results for " << suite_name << std::endl;
        std::cout << "================================================" << std::endl;
        
        double total_avg_latency = 0.0;
        double total_max_latency = 0.0;
        int latency_tests = 0;
        
        for (const auto& result : results) {
            auto latency_it = result.performance_metrics.find(PerformanceMetric::LATENCY_NS);
            if (latency_it != result.performance_metrics.end()) {
                total_avg_latency += latency_it->second;
                total_max_latency = std::max(total_max_latency, latency_it->second);
                latency_tests++;
            }
        }
        
        if (latency_tests > 0) {
            double avg_latency = total_avg_latency / latency_tests;
            std::cout << "📊 Average Latency Across Tests: " << std::fixed << std::setprecision(0) 
                      << avg_latency << "ns" << std::endl;
            std::cout << "📊 Maximum Latency: " << total_max_latency << "ns" << std::endl;
            
            // Performance assessment
            if (avg_latency < 10000.0) {
                std::cout << "🎉 EXCELLENT: Average latency < 10μs" << std::endl;
            } else if (avg_latency < 50000.0) {
                std::cout << "✅ GOOD: Average latency < 50μs" << std::endl;
            } else if (avg_latency < 100000.0) {
                std::cout << "⚠️  ACCEPTABLE: Average latency < 100μs" << std::endl;
            } else {
                std::cout << "❌ POOR: Average latency > 100μs - optimization needed" << std::endl;
            }
        }
        
        std::cout << std::endl;
    });
    
    test_runner->SetTestStartCallback([](const TestContext& context) {
        std::cout << "🚀 Benchmarking: " << context.test_name << std::endl;
        std::cout << "   Category: Performance" << std::endl;
        std::cout << "   Priority: " << (context.priority == TestPriority::CRITICAL ? "CRITICAL" : 
                                        context.priority == TestPriority::HIGH ? "HIGH" : 
                                        context.priority == TestPriority::MEDIUM ? "MEDIUM" : "LOW") << std::endl;
    });
    
    test_runner->SetTestEndCallback([](const TestResult& result) {
        std::string status_emoji;
        switch (result.status) {
            case TestStatus::PASSED: status_emoji = "✅"; break;
            case TestStatus::FAILED: status_emoji = "❌"; break;
            case TestStatus::ERROR: status_emoji = "💥"; break;
            default: status_emoji = "❓"; break;
        }
        
        double execution_ms = result.execution_time.count() / 1000000.0;
        std::cout << status_emoji << " Benchmark " << result.test_name 
                  << " completed in " << std::fixed << std::setprecision(2) 
                  << execution_ms << "ms" << std::endl;
        
        // Show detailed performance metrics
        for (const auto& [metric_type, value] : result.performance_metrics) {
            std::cout << "   📊 ";
            switch (metric_type) {
                case PerformanceMetric::LATENCY_NS:
                    std::cout << "Latency: " << std::fixed << std::setprecision(0) << value << "ns";
                    if (value < 1000.0) std::cout << " 🎉";
                    else if (value < 10000.0) std::cout << " ✅";
                    else if (value < 100000.0) std::cout << " ⚠️";
                    else std::cout << " ❌";
                    break;
                case PerformanceMetric::THROUGHPUT_OPS:
                    std::cout << "Throughput: " << std::fixed << std::setprecision(0) << value << " ops/sec";
                    if (value > 100000.0) std::cout << " 🎉";
                    else if (value > 10000.0) std::cout << " ✅";
                    else if (value > 1000.0) std::cout << " ⚠️";
                    else std::cout << " ❌";
                    break;
                case PerformanceMetric::MEMORY_BYTES:
                    std::cout << "Memory: " << std::fixed << std::setprecision(0) << value << " bytes";
                    break;
                default:
                    std::cout << "Custom: " << std::fixed << std::setprecision(2) << value;
                    break;
            }
            std::cout << std::endl;
        }
        
        if (result.status != TestStatus::PASSED) {
            std::cout << "   ❌ Error: " << result.error_message << std::endl;
            if (!result.failure_details.empty()) {
                std::cout << "   📋 Details: " << result.failure_details << std::endl;
            }
        }
        
        std::cout << std::endl;
    });
    
    // Register and run benchmarks
    auto suite = std::make_shared<LatencyBenchmarkSuite>();
    test_runner->RegisterTestSuite(suite);
    
    std::cout << "⚡ HydraFlow-X Ultra-Low Latency Performance Benchmarks" << std::endl;
    std::cout << "=======================================================" << std::endl;
    std::cout << "🎯 Measuring critical path latencies for sub-microsecond performance" << std::endl;
    std::cout << "🎯 Evaluating throughput for high-frequency trading scenarios" << std::endl;
    
    test_runner->RunAllTests();
    
    // Generate comprehensive performance report
    test_runner->GenerateReport();
    test_runner->GeneratePerformanceReport();
    
    auto stats = test_runner->GetStatistics();
    
    std::cout << "\n📊 PERFORMANCE BENCHMARK SUMMARY" << std::endl;
    std::cout << "=================================" << std::endl;
    std::cout << "Total Benchmarks: " << stats.total_tests << std::endl;
    std::cout << "Passed: " << stats.passed_tests << std::endl;
    std::cout << "Failed: " << stats.failed_tests << std::endl;
    std::cout << "Success Rate: " << std::fixed << std::setprecision(2) << stats.success_rate << "%" << std::endl;
    std::cout << "Total Benchmark Time: " << stats.total_execution_time.count() << "ms" << std::endl;
    
    if (stats.avg_latency_ns > 0) {
        std::cout << "\n⚡ LATENCY SUMMARY" << std::endl;
        std::cout << "=================" << std::endl;
        std::cout << "Average Latency: " << std::fixed << std::setprecision(0) 
                  << stats.avg_latency_ns << "ns" << std::endl;
        std::cout << "Maximum Latency: " << stats.max_latency_ns << "ns" << std::endl;
        
        // Overall performance grade
        if (stats.avg_latency_ns < 10000.0) {
            std::cout << "\n🎉 PERFORMANCE GRADE: A+ (Ultra-low latency achieved!)" << std::endl;
        } else if (stats.avg_latency_ns < 50000.0) {
            std::cout << "\n✅ PERFORMANCE GRADE: A (Excellent low latency)" << std::endl;
        } else if (stats.avg_latency_ns < 100000.0) {
            std::cout << "\n⚠️  PERFORMANCE GRADE: B (Good latency, room for improvement)" << std::endl;
        } else {
            std::cout << "\n❌ PERFORMANCE GRADE: C (Latency optimization required)" << std::endl;
        }
    }
    
    bool all_passed = (stats.failed_tests == 0 && stats.error_tests == 0);
    std::cout << "\n" << (all_passed ? "🎉 ALL PERFORMANCE BENCHMARKS PASSED! 🎉" : "❌ SOME BENCHMARKS FAILED") << std::endl;
    
    return all_passed ? 0 : 1;
}
