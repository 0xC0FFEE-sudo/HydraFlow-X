/**
 * @file production_main.cpp
 * @brief Production HydraFlow-X trading engine main application
 */

#include "trading-engine/include/production_trader.hpp"
#include "exchanges/include/coinbase_exchange.hpp"
#include "hfx-log/include/simple_logger.hpp"

#include <iostream>
#include <memory>
#include <signal.h>
#include <thread>
#include <chrono>
#include <unistd.h>
#include <fstream>

// Global variables for signal handling
std::unique_ptr<hfx::trading::ProductionTrader> g_trader;
std::unique_ptr<hfx::exchanges::CoinbaseExchange> g_exchange;
std::atomic<bool> g_shutdown_requested{false};

void signal_handler(int signal) {
    std::cout << "\n🛑 Shutdown signal received (" << signal << "), stopping trading..." << std::endl;
    g_shutdown_requested.store(true);
    
    if (g_trader) {
        g_trader->stop_trading();
    }
    
    if (g_exchange) {
        g_exchange->disconnect();
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    std::cout << "✅ Clean shutdown completed." << std::endl;
    exit(0);
}

void print_banner() {
    std::cout << R"(
    ╭─────────────────────────────────────────────────────╮
    │                                                     │
    │         🏛️  HydraFlow-X Production Engine           │
    │                                                     │
    │    Ultra-Low Latency DeFi Trading Infrastructure    │
    │                                                     │
    │    🚀 Microsecond Execution • 🔮 AI-Powered        │
    │    ⛓️  Multi-Chain • 🛡️  MEV Protected              │
    │                                                     │
    ╰─────────────────────────────────────────────────────╯
    )" << std::endl;
}

void print_performance_dashboard(const hfx::trading::ProductionTrader::PerformanceMetrics& metrics) {
    std::cout << "\n📊 ═══ LIVE PERFORMANCE DASHBOARD ═══\n\n";
    
    std::cout << "⚡ EXECUTION METRICS:\n";
    std::cout << "   Total Orders:      " << metrics.total_orders << "\n";
    std::cout << "   Filled Orders:     " << metrics.filled_orders << " (" 
              << (metrics.total_orders > 0 ? (metrics.filled_orders * 100.0 / metrics.total_orders) : 0) 
              << "%)\n";
    std::cout << "   Rejected Orders:   " << metrics.rejected_orders << "\n\n";
    
    std::cout << "💰 P&L METRICS:\n";
    std::cout << "   Total P&L:         $" << std::fixed << std::setprecision(2) << metrics.total_pnl << "\n";
    std::cout << "   Sharpe Ratio:      " << std::fixed << std::setprecision(4) << metrics.sharpe_ratio << "\n";
    std::cout << "   Max Drawdown:      " << std::fixed << std::setprecision(2) << metrics.max_drawdown << "%\n\n";
    
    std::cout << "⚡ LATENCY METRICS:\n";
    std::cout << "   Avg Latency:       " << metrics.avg_latency_ns / 1000 << " μs\n";
    std::cout << "   Max Latency:       " << metrics.max_latency_ns / 1000 << " μs\n\n";
    
    // Real-time trading indicators
    double fill_rate = metrics.total_orders > 0 ? (metrics.filled_orders * 100.0 / metrics.total_orders) : 0;
    std::string status_color = fill_rate > 80 ? "🟢" : fill_rate > 60 ? "🟡" : "🔴";
    
    std::cout << "🎯 SYSTEM STATUS: " << status_color << " ";
    if (fill_rate > 80) {
        std::cout << "OPTIMAL PERFORMANCE\n";
    } else if (fill_rate > 60) {
        std::cout << "GOOD PERFORMANCE\n"; 
    } else {
        std::cout << "DEGRADED PERFORMANCE\n";
    }
    
    std::cout << "\n" << std::string(55, '=') << "\n" << std::endl;
}

void print_positions(const std::vector<hfx::trading::Position>& positions) {
    if (positions.empty()) {
        std::cout << "📝 No active positions\n\n";
        return;
    }
    
    std::cout << "📋 ACTIVE POSITIONS:\n";
    std::cout << "   Symbol        Qty         Avg Price    Unrealized P&L\n";
    std::cout << "   ─────────────────────────────────────────────────────\n";
    
    for (const auto& pos : positions) {
        if (pos.quantity != 0.0) {
            std::cout << "   " << std::left << std::setw(12) << pos.symbol
                      << " " << std::right << std::setw(10) << std::fixed << std::setprecision(4) << pos.quantity
                      << " " << std::setw(12) << std::fixed << std::setprecision(2) << pos.avg_price
                      << " " << std::setw(14) << std::fixed << std::setprecision(2) << pos.unrealized_pnl << "\n";
        }
    }
    std::cout << "\n";
}

