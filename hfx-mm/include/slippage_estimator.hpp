#pragma once

#include <string>
#include <vector>
#include <memory>
#include <atomic>
#include <mutex>
#include <unordered_map>
#include <chrono>
#include <functional>
#include <optional>
#include <queue>

namespace hfx {
namespace mm {

// Forward declarations
struct Transaction;
struct PoolState;

// Slippage types
enum class SlippageType {
    PRICE_IMPACT = 0,      // Immediate price impact from trade size
    EXECUTION_SLIPPAGE,    // Slippage during execution window
    MEV_SLIPPAGE,         // Additional slippage from MEV attacks
    MARKET_SLIPPAGE,      // Market movement during execution
    LATENCY_SLIPPAGE,     // Slippage due to network/execution delays
    TOTAL_SLIPPAGE,       // Combined all slippage types
    CUSTOM
};

// DEX protocols
enum class DEXProtocol {
    UNISWAP_V2 = 0,
    UNISWAP_V3,
    SUSHISWAP,
    BALANCER_V2,
    CURVE,
    PANCAKESWAP_V2,
    PANCAKESWAP_V3,
    TRADERJOE,
    QUICKSWAP,
    SPOOKYSWAP,
    APESWAP,
    BEETHOVENX,
    VELODROME,
    AERODROME,
    CAMELOT,
    RAMSES,
    CUSTOM_AMM
};

// Liquidity pool information
struct LiquidityPool {
    std::string pool_address;
    DEXProtocol protocol;
    std::string token_a;
    std::string token_b;
    uint64_t reserve_a;
    uint64_t reserve_b;
    uint64_t total_liquidity;
    uint32_t fee_bps;
    
    // V3 specific
    int32_t current_tick;
    uint64_t sqrt_price_x96;
    uint64_t liquidity;
    std::vector<std::pair<int32_t, uint64_t>> tick_liquidity; // tick -> liquidity
    
    // Pool metrics
    double volume_24h;
    double tvl_usd;
    double fee_apr;
    uint32_t transaction_count_24h;
    
    std::chrono::system_clock::time_point last_updated;
    
    LiquidityPool() : protocol(DEXProtocol::UNISWAP_V2), reserve_a(0), reserve_b(0),
                     total_liquidity(0), fee_bps(0), current_tick(0), sqrt_price_x96(0),
                     liquidity(0), volume_24h(0.0), tvl_usd(0.0), fee_apr(0.0),
                     transaction_count_24h(0) {}
};

// Trade parameters
struct TradeParameters {
    std::string token_in;
    std::string token_out;
    uint64_t amount_in;
    uint64_t amount_out_expected;
    uint64_t amount_out_minimum;
    uint32_t slippage_tolerance_bps;
    
    // Execution parameters
    uint32_t deadline_blocks;
    uint64_t max_gas_price;
    bool is_exact_input;
    std::vector<std::string> route_path;
    std::vector<std::string> pools_used;
    
    // Context
    std::chrono::system_clock::time_point submission_time;
    uint32_t expected_execution_blocks;
    bool is_mev_protected;
    
    TradeParameters() : amount_in(0), amount_out_expected(0), amount_out_minimum(0),
                       slippage_tolerance_bps(0), deadline_blocks(0), max_gas_price(0),
                       is_exact_input(true), expected_execution_blocks(0), is_mev_protected(false) {}
};

// Slippage estimate result
struct SlippageEstimate {
    // Core slippage estimates (in basis points)
    double price_impact_bps;
    double execution_slippage_bps;
    double mev_slippage_bps;
    double market_slippage_bps;
    double latency_slippage_bps;
    double total_slippage_bps;
    
    // Confidence intervals (95%)
    double slippage_lower_bound_bps;
    double slippage_upper_bound_bps;
    
    // Expected amounts
    uint64_t expected_amount_out;
    uint64_t minimum_amount_out;
    uint64_t worst_case_amount_out;
    
    // Probability estimates
    double probability_within_tolerance;
    double probability_of_mev_attack;
    double probability_of_front_running;
    double probability_of_sandwich_attack;
    
    // Route analysis
    std::vector<std::string> optimal_route;
    std::vector<double> route_slippage_breakdown;
    double route_efficiency_score;
    
    // Timing factors
    uint32_t estimated_execution_time_ms;
    double time_decay_impact_bps;
    double volatility_impact_bps;
    
    // Market context
    double pool_depth_adequacy;
    double liquidity_concentration;
    double volume_to_liquidity_ratio;
    
    // Risk metrics
    double overall_risk_score;
    double execution_certainty;
    std::vector<std::string> risk_factors;
    
    // Metadata
    std::string estimation_method;
    double estimation_confidence;
    std::chrono::system_clock::time_point estimation_time;
    
