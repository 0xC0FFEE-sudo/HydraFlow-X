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
struct VolatilityEstimate;

// Correlation types
enum class CorrelationType {
    PEARSON = 0,             // Pearson linear correlation
    SPEARMAN,                // Spearman rank correlation
    KENDALL,                 // Kendall tau correlation
    MUTUAL_INFORMATION,      // Mutual information
    DISTANCE_CORRELATION,    // Distance correlation
    COPULA_CORRELATION,      // Copula-based correlation
    DYNAMIC_CORRELATION,     // Time-varying correlation
    CONDITIONAL_CORRELATION, // Conditional correlation
    TAIL_CORRELATION,        // Tail dependence
    CUSTOM
};

// Time windows for correlation calculation
enum class CorrelationWindow {
    REAL_TIME = 0,     // Continuous real-time
    MINUTE,            // 1-minute windows
    FIVE_MINUTE,       // 5-minute windows
    FIFTEEN_MINUTE,    // 15-minute windows
    HOUR,              // 1-hour windows
    FOUR_HOUR,         // 4-hour windows
    DAILY,             // Daily windows
    WEEKLY,            // Weekly windows
    MONTHLY,           // Monthly windows
    CUSTOM
};

// Market regimes for conditional correlations
enum class MarketCondition {
    NORMAL = 0,
    HIGH_VOLATILITY,
    LOW_VOLATILITY,
    TRENDING_UP,
    TRENDING_DOWN,
    SIDEWAYS,
    CRISIS,
    RECOVERY,
    BULL_MARKET,
    BEAR_MARKET,
    UNKNOWN
};

// Correlation matrix entry
struct CorrelationEntry {
    std::string symbol_1;
    std::string symbol_2;
    CorrelationType correlation_type;
    
    // Core correlation measures
    double correlation_coefficient;
    double p_value;
    double confidence_interval_lower;
    double confidence_interval_upper;
    
    // Statistical properties
    double sample_size;
    double standard_error;
    double t_statistic;
    bool is_statistically_significant;
    
    // Dynamic properties
    double correlation_volatility;
    double correlation_trend;
    std::vector<double> rolling_correlations;
    
    // Tail dependence
    double upper_tail_dependence;
    double lower_tail_dependence;
    double tail_dependence_coefficient;
    
    // Time-varying properties
    std::vector<std::pair<std::chrono::system_clock::time_point, double>> correlation_history;
    double correlation_half_life;
    double correlation_persistence;
    
    // Market condition dependencies
    std::unordered_map<MarketCondition, double> conditional_correlations;
    MarketCondition dominant_regime;
    
    // Metadata
    CorrelationWindow window_type;
    std::chrono::system_clock::time_point last_updated;
    std::chrono::system_clock::time_point data_start_time;
    std::chrono::system_clock::time_point data_end_time;
    uint32_t update_frequency_seconds;
    
    CorrelationEntry() : correlation_type(CorrelationType::PEARSON), correlation_coefficient(0.0),
                        p_value(1.0), confidence_interval_lower(0.0), confidence_interval_upper(0.0),
                        sample_size(0.0), standard_error(0.0), t_statistic(0.0),
                        is_statistically_significant(false), correlation_volatility(0.0),
                        correlation_trend(0.0), upper_tail_dependence(0.0), lower_tail_dependence(0.0),
                        tail_dependence_coefficient(0.0), correlation_half_life(0.0),
                        correlation_persistence(0.0), dominant_regime(MarketCondition::NORMAL),
                        window_type(CorrelationWindow::HOUR), update_frequency_seconds(0) {}
};

// Correlation matrix
struct CorrelationMatrix {
    std::vector<std::string> symbols;
    std::vector<std::vector<double>> correlation_coefficients;
    std::vector<std::vector<double>> p_values;
    
    // Matrix properties
    double matrix_determinant;
    double condition_number;
    std::vector<double> eigenvalues;
    std::vector<std::vector<double>> eigenvectors;
    
    // Risk metrics
    double portfolio_diversification_ratio;
    double effective_number_of_assets;
    double concentration_ratio;
    
    // Stability metrics
    double matrix_stability_score;
    double average_correlation;
    double correlation_dispersion;
    double max_correlation;
    double min_correlation;
    
