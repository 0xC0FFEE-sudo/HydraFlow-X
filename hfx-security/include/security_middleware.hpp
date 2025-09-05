#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include <chrono>
#include <vector>
#include "rate_limiter.hpp"
#include "ddos_protection.hpp"
#include "api_security.hpp"

namespace hfx {
namespace security {

// Forward declarations
struct HttpRequest;
struct SecurityMiddlewareResult;
class SecurityMiddlewareConfig;
class RateLimiter;
class DDoSProtection;
class APISecurity;

// HTTP Request structure (simplified)
struct HttpRequest {
    std::string method;
    std::string path;
    std::string body;
    std::unordered_map<std::string, std::string> headers;
    std::unordered_map<std::string, std::string> query_params;
    std::string remote_addr;
    std::chrono::system_clock::time_point timestamp;

    HttpRequest() : timestamp(std::chrono::system_clock::now()) {}
};

// Security middleware configuration
struct SecurityMiddlewareConfig {
    RateLimitConfig rate_limit_config;
    DDoSProtectionConfig ddos_config;
    APISecurityConfig api_security_config;
    bool enable_request_logging;
    bool enable_metrics_collection;
    std::chrono::seconds request_timeout;
    std::vector<std::string> allowed_origins;
    std::vector<std::string> blocked_paths;

    SecurityMiddlewareConfig() : enable_request_logging(true),
                                enable_metrics_collection(true),
                                request_timeout(std::chrono::seconds(30)) {}
};

// Security middleware result
struct SecurityMiddlewareResult {
    bool allowed;
    std::string reason;
    std::vector<std::string> warnings;
    std::chrono::microseconds processing_time;
    std::unordered_map<std::string, std::string> security_headers;

    SecurityMiddlewareResult() : allowed(true), processing_time(0) {}

    SecurityMiddlewareResult& operator=(const SecurityMiddlewareResult& other) {
        if (this != &other) {
            allowed = other.allowed;
            reason = other.reason;
            warnings = other.warnings;
            processing_time = other.processing_time;
            security_headers = other.security_headers;
        }
        return *this;
    }
};

// Security middleware class
class SecurityMiddleware {
public:
    explicit SecurityMiddleware(const SecurityMiddlewareConfig& config);
    ~SecurityMiddleware();

    SecurityMiddlewareResult process_request(const HttpRequest& request);
    void update_config(const SecurityMiddlewareConfig& config);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

// Utility functions
std::string security_middleware_result_to_string(const SecurityMiddlewareResult& result);

} // namespace security
} // namespace hfx