    SlippageEstimate() : price_impact_bps(0.0), execution_slippage_bps(0.0), mev_slippage_bps(0.0),
                        market_slippage_bps(0.0), latency_slippage_bps(0.0), total_slippage_bps(0.0),
                        slippage_lower_bound_bps(0.0), slippage_upper_bound_bps(0.0),
                        expected_amount_out(0), minimum_amount_out(0), worst_case_amount_out(0),
                        probability_within_tolerance(0.0), probability_of_mev_attack(0.0),
                        probability_of_front_running(0.0), probability_of_sandwich_attack(0.0),
                        route_efficiency_score(0.0), estimated_execution_time_ms(0),
                        time_decay_impact_bps(0.0), volatility_impact_bps(0.0),
                        pool_depth_adequacy(0.0), liquidity_concentration(0.0),
                        volume_to_liquidity_ratio(0.0), overall_risk_score(0.0),
                        execution_certainty(0.0), estimation_confidence(0.0) {}
};

// Historical slippage data
struct SlippageDataPoint {
    std::string transaction_hash;
    TradeParameters trade_params;
    SlippageEstimate predicted_slippage;
    
    // Actual results
    double actual_slippage_bps;
    uint64_t actual_amount_out;
    bool was_mev_attacked;
    bool was_front_run;
    bool was_sandwich_attacked;
    uint32_t actual_execution_time_ms;
    
    // Market conditions
    double market_volatility;
    double pool_utilization;
    uint32_t competing_transactions;
    
    std::chrono::system_clock::time_point execution_time;
    
    SlippageDataPoint() : actual_slippage_bps(0.0), actual_amount_out(0),
                         was_mev_attacked(false), was_front_run(false),
                         was_sandwich_attacked(false), actual_execution_time_ms(0),
                         market_volatility(0.0), pool_utilization(0.0),
                         competing_transactions(0) {}
};

// Slippage model configuration
struct SlippageModelConfig {
    // Supported protocols
    std::vector<DEXProtocol> enabled_protocols;
    
    // Data collection
    uint32_t historical_trades_window = 10000;
    uint32_t pool_state_cache_size = 1000;
    uint32_t cache_ttl_seconds = 60;
    
    // Model parameters
    bool enable_machine_learning = true;
    bool enable_statistical_models = true;
    bool enable_simulation_models = true;
    std::string primary_model = "ensemble";
    
    // Feature engineering
    bool include_volatility_features = true;
    bool include_liquidity_features = true;
    bool include_volume_features = true;
    bool include_time_features = true;
    bool include_mev_features = true;
    
    // Risk assessment
    double max_acceptable_slippage_bps = 500; // 5%
    double mev_risk_threshold = 0.3;
    bool enable_sandwich_detection = true;
    bool enable_frontrun_detection = true;
    
    // Performance settings
    uint32_t max_concurrent_estimates = 8;
    uint32_t estimation_timeout_ms = 500;
    bool enable_parallel_route_analysis = true;
    
    // Chain-specific settings
    std::vector<uint32_t> supported_chains;
    std::unordered_map<uint32_t, std::vector<std::string>> chain_rpc_endpoints;
    
    // Advanced features
    bool enable_dynamic_routing = true;
    bool enable_multi_hop_analysis = true;
    uint32_t max_route_hops = 4;
    bool optimize_for_slippage = true;
    bool optimize_for_gas = false;
};

// Model performance metrics
struct SlippageModelMetrics {
    // Prediction accuracy
    double mean_absolute_error_bps;
    double root_mean_squared_error_bps;
    double median_absolute_error_bps;
    double prediction_accuracy_within_1_bps;
    double prediction_accuracy_within_5_bps;
    double prediction_accuracy_within_10_bps;
    
    // MEV detection accuracy
    double mev_detection_precision;
    double mev_detection_recall;
    double mev_detection_f1_score;
    
    // Risk assessment quality
    double risk_calibration_score;
    double false_positive_rate;
    double false_negative_rate;
    
    // Performance metrics
    double avg_estimation_time_ms;
    double max_estimation_time_ms;
    uint64_t total_estimates;
    
    // Recent performance
    double recent_accuracy_1h;
    double recent_accuracy_24h;
    double recent_accuracy_7d;
    
    std::chrono::system_clock::time_point last_updated;
    
