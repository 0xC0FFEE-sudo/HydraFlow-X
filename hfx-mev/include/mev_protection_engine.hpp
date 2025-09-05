/**
 * @file mev_protection_engine.hpp
 * @brief Comprehensive MEV protection engine with advanced detection and mitigation
 * @version 1.0.0
 */

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <atomic>
#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <chrono>
#include <mutex>
#include <thread>
#include <queue>

namespace hfx::mev {

// MEV attack types
enum class MEVAttackType {
    SANDWICH,           // Front + back run around user transaction
    FRONTRUN,           // Execute before user transaction
    BACKRUN,           // Execute after user transaction for profit
    JIT_LIQUIDITY,     // Just-in-time liquidity provision
    ARBITRAGE,         // Cross-DEX arbitrage opportunities
    LIQUIDATION,       // Liquidation MEV
    TIME_BANDIT,       // Reordering transactions for profit
    UNKNOWN
};

// Protection strategies
enum class ProtectionStrategy {
    PRIVATE_MEMPOOL,    // Use private transaction pools
    BUNDLE_SUBMISSION,  // Submit as atomic bundles
    TIMING_RANDOMIZATION, // Random delays to avoid patterns
    FLASHBOTS_PROTECT,  // Flashbots protected transactions
    JITO_BUNDLE,       // Jito bundle for Solana
    STEALTH_MODE,      // Advanced obfuscation techniques
    BATCH_AUCTION      // Batch auction mechanisms
};

// Protection levels
enum class ProtectionLevel {
    NONE,              // No protection
    BASIC,             // Basic anti-MEV measures
    STANDARD,          // Standard protection suite
    HIGH,              // High security protection
    MAXIMUM            // Maximum protection (highest cost)
};

// Transaction context for MEV analysis
struct TransactionContext {
    std::string tx_hash;
    std::string from_address;
    std::string to_address;
    std::string contract_address;
    std::string function_selector;
    uint64_t gas_price;
    uint64_t gas_limit;
    uint64_t value;
    std::string data;
    uint64_t nonce;
    std::string chain_id;
    uint64_t timestamp;
    
    // DEX-specific context
    std::string token_in;
    std::string token_out;
    uint64_t amount_in;
    uint64_t amount_out_min;
    std::string pool_address;
    uint32_t fee_tier;
    double slippage_tolerance;
    
    // Position in mempool
    uint32_t mempool_position;
    std::vector<std::string> surrounding_txs;
    
    TransactionContext() = default;
};

// MEV threat detection result
struct MEVThreat {
    MEVAttackType attack_type;
    double confidence_score;       // 0.0 to 1.0
    double severity_score;         // 0.0 to 1.0  
    double profit_potential_usd;   // Estimated MEV profit
    std::string threat_description;
    std::vector<std::string> suspicious_transactions; // Related suspicious txs
    std::chrono::system_clock::time_point detected_at;
    
    // Attack-specific data
    struct SandwichDetails {
        std::string frontrun_tx;
        std::string backrun_tx;
        std::string victim_tx;
        double estimated_loss_usd;
    } sandwich_details;
    
    struct ArbitrageDetails {
        std::vector<std::string> pool_addresses;
        double price_difference_bps;
        double gas_cost_usd;
    } arbitrage_details;
};

// Protection result
struct ProtectionResult {
    bool protection_applied;
    ProtectionStrategy strategy_used;
    ProtectionLevel level_used;
    std::string protected_tx_hash;
    std::string bundle_id;
    double protection_cost_usd;
    std::chrono::nanoseconds protection_latency;
    bool successful;
    std::string error_message;
    
    // Cost breakdown
    double gas_overhead_usd;
    double relay_fee_usd;
    double timing_delay_cost_usd;
    
    ProtectionResult() : protection_applied(false), successful(false) {}
};

// Bundle configuration
struct BundleConfig {
    uint32_t max_bundle_size = 5;
    uint64_t max_block_number = 0;  // 0 = next block
    uint64_t min_timestamp = 0;
    bool reverting_tx_hashes_allowed = false;
    std::string target_block_hash;
    std::vector<std::string> builders; // Preferred block builders
    double max_bundle_fee_usd = 100.0;
    
    BundleConfig() = default;
};

// MEV engine configuration
struct MEVEngineConfig {
    // Detection settings
    bool enable_detection = true;
    double detection_threshold = 0.7;
    uint32_t mempool_analysis_depth = 100;
    std::chrono::milliseconds analysis_timeout{500};
    
