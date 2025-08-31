/**
 * @file memecoin_integrations.hpp
 * @brief Ultra-low latency integrations for Axiom Pro, Photon, BullX platforms
 * @version 1.0.0
 * 
 * Sub-millisecond execution for memecoin sniping and MEV strategies
 * Optimized for Solana and Ethereum memecoin trading
 */

#pragma once

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <atomic>
#include <chrono>
#include <functional>
#include <queue>

#include "execution_engine.hpp"
#include "signal_compressor.hpp"

namespace hfx::hft {

/**
 * @brief Platform-specific integration interfaces
 */
enum class TradingPlatform {
    AXIOM_PRO,      // Axiom Pro memecoin bot
    PHOTON_SOL,     // Photon Solana bot  
    BULLX,          // BullX trading platform
    CUSTOM_RPC      // Direct blockchain RPC
};

/**
 * @brief Memecoin token information
 */
struct MemecoinToken {
    std::string symbol;
    std::string contract_address;
    std::string blockchain; // "solana", "ethereum", "bsc"
    double liquidity_usd;
    uint64_t creation_timestamp;
    bool is_verified;
    bool has_locked_liquidity;
    double holder_count;
    std::string telegram_link;
    std::string twitter_link;
};

/**
 * @brief Ultra-fast trade execution parameters
 */
struct MemecoinTradeParams {
    std::string token_address;
    std::string action; // "BUY", "SELL", "SNIPE"
    double amount_sol_or_eth;
    double slippage_tolerance_percent;
    double max_gas_price;
    uint32_t priority_fee_lamports; // Solana priority fees
    bool use_jito_bundles; // Solana MEV protection
    bool anti_mev_protection;
    uint64_t max_execution_time_ms;
    
    // Platform-specific settings
    std::unordered_map<std::string, std::string> platform_config;
};

/**
 * @brief Real-time memecoin market data
 */
struct MemecoinMarketData {
    std::string token_address;
    double price_usd;
    double price_sol_or_eth;
    double volume_24h;
    double market_cap;
    double liquidity_sol_or_eth;
    int64_t price_change_1m_percent;
    int64_t price_change_5m_percent;
    uint64_t timestamp_ns;
    
    // MEV indicators
    bool sandwich_attack_detected;
    double mev_threat_level; // 0.0 - 1.0
    uint32_t frontrun_attempts;
};

/**
 * @brief Trade execution result
 */
struct MemecoinTradeResult {
    bool success;
    std::string transaction_hash;
    std::string error_message;
    double execution_price;
    double actual_slippage_percent;
    uint64_t execution_latency_ns;
    uint64_t confirmation_time_ms;
    double gas_used;
    double total_cost_including_fees;
    
    // Performance metrics
    bool frontran_detected;
    bool sandwich_detected;
    double mev_loss_percent;
};

/**
 * @brief Platform integration interface
 */
class IPlatformIntegration {
public:
    virtual ~IPlatformIntegration() = default;
    
    // Connection management
    virtual bool connect() = 0;
    virtual void disconnect() = 0;
    virtual bool is_connected() const = 0;
    
    // Market data
    virtual void subscribe_to_new_tokens() = 0;
    virtual void subscribe_to_price_updates(const std::string& token_address) = 0;
    virtual MemecoinMarketData get_market_data(const std::string& token_address) = 0;
    
    // Trading
    virtual MemecoinTradeResult execute_trade(const MemecoinTradeParams& params) = 0;
    virtual bool cancel_pending_orders(const std::string& token_address) = 0;
    
    // Portfolio
    virtual double get_balance() = 0;
    virtual std::vector<std::string> get_holdings() = 0;
    
    // Platform-specific features
    virtual std::unordered_map<std::string, std::string> get_platform_capabilities() = 0;
};

/**
 * @brief Axiom Pro integration
 */
class AxiomProIntegration : public IPlatformIntegration {
public:
    explicit AxiomProIntegration(const std::string& api_key, const std::string& webhook_url);
    ~AxiomProIntegration() override;
    
    bool connect() override;
    void disconnect() override;
    bool is_connected() const override;
    
    void subscribe_to_new_tokens() override;
    void subscribe_to_price_updates(const std::string& token_address) override;
    MemecoinMarketData get_market_data(const std::string& token_address) override;
    
    MemecoinTradeResult execute_trade(const MemecoinTradeParams& params) override;
    bool cancel_pending_orders(const std::string& token_address) override;
    
    double get_balance() override;
    std::vector<std::string> get_holdings() override;
    
    std::unordered_map<std::string, std::string> get_platform_capabilities() override;
    
private:
    class AxiomImpl;
    std::unique_ptr<AxiomImpl> pimpl_;
};

/**
 * @brief Photon Sol integration
 */
class PhotonSolIntegration : public IPlatformIntegration {
public:
    explicit PhotonSolIntegration(const std::string& telegram_bot_token, 
                                 const std::string& rpc_endpoint);
    ~PhotonSolIntegration() override;
    
    bool connect() override;
    void disconnect() override;
    bool is_connected() const override;
    
