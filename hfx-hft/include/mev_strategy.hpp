#pragma once

#include <string>
#include <vector>
#include <memory>
#include <atomic>
#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <chrono>
#include <cstdint>

namespace hfx::hft {

// Forward declarations
struct MemecoinTradeParams;
struct MemecoinTradeResult;
class UltraFastExecutionEngine;

/**
 * @brief MEV attack types
 */
enum class MEVAttackType {
    FRONTRUN,
    BACKRUN,
    SANDWICH,
    JIT_LIQUIDITY,
    TOXIC_ARBITRAGE,
    TIME_BANDIT,
    UNKNOWN
};

/**
 * @brief MEV protection strategy
 */
enum class MEVProtectionStrategy {
    PRIVATE_MEMPOOL,
    JITO_BUNDLE,
    FLASHBOTS_PROTECT,
    BATCH_AUCTION,
    RANDOMIZED_DELAY,
    DARK_POOL
};

/**
 * @brief MEV detection result
 */
struct MEVDetectionResult {
    bool is_mev_detected;
    MEVAttackType attack_type;
    double confidence_score;
    std::string threat_description;
    uint64_t detection_timestamp_ns;
    std::vector<std::string> suspicious_patterns;
};

/**
 * @brief MEV protection result
 */
struct MEVProtectionResult {
    bool protection_applied;
    MEVProtectionStrategy strategy_used;
    double protection_cost_basis_points;
    uint64_t protection_latency_ns;
    std::string protection_details;
};

/**
 * @brief MEV engine configuration
 */
struct MEVEngineConfig {
    bool enable_detection = true;
    bool enable_protection = true;
    double detection_threshold = 0.7;
    uint64_t max_protection_latency_ns = 50000; // 50μs
    std::vector<MEVProtectionStrategy> preferred_strategies;
    uint32_t max_jito_bundle_size = 5;
    double max_protection_cost_bps = 25.0;
};

/**
 * @brief MEV metrics for monitoring
 */
struct MEVMetrics {
    std::atomic<uint64_t> total_detections{0};
    std::atomic<uint64_t> attacks_prevented{0};
    std::atomic<uint64_t> false_positives{0};
    std::atomic<uint64_t> protection_failures{0};
    std::atomic<double> avg_protection_cost_bps{0.0};
    std::atomic<uint64_t> avg_protection_latency_ns{0};
};

/**
 * @brief High-performance MEV protection engine
 */
class MEVProtectionEngine {
public:
    /**
     * @brief Constructor
     */
    explicit MEVProtectionEngine(const MEVEngineConfig& config = {});
    
    /**
     * @brief Destructor
     */
    ~MEVProtectionEngine();

    /**
     * @brief Initialize the MEV protection engine
     */
    bool initialize();

    /**
     * @brief Shutdown the engine
     */
    void shutdown();

    /**
     * @brief Detect MEV attacks in trade parameters
     */
    MEVDetectionResult detect_mev_attack(const MemecoinTradeParams& params);

    /**
     * @brief Apply MEV protection to a trade
     */
    MEVProtectionResult apply_protection(
        const MemecoinTradeParams& params,
        const MEVDetectionResult& detection_result
    );

    /**
     * @brief Check if Jito bundle is available for protection
     */
    bool is_jito_bundle_available(const std::string& token_symbol);

    /**
     * @brief Get current MEV metrics
     */
    void get_metrics(MEVMetrics& metrics_out) const;

    /**
     * @brief Reset metrics
     */
    void reset_metrics();

    /**
     * @brief Update engine configuration
     */
    void update_config(const MEVEngineConfig& new_config);

    /**
     * @brief Add suspicious transaction pattern
     */
    void add_suspicious_pattern(const std::string& pattern);

    /**
     * @brief Check if engine is running
     */
    bool is_running() const;

private:
    struct Impl;
    std::unique_ptr<Impl> pimpl_;
};

/**
 * @brief MEV-aware order router
 */
class MEVAwareOrderRouter {
public:
    explicit MEVAwareOrderRouter(
        std::shared_ptr<UltraFastExecutionEngine> execution_engine,
        std::shared_ptr<MEVProtectionEngine> mev_engine
    );
    
    ~MEVAwareOrderRouter();

    /**
     * @brief Route order with MEV protection
     */
    MemecoinTradeResult route_order_with_protection(const MemecoinTradeParams& params);

    /**
     * @brief Set MEV protection callback
     */
    void set_mev_callback(std::function<void(const MEVDetectionResult&)> callback);

private:
    struct Impl;
    std::unique_ptr<Impl> pimpl_;
};

} // namespace hfx::hft
