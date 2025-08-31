#pragma once

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <atomic>
#include <chrono>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <random>

namespace hfx::ultra {

// Forward declarations
struct SolanaTransaction {
    std::string signature;
    std::vector<uint8_t> data;
    uint64_t compute_units = 200000;
    uint64_t priority_fee_lamports = 1000;
    
    // Additional fields for MEV analysis
    std::string payer;
    std::string program_id;
    std::string recent_blockhash;
    std::vector<std::string> accounts;
    uint64_t fee = 5000;
    bool is_mev_transaction = false;
    double estimated_mev_value = 0.0;
};

// Bundle status tracking
enum class BundleStatus {
    PENDING,
    SUBMITTED,
    CONFIRMED,
    FAILED,
    EXPIRED
};

// Jito bundle representation
struct JitoBundle {
    std::string bundle_id;
    std::vector<SolanaTransaction> transactions;
    BundleStatus status = BundleStatus::PENDING;
    uint64_t target_slot = 0;
    std::chrono::steady_clock::time_point created_at;
    uint64_t tip_lamports = 0;
};

// Bundle result from submission
struct JitoBundleResult {
    std::string bundle_id;
    BundleStatus status;
    bool success;
    std::string error_message;
    uint64_t included_slot = 0;
    std::chrono::milliseconds latency{0};
};

// Jito MEV bundle types for Solana
enum class JitoBundleType {
    STANDARD,           // Standard Jito bundle
    PRIORITY,           // High-priority bundle with tip boost
    STEALTH,            // Stealth bundle with randomization
    ATOMIC,             // Atomic bundle (all-or-nothing)
    TIMED              // Time-delayed bundle
};

// Solana transaction priorities for Jito
enum class SolanaPriority {
    NONE = 0,           // No priority fee
    LOW = 1,            // Minimum priority fee
    MEDIUM = 2,         // Medium priority fee  
    HIGH = 3,           // High priority fee
    ULTRA = 4           // Maximum priority fee
};

// Jito bundle configuration
struct JitoBundleConfig {
    JitoBundleType bundle_type = JitoBundleType::STANDARD;
    SolanaPriority priority_level = SolanaPriority::MEDIUM;
    
    // Timing configuration
    uint64_t target_slot = 0;              // 0 = next available slot
    uint64_t max_slot_delay = 5;           // Max slots to wait
    std::chrono::milliseconds submission_timeout{200};
    
    // Economic configuration
    uint64_t tip_lamports = 10000;         // Tip for validators (10k lamports = 0.01 SOL)
    uint64_t max_tip_lamports = 100000;    // Maximum tip willing to pay
    bool dynamic_tip_adjustment = true;
    
    // Bundle composition
    size_t max_bundle_size = 5;
    size_t max_transactions_per_bundle = 5;
    uint64_t max_compute_units = 1400000 * 5; // 5 Jupiter swaps max
    bool allow_failed_transactions = false;
    bool enable_bundle_simulation = true;
    
    // Performance settings
    bool use_shred_stream = true;          // Use Jito ShredStream for early data
    bool enable_tpu_direct = true;         // Direct TPU submission
    std::vector<std::string> preferred_validators;
    size_t worker_threads = 4;             // Number of worker threads
};

// Real-time Solana slot information
struct SlotInfo {
    uint64_t slot_number;
    uint64_t parent_slot;
    std::string leader;                    // Current slot leader
    uint64_t timestamp_ms;
    uint32_t transaction_count;
    bool is_finalized;
    std::chrono::nanoseconds slot_start_time;
    
    // MEV-relevant data
    uint64_t total_tips_collected;
    uint32_t bundle_count;
    std::vector<std::string> included_bundles;
};

// Extended fields for JitoBundleResult are included in the earlier definition

// Advanced Solana MEV protection using Jito bundles
class JitoMEVEngine {
public:
    using BundleCallback = std::function<void(const JitoBundleResult&)>;
    using SlotUpdateCallback = std::function<void(const SlotInfo&)>;
    
    explicit JitoMEVEngine(const JitoBundleConfig& config);
    ~JitoMEVEngine();
    
    // Core Jito operations
    bool start();
    void stop();
    bool is_running() const { return running_.load(); }
    
    // Bundle creation and submission
    std::string create_bundle(const std::vector<std::string>& transactions,
                            const JitoBundleConfig& config = {});
    
