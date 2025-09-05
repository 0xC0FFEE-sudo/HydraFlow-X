#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <atomic>
#include <mutex>
#include <chrono>
#include <functional>

namespace hfx {
namespace monitor {

// Alert severity levels
enum class AlertSeverity {
    LOW = 0,
    MEDIUM,
    HIGH,
    CRITICAL,
    EMERGENCY
};

// Alert status
enum class AlertStatus {
    ACTIVE = 0,
    ACKNOWLEDGED,
    RESOLVED
};

// Alert configuration
struct AlertConfig {
    std::string alert_name;
    AlertSeverity severity;
    std::string message_template;
    std::chrono::seconds cooldown_period;
    bool enabled;

    AlertConfig() : severity(AlertSeverity::MEDIUM),
                   cooldown_period(std::chrono::seconds(300)),
                   enabled(true) {}
};

// Alert data structure
struct Alert {
    std::string alert_id;
    std::string alert_name;
    AlertSeverity severity;
    AlertStatus status;
    std::string message;
    std::chrono::system_clock::time_point timestamp;
    std::chrono::system_clock::time_point acknowledged_at;
    std::chrono::system_clock::time_point resolved_at;
    std::unordered_map<std::string, std::string> metadata;

    Alert() : severity(AlertSeverity::MEDIUM),
             status(AlertStatus::ACTIVE),
             timestamp(std::chrono::system_clock::now()) {}
};

// Alert manager class
class AlertManager {
public:
    explicit AlertManager();
    ~AlertManager() = default;

    // Alert management
    void raise_alert(const std::string& alert_name,
                    AlertSeverity severity,
                    const std::string& message,
                    const std::unordered_map<std::string, std::string>& metadata = {});

    void acknowledge_alert(const std::string& alert_id);
    void resolve_alert(const std::string& alert_id);

    // Configuration
    void configure_alert(const AlertConfig& config);
    void disable_alert(const std::string& alert_name);
    void enable_alert(const std::string& alert_name);

    // Query methods
    std::vector<Alert> get_active_alerts() const;
    std::vector<Alert> get_alerts_by_severity(AlertSeverity severity) const;
    std::optional<Alert> get_alert_by_id(const std::string& alert_id) const;

    // Statistics
    struct AlertStats {
        std::atomic<uint64_t> total_alerts_raised{0};
        std::atomic<uint64_t> total_alerts_acknowledged{0};
        std::atomic<uint64_t> total_alerts_resolved{0};
        std::atomic<size_t> active_alerts_count{0};
        std::chrono::system_clock::time_point last_alert_time;
    };

    const AlertStats& get_alert_stats() const;
    void reset_alert_stats();

    // Callbacks
    using AlertCallback = std::function<void(const Alert&)>;
    void register_alert_callback(AlertCallback callback);

private:
    mutable std::mutex alerts_mutex_;
    std::unordered_map<std::string, Alert> active_alerts_;
    std::unordered_map<std::string, AlertConfig> alert_configs_;
    std::vector<AlertCallback> alert_callbacks_;
    AlertStats stats_;

    // Helper methods
    std::string generate_alert_id() const;
    bool should_raise_alert(const std::string& alert_name) const;
    void notify_callbacks(const Alert& alert);
};

} // namespace monitor
} // namespace hfx
