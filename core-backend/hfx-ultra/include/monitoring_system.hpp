#pragma once

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <atomic>
#include <mutex>
#include <thread>
#include <chrono>
#include <functional>
#include <queue>
#include <fstream>

namespace hfx::ultra {

// Alert severity levels
enum class AlertSeverity {
    INFO = 0,
    WARNING = 1,
    ERROR = 2,
    CRITICAL = 3,
    EMERGENCY = 4
};

// Metric types for different monitoring scenarios
enum class MetricType {
    COUNTER,        // Ever-increasing value (total requests, errors)
    GAUGE,          // Point-in-time value (CPU usage, memory)
    HISTOGRAM,      // Distribution of values (latency, response times)
    TIMER,          // Duration measurements
    RATE            // Rate of change over time
};

// Alert rules and conditions
enum class AlertCondition {
    GREATER_THAN,
    LESS_THAN,
    EQUALS,
    NOT_EQUALS,
    RATE_INCREASE,
    RATE_DECREASE,
    THRESHOLD_BREACH,
    PATTERN_MATCH
};

// Alert delivery channels
enum class AlertChannel {
    CONSOLE,        // Log to console
    EMAIL,          // Send email notification
    SLACK,          // Send to Slack channel
    WEBHOOK,        // HTTP webhook
    SMS,            // SMS notification
    PAGERDUTY,      // PagerDuty integration
    DATADOG,        // Datadog events
    PROMETHEUS      // Prometheus alerts
};

// Metric data point
struct MetricPoint {
    std::string name;
    double value;
    std::chrono::system_clock::time_point timestamp;
    std::unordered_map<std::string, std::string> labels;
    MetricType type;
    
    MetricPoint() = default;
    MetricPoint(const std::string& metric_name, double val, MetricType t = MetricType::GAUGE)
        : name(metric_name), value(val), timestamp(std::chrono::system_clock::now()), type(t) {}
};

// Alert definition
struct AlertRule {
    std::string name;
    std::string description;
    std::string metric_name;
    AlertCondition condition;
    double threshold;
    std::chrono::seconds evaluation_interval{60};
    std::chrono::seconds for_duration{300}; // Alert must be active for this long
    AlertSeverity severity;
    std::vector<AlertChannel> channels;
    std::unordered_map<std::string, std::string> labels;
    bool enabled = true;
    
    // Rate-based alert parameters
    std::chrono::seconds rate_window{300}; // 5 minutes
    double rate_threshold = 0.0;
};

// Active alert instance
struct Alert {
    std::string rule_name;
    std::string message;
    AlertSeverity severity;
    std::chrono::system_clock::time_point triggered_at;
    std::chrono::system_clock::time_point resolved_at;
    bool resolved = false;
    std::unordered_map<std::string, std::string> labels;
    std::vector<std::string> actions_taken;
};

// System health status
struct SystemHealth {
    struct ComponentHealth {
        std::string name;
        bool healthy;
        std::string status_message;
        std::chrono::system_clock::time_point last_check;
        double health_score; // 0.0 - 1.0
        std::unordered_map<std::string, double> metrics;
    };
    
    bool overall_healthy;
    double overall_score;
    std::vector<ComponentHealth> components;
    std::chrono::system_clock::time_point last_update;
    uint32_t active_alerts;
    uint32_t critical_alerts;
};

// Performance monitoring configuration
struct MonitoringConfig {
    // Collection settings
    std::chrono::seconds metric_collection_interval{10};
    std::chrono::seconds health_check_interval{30};
    std::chrono::seconds alert_evaluation_interval{60};
    
    // Retention settings
    std::chrono::hours metric_retention{24};
    uint32_t max_metrics_in_memory = 100000;
    
    // Export settings
    bool enable_prometheus_export = true;
    uint16_t prometheus_port = 9090;
    bool enable_file_export = true;
    std::string metrics_file_path = "/tmp/hydraflow_metrics.json";
    
    // Alert settings
    bool enable_alerting = true;
    std::string alert_log_path = "/tmp/hydraflow_alerts.log";
    
    // External integrations
    std::string slack_webhook_url;
    std::string email_smtp_server;
    std::string pagerduty_api_key;
    std::string datadog_api_key;
    
