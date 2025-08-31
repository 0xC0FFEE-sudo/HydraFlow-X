#pragma once

#include "ultra_fast_mempool.hpp"
#include "mev_shield.hpp"
#include "v3_tick_engine.hpp"
#include "jito_mev_engine.hpp"

#include <memory>
#include <string>
#include <atomic>
#include <chrono>
#include <functional>

namespace hfx::ultra {

// Advanced trading modes for maximum flexibility
enum class TradingMode {
    STANDARD_BUY,         // Standard single-wallet trading
    MULTI_WALLET,         // Distribute trades across multiple wallets
    AUTO_TRADING,         // Automated trading with advanced filters
    SNIPER_MODE,          // Ultra-fast token launch sniping
    COPY_TRADING,         // Mirror successful traders
    AUTONOMOUS_MODE       // Fully autonomous AI-driven trading
};

// Trade execution status tracking
enum class TradeStatus {
    PENDING,              // Trade is queued for execution
    EXECUTING,            // Trade is currently being executed
    COMPLETED,            // Trade completed successfully
    FAILED,               // Trade failed to execute
    CANCELLED             // Trade was cancelled before execution
};

// Trading strategy configuration
struct TradingStrategy {
    std::string name;
    TradingMode mode;
    std::string target_token;
    uint64_t amount;
    double slippage_bps;
};

// Comprehensive smart trading configuration
struct SmartTradingConfig {
    // Core trading parameters
    TradingMode default_mode = TradingMode::STANDARD_BUY;
    double default_slippage_bps = 50.0;  // 0.5% default slippage
    uint64_t max_gas_price = 50000000000ULL; // 50 gwei max
    
    // Multi-wallet configuration for advanced strategies
    size_t max_wallets = 10;
    bool enable_wallet_rotation = true;
    std::string primary_wallet_address;
    size_t worker_threads = 4;
    
    // Token launch sniping configuration
    struct SnipingConfig {
        bool enable_pump_fun_sniping = true;
        bool enable_raydium_sniping = true;
        bool enable_dev_wallet_monitoring = true;
        bool enable_mint_address_tracking = true;
        bool auto_sell_on_bonding_curve = true;
        
        // Market cap filters for safety
        uint64_t min_market_cap = 80000; // $80k minimum
        uint64_t max_market_cap = 1000000; // $1M maximum
        double max_snipe_slippage_bps = 800.0; // 8% for aggressive sniping
    } sniping_config;
    
    // Copy trading configuration
    struct CopyTradingConfig {
        std::vector<std::string> watched_wallets;
        double copy_percentage = 100.0; // Full position mirroring
        uint64_t min_copy_amount = 1000000; // 0.001 SOL minimum
        uint64_t max_copy_amount = 1000000000; // 1 SOL maximum
        bool enable_stop_loss = true;
        double stop_loss_percentage = -50.0; // -50% stop loss
    } copy_trading_config;
    
    // Autonomous trading mode configuration
    struct AutonomousConfig {
        bool enable_auto_buy = true;
        bool enable_auto_sell = true;
        std::vector<std::string> token_filters;
        uint64_t max_position_size = 100000000; // 0.1 SOL max
        double profit_target_percentage = 200.0; // 2x profit target
        double loss_limit_percentage = -30.0; // -30% loss limit
    } autonomous_config;
};

// Ultra-high performance metrics tracking
struct PerformanceMetrics {
    std::atomic<uint64_t> total_trades{0};
    std::atomic<uint64_t> successful_trades{0};
    std::atomic<uint64_t> failed_trades{0};
    
    // Latency metrics for ultra-fast execution
    std::atomic<double> avg_confirmation_time_ms{0.0};
    std::atomic<double> avg_decision_latency_ms{0.0};
    std::atomic<uint64_t> fastest_trade_ms{UINT64_MAX};
    
    // Sniping performance analytics
    std::atomic<uint64_t> snipe_attempts{0};
    std::atomic<uint64_t> snipe_successes{0};
    std::atomic<double> snipe_success_rate{0.0};
    
    // MEV protection effectiveness
    std::atomic<uint64_t> mev_attacks_blocked{0};
    std::atomic<uint64_t> sandwich_attempts_detected{0};
    std::atomic<uint64_t> frontrun_attempts_blocked{0};
    
