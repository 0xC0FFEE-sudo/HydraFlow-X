/**
 * @file security_middleware.cpp
 * @brief Security Middleware Implementation
 */

#include "../include/security_middleware.hpp"
#include "../include/rate_limiter.hpp"
#include "../include/ddos_protection.hpp"
#include "../include/api_security.hpp"
#include <iostream>
#include <sstream>
#include <chrono>

// Forward declarations for internal classes
namespace hfx {
namespace security {
class RateLimiter;
class DDoSProtection;
class APISecurity;
}
}

namespace hfx {
namespace security {

// These structs are now defined in the header file

// Security middleware class
class SecurityMiddleware::Impl {
public:
    explicit Impl(const SecurityMiddlewareConfig& config);
    ~Impl();

    SecurityMiddlewareResult process_request(const HttpRequest& request);
    void update_config(const SecurityMiddlewareConfig& config);

private:
    SecurityMiddlewareConfig config_;
    std::unique_ptr<RateLimiter> rate_limiter_;
    std::unique_ptr<DDoSProtection> ddos_protection_;
    std::unique_ptr<APISecurity> api_security_;

    mutable std::mutex config_mutex_;

    // Statistics
    struct MiddlewareStats {
        std::atomic<uint64_t> total_requests_processed{0};
        std::atomic<uint64_t> requests_allowed{0};
        std::atomic<uint64_t> requests_blocked{0};
        std::atomic<uint64_t> rate_limited_requests{0};
        std::atomic<uint64_t> ddos_blocked_requests{0};
        std::atomic<uint64_t> security_violations{0};
        std::chrono::system_clock::time_point last_request_processed;
    };

    mutable MiddlewareStats stats_;

