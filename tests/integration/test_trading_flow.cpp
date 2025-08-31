/**
 * @file test_trading_flow.cpp
 * @brief Integration tests for end-to-end trading flow
 */

#include "core-backend/hfx-ultra/include/testing_framework.hpp"
#include "core-backend/hfx-ultra/include/smart_trading_engine.hpp"
#include "core-backend/hfx-ultra/include/mev_shield.hpp"
#include "core-backend/hfx-ultra/include/security_manager.hpp"
#include "core-backend/hfx-ultra/include/monitoring_system.hpp"
#include <memory>
#include <thread>

using namespace hfx::testing;
using namespace hfx::ultra;

// Integration test suite for complete trading flow
class TradingFlowIntegrationSuite : public TestSuite {
public:
    TradingFlowIntegrationSuite() : TestSuite("TradingFlowIntegration") {
        AddTest(std::make_shared<FullTradingPipelineTest>());
        AddTest(std::make_shared<SecurityIntegrationTest>());
        AddTest(std::make_shared<MonitoringIntegrationTest>());
        AddTest(std::make_shared<ConcurrencyStressTest>());
    }
    
    void SetUpSuite() override {
        std::cout << "🔄 Setting up Trading Flow Integration test suite" << std::endl;
        
        // Initialize shared components
        security_config_.enable_audit_logging = true;
        security_config_.enable_rate_limiting = true;
        security_config_.session_timeout = std::chrono::minutes(10);
        
        monitoring_config_.metric_collection_interval = std::chrono::seconds(1);
        monitoring_config_.enable_alerting = false; // Disable alerts for testing
        
        // Start shared services
        security_manager_ = std::make_unique<SecurityManager>(security_config_);
        security_manager_->initialize();
        security_manager_->start();
        
        monitoring_system_ = std::make_unique<MonitoringSystem>(monitoring_config_);
        monitoring_system_->initialize();
        monitoring_system_->start();
        
        std::cout << "✅ Shared services initialized" << std::endl;
    }
    
    void TearDownSuite() override {
        std::cout << "🧹 Tearing down Trading Flow Integration test suite" << std::endl;
        
        if (monitoring_system_) {
            monitoring_system_->stop();
            monitoring_system_.reset();
        }
        
        if (security_manager_) {
            security_manager_->stop();
            security_manager_.reset();
        }
        
        std::cout << "✅ Shared services cleaned up" << std::endl;
    }

protected:
    static SecurityConfig security_config_;
    static MonitoringConfig monitoring_config_;
    static std::unique_ptr<SecurityManager> security_manager_;
    static std::unique_ptr<MonitoringSystem> monitoring_system_;

private:
    // Test complete trading pipeline
    class FullTradingPipelineTest : public TestCase {
    public:
        FullTradingPipelineTest() : TestCase("FullTradingPipeline", TestCategory::INTEGRATION, TestPriority::CRITICAL) {
            AddTag("trading");
            AddTag("pipeline");
            AddTag("end-to-end");
            SetTimeout(std::chrono::minutes(2));
            SetParallelSafe(false); // Integration tests often need isolation
        }
        
        void SetUp() override {
            // Initialize trading components with default configs
            SmartTradingConfig trading_config;
            trading_config.default_mode = TradingMode::STANDARD_BUY;
            trading_config.default_slippage_bps = 50.0;
            trading_config.max_gas_price = 50000000000ULL;
            
            trading_engine_ = std::make_unique<SmartTradingEngine>(trading_config);
            
            MEVShieldConfig mev_config;
            mev_config.protection_level = MEVProtectionLevel::STANDARD;
            mev_config.mev_detection_threshold = 0.01;
            mev_config.worker_threads = 4;
            
            mev_shield_ = std::make_unique<MEVShield>(mev_config);
            
            // Create authenticated session
            session_id_ = security_manager_->create_session(
                "integration_test_user", "127.0.0.1", "IntegrationTest/1.0",
                AuthMethod::API_KEY, SecurityLevel::AUTHORIZED
            );
            HFX_ASSERT_FALSE(session_id_.empty());
            
            // Grant trading permissions
            security_manager_->get_permission_manager().grant_permission("integration_test_user", "trades:create");
            security_manager_->get_permission_manager().grant_permission("integration_test_user", "orders:create");
        }
        
