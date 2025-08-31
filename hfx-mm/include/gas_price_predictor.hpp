#pragma once

#include <string>
#include <vector>
#include <memory>
#include <atomic>
#include <mutex>
#include <unordered_map>
#include <chrono>
#include <functional>
#include <queue>
#include <deque>
#include <optional>

namespace hfx {
namespace mm {

// Forward declarations
struct BlockInfo;
struct TransactionInfo;

// Gas price prediction models
enum class PredictionModel {
    LINEAR_REGRESSION = 0,
    EXPONENTIAL_SMOOTHING,
    ARIMA,
    NEURAL_NETWORK,
    ENSEMBLE,
    KALMAN_FILTER,
    LSTM,
    TRANSFORMER,
    REINFORCEMENT_LEARNING,
    CUSTOM
};

// Market regime types
enum class MarketRegime {
    NORMAL = 0,
    HIGH_VOLATILITY,
    NETWORK_CONGESTION,
    MEV_SURGE,
    FLASH_CRASH,
    BEAR_MARKET,
    BULL_MARKET,
    SIDEWAYS,
    UNKNOWN
};

// Time horizons for predictions
enum class TimeHorizon {
    IMMEDIATE = 0,      // Next block
    SHORT_TERM,         // 1-5 blocks
    MEDIUM_TERM,        // 5-20 blocks
    LONG_TERM,          // 20+ blocks
    INTRADAY,           // Hours
    DAILY,              // Days
    CUSTOM
};

// Gas price data point
struct GasPriceDataPoint {
    uint64_t gas_price_gwei;
    uint64_t base_fee_gwei;
    uint64_t priority_fee_gwei;
    uint64_t block_number;
    uint32_t block_time;
    uint32_t transaction_count;
    double block_utilization;
    uint64_t total_gas_used;
    uint64_t gas_limit;
    
    // Market context
    MarketRegime market_regime;
    double mev_activity_score;
    uint32_t pending_transaction_count;
    double network_congestion_score;
    
    std::chrono::system_clock::time_point timestamp;
    
    GasPriceDataPoint() : gas_price_gwei(0), base_fee_gwei(0), priority_fee_gwei(0),
                         block_number(0), block_time(0), transaction_count(0),
                         block_utilization(0.0), total_gas_used(0), gas_limit(0),
                         market_regime(MarketRegime::NORMAL), mev_activity_score(0.0),
                         pending_transaction_count(0), network_congestion_score(0.0) {}
};

// Prediction result
struct GasPricePrediction {
    uint64_t predicted_gas_price_gwei;
    uint64_t predicted_base_fee_gwei;
    uint64_t predicted_priority_fee_gwei;
    
    // Confidence intervals
    uint64_t lower_bound_95;
    uint64_t upper_bound_95;
    uint64_t lower_bound_68;
    uint64_t upper_bound_68;
    
    // Prediction metadata
    PredictionModel model_used;
    TimeHorizon time_horizon;
    uint32_t blocks_ahead;
    double confidence_score;
    double prediction_error_estimate;
    
    // Market context
    MarketRegime predicted_regime;
    double volatility_estimate;
    double trend_strength;
    
    std::chrono::system_clock::time_point prediction_time;
    std::chrono::system_clock::time_point target_time;
    
    GasPricePrediction() : predicted_gas_price_gwei(0), predicted_base_fee_gwei(0),
                          predicted_priority_fee_gwei(0), lower_bound_95(0), upper_bound_95(0),
                          lower_bound_68(0), upper_bound_68(0), model_used(PredictionModel::LINEAR_REGRESSION),
                          time_horizon(TimeHorizon::IMMEDIATE), blocks_ahead(0), confidence_score(0.0),
                          prediction_error_estimate(0.0), predicted_regime(MarketRegime::NORMAL),
                          volatility_estimate(0.0), trend_strength(0.0) {}
};

// Model performance metrics
struct ModelPerformance {
    std::string model_name;
    PredictionModel model_type;
    
    // Accuracy metrics
    double mean_absolute_error;
    double mean_squared_error;
    double root_mean_squared_error;
    double mean_absolute_percentage_error;
    double r_squared;
    
    // Prediction quality
    double directional_accuracy;    // % of correct trend predictions
    double confidence_calibration;  // How well confidence matches accuracy
    double prediction_coverage;     // % of actual values within confidence intervals
    
    // Timing metrics
    double avg_prediction_time_ms;
    double max_prediction_time_ms;
    uint64_t total_predictions;
    
    // Recent performance (rolling window)
    double recent_accuracy_1h;
    double recent_accuracy_24h;
    double recent_accuracy_7d;
    
    std::chrono::system_clock::time_point last_updated;
    
