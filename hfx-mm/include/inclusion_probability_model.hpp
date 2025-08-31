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
#include <optional>
#include <set>

namespace hfx {
namespace mm {

// Forward declarations
struct Transaction;
struct BlockInfo;
struct GasPriceDataPoint;

// Inclusion outcome
enum class InclusionOutcome {
    INCLUDED = 0,
    PENDING,
    DROPPED,
    REPLACED,
    FAILED,
    TIMEOUT,
    UNKNOWN
};

// Priority levels
enum class PriorityLevel {
    VERY_LOW = 0,
    LOW,
    MEDIUM,
    HIGH,
    VERY_HIGH,
    CRITICAL,
    CUSTOM
};

// Transaction characteristics for inclusion modeling
struct TransactionFeatures {
    // Basic transaction properties
    uint64_t gas_price_gwei;
    uint64_t gas_limit;
    uint64_t transaction_value_wei;
    uint32_t nonce;
    uint32_t data_size_bytes;
    bool is_contract_call;
    
    // EIP-1559 properties
    uint64_t max_fee_per_gas;
    uint64_t max_priority_fee_per_gas;
    uint64_t base_fee_at_submission;
    
    // Market context at submission time
    uint32_t mempool_size;
    uint64_t median_gas_price;
    double network_congestion_score;
    uint32_t pending_transaction_count;
    
    // Sender characteristics
    std::string from_address;
    uint64_t sender_nonce_count;
    double sender_historical_success_rate;
    bool is_known_mev_bot;
    bool is_exchange_address;
    
    // Timing factors
    std::chrono::system_clock::time_point submission_time;
    uint32_t blocks_since_submission;
    uint32_t time_of_day_minutes; // 0-1439
    uint32_t day_of_week; // 0-6
    
    // Competition factors
    uint32_t competing_transactions_same_nonce;
    uint32_t similar_gas_price_transactions;
    double gas_price_percentile;
    
    TransactionFeatures() : gas_price_gwei(0), gas_limit(0), transaction_value_wei(0),
                           nonce(0), data_size_bytes(0), is_contract_call(false),
                           max_fee_per_gas(0), max_priority_fee_per_gas(0), base_fee_at_submission(0),
                           mempool_size(0), median_gas_price(0), network_congestion_score(0.0),
                           pending_transaction_count(0), sender_nonce_count(0),
                           sender_historical_success_rate(0.0), is_known_mev_bot(false),
                           is_exchange_address(false), blocks_since_submission(0),
                           time_of_day_minutes(0), day_of_week(0), competing_transactions_same_nonce(0),
                           similar_gas_price_transactions(0), gas_price_percentile(0.0) {}
};

// Inclusion probability result
struct InclusionProbability {
    // Core probability estimates
    double probability_next_block;
    double probability_1_block;
    double probability_3_blocks;
    double probability_5_blocks;
    double probability_10_blocks;
    double probability_20_blocks;
    
    // Confidence intervals (95%)
    double prob_lower_bound;
    double prob_upper_bound;
    
    // Expected inclusion metrics
    double expected_blocks_to_inclusion;
    double expected_time_to_inclusion_seconds;
    uint32_t median_blocks_to_inclusion;
    
    // Risk factors
    double probability_of_replacement;
    double probability_of_drop;
    double probability_of_timeout;
    double overall_risk_score;
    
    // Market context impact
    double congestion_impact_factor;
    double gas_price_impact_factor;
    double competition_impact_factor;
    
    // Model metadata
    std::string model_version;
    double model_confidence;
    std::chrono::system_clock::time_point calculation_time;
    std::vector<std::string> key_factors;
    
