#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <atomic>
#include <mutex>
#include <functional>
#include <chrono>
#include <optional>
#include <queue>
#include <thread>

namespace hfx {
namespace mempool {

// Forward declarations and basic structures
struct Transaction {
    std::string hash;
    std::string from;
    std::string to;
    uint64_t value;
    uint64_t gas_limit;
    uint64_t gas_price;
    std::string data;
    uint32_t block_number;
    std::chrono::system_clock::time_point timestamp;

    Transaction() : value(0), gas_limit(0), gas_price(0), block_number(0) {}
};

struct ParsedIntent {
    std::string intent_type;
    std::string token_in;
    std::string token_out;
    uint64_t amount_in;
    uint64_t amount_out_min;
    std::string recipient;
    uint64_t deadline;

    ParsedIntent() : amount_in(0), amount_out_min(0), deadline(0) {}
};

struct GasEstimate {
    uint64_t safe_gas_price;
    uint64_t proposed_gas_price;
    uint64_t fast_gas_price;
    uint64_t instant_gas_price;
    uint32_t block_time_seconds;

    GasEstimate() : safe_gas_price(0), proposed_gas_price(0), fast_gas_price(0), instant_gas_price(0), block_time_seconds(0) {}
};

// Private transaction submission structure
struct PrivateTransaction {
    std::string tx_hash;
    std::string raw_transaction;
    std::string target_blockchain; // "ethereum" or "solana"
    std::string private_relay; // "flashbots", "eden", "bloxroute"
    uint64_t max_priority_fee_per_gas;
    uint64_t max_fee_per_gas;
    uint64_t gas_limit;
    std::chrono::system_clock::time_point submission_time;
    std::string status; // "pending", "submitted", "confirmed", "failed"
    std::optional<std::string> block_hash;
    std::optional<uint32_t> block_number;

    PrivateTransaction() : max_priority_fee_per_gas(0), max_fee_per_gas(0), gas_limit(0) {}
};

// MEV Protection Configuration
struct MEVProtectionConfig {
    bool enable_private_transactions = true;
    bool enable_sandwich_protection = true;
    bool enable_frontrun_protection = true;
    bool enable_gas_optimization = true;

    // Private relay preferences
    std::vector<std::string> preferred_relays = {"flashbots", "eden", "bloxroute"};

    // Protection thresholds
    double min_tx_value_eth = 0.1; // Minimum transaction value to protect
    double max_gas_price_gwei = 500.0; // Maximum gas price for protection
    uint32_t protection_window_blocks = 5; // Blocks to monitor for attacks

    // Risk thresholds
    double max_sandwich_risk = 0.3; // Maximum acceptable sandwich risk
    double max_frontrun_risk = 0.2; // Maximum acceptable frontrun risk

    // Bundle settings for Solana
    bool enable_jito_bundles = true;
    uint32_t max_bundle_size = 5; // Maximum transactions per bundle
    uint64_t bundle_tip_lamports = 1000000; // Tip for Jito bundle
};

// MEV opportunity types
enum class MEVType {
    UNKNOWN = 0,
    ARBITRAGE,              // Price differences between DEXs
    SANDWICH_ATTACK,        // Front/back-run victim transaction
    FRONTRUNNING,          // Get ahead of profitable transaction
    BACKRUNNING,           // Execute after state-changing transaction
    LIQUIDATION,           // Liquidate undercollateralized positions
    JIT_LIQUIDITY,         // Just-in-time liquidity provision
    ATOMIC_ARBITRAGE,      // Flash loan arbitrage
    MEV_SANDWICH,          // Multi-step sandwich
    CROSS_CHAIN_ARBITRAGE, // Arbitrage across chains
    STATISTICAL_ARBITRAGE, // Statistical price movements
    ORACLE_FRONT_RUNNING,  // Front-run oracle updates
    GOVERNANCE_ATTACK,     // Manipulate governance votes
    CUSTOM
};

// MEV detection confidence levels
enum class MEVConfidence {
    VERY_LOW = 0,    // 0-20%
    LOW,             // 20-40%
    MEDIUM,          // 40-60%
    HIGH,            // 60-80%
    VERY_HIGH,       // 80-90%
    CERTAIN          // 90-100%
};

// MEV opportunity structure
struct MEVOpportunity {
    MEVType type;
    MEVConfidence confidence;
    double confidence_score;
    
