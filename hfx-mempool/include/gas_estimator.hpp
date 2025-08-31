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

namespace hfx {
namespace mempool {

// Forward declarations
struct Transaction;
struct BlockInfo;

// Gas price tiers
enum class GasPriceTier {
    ECONOMY = 0,     // Slow, lowest cost
    STANDARD,        // Normal speed
    FAST,           // Fast confirmation
    INSTANT,        // Immediate inclusion
    ULTRA_FAST,     // MEV/arbitrage level
    CUSTOM
};

// Gas estimation result
struct GasEstimate {
    uint64_t gas_price_wei;
    uint64_t gas_limit;
    uint64_t max_fee_per_gas;      // EIP-1559
    uint64_t max_priority_fee;     // EIP-1559
    uint64_t base_fee;             // Current base fee
    
    // Estimates for different tiers
    std::unordered_map<GasPriceTier, uint64_t> tier_gas_prices;
    
    // Timing estimates
    uint32_t estimated_confirmation_blocks;
    uint32_t estimated_confirmation_seconds;
    double confidence_level;
    
    // Cost estimates
    double total_cost_eth;
    double total_cost_usd;
    
    // Market context
    uint64_t mempool_congestion_level;  // 0-100
    uint32_t pending_transaction_count;
    double gas_price_volatility;
    
    std::chrono::system_clock::time_point timestamp;
    
    GasEstimate() : gas_price_wei(0), gas_limit(0), max_fee_per_gas(0),
                   max_priority_fee(0), base_fee(0), estimated_confirmation_blocks(0),
                   estimated_confirmation_seconds(0), confidence_level(0.0),
                   total_cost_eth(0.0), total_cost_usd(0.0),
                   mempool_congestion_level(0), pending_transaction_count(0),
                   gas_price_volatility(0.0) {}
};

// Historical gas data point
struct GasDataPoint {
    uint64_t gas_price;
    uint64_t block_number;
    uint32_t confirmation_time_seconds;
    uint64_t base_fee;
    uint32_t transactions_in_block;
    std::chrono::system_clock::time_point timestamp;
    
    GasDataPoint() : gas_price(0), block_number(0), confirmation_time_seconds(0),
                    base_fee(0), transactions_in_block(0) {}
};

// Gas price prediction model
struct PredictionModel {
    std::string name;
    std::string description;
    double accuracy_score;
    std::chrono::system_clock::time_point last_trained;
    std::vector<double> model_parameters;
    bool is_active;
    
    PredictionModel() : accuracy_score(0.0), is_active(false) {}
};

// Network congestion metrics
struct CongestionMetrics {
    uint32_t pending_tx_count;
    uint64_t avg_gas_price;
    uint64_t median_gas_price;
    uint64_t min_gas_price;
    uint64_t max_gas_price;
    double gas_price_std_dev;
    
    uint32_t mempool_size_mb;
    double block_utilization;
    uint32_t avg_block_time_seconds;
    
    // Trend indicators
    double gas_price_trend_1h;      // % change
    double gas_price_trend_24h;     // % change
    double congestion_trend;        // increasing/decreasing
    
    std::chrono::system_clock::time_point timestamp;
    
    CongestionMetrics() : pending_tx_count(0), avg_gas_price(0), median_gas_price(0),
                         min_gas_price(0), max_gas_price(0), gas_price_std_dev(0.0),
                         mempool_size_mb(0), block_utilization(0.0), avg_block_time_seconds(0),
                         gas_price_trend_1h(0.0), gas_price_trend_24h(0.0), congestion_trend(0.0) {}
};

// Estimator configuration
struct EstimatorConfig {
    uint32_t chain_id = 1;
    std::vector<std::string> rpc_endpoints;
    
    // Data collection
    uint32_t historical_blocks = 100;
    uint32_t sample_interval_seconds = 30;
    bool collect_mempool_data = true;
    bool collect_block_data = true;
    
    // Prediction settings
    std::string default_model = "linear_regression";
    double confidence_threshold = 0.8;
    uint32_t prediction_horizon_blocks = 5;
    