    // Performance settings
    uint32_t metric_worker_threads = 2;
    uint32_t alert_worker_threads = 1;
    bool enable_real_time_processing = true;
};

// Callback types for different monitoring events
using MetricCollector = std::function<std::vector<MetricPoint>()>;
using HealthChecker = std::function<SystemHealth::ComponentHealth()>;
using AlertHandler = std::function<void(const Alert&)>;

// Main monitoring and alerting system
class MonitoringSystem {
public:
    explicit MonitoringSystem(const MonitoringConfig& config = MonitoringConfig{});
    ~MonitoringSystem();
    
    // Lifecycle management
    bool initialize();
    bool start();
    bool stop();
    bool is_running() const { return running_.load(); }
    
    // Metric collection and recording
    void record_metric(const MetricPoint& metric);
    void record_metric(const std::string& name, double value, MetricType type = MetricType::GAUGE);
    void record_counter(const std::string& name, double increment = 1.0);
    void record_gauge(const std::string& name, double value);
    void record_timer(const std::string& name, std::chrono::nanoseconds duration);
    void record_histogram(const std::string& name, double value);
    
    // Metric retrieval and querying
    std::vector<MetricPoint> get_metrics(const std::string& name, 
                                        std::chrono::system_clock::time_point start,
                                        std::chrono::system_clock::time_point end) const;
    std::vector<MetricPoint> get_recent_metrics(const std::string& name, 
                                               std::chrono::seconds duration) const;
    double get_latest_metric_value(const std::string& name) const;
    
    // Alert management
    void add_alert_rule(const AlertRule& rule);
    void remove_alert_rule(const std::string& rule_name);
    void enable_alert_rule(const std::string& rule_name, bool enabled);
    std::vector<AlertRule> get_alert_rules() const;
    
    // Active alerts
    std::vector<Alert> get_active_alerts() const;
    std::vector<Alert> get_alerts_by_severity(AlertSeverity severity) const;
    void acknowledge_alert(const std::string& rule_name);
    void resolve_alert(const std::string& rule_name);
    
    // Health monitoring
    void register_health_checker(const std::string& component_name, HealthChecker checker);
    void unregister_health_checker(const std::string& component_name);
    SystemHealth get_system_health() const;
    bool is_system_healthy() const;
    
    // Custom metric collectors
    void register_metric_collector(const std::string& name, MetricCollector collector);
    void unregister_metric_collector(const std::string& name);
    
    // Alert handlers
    void register_alert_handler(AlertChannel channel, AlertHandler handler);
    void unregister_alert_handler(AlertChannel channel);
    
    // Manual alerts
    void trigger_alert(const std::string& message, AlertSeverity severity,
                      const std::unordered_map<std::string, std::string>& labels = {});
    
    // Statistics and monitoring of the monitoring system itself
    struct MonitoringStats {
        std::atomic<uint64_t> metrics_collected{0};
        std::atomic<uint64_t> alerts_triggered{0};
        std::atomic<uint64_t> alerts_resolved{0};
        std::atomic<uint64_t> health_checks_performed{0};
        std::atomic<double> avg_metric_processing_time_us{0.0};
        std::atomic<double> avg_alert_evaluation_time_us{0.0};
        std::atomic<uint32_t> active_metric_collectors{0};
        std::atomic<uint32_t> active_health_checkers{0};
        std::atomic<uint64_t> total_memory_usage_bytes{0};
    };
    
    const MonitoringStats& get_stats() const { return stats_; }
    void reset_stats();
    
    // Export and integration
    std::string export_metrics_json() const;
    std::string export_metrics_prometheus() const;
    bool export_metrics_to_file(const std::string& filepath) const;
    
    // Configuration management
    void update_config(const MonitoringConfig& config);
    const MonitoringConfig& get_config() const { return config_; }
    
    // Trading-specific monitoring helpers
    void record_trade_latency(const std::string& symbol, std::chrono::nanoseconds latency);
    void record_order_execution(const std::string& symbol, bool success, double size_usd);
    void record_mev_opportunity(const std::string& type, double profit_potential);
    void record_risk_metric(const std::string& metric_name, double value, AlertSeverity severity = AlertSeverity::INFO);
    void record_system_performance(const std::string& component, double cpu_usage, double memory_usage);
    
    // Pre-configured alert rules for trading systems
    void setup_trading_alerts();
    void setup_system_alerts();
    void setup_performance_alerts();
    
private:
    MonitoringConfig config_;
    std::atomic<bool> running_{false};
    std::atomic<bool> shutdown_requested_{false};
    mutable MonitoringStats stats_;
    
    // Core storage
    mutable std::mutex metrics_mutex_;
    std::unordered_map<std::string, std::vector<MetricPoint>> metrics_store_;
    