    // Basic information
    std::string opportunity_id;
    std::string description;
    std::vector<std::string> involved_transactions;
    std::string victim_transaction;
    
    // Profit information
    double estimated_profit_eth;
    double estimated_profit_usd;
    double max_extractable_value;
    double risk_adjusted_profit;
    
    // Execution details
    uint64_t required_gas;
    uint64_t optimal_gas_price;
    uint32_t execution_deadline_blocks;
    std::chrono::system_clock::time_point deadline;
    
    // DEX/Protocol specific
    std::string protocol;
    std::string pool_address;
    std::string token_a;
    std::string token_b;
    uint64_t amount_in;
    uint64_t amount_out;
    double price_impact;
    double slippage;
    
    // Sandwich attack specific
    Transaction frontrun_tx;
    Transaction backrun_tx;
    double sandwich_profit;
    uint64_t victim_slippage_bps;
    
    // Arbitrage specific
    std::vector<std::string> arbitrage_path;
    std::vector<std::string> dex_sequence;
    double price_difference;
    double arbitrage_ratio;
    
    // Competition and timing
    uint32_t competing_bots;
    uint32_t priority_level;
    bool requires_flash_loan;
    uint64_t flash_loan_amount;
    double flash_loan_fee;
    
    // Risk assessment
    double execution_risk;
    double market_risk;
    double competition_risk;
    double gas_risk;
    double overall_risk_score;
    
    // Detection metadata
    std::chrono::system_clock::time_point detected_at;
    std::chrono::system_clock::time_point expires_at;
    uint32_t block_number;
    std::string detection_method;
    
    MEVOpportunity() : type(MEVType::UNKNOWN), confidence(MEVConfidence::VERY_LOW),
                      confidence_score(0.0), estimated_profit_eth(0.0), estimated_profit_usd(0.0),
                      max_extractable_value(0.0), risk_adjusted_profit(0.0), required_gas(0),
                      optimal_gas_price(0), execution_deadline_blocks(0), amount_in(0),
                      amount_out(0), price_impact(0.0), slippage(0.0), sandwich_profit(0.0),
                      victim_slippage_bps(0), price_difference(0.0), arbitrage_ratio(0.0),
                      competing_bots(0), priority_level(0), requires_flash_loan(false),
                      flash_loan_amount(0), flash_loan_fee(0.0), execution_risk(0.0),
                      market_risk(0.0), competition_risk(0.0), gas_risk(0.0),
                      overall_risk_score(0.0), block_number(0) {}
};

// DEX pool information for arbitrage detection
struct PoolInfo {
    std::string address;
    std::string dex_name;
    std::string token_a;
    std::string token_b;
    uint64_t reserve_a;
    uint64_t reserve_b;
    double price;
    uint64_t liquidity;
    uint32_t fee_bps;
    std::chrono::system_clock::time_point last_updated;
    
    PoolInfo() : reserve_a(0), reserve_b(0), price(0.0), liquidity(0), fee_bps(0) {}
};

// Price oracle information
struct PriceInfo {
    std::string token;
    double price_usd;
    std::vector<double> dex_prices;
    std::vector<std::string> dex_names;
    double price_volatility;
    std::chrono::system_clock::time_point timestamp;
    
    PriceInfo() : price_usd(0.0), price_volatility(0.0) {}
};

// MEV detector configuration
struct MEVDetectorConfig {
    // Detection settings
    bool detect_arbitrage = true;
    bool detect_sandwich = true;
    bool detect_frontrunning = true;
    bool detect_liquidations = true;
    bool detect_jit_liquidity = true;
    
    // Thresholds
    double min_profit_usd = 1.0;
    double min_confidence = 0.5;
    double max_gas_cost_ratio = 0.8;  // Max gas cost as % of profit
    double min_arbitrage_ratio = 1.001; // Min 0.1% profit
    
    // Timing settings
    uint32_t max_opportunity_age_seconds = 300;
    uint32_t sandwich_window_blocks = 5;
    uint32_t arbitrage_window_blocks = 3;
    
    // Data sources
    std::vector<std::string> dex_addresses;
    std::vector<std::string> price_oracles;
    std::vector<uint32_t> monitored_chains;
    bool use_flashloan_providers = true;
    
