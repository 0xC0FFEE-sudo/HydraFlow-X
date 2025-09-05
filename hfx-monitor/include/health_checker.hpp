#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <atomic>
#include <mutex>
#include <memory>
#include <chrono>
#include <functional>
#include <thread>

namespace hfx {
namespace monitor {

// Health status levels
enum class HealthStatus {
    HEALTHY = 0,
    DEGRADED,
    UNHEALTHY,
    CRITICAL,
    UNKNOWN
};

// Health check result
struct HealthCheckResult {
    std::string check_name;
    HealthStatus status;
    std::string message;
    std::chrono::system_clock::time_point timestamp;
    std::chrono::milliseconds response_time;
    std::unordered_map<std::string, std::string> metadata;

    HealthCheckResult() : status(HealthStatus::UNKNOWN), response_time(0) {}
};

// Health check definition
struct HealthCheck {
    std::string name;
    std::string description;
    std::chrono::seconds interval;
    std::chrono::seconds timeout;
    int max_failures;
    bool enabled;
    std::function<HealthCheckResult()> check_function;

    HealthCheck() : interval(std::chrono::seconds(30)),
                   timeout(std::chrono::seconds(5)),
                   max_failures(3),
                   enabled(true) {}
};

// Health checker configuration
struct HealthCheckerConfig {
    std::chrono::seconds global_check_interval = std::chrono::seconds(30);
    std::chrono::seconds global_timeout = std::chrono::seconds(10);
    int max_consecutive_failures = 3;
    bool enable_auto_recovery = true;
    std::chrono::minutes recovery_check_interval = std::chrono::minutes(5);
    bool export_prometheus_metrics = true;
    std::string health_endpoint_path = "/health";
};

// Overall system health
struct SystemHealth {
    HealthStatus overall_status;
    std::chrono::system_clock::time_point last_updated;
    std::vector<HealthCheckResult> component_health;
    std::unordered_map<std::string, HealthStatus> component_status;
    int healthy_components;
    int total_components;

    SystemHealth() : overall_status(HealthStatus::UNKNOWN),
                    healthy_components(0), total_components(0) {}
};

// Health checker class
class HealthChecker {
public:
    explicit HealthChecker(const HealthCheckerConfig& config);
    ~HealthChecker();

    // Health check management
    void register_health_check(const std::string& name, const std::string& description,
                              std::function<HealthCheckResult()> check_function,
                              std::chrono::seconds interval = std::chrono::seconds(30),
                              std::chrono::seconds timeout = std::chrono::seconds(5));

    void unregister_health_check(const std::string& name);
    void enable_health_check(const std::string& name);
    void disable_health_check(const std::string& name);

    // Health status queries
    HealthStatus get_overall_health() const;
    SystemHealth get_system_health() const;
    std::optional<HealthCheckResult> get_health_check_result(const std::string& name) const;
    std::vector<std::string> get_registered_checks() const;

    // Manual health checks
    HealthCheckResult run_health_check(const std::string& name);
    SystemHealth run_all_health_checks();

    // Health check monitoring
    void start_monitoring();
    void stop_monitoring();
    bool is_monitoring() const;

    // Predefined health checks
    void register_database_health_check(const std::string& connection_string);
    void register_api_health_check(const std::string& endpoint_url);
    void register_blockchain_health_check(const std::string& rpc_url, const std::string& chain_name);
    void register_system_health_check();
    void register_memory_health_check(size_t max_memory_mb = 1024);
    void register_cpu_health_check(double max_cpu_percent = 80.0);

    // Configuration
    void update_config(const HealthCheckerConfig& config);
    HealthCheckerConfig get_config() const;

    // Statistics
    struct HealthStats {
        std::atomic<uint64_t> total_checks_executed{0};
        std::atomic<uint64_t> successful_checks{0};
        std::atomic<uint64_t> failed_checks{0};
        std::atomic<uint64_t> critical_failures{0};
        std::atomic<uint64_t> recovery_events{0};
        std::chrono::system_clock::time_point last_check_execution;
        std::chrono::system_clock::time_point last_failure;
    };

    const HealthStats& get_health_stats() const;
    void reset_health_stats();

    // Alert callbacks
    using HealthAlertCallback = std::function<void(const HealthCheckResult&)>;
    void register_health_alert_callback(HealthAlertCallback callback);

private:
    HealthCheckerConfig config_;
    mutable std::mutex health_mutex_;

    // Health check storage
    std::unordered_map<std::string, HealthCheck> health_checks_;
    std::unordered_map<std::string, HealthCheckResult> latest_results_;
    std::unordered_map<std::string, int> consecutive_failures_;

    // Monitoring
    std::atomic<bool> monitoring_active_;
    std::vector<std::thread> monitoring_threads_;
    std::unordered_map<std::string, std::chrono::system_clock::time_point> last_check_times_;

    // Statistics
    mutable HealthStats stats_;

    // Callbacks
    std::vector<HealthAlertCallback> alert_callbacks_;
    mutable std::mutex callbacks_mutex_;

    // Internal methods
    void monitoring_worker();
    void execute_health_check(const std::string& name);
    HealthStatus calculate_overall_health() const;
    void handle_health_check_failure(const std::string& name, const HealthCheckResult& result);
    void handle_health_check_recovery(const std::string& name, const HealthCheckResult& result);
    void notify_alert_callbacks(const HealthCheckResult& result);
    bool should_run_health_check(const std::string& name) const;

    // Built-in health check implementations
    HealthCheckResult check_database_health(const std::string& connection_string);
    HealthCheckResult check_api_health(const std::string& endpoint_url);
    HealthCheckResult check_blockchain_health(const std::string& rpc_url, const std::string& chain_name);
    HealthCheckResult check_system_health();
    HealthCheckResult check_memory_health(size_t max_memory_mb);
    HealthCheckResult check_cpu_health(double max_cpu_percent);
};

// Utility functions
std::string health_status_to_string(HealthStatus status);
HealthStatus string_to_health_status(const std::string& status_str);
std::string format_health_check_result(const HealthCheckResult& result);
bool is_critical_health_status(HealthStatus status);

// Common health check names
namespace health_checks {
    const std::string DATABASE = "database";
    const std::string API_SERVER = "api_server";
    const std::string WEBSOCKET_SERVER = "websocket_server";
    const std::string ETHEREUM_RPC = "ethereum_rpc";
    const std::string SOLANA_RPC = "solana_rpc";
    const std::string SYSTEM_MEMORY = "system_memory";
    const std::string SYSTEM_CPU = "system_cpu";
    const std::string RISK_MANAGER = "risk_manager";
    const std::string MEV_PROTECTOR = "mev_protector";
    const std::string AUTH_SYSTEM = "auth_system";
    const std::string ORDER_ROUTER = "order_router";
}

} // namespace monitor
} // namespace hfx