    void subscribe_to_new_tokens() override;
    void subscribe_to_price_updates(const std::string& token_address) override;
    MemecoinMarketData get_market_data(const std::string& token_address) override;
    
    MemecoinTradeResult execute_trade(const MemecoinTradeParams& params) override;
    bool cancel_pending_orders(const std::string& token_address) override;
    
    double get_balance() override;
    std::vector<std::string> get_holdings() override;
    
    std::unordered_map<std::string, std::string> get_platform_capabilities() override;
    
    // Solana-specific features
    void set_jito_bundle_settings(double tip_lamports, bool use_priority_fees);
    std::vector<std::string> get_trending_solana_tokens();
    
private:
    class PhotonImpl;
    std::unique_ptr<PhotonImpl> pimpl_;
};

/**
 * @brief BullX integration
 */
class BullXIntegration : public IPlatformIntegration {
public:
    explicit BullXIntegration(const std::string& api_key, const std::string& secret);
    ~BullXIntegration() override;
    
    bool connect() override;
    void disconnect() override;
    bool is_connected() const override;
    
    void subscribe_to_new_tokens() override;
    void subscribe_to_price_updates(const std::string& token_address) override;
    MemecoinMarketData get_market_data(const std::string& token_address) override;
    
    MemecoinTradeResult execute_trade(const MemecoinTradeParams& params) override;
    bool cancel_pending_orders(const std::string& token_address) override;
    
    double get_balance() override;
    std::vector<std::string> get_holdings() override;
    
    std::unordered_map<std::string, std::string> get_platform_capabilities() override;
    
    // BullX-specific features
    void enable_smart_money_tracking();
    std::vector<MemecoinToken> get_smart_money_buys();
    
private:
    class BullXImpl;
    std::unique_ptr<BullXImpl> pimpl_;
};

/**
 * @brief Ultra-low latency memecoin execution engine
 */
class MemecoinExecutionEngine {
public:
    explicit MemecoinExecutionEngine();
    ~MemecoinExecutionEngine();
    
    // Platform management
    void add_platform(TradingPlatform platform, std::unique_ptr<IPlatformIntegration> integration);
    void remove_platform(TradingPlatform platform);
    
    // Token discovery and monitoring
    void start_token_discovery();
    void stop_token_discovery();
    void add_token_watch(const std::string& token_address);
    void remove_token_watch(const std::string& token_address);
    
    // Trading strategies
    void enable_sniper_mode(double max_buy_amount, double target_profit_percent);
    void enable_smart_money_copy(double copy_percentage, uint64_t max_delay_ms);
    void enable_mev_protection(bool use_private_mempools);
    
    // Execution
    MemecoinTradeResult execute_cross_platform_trade(const MemecoinTradeParams& params);
    MemecoinTradeResult snipe_new_token(const MemecoinToken& token, double buy_amount);
    
    // Portfolio management
    std::unordered_map<std::string, double> get_consolidated_portfolio();
    void rebalance_across_platforms();
    
    // Performance monitoring
    struct ExecutionMetrics {
        std::atomic<uint64_t> total_trades{0};
        std::atomic<uint64_t> successful_snipes{0};
        std::atomic<uint64_t> avg_execution_latency_ns{0};
        std::atomic<double> total_pnl{0.0};
        std::atomic<uint64_t> mev_attacks_avoided{0};
    };
    
    void get_metrics(ExecutionMetrics& metrics_out) const;
    
    // Callbacks
    using NewTokenCallback = std::function<void(const MemecoinToken&)>;
    using PriceUpdateCallback = std::function<void(const MemecoinMarketData&)>;
    using TradeCompleteCallback = std::function<void(const MemecoinTradeResult&)>;
    
    void register_new_token_callback(NewTokenCallback callback);
    void register_price_update_callback(PriceUpdateCallback callback);
    void register_trade_complete_callback(TradeCompleteCallback callback);
    
private:
    class ExecutionImpl;
    std::unique_ptr<ExecutionImpl> pimpl_;
};

// Forward declaration for MEV protection (implementation in mev_strategy.hpp)
class MEVProtectionEngine;

/**
 * @brief Real-time token scanner for new memecoin launches
 */
class MemecoinScanner {
public:
    struct ScannerConfig {
        std::vector<std::string> blockchains{"solana", "ethereum", "bsc"};
        double min_liquidity_usd = 1000.0;
        double max_market_cap_usd = 10000000.0;
        bool require_locked_liquidity = true;
        bool require_verified_contract = false;
        uint32_t min_holder_count = 10;
        std::vector<std::string> blacklisted_creators;
    };
    
    explicit MemecoinScanner(const ScannerConfig& config);
    ~MemecoinScanner();
    
    void start_scanning();
    void stop_scanning();
    
    void set_new_token_callback(std::function<void(const MemecoinToken&)> callback);
    
    // Advanced filtering
    void add_smart_money_filter(const std::vector<std::string>& smart_wallets);
    void add_insider_detection();
    void add_rug_pull_prevention();
    
private:
    class ScannerImpl;
    std::unique_ptr<ScannerImpl> pimpl_;
};

} // namespace hfx::hft
