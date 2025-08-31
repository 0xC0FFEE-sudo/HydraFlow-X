#pragma once

#include <string>
#include <vector>
#include <memory>
#include <atomic>
#include <mutex>
#include <unordered_map>
#include <chrono>
#include <functional>
#include <deque>
#include <optional>
#include <array>

namespace hfx {
namespace mm {

// Forward declarations
struct PriceDataPoint;
struct MarketEvent;

// Volatility model types
enum class VolatilityModel {
    HISTORICAL = 0,           // Simple historical volatility
    EXPONENTIAL_SMOOTHING,    // Exponential weighted moving average
    GARCH,                   // Generalized Autoregressive Conditional Heteroskedasticity
    EWMA,                    // Exponentially Weighted Moving Average
    REALIZED_VOLATILITY,     // High-frequency realized volatility
    IMPLIED_VOLATILITY,      // Options-based implied volatility
    STOCHASTIC_VOLATILITY,   // Stochastic volatility models
    JUMP_DIFFUSION,          // Jump-diffusion volatility
    REGIME_SWITCHING,        // Markov regime-switching models
    NEURAL_NETWORK,          // Deep learning volatility models
    ENSEMBLE,                // Ensemble of multiple models
    CUSTOM
};

// Time horizons for volatility estimation
enum class VolatilityHorizon {
    INTRABLOCK = 0,          // Sub-block (seconds)
    BLOCK,                   // Single block (~12 seconds)
    SHORT_TERM,              // Minutes (5-15 minutes)
    MEDIUM_TERM,             // Hours (1-4 hours)
    LONG_TERM,               // Days (1-7 days)
    WEEKLY,                  // Weekly
    MONTHLY,                 // Monthly
    CUSTOM
};

// Market regime types for volatility clustering
enum class MarketRegime {
    LOW_VOLATILITY = 0,
    MODERATE_VOLATILITY,
    HIGH_VOLATILITY,
    EXTREME_VOLATILITY,
    CRASH,
    RECOVERY,
    TRENDING,
    SIDEWAYS,
    UNKNOWN
};

// Volatility surface dimensions
enum class VolatilitySurfaceDimension {
    TIME_TO_EXPIRY = 0,
    MONEYNESS,
    DELTA,
    STRIKE,
    TERM_STRUCTURE
};

// Price data point for volatility calculation
struct PriceDataPoint {
    double price;
    double log_return;
    uint64_t volume;
    double high;
    double low;
    double open;
    double close;
    double vwap;
    
    // Microstructure data
    double bid;
    double ask;
    double spread;
    uint32_t trade_count;
    double dollar_volume;
    
    // Block/time information
    uint64_t block_number;
    std::chrono::system_clock::time_point timestamp;
    uint32_t time_interval_seconds;
    
    // Market context
    MarketRegime regime;
    bool is_outlier;
    double volatility_contribution;
    
    PriceDataPoint() : price(0.0), log_return(0.0), volume(0), high(0.0), low(0.0),
                      open(0.0), close(0.0), vwap(0.0), bid(0.0), ask(0.0), spread(0.0),
                      trade_count(0), dollar_volume(0.0), block_number(0), time_interval_seconds(0),
                      regime(MarketRegime::UNKNOWN), is_outlier(false), volatility_contribution(0.0) {}
};

// Volatility estimate with confidence intervals
struct VolatilityEstimate {
    // Core volatility measures
    double annualized_volatility;
    double daily_volatility;
    double hourly_volatility;
    double block_volatility;
    
    // Statistical measures
    double variance;
    double standard_deviation;
    double coefficient_of_variation;
    double skewness;
    double kurtosis;
    
    // Confidence intervals
    double lower_bound_95;
    double upper_bound_95;
    double lower_bound_68;
    double upper_bound_68;
    
    // Model-specific estimates
    std::unordered_map<VolatilityModel, double> model_estimates;
    VolatilityModel primary_model;
    double model_confidence;
    
    // Forward-looking estimates
    std::vector<double> volatility_forecast;  // Forecast for next N periods
    double trend_component;
    double cyclical_component;
    double regime_persistence_probability;
    
    // Market microstructure
    double bid_ask_volatility;
    double volume_weighted_volatility;
    double price_impact_volatility;
    double trade_size_volatility;
    
