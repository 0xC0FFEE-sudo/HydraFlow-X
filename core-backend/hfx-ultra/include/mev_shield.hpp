#pragma once

#include <memory>
#include <atomic>
#include <vector>
#include <unordered_map>
#include <string>
#include <chrono>
#include <functional>
#include <mutex>
#include <thread>
#include <queue>

namespace hfx::ultra {

// Advanced MEV protection strategy types
enum class MEVProtectionLevel {
    NONE = 0,           // No protection (fastest but risky)
    BASIC = 1,          // Basic slippage protection
    STANDARD = 2,       // Standard MEV protection with private routing
    HIGH = 3,           // High protection level
    MAXIMUM = 4,        // Maximum protection with multiple strategies
    STEALTH = 5         // Stealth mode with randomization and timing
};

// Private relay types for transaction routing
enum class PrivateRelay {
    FLASHBOTS,
    EDEN_NETWORK,
    BLOXROUTE,
    MANIFOLD,
    SECURERPC,
    JITO_SOLANA,        // For Solana
    CUSTOM
};

// MEV attack types we protect against
enum class MEVAttackType {
    FRONTRUNNING,
    BACKRUNNING,
    SANDWICHING,
    JIT_LIQUIDITY,      // Just-in-time liquidity
    LIQUIDATION_MEV,
    ARBITRAGE_MEV
};

// Transaction structure for MEV analysis
struct Transaction {
    std::string hash;
    std::string from;
    std::string to;
    uint64_t value;
    uint64_t gas_price;
    uint64_t gas_limit;
    std::string data;
};

// MEV Analysis Result
struct MEVAnalysisResult {
    bool is_mev_opportunity;
    double risk_score;  // 0.0 = safe, 1.0 = maximum MEV risk
    MEVProtectionLevel recommended_protection;
    std::vector<std::string> detected_threats;
    uint64_t estimated_mev_value;
};

// MEV Protection Result
struct MEVProtectionResult {
    bool protection_applied;
    MEVProtectionLevel level_used;
    std::string protection_tx_hash;
    uint64_t protection_cost;
    std::chrono::nanoseconds protection_latency;
};

// Real-time MEV threat assessment
struct MEVThreat {
    MEVAttackType attack_type;
    double confidence_score;    // 0.0 to 1.0
    uint64_t estimated_value;   // Estimated MEV value in wei
    uint64_t detected_at_ns;
    std::string attacker_address;
    std::vector<uint64_t> related_tx_hashes;
};

// Bundle configuration for private relay submission
struct BundleConfig {
    PrivateRelay primary_relay = PrivateRelay::FLASHBOTS;
    std::vector<PrivateRelay> backup_relays;
    uint64_t max_block_number = 0;  // 0 = current + 1
    uint64_t min_timestamp = 0;
    uint64_t max_timestamp = 0;
    bool allow_revert = false;
    uint32_t priority_fee_boost = 10; // Percentage boost
    bool enable_bundle_merging = true;
    std::chrono::milliseconds submission_timeout{500};
};

// Real-time slippage protection
struct SlippageProtection {
    double max_slippage_basis_points = 50.0; // 0.5%
    bool dynamic_adjustment = true;
    bool enable_impact_estimation = true;
    uint64_t max_gas_price = 0; // 0 = no limit
    bool emergency_cancel_on_detect = true;
};

// Transaction timing randomization for stealth
struct TimingRandomization {
    bool enable_jitter = true;
    std::chrono::microseconds min_delay{100};
    std::chrono::microseconds max_delay{2000};
    bool enable_batch_randomization = true;
    size_t batch_size_variance = 3;
};

// Ultra-advanced MEV protection configuration
struct MEVShieldConfig {
    MEVProtectionLevel protection_level = MEVProtectionLevel::STANDARD;
    
    // Relay configuration
    BundleConfig bundle_config;
    std::unordered_map<PrivateRelay, std::string> relay_endpoints;
    std::unordered_map<PrivateRelay, std::string> relay_auth_keys;
    
    // Protection settings
    SlippageProtection slippage_config;
    TimingRandomization timing_config;
    
    // Detection thresholds
    double mev_detection_threshold = 0.001; // 0.1% of trade value
    uint64_t min_protection_value = 100000000000000000ULL; // 0.1 ETH
    
    // Performance settings
    size_t worker_threads = 4;
    size_t max_concurrent_bundles = 10;
    std::chrono::milliseconds bundle_refresh_interval{100};
    bool enable_predictive_protection = true;
    
