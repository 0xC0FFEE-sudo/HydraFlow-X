/**
 * @file alert_manager.cpp
 * @brief Alert Manager Implementation
 */

#include "../include/alert_manager.hpp"
#include <random>
#include <sstream>
#include <iomanip>

namespace hfx {
namespace monitor {

// AlertManager Implementation
AlertManager::AlertManager() = default;

void AlertManager::raise_alert(const std::string& alert_name,
                              AlertSeverity severity,
                              const std::string& message,
                              const std::unordered_map<std::string, std::string>& metadata) {
    if (!should_raise_alert(alert_name)) {
        return;
    }

    std::lock_guard<std::mutex> lock(alerts_mutex_);

    Alert alert;
    alert.alert_id = generate_alert_id();
    alert.alert_name = alert_name;
    alert.severity = severity;
    alert.status = AlertStatus::ACTIVE;
    alert.message = message;
    alert.timestamp = std::chrono::system_clock::now();
    alert.metadata = metadata;

    active_alerts_[alert.alert_id] = alert;
    stats_.total_alerts_raised.fetch_add(1, std::memory_order_relaxed);
    stats_.active_alerts_count.fetch_add(1, std::memory_order_relaxed);
    stats_.last_alert_time = alert.timestamp;

    notify_callbacks(alert);
}

void AlertManager::acknowledge_alert(const std::string& alert_id) {
    std::lock_guard<std::mutex> lock(alerts_mutex_);

    auto it = active_alerts_.find(alert_id);
    if (it != active_alerts_.end()) {
        it->second.status = AlertStatus::ACKNOWLEDGED;
        it->second.acknowledged_at = std::chrono::system_clock::now();
        stats_.total_alerts_acknowledged.fetch_add(1, std::memory_order_relaxed);
    }
}

void AlertManager::resolve_alert(const std::string& alert_id) {
    std::lock_guard<std::mutex> lock(alerts_mutex_);

    auto it = active_alerts_.find(alert_id);
    if (it != active_alerts_.end()) {
        it->second.status = AlertStatus::RESOLVED;
        it->second.resolved_at = std::chrono::system_clock::now();
        active_alerts_.erase(it);
        stats_.total_alerts_resolved.fetch_add(1, std::memory_order_relaxed);
        stats_.active_alerts_count.fetch_sub(1, std::memory_order_relaxed);
    }
}

void AlertManager::configure_alert(const AlertConfig& config) {
    std::lock_guard<std::mutex> lock(alerts_mutex_);
    alert_configs_[config.alert_name] = config;
}

void AlertManager::disable_alert(const std::string& alert_name) {
    std::lock_guard<std::mutex> lock(alerts_mutex_);
    auto it = alert_configs_.find(alert_name);
    if (it != alert_configs_.end()) {
        it->second.enabled = false;
    }
}

void AlertManager::enable_alert(const std::string& alert_name) {
    std::lock_guard<std::mutex> lock(alerts_mutex_);
    auto it = alert_configs_.find(alert_name);
    if (it != alert_configs_.end()) {
        it->second.enabled = true;
    }
}

std::vector<Alert> AlertManager::get_active_alerts() const {
    std::lock_guard<std::mutex> lock(alerts_mutex_);
    std::vector<Alert> alerts;
    alerts.reserve(active_alerts_.size());

    for (const auto& pair : active_alerts_) {
        alerts.push_back(pair.second);
    }

    return alerts;
}

std::vector<Alert> AlertManager::get_alerts_by_severity(AlertSeverity severity) const {
    std::lock_guard<std::mutex> lock(alerts_mutex_);
    std::vector<Alert> alerts;

    for (const auto& pair : active_alerts_) {
        if (pair.second.severity == severity) {
            alerts.push_back(pair.second);
        }
    }

    return alerts;
}

std::optional<Alert> AlertManager::get_alert_by_id(const std::string& alert_id) const {
    std::lock_guard<std::mutex> lock(alerts_mutex_);
    auto it = active_alerts_.find(alert_id);
    if (it != active_alerts_.end()) {
        return it->second;
    }
    return std::nullopt;
}

const AlertManager::AlertStats& AlertManager::get_alert_stats() const {
    return stats_;
}

void AlertManager::reset_alert_stats() {
    stats_.total_alerts_raised.store(0, std::memory_order_relaxed);
    stats_.total_alerts_acknowledged.store(0, std::memory_order_relaxed);
    stats_.total_alerts_resolved.store(0, std::memory_order_relaxed);
    stats_.active_alerts_count.store(0, std::memory_order_relaxed);
}

void AlertManager::register_alert_callback(AlertCallback callback) {
    std::lock_guard<std::mutex> lock(alerts_mutex_);
    alert_callbacks_.push_back(callback);
}

std::string AlertManager::generate_alert_id() const {
    static std::atomic<uint64_t> counter{0};
    uint64_t id = counter.fetch_add(1, std::memory_order_relaxed);

    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);

    std::stringstream ss;
    ss << "alert_" << time_t << "_" << id;
    return ss.str();
}

bool AlertManager::should_raise_alert(const std::string& alert_name) const {
    auto it = alert_configs_.find(alert_name);
    if (it != alert_configs_.end()) {
        return it->second.enabled;
    }
    return true; // Default to enabled if not configured
}

void AlertManager::notify_callbacks(const Alert& alert) {
    for (const auto& callback : alert_callbacks_) {
        try {
            callback(alert);
        } catch (const std::exception& e) {
            // Log error but continue with other callbacks
            HFX_LOG_ERROR("Alert callback error: " << e.what() << std::endl;
        }
    }
}

} // namespace monitor
} // namespace hfx