    // Risk metrics
    double value_at_risk_1;      // 1% VaR
    double value_at_risk_5;      // 5% VaR
    double expected_shortfall_1; // 1% Expected Shortfall
    double expected_shortfall_5; // 5% Expected Shortfall
    double maximum_drawdown;
    
    // Regime information
    MarketRegime current_regime;
    double regime_probability;
    std::unordered_map<MarketRegime, double> regime_probabilities;
    
    // Metadata
    VolatilityHorizon horizon;
    uint32_t sample_size;
    std::chrono::system_clock::time_point estimation_time;
    std::chrono::system_clock::time_point data_start_time;
    std::chrono::system_clock::time_point data_end_time;
    
    VolatilityEstimate() : annualized_volatility(0.0), daily_volatility(0.0),
                          hourly_volatility(0.0), block_volatility(0.0), variance(0.0),
                          standard_deviation(0.0), coefficient_of_variation(0.0),
                          skewness(0.0), kurtosis(0.0), lower_bound_95(0.0), upper_bound_95(0.0),
                          lower_bound_68(0.0), upper_bound_68(0.0), primary_model(VolatilityModel::HISTORICAL),
                          model_confidence(0.0), trend_component(0.0), cyclical_component(0.0),
                          regime_persistence_probability(0.0), bid_ask_volatility(0.0),
                          volume_weighted_volatility(0.0), price_impact_volatility(0.0),
                          trade_size_volatility(0.0), value_at_risk_1(0.0), value_at_risk_5(0.0),
                          expected_shortfall_1(0.0), expected_shortfall_5(0.0), maximum_drawdown(0.0),
                          current_regime(MarketRegime::UNKNOWN), regime_probability(0.0),
                          horizon(VolatilityHorizon::MEDIUM_TERM), sample_size(0) {}
};

// Volatility surface for options-like instruments
struct VolatilitySurface {
    std::string underlying_asset;
    std::vector<double> time_to_expiry_points;
    std::vector<double> moneyness_points;
    std::vector<std::vector<double>> volatility_matrix;  // [time][moneyness]
    
    // Surface characteristics
    double atm_volatility;
    double volatility_skew;
    double term_structure_slope;
    double convexity;
    
    // Model parameters
    std::unordered_map<std::string, double> model_parameters;
    double surface_quality_score;
    
    std::chrono::system_clock::time_point last_updated;
    
    VolatilitySurface() : atm_volatility(0.0), volatility_skew(0.0), term_structure_slope(0.0),
                         convexity(0.0), surface_quality_score(0.0) {}
};

// GARCH model parameters
struct GARCHParameters {
    double omega;      // Constant term
    double alpha;      // ARCH parameter
    double beta;       // GARCH parameter
    double gamma;      // Asymmetry parameter (for GJR-GARCH)
    
    // Model diagnostics
    double log_likelihood;
    double aic;
    double bic;
    std::vector<double> residuals;
    std::vector<double> standardized_residuals;
    
    GARCHParameters() : omega(0.0), alpha(0.0), beta(0.0), gamma(0.0),
                       log_likelihood(0.0), aic(0.0), bic(0.0) {}
};

// Model performance metrics
struct VolatilityModelPerformance {
    VolatilityModel model_type;
    std::string model_name;
    
    // Accuracy metrics
    double mean_absolute_error;
    double mean_squared_error;
    double root_mean_squared_error;
    double mean_absolute_percentage_error;
    double forecast_accuracy;
    
    // Statistical tests
    double ljung_box_p_value;
    double jarque_bera_p_value;
    double arch_lm_p_value;
    double durbin_watson_statistic;
    
    // Regime detection accuracy
    double regime_classification_accuracy;
    double regime_transition_detection_rate;
    
    // Computational performance
    double avg_computation_time_ms;
    double max_computation_time_ms;
    uint64_t total_computations;
    
    // Robustness metrics
    double outlier_sensitivity;
    double parameter_stability;
    double out_of_sample_performance;
    
    std::chrono::system_clock::time_point last_evaluated;
    
