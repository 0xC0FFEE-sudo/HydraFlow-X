#pragma once

#include <string>
#include <vector>
#include <map>
#include <chrono>
#include <memory>
#include <mutex>
#include <atomic>

namespace hfx {
namespace security {

// Security levels for different operations
enum class SecurityLevel {
    LOW,      // Basic validation
    MEDIUM,   // Standard security checks
    HIGH,     // Enhanced security with rate limiting
    CRITICAL  // Maximum security with audit logging
};

// API key encryption and management
class APIKeyManager {
public:
    struct KeyInfo {
        std::string encrypted_key;
        std::string provider;
        std::chrono::system_clock::time_point created_at;
        std::chrono::system_clock::time_point expires_at;
        bool is_active;
        SecurityLevel security_level;
    };

    // Key management
    bool store_api_key(const std::string& provider, const std::string& key, 
                      SecurityLevel level = SecurityLevel::HIGH);
    std::string get_api_key(const std::string& provider);
    bool rotate_api_key(const std::string& provider, const std::string& new_key);
    bool revoke_api_key(const std::string& provider);
    
    // Key validation
    bool validate_key_format(const std::string& provider, const std::string& key);
    bool is_key_expired(const std::string& provider);
    
    // Audit and monitoring
    std::vector<std::string> get_audit_log();
    void enable_key_rotation_alerts(bool enable);

private:
    std::map<std::string, KeyInfo> keys_;
    std::mutex keys_mutex_;
    std::vector<std::string> audit_log_;
    std::mutex audit_mutex_;
    
    // Encryption helpers
    std::string encrypt_key(const std::string& key);
    std::string decrypt_key(const std::string& encrypted_key);
    void log_key_operation(const std::string& operation, const std::string& provider);
};

// Rate limiting for API requests
class RateLimiter {
public:
    struct LimitConfig {
        size_t max_requests_per_second = 100;
        size_t max_requests_per_minute = 1000;
        size_t max_requests_per_hour = 10000;
        size_t burst_capacity = 50;
        std::chrono::milliseconds cooldown_period{1000};
    };

    explicit RateLimiter(const LimitConfig& config);
    
    // Rate limiting checks
    bool is_allowed(const std::string& client_id);
    bool is_allowed_for_endpoint(const std::string& client_id, const std::string& endpoint);
    
    // Monitoring
    size_t get_request_count(const std::string& client_id);
    std::chrono::milliseconds get_time_until_reset(const std::string& client_id);
    
    // Configuration
    void update_limits(const std::string& client_id, const LimitConfig& config);
    void reset_client_limits(const std::string& client_id);

private:
    struct ClientState {
        std::atomic<size_t> requests_this_second{0};
        std::atomic<size_t> requests_this_minute{0};
        std::atomic<size_t> requests_this_hour{0};
        std::chrono::steady_clock::time_point last_request;
        std::chrono::steady_clock::time_point window_start;
    };

    LimitConfig default_config_;
    std::map<std::string, ClientState> client_states_;
    std::map<std::string, LimitConfig> client_configs_;
    std::mutex state_mutex_;
    
    void cleanup_old_entries();
};

// Input validation and sanitization
class InputValidator {
public:
    // Trading-specific validation
    static bool validate_token_symbol(const std::string& symbol);
    static bool validate_wallet_address(const std::string& address, const std::string& chain);
    static bool validate_amount(double amount, double min_amount = 0.0, double max_amount = 1e18);
    static bool validate_slippage(double slippage_percent);
    static bool validate_api_endpoint(const std::string& url);
    
    // General validation
    static bool validate_email(const std::string& email);
    static bool validate_json(const std::string& json_str);
    static bool validate_hex_string(const std::string& hex);
    static bool validate_base64(const std::string& base64);
    
    // Sanitization
    static std::string sanitize_string(const std::string& input, size_t max_length = 1000);
    static std::string sanitize_sql_input(const std::string& input);
    static std::string sanitize_log_message(const std::string& message);
    
    // XSS and injection protection
    static std::string escape_html(const std::string& input);
    static std::string escape_javascript(const std::string& input);
    static bool contains_sql_injection_pattern(const std::string& input);
};

// Authentication and authorization
class AuthManager {
public:
    struct UserSession {
        std::string session_id;
        std::string user_id;
        SecurityLevel access_level;
        std::chrono::system_clock::time_point created_at;
        std::chrono::system_clock::time_point expires_at;
        std::string ip_address;
        bool is_active;
    };

    // Session management
    std::string create_session(const std::string& user_id, SecurityLevel level,
                             const std::string& ip_address);
    bool validate_session(const std::string& session_id);
    bool refresh_session(const std::string& session_id);
    void terminate_session(const std::string& session_id);
    void terminate_all_sessions(const std::string& user_id);
    
    // Permission checking
    bool has_permission(const std::string& session_id, const std::string& resource,
                       const std::string& action);
    bool can_access_trading_functions(const std::string& session_id);
    bool can_modify_configuration(const std::string& session_id);
    
    // Security monitoring
    void log_login_attempt(const std::string& user_id, const std::string& ip_address, bool success);
    std::vector<std::string> get_failed_login_attempts(const std::string& ip_address);
    bool is_ip_blocked(const std::string& ip_address);

private:
    std::map<std::string, UserSession> sessions_;
    std::mutex sessions_mutex_;
    std::map<std::string, std::vector<std::chrono::system_clock::time_point>> failed_attempts_;
    std::mutex attempts_mutex_;
    
