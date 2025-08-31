/**
 * @file risk_manager.hpp
 * @brief AI-powered risk management system with real-time circuit breakers
 * @version 1.0.0
 * 
 * Advanced risk management incorporating 2024-2025 techniques:
 * - Real-time VAR calculation using Monte Carlo and historical simulation
 * - ML-based anomaly detection with staged sliding window Transformers
 * - Dynamic position sizing using Kelly Criterion optimization
 * - Multi-dimensional circuit breakers (price, volume, correlation, volatility)
 * - Integration with RiskFolio-Lib for portfolio optimization
 */

#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "strategy_engine.hpp"

#ifdef __APPLE__
// MetalPerformanceShaders conflicts with -fno-rtti flag
// For HFT, we use optimized C++ implementations instead
#include <Accelerate/Accelerate.h>
#endif

namespace hfx::risk {

using TimeStamp = std::chrono::time_point<std::chrono::high_resolution_clock>;
using Price = double;
using Volume = double;
using RiskId = std::uint64_t;

/**
 * @enum RiskLevel
 * @brief Risk severity levels
 */
enum class RiskLevel : std::uint8_t {
    LOW = 1,
    MEDIUM = 2,
    HIGH = 3,
    CRITICAL = 4,
    EMERGENCY = 5
};

/**
 * @enum CircuitBreakerType
 * @brief Types of circuit breakers
 */
enum class CircuitBreakerType : std::uint8_t {
    PRICE_MOVEMENT,        // Extreme price moves
    VOLUME_SPIKE,          // Unusual volume activity
    CORRELATION_BREAK,     // Correlation breakdown
    VOLATILITY_SURGE,      // Volatility explosion
    DRAWDOWN_LIMIT,        // Maximum drawdown reached
    POSITION_CONCENTRATION, // Position size limits
    VAR_BREACH,            // VaR limit exceeded
    LIQUIDITY_CRISIS,      // Market liquidity dried up
    GAS_PRICE_SPIKE,       // Ethereum gas price surge
    ORACLE_FAILURE         // Price oracle malfunction
};

/**
 * @struct RiskMetrics
 * @brief Comprehensive risk metrics
 */
struct alignas(64) RiskMetrics {
    // Portfolio level metrics
    double portfolio_var_1d;           // 1-day VaR at 95% confidence
    double portfolio_cvar_1d;          // 1-day CVaR (Expected Shortfall)
    double portfolio_value;            // Current portfolio value
    double unrealized_pnl;             // Mark-to-market P&L
    double realized_pnl_today;         // Realized P&L today
    
    // Risk ratios
    double sharpe_ratio;               // Risk-adjusted returns
    double sortino_ratio;              // Downside risk-adjusted returns
    double calmar_ratio;               // Return to max drawdown ratio
    double max_drawdown;               // Maximum drawdown
    double current_drawdown;           // Current drawdown level
    
    // Volatility metrics
    double portfolio_volatility;       // Annualized volatility
    double garch_volatility;           // GARCH(1,1) forecast
    double realized_volatility_10d;    // 10-day realized vol
    
    // Concentration metrics  
    double max_position_weight;        // Largest position as % of portfolio
    double herfindahl_index;           // Portfolio concentration index
    std::uint32_t num_positions;       // Number of active positions
    
    // Liquidity metrics
    double avg_bid_ask_spread;         // Average spread across positions
    double market_impact_cost;         // Estimated liquidation cost
    double days_to_liquidate;          // Time to liquidate at 10% volume
    
    // System metrics
    TimeStamp last_update;
    std::uint64_t calculation_time_ns; // Time to compute metrics
    
    RiskMetrics() = default;
} __attribute__((packed));

/**
 * @struct CircuitBreakerConfig
 * @brief Configuration for circuit breakers
 */
struct CircuitBreakerConfig {
    CircuitBreakerType type;
    bool enabled{true};
    double threshold;                   // Trigger threshold
    std::chrono::milliseconds cooldown{60000};  // Cooldown period
    std::uint32_t max_triggers_per_hour{10};    // Rate limiting
    bool auto_resume{true};             // Auto-resume after cooldown
    std::string description;
    
    CircuitBreakerConfig() = default;
    CircuitBreakerConfig(CircuitBreakerType t, double thresh, const std::string& desc)
        : type(t), threshold(thresh), description(desc) {}
};

/**
 * @struct PositionLimit
 * @brief Position sizing limits
 */
struct PositionLimit {
    std::string asset;
    double max_notional{1000000.0};     // Maximum $1M position
    double max_weight{0.1};             // Maximum 10% of portfolio
    double max_leverage{2.0};           // Maximum 2x leverage
    double stop_loss_pct{0.05};         // 5% stop loss
    bool enabled{true};
    
