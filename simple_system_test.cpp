/**
 * @file simple_system_test.cpp
 * @brief Simplified testing of all HydraFlow-X systems
 * 
 * Tests basic initialization and functionality without complex API interactions
 */

#include <iostream>
#include <memory>
#include <chrono>
#include <thread>

// Core HydraFlow modules
#include "hfx-core/include/event_engine.hpp"
#include "hfx-log/include/logger.hpp"

// Network and Strategy modules
#include "hfx-net/include/network_manager.hpp"
#include "hfx-strat/include/strategy_engine.hpp"
#include "hfx-risk/include/risk_manager.hpp"

// Visualization
#include "hfx-viz/include/telemetry_engine.hpp"
#include "hfx-viz/include/terminal_dashboard.hpp"
#include "hfx-viz/include/websocket_server.hpp"

// HFT modules
#include "hfx-hft/include/execution_engine.hpp"

// AI modules (if available)
#ifdef HFX_ENABLE_AI
#include "hfx-ai/include/sentiment_engine.hpp"
#include "hfx-ai/include/llm_decision_system.hpp"
#include "hfx-ai/include/real_time_data_aggregator.hpp"
#endif

class SimpleSystemTester {
private:
    std::unique_ptr<hfx::log::Logger> logger_;
    int tests_passed_ = 0;
    int tests_failed_ = 0;

public:
    SimpleSystemTester() {
        logger_ = std::make_unique<hfx::log::Logger>();
        logger_->info("🧪 HydraFlow-X Simple System Test");
        logger_->info("=================================");
    }

    void run_basic_tests() {
        logger_->info("🚀 Testing System Initialization...");
        
        // Test 1: Core Engine
        test_core_engine();
        
        // Test 2: Network Manager
        test_network_manager();
        
        // Test 3: Strategy Engine
        test_strategy_engine();
        
        // Test 4: Risk Manager
        test_risk_manager();
        
        // Test 5: Visualization System
        test_visualization_system();
        
        // Test 6: HFT Execution
        test_hft_execution();
        
#ifdef HFX_ENABLE_AI
        // Test 7: AI Systems
        test_ai_systems();
#endif
        
        // Final report
        print_results();
    }

private:
    bool test_core_engine() {
        logger_->info("🔧 Testing Core Event Engine...");
        try {
            auto engine = std::make_unique<hfx::core::EventEngine>();
            bool result = engine->initialize();
            
            if (result) {
                logger_->info("✅ Core Event Engine: PASSED");
                tests_passed_++;
                return true;
            } else {
                logger_->error("❌ Core Event Engine: FAILED - initialization");
                tests_failed_++;
                return false;
            }
        } catch (const std::exception& e) {
            logger_->error("💥 Core Event Engine: EXCEPTION - {}", e.what());
            tests_failed_++;
            return false;
        }
    }

    bool test_network_manager() {
        logger_->info("🌐 Testing Network Manager...");
        try {
            auto network = std::make_unique<hfx::net::NetworkManager>();
            bool result = network->initialize();
            
            if (result) {
                logger_->info("✅ Network Manager: PASSED");
                tests_passed_++;
                return true;
            } else {
                logger_->error("❌ Network Manager: FAILED - initialization");
                tests_failed_++;
                return false;
            }
        } catch (const std::exception& e) {
            logger_->error("💥 Network Manager: EXCEPTION - {}", e.what());
            tests_failed_++;
            return false;
        }
    }

    bool test_strategy_engine() {
        logger_->info("📈 Testing Strategy Engine...");
        try {
            auto strategy = std::make_unique<hfx::strat::StrategyEngine>();
            bool result = strategy->initialize();
            
            if (result) {
                logger_->info("✅ Strategy Engine: PASSED");
                tests_passed_++;
                return true;
            } else {
                logger_->error("❌ Strategy Engine: FAILED - initialization");
                tests_failed_++;
                return false;
            }
        } catch (const std::exception& e) {
            logger_->error("💥 Strategy Engine: EXCEPTION - {}", e.what());
            tests_failed_++;
            return false;
        }
    }

    bool test_risk_manager() {
        logger_->info("🛡️ Testing Risk Manager...");
        try {
            auto risk = std::make_unique<hfx::risk::RiskManager>();
            bool result = risk->initialize();
            
            if (result) {
                logger_->info("✅ Risk Manager: PASSED");
                tests_passed_++;
                return true;
            } else {
                logger_->error("❌ Risk Manager: FAILED - initialization");
                tests_failed_++;
                return false;
            }
        } catch (const std::exception& e) {
            logger_->error("💥 Risk Manager: EXCEPTION - {}", e.what());
            tests_failed_++;
            return false;
        }
    }

