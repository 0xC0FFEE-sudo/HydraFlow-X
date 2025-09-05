#pragma once

#include <string>
#include <unordered_map>
#include <atomic>
#include <mutex>
#include <memory>
#include <chrono>
#include <vector>
#include <functional>

namespace hfx {
namespace monitor {

// Metric types
enum class MetricType {
    COUNTER = 0,    // Monotonically increasing value
    GAUGE,          // Value that can go up and down
    HISTOGRAM,      // Distribution of values
    SUMMARY        // Quantiles over sliding time window
};

// Metric value union
struct MetricValue {
    double gauge_value;
    uint64_t counter_value;
    std::vector<double> histogram_values;
    std::unordered_map<std::string, double> summary_quantiles;

    MetricValue() : gauge_value(0.0), counter_value(0) {}
};

// Metric definition
struct Metric {
    std::string name;
    std::string description;
    std::vector<std::string> labels;
    MetricType type;
    std::chrono::system_clock::time_point created_at;
    std::chrono::system_clock::time_point last_updated;

    Metric() : type(MetricType::GAUGE) {}
};

// Metrics collector configuration
struct MetricsConfig {
    std::string namespace_name = "hydraflow";
    std::string subsystem_name = "trading";
    std::chrono::seconds collection_interval = std::chrono::seconds(10);
    size_t max_metrics_history = 1000;
    bool enable_prometheus_export = true;
    uint16_t prometheus_port = 9090;
};

// Metrics collector class
class MetricsCollector {
public:
    explicit MetricsCollector(const MetricsConfig& config);
    ~MetricsCollector();

    // Metric registration
    void register_metric(const std::string& name, const std::string& description,
                        MetricType type, const std::vector<std::string>& labels = {});
    void unregister_metric(const std::string& name);

    // Metric updates
    void increment_counter(const std::string& name, uint64_t value = 1,
                          const std::unordered_map<std::string, std::string>& labels = {});
    void set_gauge(const std::string& name, double value,
                   const std::unordered_map<std::string, std::string>& labels = {});
    void observe_histogram(const std::string& name, double value,
                          const std::unordered_map<std::string, std::string>& labels = {});
    void observe_summary(const std::string& name, double value,
                        const std::unordered_map<std::string, std::string>& labels = {});

    // Metric queries
    std::optional<MetricValue> get_metric_value(const std::string& name,
                                               const std::unordered_map<std::string, std::string>& labels = {});
    std::vector<std::string> get_metric_names() const;
    std::optional<Metric> get_metric_info(const std::string& name) const;

    // Bulk operations
    void update_metrics_batch(const std::unordered_map<std::string, MetricValue>& metrics);
    std::unordered_map<std::string, MetricValue> get_all_metrics() const;

    // Export formats
    std::string export_prometheus_format() const;
    std::string export_json_format() const;

    // Statistics
    struct CollectorStats {
        std::atomic<uint64_t> total_metrics_registered{0};
        std::atomic<uint64_t> total_updates{0};
        std::atomic<uint64_t> total_queries{0};
        std::atomic<size_t> current_metric_count{0};
        std::chrono::system_clock::time_point last_collection;
    };

    const CollectorStats& get_collector_stats() const;
    void reset_collector_stats();

    // Configuration
    void update_config(const MetricsConfig& config);
    MetricsConfig get_config() const;

    // Cleanup and maintenance
    void cleanup_expired_metrics();
    void reset_all_metrics();

private:
    MetricsConfig config_;
    mutable std::mutex metrics_mutex_;

    // Metric storage
    std::unordered_map<std::string, Metric> metric_definitions_;
    std::unordered_map<std::string, MetricValue> metric_values_;
    std::unordered_map<std::string, std::unordered_map<std::string, MetricValue>> labeled_metric_values_;

    // Statistics
    mutable CollectorStats stats_;

    // Internal methods
    std::string generate_metric_key(const std::string& name,
                                   const std::unordered_map<std::string, std::string>& labels = {}) const;
    void update_metric_timestamp(const std::string& name);
    bool validate_metric_name(const std::string& name) const;
    void ensure_metric_exists(const std::string& name, MetricType type,
                             const std::vector<std::string>& labels = {});
};

// Utility functions
std::string metric_type_to_string(MetricType type);
MetricType string_to_metric_type(const std::string& type_str);
std::string sanitize_metric_name(const std::string& name);

// Common metric names
namespace metrics {
    // Trading metrics
    const std::string TRADES_EXECUTED = "trades_executed_total";
    const std::string TRADE_VOLUME = "trade_volume_total";
    const std::string TRADE_LATENCY = "trade_latency_seconds";
    const std::string ACTIVE_ORDERS = "active_orders";
    const std::string PENDING_ORDERS = "pending_orders";

    // Risk metrics
    const std::string RISK_EXPOSURE = "risk_exposure_amount";
    const std::string CIRCUIT_BREAKERS_TRIGGERED = "circuit_breakers_triggered_total";
    const std::string POSITION_VALUE = "position_value_amount";
    const std::string PORTFOLIO_VAR = "portfolio_var_amount";

    // MEV metrics
    const std::string MEV_OPPORTUNITIES_DETECTED = "mev_opportunities_detected_total";
    const std::string MEV_ATTACKS_PREVENTED = "mev_attacks_prevented_total";
    const std::string PRIVATE_TRANSACTIONS = "private_transactions_total";

    // System metrics
    const std::string CPU_USAGE = "cpu_usage_percent";
    const std::string MEMORY_USAGE = "memory_usage_bytes";
    const std::string NETWORK_LATENCY = "network_latency_seconds";
    const std::string ERROR_RATE = "error_rate_ratio";

    // Authentication metrics
    const std::string AUTH_SUCCESS = "auth_success_total";
    const std::string AUTH_FAILURE = "auth_failure_total";
    const std::string ACTIVE_SESSIONS = "active_sessions";

    // Performance metrics
    const std::string REQUEST_LATENCY = "request_latency_seconds";
    const std::string REQUEST_RATE = "request_rate_per_second";
    const std::string THROUGHPUT = "throughput_requests_per_second";
}

} // namespace monitor
} // namespace hfx