    // Protection settings
    bool enable_protection = true;
    ProtectionLevel default_protection_level = ProtectionLevel::STANDARD;
    std::vector<ProtectionStrategy> preferred_strategies;
    double max_protection_cost_usd = 50.0;
    
    // Bundle settings
    BundleConfig default_bundle_config;
    std::vector<std::string> flashbots_relayers;
    std::vector<std::string> jito_relayers;
    
    // Private mempool settings
    std::vector<std::string> private_mempool_urls;
    std::string api_key_flashbots;
    std::string api_key_jito;
    
    // Performance settings
    uint32_t worker_thread_count = 4;
    uint32_t max_concurrent_analysis = 50;
    std::chrono::microseconds max_protection_latency{100000}; // 100ms
    
    MEVEngineConfig() {
        preferred_strategies = {
            ProtectionStrategy::BUNDLE_SUBMISSION,
            ProtectionStrategy::PRIVATE_MEMPOOL,
            ProtectionStrategy::TIMING_RANDOMIZATION
        };
        
        flashbots_relayers = {
            "https://relay.flashbots.net",
            "https://builder0x69.io"
        };
        
        jito_relayers = {
            "https://mainnet.block-engine.jito.wtf",
            "https://amsterdam.mainnet.block-engine.jito.wtf"
        };
    }
};

// Performance metrics
struct MEVEngineMetrics {
    std::atomic<uint64_t> total_transactions_analyzed{0};
    std::atomic<uint64_t> threats_detected{0};
    std::atomic<uint64_t> protections_applied{0};
    std::atomic<uint64_t> successful_protections{0};
    std::atomic<uint64_t> failed_protections{0};
    std::atomic<double> total_protection_cost_usd{0.0};
    std::atomic<double> total_mev_saved_usd{0.0};
    std::atomic<uint64_t> avg_analysis_time_ns{0};
    std::atomic<uint64_t> avg_protection_time_ns{0};
    
    std::chrono::system_clock::time_point start_time;
    std::chrono::system_clock::time_point last_activity;
    
    // Attack type breakdown
    std::atomic<uint64_t> sandwich_attacks_detected{0};
    std::atomic<uint64_t> frontrun_attacks_detected{0};
    std::atomic<uint64_t> arbitrage_opportunities_detected{0};
    std::atomic<uint64_t> jit_liquidity_detected{0};
    
    MEVEngineMetrics() {
        start_time = std::chrono::system_clock::now();
        last_activity = start_time;
    }
};

// Callback types
using ThreatDetectedCallback = std::function<void(const MEVThreat&)>;
using ProtectionAppliedCallback = std::function<void(const ProtectionResult&)>;
using MempoolAnalysisCallback = std::function<void(const std::vector<TransactionContext>&)>;

/**
 * @brief Advanced MEV Protection Engine
 * 
 * Provides comprehensive protection against MEV attacks including:
 * - Real-time transaction analysis and threat detection
 * - Multiple protection strategies (bundles, private mempools, etc.)
 * - Support for both Ethereum (Flashbots) and Solana (Jito)
 * - Advanced pattern recognition and ML-based detection
 * - Performance optimized for sub-100ms protection latency
 */
class MEVProtectionEngine {
public:
    explicit MEVProtectionEngine(const MEVEngineConfig& config);
    ~MEVProtectionEngine();
    
    // Core lifecycle
    bool start();
    void stop();
    bool is_running() const;
    
    // Transaction analysis and protection
    MEVThreat analyze_transaction(const TransactionContext& tx_context);
    ProtectionResult protect_transaction(const TransactionContext& tx_context, 
                                       ProtectionLevel level = ProtectionLevel::STANDARD);
    
    // Batch operations for efficiency
    std::vector<MEVThreat> analyze_transaction_batch(const std::vector<TransactionContext>& transactions);
    std::vector<ProtectionResult> protect_transaction_batch(const std::vector<TransactionContext>& transactions,
                                                          ProtectionLevel level = ProtectionLevel::STANDARD);
    
    // Bundle creation and submission
    std::string create_protection_bundle(const std::vector<std::string>& transaction_hashes,
                                       const BundleConfig& config = BundleConfig{});
    bool submit_bundle_flashbots(const std::string& bundle_data, const std::vector<std::string>& relayers);
    bool submit_bundle_jito(const std::string& bundle_data, const std::vector<std::string>& relayers);
    
    // Private mempool operations
    bool submit_to_private_mempool(const std::string& transaction_data, const std::string& mempool_url);
    std::vector<std::string> get_available_private_mempools() const;
    