    CorrelationType correlation_type;
    CorrelationWindow window_type;
    std::chrono::system_clock::time_point calculation_time;
    
    CorrelationMatrix() : matrix_determinant(0.0), condition_number(0.0),
                         portfolio_diversification_ratio(0.0), effective_number_of_assets(0.0),
                         concentration_ratio(0.0), matrix_stability_score(0.0),
                         average_correlation(0.0), correlation_dispersion(0.0),
                         max_correlation(0.0), min_correlation(0.0),
                         correlation_type(CorrelationType::PEARSON), window_type(CorrelationWindow::HOUR) {}
};

// Correlation breakdown analysis
struct CorrelationBreakdown {
    std::string symbol_pair;
    
    // Component analysis
    double systematic_correlation;      // Market-driven correlation
    double idiosyncratic_correlation;   // Asset-specific correlation
    double sector_correlation;          // Sector-driven correlation
    double volatility_correlation;      // Volatility-driven correlation
    
    // Frequency decomposition
    std::vector<double> frequency_correlations;  // Correlation by frequency band
    std::vector<std::string> frequency_labels;
    double high_frequency_correlation;
    double medium_frequency_correlation;
    double low_frequency_correlation;
    
    // Lead-lag relationships
    double lead_lag_correlation;
    int32_t optimal_lag_periods;
    std::vector<double> cross_correlations;  // At different lags
    
    // Causality measures
    double granger_causality_x_to_y;
    double granger_causality_y_to_x;
    double mutual_information_score;
    double transfer_entropy;
    
    std::chrono::system_clock::time_point analysis_time;
    
    CorrelationBreakdown() : systematic_correlation(0.0), idiosyncratic_correlation(0.0),
                            sector_correlation(0.0), volatility_correlation(0.0),
                            high_frequency_correlation(0.0), medium_frequency_correlation(0.0),
                            low_frequency_correlation(0.0), lead_lag_correlation(0.0),
                            optimal_lag_periods(0), granger_causality_x_to_y(0.0),
                            granger_causality_y_to_x(0.0), mutual_information_score(0.0),
                            transfer_entropy(0.0) {}
};

// Correlation clustering result
struct CorrelationCluster {
    uint32_t cluster_id;
    std::vector<std::string> member_symbols;
    double average_intra_cluster_correlation;
    double cluster_coherence_score;
    
    // Cluster characteristics
    std::string cluster_name;
    std::string dominant_sector;
    double cluster_volatility;
    double cluster_risk_contribution;
    
    // Temporal stability
    double cluster_stability_score;
    std::vector<std::chrono::system_clock::time_point> membership_changes;
    
    CorrelationCluster() : cluster_id(0), average_intra_cluster_correlation(0.0),
                          cluster_coherence_score(0.0), cluster_volatility(0.0),
                          cluster_risk_contribution(0.0), cluster_stability_score(0.0) {}
};

// Correlation tracker configuration
struct CorrelationConfig {
    // Symbols to track
    std::vector<std::string> tracked_symbols;
    bool auto_add_new_symbols = true;
    uint32_t max_symbols = 1000;
    
    // Correlation types to calculate
    std::vector<CorrelationType> enabled_correlation_types;
    CorrelationType primary_correlation_type = CorrelationType::PEARSON;
    
    // Time windows
    std::vector<CorrelationWindow> enabled_windows;
    CorrelationWindow primary_window = CorrelationWindow::HOUR;
    uint32_t custom_window_seconds = 3600;
    
    // Data requirements
    uint32_t min_observations = 30;
    uint32_t max_observations = 10000;
    uint32_t rolling_window_size = 100;
    bool use_overlapping_windows = true;
    
    // Statistical settings
    double significance_level = 0.05;
    bool calculate_confidence_intervals = true;
    bool perform_stationarity_tests = true;
    bool adjust_for_multiple_testing = true;
    
    // Dynamic correlation settings
    bool enable_dynamic_correlations = true;
    double correlation_decay_factor = 0.95;
    uint32_t correlation_forecast_horizon = 10;
    
    // Conditional correlation settings
    bool enable_conditional_correlations = true;
    std::vector<MarketCondition> tracked_conditions;
    double volatility_threshold_high = 0.02;  // 2% daily vol
    double volatility_threshold_low = 0.005;  // 0.5% daily vol
    