    // Performance settings
    uint32_t cache_size = 1000;
    uint32_t cache_ttl_seconds = 60;
    bool enable_fast_estimates = true;
    
    // EIP-1559 settings
    bool use_eip1559 = true;
    double base_fee_multiplier = 1.125;
    uint64_t min_priority_fee = 1000000000; // 1 gwei
    uint64_t max_priority_fee = 10000000000; // 10 gwei
    
    // Safety settings
    uint64_t max_gas_price = 1000000000000; // 1000 gwei
    double safety_multiplier = 1.1;
    uint32_t max_confirmation_blocks = 20;
};

// Estimator statistics
struct EstimatorStats {
    std::atomic<uint64_t> total_estimates{0};
    std::atomic<uint64_t> successful_estimates{0};
    std::atomic<uint64_t> failed_estimates{0};
    std::atomic<uint64_t> cache_hits{0};
    std::atomic<uint64_t> cache_misses{0};
    
    std::atomic<double> avg_estimate_time_ms{0.0};
    std::atomic<double> avg_accuracy_score{0.0};
    std::atomic<double> avg_prediction_error{0.0};
    
    std::chrono::system_clock::time_point last_reset;
};

// Main Gas Estimator Class
class GasEstimator {
public:
    explicit GasEstimator(const EstimatorConfig& config);
    ~GasEstimator();

    // Core estimation functionality
    GasEstimate estimate_gas(const Transaction& tx);
    GasEstimate estimate_gas_for_tier(const Transaction& tx, GasPriceTier tier);
    std::vector<GasEstimate> estimate_gas_batch(const std::vector<Transaction>& transactions);
    
    // Quick estimates
    uint64_t get_fast_gas_price();
    uint64_t get_standard_gas_price();
    uint64_t get_economy_gas_price();
    uint64_t get_instant_gas_price();
    
    // EIP-1559 estimates
    std::pair<uint64_t, uint64_t> estimate_eip1559_fees(const Transaction& tx);
    uint64_t estimate_max_fee_per_gas(uint32_t target_blocks = 3);
    uint64_t estimate_priority_fee(GasPriceTier tier = GasPriceTier::STANDARD);
    
    // Advanced estimation
    GasEstimate estimate_with_deadline(const Transaction& tx, 
                                     std::chrono::system_clock::time_point deadline);
    GasEstimate estimate_for_profit_margin(const Transaction& tx, double min_profit_usd);
    std::vector<GasEstimate> estimate_multiple_scenarios(const Transaction& tx);
    
    // Market analysis
    CongestionMetrics get_congestion_metrics();
    double get_network_utilization();
    uint32_t get_mempool_size();
    double predict_gas_price(uint32_t blocks_ahead);
    
    // Historical data
    std::vector<GasDataPoint> get_historical_data(uint32_t blocks = 100);
    std::vector<uint64_t> get_gas_price_history(std::chrono::hours hours);
    double calculate_gas_price_volatility(std::chrono::hours window);
    
    // Prediction models
    void train_prediction_model(const std::string& model_name);
    std::vector<PredictionModel> get_available_models();
    void set_active_model(const std::string& model_name);
    double evaluate_model_accuracy(const std::string& model_name);
    
    // Real-time monitoring
    void start_real_time_monitoring();
    void stop_real_time_monitoring();
    bool is_monitoring() const;
    
    // Callbacks for real-time updates
    using GasPriceCallback = std::function<void(uint64_t new_gas_price)>;
    using CongestionCallback = std::function<void(const CongestionMetrics&)>;
    void register_gas_price_callback(GasPriceCallback callback);
    void register_congestion_callback(CongestionCallback callback);
    
    // Configuration
    void update_config(const EstimatorConfig& config);
    EstimatorConfig get_config() const;
    void add_rpc_endpoint(const std::string& endpoint);
    void remove_rpc_endpoint(const std::string& endpoint);
    
    // Statistics and monitoring
    EstimatorStats get_statistics() const;
    void reset_statistics();
    double get_accuracy_rate() const;
    double get_average_error() const;
    