    InclusionProbability() : probability_next_block(0.0), probability_1_block(0.0),
                            probability_3_blocks(0.0), probability_5_blocks(0.0),
                            probability_10_blocks(0.0), probability_20_blocks(0.0),
                            prob_lower_bound(0.0), prob_upper_bound(0.0),
                            expected_blocks_to_inclusion(0.0), expected_time_to_inclusion_seconds(0.0),
                            median_blocks_to_inclusion(0), probability_of_replacement(0.0),
                            probability_of_drop(0.0), probability_of_timeout(0.0),
                            overall_risk_score(0.0), congestion_impact_factor(1.0),
                            gas_price_impact_factor(1.0), competition_impact_factor(1.0),
                            model_confidence(0.0) {}
};

// Historical inclusion data point
struct InclusionDataPoint {
    std::string transaction_hash;
    TransactionFeatures features;
    InclusionOutcome outcome;
    uint32_t blocks_to_inclusion;
    uint32_t actual_gas_price_paid;
    uint64_t block_number_included;
    std::chrono::system_clock::time_point inclusion_time;
    
    // Competition data
    uint32_t total_competing_transactions;
    uint32_t higher_gas_price_competitors;
    
    InclusionDataPoint() : outcome(InclusionOutcome::UNKNOWN), blocks_to_inclusion(0),
                          actual_gas_price_paid(0), block_number_included(0),
                          total_competing_transactions(0), higher_gas_price_competitors(0) {}
};

// Model performance metrics
struct ModelPerformanceMetrics {
    // Calibration metrics
    double brier_score;
    double log_loss;
    double auc_roc;
    double calibration_slope;
    double calibration_intercept;
    
    // Prediction accuracy by time horizon
    std::unordered_map<uint32_t, double> accuracy_by_blocks; // blocks -> accuracy
    std::unordered_map<uint32_t, double> precision_by_blocks;
    std::unordered_map<uint32_t, double> recall_by_blocks;
    
    // Feature importance
    std::unordered_map<std::string, double> feature_importance;
    
    // Recent performance
    double accuracy_last_100_predictions;
    double accuracy_last_24h;
    double accuracy_last_7d;
    
    std::chrono::system_clock::time_point last_updated;
    uint64_t total_predictions;
    
    ModelPerformanceMetrics() : brier_score(0.0), log_loss(0.0), auc_roc(0.0),
                               calibration_slope(0.0), calibration_intercept(0.0),
                               accuracy_last_100_predictions(0.0), accuracy_last_24h(0.0),
                               accuracy_last_7d(0.0), total_predictions(0) {}
};

// Model configuration
struct ModelConfig {
    // Training data settings
    uint32_t training_window_blocks = 10000;
    uint32_t max_training_samples = 100000;
    uint32_t min_training_samples = 1000;
    double train_validation_split = 0.8;
    
    // Model types to use
    bool use_logistic_regression = true;
    bool use_random_forest = true;
    bool use_gradient_boosting = true;
    bool use_neural_network = false;
    bool use_ensemble = true;
    
    // Feature engineering
    bool enable_interaction_features = true;
    bool enable_polynomial_features = false;
    uint32_t max_polynomial_degree = 2;
    bool enable_time_features = true;
    bool enable_market_regime_features = true;
    
    // Model parameters
    double regularization_strength = 0.01;
    uint32_t max_tree_depth = 10;
    uint32_t n_estimators = 100;
    double learning_rate = 0.1;
    uint32_t neural_network_hidden_layers = 2;
    uint32_t neural_network_neurons_per_layer = 64;
    
    // Training settings
    uint32_t retraining_frequency_blocks = 1000;
    bool enable_online_learning = true;
    double online_learning_rate = 0.001;
    bool enable_hyperparameter_tuning = false;
    
    // Prediction settings
    std::vector<uint32_t> prediction_horizons = {1, 3, 5, 10, 20};
    bool enable_uncertainty_quantification = true;
    uint32_t monte_carlo_samples = 1000;
    
    // Performance settings
    uint32_t cache_size = 10000;
    uint32_t cache_ttl_seconds = 300;
    uint32_t max_concurrent_predictions = 4;
    uint32_t prediction_timeout_ms = 1000;
    
    // Data collection
    bool collect_mempool_snapshots = true;
    uint32_t snapshot_frequency_seconds = 30;
    bool track_failed_transactions = true;
    bool track_replacement_transactions = true;
};

// Model statistics
struct ModelStats {
    std::atomic<uint64_t> total_predictions{0};
    std::atomic<uint64_t> successful_predictions{0};
    std::atomic<uint64_t> failed_predictions{0};
    std::atomic<uint64_t> cache_hits{0};
    std::atomic<uint64_t> cache_misses{0};
    