    // Clustering settings
    bool enable_correlation_clustering = true;
    uint32_t max_clusters = 10;
    double clustering_threshold = 0.7;
    std::string clustering_method = "hierarchical";
    
    // Performance settings
    uint32_t max_concurrent_calculations = 4;
    uint32_t calculation_timeout_ms = 5000;
    uint32_t cache_size = 10000;
    uint32_t cache_ttl_seconds = 300;
    
    // Real-time settings
    bool enable_real_time_updates = true;
    uint32_t update_frequency_seconds = 60;
    bool stream_correlation_updates = false;
    double correlation_change_threshold = 0.1;  // Alert on 10% change
    
    // Advanced features
    bool enable_copula_correlations = false;
    bool enable_tail_dependence = true;
    bool enable_lead_lag_analysis = true;
    bool enable_frequency_decomposition = false;
    bool enable_causality_analysis = false;
};

// Tracker statistics
struct CorrelationTrackerStats {
    std::atomic<uint64_t> total_calculations{0};
    std::atomic<uint64_t> successful_calculations{0};
    std::atomic<uint64_t> failed_calculations{0};
    std::atomic<uint64_t> cache_hits{0};
    std::atomic<uint64_t> cache_misses{0};
    
    std::atomic<double> avg_calculation_time_ms{0.0};
    std::atomic<double> avg_correlation_coefficient{0.0};
    std::atomic<double> correlation_matrix_update_frequency{0.0};
    std::atomic<uint32_t> active_symbol_pairs{0};
    
    std::chrono::system_clock::time_point last_reset;
};

// Main Correlation Tracker Class
class CorrelationTracker {
public:
    explicit CorrelationTracker(const CorrelationConfig& config);
    ~CorrelationTracker();

    // Core correlation calculation
    CorrelationEntry calculate_correlation(const std::string& symbol1, const std::string& symbol2,
                                         CorrelationType type = CorrelationType::PEARSON,
                                         CorrelationWindow window = CorrelationWindow::HOUR);
    CorrelationMatrix calculate_correlation_matrix(const std::vector<std::string>& symbols,
                                                 CorrelationType type = CorrelationType::PEARSON,
                                                 CorrelationWindow window = CorrelationWindow::HOUR);
    std::vector<CorrelationEntry> calculate_all_pairs(const std::vector<std::string>& symbols);
    
    // Historical correlation analysis
    std::vector<CorrelationEntry> get_correlation_history(const std::string& symbol1, 
                                                         const std::string& symbol2,
                                                         std::chrono::hours lookback_period);
    std::vector<double> get_rolling_correlation(const std::string& symbol1, const std::string& symbol2,
                                              uint32_t window_size);
    CorrelationEntry calculate_average_correlation(const std::string& symbol1, const std::string& symbol2,
                                                 std::chrono::hours period);
    
    // Dynamic correlation analysis
    std::vector<double> forecast_correlation(const std::string& symbol1, const std::string& symbol2,
                                           uint32_t periods_ahead);
    double estimate_correlation_volatility(const std::string& symbol1, const std::string& symbol2);
    double calculate_correlation_half_life(const std::string& symbol1, const std::string& symbol2);
    
    // Conditional correlation analysis
    std::unordered_map<MarketCondition, double> calculate_conditional_correlations(
        const std::string& symbol1, const std::string& symbol2);
    CorrelationEntry get_correlation_for_condition(const std::string& symbol1, const std::string& symbol2,
                                                  MarketCondition condition);
    double get_regime_adjusted_correlation(const std::string& symbol1, const std::string& symbol2);
    
    // Correlation breakdown and decomposition
    CorrelationBreakdown analyze_correlation_components(const std::string& symbol1, const std::string& symbol2);
    std::vector<double> decompose_correlation_by_frequency(const std::string& symbol1, const std::string& symbol2);
    std::pair<double, int32_t> analyze_lead_lag_relationship(const std::string& symbol1, const std::string& symbol2);
    
