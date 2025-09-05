#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <atomic>
#include <mutex>
#include <memory>
#include <chrono>
#include <deque>
#include <functional>
#include <thread>

namespace hfx {
namespace monitor {

// Performance metric types
enum class PerformanceMetric {
    CPU_USAGE = 0,
    MEMORY_USAGE,
    DISK_IO,
    NETWORK_IO,
    THREAD_COUNT,
    OPEN_FILES,
    CONTEXT_SWITCHES,
    PAGE_FAULTS,
    REQUEST_LATENCY,
    RESPONSE_TIME,
    THROUGHPUT,
    ERROR_RATE,
    QUEUE_DEPTH,
    CONNECTION_COUNT
};

// Performance sample
struct PerformanceSample {
    std::string metric_name;
    double value;
    std::chrono::system_clock::time_point timestamp;
    std::unordered_map<std::string, std::string> tags;

    PerformanceSample() : value(0.0) {}
};

// Performance threshold
struct PerformanceThreshold {
    std::string metric_name;
    double warning_threshold;
    double critical_threshold;
    std::string condition; // "above", "below", "equal"
    bool enabled;

    PerformanceThreshold() : warning_threshold(0.0), critical_threshold(0.0), enabled(true) {}
};

// Performance alert
struct PerformanceAlert {
    std::string alert_id;
    std::string metric_name;
    std::string severity; // "warning", "critical"
    double current_value;
    double threshold_value;
    std::chrono::system_clock::time_point timestamp;
    std::string message;
    std::unordered_map<std::string, std::string> tags;

    PerformanceAlert() : current_value(0.0), threshold_value(0.0) {}
};

// Performance monitor configuration
struct PerformanceMonitorConfig {
    std::chrono::seconds collection_interval = std::chrono::seconds(5);
    std::chrono::minutes retention_period = std::chrono::minutes(60);
    size_t max_samples_per_metric = 1000;
    bool enable_system_metrics = true;
    bool enable_application_metrics = true;
    bool enable_threshold_alerts = true;
    std::chrono::seconds alert_cooldown = std::chrono::seconds(300);
};

// Performance statistics
struct PerformanceStats {
    std::string metric_name;
    double current_value;
    double average_value;
    double min_value;
    double max_value;
    double p95_value;
    double p99_value;
    std::chrono::system_clock::time_point last_updated;
    size_t sample_count;

    PerformanceStats() : current_value(0.0), average_value(0.0),
                        min_value(0.0), max_value(0.0),
                        p95_value(0.0), p99_value(0.0), sample_count(0) {}
};

// Performance monitor class
class PerformanceMonitor {
public:
    explicit PerformanceMonitor(const PerformanceMonitorConfig& config);
    ~PerformanceMonitor();

    // Metric collection
    void start_collection();
    void stop_collection();
    bool is_collecting() const;

    // Manual sampling
    void record_sample(const std::string& metric_name, double value,
                      const std::unordered_map<std::string, std::string>& tags = {});
    void record_timer(const std::string& operation_name, std::chrono::microseconds duration,
                     const std::unordered_map<std::string, std::string>& tags = {});

    // Performance queries
    std::optional<PerformanceStats> get_performance_stats(const std::string& metric_name) const;
    std::vector<PerformanceSample> get_recent_samples(const std::string& metric_name,
                                                     std::chrono::minutes window = std::chrono::minutes(5)) const;
    std::vector<std::string> get_available_metrics() const;

    // Threshold management
    void set_threshold(const std::string& metric_name, double warning_threshold,
                      double critical_threshold, const std::string& condition = "above");
    void remove_threshold(const std::string& metric_name);
    std::vector<PerformanceThreshold> get_thresholds() const;

    // Alert management
    std::vector<PerformanceAlert> get_recent_alerts(std::chrono::minutes window = std::chrono::minutes(10)) const;
    void acknowledge_alert(const std::string& alert_id);

    // Built-in system metrics
    void enable_system_cpu_monitoring();
    void enable_system_memory_monitoring();
    void enable_system_disk_monitoring();
    void enable_system_network_monitoring();

    // Application metrics
    void enable_request_latency_monitoring();
    void enable_error_rate_monitoring();
    void enable_throughput_monitoring();
    void enable_queue_depth_monitoring();

    // Performance profiling
    using TimerHandle = std::unique_ptr<void, std::function<void(void*)>>;
    TimerHandle start_timer(const std::string& operation_name,
                           const std::unordered_map<std::string, std::string>& tags = {});

    // Bulk operations
    void record_batch_samples(const std::vector<PerformanceSample>& samples);
    std::unordered_map<std::string, PerformanceStats> get_all_performance_stats() const;