    std::atomic<double> avg_prediction_time_ms{0.0};
    std::atomic<double> avg_prediction_accuracy{0.0};
    std::atomic<double> current_model_auc{0.0};
    std::atomic<double> calibration_score{0.0};
    
    std::chrono::system_clock::time_point last_reset;
};

// Main Inclusion Probability Model Class
class InclusionProbabilityModel {
public:
    explicit InclusionProbabilityModel(const ModelConfig& config);
    ~InclusionProbabilityModel();

    // Core prediction functionality
    InclusionProbability predict_inclusion_probability(const TransactionFeatures& features);
    InclusionProbability predict_inclusion_probability(const Transaction& tx);
    std::vector<InclusionProbability> predict_batch(const std::vector<TransactionFeatures>& features_batch);
    
    // Quick probability estimates
    double get_inclusion_probability_next_block(const TransactionFeatures& features);
    double get_inclusion_probability_blocks(const TransactionFeatures& features, uint32_t blocks);
    uint32_t get_expected_inclusion_blocks(const TransactionFeatures& features);
    
    // Priority-based predictions
    InclusionProbability predict_for_priority_level(PriorityLevel priority, uint64_t current_base_fee);
    std::vector<InclusionProbability> predict_priority_scenarios(const TransactionFeatures& base_features);
    
    // Gas price optimization
    uint64_t find_optimal_gas_price(const TransactionFeatures& features, double target_probability);
    std::vector<std::pair<uint64_t, double>> get_gas_price_probability_curve(const TransactionFeatures& features);
    uint64_t get_minimum_gas_for_probability(const TransactionFeatures& features, double target_prob);
    
    // Model training and management
    void train_model();
    void retrain_model();
    void add_training_data(const InclusionDataPoint& data_point);
    void add_training_batch(const std::vector<InclusionDataPoint>& data_batch);
    void update_model_online(const InclusionDataPoint& data_point);
    
    // Model validation and performance
    ModelPerformanceMetrics evaluate_model();
    ModelPerformanceMetrics cross_validate_model(uint32_t folds = 5);
    void calibrate_model();
    std::vector<double> get_prediction_errors() const;
    
    // Feature analysis
    std::unordered_map<std::string, double> get_feature_importance() const;
    std::vector<std::string> get_most_important_features(uint32_t top_n = 10) const;
    void analyze_feature_correlations();
    void perform_feature_selection();
    
    // Data management
    void add_inclusion_data(const std::string& tx_hash, const TransactionFeatures& features, 
                           InclusionOutcome outcome, uint32_t blocks_to_inclusion);
    std::vector<InclusionDataPoint> get_training_data(uint32_t max_samples = 0) const;
    void clean_old_training_data(std::chrono::hours max_age);
    
    // Real-time monitoring
    using PredictionCallback = std::function<void(const std::string&, const InclusionProbability&)>;
    void register_prediction_callback(PredictionCallback callback);
    void start_real_time_monitoring();
    void stop_real_time_monitoring();
    bool is_monitoring() const;
    
    // Configuration management
    void update_config(const ModelConfig& config);
    ModelConfig get_config() const;
    void enable_model_type(const std::string& model_type);
    void disable_model_type(const std::string& model_type);
    
    // Statistics and monitoring
    ModelStats get_statistics() const;
    void reset_statistics();
    ModelPerformanceMetrics get_performance_metrics() const;
    double get_current_accuracy() const;
    
    // Advanced analytics
    std::vector<TransactionFeatures> find_similar_transactions(const TransactionFeatures& features, 
                                                              uint32_t max_results = 10);
    double calculate_transaction_similarity(const TransactionFeatures& a, const TransactionFeatures& b);
    std::unordered_map<std::string, double> analyze_failure_patterns();
    
    // Market analysis
    double estimate_network_congestion_impact(double congestion_level);
    std::vector<std::pair<uint32_t, double>> get_inclusion_probability_by_time_of_day();
    std::vector<std::pair<uint32_t, double>> get_inclusion_probability_by_day_of_week();
    
