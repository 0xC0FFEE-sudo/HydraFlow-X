#pragma once

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <atomic>
#include <mutex>
#include <memory>
#include <chrono>
#include <vector>
#include <regex>
#include <functional>

namespace hfx {
namespace security {

// API security event types
enum class APISecurityEvent {
    INVALID_REQUEST = 0,
    MALFORMED_JSON,
    SQL_INJECTION_ATTEMPT,
    XSS_ATTEMPT,
    CSRF_ATTEMPT,
    AUTHORIZATION_FAILURE,
    RATE_LIMIT_EXCEEDED,
    SUSPICIOUS_PAYLOAD,
    MALICIOUS_FILE_UPLOAD,
    UNEXPECTED_ENDPOINT_ACCESS,
    SENSITIVE_DATA_EXPOSURE,
    BRUTE_FORCE_ATTEMPT,
    UNKNOWN
};

// Security validation result
struct SecurityValidationResult {
    bool valid;
    APISecurityEvent event_type;
    std::string description;
    double severity_score;
    std::vector<std::string> recommendations;

    SecurityValidationResult() : valid(true),
                                event_type(APISecurityEvent::UNKNOWN),
                                severity_score(0.0) {}
};

// Input validation rule
struct InputValidationRule {
    std::string name;
    std::string field_pattern;
    std::regex validation_regex;
    std::string error_message;
    double severity_score;
    bool block_on_failure;
    bool log_on_failure;

    InputValidationRule() : severity_score(0.5),
                           block_on_failure(true),
                           log_on_failure(true) {}
};

// API endpoint security configuration
struct APIEndpointSecurity {
    std::string endpoint_pattern;
    std::vector<std::string> allowed_methods;
    std::vector<std::string> required_permissions;
    bool requires_authentication;
    bool enable_input_validation;
    bool enable_sql_injection_protection;
    bool enable_xss_protection;
    bool enable_csrf_protection;
    int max_request_size_kb;
    int max_payload_depth;
    std::vector<InputValidationRule> validation_rules;

    APIEndpointSecurity() : requires_authentication(true),
                           enable_input_validation(true),
                           enable_sql_injection_protection(true),
                           enable_xss_protection(true),
                           enable_csrf_protection(true),
                           max_request_size_kb(1024),
                           max_payload_depth(10) {}
};

// API security configuration
struct APISecurityConfig {
    bool enabled;
    bool enable_input_validation;
    bool enable_sql_injection_protection;
    bool enable_xss_protection;
    bool enable_csrf_protection;
    bool enable_sensitive_data_masking;
    bool enable_request_logging;
    int max_request_size_kb;
    int max_payload_depth;
    std::chrono::seconds token_expiration_buffer;
    std::vector<std::string> sensitive_headers;
    std::vector<std::string> sensitive_fields;
    std::unordered_map<std::string, APIEndpointSecurity> endpoint_security;

    APISecurityConfig() : enabled(true),
                         enable_input_validation(true),
                         enable_sql_injection_protection(true),
                         enable_xss_protection(true),
                         enable_csrf_protection(true),
                         enable_sensitive_data_masking(true),
                         enable_request_logging(true),
                         max_request_size_kb(1024),
                         max_payload_depth(10),
                         token_expiration_buffer(std::chrono::seconds(300)) {}
};

// SQL injection patterns
class SQLInjectionDetector {
public:
    SQLInjectionDetector();
    bool detect_sql_injection(const std::string& input) const;
    double calculate_injection_score(const std::string& input) const;

private:
    std::vector<std::regex> sql_patterns_;
    std::vector<std::string> sql_keywords_;
    void initialize_patterns();
};

// XSS patterns
class XSSDetector {
public:
    XSSDetector();
    bool detect_xss(const std::string& input) const;
    std::string sanitize_input(const std::string& input) const;

private:
    std::vector<std::regex> xss_patterns_;
    std::unordered_map<std::string, std::string> sanitization_rules_;
    void initialize_patterns();
};

// CSRF protection
class CSRFProtector {
public:
    explicit CSRFProtector(const std::string& secret_key);
    std::string generate_token(const std::string& session_id) const;
    bool validate_token(const std::string& token, const std::string& session_id) const;

private:
    std::string secret_key_;
    std::string generate_hmac(const std::string& data) const;
};

// Sensitive data masking
class DataMasker {
public:
    explicit DataMasker(const std::vector<std::string>& sensitive_fields);
    std::string mask_sensitive_data(const std::string& json_data) const;
    std::string mask_field_value(const std::string& field_name, const std::string& value) const;

private:
    std::unordered_set<std::string> sensitive_fields_;
    std::unordered_map<std::string, std::function<std::string(const std::string&)>> masking_functions_;