    ModelPerformance() : model_type(PredictionModel::LINEAR_REGRESSION),
                        mean_absolute_error(0.0), mean_squared_error(0.0), root_mean_squared_error(0.0),
                        mean_absolute_percentage_error(0.0), r_squared(0.0), directional_accuracy(0.0),
                        confidence_calibration(0.0), prediction_coverage(0.0), avg_prediction_time_ms(0.0),
                        max_prediction_time_ms(0.0), total_predictions(0), recent_accuracy_1h(0.0),
                        recent_accuracy_24h(0.0), recent_accuracy_7d(0.0) {}
};

// Feature importance for model interpretability
struct FeatureImportance {
    std::string feature_name;
    double importance_score;
    double correlation_with_target;
    double feature_stability;
    std::vector<double> importance_over_time;
    
    FeatureImportance() : importance_score(0.0), correlation_with_target(0.0), feature_stability(0.0) {}
};

// Predictor configuration
struct PredictorConfig {
    // Data collection
    uint32_t historical_blocks = 1000;
    uint32_t max_data_age_hours = 168; // 1 week
    uint32_t data_collection_interval_seconds = 12;
    bool collect_mempool_data = true;
    bool collect_block_data = true;
    
    // Models to use
    std::vector<PredictionModel> enabled_models;
    PredictionModel primary_model = PredictionModel::ENSEMBLE;
    bool use_ensemble_averaging = true;
    double ensemble_weight_decay = 0.95;
    
    // Feature engineering
    bool enable_technical_indicators = true;
    bool enable_fourier_features = true;
    bool enable_lag_features = true;
    uint32_t max_lag_periods = 20;
    
    // Model training
    double train_test_split = 0.8;
    uint32_t training_window_blocks = 500;
    uint32_t retraining_frequency_blocks = 100;
    bool enable_online_learning = true;
    double learning_rate = 0.001;
    
    // Prediction settings
    std::vector<TimeHorizon> prediction_horizons;
    uint32_t max_prediction_blocks = 50;
    bool enable_uncertainty_quantification = true;
    bool enable_regime_detection = true;
    
    // Performance settings
    uint32_t max_concurrent_predictions = 4;
    uint32_t prediction_timeout_ms = 1000;
    uint32_t cache_size = 1000;
    uint32_t cache_ttl_seconds = 60;
    
    // Chain-specific settings
    uint32_t chain_id = 1;
    std::vector<std::string> rpc_endpoints;
    bool use_eip1559 = true;
    uint64_t min_base_fee = 1000000000; // 1 gwei
    uint64_t max_gas_price = 1000000000000; // 1000 gwei
};

// Predictor statistics
struct PredictorStats {
    std::atomic<uint64_t> total_predictions{0};
    std::atomic<uint64_t> successful_predictions{0};
    std::atomic<uint64_t> failed_predictions{0};
    std::atomic<uint64_t> cache_hits{0};
    std::atomic<uint64_t> cache_misses{0};
    
    std::atomic<double> avg_prediction_time_ms{0.0};
    std::atomic<double> avg_prediction_accuracy{0.0};
    std::atomic<double> avg_confidence_score{0.0};
    std::atomic<double> current_model_r_squared{0.0};
    
    std::chrono::system_clock::time_point last_reset;
};

// Main Gas Price Predictor Class
class GasPricePredictor {
public:
    explicit GasPricePredictor(const PredictorConfig& config);
    ~GasPricePredictor();

    // Core prediction functionality
    GasPricePrediction predict_gas_price(TimeHorizon horizon = TimeHorizon::IMMEDIATE);
    GasPricePrediction predict_gas_price_blocks_ahead(uint32_t blocks);
    std::vector<GasPricePrediction> predict_multiple_horizons();
    
    // Batch predictions
    std::vector<GasPricePrediction> predict_batch(const std::vector<TimeHorizon>& horizons);
    std::vector<GasPricePrediction> predict_sequence(uint32_t sequence_length);
    
    // Model management
    void train_model(PredictionModel model_type);
    void train_all_models();
    void retrain_models();
    void set_primary_model(PredictionModel model);
    PredictionModel get_best_performing_model() const;
    
    // Data management
    void add_gas_price_data(const GasPriceDataPoint& data_point);
    void add_block_data(const BlockInfo& block);
    void update_mempool_state(uint32_t pending_count, double avg_gas_price);
    std::vector<GasPriceDataPoint> get_historical_data(uint32_t blocks) const;
    
    // Feature engineering
    std::vector<double> extract_features(const std::vector<GasPriceDataPoint>& data);
    std::vector<FeatureImportance> get_feature_importance() const;
    void update_feature_engineering();
    
