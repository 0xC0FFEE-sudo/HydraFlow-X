/**
 * @file policy_engine.hpp
 * @brief Immutable policy-as-code risk engine for HFT systems
 * @version 1.0.0
 * 
 * Hard-coded risk controls that AI models cannot override
 * Microsecond policy evaluation with full audit trails
 */

#pragma once

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <atomic>
#include <chrono>
#include <functional>
#include <array>

#include "execution_engine.hpp"
#include "signal_compressor.hpp"

namespace hfx::hft {

/**
 * @brief Policy violation severity levels
 */
enum class ViolationSeverity : uint8_t {
    INFO = 0,       // Informational, allow execution
    WARNING = 1,    // Warning, log but allow
    ERROR = 2,      // Error, block execution
    CRITICAL = 3    // Critical, emergency stop
};

/**
 * @brief Fast policy evaluation result
 */
struct alignas(32) PolicyResult {
    bool allowed;                    // Can the order proceed?
    ViolationSeverity severity;      // Highest violation severity
    uint16_t violated_policy_count;  // Number of policies violated
    uint32_t primary_violation_id;   // Most important violation
    
    char violation_reason[64];       // Human-readable reason (fixed size)
    uint64_t evaluation_time_ns;     // Time taken to evaluate
    uint32_t evaluated_policy_count; // Total policies checked
    uint32_t checksum;               // Integrity check
    
    // Helper methods
    bool is_critical() const noexcept { return severity == ViolationSeverity::CRITICAL; }
    bool requires_escalation() const noexcept { return severity >= ViolationSeverity::ERROR; }
    
    void set_violation(uint32_t policy_id, ViolationSeverity sev, const char* reason) noexcept {
        if (sev > severity) {
            severity = sev;
            primary_violation_id = policy_id;
            strncpy(violation_reason, reason, sizeof(violation_reason) - 1);
            violation_reason[sizeof(violation_reason) - 1] = '\0';
        }
        violated_policy_count++;
        allowed = (severity < ViolationSeverity::ERROR);
    }
} __attribute__((packed));

/**
 * @brief Market context for policy evaluation
 */
struct MarketContext {
    std::string symbol;
    double current_price;
    double reference_price;        // VWAP, last close, etc.
    double bid_ask_spread;
    double volume_24h;
    double volatility_1h;
    double liquidity_score;       // 0.0 - 1.0
    uint64_t timestamp_ns;
    
    // Market regime indicators
    bool is_market_open;
    bool is_news_blackout_period;
    bool is_high_volatility_period;
    bool is_low_liquidity_period;
    
    // Risk indicators
    double var_estimate;          // Value at Risk
    double correlation_to_market; // Beta-like measure
    bool circuit_breaker_active;
};

/**
 * @brief Order details for policy evaluation
 */
struct OrderDetails {
    std::string symbol;
    double quantity;              // Positive = buy, negative = sell
    double price;                 // 0.0 for market orders
    double max_slippage_percent;
    uint64_t timestamp_ns;
    
    // Order metadata
    std::string order_type;       // "MARKET", "LIMIT", "STOP"
    std::string time_in_force;    // "IOC", "FOK", "GTC"
    bool is_urgent;               // Emergency/critical order
    uint32_t client_order_id;
    
    // Signal context
    CompactSignal originating_signal;
    double signal_confidence;
    std::string signal_source;
};

/**
 * @brief Portfolio state for risk calculations
 */
struct PortfolioState {
    double total_capital;
    double available_capital;
    double used_margin;
    double unrealized_pnl;
    double realized_pnl_today;
    
    // Position data
    std::unordered_map<std::string, double> positions; // symbol -> quantity
    std::unordered_map<std::string, double> exposures;  // symbol -> notional value
    
    // Risk metrics
    double portfolio_var;         // Value at Risk
    double beta_to_market;        // Market correlation
    double concentration_risk;    // Largest position as % of capital
    double leverage_ratio;
    