    // Monitoring
    bool enable_mev_analytics = true;
    bool log_protection_events = true;
};

// Real-time MEV analytics and metrics
struct MEVAnalytics {
    std::atomic<uint64_t> total_protected_trades{0};
    std::atomic<uint64_t> mev_attacks_detected{0};
    std::atomic<uint64_t> mev_attacks_prevented{0};
    std::atomic<uint64_t> total_value_protected{0}; // in wei
    std::atomic<uint64_t> gas_saved{0};
    std::atomic<double> average_protection_latency_ms{0.0};
    
    // Attack type breakdown
    std::atomic<uint64_t> frontrun_attacks{0};
    std::atomic<uint64_t> sandwich_attacks{0};
    std::atomic<uint64_t> backrun_attacks{0};
    std::atomic<uint64_t> jit_attacks{0};
    
    // Performance metrics
    std::atomic<uint64_t> bundle_submissions{0};
    std::atomic<uint64_t> bundle_successes{0};
    std::atomic<uint64_t> bundle_failures{0};
    std::atomic<double> bundle_success_rate{0.0};
};

// Advanced MEV protection system - better than any existing bot
class MEVShield {
public:
    using ThreatDetectedCallback = std::function<void(const MEVThreat&)>;
    using ProtectionAppliedCallback = std::function<void(const std::string&, MEVProtectionLevel)>;
    
    explicit MEVShield(const MEVShieldConfig& config);
    ~MEVShield();
    
    // Core protection interface
    bool start();
    bool stop();
    bool is_running() const { return running_.load(); }
    
    // Transaction protection
    struct ProtectedTransaction {
        std::string original_tx_hash;
        std::vector<std::string> bundle_hashes;
        PrivateRelay used_relay;
        MEVProtectionLevel applied_level;
        uint64_t protection_gas_cost;
        std::chrono::nanoseconds protection_latency;
        bool successful;
    };
    
    ProtectedTransaction protect_transaction(const std::string& tx_data,
                                           MEVProtectionLevel level = MEVProtectionLevel::STANDARD);
    
    MEVAnalysisResult analyze_transaction(const Transaction& tx);
    MEVProtectionResult apply_protection(const Transaction& tx, const MEVAnalysisResult& analysis);
    std::string create_protected_bundle(const std::vector<Transaction>& transactions);
    
    // Real-time MEV detection
    std::vector<MEVThreat> detect_mev_threats(const std::string& tx_hash,
                                             const std::vector<std::string>& mempool_snapshot);
    
    // Bundle management
    std::string create_protection_bundle(const std::vector<std::string>& transactions,
                                       const BundleConfig& config);
    bool submit_bundle_to_relay(const std::string& bundle_data, PrivateRelay relay);
    
    // Dynamic protection adjustment
    void update_protection_level(MEVProtectionLevel new_level);
    void adjust_slippage_tolerance(double new_tolerance_bps);
    void enable_stealth_mode(bool enable);
    
    // Callbacks and monitoring
    void register_threat_callback(ThreatDetectedCallback callback);
    void register_protection_callback(ProtectionAppliedCallback callback);
    
    // Analytics and metrics
    const MEVAnalytics& get_analytics() const { return analytics_; }
    void reset_analytics();
    
    // Advanced features
    void enable_predictive_protection(bool enable) { predictive_protection_ = enable; }
    void set_mev_simulation_engine(bool enable) { simulation_engine_ = enable; }
    void enable_cross_dex_protection(bool enable) { cross_dex_protection_ = enable; }
    
private:
    MEVShieldConfig config_;
    std::atomic<bool> running_{false};
    MEVAnalytics analytics_;
    
    // Threading
    std::vector<std::thread> protection_threads_;
    std::vector<std::thread> detection_threads_;
    std::vector<std::thread> worker_threads_;
    
    // MEV detection engine
    class MEVDetectionEngine {
    public:
        std::vector<MEVThreat> analyze_transaction_context(
            const std::string& tx_hash,
            const std::vector<std::string>& mempool_data);
        
        bool is_sandwich_attack(const std::string& tx_hash,
                               const std::vector<std::string>& surrounding_txs);
        
        bool is_frontrun_attempt(const std::string& tx_hash,
                                const std::string& target_tx);
        
        double calculate_mev_value(const MEVThreat& threat);
        
    private:
        std::unordered_map<std::string, std::vector<std::string>> tx_patterns_;
        std::mutex pattern_mutex_;
    };
    