    SlippageModelMetrics() : mean_absolute_error_bps(0.0), root_mean_squared_error_bps(0.0),
                            median_absolute_error_bps(0.0), prediction_accuracy_within_1_bps(0.0),
                            prediction_accuracy_within_5_bps(0.0), prediction_accuracy_within_10_bps(0.0),
                            mev_detection_precision(0.0), mev_detection_recall(0.0), mev_detection_f1_score(0.0),
                            risk_calibration_score(0.0), false_positive_rate(0.0), false_negative_rate(0.0),
                            avg_estimation_time_ms(0.0), max_estimation_time_ms(0.0), total_estimates(0),
                            recent_accuracy_1h(0.0), recent_accuracy_24h(0.0), recent_accuracy_7d(0.0) {}
};

// Estimator statistics
struct SlippageEstimatorStats {
    std::atomic<uint64_t> total_estimates{0};
    std::atomic<uint64_t> successful_estimates{0};
    std::atomic<uint64_t> failed_estimates{0};
    std::atomic<uint64_t> cache_hits{0};
    std::atomic<uint64_t> cache_misses{0};
    
    std::atomic<double> avg_estimation_time_ms{0.0};
    std::atomic<double> avg_prediction_accuracy{0.0};
    std::atomic<double> avg_slippage_estimate_bps{0.0};
    std::atomic<double> mev_detection_rate{0.0};
    
    std::chrono::system_clock::time_point last_reset;
};

// Main Slippage Estimator Class
class SlippageEstimator {
public:
    explicit SlippageEstimator(const SlippageModelConfig& config);
    ~SlippageEstimator();

    // Core estimation functionality
    SlippageEstimate estimate_slippage(const TradeParameters& trade);
    SlippageEstimate estimate_slippage_for_transaction(const Transaction& tx);
    std::vector<SlippageEstimate> estimate_batch(const std::vector<TradeParameters>& trades);
    
    // Quick estimates
    double get_price_impact(const std::string& pool_address, uint64_t trade_amount);
    double get_execution_slippage(const TradeParameters& trade);
    double estimate_mev_slippage(const TradeParameters& trade);
    
    // Route optimization
    std::vector<std::string> find_optimal_route(const std::string& token_in, 
                                               const std::string& token_out,
                                               uint64_t amount_in);
    SlippageEstimate compare_routes(const std::vector<std::vector<std::string>>& routes,
                                   const TradeParameters& trade);
    std::vector<SlippageEstimate> analyze_multi_hop_routes(const TradeParameters& trade);
    
    // Pool analysis
    void update_pool_state(const LiquidityPool& pool);
    LiquidityPool get_pool_state(const std::string& pool_address) const;
    std::vector<LiquidityPool> get_pools_for_pair(const std::string& token_a, 
                                                 const std::string& token_b) const;
    double calculate_pool_depth(const std::string& pool_address, uint64_t trade_amount);
    
    // Advanced slippage analysis
    SlippageEstimate estimate_time_dependent_slippage(const TradeParameters& trade, 
                                                     uint32_t execution_delay_ms);
    SlippageEstimate estimate_volatility_adjusted_slippage(const TradeParameters& trade);
    std::vector<SlippageEstimate> simulate_market_impact_scenarios(const TradeParameters& trade);
    
    // MEV protection analysis
    double estimate_sandwich_attack_probability(const TradeParameters& trade);
    double estimate_frontrun_probability(const TradeParameters& trade);
    SlippageEstimate estimate_mev_protected_slippage(const TradeParameters& trade);
    std::vector<std::string> suggest_mev_protection_strategies(const TradeParameters& trade);
    
    // Historical data and model training
    void add_slippage_data(const SlippageDataPoint& data);
    void train_slippage_models();
    void update_models_online(const SlippageDataPoint& data);
    std::vector<SlippageDataPoint> get_historical_data(uint32_t limit = 1000) const;
    
    // Model performance and validation
    SlippageModelMetrics evaluate_model_performance();
    void calibrate_models();
    void validate_predictions();
    std::vector<double> backtest_predictions(uint32_t test_days = 7);
    
    // Real-time monitoring
    using SlippageCallback = std::function<void(const SlippageEstimate&)>;
    void register_slippage_callback(SlippageCallback callback);
    void start_real_time_monitoring();
    void stop_real_time_monitoring();
    bool is_monitoring() const;
    
    // Configuration management
    void update_config(const SlippageModelConfig& config);
    SlippageModelConfig get_config() const;
    void enable_protocol(DEXProtocol protocol);
    void disable_protocol(DEXProtocol protocol);
    void add_supported_chain(uint32_t chain_id);
    
    // Statistics and metrics
    SlippageEstimatorStats get_statistics() const;
    void reset_statistics();
    SlippageModelMetrics get_model_metrics() const;
    double get_current_accuracy() const;
    
    // Advanced analytics
    std::unordered_map<DEXProtocol, double> analyze_protocol_slippage_rates();
    std::vector<std::pair<std::string, double>> get_highest_slippage_pairs();
    std::unordered_map<std::string, double> analyze_time_of_day_slippage_patterns();
    double calculate_market_impact_coefficient(const std::string& token_pair);
    
    // Risk management
    bool is_trade_within_risk_limits(const TradeParameters& trade);
    std::vector<std::string> assess_trade_risks(const TradeParameters& trade);
    double calculate_maximum_safe_trade_size(const std::string& pool_address, 
                                           double max_slippage_bps);
    