    // Correlation clustering
    std::vector<CorrelationCluster> perform_correlation_clustering(const std::vector<std::string>& symbols);
    CorrelationCluster find_symbol_cluster(const std::string& symbol);
    std::vector<std::string> find_highly_correlated_symbols(const std::string& symbol, double threshold = 0.7);
    std::vector<std::string> find_uncorrelated_symbols(const std::string& symbol, double threshold = 0.3);
    
    // Matrix analysis
    double calculate_portfolio_diversification_ratio(const std::vector<std::string>& symbols,
                                                    const std::vector<double>& weights);
    double calculate_effective_number_of_assets(const CorrelationMatrix& matrix);
    std::vector<double> calculate_correlation_eigenvalues(const CorrelationMatrix& matrix);
    double assess_matrix_stability(const CorrelationMatrix& matrix);
    
    // Data management
    void add_symbol(const std::string& symbol);
    void remove_symbol(const std::string& symbol);
    void update_price_data(const std::string& symbol, const PriceDataPoint& data);
    void update_price_data_batch(const std::string& symbol, const std::vector<PriceDataPoint>& data);
    std::vector<std::string> get_tracked_symbols() const;
    
    // Real-time monitoring
    using CorrelationChangeCallback = std::function<void(const std::string&, const std::string&, 
                                                        double old_corr, double new_corr)>;
    using ClusterChangeCallback = std::function<void(const CorrelationCluster&)>;
    void register_correlation_change_callback(CorrelationChangeCallback callback);
    void register_cluster_change_callback(ClusterChangeCallback callback);
    void start_real_time_monitoring();
    void stop_real_time_monitoring();
    bool is_monitoring() const;
    
    // Statistical testing
    bool test_correlation_significance(const CorrelationEntry& entry, double alpha = 0.05);
    double calculate_correlation_confidence_interval(const CorrelationEntry& entry, double confidence_level = 0.95);
    bool test_correlation_stability(const std::string& symbol1, const std::string& symbol2);
    std::vector<std::string> detect_correlation_breakpoints(const std::string& symbol1, const std::string& symbol2);
    
    // Advanced correlation measures
    double calculate_mutual_information(const std::string& symbol1, const std::string& symbol2);
    double calculate_distance_correlation(const std::string& symbol1, const std::string& symbol2);
    std::pair<double, double> calculate_tail_dependence(const std::string& symbol1, const std::string& symbol2);
    double calculate_copula_correlation(const std::string& symbol1, const std::string& symbol2);
    
    // Configuration management
    void update_config(const CorrelationConfig& config);
    CorrelationConfig get_config() const;
    void enable_correlation_type(CorrelationType type);
    void disable_correlation_type(CorrelationType type);
    void add_market_condition(MarketCondition condition);
    
    // Statistics and diagnostics
    CorrelationTrackerStats get_statistics() const;
    void reset_statistics();
    std::unordered_map<std::string, std::unordered_map<std::string, double>> get_current_correlations();
    std::vector<std::pair<std::string, std::string>> get_highest_correlations(uint32_t top_n = 10);
    std::vector<std::pair<std::string, std::string>> get_lowest_correlations(uint32_t bottom_n = 10);
    
    // Visualization and reporting
    std::string export_correlation_matrix_csv(const CorrelationMatrix& matrix);
    std::string generate_correlation_report(const std::vector<std::string>& symbols);
    std::vector<std::pair<std::string, std::string>> identify_correlation_anomalies();

private:
    CorrelationConfig config_;
    std::atomic<bool> monitoring_;
    mutable std::mutex config_mutex_;
    
    // Price data storage
    std::unordered_map<std::string, std::deque<PriceDataPoint>> price_data_;
    mutable std::mutex price_data_mutex_;
    
    // Correlation storage
    std::unordered_map<std::string, std::unordered_map<std::string, CorrelationEntry>> correlations_;
    std::unordered_map<CorrelationWindow, CorrelationMatrix> correlation_matrices_;
    mutable std::mutex correlations_mutex_;
    
    // Clustering results
    std::vector<CorrelationCluster> current_clusters_;
    std::unordered_map<std::string, uint32_t> symbol_to_cluster_;
    mutable std::mutex clusters_mutex_;
    
    // Market condition detector
    std::unique_ptr<class MarketConditionDetector> condition_detector_;
    
