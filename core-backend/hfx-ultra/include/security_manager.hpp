#pragma once

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <atomic>
#include <mutex>
#include <thread>
#include <chrono>
#include <functional>
#include <queue>
#include <fstream>
#include <regex>

namespace hfx::ultra {

// Security levels for different operations
enum class SecurityLevel {
    PUBLIC = 0,        // No authentication required
    AUTHENTICATED = 1, // Basic authentication required
    AUTHORIZED = 2,    // Specific permissions required
    ADMIN = 3,         // Administrative privileges
    SYSTEM = 4         // System-level access only
};

// Authentication methods
enum class AuthMethod {
    API_KEY,           // API key authentication
    JWT_TOKEN,         // JSON Web Token
    OAUTH2,            // OAuth 2.0
    CERTIFICATE,       // Client certificate
    HARDWARE_TOKEN,    // Hardware security token
    BIOMETRIC,         // Biometric authentication
    MULTI_FACTOR       // Multi-factor authentication
};

// Audit event types
enum class AuditEventType {
    LOGIN_SUCCESS,
    LOGIN_FAILURE,
    LOGOUT,
    API_ACCESS,
    PERMISSION_DENIED,
    DATA_ACCESS,
    DATA_MODIFICATION,
    CONFIGURATION_CHANGE,
    TRADE_EXECUTION,
    FUND_TRANSFER,
    KEY_GENERATION,
    KEY_ACCESS,
    SYSTEM_COMMAND,
    SECURITY_VIOLATION,
    RATE_LIMIT_EXCEEDED,
    SUSPICIOUS_ACTIVITY
};

// Security violation severity
enum class ViolationSeverity {
    LOW = 0,
    MEDIUM = 1,
    HIGH = 2,
    CRITICAL = 3,
    EMERGENCY = 4
};

// Rate limiting strategies
enum class RateLimitStrategy {
    TOKEN_BUCKET,      // Token bucket algorithm
    SLIDING_WINDOW,    // Sliding window counter
    FIXED_WINDOW,      // Fixed window counter
    EXPONENTIAL_BACKOFF // Exponential backoff
};

// User session information
struct UserSession {
    std::string session_id;
    std::string user_id;
    std::string client_ip;
    std::string user_agent;
    AuthMethod auth_method;
    SecurityLevel clearance_level;
    std::chrono::system_clock::time_point created_at;
    std::chrono::system_clock::time_point last_access;
    std::chrono::system_clock::time_point expires_at;
    std::unordered_set<std::string> permissions;
    std::unordered_map<std::string, std::string> metadata;
    std::atomic<bool> active{true};
    std::atomic<uint32_t> request_count{0};
    
    // Copy constructor for atomic members
    UserSession(const UserSession& other) 
        : session_id(other.session_id), user_id(other.user_id), client_ip(other.client_ip),
          user_agent(other.user_agent), auth_method(other.auth_method), 
          clearance_level(other.clearance_level), created_at(other.created_at),
          last_access(other.last_access), expires_at(other.expires_at),
          permissions(other.permissions), metadata(other.metadata),
          active(other.active.load()), request_count(other.request_count.load()) {}
    
    UserSession& operator=(const UserSession& other) {
        if (this != &other) {
            session_id = other.session_id;
            user_id = other.user_id;
            client_ip = other.client_ip;
            user_agent = other.user_agent;
            auth_method = other.auth_method;
            clearance_level = other.clearance_level;
            created_at = other.created_at;
            last_access = other.last_access;
            expires_at = other.expires_at;
            permissions = other.permissions;
            metadata = other.metadata;
            active.store(other.active.load());
            request_count.store(other.request_count.load());
        }
        return *this;
    }
    
    UserSession() = default;
};

// API key information
struct APIKey {
    std::string key_id;
    std::string key_hash; // Hashed version of the actual key
    std::string user_id;
    std::string description;
    SecurityLevel max_level;
    std::unordered_set<std::string> permissions;
    std::chrono::system_clock::time_point created_at;
    std::chrono::system_clock::time_point expires_at;
    std::chrono::system_clock::time_point last_used;
    std::atomic<bool> active{true};
    std::atomic<uint64_t> usage_count{0};
    std::string source_ip_whitelist; // Comma-separated IP addresses/ranges
    