    // Performance settings
    uint32_t max_concurrent_detections = 8;
    uint32_t detection_timeout_ms = 500;
    bool enable_aggressive_detection = false;
    uint32_t cache_size = 10000;
    uint32_t cache_ttl_seconds = 60;
    
    // Risk management
    double max_position_size_eth = 100.0;
    double max_gas_price_gwei = 500.0;
    bool enable_risk_assessment = true;
    double min_success_probability = 0.8;
};

// MEV detector statistics
struct MEVDetectorStats {
    std::atomic<uint64_t> total_transactions_analyzed{0};
    std::atomic<uint64_t> opportunities_detected{0};
    std::atomic<uint64_t> arbitrage_opportunities{0};
    std::atomic<uint64_t> sandwich_opportunities{0};
    std::atomic<uint64_t> frontrun_opportunities{0};
    std::atomic<uint64_t> liquidation_opportunities{0};
    
    std::atomic<double> total_potential_profit_eth{0.0};
    std::atomic<double> avg_detection_time_ms{0.0};
    std::atomic<double> avg_confidence_score{0.0};
    std::atomic<double> success_rate{0.0};
    
    std::chrono::system_clock::time_point last_reset;
};

// Main MEV Detector Class
class MEVDetector {
public:
    explicit MEVDetector(const MEVDetectorConfig& config);
    ~MEVDetector();

    // Core detection functionality
    std::vector<MEVOpportunity> detect_mev_opportunities(const Transaction& tx);
    std::vector<MEVOpportunity> analyze_transaction_batch(const std::vector<Transaction>& transactions);
    bool is_mev_opportunity(const Transaction& tx);
    MEVType classify_mev_type(const Transaction& tx);
    
    // Specific MEV type detection
    std::vector<MEVOpportunity> detect_arbitrage_opportunities(const std::vector<Transaction>& transactions);
    std::vector<MEVOpportunity> detect_sandwich_opportunities(const std::vector<Transaction>& transactions);
    std::vector<MEVOpportunity> detect_frontrunning_opportunities(const std::vector<Transaction>& transactions);
    std::vector<MEVOpportunity> detect_liquidation_opportunities(const std::vector<Transaction>& transactions);
    
    // Real-time detection
    using MEVCallback = std::function<void(const MEVOpportunity&)>;
    void register_mev_callback(MEVCallback callback);
    void start_real_time_detection();
    void stop_real_time_detection();
    bool is_detecting() const;
    
    // Opportunity management
    std::vector<MEVOpportunity> get_active_opportunities();
    std::vector<MEVOpportunity> get_opportunities_by_type(MEVType type);
    std::vector<MEVOpportunity> get_high_confidence_opportunities(MEVConfidence min_confidence);
    void remove_expired_opportunities();
    
    // Pool and price management
    void update_pool_info(const PoolInfo& pool);
    void update_price_info(const PriceInfo& price);
    std::vector<PoolInfo> get_pools_for_token(const std::string& token);
    std::optional<PriceInfo> get_price_info(const std::string& token);
    
    // Arbitrage detection
    std::vector<std::string> find_arbitrage_paths(const std::string& token_a, const std::string& token_b);
    std::vector<MEVOpportunity> detect_triangular_arbitrage();
    double calculate_arbitrage_profit(const std::vector<std::string>& path, uint64_t amount);
    
    // Sandwich attack detection
    MEVOpportunity analyze_sandwich_opportunity(const Transaction& victim_tx);
    std::pair<Transaction, Transaction> create_sandwich_transactions(const Transaction& victim_tx);
    double calculate_sandwich_profit(const Transaction& victim_tx, uint64_t frontrun_amount);
    
    // Risk assessment
    double assess_execution_risk(const MEVOpportunity& opportunity);
    double assess_market_risk(const MEVOpportunity& opportunity);
    double assess_competition_risk(const MEVOpportunity& opportunity);
    double calculate_overall_risk_score(const MEVOpportunity& opportunity);
    
    // Profit estimation
    double estimate_profit(const MEVOpportunity& opportunity);
    double calculate_gas_costs(const MEVOpportunity& opportunity);
    double calculate_net_profit(const MEVOpportunity& opportunity);
    bool is_profitable_after_costs(const MEVOpportunity& opportunity);
    
