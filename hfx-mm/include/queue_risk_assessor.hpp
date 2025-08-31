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
struct Transaction;
struct BlockInfo;

// Queue position risk levels
enum class QueueRiskLevel {
    VERY_LOW = 0,     // Top of queue, immediate execution likely
    LOW,              // Near top, high execution probability
    MEDIUM,           // Middle position, moderate risk
    HIGH,             // Lower position, higher competition
    VERY_HIGH,        // Bottom of queue, high replacement risk
    CRITICAL          // Likely to be dropped or replaced
};

// Queue dynamics
enum class QueueDynamics {
    STABLE = 0,       // Queue size relatively stable
    GROWING,          // Queue size increasing
    SHRINKING,        // Queue size decreasing
    VOLATILE,         // Rapid changes in queue composition
    CONGESTED,        // Heavily congested, slow processing
    CLEARING,         // Queue clearing rapidly
    UNKNOWN
};

// Transaction priority factors
enum class PriorityFactor {
    GAS_PRICE = 0,
    SENDER_REPUTATION,
    TRANSACTION_VALUE,
    MEV_POTENTIAL,
    NETWORK_ACTIVITY,
    TIME_SENSITIVITY,
    PROTOCOL_PRIORITY,
    VALIDATOR_PREFERENCE,
    CUSTOM
};

// Queue position information
struct QueuePosition {
    uint32_t position_in_queue;
    uint32_t total_queue_size;
    double position_percentile;
    
    // Competition analysis
    uint32_t transactions_ahead;
    uint32_t transactions_behind;
    uint32_t transactions_same_gas_price;
    uint32_t higher_gas_price_transactions;
    
    // Gas price context
    uint64_t transaction_gas_price;
    uint64_t queue_median_gas_price;
    uint64_t queue_max_gas_price;
    uint64_t queue_min_gas_price;
    double gas_price_percentile;
    
    // Timing estimates
    uint32_t estimated_blocks_to_execution;
    uint32_t estimated_seconds_to_execution;
    double execution_probability_next_block;
    double execution_probability_3_blocks;
    double execution_probability_5_blocks;
    
    std::chrono::system_clock::time_point assessment_time;
    
    QueuePosition() : position_in_queue(0), total_queue_size(0), position_percentile(0.0),
                     transactions_ahead(0), transactions_behind(0), transactions_same_gas_price(0),
                     higher_gas_price_transactions(0), transaction_gas_price(0),
                     queue_median_gas_price(0), queue_max_gas_price(0), queue_min_gas_price(0),
                     gas_price_percentile(0.0), estimated_blocks_to_execution(0),
                     estimated_seconds_to_execution(0), execution_probability_next_block(0.0),
                     execution_probability_3_blocks(0.0), execution_probability_5_blocks(0.0) {}
};

// Risk assessment result
struct QueueRiskAssessment {
    QueueRiskLevel risk_level;
    double risk_score;              // 0.0 (lowest) to 1.0 (highest)
    double execution_certainty;     // 0.0 (uncertain) to 1.0 (certain)
    
    // Primary risk factors
    double gas_price_competition_risk;
    double queue_position_risk;
    double timing_risk;
    double replacement_risk;
    double market_volatility_risk;
    double mev_competition_risk;
    
    // Queue dynamics impact
    QueueDynamics current_dynamics;
    double dynamics_impact_score;
    double queue_growth_rate;
    double queue_volatility;
    
    // Execution predictions
    QueuePosition current_position;
    std::vector<double> execution_probability_by_block; // probability for each of next N blocks
    uint32_t expected_execution_block;
    uint32_t worst_case_execution_block;
    
    // Competition analysis
    uint32_t direct_competitors;        // Same or similar transactions
    uint32_t mev_bot_competitors;      // Known MEV bots in queue
    uint32_t whale_transactions;       // High-value transactions ahead
    double average_competitor_gas_price;
    
    // Market context
    double network_congestion_level;
    double base_fee_trend;
    double priority_fee_trend;
    uint32_t mempool_size_trend;
    
    // Risk mitigation suggestions
    std::vector<std::string> risk_factors;
    std::vector<std::string> mitigation_suggestions;
    uint64_t suggested_gas_price_increase;
    bool should_replace_transaction;
    bool should_wait_for_better_conditions;
    
    // Metadata
    std::string assessment_method;
    double assessment_confidence;
    std::chrono::system_clock::time_point assessment_time;
    
