/**
 * @file monitoring_system.cpp
 * @brief Comprehensive monitoring and alerting system for HydraFlow-X
 */

#include "monitoring_system.hpp"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <iomanip>
#include <random>
#include <curl/curl.h>

namespace hfx::ultra {

// Utility function implementations
namespace monitoring_utils {
    std::string severity_to_string(AlertSeverity severity) {
        switch (severity) {
            case AlertSeverity::INFO: return "INFO";
            case AlertSeverity::WARNING: return "WARNING";
            case AlertSeverity::ERROR: return "ERROR";
            case AlertSeverity::CRITICAL: return "CRITICAL";
            case AlertSeverity::EMERGENCY: return "EMERGENCY";
            default: return "UNKNOWN";
        }
    }

    AlertSeverity string_to_severity(const std::string& severity_str) {
        if (severity_str == "INFO") return AlertSeverity::INFO;
        if (severity_str == "WARNING") return AlertSeverity::WARNING;
        if (severity_str == "ERROR") return AlertSeverity::ERROR;
        if (severity_str == "CRITICAL") return AlertSeverity::CRITICAL;
        if (severity_str == "EMERGENCY") return AlertSeverity::EMERGENCY;
        return AlertSeverity::INFO;
    }

    std::string metric_type_to_string(MetricType type) {
        switch (type) {
            case MetricType::COUNTER: return "counter";
            case MetricType::GAUGE: return "gauge";
            case MetricType::HISTOGRAM: return "histogram";
            case MetricType::TIMER: return "timer";
            case MetricType::RATE: return "rate";
            default: return "unknown";
        }
    }

    std::string format_timestamp(std::chrono::system_clock::time_point tp) {
        auto time_t = std::chrono::system_clock::to_time_t(tp);
        std::stringstream ss;
        ss << std::put_time(std::gmtime(&time_t), "%Y-%m-%d %H:%M:%S UTC");
        return ss.str();
    }

    std::string format_duration(std::chrono::nanoseconds duration) {
        auto ns = duration.count();
        if (ns < 1000) return std::to_string(ns) + "ns";
        if (ns < 1000000) return std::to_string(ns / 1000) + "μs";
        if (ns < 1000000000) return std::to_string(ns / 1000000) + "ms";
        return std::to_string(ns / 1000000000.0) + "s";
    }

    std::string format_bytes(uint64_t bytes) {
        const char* units[] = {"B", "KB", "MB", "GB", "TB"};
        int unit = 0;
        double size = static_cast<double>(bytes);
        
        while (size >= 1024 && unit < 4) {
            size /= 1024;
            unit++;
        }
        
        std::stringstream ss;
        ss << std::fixed << std::setprecision(2) << size << units[unit];
        return ss.str();
    }

    double calculate_percentile(const std::vector<double>& values, double percentile) {
        if (values.empty()) return 0.0;
        
        std::vector<double> sorted_values = values;
        std::sort(sorted_values.begin(), sorted_values.end());
        
        double index = (percentile / 100.0) * (sorted_values.size() - 1);
        size_t lower = static_cast<size_t>(std::floor(index));
        size_t upper = static_cast<size_t>(std::ceil(index));
        
        if (lower == upper) {
            return sorted_values[lower];
        }
        
        double weight = index - lower;
        return sorted_values[lower] * (1 - weight) + sorted_values[upper] * weight;
    }

    double calculate_moving_average(const std::vector<double>& values, size_t window_size) {
        if (values.empty()) return 0.0;
        
        size_t start = values.size() > window_size ? values.size() - window_size : 0;
        double sum = 0.0;
        size_t count = 0;
        
        for (size_t i = start; i < values.size(); ++i) {
            sum += values[i];
            count++;
        }
        
        return count > 0 ? sum / count : 0.0;
    }

    double calculate_latency_health_score(std::chrono::nanoseconds avg_latency, 
                                         std::chrono::nanoseconds max_acceptable) {
        if (max_acceptable.count() == 0) return 1.0;
        
        double ratio = static_cast<double>(avg_latency.count()) / max_acceptable.count();
        if (ratio <= 0.5) return 1.0;  // Excellent
        if (ratio <= 1.0) return 1.0 - (ratio - 0.5) * 2.0; // Linear degradation
        return 0.0; // Unacceptable
    }