    // Internal methods
    SecurityMiddlewareResult apply_rate_limiting(const HttpRequest& request);
    SecurityMiddlewareResult apply_ddos_protection(const HttpRequest& request);
    SecurityMiddlewareResult apply_api_security(const HttpRequest& request);
    SecurityMiddlewareResult apply_cors_policy(const HttpRequest& request);
    void add_security_headers(SecurityMiddlewareResult& result);
    void log_security_event(const std::string& event_type, const std::string& details);
    bool is_path_blocked(const std::string& path) const;
};

// Implementation
SecurityMiddleware::Impl::Impl(const SecurityMiddlewareConfig& config) : config_(config) {
    rate_limiter_ = std::make_unique<RateLimiter>(config.rate_limit_config);
    ddos_protection_ = std::make_unique<DDoSProtection>(config.ddos_config);
    api_security_ = std::make_unique<APISecurity>(config.api_security_config);
}

SecurityMiddleware::Impl::~Impl() = default;

SecurityMiddlewareResult SecurityMiddleware::Impl::process_request(const HttpRequest& request) {
    auto start_time = std::chrono::high_resolution_clock::now();
    SecurityMiddlewareResult result;
    stats_.total_requests_processed.fetch_add(1, std::memory_order_relaxed);

    try {
        // Check if path is blocked
        if (is_path_blocked(request.path)) {
            result.allowed = false;
            result.reason = "Path blocked by security policy";
            stats_.requests_blocked.fetch_add(1, std::memory_order_relaxed);
            log_security_event("PATH_BLOCKED", "Blocked request to: " + request.path);
            return result;
        }

        // Apply rate limiting
        auto rate_limit_result = apply_rate_limiting(request);
        if (!rate_limit_result.allowed) {
            result = rate_limit_result;
            stats_.rate_limited_requests.fetch_add(1, std::memory_order_relaxed);
            return result;
        }

        // Apply DDoS protection
        auto ddos_result = apply_ddos_protection(request);
        if (!ddos_result.allowed) {
            result = ddos_result;
            stats_.ddos_blocked_requests.fetch_add(1, std::memory_order_relaxed);
            return result;
        }

        // Apply API security
        auto api_security_result = apply_api_security(request);
        if (!api_security_result.allowed) {
            result = api_security_result;
            stats_.security_violations.fetch_add(1, std::memory_order_relaxed);
            return result;
        }

        // Apply CORS policy
        auto cors_result = apply_cors_policy(request);
        if (!cors_result.allowed) {
            result = cors_result;
            return result;
        }

        // Add security headers
        add_security_headers(result);

        // All checks passed
        result.allowed = true;
        stats_.requests_allowed.fetch_add(1, std::memory_order_relaxed);
        stats_.last_request_processed = std::chrono::system_clock::now();

    } catch (const std::exception& e) {
        result.allowed = false;
        result.reason = "Security middleware error: " + std::string(e.what());
        stats_.requests_blocked.fetch_add(1, std::memory_order_relaxed);
        log_security_event("MIDDLEWARE_ERROR", std::string(e.what()));
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    result.processing_time = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);

    return result;
}

void SecurityMiddleware::Impl::update_config(const SecurityMiddlewareConfig& config) {
    std::lock_guard<std::mutex> lock(config_mutex_);
    config_ = config;

    // Update individual components
    rate_limiter_->update_config(config.rate_limit_config);
    ddos_protection_->update_config(config.ddos_config);
    api_security_->update_config(config.api_security_config);
}

SecurityMiddlewareResult SecurityMiddleware::Impl::apply_rate_limiting(const HttpRequest& request) {
    SecurityMiddlewareResult result;

    // Extract client IP
    std::string client_ip = request.headers.at("X-Forwarded-For");
    if (client_ip.empty()) {
        client_ip = request.headers.at("X-Real-IP");
    }
    if (client_ip.empty()) {
        client_ip = "unknown";
    }

    // Check rate limit
    auto rate_limit_status = rate_limiter_->check_rate_limit(client_ip, request.path, client_ip);

    if (!rate_limit_status.allowed) {
        result.allowed = false;
        result.reason = "Rate limit exceeded";

        // Add rate limit headers
        result.security_headers["X-RateLimit-Limit"] = std::to_string(rate_limit_status.remaining_requests);
        result.security_headers["X-RateLimit-Remaining"] = "0";
        result.security_headers["X-RateLimit-Reset"] = std::to_string(rate_limit_status.reset_time.count());
        result.security_headers["Retry-After"] = std::to_string(rate_limit_status.reset_time.count());

        log_security_event("RATE_LIMIT_EXCEEDED", "IP: " + client_ip + ", Path: " + request.path);
    }

    return result;
}

SecurityMiddlewareResult SecurityMiddleware::Impl::apply_ddos_protection(const HttpRequest& request) {
    SecurityMiddlewareResult result;

    // Extract client IP
    std::string client_ip = request.headers.at("X-Forwarded-For");
    if (client_ip.empty()) {
        client_ip = request.headers.at("X-Real-IP");
    }
    if (client_ip.empty()) {
        client_ip = "unknown";
    }

    // Extract user agent
    std::string user_agent = request.headers.at("User-Agent");

    // Analyze traffic
    auto protection_action = ddos_protection_->analyze_traffic(
        client_ip, request.path, user_agent, request.method);

    switch (protection_action) {
        case DDoSProtectionAction::BLOCK:
            result.allowed = false;
            result.reason = "IP blocked by DDoS protection";
            result.security_headers["X-DDoS-Protection"] = "blocked";
            log_security_event("DDOS_BLOCKED", "IP: " + client_ip);
            break;

        case DDoSProtectionAction::RATE_LIMIT:
            result.allowed = false;
            result.reason = "Rate limited by DDoS protection";
            result.security_headers["X-DDoS-Protection"] = "rate_limited";
            break;

        case DDoSProtectionAction::CAPTCHA:
            result.allowed = false;
            result.reason = "CAPTCHA required by DDoS protection";
            result.security_headers["X-DDoS-Protection"] = "captcha_required";
            break;

        case DDoSProtectionAction::ALLOW:
        default:
            result.allowed = true;
            result.security_headers["X-DDoS-Protection"] = "allowed";
            break;
    }

    return result;
}

SecurityMiddlewareResult SecurityMiddleware::Impl::apply_api_security(const HttpRequest& request) {
    SecurityMiddlewareResult result;

    // Validate request
    auto validation_result = api_security_->validate_request(
        request.method,
        request.path,
        request.body,
        request.headers
    );

    if (!validation_result.valid) {
        result.allowed = false;
        result.reason = validation_result.description;

        // Add security violation headers
        result.security_headers["X-Security-Violation"] = api_security_event_to_string(validation_result.event_type);
        result.security_headers["X-Security-Score"] = std::to_string(validation_result.severity_score);

        log_security_event("SECURITY_VIOLATION",
                          "Type: " + api_security_event_to_string(validation_result.event_type) +
                          ", Path: " + request.path);
    }

    return result;
}

SecurityMiddlewareResult SecurityMiddleware::Impl::apply_cors_policy(const HttpRequest& request) {
    SecurityMiddlewareResult result;

    // Check CORS
    std::string origin = request.headers.at("Origin");

    if (!origin.empty()) {
        bool origin_allowed = false;

        for (const auto& allowed_origin : config_.allowed_origins) {
            if (origin == allowed_origin || allowed_origin == "*") {
                origin_allowed = true;
                break;
            }
        }

        if (!origin_allowed) {
            result.allowed = false;
            result.reason = "Origin not allowed by CORS policy";
            result.security_headers["X-CORS-Policy"] = "denied";
        } else {
            result.security_headers["Access-Control-Allow-Origin"] = origin;
            result.security_headers["Access-Control-Allow-Methods"] = "GET, POST, PUT, DELETE, OPTIONS";
            result.security_headers["Access-Control-Allow-Headers"] = "Content-Type, Authorization, X-Requested-With";
            result.security_headers["Access-Control-Max-Age"] = "86400";
        }
    }

    return result;
}

void SecurityMiddleware::Impl::add_security_headers(SecurityMiddlewareResult& result) {
    // Add standard security headers
    result.security_headers["X-Content-Type-Options"] = "nosniff";
    result.security_headers["X-Frame-Options"] = "DENY";
    result.security_headers["X-XSS-Protection"] = "1; mode=block";
    result.security_headers["Strict-Transport-Security"] = "max-age=31536000; includeSubDomains";
    result.security_headers["Content-Security-Policy"] = "default-src 'self'";
    result.security_headers["Referrer-Policy"] = "strict-origin-when-cross-origin";

    // Add request processing time
    result.security_headers["X-Processing-Time"] = std::to_string(result.processing_time.count()) + "us";
}

void SecurityMiddleware::Impl::log_security_event(const std::string& event_type, const std::string& details) {
    if (config_.enable_request_logging) {
        HFX_LOG_INFO("[LOG] Message");
    }
}

bool SecurityMiddleware::Impl::is_path_blocked(const std::string& path) const {
    for (const auto& blocked_path : config_.blocked_paths) {
        if (path.find(blocked_path) != std::string::npos) {
            return true;
        }
    }
    return false;
}

// SecurityMiddleware implementation
SecurityMiddleware::SecurityMiddleware(const SecurityMiddlewareConfig& config)
    : impl_(std::make_unique<Impl>(config)) {}

SecurityMiddleware::~SecurityMiddleware() = default;

SecurityMiddlewareResult SecurityMiddleware::process_request(const HttpRequest& request) {
    return impl_->process_request(request);
}

void SecurityMiddleware::update_config(const SecurityMiddlewareConfig& config) {
    impl_->update_config(config);
}

// Utility functions
std::string security_middleware_result_to_string(const SecurityMiddlewareResult& result) {
    std::stringstream ss;
    ss << "Allowed: " << (result.allowed ? "true" : "false");
    if (!result.allowed) {
        ss << ", Reason: " << result.reason;
    }
    ss << ", Processing Time: " << result.processing_time.count() << "us";
    return ss.str();
}

} // namespace security
} // namespace hfx