    PositionLimit() = default;
    PositionLimit(const std::string& a, double notional, double weight)
        : asset(a), max_notional(notional), max_weight(weight) {}
};

/**
 * @class RiskManager
 * @brief Advanced ML-powered risk management system
 * 
 * Features:
 * - Real-time VaR calculation using multiple methodologies
 * - AI-based anomaly detection for market regime changes
 * - Dynamic circuit breakers with ML-enhanced thresholds
 * - Integration with quantitative risk libraries (RiskFolio-Lib, empyrical)
 * - Apple Neural Engine acceleration for risk model inference
 * - Multi-threaded portfolio analytics with lock-free updates
 */
class RiskManager {
public:
    using RiskAlertCallback = std::function<void(RiskLevel, const std::string&)>;
    using CircuitBreakerCallback = std::function<void(CircuitBreakerType, bool)>;  // type, triggered
    using PositionUpdateCallback = std::function<void(const std::string&, double, double)>; // asset, size, pnl

    RiskManager();
    ~RiskManager();

    // Non-copyable, non-movable
    RiskManager(const RiskManager&) = delete;
    RiskManager& operator=(const RiskManager&) = delete;
    RiskManager(RiskManager&&) = delete;
    RiskManager& operator=(RiskManager&&) = delete;

    /**
     * @brief Initialize the risk management system
     * @return true on success
     */
    bool initialize();

    /**
     * @brief Shutdown and cleanup resources
     */
    void shutdown();

    /**
     * @brief Validate a trading signal against risk limits
     * @param signal Proposed trading signal
     * @return true if signal passes risk checks
     */
    bool validate_signal(const strat::TradingSignal& signal);

    /**
     * @brief Update portfolio positions
     * @param asset Asset identifier
     * @param quantity Position quantity (signed)
     * @param avg_price Average entry price
     */
    void update_position(const std::string& asset, double quantity, double avg_price);

    /**
     * @brief Update market data for risk calculations
     * @param asset Asset identifier  
     * @param price Current market price
     * @param timestamp Update timestamp
     */
    void update_market_price(const std::string& asset, double price, TimeStamp timestamp);

    /**
     * @brief Force recalculation of all risk metrics
     */
    void recalculate_risk_metrics();

    /**
     * @brief Get current portfolio risk metrics
     * @return Current risk metrics
     */
    RiskMetrics get_risk_metrics() const;

    /**
     * @brief Configure circuit breaker parameters
     * @param config Circuit breaker configuration
     */
    void configure_circuit_breaker(const CircuitBreakerConfig& config);

    /**
     * @brief Set position limits for an asset
     * @param limit Position limit configuration
     */
    void set_position_limit(const PositionLimit& limit);

    /**
     * @brief Check if trading is currently allowed
     * @return true if trading allowed, false if halted
     */
    bool is_trading_allowed() const {
        return trading_allowed_.load(std::memory_order_acquire);
    }

    /**
     * @brief Set callback for risk alerts
     * @param callback Function to call on risk events
     */
    void set_risk_alert_callback(RiskAlertCallback callback) {
        risk_alert_callback_ = std::move(callback);
    }

    /**
     * @brief Set callback for circuit breaker events
     * @param callback Function to call on circuit breaker triggers
     */
    void set_circuit_breaker_callback(CircuitBreakerCallback callback) {
        circuit_breaker_callback_ = std::move(callback);
    }

    /**
     * @brief Set callback for hedge execution requests
     * @param callback Function to call for hedge orders
     */
    void set_hedge_callback(std::function<bool(const strat::TradingSignal&)> callback) {
        hedge_callback_ = std::move(callback);
    }

    /**
     * @brief Get circuit breaker status
     */
    struct CircuitBreakerStatus {
        CircuitBreakerType type;
        bool triggered;
        TimeStamp trigger_time;
        TimeStamp resume_time;
        std::uint32_t trigger_count_today;
        std::string reason;
    };

    std::vector<CircuitBreakerStatus> get_circuit_breaker_status() const;

    /**
     * @brief Get risk management statistics
     */
    struct RiskStatistics {
        std::uint64_t signals_validated{0};
        std::uint64_t signals_rejected{0};
        std::uint64_t circuit_breaker_triggers{0};
        std::uint64_t risk_alerts_generated{0};
        double avg_var_calculation_time_ns{0.0};
        double avg_signal_validation_time_ns{0.0};
        std::uint32_t positions_tracked{0};
    };

    RiskStatistics get_statistics() const;