    // Backtesting and simulation
    std::vector<InclusionProbability> backtest_model(const std::vector<InclusionDataPoint>& test_data);
    double simulate_inclusion_success_rate(const std::vector<TransactionFeatures>& scenarios);
    void stress_test_model(const std::vector<TransactionFeatures>& extreme_scenarios);

private:
    ModelConfig config_;
    std::atomic<bool> monitoring_;
    mutable std::mutex config_mutex_;
    
    // Training data storage
    std::vector<InclusionDataPoint> training_data_;
    std::queue<InclusionDataPoint> recent_data_;
    mutable std::mutex data_mutex_;
    
    // Machine learning models
    std::unique_ptr<class LogisticRegressionModel> logistic_model_;
    std::unique_ptr<class RandomForestModel> forest_model_;
    std::unique_ptr<class GradientBoostingModel> gbm_model_;
    std::unique_ptr<class NeuralNetworkModel> nn_model_;
    std::unique_ptr<class EnsembleModel> ensemble_model_;
    
    // Feature engineering
    std::unique_ptr<class FeatureEngineer> feature_engineer_;
    std::unordered_map<std::string, double> feature_importance_;
    
    // Performance tracking
    ModelPerformanceMetrics performance_metrics_;
    std::vector<std::pair<InclusionProbability, InclusionOutcome>> prediction_history_;
    
    // Caching
    std::unordered_map<std::string, InclusionProbability> prediction_cache_;
    mutable std::mutex cache_mutex_;
    
    // Real-time monitoring
    std::thread monitoring_thread_;
    std::vector<PredictionCallback> prediction_callbacks_;
    mutable std::mutex callbacks_mutex_;
    
    // Statistics
    mutable ModelStats stats_;
    
    // Internal prediction methods
    InclusionProbability predict_with_ensemble(const TransactionFeatures& features);
    InclusionProbability predict_with_single_model(const TransactionFeatures& features, 
                                                   const std::string& model_type);
    std::vector<double> extract_model_features(const TransactionFeatures& features);
    
    // Model training helpers
    void prepare_training_data();
    void split_training_validation_data(std::vector<InclusionDataPoint>& train_data,
                                       std::vector<InclusionDataPoint>& val_data);
    void tune_hyperparameters();
    void update_ensemble_weights();
    
    // Feature engineering
    std::vector<double> engineer_features(const TransactionFeatures& raw_features);
    void create_interaction_features(std::vector<double>& features);
    void add_polynomial_features(std::vector<double>& features);
    void normalize_features(std::vector<double>& features);
    
    // Performance evaluation
    void update_performance_metrics();
    double calculate_brier_score(const std::vector<std::pair<double, bool>>& predictions_outcomes);
    double calculate_auc_roc(const std::vector<std::pair<double, bool>>& predictions_outcomes);
    void calibrate_probabilities();
    
    // Utility methods
    std::string generate_cache_key(const TransactionFeatures& features);
    void update_cache(const std::string& key, const InclusionProbability& prediction);
    std::optional<InclusionProbability> get_from_cache(const std::string& key);
    void cleanup_expired_cache();
    
    // Real-time monitoring worker
    void monitoring_worker();
    void notify_prediction_callbacks(const std::string& tx_hash, const InclusionProbability& prediction);
    
    // Statistics update
    void update_statistics(bool success, double prediction_time_ms, double accuracy = 0.0);
    
    // Data validation
    bool validate_features(const TransactionFeatures& features);
    void clean_training_data();
    void handle_data_imbalance();
};

// Utility functions
std::string inclusion_outcome_to_string(InclusionOutcome outcome);
InclusionOutcome string_to_inclusion_outcome(const std::string& str);
std::string priority_level_to_string(PriorityLevel level);
TransactionFeatures extract_features_from_transaction(const Transaction& tx);
bool is_reasonable_inclusion_probability(const InclusionProbability& prob);
double calculate_inclusion_score(const InclusionProbability& prob);

} // namespace mm
} // namespace hfx