    // Validation and testing
    bool validate_estimate(const GasEstimate& estimate);
    double test_estimate_accuracy(const GasEstimate& estimate, uint32_t actual_blocks);
    std::vector<double> backtest_model(const std::string& model_name, uint32_t test_blocks);
    
    // Optimization
    void optimize_gas_for_mev(Transaction& tx, double target_profit_usd);
    uint64_t calculate_optimal_gas_price(const Transaction& tx, uint32_t max_blocks);
    bool should_increase_gas_price(const Transaction& tx, uint32_t blocks_waiting);

private:
    EstimatorConfig config_;
    std::atomic<bool> monitoring_;
    mutable std::mutex config_mutex_;
    
    // Data storage
    std::vector<GasDataPoint> historical_data_;
    std::queue<GasDataPoint> recent_data_;
    std::unordered_map<uint64_t, BlockInfo> block_cache_;
    mutable std::mutex data_mutex_;
    
    // Prediction models
    std::unordered_map<std::string, PredictionModel> models_;
    std::string active_model_;
    mutable std::mutex models_mutex_;
    
    // Caches
    std::unordered_map<std::string, GasEstimate> estimate_cache_;
    mutable std::mutex cache_mutex_;
    
    // Statistics
    mutable EstimatorStats stats_;
    
    // Real-time monitoring
    std::thread monitoring_thread_;
    std::vector<GasPriceCallback> gas_price_callbacks_;
    std::vector<CongestionCallback> congestion_callbacks_;
    mutable std::mutex callbacks_mutex_;
    
    // RPC connections
    std::unordered_map<std::string, std::unique_ptr<class RPCConnection>> rpc_connections_;
    
    // Internal estimation methods
    GasEstimate create_base_estimate(const Transaction& tx);
    void apply_tier_adjustments(GasEstimate& estimate, GasPriceTier tier);
    void apply_eip1559_pricing(GasEstimate& estimate);
    void calculate_timing_estimates(GasEstimate& estimate);
    void calculate_cost_estimates(GasEstimate& estimate);
    
    // Data collection
    void collect_mempool_data();
    void collect_block_data();
    void update_historical_data(const GasDataPoint& data_point);
    CongestionMetrics calculate_congestion_metrics();
    
    // Prediction algorithms
    uint64_t predict_linear_regression(uint32_t blocks_ahead);
    uint64_t predict_moving_average(uint32_t blocks_ahead);
    uint64_t predict_exponential_smoothing(uint32_t blocks_ahead);
    uint64_t predict_machine_learning(uint32_t blocks_ahead);
    
    // Model training
    void train_linear_regression();
    void train_moving_average();
    void train_exponential_smoothing();
    void update_model_accuracy(const std::string& model_name, double accuracy);
    
    // Utility methods
    std::string generate_cache_key(const Transaction& tx, GasPriceTier tier);
    void update_cache(const std::string& key, const GasEstimate& estimate);
    std::optional<GasEstimate> get_from_cache(const std::string& key);
    void cleanup_expired_cache();
    
    // Real-time monitoring worker
    void monitoring_worker();
    void notify_gas_price_callbacks(uint64_t gas_price);
    void notify_congestion_callbacks(const CongestionMetrics& metrics);
    
    // RPC interaction
    uint64_t fetch_current_gas_price(const std::string& endpoint);
    uint64_t fetch_base_fee(const std::string& endpoint);
    uint32_t fetch_pending_transaction_count(const std::string& endpoint);
    BlockInfo fetch_latest_block(const std::string& endpoint);
    
    // Statistics update
    void update_statistics(bool success, double estimate_time_ms, double accuracy = 0.0);
};

// Utility functions
std::string gas_price_tier_to_string(GasPriceTier tier);
GasPriceTier string_to_gas_price_tier(const std::string& str);
uint64_t wei_to_gwei(uint64_t wei);
uint64_t gwei_to_wei(uint64_t gwei);
double calculate_transaction_cost_usd(uint64_t gas_price, uint64_t gas_limit, double eth_price);
bool is_reasonable_gas_price(uint64_t gas_price, uint32_t chain_id);

} // namespace mempool
} // namespace hfx