    // Economic performance metrics
    std::atomic<uint64_t> total_volume_traded{0};
    std::atomic<int64_t> total_pnl{0}; // Can be negative
    std::atomic<uint64_t> gas_fees_paid{0};
    std::atomic<uint64_t> mev_protection_cost{0};
};

// Main smart trading engine - orchestrates all ultra-fast components
class SmartTradingEngine {
public:
    using TradeCallback = std::function<void(const std::string& token, uint64_t amount, bool success)>;
    using SnipeCallback = std::function<void(const std::string& token, uint64_t profit_loss)>;
    
    explicit SmartTradingEngine(const SmartTradingConfig& config);
    ~SmartTradingEngine();
    
    // Core engine lifecycle
    bool start();
    bool stop();
    bool is_running() const { return running_.load(); }
    
    // Trading operations with comprehensive result tracking
    struct TradeResult {
        std::string transaction_hash;
        bool successful;
        uint64_t actual_amount_out;
        uint64_t gas_used;
        std::chrono::nanoseconds execution_time;
        MEVProtectionLevel mev_protection_used;
        std::string trade_id;
        bool success; // Alias for successful for compatibility
    };
    
    TradeResult buy_token(const std::string& token_address,
                         uint64_t amount_in,
                         TradingMode mode = TradingMode::STANDARD_BUY);
    
    TradeResult sell_token(const std::string& token_address,
                          uint64_t amount_to_sell,
                          TradingMode mode = TradingMode::STANDARD_BUY);
    
    // Advanced sniping operations
    bool start_sniper(const std::string& target_token_or_pool);
    void stop_sniper(const std::string& target);
    std::vector<std::string> get_active_snipers() const;
    
    // Strategy execution system
    bool execute_trading_strategy(const TradingStrategy& strategy);
    std::vector<TradeResult> get_active_trades() const;
    
    // Copy trading functionality
    bool add_copy_wallet(const std::string& wallet_address, double copy_percentage = 100.0);
    bool remove_copy_wallet(const std::string& wallet_address);
    std::vector<std::string> get_copy_wallets() const;
    
    // Autonomous mode (hands-off AI trading)
    bool enable_autonomous_mode(bool enable);
    bool is_autonomous_mode_active() const { return autonomous_mode_active_.load(); }
    
    // Multi-wallet management for advanced strategies
    struct WalletInfo {
        std::string address;
        uint64_t balance_sol;
        uint32_t active_trades;
        bool is_primary;
    };
    
    std::vector<WalletInfo> get_wallet_info() const;
    bool set_primary_wallet(const std::string& wallet_address);
    bool add_wallet(const std::string& private_key); // Securely managed
    
    // Performance analytics and monitoring
    const PerformanceMetrics& get_performance_metrics() const { return metrics_; }
    void reset_metrics();
    
    // Dynamic configuration updates
    void update_slippage(double new_slippage_bps);
    void update_gas_settings(uint64_t max_gas_price, uint64_t priority_fee);
    void update_snipe_filters(uint64_t min_mcap, uint64_t max_mcap);
    
    // Advanced AI-powered features
    void enable_ai_routing(bool enable) { ai_routing_enabled_ = enable; }
    void enable_predictive_sniping(bool enable) { predictive_sniping_ = enable; }
    void enable_cross_dex_arbitrage(bool enable) { cross_dex_arbitrage_ = enable; }
    
    // Event callback registration
    void register_trade_callback(TradeCallback callback);
    void register_snipe_callback(SnipeCallback callback);
    
private:
    SmartTradingConfig config_;
    std::atomic<bool> running_{false};
    std::atomic<bool> autonomous_mode_active_{false};
    PerformanceMetrics metrics_;
    
    // Core ultra-fast engine components
    std::unique_ptr<UltraFastMempoolMonitor> mempool_monitor_;
    std::unique_ptr<MEVShield> mev_shield_;
    std::unique_ptr<V3TickEngine> tick_engine_;
    std::unique_ptr<JitoMEVEngine> jito_engine_; // For Solana optimization
    
    // Trading state management
    struct ActiveTrade {
        std::string token_address;
        uint64_t amount;
        TradingMode mode;
        std::chrono::steady_clock::time_point started_at;
        std::atomic<bool> completed{false};
        std::string trade_id;
        std::string strategy;
        TradeStatus status{TradeStatus::PENDING};
        
