/**
 * @file test_basic_benchmarks.cpp
 * @brief Basic performance benchmarks for HydraFlow-X core functionality
 */

#include "core-backend/hfx-ultra/include/testing_framework.hpp"
#include <memory>
#include <random>
#include <algorithm>
#include <numeric>
#include <cmath>

using namespace hfx::testing;

// Basic performance benchmark suite focusing on computational performance
class BasicBenchmarkSuite : public TestSuite {
public:
    BasicBenchmarkSuite() : TestSuite("BasicBenchmarks") {
        AddTest(std::make_shared<ComputationalBenchmark>());
        AddTest(std::make_shared<MemoryBenchmark>());
        AddTest(std::make_shared<AlgorithmBenchmark>());
        AddTest(std::make_shared<ConcurrencyBenchmark>());
        AddTest(std::make_shared<TestingFrameworkBenchmark>());
    }
    
    void SetUpSuite() override {
        std::cout << "⚡ Setting up Basic Performance Benchmark Suite" << std::endl;
        std::cout << "Target: Measure core computational and framework performance" << std::endl;
        
        // Initialize random number generator for reproducible tests
        rng_.seed(42);
    }
    
    void TearDownSuite() override {
        std::cout << "⚡ Basic performance benchmarks completed" << std::endl;
    }

protected:
    static std::mt19937 rng_;

private:
    // Computational performance benchmarks
    class ComputationalBenchmark : public TestCase {
    public:
        ComputationalBenchmark() : TestCase("ComputationalPerformance", TestCategory::PERFORMANCE, TestPriority::HIGH) {
            AddTag("computation");
            AddTag("math");
            SetTimeout(std::chrono::minutes(2));
        }
        
        void Run() override {
            std::cout << "🔸 Benchmarking computational performance..." << std::endl;
            
            PerformanceBenchmark benchmark("computation");
            
            // Benchmark 1: Mathematical computations (common in trading algorithms)
            auto math_latency = benchmark.MeasureLatency([this]() {
                volatile double result = 0.0;
                for (int i = 0; i < 1000; ++i) {
                    double x = i * 0.001;
                    result += std::sin(x) * std::cos(x) + std::exp(-x) + std::log(1.0 + x);
                }
                (void)result; // Prevent optimization
            }, 10000);
            
            RecordPerformanceMetric(PerformanceMetric::LATENCY_NS, "math_computation", math_latency.mean_value);
            
            // Benchmark 2: Integer operations
            auto int_latency = benchmark.MeasureLatency([this]() {
                volatile uint64_t result = 0;
                for (int i = 0; i < 1000; ++i) {
                    result += (i * 17) ^ (i << 3) + (i >> 2);
                }
                (void)result;
            }, 10000);
            
            RecordPerformanceMetric(PerformanceMetric::LATENCY_NS, "integer_operations", int_latency.mean_value);
            
            // Benchmark 3: Floating point division (expensive operation)
            auto div_latency = benchmark.MeasureLatency([this]() {
                volatile double result = 1000000.0;
                for (int i = 1; i < 100; ++i) {
                    result /= (1.0 + i * 0.001);
                }
                (void)result;
            }, 10000);
            
            RecordPerformanceMetric(PerformanceMetric::LATENCY_NS, "floating_division", div_latency.mean_value);
            
            // Performance assertions for computational benchmarks
            HFX_ASSERT_LT(math_latency.mean_value, 100000.0);    // < 100μs
            HFX_ASSERT_LT(int_latency.mean_value, 10000.0);      // < 10μs  
            HFX_ASSERT_LT(div_latency.mean_value, 50000.0);      // < 50μs
            
            std::cout << "   Math computation: " << std::fixed << std::setprecision(0) 
                      << math_latency.mean_value << "ns" << std::endl;
            std::cout << "   Integer operations: " << int_latency.mean_value << "ns" << std::endl;
            std::cout << "   Floating division: " << div_latency.mean_value << "ns" << std::endl;
        }
    };
    