    // Mempool monitoring
    void start_mempool_monitoring(const std::string& rpc_url);
    void stop_mempool_monitoring();
    std::vector<TransactionContext> get_mempool_snapshot();
    
    // Dynamic configuration
    void update_config(const MEVEngineConfig& config);
    void update_protection_level(ProtectionLevel level);
    void enable_strategy(ProtectionStrategy strategy, bool enabled);
    void set_max_protection_cost(double max_cost_usd);
    
    // Callbacks
    void register_threat_callback(ThreatDetectedCallback callback);
    void register_protection_callback(ProtectionAppliedCallback callback);
    void register_mempool_callback(MempoolAnalysisCallback callback);
    
    // Analytics and metrics
    MEVEngineMetrics get_metrics() const;
    void reset_metrics();
    std::vector<MEVThreat> get_recent_threats(std::chrono::minutes lookback = std::chrono::minutes(60)) const;
    std::vector<ProtectionResult> get_recent_protections(std::chrono::minutes lookback = std::chrono::minutes(60)) const;
    
    // Advanced features
    void enable_stealth_mode(bool enabled);
    void set_timing_randomization(bool enabled, std::chrono::milliseconds max_delay = std::chrono::milliseconds(1000));
    void enable_pattern_learning(bool enabled);
    void update_threat_signatures(const std::vector<std::string>& signatures);
    
    // Simulation and testing
    struct SimulationResult {
        bool would_be_attacked;
        MEVAttackType attack_type;
        double estimated_loss_usd;
        ProtectionStrategy recommended_protection;
        double protection_cost_usd;
    };
    
    SimulationResult simulate_transaction(const TransactionContext& tx_context) const;
    bool test_protection_strategy(ProtectionStrategy strategy, const TransactionContext& tx_context);

private:
    class MEVEngineImpl;
    std::unique_ptr<MEVEngineImpl> pimpl_;
};

/**
 * @brief MEV Detection Algorithms
 * 
 * Advanced algorithms for detecting various MEV attack patterns
 */
class MEVDetectionAlgorithms {
public:
    // Sandwich attack detection
    static MEVThreat detect_sandwich_attack(const TransactionContext& tx, 
                                          const std::vector<TransactionContext>& mempool);
    
    // Frontrunning detection
    static MEVThreat detect_frontrunning(const TransactionContext& tx,
                                       const std::vector<TransactionContext>& mempool);
    
    // JIT liquidity detection  
    static MEVThreat detect_jit_liquidity(const TransactionContext& tx,
                                        const std::vector<TransactionContext>& mempool);
    
    // Arbitrage opportunity detection
    static MEVThreat detect_arbitrage_opportunity(const TransactionContext& tx);
    
    // Pattern-based detection using ML
    static MEVThreat detect_using_patterns(const TransactionContext& tx,
                                         const std::vector<std::string>& learned_patterns);
    
    // Cross-chain MEV detection
    static std::vector<MEVThreat> detect_cross_chain_mev(const std::vector<TransactionContext>& transactions);
};

/**
 * @brief MEV Protection Strategies Implementation
 */
class MEVProtectionStrategies {
public:
    // Bundle-based protection
    static ProtectionResult apply_bundle_protection(const TransactionContext& tx,
                                                   const BundleConfig& config);
    
    // Private mempool protection
    static ProtectionResult apply_private_mempool_protection(const TransactionContext& tx,
                                                           const std::vector<std::string>& mempool_urls);
    
    // Timing randomization protection
    static ProtectionResult apply_timing_randomization(const TransactionContext& tx,
                                                     std::chrono::milliseconds max_delay);
    
    // Stealth mode protection
    static ProtectionResult apply_stealth_protection(const TransactionContext& tx);
    
    // Batch auction protection
    static ProtectionResult apply_batch_auction_protection(const std::vector<TransactionContext>& transactions);
};

/**
 * @brief Factory for creating MEV protection engines with preset configurations
 */
class MEVEngineFactory {
public:
    // Preset configurations
    static MEVEngineConfig create_ethereum_config();
    static MEVEngineConfig create_solana_config();
    static MEVEngineConfig create_high_frequency_config();
    static MEVEngineConfig create_conservative_config();
    static MEVEngineConfig create_aggressive_config();
    
    // Custom configurations
    static MEVEngineConfig create_custom_config(ProtectionLevel default_level,
                                               const std::vector<ProtectionStrategy>& strategies,
                                               double max_cost_usd);
};

} // namespace hfx::mev
