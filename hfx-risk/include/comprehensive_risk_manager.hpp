/**
 * @file comprehensive_risk_manager.hpp
 * @brief Advanced risk management system with comprehensive controls
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
#include <deque>

namespace hfx::risk {

// Risk levels
enum class RiskLevel {
    LOW,
    MODERATE,
    HIGH,
    CRITICAL,
    EMERGENCY
};

// Circuit breaker types
enum class CircuitBreakerType {
    PORTFOLIO_DRAWDOWN,    // Portfolio maximum drawdown
    DAILY_LOSS,           // Daily loss limit
    POSITION_SIZE,        // Individual position size
    VOLATILITY,           // Market volatility spike
    CORRELATION,          // Correlation breakdown
    LIQUIDITY,            // Liquidity crisis
    CONCENTRATION,        // Concentration risk
    LEVERAGE,             // Leverage limits
    MARGIN_CALL           // Margin requirements
};

// Position information
struct Position {
    std::string symbol;
    double quantity;           // Positive for long, negative for short
    double average_price;      // Average entry price
    double current_price;      // Current market price
    double unrealized_pnl;     // Current P&L
    double realized_pnl;       // Realized P&L from trades
    double market_value;       // Current market value
    std::chrono::system_clock::time_point entry_time;
    std::chrono::system_clock::time_point last_update;
    
    // Risk metrics per position
    double var_contribution;   // VaR contribution to portfolio
    double beta;              // Beta to market
    double volatility;        // Historical volatility
    double maximum_loss;      // Maximum observed loss
    
    Position() : quantity(0), average_price(0), current_price(0), 
                unrealized_pnl(0), realized_pnl(0), market_value(0),
                var_contribution(0), beta(0), volatility(0), maximum_loss(0) {}
};

// Risk metrics
struct RiskMetrics {
    // Portfolio level metrics
    double total_value;
    double total_pnl;
    double daily_pnl;
    double unrealized_pnl;
    double realized_pnl;
    
    // Risk measures
    double portfolio_var_95;      // 95% Value at Risk
    double portfolio_cvar_95;     // 95% Conditional Value at Risk
    double portfolio_var_99;      // 99% Value at Risk
    double portfolio_cvar_99;     // 99% Conditional Value at Risk
    double maximum_drawdown;      // Maximum observed drawdown
    double current_drawdown;      // Current drawdown from peak
    
    // Performance metrics
    double sharpe_ratio;          // Risk-adjusted return
    double sortino_ratio;         // Downside-adjusted return
    double calmar_ratio;          // Return/maximum drawdown
    double information_ratio;     // Active return/tracking error
    double total_return;          // Total return since inception
    double annualized_return;     // Annualized return
    double annualized_volatility; // Annualized volatility
    
    // Concentration metrics
    double largest_position_pct;  // Largest position as % of portfolio
    double top_5_positions_pct;   // Top 5 positions as % of portfolio
    double sector_concentration;  // Maximum sector concentration
    double correlation_risk;      // Portfolio correlation risk
    
    // Leverage and margin
    double gross_exposure;        // Total gross exposure
    double net_exposure;          // Net exposure
    double leverage_ratio;        // Leverage ratio
    double margin_utilization;    // Margin utilization percentage
    
    // Timing
    std::chrono::system_clock::time_point last_update;
    std::chrono::system_clock::time_point calculation_time;
    
    RiskMetrics() : total_value(0), total_pnl(0), daily_pnl(0), unrealized_pnl(0), realized_pnl(0),
                   portfolio_var_95(0), portfolio_cvar_95(0), portfolio_var_99(0), portfolio_cvar_99(0),
                   maximum_drawdown(0), current_drawdown(0), sharpe_ratio(0), sortino_ratio(0), 
                   calmar_ratio(0), information_ratio(0), total_return(0), annualized_return(0),
                   annualized_volatility(0), largest_position_pct(0), top_5_positions_pct(0),
                   sector_concentration(0), correlation_risk(0), gross_exposure(0), net_exposure(0),
                   leverage_ratio(0), margin_utilization(0) {}
};

// Risk limits configuration
struct RiskLimits {
    // Portfolio limits
    double max_portfolio_value = 10000000.0;      // $10M max portfolio
    double max_daily_loss = 100000.0;             // $100K daily loss limit
    double max_drawdown_pct = 20.0;               // 20% maximum drawdown
    double max_position_size_usd = 500000.0;      // $500K max single position
    double max_position_size_pct = 10.0;          // 10% max position as % of portfolio
    
    // Risk metrics limits
    double max_portfolio_var = 50000.0;           // $50K daily VaR limit
    double max_leverage_ratio = 3.0;              // 3:1 leverage limit
    double max_concentration_pct = 25.0;          // 25% max sector concentration
    double max_correlation_risk = 0.8;            // Max correlation risk score
    
    // Performance limits
    double min_sharpe_ratio = -0.5;               // Minimum acceptable Sharpe ratio
    double max_volatility_pct = 30.0;             // 30% max annual volatility
    
    // Individual position limits
    double max_single_trade_usd = 100000.0;       // $100K max single trade
    double max_positions_per_sector = 10;         // Max positions per sector
    double min_liquidity_threshold = 1000000.0;   // $1M min daily liquidity
    
    // Time-based limits
    std::chrono::hours max_position_hold_time{72}; // 72 hour max hold time
    uint32_t max_daily_trades = 1000;             // Max trades per day
    
    // Blacklists
    std::unordered_set<std::string> blacklisted_symbols;
    std::unordered_set<std::string> restricted_sectors;
    
    RiskLimits() = default;
};

// Circuit breaker configuration
struct CircuitBreakerConfig {
    CircuitBreakerType type;
    bool enabled = true;
    double trigger_threshold;
    double reset_threshold;
    std::chrono::minutes timeout_duration{30};
    bool auto_reset = true;
    uint32_t max_triggers_per_day = 10;
    
    // Notification settings
    bool send_alerts = true;
    bool emergency_liquidation = false;
    std::string notification_channel;
    
    CircuitBreakerConfig() : type(CircuitBreakerType::PORTFOLIO_DRAWDOWN), 
                           trigger_threshold(0.1), reset_threshold(0.05) {}
};

// Risk alert
struct RiskAlert {
    RiskLevel level;
    std::string alert_type;
    std::string description;
    std::string affected_symbol;
    double current_value;
    double threshold_value;
    std::chrono::system_clock::time_point timestamp;
    bool acknowledged = false;
    std::string action_taken;
    
    RiskAlert() : level(RiskLevel::LOW) {}
};

// Market data for risk calculations
struct MarketData {
    std::string symbol;
    double price;
    double volume;
    double volatility;
    double bid;
    double ask;
    double spread;
    std::chrono::system_clock::time_point timestamp;
    
    MarketData() : price(0), volume(0), volatility(0), bid(0), ask(0), spread(0) {}
};

// Historical price data for risk calculations
struct HistoricalData {
    std::vector<double> prices;
    std::vector<double> returns;
    std::vector<double> log_returns;
    std::deque<double> rolling_returns;
    size_t max_history_size = 252; // One trading year
    
    void add_price(double price);
    void calculate_returns();
    double get_volatility(size_t periods = 30) const;
    double get_var(double confidence = 0.95, size_t periods = 30) const;
    double get_cvar(double confidence = 0.95, size_t periods = 30) const;
};

// Portfolio performance tracking
struct PerformanceTracker {
    std::deque<double> daily_pnl;
    std::deque<double> portfolio_values;
    double peak_portfolio_value = 0.0;
    double trough_portfolio_value = 0.0;
    std::chrono::system_clock::time_point inception_time;
    
    void update(double current_value, double daily_pnl);
    double calculate_sharpe_ratio(size_t periods = 252) const;
    double calculate_maximum_drawdown() const;
    double calculate_calmar_ratio() const;
    std::vector<double> get_rolling_returns(size_t window = 30) const;
};

// Callback types
using RiskAlertCallback = std::function<void(const RiskAlert&)>;
using CircuitBreakerCallback = std::function<void(CircuitBreakerType, bool)>;
using PositionUpdateCallback = std::function<void(const Position&)>;
using MetricsUpdateCallback = std::function<void(const RiskMetrics&)>;

/**
 * @brief Comprehensive Risk Management System
 * 
 * Provides enterprise-grade risk management with:
 * - Real-time portfolio risk monitoring
 * - Multi-dimensional circuit breakers
 * - Advanced risk metrics (VaR, CVaR, Sharpe, etc.)
 * - Position-level and portfolio-level risk controls
 * - Historical performance tracking
 * - Automated risk alerts and emergency actions
 */