    VolatilityModelPerformance() : model_type(VolatilityModel::HISTORICAL),
                                  mean_absolute_error(0.0), mean_squared_error(0.0),
                                  root_mean_squared_error(0.0), mean_absolute_percentage_error(0.0),
                                  forecast_accuracy(0.0), ljung_box_p_value(0.0), jarque_bera_p_value(0.0),
                                  arch_lm_p_value(0.0), durbin_watson_statistic(0.0),
                                  regime_classification_accuracy(0.0), regime_transition_detection_rate(0.0),
                                  avg_computation_time_ms(0.0), max_computation_time_ms(0.0),
                                  total_computations(0), outlier_sensitivity(0.0), parameter_stability(0.0),
                                  out_of_sample_performance(0.0) {}
};

// Volatility model configuration
struct VolatilityConfig {
    // Model selection
    std::vector<VolatilityModel> enabled_models;
    VolatilityModel primary_model = VolatilityModel::ENSEMBLE;
    bool use_ensemble_averaging = true;
    
    // Data settings
    uint32_t historical_window_size = 1000;
    uint32_t min_required_observations = 50;
    uint32_t max_data_age_hours = 168; // 1 week
    bool use_high_frequency_data = true;
    bool filter_outliers = true;
    double outlier_threshold_sigma = 3.0;
    
    // Time horizons
    std::vector<VolatilityHorizon> estimation_horizons;
    uint32_t forecast_horizon_periods = 20;
    bool enable_multi_horizon_forecasting = true;
    
    // Model-specific parameters
    // GARCH parameters
    uint32_t garch_p = 1;  // GARCH order
    uint32_t garch_q = 1;  // ARCH order
    bool use_gjr_garch = true;  // Asymmetric GARCH
    
    // EWMA parameters
    double ewma_lambda = 0.94;
    bool adaptive_lambda = true;
    
    // Regime switching parameters
    uint32_t max_regimes = 4;
    bool enable_regime_switching = true;
    double regime_switching_threshold = 0.8;
    
    // Neural network parameters
    std::vector<uint32_t> nn_hidden_layers = {64, 32};
    double nn_learning_rate = 0.001;
    uint32_t nn_epochs = 100;
    bool use_lstm = true;
    
    // Performance settings
    uint32_t max_concurrent_calculations = 4;
    uint32_t calculation_timeout_ms = 5000;
    uint32_t cache_size = 1000;
    uint32_t cache_ttl_seconds = 300;
    
    // Real-time settings
    bool enable_real_time_updates = true;
    uint32_t update_frequency_seconds = 30;
    bool stream_volatility_updates = false;
    
    // Risk management
    double max_volatility_threshold = 5.0;  // 500% annualized
    bool enable_volatility_alerts = true;
    std::vector<double> alert_thresholds = {0.5, 1.0, 2.0}; // 50%, 100%, 200%
};

// Volatility statistics
struct VolatilityStats {
    std::atomic<uint64_t> total_calculations{0};
    std::atomic<uint64_t> successful_calculations{0};
    std::atomic<uint64_t> failed_calculations{0};
    std::atomic<uint64_t> cache_hits{0};
    std::atomic<uint64_t> cache_misses{0};
    
    std::atomic<double> avg_calculation_time_ms{0.0};
    std::atomic<double> avg_volatility_estimate{0.0};
    std::atomic<double> current_model_accuracy{0.0};
    std::atomic<double> regime_detection_accuracy{0.0};
    
    std::chrono::system_clock::time_point last_reset;
};

// Main Volatility Models Class
class VolatilityModels {
public:
    explicit VolatilityModels(const VolatilityConfig& config);
    ~VolatilityModels();

    // Core volatility estimation
    VolatilityEstimate estimate_volatility(const std::string& symbol, 
                                         VolatilityHorizon horizon = VolatilityHorizon::MEDIUM_TERM);
    VolatilityEstimate estimate_volatility(const std::vector<PriceDataPoint>& price_data,
                                         VolatilityHorizon horizon = VolatilityHorizon::MEDIUM_TERM);
    std::vector<VolatilityEstimate> estimate_multiple_horizons(const std::string& symbol);
    
    // Model-specific estimations
    VolatilityEstimate estimate_historical_volatility(const std::vector<PriceDataPoint>& data);
    VolatilityEstimate estimate_garch_volatility(const std::vector<PriceDataPoint>& data);
    VolatilityEstimate estimate_ewma_volatility(const std::vector<PriceDataPoint>& data);
    VolatilityEstimate estimate_realized_volatility(const std::vector<PriceDataPoint>& data);
    VolatilityEstimate estimate_regime_switching_volatility(const std::vector<PriceDataPoint>& data);
    
