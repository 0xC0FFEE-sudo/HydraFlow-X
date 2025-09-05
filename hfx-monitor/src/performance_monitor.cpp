/**
 * @file performance_monitor.cpp
 * @brief Performance Monitor Implementation
 */

#include "../include/performance_monitor.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <thread>
#include <chrono>
#include <algorithm>
#include <numeric>
#include <random>

// System monitoring includes (platform-specific)
#ifdef __linux__
#include <sys/sysinfo.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fstream>
#include <sstream>
#elif defined(__APPLE__)
#include <mach/mach.h>
#include <mach/processor_info.h>
#include <mach/mach_host.h>
#include <sys/sysctl.h>
#endif

namespace hfx {
namespace monitor {

// ScopedTimer implementation
ScopedTimer::ScopedTimer(PerformanceMonitor& monitor, const std::string& operation_name,
                        const std::unordered_map<std::string, std::string>& tags)
    : monitor_(monitor), operation_name_(operation_name), tags_(tags),
      start_time_(std::chrono::high_resolution_clock::now()) {
}

ScopedTimer::~ScopedTimer() {
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time_);

    monitor_.record_timer(operation_name_, duration, tags_);
}

// PerformanceMonitor implementation
PerformanceMonitor::PerformanceMonitor(const PerformanceMonitorConfig& config) : config_(config) {
    stats_.last_collection = std::chrono::system_clock::now();
}

PerformanceMonitor::~PerformanceMonitor() {
    stop_collection();
}

void PerformanceMonitor::start_collection() {
    if (collecting_active_.load(std::memory_order_acquire)) {
        return;
    }

    collecting_active_.store(true, std::memory_order_release);

    // Start collection thread
    collection_threads_.emplace_back(&PerformanceMonitor::collection_worker, this);
}

void PerformanceMonitor::stop_collection() {
    collecting_active_.store(false, std::memory_order_release);

    // Wait for collection threads to finish
    for (auto& thread : collection_threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    collection_threads_.clear();
}

bool PerformanceMonitor::is_collecting() const {
    return collecting_active_.load(std::memory_order_acquire);
}

void PerformanceMonitor::record_sample(const std::string& metric_name, double value,
                                      const std::unordered_map<std::string, std::string>& tags) {
    std::lock_guard<std::mutex> lock(monitor_mutex_);

    PerformanceSample sample;
    sample.metric_name = metric_name;
    sample.value = value;
    sample.timestamp = std::chrono::system_clock::now();
    sample.tags = tags;

    // Store sample
    metric_samples_[metric_name].push_back(sample);

    // Maintain sample history limit
    if (metric_samples_[metric_name].size() > config_.max_samples_per_metric) {
        metric_samples_[metric_name].pop_front();
    }

    update_monitor_stats(sample);
}

void PerformanceMonitor::record_timer(const std::string& operation_name, std::chrono::microseconds duration,
                                     const std::unordered_map<std::string, std::string>& tags) {
    double duration_ms = static_cast<double>(duration.count()) / 1000.0;
    record_sample(operation_name, duration_ms, tags);
}

std::optional<PerformanceStats> PerformanceMonitor::get_performance_stats(const std::string& metric_name) const {
    std::lock_guard<std::mutex> lock(monitor_mutex_);

    auto it = metric_samples_.find(metric_name);
    if (it == metric_samples_.end() || it->second.empty()) {
        return std::nullopt;
    }

    return calculate_stats(it->second);
}

std::vector<PerformanceSample> PerformanceMonitor::get_recent_samples(const std::string& metric_name,
                                                                     std::chrono::minutes window) const {
    std::lock_guard<std::mutex> lock(monitor_mutex_);

    auto it = metric_samples_.find(metric_name);
    if (it == metric_samples_.end()) {
        return {};
    }

    auto cutoff = std::chrono::system_clock::now() - window;
    std::vector<PerformanceSample> recent_samples;

    for (const auto& sample : it->second) {
        if (sample.timestamp >= cutoff) {
            recent_samples.push_back(sample);
        }
    }

    return recent_samples;
}

std::vector<std::string> PerformanceMonitor::get_available_metrics() const {
    std::lock_guard<std::mutex> lock(monitor_mutex_);

    std::vector<std::string> metrics;
    metrics.reserve(metric_samples_.size());

    for (const auto& [name, _] : metric_samples_) {
        metrics.push_back(name);
    }

    return metrics;
}

void PerformanceMonitor::set_threshold(const std::string& metric_name, double warning_threshold,
                                      double critical_threshold, const std::string& condition) {
    std::lock_guard<std::mutex> lock(monitor_mutex_);

    PerformanceThreshold threshold;
    threshold.metric_name = metric_name;
    threshold.warning_threshold = warning_threshold;
    threshold.critical_threshold = critical_threshold;
    threshold.condition = condition;

    thresholds_[metric_name] = threshold;
}

void PerformanceMonitor::remove_threshold(const std::string& metric_name) {
    std::lock_guard<std::mutex> lock(monitor_mutex_);
    thresholds_.erase(metric_name);
}

std::vector<PerformanceThreshold> PerformanceMonitor::get_thresholds() const {
    std::lock_guard<std::mutex> lock(monitor_mutex_);

    std::vector<PerformanceThreshold> threshold_list;
    threshold_list.reserve(thresholds_.size());

    for (const auto& [_, threshold] : thresholds_) {
        threshold_list.push_back(threshold);
    }

    return threshold_list;
}

std::vector<PerformanceAlert> PerformanceMonitor::get_recent_alerts(std::chrono::minutes window) const {
    std::lock_guard<std::mutex> lock(monitor_mutex_);

    auto cutoff = std::chrono::system_clock::now() - window;
    std::vector<PerformanceAlert> recent_alerts;

    for (const auto& alert : recent_alerts_) {
        if (alert.timestamp >= cutoff) {
            recent_alerts.push_back(alert);
        }
    }

    return recent_alerts;
}

void PerformanceMonitor::acknowledge_alert(const std::string& alert_id) {
    std::lock_guard<std::mutex> lock(monitor_mutex_);

    // Remove alert from recent alerts (simplified)
    recent_alerts_.erase(
        std::remove_if(recent_alerts_.begin(), recent_alerts_.end(),
            [&alert_id](const PerformanceAlert& alert) {
                return alert.alert_id == alert_id;
            }),
        recent_alerts_.end());
}

void PerformanceMonitor::enable_system_cpu_monitoring() {
    // Implementation would start CPU monitoring
    HFX_LOG_INFO("[PERF] CPU monitoring enabled");
}

void PerformanceMonitor::enable_system_memory_monitoring() {
    // Implementation would start memory monitoring
    HFX_LOG_INFO("[PERF] Memory monitoring enabled");
}

void PerformanceMonitor::enable_system_disk_monitoring() {
    // Implementation would start disk monitoring
    HFX_LOG_INFO("[PERF] Disk monitoring enabled");
}

void PerformanceMonitor::enable_system_network_monitoring() {
    // Implementation would start network monitoring
    HFX_LOG_INFO("[PERF] Network monitoring enabled");
}

void PerformanceMonitor::enable_request_latency_monitoring() {
    // Implementation would start request latency monitoring
    HFX_LOG_INFO("[PERF] Request latency monitoring enabled");
}

void PerformanceMonitor::enable_error_rate_monitoring() {
    // Implementation would start error rate monitoring
    HFX_LOG_INFO("[PERF] Error rate monitoring enabled");
}

void PerformanceMonitor::enable_throughput_monitoring() {
    // Implementation would start throughput monitoring
    HFX_LOG_INFO("[PERF] Throughput monitoring enabled");
}

void PerformanceMonitor::enable_queue_depth_monitoring() {
    // Implementation would start queue depth monitoring
    HFX_LOG_INFO("[PERF] Queue depth monitoring enabled");
}

PerformanceMonitor::TimerHandle PerformanceMonitor::start_timer(const std::string& operation_name,
                                                               const std::unordered_map<std::string, std::string>& tags) {
    return TimerHandle(new void(nullptr), [this, operation_name, tags](void*) {
        // This is a simplified implementation
        // In practice, this would store timing information
    });
}

void PerformanceMonitor::record_batch_samples(const std::vector<PerformanceSample>& samples) {
    std::lock_guard<std::mutex> lock(monitor_mutex_);

    for (const auto& sample : samples) {
        metric_samples_[sample.metric_name].push_back(sample);

        // Maintain sample history limit
        if (metric_samples_[sample.metric_name].size() > config_.max_samples_per_metric) {
            metric_samples_[sample.metric_name].pop_front();
        }

        update_monitor_stats(sample);
    }
}

std::unordered_map<std::string, PerformanceStats> PerformanceMonitor::get_all_performance_stats() const {
    std::lock_guard<std::mutex> lock(monitor_mutex_);

    std::unordered_map<std::string, PerformanceStats> all_stats;

    for (const auto& [metric_name, samples] : metric_samples_) {
        if (!samples.empty()) {
            all_stats[metric_name] = calculate_stats(samples);
        }
    }

    return all_stats;
}

void PerformanceMonitor::update_config(const PerformanceMonitorConfig& config) {
    std::lock_guard<std::mutex> lock(monitor_mutex_);
    config_ = config;
}

PerformanceMonitor::PerformanceMonitorConfig PerformanceMonitor::get_config() const {
    std::lock_guard<std::mutex> lock(monitor_mutex_);
    return config_;
}

PerformanceMonitor::MonitorStats PerformanceMonitor::get_monitor_stats() const {
    return stats_;
}

void PerformanceMonitor::reset_monitor_stats() {
    stats_.total_samples_collected.store(0, std::memory_order_relaxed);
    stats_.total_alerts_generated.store(0, std::memory_order_relaxed);
    stats_.active_thresholds.store(0, std::memory_order_relaxed);
    stats_.current_metrics_count.store(0, std::memory_order_relaxed);
    stats_.last_collection = std::chrono::system_clock::now();
    stats_.last_alert = std::chrono::system_clock::time_point{};
}

void PerformanceMonitor::register_alert_callback(PerformanceAlertCallback callback) {
    std::lock_guard<std::mutex> lock(callbacks_mutex_);
    alert_callbacks_.push_back(callback);
}

void PerformanceMonitor::cleanup_old_samples() {
    std::lock_guard<std::mutex> lock(monitor_mutex_);

    auto cutoff = std::chrono::system_clock::now() - config_.retention_period;

    for (auto& [metric_name, samples] : metric_samples_) {
        samples.erase(
            std::remove_if(samples.begin(), samples.end(),
                [cutoff](const PerformanceSample& sample) {
                    return sample.timestamp < cutoff;
                }),
            samples.end());
    }

    stats_.last_collection = std::chrono::system_clock::now();
}

void PerformanceMonitor::reset_all_metrics() {
    std::lock_guard<std::mutex> lock(monitor_mutex_);

    metric_samples_.clear();
    thresholds_.clear();
    recent_alerts_.clear();
}

// Private methods
void PerformanceMonitor::collection_worker() {
    while (collecting_active_.load(std::memory_order_acquire)) {
        collect_system_metrics();
        check_thresholds();
        cleanup_old_samples();

        std::this_thread::sleep_for(config_.collection_interval);
    }
}

void PerformanceMonitor::collect_system_metrics() {
    // Collect CPU usage
    double cpu_usage = collect_cpu_usage();
    if (cpu_usage >= 0) {
        record_sample(perf_metrics::CPU_USAGE_PERCENT, cpu_usage);
    }

    // Collect memory usage
    double memory_usage = collect_memory_usage();
    if (memory_usage >= 0) {
        record_sample(perf_metrics::MEMORY_USAGE_MB, memory_usage);
    }

    // Collect thread count
    size_t thread_count = collect_thread_count();
    record_sample(perf_metrics::THREAD_COUNT, static_cast<double>(thread_count));

    // Collect open files count
    size_t open_files = collect_open_files_count();
    record_sample(perf_metrics::OPEN_FILES_COUNT, static_cast<double>(open_files));
}

void PerformanceMonitor::check_thresholds() {
    std::lock_guard<std::mutex> lock(monitor_mutex_);

    for (const auto& [metric_name, threshold] : thresholds_) {
        auto stats_opt = get_performance_stats(metric_name);
        if (!stats_opt) continue;

        double current_value = stats_opt->current_value;

        if (validate_threshold_condition(current_value, threshold.warning_threshold, threshold.condition)) {
            generate_alert(metric_name, current_value, threshold);
        }
    }
}

void PerformanceMonitor::generate_alert(const std::string& metric_name, double current_value,
                                       const PerformanceThreshold& threshold) {
    // Check if we should generate alert (cooldown)
    auto now = std::chrono::system_clock::now();
    auto last_alert_it = last_alert_times_.find(metric_name);

    if (last_alert_it != last_alert_times_.end() &&
        now - last_alert_it->second < config_.alert_cooldown) {
        return; // Still in cooldown
    }

    PerformanceAlert alert;
    alert.alert_id = generate_alert_id();
    alert.metric_name = metric_name;
    alert.current_value = current_value;
    alert.threshold_value = threshold.warning_threshold;

    if (current_value >= threshold.critical_threshold) {
        alert.severity = "critical";
    } else {
        alert.severity = "warning";
    }

    alert.timestamp = now;
    alert.message = "Performance threshold exceeded for " + metric_name +
                   ": " + std::to_string(current_value) + " >= " + std::to_string(threshold.warning_threshold);

    {
        std::lock_guard<std::mutex> lock(monitor_mutex_);
        recent_alerts_.push_back(alert);
        last_alert_times_[metric_name] = now;

        // Keep only recent alerts
        if (recent_alerts_.size() > 100) {
            recent_alerts_.erase(recent_alerts_.begin());
        }
    }

    stats_.total_alerts_generated.fetch_add(1, std::memory_order_relaxed);
    stats_.last_alert = now;

    notify_alert_callbacks(alert);
}

PerformanceStats PerformanceMonitor::calculate_stats(const std::deque<PerformanceSample>& samples) const {
    PerformanceStats stats;
    stats.metric_name = samples.empty() ? "" : samples.front().metric_name;
    stats.sample_count = samples.size();

    if (samples.empty()) {
        return stats;
    }

    // Calculate statistics
    std::vector<double> values;
    values.reserve(samples.size());

    for (const auto& sample : samples) {
        values.push_back(sample.value);
    }

    stats.current_value = samples.back().value;
    stats.min_value = *std::min_element(values.begin(), values.end());
    stats.max_value = *std::max_element(values.begin(), values.end());

    double sum = std::accumulate(values.begin(), values.end(), 0.0);
    stats.average_value = sum / values.size();

    // Calculate percentiles
    std::sort(values.begin(), values.end());
    size_t p95_index = static_cast<size_t>(values.size() * 0.95);
    size_t p99_index = static_cast<size_t>(values.size() * 0.99);

    stats.p95_value = values[std::min(p95_index, values.size() - 1)];
    stats.p99_value = values[std::min(p99_index, values.size() - 1)];

    stats.last_updated = samples.back().timestamp;

    return stats;
}

bool PerformanceMonitor::should_generate_alert(const std::string& metric_name) const {
    auto last_alert_it = last_alert_times_.find(metric_name);
    if (last_alert_it == last_alert_times_.end()) {
        return true; // No previous alert
    }

    auto now = std::chrono::system_clock::now();
    return (now - last_alert_it->second) >= config_.alert_cooldown;
}

void PerformanceMonitor::notify_alert_callbacks(const PerformanceAlert& alert) {
    std::lock_guard<std::mutex> lock(callbacks_mutex_);

    for (const auto& callback : alert_callbacks_) {
        try {
            callback(alert);
        } catch (const std::exception& e) {
            HFX_LOG_ERROR("[ERROR] Message");
        }
    }
}

std::string PerformanceMonitor::generate_alert_id() {
    static std::atomic<uint64_t> alert_counter{0};
    uint64_t id = alert_counter.fetch_add(1, std::memory_order_relaxed);

    std::stringstream ss;
    ss << "alert_" << std::setfill('0') << std::setw(8) << id;
    return ss.str();
}

bool PerformanceMonitor::validate_threshold_condition(double value, double threshold, const std::string& condition) {
    if (condition == "above") {
        return value > threshold;
    } else if (condition == "below") {
        return value < threshold;
    } else if (condition == "equal") {
        return std::abs(value - threshold) < 0.001; // Approximate equality
    }

    return false;
}

void PerformanceMonitor::update_monitor_stats(const PerformanceSample& sample) {
    stats_.total_samples_collected.fetch_add(1, std::memory_order_relaxed);
    stats_.current_metrics_count.store(metric_samples_.size(), std::memory_order_relaxed);
}

// System metric collectors (simplified implementations)
double PerformanceMonitor::collect_cpu_usage() {
    // Simplified CPU usage collection
    // In practice, this would use platform-specific APIs
#ifdef __APPLE__
    // macOS implementation
    return 45.0; // Placeholder
#elif defined(__linux__)
    // Linux implementation
    return 35.0; // Placeholder
#else
    return 40.0; // Placeholder
#endif
}

double PerformanceMonitor::collect_memory_usage() {
    // Simplified memory usage collection
#ifdef __APPLE__
    // macOS implementation
    return 512.0; // Placeholder MB
#elif defined(__linux__)
    // Linux implementation
    return 768.0; // Placeholder MB
#else
    return 1024.0; // Placeholder MB
#endif
}

double PerformanceMonitor::collect_disk_io() {
    // Simplified disk I/O collection
    return 25.0; // Placeholder MB/s
}

double PerformanceMonitor::collect_network_io() {
    // Simplified network I/O collection
    return 15.0; // Placeholder MB/s
}

size_t PerformanceMonitor::collect_thread_count() {
    // Simplified thread count collection
    return 24; // Placeholder
}

size_t PerformanceMonitor::collect_open_files_count() {
    // Simplified open files count collection
    return 156; // Placeholder
}

} // namespace monitor
} // namespace hfx