    // Configuration
    void update_config(const PerformanceMonitorConfig& config);
    PerformanceMonitorConfig get_config() const;

    // Statistics and monitoring
    struct MonitorStats {
        std::atomic<uint64_t> total_samples_collected{0};
        std::atomic<uint64_t> total_alerts_generated{0};
        std::atomic<uint64_t> active_thresholds{0};
        std::atomic<size_t> current_metrics_count{0};
        std::chrono::system_clock::time_point last_collection;
        std::chrono::system_clock::time_point last_alert;
    };

    MonitorStats get_monitor_stats() const;
    void reset_monitor_stats();

    // Alert callbacks
    using PerformanceAlertCallback = std::function<void(const PerformanceAlert&)>;
    void register_alert_callback(PerformanceAlertCallback callback);

    // Cleanup and maintenance
    void cleanup_old_samples();
    void reset_all_metrics();

private:
    PerformanceMonitorConfig config_;
    mutable std::mutex monitor_mutex_;

    // Sample storage
    std::unordered_map<std::string, std::deque<PerformanceSample>> metric_samples_;
    std::unordered_map<std::string, PerformanceThreshold> thresholds_;
    std::unordered_map<std::string, PerformanceAlert> recent_alerts_;

    // Monitoring state
    std::atomic<bool> collecting_active_;
    std::vector<std::thread> collection_threads_;
    std::unordered_map<std::string, std::chrono::system_clock::time_point> last_alert_times_;

    // Statistics
    mutable MonitorStats stats_;

    // Callbacks
    std::vector<PerformanceAlertCallback> alert_callbacks_;
    mutable std::mutex callbacks_mutex_;

    // Internal methods
    void collection_worker();
    void collect_system_metrics();
    void check_thresholds();
    void generate_alert(const std::string& metric_name, double current_value,
                       const PerformanceThreshold& threshold);
    void cleanup_expired_alerts();
    PerformanceStats calculate_stats(const std::deque<PerformanceSample>& samples) const;
    bool should_generate_alert(const std::string& metric_name) const;

    // System metric collectors
    double collect_cpu_usage();
    double collect_memory_usage();
    double collect_disk_io();
    double collect_network_io();
    size_t collect_thread_count();
    size_t collect_open_files_count();

    // Utility methods
    std::string generate_alert_id();
    bool validate_threshold_condition(double value, double threshold, const std::string& condition);
    void notify_alert_callbacks(const PerformanceAlert& alert);
    void update_monitor_stats(const PerformanceSample& sample);
};

// Utility functions
std::string performance_metric_to_string(PerformanceMetric metric);
std::string format_performance_sample(const PerformanceSample& sample);
std::string format_performance_alert(const PerformanceAlert& alert);

// Performance profiling helpers
class ScopedTimer {
public:
    ScopedTimer(PerformanceMonitor& monitor, const std::string& operation_name,
               const std::unordered_map<std::string, std::string>& tags = {});
    ~ScopedTimer();

private:
    PerformanceMonitor& monitor_;
    std::string operation_name_;
    std::unordered_map<std::string, std::string> tags_;
    std::chrono::high_resolution_clock::time_point start_time_;
};

// Common metric names
namespace perf_metrics {
    // System metrics
    const std::string CPU_USAGE_PERCENT = "cpu_usage_percent";
    const std::string MEMORY_USAGE_MB = "memory_usage_mb";
    const std::string DISK_READ_MBPS = "disk_read_mbps";
    const std::string DISK_WRITE_MBPS = "disk_write_mbps";
    const std::string NETWORK_RX_MBPS = "network_rx_mbps";
    const std::string NETWORK_TX_MBPS = "network_tx_mbps";
    const std::string THREAD_COUNT = "thread_count";
    const std::string OPEN_FILES_COUNT = "open_files_count";

    // Application metrics
    const std::string REQUEST_LATENCY_MS = "request_latency_ms";
    const std::string RESPONSE_TIME_MS = "response_time_ms";
    const std::string REQUESTS_PER_SECOND = "requests_per_second";
    const std::string ERROR_RATE_PERCENT = "error_rate_percent";
    const std::string QUEUE_DEPTH = "queue_depth";
    const std::string ACTIVE_CONNECTIONS = "active_connections";

    // Trading metrics
    const std::string ORDER_PROCESSING_TIME_MS = "order_processing_time_ms";
    const std::string TRADE_EXECUTION_TIME_MS = "trade_execution_time_ms";
    const std::string RISK_CALCULATION_TIME_MS = "risk_calculation_time_ms";
    const std::string MEV_DETECTION_TIME_MS = "mev_detection_time_ms";
}

} // namespace monitor
} // namespace hfx
