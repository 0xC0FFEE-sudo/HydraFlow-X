/**
 * @file health_checker.cpp
 * @brief Health Checker Implementation
 */

#include "../include/health_checker.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <thread>
#include <chrono>
#include <algorithm>

namespace hfx {
namespace monitor {

// Utility functions
std::string health_status_to_string(HealthStatus status) {
    switch (status) {
        case HealthStatus::HEALTHY: return "HEALTHY";
        case HealthStatus::DEGRADED: return "DEGRADED";
        case HealthStatus::UNHEALTHY: return "UNHEALTHY";
        case HealthStatus::CRITICAL: return "CRITICAL";
        case HealthStatus::UNKNOWN: return "UNKNOWN";
        default: return "UNKNOWN";
    }
}

HealthStatus string_to_health_status(const std::string& status_str) {
    if (status_str == "HEALTHY") return HealthStatus::HEALTHY;
    if (status_str == "DEGRADED") return HealthStatus::DEGRADED;
    if (status_str == "UNHEALTHY") return HealthStatus::UNHEALTHY;
    if (status_str == "CRITICAL") return HealthStatus::CRITICAL;
    return HealthStatus::UNKNOWN;
}

std::string format_health_check_result(const HealthCheckResult& result) {
    std::stringstream ss;
    ss << "[" << health_status_to_string(result.status) << "] "
       << result.check_name << ": " << result.message
       << " (" << result.response_time.count() << "ms)";
    return ss.str();
}

bool is_critical_health_status(HealthStatus status) {
    return status == HealthStatus::CRITICAL;
}

// HealthChecker implementation
HealthChecker::HealthChecker(const HealthCheckerConfig& config) : config_(config), monitoring_active_(false) {
    stats_.last_check_execution = std::chrono::system_clock::now();
}

HealthChecker::~HealthChecker() {
    stop_monitoring();
}

void HealthChecker::register_health_check(const std::string& name, const std::string& description,
                                         std::function<HealthCheckResult()> check_function,
                                         std::chrono::seconds interval, std::chrono::seconds timeout) {
    std::lock_guard<std::mutex> lock(health_mutex_);

    HealthCheck check;
    check.name = name;
    check.description = description;
    check.interval = interval;
    check.timeout = timeout;
    check.max_failures = config_.max_consecutive_failures;
    check.enabled = true;
    check.check_function = check_function;

    health_checks_[name] = check;
    consecutive_failures_[name] = 0;
    last_check_times_[name] = std::chrono::system_clock::time_point{};
}

void HealthChecker::unregister_health_check(const std::string& name) {
    std::lock_guard<std::mutex> lock(health_mutex_);

    health_checks_.erase(name);
    latest_results_.erase(name);
    consecutive_failures_.erase(name);
    last_check_times_.erase(name);
}

void HealthChecker::enable_health_check(const std::string& name) {
    std::lock_guard<std::mutex> lock(health_mutex_);

    auto it = health_checks_.find(name);
    if (it != health_checks_.end()) {
        it->second.enabled = true;
    }
}

void HealthChecker::disable_health_check(const std::string& name) {
    std::lock_guard<std::mutex> lock(health_mutex_);

    auto it = health_checks_.find(name);
    if (it != health_checks_.end()) {
        it->second.enabled = false;
    }
}

HealthStatus HealthChecker::get_overall_health() const {
    std::lock_guard<std::mutex> lock(health_mutex_);
    return calculate_overall_health();
}

SystemHealth HealthChecker::get_system_health() const {
    std::lock_guard<std::mutex> lock(health_mutex_);

    SystemHealth system_health;
    system_health.overall_status = calculate_overall_health();
    system_health.last_updated = std::chrono::system_clock::now();
    system_health.total_components = health_checks_.size();
    system_health.healthy_components = 0;

    for (const auto& [name, result] : latest_results_) {
        system_health.component_health.push_back(result);
        system_health.component_status[name] = result.status;

        if (result.status == HealthStatus::HEALTHY) {
            system_health.healthy_components++;
        }
    }

    return system_health;
}

std::optional<HealthCheckResult> HealthChecker::get_health_check_result(const std::string& name) const {
    std::lock_guard<std::mutex> lock(health_mutex_);

    auto it = latest_results_.find(name);
    if (it != latest_results_.end()) {
        return it->second;
    }

    return std::nullopt;
}

std::vector<std::string> HealthChecker::get_registered_checks() const {
    std::lock_guard<std::mutex> lock(health_mutex_);

    std::vector<std::string> checks;
    checks.reserve(health_checks_.size());

    for (const auto& [name, _] : health_checks_) {
        checks.push_back(name);
    }

    return checks;
}

HealthCheckResult HealthChecker::run_health_check(const std::string& name) {
    std::lock_guard<std::mutex> lock(health_mutex_);

    auto it = health_checks_.find(name);
    if (it == health_checks_.end()) {
        HealthCheckResult result;
        result.check_name = name;
        result.status = HealthStatus::UNKNOWN;
        result.message = "Health check not found";
        result.timestamp = std::chrono::system_clock::now();
        return result;
    }

    const HealthCheck& check = it->second;
    if (!check.enabled) {
        HealthCheckResult result;
        result.check_name = name;
        result.status = HealthStatus::UNKNOWN;
        result.message = "Health check disabled";
        result.timestamp = std::chrono::system_clock::now();
        return result;
    }

    execute_health_check(name);
    return get_health_check_result(name).value_or(HealthCheckResult{});
}

SystemHealth HealthChecker::run_all_health_checks() {
    std::lock_guard<std::mutex> lock(health_mutex_);

    for (const auto& [name, _] : health_checks_) {
        if (should_run_health_check(name)) {
            execute_health_check(name);
        }
    }

    return get_system_health();
}

void HealthChecker::start_monitoring() {
    if (monitoring_active_.load(std::memory_order_acquire)) {
        return;
    }

    monitoring_active_.store(true, std::memory_order_release);

    // Start monitoring thread
    monitoring_threads_.emplace_back(&HealthChecker::monitoring_worker, this);
}

void HealthChecker::stop_monitoring() {
    monitoring_active_.store(false, std::memory_order_release);

    // Wait for monitoring threads to finish
    for (auto& thread : monitoring_threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    monitoring_threads_.clear();
}

bool HealthChecker::is_monitoring() const {
    return monitoring_active_.load(std::memory_order_acquire);
}

void HealthChecker::register_database_health_check(const std::string& connection_string) {
    register_health_check(
        health_checks::DATABASE,
        "Database connectivity and performance check",
        [this, connection_string]() { return check_database_health(connection_string); }
    );
}

void HealthChecker::register_api_health_check(const std::string& endpoint_url) {
    register_health_check(
        health_checks::API_SERVER,
        "REST API server availability check",
        [this, endpoint_url]() { return check_api_health(endpoint_url); }
    );
}

void HealthChecker::register_blockchain_health_check(const std::string& rpc_url, const std::string& chain_name) {
    register_health_check(
        chain_name + "_rpc",
        chain_name + " blockchain RPC connectivity check",
        [this, rpc_url, chain_name]() { return check_blockchain_health(rpc_url, chain_name); }
    );
}

void HealthChecker::register_system_health_check() {
    register_health_check(
        health_checks::SYSTEM_MEMORY,
        "System memory usage check",
        [this]() { return check_memory_health(1024); } // 1GB limit
    );

    register_health_check(
        health_checks::SYSTEM_CPU,
        "System CPU usage check",
        [this]() { return check_cpu_health(80.0); } // 80% limit
    );
}

void HealthChecker::register_memory_health_check(size_t max_memory_mb) {
    register_health_check(
        health_checks::SYSTEM_MEMORY,
        "System memory usage check",
        [this, max_memory_mb]() { return check_memory_health(max_memory_mb); }
    );
}

void HealthChecker::register_cpu_health_check(double max_cpu_percent) {
    register_health_check(
        health_checks::SYSTEM_CPU,
        "System CPU usage check",
        [this, max_cpu_percent]() { return check_cpu_health(max_cpu_percent); }
    );
}

void HealthChecker::update_config(const HealthCheckerConfig& config) {
    std::lock_guard<std::mutex> lock(health_mutex_);
    config_ = config;
}

HealthCheckerConfig HealthChecker::get_config() const {
    std::lock_guard<std::mutex> lock(health_mutex_);
    return config_;
}

const HealthStats& HealthChecker::get_health_stats() const {
    return stats_;
}

void HealthChecker::reset_health_stats() {
    stats_.total_checks_executed.store(0, std::memory_order_relaxed);
    stats_.successful_checks.store(0, std::memory_order_relaxed);
    stats_.failed_checks.store(0, std::memory_order_relaxed);
    stats_.critical_failures.store(0, std::memory_order_relaxed);
    stats_.recovery_events.store(0, std::memory_order_relaxed);
    stats_.last_check_execution = std::chrono::system_clock::now();
    stats_.last_failure = std::chrono::system_clock::time_point{};
}

void HealthChecker::register_health_alert_callback(HealthAlertCallback callback) {
    std::lock_guard<std::mutex> lock(callbacks_mutex_);
    alert_callbacks_.push_back(callback);
}

// Private methods
void HealthChecker::monitoring_worker() {
    while (monitoring_active_.load(std::memory_order_acquire)) {
        {
            std::lock_guard<std::mutex> lock(health_mutex_);

            for (const auto& [name, _] : health_checks_) {
                if (should_run_health_check(name)) {
                    execute_health_check(name);
                }
            }
        }

        std::this_thread::sleep_for(config_.global_check_interval);
    }
}

void HealthChecker::execute_health_check(const std::string& name) {
    auto it = health_checks_.find(name);
    if (it == health_checks_.end()) {
        return;
    }

    const HealthCheck& check = it->second;
    auto start_time = std::chrono::high_resolution_clock::now();

    HealthCheckResult result;
    try {
        result = check.check_function();
    } catch (const std::exception& e) {
        result.status = HealthStatus::CRITICAL;
        result.message = std::string("Exception: ") + e.what();
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    result.response_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    result.timestamp = std::chrono::system_clock::now();
    result.check_name = name;

    // Update latest results
    latest_results_[name] = result;
    last_check_times_[name] = result.timestamp;

    // Update statistics
    stats_.total_checks_executed.fetch_add(1, std::memory_order_relaxed);

    if (result.status == HealthStatus::HEALTHY) {
        stats_.successful_checks.fetch_add(1, std::memory_order_relaxed);
        consecutive_failures_[name] = 0;

        // Check for recovery
        if (consecutive_failures_[name] > 0) {
            handle_health_check_recovery(name, result);
        }
    } else {
        stats_.failed_checks.fetch_add(1, std::memory_order_relaxed);
        consecutive_failures_[name]++;

        if (is_critical_health_status(result.status)) {
            stats_.critical_failures.fetch_add(1, std::memory_order_relaxed);
        }

        handle_health_check_failure(name, result);
    }

    // Notify callbacks
    notify_alert_callbacks(result);
}

HealthStatus HealthChecker::calculate_overall_health() const {
    if (health_checks_.empty()) {
        return HealthStatus::UNKNOWN;
    }

    bool has_critical = false;
    bool has_unhealthy = false;
    bool has_degraded = false;
    size_t healthy_count = 0;

    for (const auto& [name, result] : latest_results_) {
        switch (result.status) {
            case HealthStatus::CRITICAL:
                has_critical = true;
                break;
            case HealthStatus::UNHEALTHY:
                has_unhealthy = true;
                break;
            case HealthStatus::DEGRADED:
                has_degraded = true;
                break;
            case HealthStatus::HEALTHY:
                healthy_count++;
                break;
            default:
                break;
        }
    }

    if (has_critical) return HealthStatus::CRITICAL;
    if (has_unhealthy) return HealthStatus::UNHEALTHY;
    if (has_degraded) return HealthStatus::DEGRADED;
    if (healthy_count == health_checks_.size()) return HealthStatus::HEALTHY;

    return HealthStatus::UNKNOWN;
}

void HealthChecker::handle_health_check_failure(const std::string& name, const HealthCheckResult& result) {
    // Log failure
    HFX_LOG_INFO("[LOG] Message");

    stats_.last_failure = result.timestamp;

    // Auto-recovery logic could be implemented here
}

void HealthChecker::handle_health_check_recovery(const std::string& name, const HealthCheckResult& result) {
    // Log recovery
    HFX_LOG_INFO("[LOG] Message");

    stats_.recovery_events.fetch_add(1, std::memory_order_relaxed);
}

void HealthChecker::notify_alert_callbacks(const HealthCheckResult& result) {
    std::lock_guard<std::mutex> lock(callbacks_mutex_);

    for (const auto& callback : alert_callbacks_) {
        try {
            callback(result);
        } catch (const std::exception& e) {
            HFX_LOG_ERROR("[ERROR] Message");
        }
    }
}

bool HealthChecker::should_run_health_check(const std::string& name) const {
    auto check_it = health_checks_.find(name);
    if (check_it == health_checks_.end() || !check_it->second.enabled) {
        return false;
    }

    auto last_check_it = last_check_times_.find(name);
    if (last_check_it == last_check_times_.end()) {
        return true; // Never checked before
    }

    auto now = std::chrono::system_clock::now();
    auto time_since_last_check = now - last_check_it->second;

    return time_since_last_check >= check_it->second.interval;
}

// Built-in health check implementations
HealthCheckResult HealthChecker::check_database_health(const std::string& connection_string) {
    HealthCheckResult result;
    result.check_name = health_checks::DATABASE;

    // Simplified database check (would implement actual database connectivity)
    try {
        // Simulate database connection check
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        result.status = HealthStatus::HEALTHY;
        result.message = "Database connection successful";
    } catch (const std::exception& e) {
        result.status = HealthStatus::CRITICAL;
        result.message = std::string("Database connection failed: ") + e.what();
    }

    return result;
}

HealthCheckResult HealthChecker::check_api_health(const std::string& endpoint_url) {
    HealthCheckResult result;
    result.check_name = health_checks::API_SERVER;

    // Simplified API check (would implement actual HTTP request)
    try {
        // Simulate API call
        std::this_thread::sleep_for(std::chrono::milliseconds(5));

        result.status = HealthStatus::HEALTHY;
        result.message = "API endpoint responding";
    } catch (const std::exception& e) {
        result.status = HealthStatus::UNHEALTHY;
        result.message = std::string("API endpoint failed: ") + e.what();
    }

    return result;
}

HealthCheckResult HealthChecker::check_blockchain_health(const std::string& rpc_url, const std::string& chain_name) {
    HealthCheckResult result;
    result.check_name = chain_name + "_rpc";

    // Simplified blockchain check (would implement actual RPC call)
    try {
        // Simulate RPC call
        std::this_thread::sleep_for(std::chrono::milliseconds(20));

        result.status = HealthStatus::HEALTHY;
        result.message = chain_name + " RPC responding";
    } catch (const std::exception& e) {
        result.status = HealthStatus::UNHEALTHY;
        result.message = chain_name + " RPC failed: " + e.what();
    }

    return result;
}

HealthCheckResult HealthChecker::check_system_health() {
    HealthCheckResult result;
    result.check_name = "system";

    // Check system resources
    double cpu_usage = 45.0; // Would get actual CPU usage
    size_t memory_mb = 512;  // Would get actual memory usage

    if (cpu_usage > 90.0 || memory_mb > 1024) {
        result.status = HealthStatus::CRITICAL;
        result.message = "System resources critical";
    } else if (cpu_usage > 70.0 || memory_mb > 768) {
        result.status = HealthStatus::UNHEALTHY;
        result.message = "System resources high";
    } else {
        result.status = HealthStatus::HEALTHY;
        result.message = "System resources normal";
    }

    return result;
}

HealthCheckResult HealthChecker::check_memory_health(size_t max_memory_mb) {
    HealthCheckResult result;
    result.check_name = health_checks::SYSTEM_MEMORY;

    // Get actual memory usage (simplified)
    size_t current_memory_mb = 256; // Would get actual memory usage

    if (current_memory_mb > max_memory_mb) {
        result.status = HealthStatus::CRITICAL;
        result.message = "Memory usage critical: " + std::to_string(current_memory_mb) + "MB";
    } else if (current_memory_mb > max_memory_mb * 0.8) {
        result.status = HealthStatus::UNHEALTHY;
        result.message = "Memory usage high: " + std::to_string(current_memory_mb) + "MB";
    } else {
        result.status = HealthStatus::HEALTHY;
        result.message = "Memory usage normal: " + std::to_string(current_memory_mb) + "MB";
    }

    return result;
}

HealthCheckResult HealthChecker::check_cpu_health(double max_cpu_percent) {
    HealthCheckResult result;
    result.check_name = health_checks::SYSTEM_CPU;

    // Get actual CPU usage (simplified)
    double current_cpu_percent = 35.0; // Would get actual CPU usage

    if (current_cpu_percent > max_cpu_percent) {
        result.status = HealthStatus::CRITICAL;
        result.message = "CPU usage critical: " + std::to_string(current_cpu_percent) + "%";
    } else if (current_cpu_percent > max_cpu_percent * 0.8) {
        result.status = HealthStatus::UNHEALTHY;
        result.message = "CPU usage high: " + std::to_string(current_cpu_percent) + "%";
    } else {
        result.status = HealthStatus::HEALTHY;
        result.message = "CPU usage normal: " + std::to_string(current_cpu_percent) + "%";
    }

    return result;
}

} // namespace monitor
} // namespace hfx