    double calculate_error_rate_health_score(double error_rate, double max_acceptable_rate) {
        if (error_rate <= max_acceptable_rate * 0.1) return 1.0; // Excellent
        if (error_rate <= max_acceptable_rate) {
            return 1.0 - (error_rate / max_acceptable_rate) * 0.8;
        }
        return 0.0; // Unacceptable
    }
}

// MonitoringSystem implementation
MonitoringSystem::MonitoringSystem(const MonitoringConfig& config) 
    : config_(config) {
    HFX_LOG_INFO("📊 Initializing Monitoring System...");
    HFX_LOG_INFO("   Metric collection interval: " + std::to_string(config_.metric_collection_interval.count()) + "s");
    HFX_LOG_INFO("   Alert evaluation interval: " + std::to_string(config_.alert_evaluation_interval.count()) + "s");
    HFX_LOG_INFO("   Health check interval: " + std::to_string(config_.health_check_interval.count()) + "s");
}

MonitoringSystem::~MonitoringSystem() {
    stop();
}

bool MonitoringSystem::initialize() {
    HFX_LOG_INFO("🔧 Initializing monitoring system components...");
    
    // Initialize cURL for webhook/HTTP alerts
    curl_global_init(CURL_GLOBAL_DEFAULT);
    
    // Set up default alert handlers
    register_alert_handler(AlertChannel::CONSOLE, [this](const Alert& alert) {
        send_console_alert(alert);
    });
    
    if (!config_.slack_webhook_url.empty()) {
        register_alert_handler(AlertChannel::SLACK, [this](const Alert& alert) {
            send_slack_alert(alert);
        });
    }
    
    // Register self-monitoring health checker
    register_health_checker("monitoring_system", [this]() {
        return create_monitoring_health();
    });
    
    HFX_LOG_INFO("✅ Monitoring system initialized");
    return true;
}

bool MonitoringSystem::start() {
    if (running_.load()) {
        HFX_LOG_INFO("⚠️  Monitoring system already running");
        return true;
    }
    
    HFX_LOG_INFO("🚀 Starting monitoring system...");
    
    running_.store(true);
    shutdown_requested_.store(false);
    
    // Start metric collection workers
    for (uint32_t i = 0; i < config_.metric_worker_threads; ++i) {
        metric_workers_.emplace_back(&MonitoringSystem::metric_worker, this);
    }
    
    // Start alert evaluator
    alert_evaluator_ = std::thread(&MonitoringSystem::alert_evaluator_worker, this);
    
    // Start health monitor
    health_monitor_ = std::thread(&MonitoringSystem::health_monitor_worker, this);
    
    // Start cleanup worker
    cleanup_worker_ = std::thread(&MonitoringSystem::cleanup_worker, this);
    
    HFX_LOG_INFO("✅ Monitoring system started with " + std::to_string(config_.metric_worker_threads) 
              + " metric workers");
    return true;
}

bool MonitoringSystem::stop() {
    if (!running_.load()) {
        return true;
    }
    
    HFX_LOG_INFO("🛑 Stopping monitoring system...");
    
    shutdown_requested_.store(true);
    running_.store(false);
    
    // Notify all workers
    metric_queue_cv_.notify_all();
    alert_queue_cv_.notify_all();
    
    // Join worker threads
    for (auto& worker : metric_workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }
    metric_workers_.clear();
    
    if (alert_evaluator_.joinable()) {
        alert_evaluator_.join();
    }
    
    if (health_monitor_.joinable()) {
        health_monitor_.join();
    }
    
    if (cleanup_worker_.joinable()) {
        cleanup_worker_.join();
    }
    
    // Cleanup cURL
    curl_global_cleanup();
    
    HFX_LOG_INFO("✅ Monitoring system stopped");
    return true;
}

void MonitoringSystem::record_metric(const MetricPoint& metric) {
    if (!running_.load()) return;
    
    {
        std::lock_guard<std::mutex> lock(metric_queue_mutex_);
        metric_queue_.push(metric);
    }
    metric_queue_cv_.notify_one();
    
    stats_.metrics_collected.fetch_add(1);
}

void MonitoringSystem::record_metric(const std::string& name, double value, MetricType type) {
    MetricPoint metric(name, value, type);
    record_metric(metric);
}

void MonitoringSystem::record_counter(const std::string& name, double increment) {
    record_metric(name, increment, MetricType::COUNTER);
}

void MonitoringSystem::record_gauge(const std::string& name, double value) {
    record_metric(name, value, MetricType::GAUGE);
}

void MonitoringSystem::record_timer(const std::string& name, std::chrono::nanoseconds duration) {
    record_metric(name, static_cast<double>(duration.count()), MetricType::TIMER);
}

std::vector<MetricPoint> MonitoringSystem::get_recent_metrics(const std::string& name, 
                                                             std::chrono::seconds duration) const {
    auto end_time = std::chrono::system_clock::now();
    auto start_time = end_time - duration;
    return get_metrics(name, start_time, end_time);
}

std::vector<MetricPoint> MonitoringSystem::get_metrics(const std::string& name, 
                                                      std::chrono::system_clock::time_point start,
                                                      std::chrono::system_clock::time_point end) const {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    
    auto it = metrics_store_.find(name);
    if (it == metrics_store_.end()) {
        return {};
    }
    
    std::vector<MetricPoint> result;
    for (const auto& metric : it->second) {
        if (metric.timestamp >= start && metric.timestamp <= end) {
            result.push_back(metric);
        }
    }
    
    return result;
}

double MonitoringSystem::get_latest_metric_value(const std::string& name) const {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    
    auto it = metrics_store_.find(name);
    if (it == metrics_store_.end() || it->second.empty()) {
        return 0.0;
    }
    
    return it->second.back().value;
}

void MonitoringSystem::add_alert_rule(const AlertRule& rule) {
    std::lock_guard<std::mutex> lock(alerts_mutex_);
    
    // Remove existing rule with same name
    alert_rules_.erase(
        std::remove_if(alert_rules_.begin(), alert_rules_.end(),
                      [&rule](const AlertRule& r) { return r.name == rule.name; }),
        alert_rules_.end());
    
    alert_rules_.push_back(rule);
    
    HFX_LOG_INFO("📋 Added alert rule: " + rule.name + 
              " (" + monitoring_utils::severity_to_string(rule.severity) + ")");
}

void MonitoringSystem::trigger_alert(const std::string& message, AlertSeverity severity,
                                    const std::unordered_map<std::string, std::string>& labels) {
    Alert alert;
    alert.rule_name = "manual_alert";
    alert.message = message;
    alert.severity = severity;
    alert.triggered_at = std::chrono::system_clock::now();
    alert.labels = labels;
    
    trigger_alert_internal(alert);
}

std::vector<Alert> MonitoringSystem::get_active_alerts() const {
    std::lock_guard<std::mutex> lock(alerts_mutex_);
    
    std::vector<Alert> active;
    for (const auto& alert : active_alerts_) {
        if (!alert.resolved) {
            active.push_back(alert);
        }
    }
    
    return active;
}

SystemHealth MonitoringSystem::get_system_health() const {
    std::lock_guard<std::mutex> lock(health_mutex_);
    return current_health_;
}

bool MonitoringSystem::is_system_healthy() const {
    auto health = get_system_health();
    return health.overall_healthy && health.critical_alerts == 0;
}

void MonitoringSystem::register_health_checker(const std::string& component_name, HealthChecker checker) {
    std::lock_guard<std::mutex> lock(health_mutex_);
    health_checkers_[component_name] = checker;
    stats_.active_health_checkers.fetch_add(1);
    
    HFX_LOG_INFO("🏥 Registered health checker for: " + component_name);
}

void MonitoringSystem::register_alert_handler(AlertChannel channel, AlertHandler handler) {
    alert_handlers_[channel] = handler;
    
    HFX_LOG_INFO("📢 Registered alert handler for: " 
              << monitoring_utils::channel_to_string(channel) << std::endl;
}

// Trading-specific monitoring helpers
void MonitoringSystem::record_trade_latency(const std::string& symbol, std::chrono::nanoseconds latency) {
    MetricPoint metric;
    metric.name = "trade_latency_ns";
    metric.value = static_cast<double>(latency.count());
    metric.type = MetricType::HISTOGRAM;
    metric.labels["symbol"] = symbol;
    metric.timestamp = std::chrono::system_clock::now();
    
    record_metric(metric);
}

void MonitoringSystem::record_order_execution(const std::string& symbol, bool success, double size_usd) {
    // Record execution success/failure
    record_counter(std::string("orders_") + (success ? "successful" : "failed"), 1.0);
    
    // Record trade size
    MetricPoint size_metric;
    size_metric.name = "order_size_usd";
    size_metric.value = size_usd;
    size_metric.type = MetricType::HISTOGRAM;
    size_metric.labels["symbol"] = symbol;
    size_metric.labels["status"] = success ? "success" : "failure";
    record_metric(size_metric);
}

void MonitoringSystem::record_mev_opportunity(const std::string& type, double profit_potential) {
    record_counter("mev_opportunities_total", 1.0);
    
    MetricPoint profit_metric;
    profit_metric.name = "mev_profit_potential_usd";
    profit_metric.value = profit_potential;
    profit_metric.type = MetricType::HISTOGRAM;
    profit_metric.labels["type"] = type;
    record_metric(profit_metric);
}

void MonitoringSystem::record_risk_metric(const std::string& metric_name, double value, AlertSeverity severity) {
    MetricPoint metric;
    metric.name = std::string("risk_") + metric_name;
    metric.value = value;
    metric.type = MetricType::GAUGE;
    metric.labels["severity"] = monitoring_utils::severity_to_string(severity);
    record_metric(metric);
}

void MonitoringSystem::record_system_performance(const std::string& component, double cpu_usage, double memory_usage) {
    MetricPoint cpu_metric;
    cpu_metric.name = "cpu_usage_percent";
    cpu_metric.value = cpu_usage;
    cpu_metric.type = MetricType::GAUGE;
    cpu_metric.labels["component"] = component;
    record_metric(cpu_metric);
    
    MetricPoint memory_metric;
    memory_metric.name = "memory_usage_percent";
    memory_metric.value = memory_usage;
    memory_metric.type = MetricType::GAUGE;
    memory_metric.labels["component"] = component;
    record_metric(memory_metric);
}

// Pre-configured alert rules
void MonitoringSystem::setup_trading_alerts() {
    HFX_LOG_INFO("📊 Setting up trading-specific alert rules...");
    
    // High latency alert
    AlertRule latency_rule;
    latency_rule.name = "high_trade_latency";
    latency_rule.description = "Trade execution latency is too high";
    latency_rule.metric_name = "trade_latency_ns";
    latency_rule.condition = AlertCondition::GREATER_THAN;
    latency_rule.threshold = 50000000.0; // 50ms in nanoseconds
    latency_rule.severity = AlertSeverity::WARNING;
    latency_rule.channels = {AlertChannel::CONSOLE, AlertChannel::SLACK};
    latency_rule.evaluation_interval = std::chrono::seconds(30);
    add_alert_rule(latency_rule);
    
    // Order failure rate alert
    AlertRule failure_rule;
    failure_rule.name = "high_order_failure_rate";
    failure_rule.description = "Order failure rate is too high";
    failure_rule.metric_name = "orders_failed";
    failure_rule.condition = AlertCondition::RATE_INCREASE;
    failure_rule.rate_threshold = 10.0; // 10 failures per minute
    failure_rule.severity = AlertSeverity::CRITICAL;
    failure_rule.channels = {AlertChannel::CONSOLE, AlertChannel::SLACK};
    add_alert_rule(failure_rule);
    
    // MEV opportunity alert
    AlertRule mev_rule;
    mev_rule.name = "large_mev_opportunity";
    mev_rule.description = "Large MEV opportunity detected";
    mev_rule.metric_name = "mev_profit_potential_usd";
    mev_rule.condition = AlertCondition::GREATER_THAN;
    mev_rule.threshold = 10000.0; // $10k opportunity
    mev_rule.severity = AlertSeverity::INFO;
    mev_rule.channels = {AlertChannel::CONSOLE};
    add_alert_rule(mev_rule);
    
    HFX_LOG_INFO("✅ Trading alert rules configured");
}

void MonitoringSystem::setup_system_alerts() {
    HFX_LOG_INFO("🖥️  Setting up system-level alert rules...");
    
    // High CPU usage
    AlertRule cpu_rule;
    cpu_rule.name = "high_cpu_usage";
    cpu_rule.description = "CPU usage is critically high";
    cpu_rule.metric_name = "cpu_usage_percent";
    cpu_rule.condition = AlertCondition::GREATER_THAN;
    cpu_rule.threshold = 90.0;
    cpu_rule.severity = AlertSeverity::WARNING;
    cpu_rule.channels = {AlertChannel::CONSOLE};
    add_alert_rule(cpu_rule);
    
    // High memory usage
    AlertRule memory_rule;
    memory_rule.name = "high_memory_usage";
    memory_rule.description = "Memory usage is critically high";
    memory_rule.metric_name = "memory_usage_percent";
    memory_rule.condition = AlertCondition::GREATER_THAN;
    memory_rule.threshold = 85.0;
    memory_rule.severity = AlertSeverity::WARNING;
    memory_rule.channels = {AlertChannel::CONSOLE};
    add_alert_rule(memory_rule);
    
    HFX_LOG_INFO("✅ System alert rules configured");
}

// Worker thread implementations
void MonitoringSystem::metric_worker() {
    HFX_LOG_INFO("📊 Starting metric worker thread");
    
    while (running_.load()) {
        std::unique_lock<std::mutex> lock(metric_queue_mutex_);
        metric_queue_cv_.wait(lock, [this]() {
            return !metric_queue_.empty() || !running_.load();
        });
        
        while (!metric_queue_.empty() && running_.load()) {
            MetricPoint metric = metric_queue_.front();
            metric_queue_.pop();
            lock.unlock();
            
            process_metric(metric);
            
            lock.lock();
        }
    }
    
    HFX_LOG_INFO("📊 Metric worker thread stopped");
}

void MonitoringSystem::alert_evaluator_worker() {
    HFX_LOG_INFO("🚨 Starting alert evaluator thread");
    
    while (running_.load()) {
        std::this_thread::sleep_for(config_.alert_evaluation_interval);
        
        if (!running_.load()) break;
        
        evaluate_alert_rules();
    }
    
    HFX_LOG_INFO("🚨 Alert evaluator thread stopped");
}

void MonitoringSystem::health_monitor_worker() {
    HFX_LOG_INFO("🏥 Starting health monitor thread");
    
    while (running_.load()) {
        std::this_thread::sleep_for(config_.health_check_interval);
        
        if (!running_.load()) break;
        
        update_system_health();
    }
    
    HFX_LOG_INFO("🏥 Health monitor thread stopped");
}

void MonitoringSystem::cleanup_worker() {
    HFX_LOG_INFO("🧹 Starting cleanup worker thread");
    
    while (running_.load()) {
        std::this_thread::sleep_for(std::chrono::minutes(5));
        
        if (!running_.load()) break;
        
        cleanup_old_metrics();
        cleanup_resolved_alerts();
    }
    
    HFX_LOG_INFO("🧹 Cleanup worker thread stopped");
}

void MonitoringSystem::process_metric(const MetricPoint& metric) {
    auto start_time = std::chrono::high_resolution_clock::now();
    
    {
        std::lock_guard<std::mutex> lock(metrics_mutex_);
        metrics_store_[metric.name].push_back(metric);
        
        // Keep only recent metrics in memory
        auto& metric_list = metrics_store_[metric.name];
        auto cutoff_time = std::chrono::system_clock::now() - config_.metric_retention;
        
        metric_list.erase(
            std::remove_if(metric_list.begin(), metric_list.end(),
                          [cutoff_time](const MetricPoint& m) { 
                              return m.timestamp < cutoff_time; 
                          }),
            metric_list.end());
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto processing_time = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    
    // Update processing time metric
    double current_avg = stats_.avg_metric_processing_time_us.load();
    double new_avg = (current_avg * 0.9) + (processing_time.count() * 0.1);
    stats_.avg_metric_processing_time_us.store(new_avg);
}

void MonitoringSystem::evaluate_alert_rules() {
    auto start_time = std::chrono::high_resolution_clock::now();
    
    std::lock_guard<std::mutex> lock(alerts_mutex_);
    
    for (const auto& rule : alert_rules_) {
        if (!rule.enabled) continue;
        
        evaluate_single_rule(rule);
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto evaluation_time = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    
    double current_avg = stats_.avg_alert_evaluation_time_us.load();
    double new_avg = (current_avg * 0.9) + (evaluation_time.count() * 0.1);
    stats_.avg_alert_evaluation_time_us.store(new_avg);
}

void MonitoringSystem::evaluate_single_rule(const AlertRule& rule) {
    auto recent_metrics = get_recent_metrics(rule.metric_name, rule.evaluation_interval * 2);
    
    if (should_trigger_alert(rule, recent_metrics)) {
        // Check if alert is already active
        bool already_active = false;
        for (const auto& alert : active_alerts_) {
            if (alert.rule_name == rule.name && !alert.resolved) {
                already_active = true;
                break;
            }
        }
        
        if (!already_active) {
            Alert alert;
            alert.rule_name = rule.name;
            alert.message = rule.description;
            alert.severity = rule.severity;
            alert.triggered_at = std::chrono::system_clock::now();
            alert.labels = rule.labels;
            
            trigger_alert_internal(alert);
        }
    }
}

bool MonitoringSystem::should_trigger_alert(const AlertRule& rule, 
                                           const std::vector<MetricPoint>& recent_metrics) const {
    if (recent_metrics.empty()) return false;
    
    switch (rule.condition) {
        case AlertCondition::GREATER_THAN: {
            double latest_value = recent_metrics.back().value;
            return latest_value > rule.threshold;
        }
        case AlertCondition::LESS_THAN: {
            double latest_value = recent_metrics.back().value;
            return latest_value < rule.threshold;
        }
        case AlertCondition::RATE_INCREASE: {
            double rate = calculate_metric_rate(recent_metrics, rule.rate_window);
            return rate > rule.rate_threshold;
        }
        default:
            return false;
    }
}

double MonitoringSystem::calculate_metric_rate(const std::vector<MetricPoint>& metrics, 
                                              std::chrono::seconds window) const {
    if (metrics.size() < 2) return 0.0;
    
    auto now = std::chrono::system_clock::now();
    auto window_start = now - window;
    
    double sum = 0.0;
    int count = 0;
    
    for (const auto& metric : metrics) {
        if (metric.timestamp >= window_start) {
            sum += metric.value;
            count++;
        }
    }
    
    if (count == 0) return 0.0;
    
    // Calculate rate per minute
    double window_minutes = window.count() / 60.0;
    return (sum / window_minutes);
}

void MonitoringSystem::trigger_alert_internal(const Alert& alert) {
    {
        std::lock_guard<std::mutex> lock(alerts_mutex_);
        active_alerts_.push_back(alert);
    }
    
    // Find the rule to get channels
    std::vector<AlertChannel> channels = {AlertChannel::CONSOLE};
    for (const auto& rule : alert_rules_) {
        if (rule.name == alert.rule_name) {
            channels = rule.channels;
            break;
        }
    }
    
    send_alert_to_channels(alert, channels);
    
    stats_.alerts_triggered.fetch_add(1);
    
    HFX_LOG_INFO("🚨 Alert triggered: " + alert.rule_name 
              + " (" + monitoring_utils::severity_to_string(alert.severity) + ")");
}

void MonitoringSystem::send_alert_to_channels(const Alert& alert, const std::vector<AlertChannel>& channels) {
    for (AlertChannel channel : channels) {
        auto it = alert_handlers_.find(channel);
        if (it != alert_handlers_.end()) {
            try {
                it->second(alert);
            } catch (const std::exception& e) {
                HFX_LOG_ERROR("❌ Failed to send alert via " 
                          << monitoring_utils::channel_to_string(channel) 
                          << ": " << e.what() << std::endl;
            }
        }
    }
}

void MonitoringSystem::send_console_alert(const Alert& alert) {
    std::string severity_emoji;
    switch (alert.severity) {
        case AlertSeverity::INFO: severity_emoji = "ℹ️"; break;
        case AlertSeverity::WARNING: severity_emoji = "⚠️"; break;
        case AlertSeverity::ERROR: severity_emoji = "❌"; break;
        case AlertSeverity::CRITICAL: severity_emoji = "🔥"; break;
        case AlertSeverity::EMERGENCY: severity_emoji = "🚨"; break;
    }
    
    HFX_LOG_INFO(severity_emoji + " ALERT [" 
              + monitoring_utils::severity_to_string(alert.severity) 
              + "] " + alert.rule_name + ": " + alert.message 
              + " (at " + monitoring_utils::format_timestamp(alert.triggered_at) + ")");
}

void MonitoringSystem::send_slack_alert(const Alert& alert) {
    if (config_.slack_webhook_url.empty()) return;
    
    // Mock Slack integration - in real implementation would use cURL to send webhook
    HFX_LOG_INFO("📱 Sending Slack alert: " + alert.message);
}

void MonitoringSystem::update_system_health() {
    std::lock_guard<std::mutex> lock(health_mutex_);
    
    current_health_.components.clear();
    current_health_.last_update = std::chrono::system_clock::now();
    
    // Collect health from all registered checkers
    for (const auto& [name, checker] : health_checkers_) {
        try {
            auto component_health = checker();
            component_health.last_check = std::chrono::system_clock::now();
            current_health_.components.push_back(component_health);
        } catch (const std::exception& e) {
            SystemHealth::ComponentHealth failed_health;
            failed_health.name = name;
            failed_health.healthy = false;
            failed_health.status_message = "Health check failed: " + std::string(e.what());
            failed_health.health_score = 0.0;
            failed_health.last_check = std::chrono::system_clock::now();
            current_health_.components.push_back(failed_health);
        }
    }
    
    // Calculate overall health
    double total_score = 0.0;
    bool all_healthy = true;
    uint32_t critical_count = 0;
    
    for (const auto& component : current_health_.components) {
        total_score += component.health_score;
        if (!component.healthy) {
            all_healthy = false;
        }
        if (component.health_score < 0.3) { // Consider < 30% as critical
            critical_count++;
        }
    }
    
    current_health_.overall_healthy = all_healthy;
    current_health_.overall_score = current_health_.components.empty() ? 1.0 : 
                                   total_score / current_health_.components.size();
    current_health_.active_alerts = static_cast<uint32_t>(get_active_alerts().size());
    current_health_.critical_alerts = critical_count;
    
    stats_.health_checks_performed.fetch_add(1);
}

SystemHealth::ComponentHealth MonitoringSystem::create_monitoring_health() const {
    SystemHealth::ComponentHealth health;
    health.name = "monitoring_system";
    health.healthy = running_.load();
    health.status_message = running_.load() ? "Running normally" : "Stopped";
    health.health_score = running_.load() ? 1.0 : 0.0;
    
    // Add monitoring-specific metrics
    health.metrics["metrics_collected"] = static_cast<double>(stats_.metrics_collected.load());
    health.metrics["alerts_triggered"] = static_cast<double>(stats_.alerts_triggered.load());
    health.metrics["avg_processing_time_us"] = stats_.avg_metric_processing_time_us.load();
    health.metrics["active_collectors"] = static_cast<double>(stats_.active_metric_collectors.load());
    health.metrics["active_health_checkers"] = static_cast<double>(stats_.active_health_checkers.load());
    
    return health;
}

void MonitoringSystem::cleanup_old_metrics() {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    
    auto cutoff_time = std::chrono::system_clock::now() - config_.metric_retention;
    size_t total_removed = 0;
    
    for (auto& [name, metrics] : metrics_store_) {
        size_t original_size = metrics.size();
        
        metrics.erase(
            std::remove_if(metrics.begin(), metrics.end(),
                          [cutoff_time](const MetricPoint& m) { 
                              return m.timestamp < cutoff_time; 
                          }),
            metrics.end());
        
        total_removed += (original_size - metrics.size());
    }
    
    if (total_removed > 0) {
        HFX_LOG_INFO("🧹 Cleaned up " + std::to_string(total_removed) + " old metrics");
    }
}

void MonitoringSystem::cleanup_resolved_alerts() {
    std::lock_guard<std::mutex> lock(alerts_mutex_);
    
    auto cutoff_time = std::chrono::system_clock::now() - std::chrono::hours(24);
    size_t original_size = active_alerts_.size();
    
    active_alerts_.erase(
        std::remove_if(active_alerts_.begin(), active_alerts_.end(),
                      [cutoff_time](const Alert& alert) { 
                          return alert.resolved && alert.resolved_at < cutoff_time; 
                      }),
        active_alerts_.end());
    
    size_t removed = original_size - active_alerts_.size();
    if (removed > 0) {
        HFX_LOG_INFO("🧹 Cleaned up " + std::to_string(removed) + " resolved alerts");
    }
}

std::string MonitoringSystem::export_metrics_json() const {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    
    std::stringstream json;
    json << "{\n  \"metrics\": [\n";
    
    bool first_metric = true;
    for (const auto& [name, metrics] : metrics_store_) {
        for (const auto& metric : metrics) {
            if (!first_metric) json << ",\n";
            json << "    " << metric_point_to_json(metric);
            first_metric = false;
        }
    }
    
    json << "\n  ],\n  \"timestamp\": \"" 
         << monitoring_utils::format_timestamp(std::chrono::system_clock::now()) 
         << "\"\n}";
    
    return json.str();
}

std::string MonitoringSystem::metric_point_to_json(const MetricPoint& metric) const {
    std::stringstream json;
    json << "{\n";
    json << "      \"name\": \"" << metric.name << "\",\n";
    json << "      \"value\": " << metric.value << ",\n";
    json << "      \"type\": \"" << monitoring_utils::metric_type_to_string(metric.type) << "\",\n";
    json << "      \"timestamp\": \"" << monitoring_utils::format_timestamp(metric.timestamp) << "\"";
    
    if (!metric.labels.empty()) {
        json << ",\n      \"labels\": {\n";
        bool first_label = true;
        for (const auto& [key, value] : metric.labels) {
            if (!first_label) json << ",\n";
            json << "        \"" << key << "\": \"" << value << "\"";
            first_label = false;
        }
        json << "\n      }";
    }
    
    json << "\n    }";
    return json.str();
}

void MonitoringSystem::reset_stats() {
    stats_.metrics_collected.store(0);
    stats_.alerts_triggered.store(0);
    stats_.alerts_resolved.store(0);
    stats_.health_checks_performed.store(0);
    stats_.avg_metric_processing_time_us.store(0.0);
    stats_.avg_alert_evaluation_time_us.store(0.0);
    
    HFX_LOG_INFO("📊 Monitoring statistics reset");
}

// Utility functions continued
namespace monitoring_utils {
    std::string channel_to_string(AlertChannel channel) {
        switch (channel) {
            case AlertChannel::CONSOLE: return "console";
            case AlertChannel::EMAIL: return "email";
            case AlertChannel::SLACK: return "slack";
            case AlertChannel::WEBHOOK: return "webhook";
            case AlertChannel::SMS: return "sms";
            case AlertChannel::PAGERDUTY: return "pagerduty";
            case AlertChannel::DATADOG: return "datadog";
            case AlertChannel::PROMETHEUS: return "prometheus";
            default: return "unknown";
        }
    }
}

// Factory implementations
std::unique_ptr<MonitoringSystem> MonitoringFactory::create_trading_monitor() {
    MonitoringConfig config;
    config.metric_collection_interval = std::chrono::seconds(5);
    config.health_check_interval = std::chrono::seconds(15);
    config.alert_evaluation_interval = std::chrono::seconds(30);
    config.metric_retention = std::chrono::hours(12);
    config.enable_real_time_processing = true;
    config.metric_worker_threads = 4;
    
    auto monitor = std::make_unique<MonitoringSystem>(config);
    monitor->setup_trading_alerts();
    monitor->setup_system_alerts();
    
    return monitor;
}

std::unique_ptr<MonitoringSystem> MonitoringFactory::create_development_monitor() {
    MonitoringConfig config;
    config.metric_collection_interval = std::chrono::seconds(10);
    config.health_check_interval = std::chrono::seconds(60);
    config.alert_evaluation_interval = std::chrono::seconds(60);
    config.metric_retention = std::chrono::hours(2);
    config.enable_alerting = false; // Less noisy for development
    config.metric_worker_threads = 1;
    
    return std::make_unique<MonitoringSystem>(config);
}

std::unique_ptr<MonitoringSystem> MonitoringFactory::create_production_monitor() {
    MonitoringConfig config;
    config.metric_collection_interval = std::chrono::seconds(1);
    config.health_check_interval = std::chrono::seconds(10);
    config.alert_evaluation_interval = std::chrono::seconds(15);
    config.metric_retention = std::chrono::hours(48);
    config.enable_prometheus_export = true;
    config.enable_file_export = true;
    config.enable_real_time_processing = true;
    config.metric_worker_threads = 8;
    config.alert_worker_threads = 2;
    
    auto monitor = std::make_unique<MonitoringSystem>(config);
    monitor->setup_trading_alerts();
    monitor->setup_system_alerts();
    monitor->setup_performance_alerts();
    
    return monitor;
}

} // namespace hfx::ultra