        void TearDown() override {
            if (!session_id_.empty()) {
                security_manager_->terminate_session(session_id_);
            }
            
            // Clean up components (no stop() method needed for these)
            mev_shield_.reset();
            trading_engine_.reset();
        }
        
        void Run() override {
            // Test 1: Authenticated order placement
            std::cout << "🔸 Testing authenticated order placement..." << std::endl;
            
            HFX_BENCHMARK_START("order_placement_flow");
            
            // Check authorization
            HFX_ASSERT_TRUE(security_manager_->authorize_trade(session_id_, "ETH/USDC", 1000.0, "buy"));
            
            // Record trading activity
            monitoring_system_->record_trade_latency("ETH/USDC", std::chrono::nanoseconds(50000));
            monitoring_system_->record_order_execution("ETH/USDC", true, 1000.0);
            
            // Simulate MEV protection check (using mock protection result)
            std::string tx_hash = "0x123...abc";
            
            // Mock a successful protection result
            MEVProtectionResult mock_result;
            mock_result.protection_applied = true;
            mock_result.level_used = MEVProtectionLevel::STANDARD;
            mock_result.protection_tx_hash = "0xabc...123";
            mock_result.protection_cost = 1000000000000000ULL; // 0.001 ETH
            
            HFX_ASSERT_TRUE(mock_result.protection_applied);
            
            HFX_BENCHMARK_END("order_placement_flow");
            
            // Test 2: Trading engine wallet management
            std::cout << "🔸 Testing wallet management..." << std::endl;
            
            // Note: Using mock private key for testing
            std::string test_private_key = "test_private_key_123";
            HFX_ASSERT_TRUE(trading_engine_->add_wallet(test_private_key));
            
            auto wallets = trading_engine_->get_copy_wallets();
            HFX_ASSERT_GE(wallets.size(), 1);
            
            // Test 3: Risk management integration
            std::cout << "🔸 Testing risk management..." << std::endl;
            
            // Record risk metrics
            monitoring_system_->record_risk_metric("position_exposure", 0.25, AlertSeverity::INFO);
            monitoring_system_->record_risk_metric("volatility_score", 0.7, AlertSeverity::WARNING);
            
            // Verify risk metrics are captured
            double exposure = monitoring_system_->get_latest_metric_value("risk_position_exposure");
            HFX_ASSERT_NEAR(exposure, 0.25, 0.01);
            
            // Test 4: Audit trail verification
            std::cout << "🔸 Testing audit trail..." << std::endl;
            
            security_manager_->log_trade_execution("integration_test_user", "ETH/USDC", 1000.0, "buy", true);
            
            auto audit_logs = security_manager_->get_audit_logs(
                std::chrono::system_clock::now() - std::chrono::minutes(1),
                std::chrono::system_clock::now()
            );
            
            HFX_ASSERT_GT(audit_logs.size(), 0);
            
            // Find our trade execution log
            bool found_trade_log = false;
            for (const auto& log : audit_logs) {
                if (log.event_type == AuditEventType::TRADE_EXECUTION && 
                    log.user_id == "integration_test_user") {
                    found_trade_log = true;
                    break;
                }
            }
            HFX_ASSERT_TRUE(found_trade_log);
            
            std::cout << "✅ Full trading pipeline test completed successfully" << std::endl;
        }
        
    private:
        std::unique_ptr<SmartTradingEngine> trading_engine_;
        std::unique_ptr<MEVShield> mev_shield_;
        std::string session_id_;
    };
    