    bool test_visualization_system() {
        logger_->info("📊 Testing Visualization System...");
        try {
            // Test Telemetry Engine
            auto telemetry = std::make_shared<hfx::viz::TelemetryEngine>();
            bool telemetry_ok = telemetry->initialize();
            
            // Test Terminal Dashboard
            auto dashboard = std::make_unique<hfx::viz::TerminalDashboard>();
            dashboard->set_telemetry_engine(telemetry);
            bool dashboard_ok = dashboard->initialize();
            
            // Test WebSocket Server
            auto websocket = std::make_unique<hfx::viz::WebSocketServer>(8080);
            websocket->set_telemetry_engine(telemetry);
            bool websocket_ok = websocket->start();
            
            // Stop websocket immediately
            if (websocket_ok) {
                websocket->stop();
            }
            
            if (telemetry_ok && dashboard_ok && websocket_ok) {
                logger_->info("✅ Visualization System: PASSED");
                tests_passed_++;
                return true;
            } else {
                logger_->error("❌ Visualization System: FAILED - component initialization");
                tests_failed_++;
                return false;
            }
        } catch (const std::exception& e) {
            logger_->error("💥 Visualization System: EXCEPTION - {}", e.what());
            tests_failed_++;
            return false;
        }
    }

    bool test_hft_execution() {
        logger_->info("⚡ Testing HFT Execution Engine...");
        try {
            // Test with minimal config
            hfx::hft::UltraFastExecutionEngine::Config config;
            // Use default config without setting specific fields
            
            auto execution = std::make_unique<hfx::hft::UltraFastExecutionEngine>(config);
            bool result = execution->initialize();
            
            if (result) {
                logger_->info("✅ HFT Execution Engine: PASSED");
                tests_passed_++;
                return true;
            } else {
                logger_->error("❌ HFT Execution Engine: FAILED - initialization");
                tests_failed_++;
                return false;
            }
        } catch (const std::exception& e) {
            logger_->error("💥 HFT Execution Engine: EXCEPTION - {}", e.what());
            tests_failed_++;
            return false;
        }
    }

#ifdef HFX_ENABLE_AI
    bool test_ai_systems() {
        logger_->info("🤖 Testing AI Systems...");
        int ai_passed = 0;
        int ai_total = 0;
        
        // Test Sentiment Engine
        ai_total++;
        try {
            auto sentiment = std::make_unique<hfx::ai::SentimentEngine>();
            if (sentiment->initialize()) {
                logger_->info("✅ AI Sentiment Engine: PASSED");
                ai_passed++;
            } else {
                logger_->error("❌ AI Sentiment Engine: FAILED");
            }
        } catch (const std::exception& e) {
            logger_->error("💥 AI Sentiment Engine: EXCEPTION - {}", e.what());
        }
        
        // Test LLM Decision System
        ai_total++;
        try {
            auto llm = std::make_unique<hfx::ai::LLMDecisionSystem>();
            if (llm->initialize()) {
                logger_->info("✅ AI LLM Decision System: PASSED");
                ai_passed++;
            } else {
                logger_->error("❌ AI LLM Decision System: FAILED");
            }
        } catch (const std::exception& e) {
            logger_->error("💥 AI LLM Decision System: EXCEPTION - {}", e.what());
        }
        
        // Test Real-Time Data Aggregator
        ai_total++;
        try {
            auto aggregator = std::make_unique<hfx::ai::RealTimeDataAggregator>();
            if (aggregator->initialize()) {
                logger_->info("✅ AI Real-Time Data Aggregator: PASSED");
                ai_passed++;
            } else {
                logger_->error("❌ AI Real-Time Data Aggregator: FAILED");
            }
        } catch (const std::exception& e) {
            logger_->error("💥 AI Real-Time Data Aggregator: EXCEPTION - {}", e.what());
        }
        
        if (ai_passed == ai_total) {
            logger_->info("✅ AI Systems: ALL PASSED ({}/{})", ai_passed, ai_total);
            tests_passed_++;
            return true;
        } else {
            logger_->error("❌ AI Systems: PARTIAL FAILURE ({}/{})", ai_passed, ai_total);
            tests_failed_++;
            return false;
        }
    }
#endif

    void print_results() {
        logger_->info("");
        logger_->info("📋 SYSTEM TEST RESULTS");
        logger_->info("======================");
        logger_->info("✅ Tests Passed: {}", tests_passed_);
        logger_->info("❌ Tests Failed: {}", tests_failed_);
        logger_->info("📊 Success Rate: {:.1f}%", 
                     (tests_passed_ * 100.0) / (tests_passed_ + tests_failed_));
        
        logger_->info("");
        if (tests_failed_ == 0) {
            logger_->info("🎉 ALL SYSTEMS OPERATIONAL!");
            logger_->info("🚀 HydraFlow-X is ready for ultra-fast trading!");
        } else {
            logger_->info("⚠️  {} system(s) need attention", tests_failed_);
            logger_->info("🔧 Check the error messages above");
        }
        logger_->info("==============================================");
    }
};

int main() {
    try {
        SimpleSystemTester tester;
        tester.run_basic_tests();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "💥 Test suite crashed: " << e.what() << std::endl;
        return 1;
    }
}