    std::string generate_session_id();
    void cleanup_expired_sessions();
};

// Audit logging system
class AuditLogger {
public:
    enum class EventType {
        LOGIN,
        LOGOUT,
        TRADING_ACTION,
        CONFIG_CHANGE,
        API_CALL,
        SECURITY_VIOLATION,
        SYSTEM_ERROR
    };

    struct AuditEvent {
        EventType type;
        std::string user_id;
        std::string session_id;
        std::string action;
        std::string resource;
        std::string details;
        std::string ip_address;
        std::chrono::system_clock::time_point timestamp;
        SecurityLevel severity;
    };

    // Logging methods
    void log_event(const AuditEvent& event);
    void log_login(const std::string& user_id, const std::string& ip_address, bool success);
    void log_trading_action(const std::string& user_id, const std::string& action,
                          const std::string& details);
    void log_config_change(const std::string& user_id, const std::string& config_key,
                         const std::string& old_value, const std::string& new_value);
    void log_security_violation(const std::string& details, const std::string& ip_address);
    
    // Query methods
    std::vector<AuditEvent> get_events_by_user(const std::string& user_id,
                                              std::chrono::hours lookback = std::chrono::hours(24));
    std::vector<AuditEvent> get_events_by_type(EventType type,
                                              std::chrono::hours lookback = std::chrono::hours(24));
    std::vector<AuditEvent> get_security_violations(std::chrono::hours lookback = std::chrono::hours(24));
    
    // Export and archival
    bool export_to_file(const std::string& filename, std::chrono::hours lookback);
    void archive_old_events(std::chrono::hours max_age = std::chrono::hours(24 * 30)); // 30 days

private:
    std::vector<AuditEvent> events_;
    std::mutex events_mutex_;
    std::atomic<size_t> max_events_{100000};
    
    void ensure_space_for_new_event();
    std::string serialize_event(const AuditEvent& event);
};

// Network security
class NetworkSecurity {
public:
    // IP filtering
    bool is_ip_allowed(const std::string& ip_address);
    void add_allowed_ip(const std::string& ip_address);
    void remove_allowed_ip(const std::string& ip_address);
    void add_blocked_ip(const std::string& ip_address);
    void remove_blocked_ip(const std::string& ip_address);
    
    // Rate limiting by IP
    bool is_ip_rate_limited(const std::string& ip_address);
    void apply_rate_limit_to_ip(const std::string& ip_address, std::chrono::seconds duration);
    
    // DDoS protection
    bool detect_ddos_pattern(const std::string& ip_address);
    void enable_ddos_protection(bool enable);
    
    // SSL/TLS configuration
    struct TLSConfig {
        std::string cert_file;
        std::string key_file;
        std::string ca_file;
        std::vector<std::string> cipher_suites;
        bool require_client_cert;
        bool verify_hostname;
    };
    
    bool configure_tls(const TLSConfig& config);
    bool is_tls_enabled();

private:
    std::vector<std::string> allowed_ips_;
    std::vector<std::string> blocked_ips_;
    std::map<std::string, std::chrono::steady_clock::time_point> rate_limited_ips_;
    std::mutex network_mutex_;
    bool ddos_protection_enabled_{true};
    TLSConfig tls_config_;
};

// Main security configuration
class SecurityConfig {
public:
    struct Config {
        // General security settings
        SecurityLevel default_security_level = SecurityLevel::HIGH;
        bool enable_audit_logging = true;
        bool enable_rate_limiting = true;
        bool enable_input_validation = true;
        bool enable_session_management = true;
        
        // Crypto settings
        bool require_key_encryption = true;
        std::chrono::minutes key_rotation_interval{60};
        std::chrono::minutes session_timeout{30};
        
        // Network security
        bool enable_ip_filtering = false;
        bool enable_ddos_protection = true;
        bool require_tls = false;
        
        // Trading security
        double max_single_trade_amount = 10000.0;
        double max_daily_trade_volume = 100000.0;
        size_t max_concurrent_positions = 50;
        bool require_trade_confirmation = false;
        
        // API security
        size_t max_api_requests_per_second = 100;
        bool enable_api_key_rotation = true;
        std::chrono::hours api_key_max_age{24 * 7}; // 1 week
    };

    explicit SecurityConfig(const Config& config = Config{});
    
    // Configuration management
    void update_config(const Config& new_config);
    Config get_config() const;
    
    // Component access
    APIKeyManager& get_key_manager() { return key_manager_; }
    RateLimiter& get_rate_limiter() { return rate_limiter_; }
    AuthManager& get_auth_manager() { return auth_manager_; }
    AuditLogger& get_audit_logger() { return audit_logger_; }
    NetworkSecurity& get_network_security() { return network_security_; }
    
    // Validation helpers
    bool validate_trading_request(const std::string& session_id, double amount,
                                const std::string& symbol);
    bool validate_config_change(const std::string& session_id, const std::string& key,
                              const std::string& value);
    
    // Security status
    bool perform_security_check();
    std::string get_security_status_report();

private:
    Config config_;
    mutable std::mutex config_mutex_;
    
    APIKeyManager key_manager_;
    RateLimiter rate_limiter_;
    AuthManager auth_manager_;
    AuditLogger audit_logger_;
    NetworkSecurity network_security_;
};

} // namespace security
} // namespace hfx
