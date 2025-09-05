/**
 * @file hft_main.cpp
 * @brief Main HFT integration and demo
 */

#include "memecoin_integrations.hpp"
#include "execution_engine.hpp"
#include "signal_compressor.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <memory>

using namespace hfx::hft;

class HFTDemoSystem {
public:
    HFTDemoSystem() {
        HFX_LOG_INFO("\n🚀 HydraFlow-X Ultra-Low Latency HFT System 🚀\n");
        HFX_LOG_INFO("Initializing fastest memecoin trading engine in the universe...");
        
        // Initialize execution engine
        UltraFastExecutionEngine::Config exec_config;
        exec_config.worker_threads = 8;
        exec_config.enable_cpu_affinity = true;
        exec_config.enable_real_time_priority = true;
        exec_config.max_execution_latency_ns = 100000; // 100μs target
        
        execution_engine_ = std::make_unique<UltraFastExecutionEngine>(exec_config);
        
        // Initialize memecoin execution engine
        memecoin_engine_ = std::make_unique<MemecoinExecutionEngine>();
        
        // Initialize scanner
        MemecoinScanner::ScannerConfig scanner_config;
        scanner_config.blockchains = {"solana", "ethereum", "bsc"};
        scanner_config.min_liquidity_usd = 5000.0;
        scanner_config.max_market_cap_usd = 1000000.0;
        scanner_config.require_locked_liquidity = true;
        scanner_config.min_holder_count = 20;
        
        scanner_ = std::make_unique<MemecoinScanner>(scanner_config);
        
        // Initialize signal compressor
        SignalCompressor::CompressionConfig compression_config;
        compression_config.default_ttl_ms = 200; // Very short TTL for memecoin trading
        compression_config.enable_compression_stats = true;
        
        signal_compressor_ = std::make_unique<SignalCompressor>(compression_config);
    }
    
    void initialize_platforms() {
        HFX_LOG_INFO("\n📡 Initializing Trading Platforms:");
        
        // Initialize Axiom Pro
        auto axiom = std::make_unique<AxiomProIntegration>("demo_api_key", "https://webhook.example.com");
        if (axiom->connect()) {
            memecoin_engine_->add_platform(TradingPlatform::AXIOM_PRO, std::move(axiom));
            HFX_LOG_INFO("✅ Axiom Pro connected");
        }
        
        // Initialize Photon Sol
        auto photon = std::make_unique<PhotonSolIntegration>("demo_telegram_token", "https://api.mainnet-beta.solana.com");
        if (photon->connect()) {
            photon->set_jito_bundle_settings(10000, true); // Enable Jito with 10k lamport tip
            memecoin_engine_->add_platform(TradingPlatform::PHOTON_SOL, std::move(photon));
            HFX_LOG_INFO("✅ Photon Sol connected with Jito bundles");
        }
        
        // Initialize BullX
        auto bullx = std::make_unique<BullXIntegration>("demo_api_key", "demo_secret");
        if (bullx->connect()) {
            bullx->enable_smart_money_tracking();
            memecoin_engine_->add_platform(TradingPlatform::BULLX, std::move(bullx));
            HFX_LOG_INFO("✅ BullX connected with smart money tracking");
        }
    }
    
    void configure_strategies() {
        HFX_LOG_INFO("\n⚡ Configuring Ultra-Fast Strategies:");
        
        // Enable sniper mode
        memecoin_engine_->enable_sniper_mode(5.0, 300.0); // 5 ETH/SOL max, 300% target profit
        
        // Enable smart money copying
        memecoin_engine_->enable_smart_money_copy(50.0, 100); // Copy 50% within 100ms
        
        // Enable MEV protection
        memecoin_engine_->enable_mev_protection(true);
        
        HFX_LOG_INFO("✅ All strategies configured for maximum speed and profit");
    }
    
    void setup_callbacks() {
        HFX_LOG_INFO("\n🔔 Setting up Real-time Callbacks:");
        
        // New token discovery callback
        memecoin_engine_->register_new_token_callback([this](const MemecoinToken& token) {
            handle_new_token(token);
        });
        
        // Trade completion callback
        memecoin_engine_->register_trade_complete_callback([this](const MemecoinTradeResult& result) {
            handle_trade_complete(result);
        });
        
        // Scanner callback
        scanner_->set_new_token_callback([this](const MemecoinToken& token) {
            handle_scanner_discovery(token);
        });
        
        HFX_LOG_INFO("✅ Real-time callbacks configured");
    }
    
    void start_systems() {
        HFX_LOG_INFO("\n🚀 Starting Ultra-Low Latency Systems:");
        
        // Start execution engine
        if (execution_engine_->initialize()) {
            HFX_LOG_INFO("✅ Execution engine started");
        }
        
        // Start token discovery
        memecoin_engine_->start_token_discovery();
        scanner_->start_scanning();
        HFX_LOG_INFO("✅ Token discovery and scanning started");
        
        HFX_LOG_INFO("\n🎯 SYSTEM LIVE - Ready for memecoin sniping!");
        HFX_LOG_INFO("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
    }
    
    void run_demo() {
        HFX_LOG_INFO("\n🎮 Running Live Demo...");
        
        auto start_time = std::chrono::high_resolution_clock::now();
        
        // Run for 30 seconds
        while (std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::high_resolution_clock::now() - start_time).count() < 30) {
            
            print_live_stats();
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }
        
        print_final_summary();
    }
    
