/**
 * @file metrics_collector.cpp
 * @brief Metrics Collector Implementation
 */

#include "../include/metrics_collector.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <numeric>

namespace hfx {
namespace monitor {

// Utility functions
std::string metric_type_to_string(MetricType type) {
    switch (type) {
        case MetricType::COUNTER: return "counter";
        case MetricType::GAUGE: return "gauge";
        case MetricType::HISTOGRAM: return "histogram";
        case MetricType::SUMMARY: return "summary";
        default: return "unknown";
    }
}

MetricType string_to_metric_type(const std::string& type_str) {
    if (type_str == "counter") return MetricType::COUNTER;
    if (type_str == "gauge") return MetricType::GAUGE;
    if (type_str == "histogram") return MetricType::HISTOGRAM;
    if (type_str == "summary") return MetricType::SUMMARY;
    return MetricType::GAUGE;
}

std::string sanitize_metric_name(const std::string& name) {
    std::string sanitized = name;
    // Replace invalid characters with underscores
    std::replace_if(sanitized.begin(), sanitized.end(),
                   [](char c) { return !std::isalnum(c) && c != '_'; }, '_');
    return sanitized;
}

// MetricsCollector implementation
MetricsCollector::MetricsCollector(const MetricsConfig& config) : config_(config) {
    stats_.last_collection = std::chrono::system_clock::now();
}

MetricsCollector::~MetricsCollector() = default;

void MetricsCollector::register_metric(const std::string& name, const std::string& description,
                                      MetricType type, const std::vector<std::string>& labels) {
    std::lock_guard<std::mutex> lock(metrics_mutex_);

    if (!validate_metric_name(name)) {
        throw std::invalid_argument("Invalid metric name: " + name);
    }

    Metric metric;
    metric.name = sanitize_metric_name(name);
    metric.description = description;
    metric.type = type;
    metric.labels = labels;
    metric.created_at = std::chrono::system_clock::now();
    metric.last_updated = metric.created_at;

    metric_definitions_[metric.name] = metric;
    stats_.total_metrics_registered.fetch_add(1, std::memory_order_relaxed);
    stats_.current_metric_count.store(metric_definitions_.size(), std::memory_order_relaxed);
}

void MetricsCollector::unregister_metric(const std::string& name) {
    std::lock_guard<std::mutex> lock(metrics_mutex_);

    auto sanitized_name = sanitize_metric_name(name);
    metric_definitions_.erase(sanitized_name);
    metric_values_.erase(sanitized_name);
    labeled_metric_values_.erase(sanitized_name);
    stats_.current_metric_count.store(metric_definitions_.size(), std::memory_order_relaxed);
}

void MetricsCollector::increment_counter(const std::string& name, uint64_t value,
                                        const std::unordered_map<std::string, std::string>& labels) {
    std::lock_guard<std::mutex> lock(metrics_mutex_);

    auto sanitized_name = sanitize_metric_name(name);
    ensure_metric_exists(sanitized_name, MetricType::COUNTER);

    auto key = generate_metric_key(sanitized_name, labels);
    auto& metric_value = labeled_metric_values_[key][sanitized_name];
    metric_value.counter_value += value;

    update_metric_timestamp(sanitized_name);
    stats_.total_updates.fetch_add(1, std::memory_order_relaxed);
}

void MetricsCollector::set_gauge(const std::string& name, double value,
                                const std::unordered_map<std::string, std::string>& labels) {
    std::lock_guard<std::mutex> lock(metrics_mutex_);

    auto sanitized_name = sanitize_metric_name(name);
    ensure_metric_exists(sanitized_name, MetricType::GAUGE);

    auto key = generate_metric_key(sanitized_name, labels);
    labeled_metric_values_[key][sanitized_name].gauge_value = value;

    update_metric_timestamp(sanitized_name);
    stats_.total_updates.fetch_add(1, std::memory_order_relaxed);
}

void MetricsCollector::observe_histogram(const std::string& name, double value,
                                        const std::unordered_map<std::string, std::string>& labels) {
    std::lock_guard<std::mutex> lock(metrics_mutex_);

    auto sanitized_name = sanitize_metric_name(name);
    ensure_metric_exists(sanitized_name, MetricType::HISTOGRAM);

    auto key = generate_metric_key(sanitized_name, labels);
    labeled_metric_values_[key][sanitized_name].histogram_values.push_back(value);

    // Keep only recent values
    if (labeled_metric_values_[key][sanitized_name].histogram_values.size() > 100) {
        labeled_metric_values_[key][sanitized_name].histogram_values.erase(
            labeled_metric_values_[key][sanitized_name].histogram_values.begin());
    }

    update_metric_timestamp(sanitized_name);
    stats_.total_updates.fetch_add(1, std::memory_order_relaxed);
}

void MetricsCollector::observe_summary(const std::string& name, double value,
                                      const std::unordered_map<std::string, std::string>& labels) {
    std::lock_guard<std::mutex> lock(metrics_mutex_);

    auto sanitized_name = sanitize_metric_name(name);
    ensure_metric_exists(sanitized_name, MetricType::SUMMARY);

    auto key = generate_metric_key(sanitized_name, labels);
    auto& summary = labeled_metric_values_[key][sanitized_name];

    // Simple quantiles calculation (placeholder)
    summary.summary_quantiles["p50"] = value;
    summary.summary_quantiles["p95"] = value * 1.1;
    summary.summary_quantiles["p99"] = value * 1.2;

    update_metric_timestamp(sanitized_name);
    stats_.total_updates.fetch_add(1, std::memory_order_relaxed);
}

std::optional<MetricValue> MetricsCollector::get_metric_value(const std::string& name,
                                                             const std::unordered_map<std::string, std::string>& labels) {
    std::lock_guard<std::mutex> lock(metrics_mutex_);

    auto sanitized_name = sanitize_metric_name(name);
    auto key = generate_metric_key(sanitized_name, labels);

    auto labeled_it = labeled_metric_values_.find(key);
    if (labeled_it != labeled_metric_values_.end()) {
        auto value_it = labeled_it->second.find(sanitized_name);
        if (value_it != labeled_it->second.end()) {
            stats_.total_queries.fetch_add(1, std::memory_order_relaxed);
            return value_it->second;
        }
    }

    // Fallback to unlabeled metrics
    auto value_it = metric_values_.find(sanitized_name);
    if (value_it != metric_values_.end()) {
        stats_.total_queries.fetch_add(1, std::memory_order_relaxed);
        return value_it->second;
    }

    return std::nullopt;
}

std::vector<std::string> MetricsCollector::get_metric_names() const {
    std::lock_guard<std::mutex> lock(metrics_mutex_);

    std::vector<std::string> names;
    names.reserve(metric_definitions_.size());

    for (const auto& [name, _] : metric_definitions_) {
        names.push_back(name);
    }

    return names;
}

std::optional<Metric> MetricsCollector::get_metric_info(const std::string& name) const {
    std::lock_guard<std::mutex> lock(metrics_mutex_);

    auto sanitized_name = sanitize_metric_name(name);
    auto it = metric_definitions_.find(sanitized_name);

    if (it != metric_definitions_.end()) {
        return it->second;
    }

    return std::nullopt;
}

void MetricsCollector::update_metrics_batch(const std::unordered_map<std::string, MetricValue>& metrics) {
    std::lock_guard<std::mutex> lock(metrics_mutex_);

    for (const auto& [name, value] : metrics) {
        auto sanitized_name = sanitize_metric_name(name);
        metric_values_[sanitized_name] = value;
        update_metric_timestamp(sanitized_name);
    }

    stats_.total_updates.fetch_add(metrics.size(), std::memory_order_relaxed);
}

std::unordered_map<std::string, MetricValue> MetricsCollector::get_all_metrics() const {
    std::lock_guard<std::mutex> lock(metrics_mutex_);

    std::unordered_map<std::string, MetricValue> all_metrics;

    // Add unlabeled metrics
    for (const auto& [name, value] : metric_values_) {
        all_metrics[name] = value;
    }

    // Add labeled metrics (flatten with keys)
    for (const auto& [key, metrics] : labeled_metric_values_) {
        for (const auto& [name, value] : metrics) {
            std::string full_key = name + "{" + key + "}";
            all_metrics[full_key] = value;
        }
    }

    stats_.total_queries.fetch_add(1, std::memory_order_relaxed);
    return all_metrics;
}

std::string MetricsCollector::export_prometheus_format() const {
    std::lock_guard<std::mutex> lock(metrics_mutex_);

    std::stringstream ss;

    // Add HELP and TYPE comments
    for (const auto& [name, metric] : metric_definitions_) {
        ss << "# HELP " << config_.namespace_name << "_" << name << " " << metric.description << "\n";
        ss << "# TYPE " << config_.namespace_name << "_" << name << " " << metric_type_to_string(metric.type) << "\n";

        // Add metric values
        auto unlabeled_it = metric_values_.find(name);
        if (unlabeled_it != metric_values_.end()) {
            ss << config_.namespace_name << "_" << name << " " << unlabeled_it->second.gauge_value << "\n";
        }

        // Add labeled metrics
        for (const auto& [key, metrics] : labeled_metric_values_) {
            auto labeled_it = metrics.find(name);
            if (labeled_it != metrics.end()) {
                ss << config_.namespace_name << "_" << name << "{" << key << "} "
                   << labeled_it->second.gauge_value << "\n";
            }
        }

        ss << "\n";
    }

    return ss.str();
}

std::string MetricsCollector::export_json_format() const {
    std::lock_guard<std::mutex> lock(metrics_mutex_);

    std::stringstream ss;
    ss << "{\n";

    auto all_metrics = get_all_metrics();
    size_t i = 0;

    for (const auto& [name, value] : all_metrics) {
        ss << "  \"" << name << "\": " << value.gauge_value;
        if (i < all_metrics.size() - 1) {
            ss << ",";
        }
        ss << "\n";
        ++i;
    }

    ss << "}\n";
    return ss.str();
}

const MetricsCollector::CollectorStats& MetricsCollector::get_collector_stats() const {
    return stats_;
}

void MetricsCollector::reset_collector_stats() {
    stats_.total_metrics_registered.store(0, std::memory_order_relaxed);
    stats_.total_updates.store(0, std::memory_order_relaxed);
    stats_.total_queries.store(0, std::memory_order_relaxed);
    stats_.current_metric_count.store(0, std::memory_order_relaxed);
    stats_.last_collection = std::chrono::system_clock::now();
}

void MetricsCollector::update_config(const MetricsConfig& config) {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    config_ = config;
}

MetricsConfig MetricsCollector::get_config() const {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    return config_;
}

void MetricsCollector::cleanup_expired_metrics() {
    // Placeholder for cleanup logic
    stats_.last_collection = std::chrono::system_clock::now();
}

void MetricsCollector::reset_all_metrics() {
    std::lock_guard<std::mutex> lock(metrics_mutex_);

    metric_values_.clear();
    labeled_metric_values_.clear();

    for (auto& [name, metric] : metric_definitions_) {
        metric.last_updated = std::chrono::system_clock::now();
    }
}

// Private methods
std::string MetricsCollector::generate_metric_key(const std::string& name,
                                                 const std::unordered_map<std::string, std::string>& labels) const {
    if (labels.empty()) {
        return "";
    }

    std::stringstream ss;
    bool first = true;

    for (const auto& [key, value] : labels) {
        if (!first) ss << ",";
        ss << key << "=\"" << value << "\"";
        first = false;
    }

    return ss.str();
}

void MetricsCollector::update_metric_timestamp(const std::string& name) {
    auto it = metric_definitions_.find(name);
    if (it != metric_definitions_.end()) {
        it->second.last_updated = std::chrono::system_clock::now();
    }
}

bool MetricsCollector::validate_metric_name(const std::string& name) const {
    if (name.empty()) return false;

    // Must start with letter or underscore
    if (!std::isalpha(name[0]) && name[0] != '_') return false;

    // Must contain only alphanumeric characters and underscores
    for (char c : name) {
        if (!std::isalnum(c) && c != '_') return false;
    }

    return true;
}

void MetricsCollector::ensure_metric_exists(const std::string& name, MetricType type,
                                           const std::vector<std::string>& labels) {
    if (metric_definitions_.find(name) == metric_definitions_.end()) {
        register_metric(name, "Auto-registered metric", type, labels);
    }
}

} // namespace monitor
} // namespace hfx