    mutable std::mutex alerts_mutex_;
    std::vector<AlertRule> alert_rules_;
    std::vector<Alert> active_alerts_;
    
    mutable std::mutex health_mutex_;
    std::unordered_map<std::string, HealthChecker> health_checkers_;
    SystemHealth current_health_;
    
    // Custom collectors and handlers
    mutable std::mutex collectors_mutex_;
    std::unordered_map<std::string, MetricCollector> metric_collectors_;
    std::unordered_map<AlertChannel, AlertHandler> alert_handlers_;
    
    // Worker threads
    std::vector<std::thread> metric_workers_;
    std::thread alert_evaluator_;
    std::thread health_monitor_;
    std::thread cleanup_worker_;
    
    // Work queues
    std::queue<MetricPoint> metric_queue_;
    std::mutex metric_queue_mutex_;
    std::condition_variable metric_queue_cv_;
    
    std::queue<Alert> alert_queue_;
    std::mutex alert_queue_mutex_;
    std::condition_variable alert_queue_cv_;
    
    // Internal methods
    void metric_worker();
    void alert_evaluator_worker();
    void health_monitor_worker();
    void cleanup_worker();
    
    void process_metric(const MetricPoint& metric);
    void evaluate_alert_rules();
    void evaluate_single_rule(const AlertRule& rule);
    void trigger_alert_internal(const Alert& alert);
    void send_alert_to_channels(const Alert& alert, const std::vector<AlertChannel>& channels);
    
    void cleanup_old_metrics();
    void cleanup_resolved_alerts();
    
    bool should_trigger_alert(const AlertRule& rule, const std::vector<MetricPoint>& recent_metrics) const;
    double calculate_metric_rate(const std::vector<MetricPoint>& metrics, std::chrono::seconds window) const;
    
    void update_system_health();
    SystemHealth::ComponentHealth create_monitoring_health() const;
    
    // Export helpers
    std::string metric_point_to_json(const MetricPoint& metric) const;
    std::string alert_to_json(const Alert& alert) const;
    std::string metric_point_to_prometheus(const MetricPoint& metric) const;
    
    // Alert channel implementations
    void send_console_alert(const Alert& alert);
    void send_email_alert(const Alert& alert);
    void send_slack_alert(const Alert& alert);
    void send_webhook_alert(const Alert& alert);
    void send_sms_alert(const Alert& alert);
    void send_pagerduty_alert(const Alert& alert);
    void send_datadog_alert(const Alert& alert);
    void send_prometheus_alert(const Alert& alert);
};

// Factory for creating monitoring systems with different configurations
class MonitoringFactory {
public:
    // Pre-configured monitoring systems
    static std::unique_ptr<MonitoringSystem> create_trading_monitor();
    static std::unique_ptr<MonitoringSystem> create_development_monitor();
    static std::unique_ptr<MonitoringSystem> create_production_monitor();
    static std::unique_ptr<MonitoringSystem> create_high_frequency_monitor();
    
    // Custom configurations
    static std::unique_ptr<MonitoringSystem> create_with_config(const MonitoringConfig& config);
};

// Utility functions for common monitoring patterns
namespace monitoring_utils {
    std::string severity_to_string(AlertSeverity severity);
    AlertSeverity string_to_severity(const std::string& severity_str);
    
    std::string metric_type_to_string(MetricType type);
    MetricType string_to_metric_type(const std::string& type_str);
    
    std::string condition_to_string(AlertCondition condition);
    AlertCondition string_to_condition(const std::string& condition_str);
    
    std::string channel_to_string(AlertChannel channel);
    AlertChannel string_to_channel(const std::string& channel_str);
    
    // Time and formatting utilities
    std::string format_timestamp(std::chrono::system_clock::time_point tp);
    std::string format_duration(std::chrono::nanoseconds duration);
    std::string format_bytes(uint64_t bytes);
    std::string format_rate(double rate_per_second, const std::string& unit = "ops");
    
    // Metric calculation helpers
    double calculate_percentile(const std::vector<double>& values, double percentile);
    double calculate_moving_average(const std::vector<double>& values, size_t window_size);
    double calculate_standard_deviation(const std::vector<double>& values);
    
    // Health scoring functions
    double calculate_latency_health_score(std::chrono::nanoseconds avg_latency, 
                                         std::chrono::nanoseconds max_acceptable);
    double calculate_error_rate_health_score(double error_rate, double max_acceptable_rate);
    double calculate_resource_health_score(double usage_percentage, double warning_threshold);
}

} // namespace hfx::ultra