class ComprehensiveRiskManager {
public:
    explicit ComprehensiveRiskManager(const RiskLimits& limits = RiskLimits{});
    ~ComprehensiveRiskManager();
    
    // Core lifecycle
    bool start();
    void stop();
    bool is_running() const;
    
    // Position management
    bool validate_trade(const std::string& symbol, double quantity, double price);
    void add_position(const std::string& symbol, double quantity, double price);
    void update_position(const std::string& symbol, double new_price);
    void close_position(const std::string& symbol, double close_price);
    std::vector<Position> get_all_positions() const;
    Position get_position(const std::string& symbol) const;
    
    // Market data updates
    void update_market_data(const std::string& symbol, const MarketData& data);
    void update_historical_data(const std::string& symbol, double price);
    
    // Risk metrics and monitoring
    RiskMetrics calculate_risk_metrics();
    RiskMetrics get_current_risk_metrics() const;
    bool check_risk_limits(const RiskMetrics& metrics);
    std::vector<RiskAlert> get_active_alerts() const;
    
    // Circuit breakers
    void configure_circuit_breaker(const CircuitBreakerConfig& config);
    void enable_circuit_breaker(CircuitBreakerType type, bool enabled);
    bool is_circuit_breaker_triggered(CircuitBreakerType type) const;
    void reset_circuit_breaker(CircuitBreakerType type);
    std::vector<CircuitBreakerType> get_triggered_circuit_breakers() const;
    