    // Counters
    uint32_t trades_today;
    uint32_t failed_trades_today;
    uint64_t last_trade_timestamp_ns;
};

/**
 * @brief Base policy interface
 */
class IPolicy {
public:
    virtual ~IPolicy() = default;
    
    virtual uint32_t get_policy_id() const = 0;
    virtual std::string get_policy_name() const = 0;
    virtual ViolationSeverity get_default_severity() const = 0;
    
    // Fast path evaluation (microsecond target)
    virtual bool evaluate(const OrderDetails& order,
                         const MarketContext& market,
                         const PortfolioState& portfolio,
                         PolicyResult& result) = 0;
    
    // Policy configuration
    virtual void update_parameters(const std::unordered_map<std::string, double>& params) = 0;
    virtual std::unordered_map<std::string, double> get_parameters() const = 0;
    
    // Audit and compliance
    virtual std::string get_policy_description() const = 0;
    virtual std::string get_last_violation_details() const = 0;
};

/**
 * @brief Position size limit policy
 */
class PositionSizePolicy : public IPolicy {
public:
    struct Config {
        double max_position_percent = 10.0;    // % of total capital
        double max_single_order_percent = 2.0; // % of total capital per order
        double max_symbol_exposure = 15.0;     // % of capital in one symbol
        bool enforce_per_symbol_limits = true;
    };
    
    explicit PositionSizePolicy(const Config& config);
    
    uint32_t get_policy_id() const override { return 1001; }
    std::string get_policy_name() const override { return "PositionSizePolicy"; }
    ViolationSeverity get_default_severity() const override { return ViolationSeverity::ERROR; }
    
    bool evaluate(const OrderDetails& order, const MarketContext& market,
                 const PortfolioState& portfolio, PolicyResult& result) override;
    
    void update_parameters(const std::unordered_map<std::string, double>& params) override;
    std::unordered_map<std::string, double> get_parameters() const override;
    
    std::string get_policy_description() const override;
    std::string get_last_violation_details() const override;
    
private:
    Config config_;
    std::string last_violation_;
};

/**
 * @brief Price deviation policy (fat finger protection)
 */
class PriceDeviationPolicy : public IPolicy {
public:
    struct Config {
        double max_deviation_percent = 5.0;    // Max % deviation from reference
        double volatility_multiplier = 3.0;    // Allow larger deviations in volatile markets
        bool use_dynamic_thresholds = true;    // Adjust based on market conditions
        std::string reference_price_type = "VWAP"; // "LAST", "VWAP", "MID"
    };
    
    explicit PriceDeviationPolicy(const Config& config);
    
    uint32_t get_policy_id() const override { return 1002; }
    std::string get_policy_name() const override { return "PriceDeviationPolicy"; }
    ViolationSeverity get_default_severity() const override { return ViolationSeverity::ERROR; }
    
    bool evaluate(const OrderDetails& order, const MarketContext& market,
                 const PortfolioState& portfolio, PolicyResult& result) override;
    
    void update_parameters(const std::unordered_map<std::string, double>& params) override;
    std::unordered_map<std::string, double> get_parameters() const override;
    
    std::string get_policy_description() const override;
    std::string get_last_violation_details() const override;
    
private:
    Config config_;
    std::string last_violation_;
    
    double calculate_reference_price(const MarketContext& market) const;
    double calculate_max_deviation(const MarketContext& market) const;
};

/**
 * @brief Trading frequency limits policy
 */
class TradingFrequencyPolicy : public IPolicy {
public:
    struct Config {
        uint32_t max_orders_per_second = 100;
        uint32_t max_orders_per_minute = 1000;
        uint32_t max_orders_per_symbol_per_minute = 50;
        uint32_t max_daily_trades = 10000;
        bool enforce_cooling_period = true;
        uint64_t min_time_between_orders_ns = 1000000; // 1ms
    };
    
    explicit TradingFrequencyPolicy(const Config& config);
    