    JitoBundleResult submit_bundle(const std::string& bundle_id,
                                 bool wait_for_confirmation = true);
    
    // Transaction bundling strategies
    std::string create_snipe_bundle(const std::string& target_token,
                                  uint64_t amount_lamports,
                                  SolanaPriority priority = SolanaPriority::HIGH);
    
    std::string create_arbitrage_bundle(const std::vector<std::string>& dex_transactions,
                                      uint64_t min_profit_lamports = 5000);
    
    std::string create_liquidation_bundle(const std::string& liquidation_tx,
                                        const std::vector<std::string>& setup_txs = {});
    
    // Advanced MEV strategies
    struct MEVOpportunity {
        enum Type { ARBITRAGE, LIQUIDATION, SANDWICH, JIT_LIQUIDITY } type;
        std::string target_pool;
        uint64_t estimated_profit_lamports;
        std::vector<std::string> required_transactions;
        uint64_t optimal_slot;
        SolanaPriority recommended_priority;
        std::chrono::milliseconds time_window;
    };
    
    std::vector<MEVOpportunity> scan_mev_opportunities();
    JitoBundleResult execute_mev_opportunity(const MEVOpportunity& opportunity);
    
    // Slot tracking and timing
    SlotInfo get_current_slot_info() const;
    uint64_t get_next_slot_with_leader(const std::string& validator_identity) const;
    std::chrono::nanoseconds get_time_until_slot(uint64_t target_slot) const;
    
    // ShredStream integration for early block data access
    void enable_shred_stream(bool enable) { use_shred_stream_ = enable; }
    void register_early_block_callback(std::function<void(uint64_t slot, const std::vector<uint8_t>&)> callback);
    
    // Dynamic tip calculation
    uint64_t calculate_optimal_tip(SolanaPriority priority,
                                 uint64_t transaction_value_lamports) const;
    
    void update_tip_strategy(bool aggressive_tipping) { aggressive_tipping_ = aggressive_tipping; }
    
    // Callbacks
    void register_bundle_callback(BundleCallback callback);
    void register_slot_callback(SlotUpdateCallback callback);
    
    // Additional methods
    BundleStatus get_bundle_status(const std::string& bundle_id) const;
    bool add_shred_stream_callback(std::function<void(uint64_t, const std::vector<uint8_t>&)> callback);
    uint64_t get_current_slot() const;
    uint64_t get_current_leader_slot() const;
    bool update_slot_info(uint64_t slot, const SlotInfo& info);
    
    // Performance metrics
    struct Metrics {
        std::atomic<uint64_t> bundles_created{0};
        std::atomic<uint64_t> bundles_submitted{0};
        std::atomic<uint64_t> bundles_landed{0};
        std::atomic<uint64_t> bundles_failed{0};
        std::atomic<uint64_t> total_tips_paid{0};
        std::atomic<double> total_mev_extracted{0.0};
        std::atomic<double> average_confirmation_time_ms{0.0};
        std::atomic<double> avg_bundle_latency{0.0};
        std::atomic<double> bundle_success_rate{0.0};
        
        // Slot tracking
        std::atomic<uint64_t> current_slot{0};
        std::atomic<uint64_t> slots_tracked{0};
        std::atomic<double> average_slot_time_ms{400.0}; // ~400ms per slot
        
        // Strategy performance
        std::atomic<uint64_t> arbitrage_profits{0};
        std::atomic<uint64_t> liquidation_profits{0};
        std::atomic<uint64_t> snipe_successes{0};
    };
    
    void get_metrics(Metrics& metrics_out) const {
        metrics_out.bundles_created.store(metrics_.bundles_created.load());
        metrics_out.bundles_submitted.store(metrics_.bundles_submitted.load());
        metrics_out.bundles_landed.store(metrics_.bundles_landed.load());
        metrics_out.total_tips_paid.store(metrics_.total_tips_paid.load());
        metrics_out.total_mev_extracted.store(metrics_.total_mev_extracted.load());
        metrics_out.average_confirmation_time_ms.store(metrics_.average_confirmation_time_ms.load());
        metrics_out.bundle_success_rate.store(metrics_.bundle_success_rate.load());
        metrics_out.current_slot.store(metrics_.current_slot.load());
        metrics_out.slots_tracked.store(metrics_.slots_tracked.load());
        metrics_out.average_slot_time_ms.store(metrics_.average_slot_time_ms.load());
        metrics_out.arbitrage_profits.store(metrics_.arbitrage_profits.load());
        metrics_out.liquidation_profits.store(metrics_.liquidation_profits.load());
        metrics_out.snipe_successes.store(metrics_.snipe_successes.load());
    }
    void reset_metrics();
    
private:
    JitoBundleConfig config_;
    std::atomic<bool> running_{false};
    mutable Metrics metrics_;
    