    // Memory performance benchmarks
    class MemoryBenchmark : public TestCase {
    public:
        MemoryBenchmark() : TestCase("MemoryPerformance", TestCategory::PERFORMANCE, TestPriority::MEDIUM) {
            AddTag("memory");
            AddTag("allocation");
        }
        
        void Run() override {
            std::cout << "🔸 Benchmarking memory performance..." << std::endl;
            
            PerformanceBenchmark benchmark("memory");
            
            // Benchmark 1: Vector allocation and growth
            auto vector_latency = benchmark.MeasureLatency([this]() {
                std::vector<double> vec;
                vec.reserve(1000);
                for (int i = 0; i < 1000; ++i) {
                    vec.push_back(i * 3.14159);
                }
                volatile size_t size = vec.size();
                (void)size;
            }, 5000);
            
            RecordPerformanceMetric(PerformanceMetric::LATENCY_NS, "vector_allocation", vector_latency.mean_value);
            
            // Benchmark 2: Memory copy performance
            auto copy_latency = benchmark.MeasureLatency([this]() {
                std::vector<uint64_t> src(1000);
                std::iota(src.begin(), src.end(), 0);
                std::vector<uint64_t> dst(1000);
                std::copy(src.begin(), src.end(), dst.begin());
                volatile size_t sum = std::accumulate(dst.begin(), dst.end(), 0ULL);
                (void)sum;
            }, 5000);
            
            RecordPerformanceMetric(PerformanceMetric::LATENCY_NS, "memory_copy", copy_latency.mean_value);
            
            // Benchmark 3: Heap allocation performance
            auto alloc_latency = benchmark.MeasureLatency([this]() {
                std::vector<std::unique_ptr<double>> ptrs;
                ptrs.reserve(100);
                for (int i = 0; i < 100; ++i) {
                    ptrs.push_back(std::make_unique<double>(i * 2.718));
                }
                volatile size_t count = ptrs.size();
                (void)count;
            }, 5000);
            
            RecordPerformanceMetric(PerformanceMetric::LATENCY_NS, "heap_allocation", alloc_latency.mean_value);
            
            // Memory performance assertions
            HFX_ASSERT_LT(vector_latency.mean_value, 50000.0);   // < 50μs
            HFX_ASSERT_LT(copy_latency.mean_value, 30000.0);     // < 30μs
            HFX_ASSERT_LT(alloc_latency.mean_value, 100000.0);   // < 100μs
            
            std::cout << "   Vector allocation: " << std::fixed << std::setprecision(0) 
                      << vector_latency.mean_value << "ns" << std::endl;
            std::cout << "   Memory copy: " << copy_latency.mean_value << "ns" << std::endl;
            std::cout << "   Heap allocation: " << alloc_latency.mean_value << "ns" << std::endl;
        }
    };
    
    // Algorithm performance benchmarks
    class AlgorithmBenchmark : public TestCase {
    public:
        AlgorithmBenchmark() : TestCase("AlgorithmPerformance", TestCategory::PERFORMANCE, TestPriority::MEDIUM) {
            AddTag("algorithm");
            AddTag("sorting");
        }
        