    QueueRiskAssessment() : risk_level(QueueRiskLevel::MEDIUM), risk_score(0.5),
                           execution_certainty(0.5), gas_price_competition_risk(0.0),
                           queue_position_risk(0.0), timing_risk(0.0), replacement_risk(0.0),
                           market_volatility_risk(0.0), mev_competition_risk(0.0),
                           current_dynamics(QueueDynamics::STABLE), dynamics_impact_score(0.0),
                           queue_growth_rate(0.0), queue_volatility(0.0), expected_execution_block(0),
                           worst_case_execution_block(0), direct_competitors(0),
                           mev_bot_competitors(0), whale_transactions(0),
                           average_competitor_gas_price(0.0), network_congestion_level(0.0),
                           base_fee_trend(0.0), priority_fee_trend(0.0), mempool_size_trend(0),
                           suggested_gas_price_increase(0), should_replace_transaction(false),
                           should_wait_for_better_conditions(false), assessment_confidence(0.0) {}
};

// Transaction queue snapshot
struct QueueSnapshot {
    std::vector<std::string> transaction_hashes;
    std::vector<uint64_t> gas_prices;
    std::vector<uint64_t> transaction_values;
    std::vector<std::string> sender_addresses;
    std::vector<uint32_t> nonces;
    
    // Queue statistics
    uint32_t total_transactions;
    uint64_t median_gas_price;
    uint64_t average_gas_price;
    uint64_t gas_price_std_dev;
    uint64_t total_gas_limit;
    
    // Timing information
    std::chrono::system_clock::time_point snapshot_time;
    uint64_t block_number;
    uint32_t time_since_last_block_seconds;
    
    QueueSnapshot() : total_transactions(0), median_gas_price(0), average_gas_price(0),
                     gas_price_std_dev(0), total_gas_limit(0), block_number(0),
                     time_since_last_block_seconds(0) {}
};

// Historical queue performance data
struct QueuePerformanceData {
    std::string transaction_hash;
    QueueRiskAssessment initial_assessment;
    QueuePosition initial_position;
    
    // Actual outcomes
    bool was_executed;
    uint32_t actual_execution_block;
    uint32_t actual_blocks_waited;
    bool was_replaced;
    bool was_dropped;
    uint64_t final_gas_price_paid;
    
    // Queue evolution
    std::vector<QueuePosition> position_history;
    std::vector<QueueSnapshot> queue_snapshots;
    
    // Performance metrics
    double prediction_accuracy;
    double risk_assessment_accuracy;
    
    std::chrono::system_clock::time_point submission_time;
    std::chrono::system_clock::time_point resolution_time;
    
    QueuePerformanceData() : was_executed(false), actual_execution_block(0),
                            actual_blocks_waited(0), was_replaced(false), was_dropped(false),
                            final_gas_price_paid(0), prediction_accuracy(0.0),
                            risk_assessment_accuracy(0.0) {}
};

// Assessor configuration
struct QueueRiskConfig {
    // Data collection settings
    uint32_t queue_snapshot_frequency_seconds = 15;
    uint32_t historical_data_retention_blocks = 10000;
    bool track_mempool_evolution = true;
    bool track_competitor_behavior = true;
    
    // Risk calculation parameters
    std::vector<PriorityFactor> priority_factors;
    std::unordered_map<PriorityFactor, double> factor_weights;
    double base_risk_threshold = 0.5;
    double high_risk_threshold = 0.7;
    double critical_risk_threshold = 0.9;
    
    // Queue analysis settings
    uint32_t queue_depth_analysis = 100;      // How deep to analyze queue
    uint32_t competitor_analysis_window = 50; // Window for competitor analysis
    bool enable_mev_bot_detection = true;
    bool enable_whale_detection = true;
    uint64_t whale_threshold_eth = 10;         // ETH value threshold for whale classification
    
    // Prediction models
    bool use_statistical_models = true;
    bool use_machine_learning = true;
    bool use_simulation_models = false;
    std::string primary_model = "ensemble";
    
    // Performance settings
    uint32_t max_concurrent_assessments = 8;
    uint32_t assessment_timeout_ms = 1000;
    uint32_t cache_size = 5000;
    uint32_t cache_ttl_seconds = 30;
    
    // Chain-specific settings
    uint32_t chain_id = 1;
    std::vector<std::string> rpc_endpoints;
    bool use_eip1559 = true;
    uint32_t average_block_time_seconds = 12;
    
    // Advanced features
    bool enable_real_time_updates = true;
    bool enable_predictive_positioning = true;
    bool enable_competitor_tracking = true;
    double update_frequency_multiplier = 1.0;
};