    // Rate limiting
    uint32_t requests_per_minute = 1000;
    uint32_t requests_per_hour = 10000;
    uint32_t requests_per_day = 100000;
    
    APIKey() = default;
    
    APIKey(const APIKey& other) 
        : key_id(other.key_id), key_hash(other.key_hash), user_id(other.user_id),
          description(other.description), max_level(other.max_level),
          permissions(other.permissions), created_at(other.created_at),
          expires_at(other.expires_at), last_used(other.last_used),
          active(other.active.load()), usage_count(other.usage_count.load()),
          source_ip_whitelist(other.source_ip_whitelist),
          requests_per_minute(other.requests_per_minute),
          requests_per_hour(other.requests_per_hour),
          requests_per_day(other.requests_per_day) {}
    
    APIKey(APIKey&& other) noexcept
        : key_id(std::move(other.key_id)), key_hash(std::move(other.key_hash)), 
          user_id(std::move(other.user_id)), description(std::move(other.description)),
          max_level(other.max_level), permissions(std::move(other.permissions)),
          created_at(other.created_at), expires_at(other.expires_at), last_used(other.last_used),
          active(other.active.load()), usage_count(other.usage_count.load()),
          source_ip_whitelist(std::move(other.source_ip_whitelist)),
          requests_per_minute(other.requests_per_minute),
          requests_per_hour(other.requests_per_hour),
          requests_per_day(other.requests_per_day) {}
    
    APIKey& operator=(APIKey&& other) noexcept {
        if (this != &other) {
            key_id = std::move(other.key_id);
            key_hash = std::move(other.key_hash);
            user_id = std::move(other.user_id);
            description = std::move(other.description);
            max_level = other.max_level;
            permissions = std::move(other.permissions);
            created_at = other.created_at;
            expires_at = other.expires_at;
            last_used = other.last_used;
            active.store(other.active.load());
            usage_count.store(other.usage_count.load());
            source_ip_whitelist = std::move(other.source_ip_whitelist);
            requests_per_minute = other.requests_per_minute;
            requests_per_hour = other.requests_per_hour;
            requests_per_day = other.requests_per_day;
        }
        return *this;
    }
    
    // Delete copy assignment operator
    APIKey& operator=(const APIKey&) = delete;
};

// Audit log entry
struct AuditLogEntry {
    std::string entry_id;
    std::chrono::system_clock::time_point timestamp;
    AuditEventType event_type;
    ViolationSeverity severity;
    std::string user_id;
    std::string session_id;
    std::string client_ip;
    std::string resource;
    std::string action;
    std::string details;
    std::unordered_map<std::string, std::string> metadata;
    bool success;
    std::string error_message;
};

// Rate limiting bucket
struct RateLimitBucket {
    uint32_t max_tokens;
    std::atomic<uint32_t> current_tokens;
    std::chrono::steady_clock::time_point last_refill;
    std::chrono::milliseconds refill_interval;
    std::mutex bucket_mutex;
    
    RateLimitBucket(uint32_t max, std::chrono::milliseconds interval)
        : max_tokens(max), current_tokens(max), 
          last_refill(std::chrono::steady_clock::now()),
          refill_interval(interval) {}
};

// Security configuration
struct SecurityConfig {
    // Session management
    std::chrono::minutes session_timeout{30};
    std::chrono::hours max_session_duration{24};
    bool require_session_renewal = true;
    uint32_t max_concurrent_sessions_per_user = 5;
    
    // Authentication
    bool require_strong_passwords = true;
    uint32_t min_password_length = 12;
    bool require_mfa = false;
    std::chrono::minutes mfa_timeout{5};
    
    // API key management
    std::chrono::days api_key_expiry{90};
    bool auto_rotate_keys = true;
    std::chrono::days key_rotation_interval{30};
    
    // Rate limiting
    bool enable_rate_limiting = true;
    RateLimitStrategy rate_limit_strategy = RateLimitStrategy::TOKEN_BUCKET;
    uint32_t global_requests_per_second = 1000;
    uint32_t per_user_requests_per_minute = 100;
    uint32_t per_api_key_requests_per_minute = 1000;
    