        void Run() override {
            std::cout << "🔸 Benchmarking algorithm performance..." << std::endl;
            
            PerformanceBenchmark benchmark("algorithm");
            
            // Benchmark 1: Sorting performance
            auto sort_latency = benchmark.MeasureLatency([this]() {
                std::vector<int> data(10000);
                std::iota(data.begin(), data.end(), 0);
                std::shuffle(data.begin(), data.end(), rng_);
                std::sort(data.begin(), data.end());
                volatile bool sorted = std::is_sorted(data.begin(), data.end());
                (void)sorted;
            }, 1000);
            
            RecordPerformanceMetric(PerformanceMetric::LATENCY_NS, "sorting_10k", sort_latency.mean_value);
            
            // Benchmark 2: Search performance
            auto search_latency = benchmark.MeasureLatency([this]() {
                std::vector<int> data(10000);
                std::iota(data.begin(), data.end(), 0);
                
                volatile int found_count = 0;
                for (int i = 0; i < 100; ++i) {
                    int target = rng_() % 10000;
                    auto it = std::binary_search(data.begin(), data.end(), target);
                    if (it) found_count++;
                }
                (void)found_count;
            }, 1000);
            
            RecordPerformanceMetric(PerformanceMetric::LATENCY_NS, "binary_search", search_latency.mean_value);
            
            // Benchmark 3: Hash map performance
            auto hashmap_latency = benchmark.MeasureLatency([this]() {
                std::unordered_map<int, double> map;
                map.reserve(1000);
                
                // Insert
                for (int i = 0; i < 1000; ++i) {
                    map[i] = i * 1.618;
                }
                
                // Lookup
                volatile double sum = 0.0;
                for (int i = 0; i < 1000; ++i) {
                    sum += map[i];
                }
                (void)sum;
            }, 2000);
            
            RecordPerformanceMetric(PerformanceMetric::LATENCY_NS, "hashmap_ops", hashmap_latency.mean_value);
            
            // Algorithm performance assertions
            HFX_ASSERT_LT(sort_latency.mean_value, 500000.0);    // < 500μs for 10k items
            HFX_ASSERT_LT(search_latency.mean_value, 50000.0);   // < 50μs for 100 searches
            HFX_ASSERT_LT(hashmap_latency.mean_value, 100000.0); // < 100μs for 1k ops
            
            std::cout << "   Sorting 10k items: " << std::fixed << std::setprecision(0) 
                      << sort_latency.mean_value << "ns" << std::endl;
            std::cout << "   Binary search (100x): " << search_latency.mean_value << "ns" << std::endl;
            std::cout << "   HashMap operations: " << hashmap_latency.mean_value << "ns" << std::endl;
        }
    };
    
    // Concurrency performance benchmarks
    class ConcurrencyBenchmark : public TestCase {
    public:
        ConcurrencyBenchmark() : TestCase("ConcurrencyPerformance", TestCategory::PERFORMANCE, TestPriority::LOW) {
            AddTag("concurrency");
            AddTag("threading");
        }
        
        void Run() override {
            std::cout << "🔸 Benchmarking concurrency performance..." << std::endl;
            
            PerformanceBenchmark benchmark("concurrency");
            
            // Benchmark 1: Mutex contention
            auto mutex_latency = benchmark.MeasureLatency([this]() {
                std::mutex mtx;
                std::vector<std::thread> threads;
                std::atomic<int> counter{0};
                
                const int num_threads = 4;
                const int ops_per_thread = 100;
                
                for (int t = 0; t < num_threads; ++t) {
                    threads.emplace_back([&mtx, &counter, ops_per_thread]() {
                        for (int i = 0; i < ops_per_thread; ++i) {
                            std::lock_guard<std::mutex> lock(mtx);
                            counter.fetch_add(1);
                        }
                    });
                }
                
                for (auto& thread : threads) {
                    thread.join();
                }
                
                volatile int final_count = counter.load();
                (void)final_count;
            }, 100);
            
            RecordPerformanceMetric(PerformanceMetric::LATENCY_NS, "mutex_contention", mutex_latency.mean_value);
            
            // Benchmark 2: Atomic operations
            auto atomic_latency = benchmark.MeasureLatency([this]() {
                std::atomic<uint64_t> counter{0};
                std::vector<std::thread> threads;
                
                const int num_threads = 4;
                const int ops_per_thread = 1000;
                
                for (int t = 0; t < num_threads; ++t) {
                    threads.emplace_back([&counter, ops_per_thread]() {
                        for (int i = 0; i < ops_per_thread; ++i) {
                            counter.fetch_add(1, std::memory_order_relaxed);
                        }
                    });
                }
                
                for (auto& thread : threads) {
                    thread.join();
                }
                
                volatile uint64_t final_count = counter.load();
                (void)final_count;
            }, 100);
            
            RecordPerformanceMetric(PerformanceMetric::LATENCY_NS, "atomic_operations", atomic_latency.mean_value);
            
            // Concurrency performance assertions
            HFX_ASSERT_LT(mutex_latency.mean_value, 1000000.0);   // < 1ms
            HFX_ASSERT_LT(atomic_latency.mean_value, 500000.0);   // < 500μs
            
            std::cout << "   Mutex contention (4 threads): " << std::fixed << std::setprecision(0) 
                      << mutex_latency.mean_value << "ns" << std::endl;
            std::cout << "   Atomic operations (4 threads): " << atomic_latency.mean_value << "ns" << std::endl;
        }
    };
    