    // Ensemble and advanced methods
    VolatilityEstimate ensemble_volatility_estimate(const std::vector<PriceDataPoint>& data);
    VolatilityEstimate forecast_volatility(const std::string& symbol, uint32_t periods_ahead);
    std::vector<double> compute_volatility_term_structure(const std::string& symbol);
    
    // Volatility surface construction
    VolatilitySurface construct_volatility_surface(const std::string& underlying);
    double interpolate_volatility(const VolatilitySurface& surface, double time_to_expiry, double moneyness);
    void update_volatility_surface(const std::string& underlying, const VolatilitySurface& surface);
    
    // Regime detection and analysis
    MarketRegime detect_current_regime(const std::string& symbol);
    std::vector<MarketRegime> get_regime_history(const std::string& symbol, uint32_t periods);
    std::unordered_map<MarketRegime, double> get_regime_probabilities(const std::string& symbol);
    double estimate_regime_persistence(const std::string& symbol, MarketRegime regime);
    
    // Risk metrics calculation
    double calculate_value_at_risk(const std::string& symbol, double confidence_level, uint32_t holding_period);
    double calculate_expected_shortfall(const std::string& symbol, double confidence_level, uint32_t holding_period);
    double calculate_maximum_drawdown(const std::string& symbol, uint32_t lookback_periods);
    std::vector<double> calculate_volatility_percentiles(const std::string& symbol);
    
    // Data management
    void add_price_data(const std::string& symbol, const PriceDataPoint& data_point);
    void add_price_data_batch(const std::string& symbol, const std::vector<PriceDataPoint>& data_batch);
    void update_market_data(const std::string& symbol, double price, uint64_t volume);
    std::vector<PriceDataPoint> get_price_history(const std::string& symbol, uint32_t periods);
    
    // Model management and training
    void train_models(const std::string& symbol);
    void train_specific_model(VolatilityModel model, const std::string& symbol);
    void calibrate_models();
    void set_primary_model(VolatilityModel model);
    VolatilityModel get_best_performing_model(const std::string& symbol);
    
    // Model validation and performance
    std::vector<VolatilityModelPerformance> evaluate_model_performance(const std::string& symbol);
    VolatilityModelPerformance cross_validate_model(VolatilityModel model, const std::string& symbol);
    void backtest_volatility_forecasts(const std::string& symbol, uint32_t test_periods);
    std::vector<double> compute_forecast_errors(const std::string& symbol, uint32_t periods);
    
    // Real-time monitoring
    using VolatilityCallback = std::function<void(const std::string&, const VolatilityEstimate&)>;
    using RegimeChangeCallback = std::function<void(const std::string&, MarketRegime, MarketRegime)>;
    void register_volatility_callback(VolatilityCallback callback);
    void register_regime_change_callback(RegimeChangeCallback callback);
    void start_real_time_monitoring();
    void stop_real_time_monitoring();
    bool is_monitoring() const;
    
    // Configuration management
    void update_config(const VolatilityConfig& config);
    VolatilityConfig get_config() const;
    void enable_model(VolatilityModel model);
    void disable_model(VolatilityModel model);
    void set_model_weight(VolatilityModel model, double weight);
    
    // Statistics and diagnostics
    VolatilityStats get_statistics() const;
    void reset_statistics();
    std::unordered_map<std::string, VolatilityEstimate> get_current_volatilities();
    std::vector<std::string> get_high_volatility_symbols(double threshold);
    
    // Advanced analytics
    std::unordered_map<VolatilityModel, double> analyze_model_contributions();
    std::vector<std::pair<std::string, double>> get_volatility_rankings();
    double calculate_portfolio_volatility(const std::vector<std::string>& symbols, 
                                        const std::vector<double>& weights);
    std::unordered_map<std::string, std::vector<double>> compute_volatility_correlations();
    
    // Volatility clustering analysis
    double detect_volatility_clustering(const std::string& symbol);
    std::vector<std::pair<std::chrono::system_clock::time_point, double>> 
        identify_volatility_breakpoints(const std::string& symbol);
    double calculate_volatility_persistence(const std::string& symbol);

private:
    VolatilityConfig config_;
    std::atomic<bool> monitoring_;
    mutable std::mutex config_mutex_;
    