// Assessor statistics
struct QueueRiskStats {
    std::atomic<uint64_t> total_assessments{0};
    std::atomic<uint64_t> successful_assessments{0};
    std::atomic<uint64_t> failed_assessments{0};
    std::atomic<uint64_t> cache_hits{0};
    std::atomic<uint64_t> cache_misses{0};
    
    std::atomic<double> avg_assessment_time_ms{0.0};
    std::atomic<double> avg_prediction_accuracy{0.0};
    std::atomic<double> avg_risk_score{0.0};
    std::atomic<double> execution_prediction_accuracy{0.0};
    
    std::chrono::system_clock::time_point last_reset;
};

// Main Queue Risk Assessor Class
class QueueRiskAssessor {
public:
    explicit QueueRiskAssessor(const QueueRiskConfig& config);
    ~QueueRiskAssessor();

    // Core assessment functionality
    QueueRiskAssessment assess_queue_risk(const std::string& transaction_hash);
    QueueRiskAssessment assess_queue_risk(const Transaction& tx);
    std::vector<QueueRiskAssessment> assess_batch(const std::vector<std::string>& transaction_hashes);
    
    // Queue position analysis
    QueuePosition get_queue_position(const std::string& transaction_hash);
    std::vector<QueuePosition> get_queue_positions(const std::vector<std::string>& transaction_hashes);
    uint32_t estimate_queue_position(const Transaction& tx);
    
    // Risk factor analysis
    double calculate_gas_price_risk(const Transaction& tx, const QueueSnapshot& queue_state);
    double calculate_timing_risk(const Transaction& tx);
    double calculate_replacement_risk(const Transaction& tx);
    double calculate_mev_competition_risk(const Transaction& tx);
    double calculate_market_volatility_risk();
    
    // Queue dynamics analysis
    QueueDynamics analyze_queue_dynamics();
    double calculate_queue_growth_rate();
    double calculate_queue_volatility();
    std::vector<QueueSnapshot> get_recent_queue_snapshots(uint32_t count = 10);
    
    // Competition analysis
    uint32_t count_direct_competitors(const Transaction& tx);
    std::vector<std::string> identify_mev_bot_competitors(const Transaction& tx);
    std::vector<std::string> identify_whale_transactions_ahead(const Transaction& tx);
    double estimate_average_competitor_gas_price(const Transaction& tx);
    
    // Predictive analysis
    std::vector<double> predict_execution_probabilities(const Transaction& tx, uint32_t blocks_ahead = 10);
    uint32_t predict_execution_block(const Transaction& tx);
    double predict_execution_certainty(const Transaction& tx);
    
    // Risk mitigation
    std::vector<std::string> suggest_risk_mitigation(const QueueRiskAssessment& assessment);
    uint64_t calculate_optimal_gas_price_adjustment(const Transaction& tx);
    bool should_replace_transaction(const QueueRiskAssessment& assessment);
    std::chrono::system_clock::time_point suggest_optimal_submission_time(const Transaction& tx);
    
    // Data management
    void update_queue_snapshot(const QueueSnapshot& snapshot);
    void add_performance_data(const QueuePerformanceData& data);
    QueueSnapshot capture_current_queue_state();
    std::vector<QueueSnapshot> get_historical_snapshots(std::chrono::hours window);
    
    // Real-time monitoring
    using RiskUpdateCallback = std::function<void(const std::string&, const QueueRiskAssessment&)>;
    void register_risk_callback(RiskUpdateCallback callback);
    void start_real_time_monitoring();
    void stop_real_time_monitoring();
    bool is_monitoring() const;
    
    // Model training and validation
    void train_risk_models();
    void validate_risk_predictions();
    double evaluate_prediction_accuracy();
    void calibrate_risk_scores();
    std::vector<double> backtest_assessments(uint32_t test_blocks = 1000);
    
    // Configuration management
    void update_config(const QueueRiskConfig& config);
    QueueRiskConfig get_config() const;
    void set_priority_factor_weight(PriorityFactor factor, double weight);
    void enable_real_time_updates(bool enabled);
    
    // Statistics and performance
    QueueRiskStats get_statistics() const;
    void reset_statistics();
    std::vector<QueuePerformanceData> get_recent_performance_data(uint32_t count = 100);
    double get_current_accuracy() const;
    