    // Testing framework performance
    class TestingFrameworkBenchmark : public TestCase {
    public:
        TestingFrameworkBenchmark() : TestCase("TestingFramework", TestCategory::PERFORMANCE, TestPriority::LOW) {
            AddTag("framework");
            AddTag("testing");
        }
        
        void Run() override {
            std::cout << "🔸 Benchmarking testing framework performance..." << std::endl;
            
            PerformanceBenchmark benchmark("framework");
            
            // Benchmark 1: Assertion performance
            auto assertion_latency = benchmark.MeasureLatency([this]() {
                for (int i = 0; i < 1000; ++i) {
                    HFX_ASSERT_TRUE(i >= 0);
                    HFX_ASSERT_EQ(i, i);
                    HFX_ASSERT_LT(i, 1000);
                }
            }, 1000);
            
            RecordPerformanceMetric(PerformanceMetric::LATENCY_NS, "assertion_performance", assertion_latency.mean_value);
            
            // Benchmark 2: Performance measurement overhead
            auto measurement_latency = benchmark.MeasureLatency([this]() {
                HFX_BENCHMARK_START("nested_measurement");
                // Simulate some work
                volatile double result = 0.0;
                for (int i = 0; i < 100; ++i) {
                    result += std::sin(i * 0.01);
                }
                HFX_BENCHMARK_END("nested_measurement");
                (void)result;
            }, 1000);
            
            RecordPerformanceMetric(PerformanceMetric::LATENCY_NS, "measurement_overhead", measurement_latency.mean_value);
            
            // Framework performance assertions
            HFX_ASSERT_LT(assertion_latency.mean_value, 100000.0);      // < 100μs for 3000 assertions
            HFX_ASSERT_LT(measurement_latency.mean_value, 50000.0);     // < 50μs measurement overhead
            
            std::cout << "   Assertion performance (3000x): " << std::fixed << std::setprecision(0) 
                      << assertion_latency.mean_value << "ns" << std::endl;
            std::cout << "   Measurement overhead: " << measurement_latency.mean_value << "ns" << std::endl;
        }
    };
};

// Static member definition
std::mt19937 BasicBenchmarkSuite::rng_;