    // Jito connection management
    class JitoConnection {
    public:
        bool connect(const std::string& endpoint, const std::string& auth_key);
        void disconnect();
        bool is_connected() const { return connected_; }
        
        std::string submit_bundle_request(const std::string& bundle_data);
        std::vector<std::string> get_bundle_status(const std::string& bundle_id);
        SlotInfo get_slot_info(uint64_t slot = 0); // 0 = current
        
    private:
        std::atomic<bool> connected_{false};
        std::string endpoint_;
        std::string auth_key_;
        std::mutex connection_mutex_;
    };
    
    std::unique_ptr<JitoConnection> jito_connection_;
    
    // Bundle management
    struct PendingBundle {
        std::string bundle_id;
        std::vector<std::string> transactions;
        JitoBundleConfig config;
        uint64_t target_slot;
        std::chrono::steady_clock::time_point created_at;
        std::atomic<bool> submitted{false};
        std::atomic<bool> confirmed{false};
        BundleStatus status{BundleStatus::PENDING};
        
        // Additional fields for enhanced tracking
        double estimated_mev_value = 0.0;
        uint64_t compute_units = 0;
        std::string jito_tip_account;
        uint64_t actual_tip_paid = 0;
        
        // Custom copy constructor and assignment operator
        PendingBundle() = default;
        PendingBundle(const PendingBundle& other) 
            : bundle_id(other.bundle_id), transactions(other.transactions), 
              config(other.config), target_slot(other.target_slot), 
              created_at(other.created_at), submitted(other.submitted.load()), 
              confirmed(other.confirmed.load()), status(other.status),
              estimated_mev_value(other.estimated_mev_value), compute_units(other.compute_units),
              jito_tip_account(other.jito_tip_account), actual_tip_paid(other.actual_tip_paid) {}
        PendingBundle& operator=(const PendingBundle& other) {
            if (this != &other) {
                bundle_id = other.bundle_id;
                transactions = other.transactions;
                config = other.config;
                target_slot = other.target_slot;
                created_at = other.created_at;
                status = other.status;
                estimated_mev_value = other.estimated_mev_value;
                compute_units = other.compute_units;
                jito_tip_account = other.jito_tip_account;
                actual_tip_paid = other.actual_tip_paid;
                submitted.store(other.submitted.load());
                confirmed.store(other.confirmed.load());
            }
            return *this;
        }
    };
    
    std::unordered_map<std::string, PendingBundle> pending_bundles_;
    mutable std::mutex bundles_mutex_;
    
    // Slot tracking
    std::atomic<uint64_t> current_slot_{0};
    std::atomic<uint64_t> current_leader_slot_{0};
    std::unordered_map<uint64_t, SlotInfo> slot_history_;
    mutable std::mutex slot_mutex_;
    
    // ShredStream integration
    std::atomic<bool> use_shred_stream_{true};
    std::function<void(uint64_t, const std::vector<uint8_t>&)> shred_callback_;
    
    // Tip calculation and strategy
    std::atomic<bool> aggressive_tipping_{false};
    std::unordered_map<SolanaPriority, uint64_t> base_tips_;
    
    // Threading
    std::vector<std::thread> worker_threads_;
    std::thread slot_monitor_thread_;
    std::thread shred_stream_thread_;
    
    // Callbacks
    std::vector<BundleCallback> bundle_callbacks_;
    std::vector<SlotUpdateCallback> slot_callbacks_;
    mutable std::mutex callback_mutex_;
    
    // MEV analysis and simulation
    std::unordered_map<std::string, std::string> common_addresses_;
    mutable std::mt19937 random_generator_;
    
    // Internal methods
    void slot_monitor_worker();
    void shred_stream_worker();
    void bundle_monitor_worker();
    