    // IP filtering
    bool enable_ip_whitelist = false;
    std::vector<std::string> allowed_ip_ranges;
    std::vector<std::string> blocked_ip_ranges;
    
    // Audit logging
    bool enable_audit_logging = true;
    std::string audit_log_path = "/var/log/hydraflow/audit.log";
    std::chrono::days audit_retention_days{365};
    bool encrypt_audit_logs = true;
    
    // Security headers
    bool enforce_https = true;
    bool enable_cors = false;
    std::vector<std::string> allowed_origins;
    
    // Encryption
    std::string encryption_algorithm = "AES-256-GCM";
    bool encrypt_sensitive_data = true;
    bool use_hsm_for_keys = false;
    std::string key_derivation_function = "PBKDF2";
    uint32_t key_derivation_iterations = 100000;
    
    // Monitoring and alerting
    bool enable_security_monitoring = true;
    uint32_t failed_login_threshold = 5;
    std::chrono::minutes failed_login_window{15};
    uint32_t suspicious_activity_threshold = 10;
    
    // File access
    bool restrict_file_access = true;
    std::vector<std::string> allowed_file_extensions;
    uint64_t max_file_size_bytes = 10 * 1024 * 1024; // 10MB
};

// Security violation information
struct SecurityViolation {
    std::string violation_id;
    std::chrono::system_clock::time_point timestamp;
    ViolationSeverity severity;
    std::string violation_type;
    std::string user_id;
    std::string client_ip;
    std::string description;
    std::unordered_map<std::string, std::string> details;
    bool resolved = false;
    std::string resolution_action;
    std::chrono::system_clock::time_point resolved_at;
};

// Permission system
class PermissionManager {
public:
    // Permission operations
    bool grant_permission(const std::string& user_id, const std::string& permission);
    bool revoke_permission(const std::string& user_id, const std::string& permission);
    bool has_permission(const std::string& user_id, const std::string& permission) const;
    std::unordered_set<std::string> get_user_permissions(const std::string& user_id) const;
    
    // Role-based access control
    bool create_role(const std::string& role_name, const std::unordered_set<std::string>& permissions);
    bool assign_role(const std::string& user_id, const std::string& role_name);
    bool remove_role(const std::string& user_id, const std::string& role_name);
    std::unordered_set<std::string> get_user_roles(const std::string& user_id) const;
    
    // Resource access control
    bool can_access_resource(const std::string& user_id, const std::string& resource, 
                           const std::string& action) const;
    
private:
    mutable std::mutex permissions_mutex_;
    std::unordered_map<std::string, std::unordered_set<std::string>> user_permissions_;
    std::unordered_map<std::string, std::unordered_set<std::string>> user_roles_;
    std::unordered_map<std::string, std::unordered_set<std::string>> role_permissions_;
};

// Main security manager
class SecurityManager {
public:
    explicit SecurityManager(const SecurityConfig& config = SecurityConfig{});
    ~SecurityManager();
    
    // Lifecycle management
    bool initialize();
    bool start();
    bool stop();
    bool is_running() const { return running_.load(); }
    
    // Session management
    std::string create_session(const std::string& user_id, const std::string& client_ip,
                              const std::string& user_agent, AuthMethod auth_method,
                              SecurityLevel clearance_level = SecurityLevel::AUTHENTICATED);
    bool validate_session(const std::string& session_id);
    bool extend_session(const std::string& session_id);
    bool terminate_session(const std::string& session_id);
    void cleanup_expired_sessions();
    UserSession get_session(const std::string& session_id) const;
    std::vector<UserSession> get_user_sessions(const std::string& user_id) const;
    
    // API key management
    std::string create_api_key(const std::string& user_id, const std::string& description,
                              SecurityLevel max_level, 
                              const std::unordered_set<std::string>& permissions,
                              std::chrono::system_clock::time_point expires_at);
    bool validate_api_key(const std::string& api_key, const std::string& client_ip = "");
    bool revoke_api_key(const std::string& key_id);
    void rotate_api_keys();
    std::vector<APIKey> get_user_api_keys(const std::string& user_id) const;
    
    // Authentication
    bool authenticate_user(const std::string& user_id, const std::string& credential,
                          AuthMethod method = AuthMethod::API_KEY);
    bool verify_multi_factor(const std::string& user_id, const std::string& mfa_code);
    