    /**
     * @brief Check if engine is running
     */
    bool is_running() const noexcept {
        return running_.load(std::memory_order_acquire);
    }

private:
    class VarCalculator;         // Forward declaration
    class AnomalyDetector;       // Forward declaration
    class CircuitBreakerSystem;  // Forward declaration
    class PortfolioAnalytics;    // Forward declaration

    std::atomic<bool> running_{false};
    std::atomic<bool> trading_allowed_{true};

    // Core risk systems
    std::unique_ptr<VarCalculator> var_calculator_;
    std::unique_ptr<AnomalyDetector> anomaly_detector_;
    std::unique_ptr<CircuitBreakerSystem> circuit_breaker_system_;
    std::unique_ptr<PortfolioAnalytics> portfolio_analytics_;

    // Portfolio state
    std::unordered_map<std::string, double> positions_;      // asset -> quantity
    std::unordered_map<std::string, double> avg_prices_;     // asset -> avg price
    std::unordered_map<std::string, double> market_prices_;  // asset -> current price
    std::unordered_map<std::string, PositionLimit> position_limits_;

    // Risk metrics cache
    mutable RiskMetrics cached_metrics_;
    mutable std::atomic<TimeStamp> last_metrics_update_{TimeStamp{}};

    // Callbacks
    RiskAlertCallback risk_alert_callback_;
    CircuitBreakerCallback circuit_breaker_callback_;
    std::function<bool(const strat::TradingSignal&)> hedge_callback_;

    // Apple-specific ML acceleration
#ifdef __APPLE__
    std::unique_ptr<class AppleRiskMLAccelerator> ml_accelerator_;
#endif

    // Statistics
    mutable std::atomic<std::uint64_t> signals_validated_{0};
    mutable std::atomic<std::uint64_t> signals_rejected_{0};
    mutable std::atomic<std::uint64_t> circuit_breaker_triggers_{0};
    mutable std::atomic<std::uint64_t> risk_alerts_generated_{0};
    mutable std::atomic<std::uint64_t> total_var_calc_time_ns_{0};
    mutable std::atomic<std::uint64_t> total_validation_time_ns_{0};

    /**
     * @brief Initialize Apple-specific ML acceleration
     */
    bool initialize_apple_ml();

    /**
     * @brief Initialize Python quantitative risk libraries
     */
    bool initialize_risk_libraries();

    /**
     * @brief Calculate position-level risk metrics
     * @param asset Asset to analyze
     * @return Risk contribution of this position
     */
    double calculate_position_risk(const std::string& asset) const;

    /**
     * @brief Check if signal violates position limits
     * @param signal Trading signal to validate
     * @return true if within limits
     */
    bool check_position_limits(const strat::TradingSignal& signal) const;

    /**
     * @brief Check if signal passes correlation limits
     * @param signal Trading signal to validate  
     * @return true if correlations acceptable
     */
    bool check_correlation_limits(const strat::TradingSignal& signal) const;

    /**
     * @brief Calculate Kelly Criterion optimal position size
     * @param signal Trading signal with expected returns
     * @return Optimal position size as fraction of portfolio
     */
    double calculate_kelly_position_size(const strat::TradingSignal& signal) const;

    /**
     * @brief Trigger risk alert
     * @param level Alert severity
     * @param message Alert message
     */
    void trigger_risk_alert(RiskLevel level, const std::string& message);

    /**
     * @brief Update performance statistics
     * @param validation_time_ns Time taken for signal validation
     */
    void update_statistics(std::uint64_t validation_time_ns);
};

/**
 * @brief Default risk management configuration
 */
struct DefaultRiskConfig {
    // Portfolio level limits
    static constexpr double MAX_PORTFOLIO_VAR = 50000.0;      // $50k daily VaR
    static constexpr double MAX_PORTFOLIO_LEVERAGE = 3.0;      // 3x max leverage
    static constexpr double MAX_DRAWDOWN = 0.15;               // 15% max drawdown
    static constexpr double MIN_LIQUIDITY_RATIO = 0.1;        // 10% min cash
    
    // Position level limits
    static constexpr double MAX_SINGLE_POSITION = 0.2;        // 20% max single position
    static constexpr double MAX_SECTOR_EXPOSURE = 0.4;        // 40% max sector exposure
    static constexpr double DEFAULT_STOP_LOSS = 0.05;         // 5% stop loss
    
    // Circuit breaker thresholds
    static constexpr double PRICE_CIRCUIT_BREAKER = 0.10;     // 10% price move
    static constexpr double VOLUME_CIRCUIT_BREAKER = 5.0;     // 5x normal volume
    static constexpr double VOLATILITY_CIRCUIT_BREAKER = 3.0; // 3x normal volatility
    static constexpr double GAS_PRICE_CIRCUIT_BREAKER = 200.0; // 200 gwei gas price
};

} // namespace hfx::risk