    void update_slot_info(const SlotInfo& slot_info);
    void process_bundle_confirmations();
    
    std::string generate_bundle_id() const;
    bool validate_bundle_transactions(const std::vector<std::string>& transactions) const;
    
    // MEV opportunity detection
    std::vector<MEVOpportunity> scan_arbitrage_opportunities() const;
    std::vector<MEVOpportunity> scan_liquidation_opportunities() const;
    bool is_profitable_opportunity(const MEVOpportunity& opportunity) const;
    
    // Timing optimization
    uint64_t predict_optimal_submission_slot(uint64_t target_execution_slot) const;
    std::chrono::nanoseconds calculate_submission_timing(uint64_t target_slot) const;
    
    // Performance optimization
    void optimize_tip_for_slot(uint64_t slot, uint64_t& tip_lamports) const;
    void update_success_metrics(const JitoBundleResult& result);
    
    // Worker thread methods
    void worker_thread(size_t thread_id);
    void process_pending_bundles();
    void update_current_slot();
    
    // Helper methods for enhanced functionality
    std::string generateBundleId();
    SolanaTransaction parseTransaction(const std::string& tx_data);
    double estimateMEVPotential(const SolanaTransaction& tx);
    uint64_t estimateComputeUnits(const SolanaTransaction& tx);
    void initializeCommonAddresses();
    void updateLeaderSchedule(uint64_t slot);
    std::string generateRandomAddress();
    std::string generateRandomBlockhash();
    std::vector<std::string> generateRandomAccounts();
};

// Automated trading mode for Jito
class JitoAFKMode {
public:
    struct AFKConfig {
        // Automated sniping
        bool enable_pump_fun_sniping = true;
        bool enable_raydium_sniping = true;
        uint64_t max_snipe_amount_lamports = 1000000; // 0.001 SOL
        
        // Automated selling
        bool enable_auto_sell_on_bonding = true;
        double auto_sell_profit_threshold = 2.0; // 2x profit
        double auto_sell_loss_threshold = -0.5;  // 50% loss
        
        // Risk management
        uint64_t max_daily_loss_lamports = 100000000; // 0.1 SOL
        uint32_t max_trades_per_hour = 10;
        
        // Filtering
        uint64_t min_market_cap_lamports = 80000000000; // 80k SOL minimum
        uint64_t max_market_cap_lamports = 1000000000000; // 1M SOL
        bool require_liquidity_lock = true;
        uint32_t min_lock_days = 30;
    };
    
    explicit JitoAFKMode(std::shared_ptr<JitoMEVEngine> engine, const AFKConfig& config);
    
    bool start_afk_mode();
    void stop_afk_mode();
    bool is_afk_running() const { return afk_running_.load(); }
    
    // Configure automated strategies
    void set_snipe_filters(const std::vector<std::string>& token_patterns);
    void set_profit_targets(double min_profit_pct, double max_profit_pct);
    void update_risk_limits(uint64_t max_daily_loss, uint32_t max_hourly_trades);
    
private:
    std::shared_ptr<JitoMEVEngine> jito_engine_;
    AFKConfig config_;
    std::atomic<bool> afk_running_{false};
    
    // Automated workers
    std::thread snipe_monitor_thread_;
    std::thread portfolio_manager_thread_;
    std::thread risk_monitor_thread_;
    
    void snipe_monitor_worker();
    void portfolio_manager_worker();
    void risk_monitor_worker();
    
    bool should_snipe_token(const std::string& token_address) const;
    bool should_sell_position(const std::string& token_address, double current_pnl) const;
};

// Factory for creating Jito engines with different configurations
class JitoEngineFactory {
public:
    // Predefined configurations for different use cases
    static std::unique_ptr<JitoMEVEngine> create_high_performance_engine();
    static std::unique_ptr<JitoMEVEngine> create_sniper_engine();
    static std::unique_ptr<JitoMEVEngine> create_arbitrage_engine();
    static std::unique_ptr<JitoMEVEngine> create_custom_engine(const JitoBundleConfig& config);
    
    // Utility methods
    static JitoBundleConfig get_optimal_config_for_strategy(const std::string& strategy);
    static std::vector<std::string> get_high_performance_validators();
    static uint64_t estimate_optimal_tip(uint64_t trade_value_lamports, SolanaPriority priority);
};

} // namespace hfx::ultra