    // Authorization
    bool authorize_action(const std::string& session_id, const std::string& resource,
                         const std::string& action, SecurityLevel required_level);
    bool check_permission(const std::string& user_id, const std::string& permission) const;
    
    // Rate limiting
    bool check_rate_limit(const std::string& identifier, const std::string& endpoint);
    void reset_rate_limit(const std::string& identifier);
    uint32_t get_remaining_requests(const std::string& identifier) const;
    
    // Input validation and sanitization
    bool validate_input(const std::string& input, const std::string& pattern);
    std::string sanitize_input(const std::string& input);
    bool is_safe_filename(const std::string& filename) const;
    bool is_valid_email(const std::string& email) const;
    bool is_valid_ip_address(const std::string& ip) const;
    
    // IP filtering
    bool is_ip_allowed(const std::string& ip_address) const;
    void add_ip_to_whitelist(const std::string& ip_range);
    void add_ip_to_blacklist(const std::string& ip_range);
    void remove_ip_from_whitelist(const std::string& ip_range);
    void remove_ip_from_blacklist(const std::string& ip_range);
    
    // Audit logging
    void log_audit_event(AuditEventType event_type, const std::string& user_id,
                        const std::string& resource, const std::string& action,
                        bool success, const std::string& details = "",
                        ViolationSeverity severity = ViolationSeverity::LOW);
    std::vector<AuditLogEntry> get_audit_logs(std::chrono::system_clock::time_point start,
                                             std::chrono::system_clock::time_point end) const;
    void export_audit_logs(const std::string& filepath,
                          std::chrono::system_clock::time_point start,
                          std::chrono::system_clock::time_point end) const;
    
    // Security violation tracking
    void report_violation(ViolationSeverity severity, const std::string& violation_type,
                         const std::string& user_id, const std::string& client_ip,
                         const std::string& description,
                         const std::unordered_map<std::string, std::string>& details = {});
    std::vector<SecurityViolation> get_violations(ViolationSeverity min_severity = ViolationSeverity::LOW) const;
    void resolve_violation(const std::string& violation_id, const std::string& resolution_action);
    
    // Encryption and hashing
    std::string hash_password(const std::string& password, const std::string& salt = "");
    bool verify_password(const std::string& password, const std::string& hash);
    std::string encrypt_data(const std::string& data, const std::string& key = "");
    std::string decrypt_data(const std::string& encrypted_data, const std::string& key = "");
    std::string generate_secure_token(size_t length = 32);
    std::string generate_salt(size_t length = 16);
    
    // Security headers for HTTP responses
    std::unordered_map<std::string, std::string> get_security_headers() const;
    
    // Permission management
    PermissionManager& get_permission_manager() { return permission_manager_; }
    const PermissionManager& get_permission_manager() const { return permission_manager_; }
    
    // Configuration management
    void update_config(const SecurityConfig& config);
    const SecurityConfig& get_config() const { return config_; }
    
    // Security statistics
    struct SecurityStats {
        std::atomic<uint64_t> total_sessions_created{0};
        std::atomic<uint64_t> active_sessions{0};
        std::atomic<uint64_t> failed_authentications{0};
        std::atomic<uint64_t> successful_authentications{0};
        std::atomic<uint64_t> rate_limit_violations{0};
        std::atomic<uint64_t> security_violations{0};
        std::atomic<uint64_t> audit_log_entries{0};
        std::atomic<uint64_t> api_keys_created{0};
        std::atomic<uint64_t> api_keys_revoked{0};
        std::atomic<double> avg_session_duration_minutes{0.0};
    };
    
    const SecurityStats& get_stats() const { return stats_; }
    void reset_stats();
    
    // Trading-specific security helpers
    bool authorize_trade(const std::string& session_id, const std::string& symbol, 
                        double amount, const std::string& side);
    bool authorize_withdrawal(const std::string& session_id, double amount, 
                             const std::string& destination);
    bool authorize_api_access(const std::string& api_key, const std::string& endpoint,
                             const std::string& client_ip);
    void log_trade_execution(const std::string& user_id, const std::string& symbol,
                           double amount, const std::string& side, bool success);
    void log_fund_movement(const std::string& user_id, double amount,
                          const std::string& source, const std::string& destination, bool success);
    