    // Optimization helpers
    TradeParameters optimize_trade_parameters(const TradeParameters& initial_params);
    uint32_t find_optimal_slippage_tolerance(const TradeParameters& trade);
    std::vector<std::string> suggest_trade_improvements(const TradeParameters& trade);

private:
    SlippageModelConfig config_;
    std::atomic<bool> monitoring_;
    mutable std::mutex config_mutex_;
    
    // Pool state cache
    std::unordered_map<std::string, LiquidityPool> pool_cache_;
    mutable std::mutex pool_cache_mutex_;
    
    // Historical slippage data
    std::vector<SlippageDataPoint> historical_data_;
    std::queue<SlippageDataPoint> recent_data_;
    mutable std::mutex data_mutex_;
    
    // Prediction models
    std::unique_ptr<class LinearSlippageModel> linear_model_;
    std::unique_ptr<class MLSlippageModel> ml_model_;
    std::unique_ptr<class SimulationSlippageModel> simulation_model_;
    std::unique_ptr<class EnsembleSlippageModel> ensemble_model_;
    
    // Route finder and optimizer
    std::unique_ptr<class RouteOptimizer> route_optimizer_;
    std::unique_ptr<class PathFinder> path_finder_;
    
    // MEV detector
    std::unique_ptr<class MEVDetector> mev_detector_;
    
    // Performance tracking
    SlippageModelMetrics model_metrics_;
    std::vector<std::pair<SlippageEstimate, double>> prediction_history_; // estimate, actual
    
    // Caching
    std::unordered_map<std::string, SlippageEstimate> estimate_cache_;
    mutable std::mutex cache_mutex_;
    
    // Real-time monitoring
    std::thread monitoring_thread_;
    std::vector<SlippageCallback> slippage_callbacks_;
    mutable std::mutex callbacks_mutex_;
    
    // Statistics
    mutable SlippageEstimatorStats stats_;
    
    // Internal estimation methods
    SlippageEstimate estimate_uniswap_v2_slippage(const TradeParameters& trade);
    SlippageEstimate estimate_uniswap_v3_slippage(const TradeParameters& trade);
    SlippageEstimate estimate_curve_slippage(const TradeParameters& trade);
    SlippageEstimate estimate_balancer_slippage(const TradeParameters& trade);
    
    // Price impact calculations
    double calculate_constant_product_impact(uint64_t reserve_in, uint64_t reserve_out, 
                                           uint64_t amount_in, uint32_t fee_bps);
    double calculate_v3_price_impact(const LiquidityPool& pool, uint64_t amount_in);
    double calculate_curve_price_impact(const LiquidityPool& pool, uint64_t amount_in);
    
    // Route analysis
    std::vector<std::vector<std::string>> find_all_routes(const std::string& token_in,
                                                         const std::string& token_out,
                                                         uint32_t max_hops);
    double calculate_route_slippage(const std::vector<std::string>& route, uint64_t amount_in);
    
    // MEV analysis
    double analyze_sandwich_vulnerability(const TradeParameters& trade);
    double analyze_frontrun_vulnerability(const TradeParameters& trade);
    double estimate_mev_profit_potential(const TradeParameters& trade);
    
    // Model training helpers
    void prepare_training_features(const std::vector<SlippageDataPoint>& data);
    void update_model_performance(const SlippageDataPoint& actual_result);
    void retrain_models_if_needed();
    
    // Utility methods
    std::string generate_cache_key(const TradeParameters& trade);
    void update_cache(const std::string& key, const SlippageEstimate& estimate);
    std::optional<SlippageEstimate> get_from_cache(const std::string& key);
    void cleanup_expired_cache();
    
    // Pool data management
    void fetch_pool_data(const std::string& pool_address);
    void update_pool_reserves(const std::string& pool_address);
    void cleanup_stale_pool_data();
    
    // Real-time monitoring worker
    void monitoring_worker();
    void notify_slippage_callbacks(const SlippageEstimate& estimate);
    
    // Statistics update
    void update_statistics(bool success, double estimation_time_ms, double accuracy = 0.0);
    
    // Validation
    bool validate_trade_parameters(const TradeParameters& trade);
    bool validate_pool_state(const LiquidityPool& pool);
};

// Utility functions
std::string slippage_type_to_string(SlippageType type);
std::string dex_protocol_to_string(DEXProtocol protocol);
DEXProtocol string_to_dex_protocol(const std::string& str);
double basis_points_to_percentage(double bps);
double percentage_to_basis_points(double percentage);
bool is_reasonable_slippage_estimate(const SlippageEstimate& estimate);
double calculate_slippage_score(const SlippageEstimate& estimate);

} // namespace mm
} // namespace hfx