    // Market regime detection
    MarketRegime detect_current_regime();
    std::vector<MarketRegime> get_regime_history(uint32_t blocks) const;
    double get_regime_transition_probability(MarketRegime from, MarketRegime to) const;
    
    // Model performance and validation
    std::vector<ModelPerformance> get_model_performance() const;
    ModelPerformance evaluate_model(PredictionModel model, uint32_t test_blocks = 100);
    void validate_predictions();
    double calculate_prediction_accuracy() const;
    
    // Real-time prediction updates
    using PredictionCallback = std::function<void(const GasPricePrediction&)>;
    void register_prediction_callback(PredictionCallback callback);
    void start_real_time_predictions();
    void stop_real_time_predictions();
    bool is_predicting() const;
    
    // Configuration management
    void update_config(const PredictorConfig& config);
    PredictorConfig get_config() const;
    void enable_model(PredictionModel model);
    void disable_model(PredictionModel model);
    
    // Statistics and monitoring
    PredictorStats get_statistics() const;
    void reset_statistics();
    std::vector<double> get_recent_errors(uint32_t count = 100) const;
    double get_current_accuracy() const;
    
    // Advanced features
    GasPricePrediction predict_with_context(const std::string& transaction_type, 
                                           uint64_t transaction_value);
    std::vector<GasPricePrediction> predict_volatility_adjusted();
    GasPricePrediction predict_optimal_gas_price(double target_confirmation_probability);
    
    // Calibration and optimization
    void calibrate_models();
    void optimize_hyperparameters();
    void perform_feature_selection();
    void update_ensemble_weights();

private:
    PredictorConfig config_;
    std::atomic<bool> predicting_;
    mutable std::mutex config_mutex_;
    
    // Data storage
    std::deque<GasPriceDataPoint> historical_data_;
    std::queue<GasPriceDataPoint> recent_data_;
    mutable std::mutex data_mutex_;
    
    // Prediction models
    std::unordered_map<PredictionModel, std::unique_ptr<class PredictionModelBase>> models_;
    PredictionModel primary_model_;
    std::vector<double> ensemble_weights_;
    mutable std::mutex models_mutex_;
    
    // Feature engineering
    std::unique_ptr<class FeatureEngineer> feature_engineer_;
    std::vector<FeatureImportance> feature_importance_;
    
    // Market regime detector
    std::unique_ptr<class RegimeDetector> regime_detector_;
    MarketRegime current_regime_;
    
    // Performance tracking
    std::unordered_map<PredictionModel, ModelPerformance> model_performance_;
    std::vector<std::pair<GasPricePrediction, uint64_t>> prediction_history_; // prediction, actual
    
    // Caching
    std::unordered_map<std::string, GasPricePrediction> prediction_cache_;
    mutable std::mutex cache_mutex_;
    
    // Real-time prediction
    std::thread prediction_thread_;
    std::vector<PredictionCallback> prediction_callbacks_;
    mutable std::mutex callbacks_mutex_;
    
    // Statistics
    mutable PredictorStats stats_;
    
    // Internal prediction methods
    GasPricePrediction predict_with_model(PredictionModel model, TimeHorizon horizon);
    GasPricePrediction ensemble_predict(TimeHorizon horizon);
    GasPricePrediction combine_predictions(const std::vector<GasPricePrediction>& predictions);
    
    // Data preprocessing
    std::vector<double> preprocess_data(const std::vector<GasPriceDataPoint>& data);
    std::vector<double> normalize_features(const std::vector<double>& features);
    void detect_and_handle_outliers(std::vector<GasPriceDataPoint>& data);
    
    // Model training helpers
    void prepare_training_data(PredictionModel model);
    void update_model_performance(PredictionModel model, const std::vector<double>& errors);
    void cross_validate_model(PredictionModel model);
    
    // Utility methods
    std::string generate_cache_key(TimeHorizon horizon, uint32_t blocks_ahead);
    void update_cache(const std::string& key, const GasPricePrediction& prediction);
    std::optional<GasPricePrediction> get_from_cache(const std::string& key);
    void cleanup_expired_cache();
    
    // Real-time worker
    void prediction_worker();
    void notify_prediction_callbacks(const GasPricePrediction& prediction);
    
    // Statistics update
    void update_statistics(bool success, double prediction_time_ms, double accuracy = 0.0);
};

// Utility functions
std::string prediction_model_to_string(PredictionModel model);
PredictionModel string_to_prediction_model(const std::string& str);
std::string market_regime_to_string(MarketRegime regime);
std::string time_horizon_to_string(TimeHorizon horizon);
double calculate_prediction_error(const GasPricePrediction& prediction, uint64_t actual_gas_price);
bool is_prediction_reasonable(const GasPricePrediction& prediction);

} // namespace mm
} // namespace hfx