    // Configuration management
    void update_config(const MEVDetectorConfig& config);
    MEVDetectorConfig get_config() const;
    void add_dex_address(const std::string& address);
    void remove_dex_address(const std::string& address);
    void add_monitored_chain(uint32_t chain_id);
    void remove_monitored_chain(uint32_t chain_id);
    
    // Statistics and monitoring
    const MEVDetectorStats& get_statistics() const;
    void reset_statistics();
    std::vector<MEVOpportunity> get_recent_opportunities(std::chrono::minutes window);
    double get_detection_success_rate() const;
    
    // Advanced features
    void enable_machine_learning_detection();
    void disable_machine_learning_detection();
    void train_detection_models();
    double get_model_accuracy() const;
    
    // Backtesting and validation
    std::vector<MEVOpportunity> backtest_detection(const std::vector<Transaction>& historical_txs);
    double validate_opportunity(const MEVOpportunity& opportunity);
    std::vector<double> calculate_historical_success_rates();

private:
    MEVDetectorConfig config_;
    std::atomic<bool> detecting_;
    mutable std::mutex config_mutex_;
    
    // Data storage
    std::unordered_map<std::string, PoolInfo> pools_;
    std::unordered_map<std::string, PriceInfo> prices_;
    std::vector<MEVOpportunity> active_opportunities_;
    mutable std::mutex data_mutex_;
    
    // Detection threads
    std::vector<std::thread> detection_threads_;
    std::queue<Transaction> detection_queue_;
    mutable std::mutex queue_mutex_;
    
    // Callbacks
    std::vector<MEVCallback> mev_callbacks_;
    mutable std::mutex callbacks_mutex_;
    
    // Caches
    std::unordered_map<std::string, std::vector<MEVOpportunity>> detection_cache_;
    mutable std::mutex cache_mutex_;
    
    // Statistics
    mutable MEVDetectorStats stats_;
    
    // Machine learning models (placeholder for future implementation)
    // std::unique_ptr<class MLModel> arbitrage_model_;
    // std::unique_ptr<class MLModel> sandwich_model_;
    // std::unique_ptr<class MLModel> frontrun_model_;
    
    // Internal detection methods
    std::vector<MEVOpportunity> detect_arbitrage_internal(const Transaction& tx);
    std::vector<MEVOpportunity> detect_sandwich_internal(const Transaction& tx);
    std::vector<MEVOpportunity> detect_frontrunning_internal(const Transaction& tx);
    std::vector<MEVOpportunity> detect_liquidation_internal(const Transaction& tx);
    
    // Analysis helpers
    bool is_dex_transaction(const Transaction& tx);
    bool is_swap_transaction(const Transaction& tx);
    bool is_liquidity_transaction(const Transaction& tx);
    std::string extract_dex_protocol(const Transaction& tx);
    std::pair<std::string, std::string> extract_token_pair(const Transaction& tx);
    
    // Price analysis
    double get_token_price(const std::string& token, const std::string& dex);
    double calculate_price_impact(const std::string& pool, uint64_t amount);
    std::vector<double> get_price_history(const std::string& token, std::chrono::hours window);
    
    // Path finding for arbitrage
    std::vector<std::vector<std::string>> find_arbitrage_paths(const std::string& token_a, 
                                                              const std::string& token_b, 
                                                              uint32_t max_hops);
    double calculate_path_profit(const std::vector<std::string>& path, uint64_t amount);
    
    // Competition analysis
    uint32_t count_competing_transactions(const MEVOpportunity& opportunity);
    double estimate_competition_level(MEVType type);
    bool is_opportunity_unique(const MEVOpportunity& opportunity);
    
    // Risk calculations
    double calculate_execution_risk(const MEVOpportunity& opportunity);
    double calculate_slippage_risk(const MEVOpportunity& opportunity);
    double calculate_timing_risk(const MEVOpportunity& opportunity);
    
    // Utility methods
    std::string generate_opportunity_id();
    std::string generate_cache_key(const Transaction& tx);
    void update_cache(const std::string& key, const std::vector<MEVOpportunity>& opportunities);
    void cleanup_expired_opportunities();
    void cleanup_expired_cache();
    
    // Worker thread functions
    void detection_worker();
    void opportunity_cleanup_worker();
    void price_update_worker();
    
    // Callbacks and notifications
    void notify_mev_callbacks(const MEVOpportunity& opportunity);
    void update_statistics(const std::vector<MEVOpportunity>& opportunities, double detection_time_ms);
};

// MEV Protection Manager Class
class MEVProtectionManager {
public:
    explicit MEVProtectionManager(const MEVProtectionConfig& config);
    ~MEVProtectionManager();