    // Risk limits management
    void update_risk_limits(const RiskLimits& limits);
    RiskLimits get_risk_limits() const;
    void add_blacklisted_symbol(const std::string& symbol);
    void remove_blacklisted_symbol(const std::string& symbol);
    
    // Emergency controls
    void emergency_liquidate_all(const std::string& reason);
    void emergency_stop_trading(const std::string& reason);
    void pause_symbol(const std::string& symbol, const std::string& reason);
    void resume_symbol(const std::string& symbol);
    
    // Analytics and reporting
    std::vector<double> get_portfolio_value_history(size_t periods = 100) const;
    std::vector<double> get_daily_pnl_history(size_t periods = 100) const;
    double get_var_contribution(const std::string& symbol) const;
    std::vector<std::pair<std::string, double>> get_risk_contributions() const;
    
    // Performance analysis
    double calculate_portfolio_beta() const;
    double calculate_tracking_error() const;
    std::vector<std::pair<std::string, double>> get_top_positions(size_t count = 10) const;
    
    // Stress testing
    struct StressTestScenario {
        std::string name;
        std::unordered_map<std::string, double> price_shocks; // symbol -> % change
        double market_shock = 0.0; // Overall market shock %
    };
    
    RiskMetrics run_stress_test(const StressTestScenario& scenario);
    std::vector<RiskMetrics> run_monte_carlo_simulation(size_t iterations = 10000);
    
    // Configuration and callbacks
    void register_alert_callback(RiskAlertCallback callback);
    void register_circuit_breaker_callback(CircuitBreakerCallback callback);
    void register_position_update_callback(PositionUpdateCallback callback);
    void register_metrics_update_callback(MetricsUpdateCallback callback);
    
    // Advanced features
    void enable_real_time_monitoring(bool enabled);
    void set_monitoring_frequency(std::chrono::milliseconds frequency);
    void enable_auto_hedging(bool enabled, double hedge_ratio = 0.5);
    void enable_dynamic_position_sizing(bool enabled);
    
    // Compliance and reporting
    void generate_risk_report() const;
    void export_positions_to_csv(const std::string& filename) const;
    void export_risk_metrics_to_json(const std::string& filename) const;
    std::vector<RiskAlert> get_risk_alerts_since(std::chrono::system_clock::time_point since) const;

private:
    class RiskManagerImpl;
    std::unique_ptr<RiskManagerImpl> pimpl_;
};

/**
 * @brief Risk Calculator Utilities
 * 
 * Advanced mathematical functions for risk calculations
 */
class RiskCalculator {
public:
    // Value at Risk calculations
    static double calculate_historical_var(const std::vector<double>& returns, double confidence = 0.95);
    static double calculate_parametric_var(const std::vector<double>& returns, double confidence = 0.95);
    static double calculate_monte_carlo_var(const std::vector<double>& returns, double confidence = 0.95, size_t simulations = 10000);
    
    // Conditional Value at Risk
    static double calculate_cvar(const std::vector<double>& returns, double confidence = 0.95);
    
    // Risk-adjusted performance metrics
    static double calculate_sharpe_ratio(const std::vector<double>& returns, double risk_free_rate = 0.02);
    static double calculate_sortino_ratio(const std::vector<double>& returns, double target_return = 0.0);
    static double calculate_calmar_ratio(const std::vector<double>& returns);
    static double calculate_maximum_drawdown(const std::vector<double>& values);
    
    // Portfolio risk metrics
    static double calculate_portfolio_var(const std::vector<double>& weights, 
                                        const std::vector<std::vector<double>>& covariance_matrix);
    static std::vector<double> calculate_risk_contributions(const std::vector<double>& weights,
                                                           const std::vector<std::vector<double>>& covariance_matrix);
    
    // Correlation and diversification
    static std::vector<std::vector<double>> calculate_correlation_matrix(const std::vector<std::vector<double>>& returns);
    static double calculate_diversification_ratio(const std::vector<double>& weights,
                                                 const std::vector<double>& volatilities,
                                                 const std::vector<std::vector<double>>& correlation_matrix);
    
    // Options Greeks for risk (if applicable)
    static double calculate_delta(double spot, double strike, double time_to_expiry, double volatility, double risk_free_rate);
    static double calculate_gamma(double spot, double strike, double time_to_expiry, double volatility, double risk_free_rate);
    static double calculate_vega(double spot, double strike, double time_to_expiry, double volatility, double risk_free_rate);
};

/**
 * @brief Risk Model Factory
 * 
 * Factory for creating different risk model configurations
 */
class RiskModelFactory {
public:
    // Preset risk models
    static RiskLimits create_conservative_limits();
    static RiskLimits create_moderate_limits();
    static RiskLimits create_aggressive_limits();
    static RiskLimits create_high_frequency_limits();
    static RiskLimits create_market_making_limits();
    
    // Circuit breaker presets
    static std::vector<CircuitBreakerConfig> create_standard_circuit_breakers();
    static std::vector<CircuitBreakerConfig> create_high_frequency_circuit_breakers();
    static std::vector<CircuitBreakerConfig> create_conservative_circuit_breakers();
};

} // namespace hfx::risk