class TradingConfig {
public:
    std::string coinbase_api_key;
    std::string coinbase_api_secret;
    std::string coinbase_passphrase;
    bool sandbox_mode = true;
    std::vector<std::string> trading_pairs = {"BTC-USD", "ETH-USD"};
    double max_position_size = 0.1; // BTC
    double stop_loss_pct = 0.02;    // 2%
    double take_profit_pct = 0.04;  // 4%
    
    bool load_from_file(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cout << "⚠️  Config file '" << filename << "' not found, using defaults\n";
            return false;
        }
        
        // Simple config loading (in production, use proper JSON/YAML parser)
        std::string line;
        while (std::getline(file, line)) {
            if (line.find("coinbase_api_key=") == 0) {
                coinbase_api_key = line.substr(17);
            }
            // Add other config parsing as needed
        }
        
        return true;
    }
};

int main(int /*argc*/, char* /*argv*/[]) {
    print_banner();
    
    // Install signal handlers for clean shutdown
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    std::cout << "🔧 Initializing HydraFlow-X Production Engine...\n";
    
    // Load configuration
    TradingConfig config;
    config.load_from_file("config/trading.conf");
    
    if (config.coinbase_api_key.empty()) {
        std::cout << "⚠️  No API credentials configured, running in demo mode\n";
        config.sandbox_mode = true;
    }
    
    try {
        // Initialize exchange connection
        hfx::exchanges::CoinbaseConfig exchange_config;
        exchange_config.api_key = config.coinbase_api_key;
        exchange_config.api_secret = config.coinbase_api_secret;
        exchange_config.passphrase = config.coinbase_passphrase;
        exchange_config.sandbox_mode = config.sandbox_mode;
        
        g_exchange = std::make_unique<hfx::exchanges::CoinbaseExchange>(exchange_config);
        
        // Initialize trading engine
        g_trader = hfx::trading::TradingStrategyFactory::create_trader(
            hfx::trading::TradingStrategyFactory::StrategyType::MARKET_MAKING
        );
        
        if (!g_trader->initialize()) {
            std::cerr << "❌ Failed to initialize trading engine" << std::endl;
            return 1;
        }
        
        // Connect to exchange
        if (!g_exchange->connect()) {
            std::cerr << "❌ Failed to connect to Coinbase Pro" << std::endl;
            return 1;
        }
        
        std::cout << "✅ Exchange connection established\n";
        
        // Set up risk management
        for (const auto& pair : config.trading_pairs) {
            g_trader->set_max_position(pair, config.max_position_size);
        }
        g_trader->set_stop_loss(config.stop_loss_pct);
        g_trader->set_take_profit(config.take_profit_pct);
        
        std::cout << "✅ Risk management configured\n";
        
        // Subscribe to market data
        for (const auto& pair : config.trading_pairs) {
            g_exchange->subscribe_ticker(pair, [&](const hfx::exchanges::Ticker& ticker) {
                // Convert to trading engine format
                hfx::trading::MarketData market_data;
                market_data.symbol = ticker.symbol;
                market_data.bid_price = ticker.bid;
                market_data.ask_price = ticker.ask;
                market_data.volume = ticker.volume;
                market_data.timestamp_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
                    ticker.time.time_since_epoch()
                ).count();
                
                g_trader->on_market_data(market_data);
            });
        }
        
        std::cout << "✅ Market data subscriptions active\n";
        
        // Start trading engine
        g_trader->start_trading();
        std::cout << "✅ Trading engine started\n\n";
        
        std::cout << "🚀 HydraFlow-X is now LIVE and ready for trading!\n";
        std::cout << "    Press Ctrl+C for clean shutdown\n\n";
        
        // Main monitoring loop
        auto last_update = std::chrono::steady_clock::now();
        auto dashboard_interval = std::chrono::seconds(5);
        
        while (!g_shutdown_requested.load()) {
            auto now = std::chrono::steady_clock::now();
            
            if (now - last_update >= dashboard_interval) {
                // Clear screen and show live dashboard
                system("clear");
                print_banner();
                
                auto metrics = g_trader->get_metrics();
                print_performance_dashboard(metrics);
                
                auto positions = g_trader->get_positions();
                print_positions(positions);
                
                // Show current time
                auto time_t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
                std::cout << "🕐 Last Update: " << std::ctime(&time_t);
                
                last_update = now;
            }
            
            // Check for keyboard input (simple implementation)
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Fatal error: " << e.what() << std::endl;
        return 1;
    }
    
    std::cout << "👋 HydraFlow-X production engine stopped." << std::endl;
    return 0;
}