    std::string mask_credit_card(const std::string& value) const;
    std::string mask_ssn(const std::string& value) const;
    std::string mask_email(const std::string& value) const;
    std::string mask_phone(const std::string& value) const;
    std::string mask_api_key(const std::string& value) const;
};

// API security middleware
class APISecurity {
public:
    explicit APISecurity(const APISecurityConfig& config);
    ~APISecurity();

    // Request validation
    SecurityValidationResult validate_request(const std::string& method,
                                            const std::string& endpoint,
                                            const std::string& body,
                                            const std::unordered_map<std::string, std::string>& headers,
                                            const std::string& query_params = "");

    // Input sanitization
    std::string sanitize_input(const std::string& input, const std::string& field_name = "") const;
    std::string sanitize_json_payload(const std::string& json_payload) const;

    // Security checks
    bool is_sql_injection_attempt(const std::string& input) const;
    bool is_xss_attempt(const std::string& input) const;
    bool validate_json_structure(const std::string& json_data) const;

    // CSRF protection
    std::string generate_csrf_token(const std::string& session_id) const;
    bool validate_csrf_token(const std::string& token, const std::string& session_id) const;

    // Data masking
    std::string mask_response_data(const std::string& response_data) const;

    // Endpoint security
    void configure_endpoint_security(const std::string& endpoint, const APIEndpointSecurity& security);
    const APIEndpointSecurity* get_endpoint_security(const std::string& endpoint) const;

    // Statistics and monitoring
    struct APISecurityStats {
        std::atomic<uint64_t> total_requests_validated{0};
        std::atomic<uint64_t> validation_failures{0};
        std::atomic<uint64_t> sql_injection_attempts{0};
        std::atomic<uint64_t> xss_attempts{0};
        std::atomic<uint64_t> csrf_violations{0};
        std::atomic<uint64_t> blocked_requests{0};
        std::atomic<uint64_t> sanitized_inputs{0};
        std::atomic<uint64_t> masked_responses{0};
        std::chrono::system_clock::time_point last_security_event;
    };

    APISecurityStats get_stats() const;
    void reset_stats();

    // Configuration
    void update_config(const APISecurityConfig& config);
    APISecurityConfig get_config() const;

    // Security event logging
    void log_security_event(APISecurityEvent event_type,
                           const std::string& description,
                           double severity_score,
                           const std::unordered_map<std::string, std::string>& metadata = {});

private:
    APISecurityConfig config_;
    mutable std::mutex config_mutex_;

    // Security detectors
    std::unique_ptr<SQLInjectionDetector> sql_detector_;
    std::unique_ptr<XSSDetector> xss_detector_;
    std::unique_ptr<CSRFProtector> csrf_protector_;
    std::unique_ptr<DataMasker> data_masker_;

    // Endpoint security configurations
    std::unordered_map<std::string, APIEndpointSecurity> endpoint_security_;

    // Statistics
    mutable APISecurityStats stats_;

    // Mutexes
    mutable std::mutex endpoints_mutex_;

    // Internal methods
    SecurityValidationResult validate_method_and_endpoint(const std::string& method,
                                                        const std::string& endpoint) const;
    SecurityValidationResult validate_request_body(const std::string& body,
                                                 const APIEndpointSecurity& endpoint_security) const;
    SecurityValidationResult validate_headers(const std::unordered_map<std::string, std::string>& headers) const;
    SecurityValidationResult validate_query_params(const std::string& query_params) const;

    bool is_valid_json(const std::string& json_str) const;
    int calculate_json_depth(const std::string& json_str) const;
    bool contains_suspicious_patterns(const std::string& input) const;

    const APIEndpointSecurity& get_default_endpoint_security() const;
    void apply_validation_rules(const std::string& input,
                               const std::vector<InputValidationRule>& rules,
                               SecurityValidationResult& result) const;

    void update_stats(const SecurityValidationResult& result);
};

// Utility functions
std::string api_security_event_to_string(APISecurityEvent event);
APISecurityEvent string_to_api_security_event(const std::string& event_str);
bool is_valid_json_structure(const std::string& json_str);
std::string escape_html_entities(const std::string& input);
std::string url_decode(const std::string& input);
std::string base64_decode(const std::string& input);

// Common validation patterns
namespace patterns {
    const std::regex EMAIL_PATTERN = std::regex(R"(^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,}$)");
    const std::regex UUID_PATTERN = std::regex(R"(^[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}$)");
    const std::regex ALPHANUMERIC_PATTERN = std::regex(R"(^[a-zA-Z0-9]+$)");
    const std::regex NUMERIC_PATTERN = std::regex(R"(^[0-9]+(\.[0-9]+)?$)");
    const std::regex BASE64_PATTERN = std::regex(R"(^[A-Za-z0-9+/]*={0,2}$)");
}

} // namespace security
} // namespace hfx