    uint32_t get_policy_id() const override { return 1003; }
    std::string get_policy_name() const override { return "TradingFrequencyPolicy"; }
    ViolationSeverity get_default_severity() const override { return ViolationSeverity::WARNING; }
    
    bool evaluate(const OrderDetails& order, const MarketContext& market,
                 const PortfolioState& portfolio, PolicyResult& result) override;
    
    void update_parameters(const std::unordered_map<std::string, double>& params) override;
    std::unordered_map<std::string, double> get_parameters() const override;
    
    std::string get_policy_description() const override;
    std::string get_last_violation_details() const override;
    
private:
    Config config_;
    std::string last_violation_;
    
    // Rate tracking
    struct RateTracker {
        std::array<uint64_t, 60> second_buckets{};  // Last 60 seconds
        std::array<uint64_t, 60> minute_buckets{};  // Last 60 minutes
        uint64_t current_second = 0;
        uint64_t current_minute = 0;
        uint32_t daily_count = 0;
        uint64_t last_order_time = 0;
    };
    
    std::unordered_map<std::string, RateTracker> symbol_trackers_;
    RateTracker global_tracker_;
    
    void update_rate_tracker(RateTracker& tracker, uint64_t timestamp_ns);
    uint32_t get_current_rate(const RateTracker& tracker, uint32_t window_seconds) const;
};

/**
 * @brief Risk limits policy
 */
class RiskLimitsPolicy : public IPolicy {
public:
    struct Config {
        double max_portfolio_var_percent = 3.0;     // Max portfolio VaR
        double max_daily_loss_percent = 5.0;        // Max daily loss
        double max_drawdown_percent = 10.0;         // Max drawdown from peak
        double max_leverage_ratio = 3.0;            // Max leverage
        double max_concentration_percent = 20.0;    // Max single position
        bool enforce_correlation_limits = true;     // Check portfolio correlation
    };
    
    explicit RiskLimitsPolicy(const Config& config);
    
    uint32_t get_policy_id() const override { return 1004; }
    std::string get_policy_name() const override { return "RiskLimitsPolicy"; }
    ViolationSeverity get_default_severity() const override { return ViolationSeverity::CRITICAL; }
    
    bool evaluate(const OrderDetails& order, const MarketContext& market,
                 const PortfolioState& portfolio, PolicyResult& result) override;
    
    void update_parameters(const std::unordered_map<std::string, double>& params) override;
    std::unordered_map<std::string, double> get_parameters() const override;
    
    std::string get_policy_description() const override;
    std::string get_last_violation_details() const override;
    
private:
    Config config_;
    std::string last_violation_;
    
    double calculate_position_var(const OrderDetails& order, const MarketContext& market) const;
    double calculate_portfolio_var_impact(const OrderDetails& order, 
                                        const PortfolioState& portfolio) const;
};

/**
 * @brief Market conditions policy
 */
class MarketConditionsPolicy : public IPolicy {
public:
    struct Config {
        bool block_during_news_blackout = true;
        bool block_during_circuit_breakers = true;
        bool block_during_low_liquidity = true;
        double min_liquidity_score = 0.3;
        double max_volatility_threshold = 50.0;     // % volatility
        bool allow_emergency_orders = true;
        std::vector<std::string> restricted_symbols;
    };
    
    explicit MarketConditionsPolicy(const Config& config);
    
    uint32_t get_policy_id() const override { return 1005; }
    std::string get_policy_name() const override { return "MarketConditionsPolicy"; }
    ViolationSeverity get_default_severity() const override { return ViolationSeverity::WARNING; }
    
    bool evaluate(const OrderDetails& order, const MarketContext& market,
                 const PortfolioState& portfolio, PolicyResult& result) override;
    
    void update_parameters(const std::unordered_map<std::string, double>& params) override;
    std::unordered_map<std::string, double> get_parameters() const override;
    
    std::string get_policy_description() const override;
    std::string get_last_violation_details() const override;
    
private:
    Config config_;
    std::string last_violation_;
    