    // Core protection functionality
    bool submit_private_transaction(const PrivateTransaction& tx);
    bool submit_transaction_bundle(const std::vector<PrivateTransaction>& bundle);
    bool is_transaction_safe(const Transaction& tx);
    double calculate_transaction_risk(const Transaction& tx);

    // Protection strategies
    bool enable_sandwich_protection(const Transaction& tx);
    bool enable_frontrun_protection(const Transaction& tx);
    bool optimize_gas_price(const Transaction& tx, GasEstimate& estimate);

    // Private relay management
    bool connect_to_relay(const std::string& relay_name);
    bool disconnect_from_relay(const std::string& relay_name);
    std::vector<std::string> get_available_relays() const;

    // Monitoring and statistics
    struct ProtectionStats {
        std::atomic<uint64_t> transactions_protected{0};
        std::atomic<uint64_t> attacks_prevented{0};
        std::atomic<uint64_t> private_submissions{0};
        std::atomic<double> avg_protection_time_ms{0.0};
        std::atomic<double> protection_success_rate{0.0};
        std::chrono::system_clock::time_point last_updated;
    };

    const ProtectionStats& get_protection_stats() const;
    void reset_protection_stats();

    // Configuration
    void update_config(const MEVProtectionConfig& config);
    MEVProtectionConfig get_config() const;

    // Real-time protection
    using ProtectionCallback = std::function<void(const Transaction&, bool /*is_protected*/)>;
    void register_protection_callback(ProtectionCallback callback);

    // Bundle management for Solana
    bool create_jito_bundle(const std::vector<Transaction>& transactions,
                           uint64_t tip_amount,
                           std::string& bundle_id);
    bool submit_jito_bundle(const std::string& bundle_id);

    // Flashbots bundle support for Ethereum
    bool submit_flashbots_bundle(const std::vector<PrivateTransaction>& bundle);

    // Emergency controls
    void emergency_stop_protection();
    void resume_protection();
    bool is_protection_active() const;

private:
    MEVProtectionConfig config_;
    std::atomic<bool> protection_active_;
    mutable std::mutex config_mutex_;

    // Private relay connections
    std::unordered_map<std::string, bool> relay_connections_;
    mutable std::mutex relay_mutex_;

    // Protection statistics
    mutable ProtectionStats stats_;

    // Callbacks
    std::vector<ProtectionCallback> protection_callbacks_;
    mutable std::mutex callbacks_mutex_;

    // Transaction monitoring
    std::unordered_map<std::string, Transaction> monitored_transactions_;
    mutable std::mutex transaction_mutex_;

    // Worker threads
    std::vector<std::thread> protection_threads_;
    std::queue<Transaction> protection_queue_;
    mutable std::mutex queue_mutex_;

    // Internal methods
    bool submit_to_flashbots(const PrivateTransaction& tx);
    bool submit_to_eden(const PrivateTransaction& tx);
    bool submit_to_bloxroute(const PrivateTransaction& tx);
    bool submit_to_jito(const PrivateTransaction& tx);

    double assess_sandwich_risk(const Transaction& tx);
    double assess_frontrun_risk(const Transaction& tx);
    double assess_mempool_risk(const Transaction& tx);

    bool apply_protection_measures(Transaction& tx);
    void monitor_transaction_status(const std::string& tx_hash);

    // Worker thread functions
    void protection_worker();
    void monitoring_worker();
    void cleanup_worker();

    // Utility methods
    std::string select_best_relay(const Transaction& tx);
    bool validate_transaction(const Transaction& tx);
    void update_statistics(const Transaction& tx, bool is_protected);
    void notify_protection_callbacks(const Transaction& tx, bool is_protected);
};

// Utility functions
std::string mev_type_to_string(MEVType type);
MEVType string_to_mev_type(const std::string& str);
std::string mev_confidence_to_string(MEVConfidence confidence);
std::string format_mev_opportunity(const MEVOpportunity& opportunity);
bool is_high_value_opportunity(const MEVOpportunity& opportunity, double threshold_usd = 100.0);
double calculate_opportunity_score(const MEVOpportunity& opportunity);

} // namespace mempool
} // namespace hfx