    // Price data storage
    std::unordered_map<std::string, std::deque<PriceDataPoint>> price_data_;
    mutable std::mutex price_data_mutex_;
    
    // Volatility models
    std::unique_ptr<class HistoricalVolatilityModel> historical_model_;
    std::unique_ptr<class GARCHModel> garch_model_;
    std::unique_ptr<class EWMAModel> ewma_model_;
    std::unique_ptr<class RealizedVolatilityModel> realized_model_;
    std::unique_ptr<class RegimeSwitchingModel> regime_model_;
    std::unique_ptr<class NeuralNetworkVolatilityModel> nn_model_;
    std::unique_ptr<class EnsembleVolatilityModel> ensemble_model_;
    
    // Model performance tracking
    std::unordered_map<std::string, std::unordered_map<VolatilityModel, VolatilityModelPerformance>> performance_metrics_;
    
    // Volatility surfaces
    std::unordered_map<std::string, VolatilitySurface> volatility_surfaces_;
    mutable std::mutex surfaces_mutex_;
    
    // Regime tracking
    std::unordered_map<std::string, MarketRegime> current_regimes_;
    std::unordered_map<std::string, std::vector<MarketRegime>> regime_history_;
    mutable std::mutex regime_mutex_;
    
    // Caching
    std::unordered_map<std::string, VolatilityEstimate> volatility_cache_;
    mutable std::mutex cache_mutex_;
    
    // Real-time monitoring
    std::thread monitoring_thread_;
    std::vector<VolatilityCallback> volatility_callbacks_;
    std::vector<RegimeChangeCallback> regime_callbacks_;
    mutable std::mutex callbacks_mutex_;
    
    // Statistics
    mutable VolatilityStats stats_;
    
    // Internal calculation methods
    std::vector<double> calculate_log_returns(const std::vector<PriceDataPoint>& data);
    std::vector<double> calculate_realized_volatility_series(const std::vector<PriceDataPoint>& data);
    GARCHParameters fit_garch_model(const std::vector<double>& returns);
    
    // Regime detection helpers
    std::vector<MarketRegime> detect_regimes_hmm(const std::vector<double>& returns);
    void update_regime_probabilities(const std::string& symbol);
    
    // Statistical computations
    double compute_historical_volatility(const std::vector<double>& returns, uint32_t window);
    double compute_ewma_volatility(const std::vector<double>& returns, double lambda);
    std::vector<double> compute_garch_volatility_forecast(const GARCHParameters& params, uint32_t periods);
    
    // Data preprocessing
    void clean_price_data(std::vector<PriceDataPoint>& data);
    void detect_and_handle_outliers(std::vector<PriceDataPoint>& data);
    void interpolate_missing_data(std::vector<PriceDataPoint>& data);
    
    // Model validation helpers
    void perform_residual_diagnostics(const std::vector<double>& residuals);
    double compute_forecast_accuracy(const std::vector<double>& forecasts, const std::vector<double>& actuals);
    void update_model_performance(const std::string& symbol, VolatilityModel model, double accuracy);
    
    // Utility methods
    std::string generate_cache_key(const std::string& symbol, VolatilityHorizon horizon);
    void update_cache(const std::string& key, const VolatilityEstimate& estimate);
    std::optional<VolatilityEstimate> get_from_cache(const std::string& key);
    void cleanup_expired_cache();
    
    // Real-time monitoring worker
    void monitoring_worker();
    void process_volatility_updates();
    void notify_volatility_callbacks(const std::string& symbol, const VolatilityEstimate& estimate);
    void notify_regime_change_callbacks(const std::string& symbol, MarketRegime old_regime, MarketRegime new_regime);
    
    // Statistics update
    void update_statistics(bool success, double calculation_time_ms, double accuracy = 0.0);
};

// Utility functions
std::string volatility_model_to_string(VolatilityModel model);
VolatilityModel string_to_volatility_model(const std::string& str);
std::string volatility_horizon_to_string(VolatilityHorizon horizon);
std::string market_regime_to_string(MarketRegime regime);
double annualize_volatility(double volatility, double frequency);
double convert_volatility_horizon(double volatility, VolatilityHorizon from, VolatilityHorizon to);
bool is_reasonable_volatility_estimate(const VolatilityEstimate& estimate);

} // namespace mm
} // namespace hfx