    bool is_symbol_restricted(const std::string& symbol) const;
};

/**
 * @brief Ultra-fast policy evaluation engine
 */
class PolicyEngine {
public:
    struct EngineConfig {
        bool enable_parallel_evaluation = true;
        bool enable_early_termination = true;     // Stop on first critical violation
        bool enable_policy_caching = true;        // Cache policy results
        uint64_t max_evaluation_time_ns = 100000; // 100μs timeout
        size_t max_concurrent_evaluations = 1000;
    };
    
    explicit PolicyEngine(const EngineConfig& config);
    ~PolicyEngine();
    
    // Policy management
    void add_policy(std::unique_ptr<IPolicy> policy);
    void remove_policy(uint32_t policy_id);
    void enable_policy(uint32_t policy_id, bool enabled);
    
    // Fast path evaluation
    PolicyResult evaluate_order(const OrderDetails& order,
                               const MarketContext& market,
                               const PortfolioState& portfolio);
    
    // Batch evaluation
    size_t evaluate_orders(const OrderDetails* orders, size_t count,
                          const MarketContext& market,
                          const PortfolioState& portfolio,
                          PolicyResult* results);
    
    // Policy configuration
    void update_policy_parameters(uint32_t policy_id, 
                                 const std::unordered_map<std::string, double>& params);
    
    // Emergency controls
    void emergency_stop_all();
    void reset_emergency_stop();
    bool is_emergency_stopped() const;
    
    // Audit and logging
    struct AuditEntry {
        uint64_t timestamp_ns;
        uint32_t order_id;
        std::string symbol;
        PolicyResult result;
        std::vector<uint32_t> evaluated_policies;
    };
    
    void enable_audit_logging(bool enabled);
    std::vector<AuditEntry> get_audit_trail(uint64_t since_timestamp_ns) const;
    void clear_audit_trail();
    
    // Performance metrics
    struct PolicyMetrics {
        std::atomic<uint64_t> evaluations_total{0};
        std::atomic<uint64_t> evaluations_passed{0};
        std::atomic<uint64_t> evaluations_failed{0};
        std::atomic<uint64_t> avg_evaluation_time_ns{0};
        std::atomic<uint64_t> max_evaluation_time_ns{0};
        std::atomic<uint64_t> timeout_count{0};
        std::atomic<uint64_t> emergency_stops{0};
    };
    
    const PolicyMetrics& get_metrics() const;
    void reset_metrics();
    
    // Policy statistics per policy ID
    struct PolicyStats {
        uint64_t evaluations;
        uint64_t violations;
        uint64_t avg_time_ns;
        ViolationSeverity max_severity;
    };
    
    std::unordered_map<uint32_t, PolicyStats> get_policy_statistics() const;
    
private:
    class PolicyEngineImpl;
    std::unique_ptr<PolicyEngineImpl> pimpl_;
};

/**
 * @brief Policy configuration manager
 */
class PolicyConfigManager {
public:
    explicit PolicyConfigManager();
    ~PolicyConfigManager();
    
    // Configuration loading
    bool load_config_from_file(const std::string& config_path);
    bool load_config_from_string(const std::string& json_config);
    
    // Configuration validation
    bool validate_config() const;
    std::vector<std::string> get_validation_errors() const;
    
    // Policy factory
    std::unique_ptr<IPolicy> create_policy(const std::string& policy_type, 
                                          const std::unordered_map<std::string, double>& params);
    
    // Configuration management
    void save_config_to_file(const std::string& config_path) const;
    std::string export_config_to_string() const;
    
    // Hot reloading
    void enable_hot_reload(const std::string& config_path);
    void disable_hot_reload();
    
    // Change tracking
    using ConfigChangeCallback = std::function<void(const std::string& policy_type, 
                                                   const std::unordered_map<std::string, double>& new_params)>;
    void set_config_change_callback(ConfigChangeCallback callback);
    
private:
    class ConfigManagerImpl;
    std::unique_ptr<ConfigManagerImpl> pimpl_;
};

} // namespace hfx::hft