// Performance test runner
int main() {
    auto test_runner = TestRunnerFactory::CreatePerformanceTestRunner();
    
    // Configure for performance testing
    test_runner->SetVerboseMode(true);
    test_runner->SetOutputFormat("json");
    test_runner->SetOutputFile("basic_performance_benchmarks.json");
    
    // Set up performance-specific callbacks
    test_runner->SetSuiteStartCallback([](const std::string& suite_name) {
        std::cout << "\n⚡ Starting Basic Performance Benchmark Suite: " << suite_name << std::endl;
        std::cout << "=============================================" << std::endl;
        std::cout << "🎯 Measuring computational and framework performance" << std::endl;
        std::cout << "🎯 Validating ultra-low latency requirements" << std::endl;
        std::cout << std::endl;
    });
    
    test_runner->SetSuiteEndCallback([](const std::string& suite_name, const std::vector<TestResult>& results) {
        std::cout << "\n⚡ Performance Benchmark Results for " << suite_name << std::endl;
        std::cout << "==========================================" << std::endl;
        
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
            
            // Performance grading
            if (avg_latency < 10000.0) {
                std::cout << "🎉 GRADE: A+ (Ultra-low latency: < 10μs)" << std::endl;
            } else if (avg_latency < 50000.0) {
                std::cout << "✅ GRADE: A (Excellent latency: < 50μs)" << std::endl;
            } else if (avg_latency < 100000.0) {
                std::cout << "⚠️  GRADE: B (Good latency: < 100μs)" << std::endl;
            } else {
                std::cout << "❌ GRADE: C (Optimization needed: > 100μs)" << std::endl;
            }
        }
        
        std::cout << std::endl;
    });
    
    test_runner->SetTestStartCallback([](const TestContext& context) {
        std::cout << "🚀 Benchmarking: " << context.test_name << std::endl;
    });
    
    test_runner->SetTestEndCallback([](const TestResult& result) {
        std::string status_emoji = (result.status == TestStatus::PASSED) ? "✅" : "❌";
        double execution_ms = result.execution_time.count() / 1000000.0;
        
        std::cout << status_emoji << " Benchmark " << result.test_name 
                  << " completed in " << std::fixed << std::setprecision(2) 
                  << execution_ms << "ms" << std::endl;
        
        // Show performance metrics
        for (const auto& [metric_type, value] : result.performance_metrics) {
            std::cout << "   📊 ";
            if (metric_type == PerformanceMetric::LATENCY_NS) {
                std::cout << "Latency: " << std::fixed << std::setprecision(0) << value << "ns";
                if (value < 10000.0) std::cout << " 🎉";
                else if (value < 50000.0) std::cout << " ✅";
                else if (value < 100000.0) std::cout << " ⚠️";
                else std::cout << " ❌";
            } else {
                std::cout << "Metric: " << std::fixed << std::setprecision(2) << value;
            }
            std::cout << std::endl;
        }
        
        if (result.status != TestStatus::PASSED) {
            std::cout << "   ❌ Error: " << result.error_message << std::endl;
        }
        
        std::cout << std::endl;
    });
    
    // Register and run benchmarks
    auto suite = std::make_shared<BasicBenchmarkSuite>();
    test_runner->RegisterTestSuite(suite);
    
    std::cout << "⚡ HydraFlow-X Basic Performance Benchmarks" << std::endl;
    std::cout << "==========================================" << std::endl;
    std::cout << "🎯 Measuring core computational performance" << std::endl;
    std::cout << "🎯 Validating testing framework efficiency" << std::endl;
    
    test_runner->RunAllTests();
    
    // Generate comprehensive performance report
    test_runner->GenerateReport();
    test_runner->GeneratePerformanceReport();
    
    auto stats = test_runner->GetStatistics();
    
    std::cout << "\n📊 BASIC PERFORMANCE BENCHMARK SUMMARY" << std::endl;
    std::cout << "======================================" << std::endl;
    std::cout << "Total Benchmarks: " << stats.total_tests << std::endl;
    std::cout << "Passed: " << stats.passed_tests << std::endl;
    std::cout << "Failed: " << stats.failed_tests << std::endl;
    std::cout << "Success Rate: " << std::fixed << std::setprecision(2) << stats.success_rate << "%" << std::endl;
    
    if (stats.avg_latency_ns > 0) {
        std::cout << "\n⚡ PERFORMANCE SUMMARY" << std::endl;
        std::cout << "=====================" << std::endl;
        std::cout << "Average Latency: " << std::fixed << std::setprecision(0) 
                  << stats.avg_latency_ns << "ns" << std::endl;
        std::cout << "Maximum Latency: " << stats.max_latency_ns << "ns" << std::endl;
        
        // Final performance assessment
        if (stats.avg_latency_ns < 10000.0) {
            std::cout << "\n🎉 EXCELLENT: Ultra-low latency achieved!" << std::endl;
        } else if (stats.avg_latency_ns < 50000.0) {
            std::cout << "\n✅ GOOD: Low latency performance" << std::endl;
        } else {
            std::cout << "\n⚠️  NEEDS OPTIMIZATION: Consider performance improvements" << std::endl;
        }
    }
    
    bool all_passed = (stats.failed_tests == 0 && stats.error_tests == 0);
    std::cout << "\n" << (all_passed ? "🎉 ALL PERFORMANCE BENCHMARKS PASSED! 🎉" : "❌ SOME BENCHMARKS FAILED") << std::endl;
    
    return all_passed ? 0 : 1;
}