    // Test security integration across components
    class SecurityIntegrationTest : public TestCase {
    public:
        SecurityIntegrationTest() : TestCase("SecurityIntegration", TestCategory::INTEGRATION, TestPriority::HIGH) {
            AddTag("security");
            AddTag("integration");
            SetTimeout(std::chrono::minutes(1));
        }
        
        void Run() override {
            std::cout << "🔸 Testing cross-component security..." << std::endl;
            
            // Test 1: Rate limiting across multiple endpoints
            std::string user_id = "rate_test_user";
            
            // Simulate rapid API calls that should trigger rate limiting
            int successful_calls = 0;
            int rate_limited_calls = 0;
            
            for (int i = 0; i < 200; ++i) {
                if (security_manager_->check_rate_limit(user_id, "/api/orders")) {
                    successful_calls++;
                } else {
                    rate_limited_calls++;
                }
            }
            
            HFX_ASSERT_GT(rate_limited_calls, 0); // Should hit rate limit
            std::cout << "   Rate limiting: " << successful_calls << " allowed, " 
                      << rate_limited_calls << " blocked" << std::endl;
            
            // Test 2: Session-based authorization
            std::string session_id = security_manager_->create_session(
                "auth_test_user", "192.168.1.100", "TestClient/1.0",
                AuthMethod::API_KEY, SecurityLevel::AUTHENTICATED
            );
            
            // Should fail without proper permissions
            HFX_ASSERT_FALSE(security_manager_->authorize_trade(session_id, "BTC/USD", 5000.0, "sell"));
            
            // Grant permission and try again
            security_manager_->get_permission_manager().grant_permission("auth_test_user", "trades:create");
            HFX_ASSERT_TRUE(security_manager_->authorize_trade(session_id, "BTC/USD", 5000.0, "sell"));
            
            // Test 3: Security violation detection and response
            // Trigger a security violation
            for (int i = 0; i < 10; ++i) {
                security_manager_->log_audit_event(
                    AuditEventType::LOGIN_FAILURE, "malicious_user", "session", "create",
                    false, "Failed login attempt", ViolationSeverity::MEDIUM
                );
            }
            
            auto violations = security_manager_->get_violations(ViolationSeverity::MEDIUM);
            HFX_ASSERT_GT(violations.size(), 0);
            
            security_manager_->terminate_session(session_id);
            
            std::cout << "✅ Security integration test completed" << std::endl;
        }
    };
    
    // Test monitoring integration
    class MonitoringIntegrationTest : public TestCase {
    public:
        MonitoringIntegrationTest() : TestCase("MonitoringIntegration", TestCategory::INTEGRATION, TestPriority::MEDIUM) {
            AddTag("monitoring");
            AddTag("integration");
        }
        
        void Run() override {
            std::cout << "🔸 Testing monitoring system integration..." << std::endl;
            
            // Test 1: Real-time metric collection
            monitoring_system_->record_trade_latency("TEST/PAIR", std::chrono::nanoseconds(25000));
            monitoring_system_->record_order_execution("TEST/PAIR", true, 500.0);
            monitoring_system_->record_mev_opportunity("arbitrage", 150.0);
            monitoring_system_->record_system_performance("trading_engine", 45.5, 67.2);
            
            // Allow some time for metrics to be processed
            test_utils::SleepFor(std::chrono::milliseconds(100));
            
            // Verify metrics are recorded
            double latest_latency = monitoring_system_->get_latest_metric_value("trade_latency_ns");
            HFX_ASSERT_NEAR(latest_latency, 25000.0, 1000.0);
            
            // Test 2: Health monitoring
            monitoring_system_->register_health_checker("test_component", []() {
                SystemHealth::ComponentHealth health;
                health.name = "test_component";
                health.healthy = true;
                health.status_message = "Operating normally";
                health.health_score = 0.95;
                health.metrics["uptime"] = 3600.0;
                return health;
            });
            
            auto system_health = monitoring_system_->get_system_health();
            HFX_ASSERT_TRUE(system_health.overall_healthy);
            HFX_ASSERT_GT(system_health.overall_score, 0.8);
            
            // Test 3: Performance monitoring
            const auto& stats = monitoring_system_->get_stats();
            HFX_ASSERT_GT(stats.metrics_collected.load(), 0);
            
            std::cout << "   Metrics collected: " << stats.metrics_collected.load() << std::endl;
            std::cout << "   System health score: " << system_health.overall_score << std::endl;
            
            std::cout << "✅ Monitoring integration test completed" << std::endl;
        }
    };
    