    // Advanced analytics
    std::unordered_map<QueueRiskLevel, uint32_t> analyze_risk_distribution();
    std::vector<std::pair<std::string, double>> get_top_risk_factors();
    std::unordered_map<std::string, double> analyze_competitor_success_rates();
    double calculate_queue_efficiency_score();
    
    // Market analysis
    double analyze_network_congestion_impact();
    std::vector<std::pair<uint32_t, double>> get_risk_by_time_of_day();
    std::vector<std::pair<uint32_t, double>> get_execution_success_by_gas_price_percentile();
    double estimate_base_fee_impact_on_queue();

private:
    QueueRiskConfig config_;
    std::atomic<bool> monitoring_;
    mutable std::mutex config_mutex_;
    
    // Queue state tracking
    std::deque<QueueSnapshot> queue_history_;
    QueueSnapshot current_queue_state_;
    mutable std::mutex queue_state_mutex_;
    
    // Performance tracking
    std::vector<QueuePerformanceData> performance_history_;
    mutable std::mutex performance_mutex_;
    
    // Risk models
    std::unique_ptr<class StatisticalRiskModel> statistical_model_;
    std::unique_ptr<class MLRiskModel> ml_model_;
    std::unique_ptr<class SimulationRiskModel> simulation_model_;
    std::unique_ptr<class EnsembleRiskModel> ensemble_model_;
    
    // Competition tracker
    std::unique_ptr<class CompetitorTracker> competitor_tracker_;
    
    // Known addresses
    std::unordered_set<std::string> known_mev_bots_;
    std::unordered_set<std::string> known_whale_addresses_;
    std::unordered_set<std::string> known_exchange_addresses_;
    mutable std::mutex address_sets_mutex_;
    
    // Caching
    std::unordered_map<std::string, QueueRiskAssessment> assessment_cache_;
    mutable std::mutex cache_mutex_;
    
    // Real-time monitoring
    std::thread monitoring_thread_;
    std::vector<RiskUpdateCallback> risk_callbacks_;
    mutable std::mutex callbacks_mutex_;
    
    // Statistics
    mutable QueueRiskStats stats_;
    
    // Internal assessment methods
    QueueRiskAssessment assess_with_statistical_model(const Transaction& tx);
    QueueRiskAssessment assess_with_ml_model(const Transaction& tx);
    QueueRiskAssessment assess_with_simulation_model(const Transaction& tx);
    QueueRiskAssessment combine_assessments(const std::vector<QueueRiskAssessment>& assessments);
    
    // Risk calculation helpers
    double calculate_position_risk(uint32_t position, uint32_t queue_size);
    double calculate_gas_price_competitiveness(uint64_t gas_price, const QueueSnapshot& queue);
    double calculate_timing_pressure(const Transaction& tx);
    
    // Queue analysis helpers
    void analyze_queue_composition(const QueueSnapshot& snapshot);
    void detect_queue_patterns();
    void update_competitor_profiles();
    
    // Prediction helpers
    std::vector<double> simulate_queue_evolution(const Transaction& tx, uint32_t blocks);
    double calculate_execution_probability_for_block(const Transaction& tx, uint32_t block_offset);
    
    // Address classification
    bool is_mev_bot_address(const std::string& address);
    bool is_whale_address(const std::string& address);
    bool is_exchange_address(const std::string& address);
    void update_address_classifications();
    
    // Utility methods
    std::string generate_cache_key(const std::string& transaction_hash);
    void update_cache(const std::string& key, const QueueRiskAssessment& assessment);
    std::optional<QueueRiskAssessment> get_from_cache(const std::string& key);
    void cleanup_expired_cache();
    
    // Real-time monitoring worker
    void monitoring_worker();
    void process_queue_updates();
    void notify_risk_callbacks(const std::string& tx_hash, const QueueRiskAssessment& assessment);
    
    // Statistics update
    void update_statistics(bool success, double assessment_time_ms, double accuracy = 0.0);
    
    // Data validation
    bool validate_transaction(const Transaction& tx);
    bool validate_queue_snapshot(const QueueSnapshot& snapshot);
};

// Utility functions
std::string queue_risk_level_to_string(QueueRiskLevel level);
QueueRiskLevel string_to_queue_risk_level(const std::string& str);
std::string queue_dynamics_to_string(QueueDynamics dynamics);
std::string priority_factor_to_string(PriorityFactor factor);
double calculate_risk_score(const QueueRiskAssessment& assessment);
bool is_high_risk_assessment(const QueueRiskAssessment& assessment);

} // namespace mm
} // namespace hfx