        // Custom copy constructor and assignment operator for atomic handling
        ActiveTrade() = default;
        ActiveTrade(const ActiveTrade& other) 
            : token_address(other.token_address), amount(other.amount), 
              mode(other.mode), started_at(other.started_at), 
              trade_id(other.trade_id), strategy(other.strategy), 
              status(other.status), completed(other.completed.load()) {}
        ActiveTrade& operator=(const ActiveTrade& other) {
            if (this != &other) {
                token_address = other.token_address;
                amount = other.amount;
                mode = other.mode;
                started_at = other.started_at;
                trade_id = other.trade_id;
                strategy = other.strategy;
                status = other.status;
                completed.store(other.completed.load());
            }
            return *this;
        }
    };
    
    std::unordered_map<std::string, ActiveTrade> active_trades_;
    mutable std::mutex trades_mutex_;
    
    // Wallet management system
    std::vector<WalletInfo> managed_wallets_;
    std::string primary_wallet_address_;
    std::unordered_map<std::string, std::string> wallet_private_keys_; // address -> encrypted_private_key
    mutable std::mutex wallets_mutex_;
    
    // Sniping state tracking
    std::unordered_map<std::string, bool> active_snipers_;
    mutable std::mutex snipers_mutex_;
    
    // Copy trading state management
    std::unordered_map<std::string, double> copy_wallets_; // address -> copy percentage
    mutable std::mutex copy_mutex_;
    
    // High-performance threading system
    std::vector<std::thread> worker_threads_;
    std::thread sniper_thread_;
    std::thread copy_trader_thread_;
    std::thread autonomous_monitor_thread_;
    
    // Advanced AI features
    std::atomic<bool> ai_routing_enabled_{true};
    std::atomic<bool> predictive_sniping_{true};
    std::atomic<bool> cross_dex_arbitrage_{true};
    
    // Event callback system
    std::vector<TradeCallback> trade_callbacks_;
    std::vector<SnipeCallback> snipe_callbacks_;
    mutable std::mutex callback_mutex_;
    
    // Internal worker methods
    void worker_thread(size_t thread_id);
    void sniper_worker();
    void copy_trader_worker();
    void autonomous_monitor_worker();
    void performance_monitor_worker();
    
    TradeResult execute_trade_internal(const std::string& token_address,
                                     uint64_t amount,
                                     bool is_buy,
                                     TradingMode mode);
    
    bool should_execute_copy_trade(const std::string& source_wallet,
                                  const std::string& token_address,
                                  uint64_t amount) const;
    
    void update_performance_metrics(const TradeResult& result);
    std::string select_optimal_wallet_for_trade(uint64_t required_amount) const;
    
    // Wallet management helpers
    std::string derive_wallet_address(const std::string& private_key) const;
    std::string encrypt_private_key(const std::string& private_key) const;
    std::string decrypt_private_key(const std::string& encrypted_key) const;
    uint64_t fetch_wallet_balance(const std::string& wallet_address) const;
    bool send_transaction(const std::string& from_wallet, const std::string& transaction_data) const;
    
    // AI-powered route optimization
    std::vector<std::string> find_optimal_route(const std::string& token_in,
                                               const std::string& token_out,
                                               uint64_t amount_in) const;
    
    // MEV protection strategy selection
    MEVProtectionLevel determine_protection_level(const std::string& token_address,
                                                 uint64_t trade_value) const;
    
    // Performance optimization algorithms
    void optimize_gas_strategy(uint64_t& gas_price, uint64_t& priority_fee) const;
    void calculate_optimal_timing(std::chrono::nanoseconds& delay) const;
};

// Factory for creating optimized trading engines with different presets
class SmartTradingEngineFactory {
public:
    // Preset configurations for different trading styles
    static std::unique_ptr<SmartTradingEngine> create_sniper_engine();
    static std::unique_ptr<SmartTradingEngine> create_copy_trader_engine();
    static std::unique_ptr<SmartTradingEngine> create_autonomous_engine();
    static std::unique_ptr<SmartTradingEngine> create_arbitrage_engine();
    static std::unique_ptr<SmartTradingEngine> create_balanced_engine();
    
    // Create engine with custom configuration
    static std::unique_ptr<SmartTradingEngine> create_custom_engine(const SmartTradingConfig& config);
    
    // Utility methods for configuration optimization
    static SmartTradingConfig get_optimal_config_for_chain(const std::string& chain_name);
    static SmartTradingConfig get_high_performance_config();
    static SmartTradingConfig get_conservative_config();
};

} // namespace hfx::ultra