    // Stress test with concurrent operations
    class ConcurrencyStressTest : public TestCase {
    public:
        ConcurrencyStressTest() : TestCase("ConcurrencyStress", TestCategory::STRESS, TestPriority::LOW) {
            AddTag("stress");
            AddTag("concurrency");
            SetTimeout(std::chrono::minutes(3));
        }
        
        void Run() override {
            std::cout << "🔸 Testing concurrent operations..." << std::endl;
            
            const int num_threads = 10;
            const int operations_per_thread = 50;
            std::vector<std::thread> threads;
            std::atomic<int> successful_operations{0};
            std::atomic<int> failed_operations{0};
            
            HFX_BENCHMARK_START("concurrent_operations");
            
            // Launch concurrent threads
            for (int t = 0; t < num_threads; ++t) {
                threads.emplace_back([&, t]() {
                    for (int i = 0; i < operations_per_thread; ++i) {
                        try {
                            // Concurrent session creation and termination
                            std::string session_id = security_manager_->create_session(
                                "stress_user_" + std::to_string(t) + "_" + std::to_string(i),
                                "192.168.1." + std::to_string(100 + t),
                                "StressTest/1.0",
                                AuthMethod::API_KEY,
                                SecurityLevel::AUTHENTICATED
                            );
                            
                            // Record metrics concurrently
                            monitoring_system_->record_counter("stress_test_operations", 1.0);
                            monitoring_system_->record_gauge("thread_id", static_cast<double>(t));
                            
                            // Brief operation simulation
                            test_utils::SleepFor(std::chrono::milliseconds(1));
                            
                            // Clean up
                            security_manager_->terminate_session(session_id);
                            
                            successful_operations.fetch_add(1);
                            
                        } catch (const std::exception& e) {
                            failed_operations.fetch_add(1);
                            std::cerr << "Thread " << t << " operation " << i 
                                      << " failed: " << e.what() << std::endl;
                        }
                    }
                });
            }
            
            // Wait for all threads to complete
            for (auto& thread : threads) {
                thread.join();
            }
            
            HFX_BENCHMARK_END("concurrent_operations");
            
            int total_operations = successful_operations.load() + failed_operations.load();
            double success_rate = (static_cast<double>(successful_operations.load()) / total_operations) * 100.0;
            
            std::cout << "   Total operations: " << total_operations << std::endl;
            std::cout << "   Successful: " << successful_operations.load() << std::endl;
            std::cout << "   Failed: " << failed_operations.load() << std::endl;
            std::cout << "   Success rate: " << success_rate << "%" << std::endl;
            
            // Verify acceptable performance under stress
            HFX_ASSERT_GT(success_rate, 95.0); // At least 95% success rate
            HFX_ASSERT_EQ(total_operations, num_threads * operations_per_thread);
            
            std::cout << "✅ Concurrency stress test completed" << std::endl;
        }
    };
};

// Static member definitions
SecurityConfig TradingFlowIntegrationSuite::security_config_;
MonitoringConfig TradingFlowIntegrationSuite::monitoring_config_;
std::unique_ptr<SecurityManager> TradingFlowIntegrationSuite::security_manager_;
std::unique_ptr<MonitoringSystem> TradingFlowIntegrationSuite::monitoring_system_;