    void shutdown() {
        HFX_LOG_INFO("\n🛑 Shutting down systems...");
        
        scanner_->stop_scanning();
        memecoin_engine_->stop_token_discovery();
        execution_engine_->shutdown();
        
        HFX_LOG_INFO("✅ All systems shut down cleanly");
    }
    
private:
    std::unique_ptr<UltraFastExecutionEngine> execution_engine_;
    std::unique_ptr<MemecoinExecutionEngine> memecoin_engine_;
    std::unique_ptr<MemecoinScanner> scanner_;
    std::unique_ptr<SignalCompressor> signal_compressor_;
    
    std::atomic<uint64_t> tokens_discovered_{0};
    std::atomic<uint64_t> trades_executed_{0};
    std::atomic<uint64_t> successful_snipes_{0};
    
    void handle_new_token(const MemecoinToken& token) {
        tokens_discovered_.fetch_add(1);
        HFX_LOG_INFO("🆕 NEW TOKEN: " << token.symbol << " on " << token.blockchain 
                  << " (Liquidity: $" << static_cast<int>(token.liquidity_usd) << ")" << std::endl;
    }
    
    void handle_trade_complete(const MemecoinTradeResult& result) {
        trades_executed_.fetch_add(1);
        
        if (result.success) {
            successful_snipes_.fetch_add(1);
            HFX_LOG_INFO("✅ TRADE SUCCESS: " << result.transaction_hash 
                      << " (Latency: " << result.execution_latency_ns / 1000 << "μs)" << std::endl;
        } else {
            HFX_LOG_INFO("❌ TRADE FAILED: " << result.error_message << std::endl;
        }
    }
    
    void handle_scanner_discovery(const MemecoinToken& token) {
        // Auto-snipe promising tokens
        if (should_auto_snipe(token)) {
            double snipe_amount = calculate_snipe_amount(token);
            auto result = memecoin_engine_->snipe_new_token(token, snipe_amount);
            
            HFX_LOG_INFO("🎯 AUTO-SNIPE: " << token.symbol 
                      << " (" << snipe_amount << " " << token.blockchain << ")" << std::endl;
        }
    }
    
    bool should_auto_snipe(const MemecoinToken& token) {
        // Simple auto-snipe criteria
        return token.liquidity_usd > 10000 && 
               token.has_locked_liquidity && 
               token.holder_count > 50;
    }
    
    double calculate_snipe_amount(const MemecoinToken& token) {
        // Dynamic position sizing
        if (token.liquidity_usd > 50000) return 2.0;
        if (token.liquidity_usd > 20000) return 1.0;
        return 0.5;
    }
    
    void print_live_stats() {
        MemecoinExecutionEngine::ExecutionMetrics metrics;
        UltraFastExecutionEngine::PerformanceMetrics exec_metrics;
        memecoin_engine_->get_metrics(metrics);
        execution_engine_->get_metrics(exec_metrics);
        
        HFX_LOG_INFO("\n📊 LIVE STATS:");
        HFX_LOG_INFO("   Tokens Discovered: " << tokens_discovered_.load() << std::endl;
        HFX_LOG_INFO("   Total Trades: " << metrics.total_trades.load() << std::endl;
        HFX_LOG_INFO("   Successful Snipes: " << metrics.successful_snipes.load() << std::endl;
        HFX_LOG_INFO("   Avg Execution Latency: " << exec_metrics.avg_latency_ns.load() / 1000 << "μs" << std::endl;
        HFX_LOG_INFO("   Current P&L: $" << static_cast<int>(metrics.total_pnl.load()) << std::endl;
        HFX_LOG_INFO("   MEV Attacks Avoided: " << metrics.mev_attacks_avoided.load() << std::endl;
    }
    
    void print_final_summary() {
        MemecoinExecutionEngine::ExecutionMetrics metrics;
        UltraFastExecutionEngine::PerformanceMetrics exec_metrics;
        memecoin_engine_->get_metrics(metrics);
        execution_engine_->get_metrics(exec_metrics);
        
        HFX_LOG_INFO("\n🏁 FINAL SUMMARY:");
        HFX_LOG_INFO("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
        HFX_LOG_INFO("Total Tokens Discovered: " << tokens_discovered_.load() << std::endl;
        HFX_LOG_INFO("Total Trades Executed: " << metrics.total_trades.load() << std::endl;
        HFX_LOG_INFO("Successful Snipes: " << metrics.successful_snipes.load() << std::endl;
        HFX_LOG_INFO("Success Rate: " << (metrics.total_trades.load() > 0 ? 
            (100.0 * metrics.successful_snipes.load() / metrics.total_trades.load()) : 0) << "%" << std::endl;
        HFX_LOG_INFO("Average Execution Latency: " << exec_metrics.avg_latency_ns.load() / 1000 << "μs" << std::endl;
        HFX_LOG_INFO("Maximum Execution Latency: " << exec_metrics.max_latency_ns.load() / 1000 << "μs" << std::endl;
        HFX_LOG_INFO("Total P&L: $" << static_cast<int>(metrics.total_pnl.load()) << std::endl;
        HFX_LOG_INFO("MEV Attacks Avoided: " << metrics.mev_attacks_avoided.load() << std::endl;
        HFX_LOG_INFO("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
        HFX_LOG_INFO("\n🎉 HydraFlow-X Demo Complete! Fastest memecoin trading system operational.");
    }
};

int main() {
    HFTDemoSystem demo;
        
        demo.initialize_platforms();
        demo.configure_strategies();
        demo.setup_callbacks();
        demo.start_systems();
        
        demo.run_demo();
        
        demo.shutdown();
        
        return 0;
}