    std::unique_ptr<MEVDetectionEngine> detection_engine_;
    
    // Bundle management
    struct PendingBundle {
        std::string bundle_id;
        std::vector<std::string> transactions;
        PrivateRelay target_relay;
        uint64_t target_block;
        std::chrono::steady_clock::time_point created_at;
        std::atomic<bool> submitted{false};
        std::atomic<bool> included{false};
        MEVProtectionLevel protection_level{MEVProtectionLevel::STANDARD};
        
        // Custom copy constructor and assignment operator
        PendingBundle() = default;
        PendingBundle(const PendingBundle& other) 
            : bundle_id(other.bundle_id), transactions(other.transactions), 
              target_relay(other.target_relay), target_block(other.target_block),
              created_at(other.created_at), protection_level(other.protection_level),
              submitted(other.submitted.load()), included(other.included.load()) {}
        PendingBundle& operator=(const PendingBundle& other) {
            if (this != &other) {
                bundle_id = other.bundle_id;
                transactions = other.transactions;
                target_relay = other.target_relay;
                target_block = other.target_block;
                created_at = other.created_at;
                protection_level = other.protection_level;
                submitted.store(other.submitted.load());
                included.store(other.included.load());
            }
            return *this;
        }
    };
    
    std::vector<PendingBundle> pending_bundles_;
    mutable std::mutex bundles_mutex_;
    
    // Relay management
    class RelayManager {
    public:
        bool submit_bundle(const std::string& bundle_data, 
                          PrivateRelay relay,
                          const std::string& endpoint,
                          const std::string& auth_key);
        
        bool is_relay_healthy(PrivateRelay relay);
        uint64_t get_relay_latency_ms(PrivateRelay relay);
        
    private:
        std::unordered_map<PrivateRelay, std::chrono::steady_clock::time_point> last_successful_;
        std::unordered_map<PrivateRelay, uint64_t> relay_latencies_;
        mutable std::mutex relay_mutex_;
    };
    
    std::unique_ptr<RelayManager> relay_manager_;
    
    // Protection strategies
    ProtectedTransaction apply_basic_protection(const std::string& tx_data);
    ProtectedTransaction apply_standard_protection(const std::string& tx_data);
    ProtectedTransaction apply_maximum_protection(const std::string& tx_data);
    ProtectedTransaction apply_stealth_protection(const std::string& tx_data);
    
    // Timing and randomization
    std::chrono::microseconds calculate_optimal_delay(const std::string& tx_data);
    void apply_timing_jitter(std::chrono::microseconds base_delay);
    
    // Callbacks
    std::vector<ThreatDetectedCallback> threat_callbacks_;
    std::vector<ProtectionAppliedCallback> protection_callbacks_;
    mutable std::mutex callback_mutex_;
    
    // Configuration management
    std::atomic<bool> predictive_protection_{true};
    std::atomic<bool> simulation_engine_{true};
    std::atomic<bool> cross_dex_protection_{true};
    
    // Internal methods
    void worker_thread(size_t thread_id);
    void protection_worker_thread(size_t thread_id);
    void detection_worker_thread(size_t thread_id);
    void bundle_monitor_thread();
    
    bool should_protect_transaction(const std::string& tx_data) const;
    PrivateRelay select_optimal_relay() const;
    void update_analytics(const ProtectedTransaction& result);
    
    std::string generate_bundle_id() const;
    uint64_t estimate_gas_cost(MEVProtectionLevel level) const;
    
    // Advanced protection techniques
    std::string create_decoy_transactions(const std::string& real_tx);
    std::vector<std::string> fragment_large_transaction(const std::string& tx_data);
    void implement_time_weighted_protection(const std::string& tx_data);
};

// Factory for creating chain-specific MEV shields
class MEVShieldFactory {
public:
    static std::unique_ptr<MEVShield> create_ethereum_shield();
    static std::unique_ptr<MEVShield> create_bsc_shield();
    static std::unique_ptr<MEVShield> create_arbitrum_shield();
    static std::unique_ptr<MEVShield> create_solana_shield();
    static std::unique_ptr<MEVShield> create_custom_shield(const MEVShieldConfig& config);
    
    // Utility methods
    static BundleConfig get_optimal_bundle_config(PrivateRelay relay);
    static std::vector<PrivateRelay> get_available_relays_for_chain(const std::string& chain_id);
};

} // namespace hfx::ultra