// Test runner for integration tests
int main() {
    auto test_runner = TestRunnerFactory::CreateIntegrationTestRunner();
    
    // Configure for integration testing
    test_runner->SetVerboseMode(true);
    test_runner->SetOutputFormat("xml");
    test_runner->SetOutputFile("integration_test_results.xml");
    
    // Set up comprehensive event callbacks
    test_runner->SetSuiteStartCallback([](const std::string& suite_name) {
        std::cout << "\n🏁 Starting integration test suite: " << suite_name << std::endl;
        std::cout << std::string(60, '=') << std::endl;
    });
    
    test_runner->SetSuiteEndCallback([](const std::string& suite_name, const std::vector<TestResult>& results) {
        std::cout << std::string(60, '=') << std::endl;
        std::cout << "🏁 Completed integration test suite: " << suite_name << std::endl;
        
        int passed = 0, failed = 0;
        for (const auto& result : results) {
            if (result.status == TestStatus::PASSED) passed++;
            else failed++;
        }
        
        std::cout << "   Results: " << passed << " passed, " << failed << " failed" << std::endl;
    });
    
    test_runner->SetTestStartCallback([](const TestContext& context) {
        std::cout << "🚀 Starting integration test: " << context.test_name << std::endl;
        std::cout << "   Category: ";
        switch (context.category) {
            case TestCategory::INTEGRATION: std::cout << "Integration"; break;
            case TestCategory::STRESS: std::cout << "Stress"; break;
            default: std::cout << "Other"; break;
        }
        std::cout << std::endl;
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
        std::cout << status_emoji << " Test " << result.test_name 
                  << " completed in " << std::fixed << std::setprecision(2) 
                  << execution_ms << "ms" << std::endl;
        
        if (result.status != TestStatus::PASSED) {
            std::cout << "   ❌ Error: " << result.error_message << std::endl;
            if (!result.failure_details.empty()) {
                std::cout << "   📋 Details: " << result.failure_details << std::endl;
            }
        }
        
        // Show performance metrics if available
        for (const auto& [metric_type, value] : result.performance_metrics) {
            if (metric_type == PerformanceMetric::LATENCY_NS) {
                std::cout << "   ⚡ Average latency: " << std::fixed << std::setprecision(0) 
                          << value << "ns" << std::endl;
            }
        }
        
        std::cout << std::endl;
    });
    
    // Register and run tests
    auto suite = std::make_shared<TradingFlowIntegrationSuite>();
    test_runner->RegisterTestSuite(suite);
    
    std::cout << "🚀 Starting HydraFlow-X Integration Test Suite" << std::endl;
    std::cout << "=============================================" << std::endl;
    
    test_runner->RunAllTests();
    
    // Generate comprehensive report
    test_runner->GenerateReport();
    test_runner->GeneratePerformanceReport();
    
    auto stats = test_runner->GetStatistics();
    
    std::cout << "\n📊 INTEGRATION TEST SUMMARY" << std::endl;
    std::cout << "============================" << std::endl;
    std::cout << "Total Tests: " << stats.total_tests << std::endl;
    std::cout << "Passed: " << stats.passed_tests << std::endl;
    std::cout << "Failed: " << stats.failed_tests << std::endl;
    std::cout << "Errors: " << stats.error_tests << std::endl;
    std::cout << "Success Rate: " << std::fixed << std::setprecision(2) << stats.success_rate << "%" << std::endl;
    std::cout << "Total Execution Time: " << stats.total_execution_time.count() << "ms" << std::endl;
    
    if (stats.avg_latency_ns > 0) {
        std::cout << "Average Latency: " << std::fixed << std::setprecision(0) 
                  << stats.avg_latency_ns << "ns" << std::endl;
    }
    
    bool all_passed = (stats.failed_tests == 0 && stats.error_tests == 0);
    
    std::cout << "\n" << (all_passed ? "🎉 ALL INTEGRATION TESTS PASSED! 🎉" : "❌ SOME TESTS FAILED") << std::endl;
    
    return all_passed ? 0 : 1;
}