    // Statistical calculators
    std::unique_ptr<class PearsonCalculator> pearson_calculator_;
    std::unique_ptr<class SpearmanCalculator> spearman_calculator_;
    std::unique_ptr<class KendallCalculator> kendall_calculator_;
    std::unique_ptr<class MutualInformationCalculator> mi_calculator_;
    std::unique_ptr<class CopulaCalculator> copula_calculator_;
    
    // Clustering algorithm
    std::unique_ptr<class CorrelationClusterer> clusterer_;
    
    // Caching
    std::unordered_map<std::string, CorrelationEntry> correlation_cache_;
    mutable std::mutex cache_mutex_;
    
    // Real-time monitoring
    std::thread monitoring_thread_;
    std::vector<CorrelationChangeCallback> correlation_callbacks_;
    std::vector<ClusterChangeCallback> cluster_callbacks_;
    mutable std::mutex callbacks_mutex_;
    
    // Statistics
    mutable CorrelationTrackerStats stats_;
    
    // Internal calculation methods
    double calculate_pearson_correlation(const std::vector<double>& x, const std::vector<double>& y);
    double calculate_spearman_correlation(const std::vector<double>& x, const std::vector<double>& y);
    double calculate_kendall_correlation(const std::vector<double>& x, const std::vector<double>& y);
    
    // Dynamic correlation methods
    std::vector<double> calculate_rolling_correlation_internal(const std::vector<double>& x,
                                                             const std::vector<double>& y,
                                                             uint32_t window_size);
    double calculate_ewma_correlation(const std::vector<double>& x, const std::vector<double>& y, double decay);
    
    // Matrix operations
    CorrelationMatrix build_correlation_matrix(const std::vector<std::string>& symbols, CorrelationType type);
    void calculate_matrix_properties(CorrelationMatrix& matrix);
    std::vector<double> compute_eigenvalues(const std::vector<std::vector<double>>& matrix);
    
    // Clustering helpers
    void update_correlation_clusters();
    double calculate_cluster_quality(const CorrelationCluster& cluster);
    void assign_symbols_to_clusters();
    
    // Conditional correlation helpers
    std::vector<std::pair<double, double>> filter_data_by_condition(const std::vector<PriceDataPoint>& data1,
                                                                   const std::vector<PriceDataPoint>& data2,
                                                                   MarketCondition condition);
    MarketCondition detect_current_market_condition();
    
    // Statistical testing helpers
    double calculate_correlation_t_statistic(double correlation, uint32_t sample_size);
    double calculate_correlation_p_value(double t_statistic, uint32_t degrees_of_freedom);
    std::pair<double, double> calculate_fisher_z_confidence_interval(double correlation, uint32_t sample_size, double alpha);
    
    // Data preprocessing
    std::vector<double> extract_returns(const std::deque<PriceDataPoint>& data);
    void synchronize_data_series(std::vector<double>& x, std::vector<double>& y);
    void handle_missing_data(std::vector<double>& data);
    
    // Utility methods
    std::string generate_cache_key(const std::string& symbol1, const std::string& symbol2, 
                                  CorrelationType type, CorrelationWindow window);
    void update_cache(const std::string& key, const CorrelationEntry& entry);
    std::optional<CorrelationEntry> get_from_cache(const std::string& key);
    void cleanup_expired_cache();
    
    // Real-time monitoring worker
    void monitoring_worker();
    void process_correlation_updates();
    void detect_correlation_changes();
    void notify_correlation_callbacks(const std::string& symbol1, const std::string& symbol2,
                                    double old_correlation, double new_correlation);
    void notify_cluster_callbacks(const CorrelationCluster& cluster);
    
    // Statistics update
    void update_statistics(bool success, double calculation_time_ms);
};

// Utility functions
std::string correlation_type_to_string(CorrelationType type);
CorrelationType string_to_correlation_type(const std::string& str);
std::string correlation_window_to_string(CorrelationWindow window);
std::string market_condition_to_string(MarketCondition condition);
double fisher_z_transform(double correlation);
double inverse_fisher_z_transform(double z);
bool is_strong_correlation(double correlation, double threshold = 0.7);
bool is_weak_correlation(double correlation, double threshold = 0.3);

} // namespace mm
} // namespace hfx