    // Emergency security procedures
    void trigger_security_lockdown(const std::string& reason);
    void emergency_revoke_all_sessions();
    void emergency_disable_api_access();
    bool is_in_lockdown() const { return security_lockdown_.load(); }
    void lift_security_lockdown();
    
private:
    SecurityConfig config_;
    std::atomic<bool> running_{false};
    std::atomic<bool> shutdown_requested_{false};
    std::atomic<bool> security_lockdown_{false};
    mutable SecurityStats stats_;
    
    // Core storage
    mutable std::mutex sessions_mutex_;
    std::unordered_map<std::string, UserSession> active_sessions_;
    
    mutable std::mutex api_keys_mutex_;
    std::unordered_map<std::string, APIKey> api_keys_;
    std::unordered_map<std::string, std::string> key_hash_to_id_; // For quick lookup
    
    mutable std::mutex audit_logs_mutex_;
    std::queue<AuditLogEntry> audit_log_queue_;
    std::vector<AuditLogEntry> audit_logs_;
    
    mutable std::mutex violations_mutex_;
    std::vector<SecurityViolation> security_violations_;
    
    mutable std::mutex rate_limits_mutex_;
    std::unordered_map<std::string, std::unique_ptr<RateLimitBucket>> rate_limit_buckets_;
    
    // Permission management
    PermissionManager permission_manager_;
    
    // Worker threads
    std::thread session_cleanup_worker_;
    std::thread audit_log_writer_;
    std::thread rate_limit_refill_worker_;
    std::thread security_monitor_;
    
    // Internal methods
    void session_cleanup_worker();
    void audit_log_writer_worker();
    void rate_limit_refill_worker();
    void security_monitor_worker();
    
    std::string generate_session_id();
    std::string generate_api_key();
    std::string generate_violation_id();
    std::string generate_audit_entry_id();
    
    bool is_session_expired(const UserSession& session) const;
    bool is_api_key_expired(const APIKey& key) const;
    bool is_ip_in_range(const std::string& ip, const std::string& range) const;
    
    void write_audit_log_to_file(const AuditLogEntry& entry);
    void detect_suspicious_activity();
    void check_failed_login_attempts();
    
    // Crypto helpers
    std::string compute_hash(const std::string& data, const std::string& salt = "");
    std::string generate_random_string(size_t length);
    
    // File operations with security
    bool is_safe_file_operation(const std::string& filepath) const;
    void secure_delete_file(const std::string& filepath);
};

// Factory for creating security managers with different configurations
class SecurityManagerFactory {
public:
    // Pre-configured security managers
    static std::unique_ptr<SecurityManager> create_development_security();
    static std::unique_ptr<SecurityManager> create_production_security();
    static std::unique_ptr<SecurityManager> create_high_security();
    static std::unique_ptr<SecurityManager> create_trading_security();
    
    // Custom configurations
    static std::unique_ptr<SecurityManager> create_with_config(const SecurityConfig& config);
};

// Utility functions for security operations
namespace security_utils {
    std::string severity_to_string(ViolationSeverity severity);
    ViolationSeverity string_to_severity(const std::string& severity_str);
    
    std::string auth_method_to_string(AuthMethod method);
    AuthMethod string_to_auth_method(const std::string& method_str);
    
    std::string security_level_to_string(SecurityLevel level);
    SecurityLevel string_to_security_level(const std::string& level_str);
    
    std::string event_type_to_string(AuditEventType event_type);
    AuditEventType string_to_event_type(const std::string& event_str);
    
    // Validation helpers
    bool is_strong_password(const std::string& password);
    bool is_valid_session_id(const std::string& session_id);
    bool is_valid_api_key_format(const std::string& api_key);
    
    // Security analysis
    double calculate_password_strength(const std::string& password);
    std::vector<std::string> analyze_security_threats(const std::vector<AuditLogEntry>& logs);
    double calculate_risk_score(const std::vector<SecurityViolation>& violations);
    
    // Formatting utilities
    std::string format_session_info(const UserSession& session);
    std::string format_audit_entry(const AuditLogEntry& entry);
    std::string format_violation(const SecurityViolation& violation);
}

} // namespace hfx::ultra
