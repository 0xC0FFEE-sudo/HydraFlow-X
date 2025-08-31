# Security & Authentication APIs

Enterprise-grade security with multi-factor authentication, authorization, and comprehensive audit logging.

---

## Security Manager (`hfx::ultra::SecurityManager`)

Comprehensive security management system for authentication, authorization, and audit logging.

### Class Interface

```cpp
#include "hfx-ultra/include/security_manager.hpp"

namespace hfx::ultra {

class SecurityManager {
public:
    explicit SecurityManager(const SecurityConfig& config = SecurityConfig{});
    ~SecurityManager();
    
    // Lifecycle Management
    bool initialize();
    bool start();
    void stop();
    
    // User Authentication
    std::string authenticate_user(const std::string& username, 
                                 const std::string& password,
                                 const std::string& mfa_token = "");
    
    bool validate_session(const std::string& session_id) const;
    bool extend_session(const std::string& session_id);
    bool refresh_session(const std::string& session_id);
    void revoke_session(const std::string& session_id);
    
    // Session Management
    std::string create_session(const std::string& user_id,
                              const std::string& client_ip,
                              const std::string& user_agent,
                              AuthMethod auth_method,
                              SecurityLevel security_level);
    
    UserSession get_session_info(const std::string& session_id) const;
    std::vector<UserSession> get_active_sessions(const std::string& user_id) const;
    void terminate_all_sessions(const std::string& user_id);
    
    // API Key Management
    std::string create_api_key(const std::string& user_id,
                              const std::string& description,
                              SecurityLevel security_level,
                              const std::unordered_set<std::string>& permissions,
                              std::chrono::system_clock::time_point expires_at);
    
    bool validate_api_key(const std::string& api_key, 
                         const std::string& client_ip = "") const;
    void revoke_api_key(const std::string& api_key);
    std::vector<APIKey> get_user_api_keys(const std::string& user_id) const;
    
    // Authorization
    bool authorize_action(const std::string& session_id,
                         const std::string& resource,
                         const std::string& action,
                         SecurityLevel required_level) const;
    
    bool authorize_trade(const std::string& session_id,
                        const std::string& symbol,
                        double amount,
                        const std::string& side) const;
    
    bool authorize_withdrawal(const std::string& session_id,
                             double amount,
                             const std::string& destination) const;
    
    // Rate Limiting
    bool check_rate_limit(const std::string& identifier, 
                         const std::string& endpoint) const;
    void reset_rate_limit(const std::string& identifier);
    RateLimitStatus get_rate_limit_status(const std::string& identifier) const;
    
    // Security Monitoring
    void log_audit_event(AuditEventType event_type,
                        const std::string& user_id,
                        const std::string& resource,
                        const std::string& action,
                        bool success,
                        const std::string& details = "",
                        ViolationSeverity severity = ViolationSeverity::LOW);
    
    std::vector<AuditLogEntry> get_audit_logs(
        std::chrono::system_clock::time_point start,
        std::chrono::system_clock::time_point end) const;
    
    std::vector<SecurityViolation> get_violations(
        ViolationSeverity min_severity = ViolationSeverity::LOW) const;
    
    SecurityStats get_security_stats() const;
    void reset_security_stats();
    void emergency_lockdown();
    
    // Password Management
    bool validate_password_strength(const std::string& password) const;
    std::string hash_password(const std::string& password) const;
    bool verify_password(const std::string& password, const std::string& hash) const;
    void force_password_reset(const std::string& user_id);
    
    // Multi-Factor Authentication
    std::string generate_mfa_secret(const std::string& user_id);
    bool verify_mfa_token(const std::string& user_id, const std::string& token) const;
    void enable_mfa(const std::string& user_id, const std::string& secret);
    void disable_mfa(const std::string& user_id);
    
    // Input Validation & Sanitization
    bool validate_input(const std::string& input, const std::string& pattern) const;
    std::string sanitize_input(const std::string& input) const;
    bool is_valid_email(const std::string& email) const;
    bool is_valid_ip_address(const std::string& ip) const;
    bool is_safe_filename(const std::string& filename) const;
    
    // Permission Management
    PermissionManager& get_permission_manager() { return permission_manager_; }
    const PermissionManager& get_permission_manager() const { return permission_manager_; }
};

} // namespace hfx::ultra
```

### Configuration

```cpp
struct SecurityConfig {
    // Session Management
    std::chrono::minutes session_timeout{30};
    bool enable_session_renewal{true};
    size_t max_sessions_per_user{5};
    
    // API Keys
    std::chrono::hours api_key_default_expiry{24 * 30}; // 30 days
    size_t max_api_keys_per_user{10};
    
    // Rate Limiting
    bool enable_rate_limiting{true};
    int per_user_requests_per_minute{1000};
    int per_ip_requests_per_minute{100};
    int global_requests_per_second{10000};
    
    // Password Policy
    size_t min_password_length{12};
    bool require_uppercase{true};
    bool require_lowercase{true};
    bool require_numbers{true};
    bool require_special_chars{true};
    int password_history_size{5};
    
    // Multi-Factor Authentication
    bool require_mfa_for_trading{true};
    bool require_mfa_for_withdrawals{true};
    std::chrono::seconds mfa_token_validity{30};
    
    // Security Features
    bool enable_ip_whitelisting{false};
    std::vector<std::string> whitelisted_ips;
    bool enable_geolocation_blocking{false};
    std::vector<std::string> blocked_countries;
    
    // Audit Logging
    bool enable_audit_logging{true};
    std::chrono::days audit_retention_period{365};
    bool encrypt_audit_logs{true};
    
    // Advanced Security
    bool enforce_https{true};
    bool enable_csrf_protection{true};
    bool encrypt_sensitive_data{true};
    std::vector<std::string> allowed_origins;
    bool enable_cors{false};
    
    // Monitoring & Alerting
    bool enable_security_monitoring{true};
    int max_failed_login_attempts{5};
    std::chrono::minutes lockout_duration{15};
    bool enable_breach_detection{true};
};
```

### Data Structures

```cpp
// Authentication Methods
enum class AuthMethod {
    PASSWORD,
    API_KEY,
    MFA,
    OAUTH,
    CERTIFICATE
};

// Security Levels
enum class SecurityLevel {
    GUEST = 0,
    AUTHENTICATED = 1,
    AUTHORIZED = 2,
    ADMIN = 3,
    SYSTEM = 4
};

// Session Information
struct UserSession {
    std::string session_id;
    std::string user_id;
    std::string client_ip;
    std::string user_agent;
    AuthMethod auth_method;
    SecurityLevel security_level;
    std::chrono::system_clock::time_point created_at;
    std::chrono::system_clock::time_point expires_at;
    std::chrono::system_clock::time_point last_activity;
    bool is_active;
    std::unordered_set<std::string> permissions;
};

// API Key Information
struct APIKey {
    std::string key_id;
    std::string user_id;
    std::string description;
    SecurityLevel security_level;
    std::unordered_set<std::string> permissions;
    std::chrono::system_clock::time_point created_at;
    std::chrono::system_clock::time_point expires_at;
    std::chrono::system_clock::time_point last_used;
    std::atomic<bool> active{true};
    std::atomic<uint64_t> usage_count{0};
};

// Audit Log Entry
enum class AuditEventType {
    LOGIN_SUCCESS,
    LOGIN_FAILURE,
    LOGOUT,
    SESSION_CREATED,
    SESSION_EXPIRED,
    API_KEY_CREATED,
    API_KEY_REVOKED,
    TRADE_EXECUTION,
    WITHDRAWAL_REQUEST,
    PERMISSION_GRANTED,
    PERMISSION_REVOKED,
    SECURITY_VIOLATION,
    SYSTEM_ACCESS,
    DATA_EXPORT,
    CONFIGURATION_CHANGE
};

struct AuditLogEntry {
    std::string id;
    AuditEventType event_type;
    std::string user_id;
    std::string session_id;
    std::string resource;
    std::string action;
    bool success;
    std::string client_ip;
    std::string user_agent;
    std::chrono::system_clock::time_point timestamp;
    std::string details;
    ViolationSeverity severity;
    std::unordered_map<std::string, std::string> metadata;
};

// Security Violations
enum class ViolationSeverity {
    LOW = 0,
    MEDIUM = 1,
    HIGH = 2,
    CRITICAL = 3
};

struct SecurityViolation {
    std::string id;
    ViolationSeverity severity;
    std::string description;
    std::string user_id;
    std::string client_ip;
    std::chrono::system_clock::time_point timestamp;
    std::string action_taken;
    bool resolved;
    std::unordered_map<std::string, std::string> context;
};
```

---

## Usage Examples

### Basic Authentication Flow

```cpp
#include "security_manager.hpp"

SecurityManager security_mgr;
SecurityConfig config;

// Configure security policies
config.session_timeout = std::chrono::minutes(60);
config.require_mfa_for_trading = true;
config.enable_rate_limiting = true;
config.max_failed_login_attempts = 3;

security_mgr.initialize();
security_mgr.start();

// User login with MFA
std::string session_id = security_mgr.authenticate_user(
    "trader123", 
    "SecurePassword123!", 
    "123456"  // MFA token
);

if (!session_id.empty()) {
    std::cout << "Login successful, session: " << session_id << std::endl;
    
    // Log successful authentication
    security_mgr.log_audit_event(
        AuditEventType::LOGIN_SUCCESS,
        "trader123",
        "authentication",
        "login",
        true,
        "MFA authentication successful"
    );
} else {
    std::cout << "Login failed" << std::endl;
}
```

### API Key Management

```cpp
// Create trading API key
auto expires_at = std::chrono::system_clock::now() + std::chrono::hours(24 * 30); // 30 days

std::string api_key = security_mgr.create_api_key(
    "trader123",
    "High-Frequency Trading Key",
    SecurityLevel::AUTHORIZED,
    {"trades:create", "orders:read", "portfolio:read", "market_data:read"},
    expires_at
);

std::cout << "Created API key: " << api_key << std::endl;

// Validate API key for each request
bool is_valid = security_mgr.validate_api_key(api_key, "192.168.1.100");
if (is_valid) {
    // Process API request
    process_trading_request();
} else {
    // Reject request
    send_unauthorized_response();
}
```

### Authorization and Permission Checking

```cpp
// Check if user can execute a high-value trade
bool can_trade = security_mgr.authorize_trade(
    session_id,
    "BTC/USDT",
    50000.0,  // $50,000 trade
    "buy"
);

if (can_trade) {
    // Execute trade
    auto trade_result = trading_engine.execute_trade({
        .symbol = "BTC/USDT",
        .side = TradeSide::BUY,
        .amount = 50000.0
    });
    
    // Log trade execution
    security_mgr.log_audit_event(
        AuditEventType::TRADE_EXECUTION,
        get_user_from_session(session_id),
        "trading",
        "execute_trade",
        trade_result.success,
        "BTC/USDT buy order for $50,000"
    );
} else {
    security_mgr.log_audit_event(
        AuditEventType::SECURITY_VIOLATION,
        get_user_from_session(session_id),
        "trading",
        "execute_trade",
        false,
        "Unauthorized trade attempt",
        ViolationSeverity::HIGH
    );
}
```

### Rate Limiting

```cpp
// Check rate limits before processing request
std::string user_identifier = get_user_from_session(session_id);
bool within_limit = security_mgr.check_rate_limit(user_identifier, "/api/v1/trades");

if (within_limit) {
    // Process trading request
    process_trade_request(request);
} else {
    // Return rate limit error
    return HttpResponse{
        .status_code = 429,
        .body = R"({"error": "Rate limit exceeded", "retry_after": 60})"
    };
}
```

### Security Monitoring and Alerting

```cpp
// Monitor for suspicious activity
auto violations = security_mgr.get_violations(ViolationSeverity::HIGH);

for (const auto& violation : violations) {
    if (!violation.resolved) {
        // Send alert to security team
        alert_system.send_security_alert({
            .severity = "HIGH",
            .message = violation.description,
            .user_id = violation.user_id,
            .timestamp = violation.timestamp,
            .action_required = true
        });
        
        // Take automatic action for critical violations
        if (violation.severity == ViolationSeverity::CRITICAL) {
            security_mgr.terminate_all_sessions(violation.user_id);
            user_manager.suspend_account(violation.user_id);
        }
    }
}
```

### Audit Trail Analysis

```cpp
// Get audit logs for security analysis
auto start_time = std::chrono::system_clock::now() - std::chrono::hours(24);
auto end_time = std::chrono::system_clock::now();

auto audit_logs = security_mgr.get_audit_logs(start_time, end_time);

// Analyze failed login attempts
int failed_logins = 0;
std::unordered_map<std::string, int> failed_by_ip;

for (const auto& log : audit_logs) {
    if (log.event_type == AuditEventType::LOGIN_FAILURE) {
        failed_logins++;
        failed_by_ip[log.client_ip]++;
    }
}

// Alert on suspicious patterns
for (const auto& [ip, count] : failed_by_ip) {
    if (count > 10) {  // More than 10 failed attempts from same IP
        security_mgr.log_audit_event(
            AuditEventType::SECURITY_VIOLATION,
            "",
            "authentication",
            "brute_force_detection",
            false,
            "Potential brute force attack from IP: " + ip,
            ViolationSeverity::HIGH
        );
        
        // Consider IP blocking
        firewall.block_ip(ip, std::chrono::hours(1));
    }
}
```

### Emergency Security Procedures

```cpp
// Emergency lockdown in case of security breach
void handle_security_breach() {
    // Immediate lockdown
    security_mgr.emergency_lockdown();
    
    // Revoke all active sessions
    auto active_sessions = security_mgr.get_all_active_sessions();
    for (const auto& session : active_sessions) {
        security_mgr.revoke_session(session.session_id);
    }
    
    // Disable all API keys temporarily
    auto all_api_keys = security_mgr.get_all_api_keys();
    for (const auto& key : all_api_keys) {
        security_mgr.revoke_api_key(key.key_id);
    }
    
    // Log security incident
    security_mgr.log_audit_event(
        AuditEventType::SECURITY_VIOLATION,
        "SYSTEM",
        "security",
        "emergency_lockdown",
        true,
        "Emergency lockdown activated due to security breach",
        ViolationSeverity::CRITICAL
    );
    
    // Notify security team immediately
    alert_system.send_critical_alert({
        .message = "SECURITY BREACH DETECTED - Emergency lockdown activated",
        .priority = AlertPriority::CRITICAL,
        .require_immediate_response = true
    });
}
```

---

## Permission Manager (`hfx::ultra::PermissionManager`)

Role-based access control (RBAC) system for fine-grained authorization.

### Class Interface

```cpp
class PermissionManager {
public:
    // Role Management
    bool create_role(const std::string& role_name, 
                    const std::vector<std::string>& permissions);
    bool delete_role(const std::string& role_name);
    bool role_exists(const std::string& role_name) const;
    
    // User-Role Assignment
    bool assign_role_to_user(const std::string& user_id, const std::string& role_name);
    bool remove_role_from_user(const std::string& user_id, const std::string& role_name);
    std::vector<std::string> get_user_roles(const std::string& user_id) const;
    
    // Permission Management
    bool grant_permission(const std::string& user_id, const std::string& permission);
    bool revoke_permission(const std::string& user_id, const std::string& permission);
    bool has_permission(const std::string& user_id, const std::string& permission) const;
    std::unordered_set<std::string> get_user_permissions(const std::string& user_id) const;
    
    // Temporary Permissions
    bool grant_temporary_permission(const std::string& user_id, 
                                   const std::string& permission,
                                   std::chrono::system_clock::time_point expires_at);
    
    // Resource-based Permissions
    bool check_resource_access(const std::string& user_id,
                              const std::string& resource,
                              const std::string& action) const;
};
```

### Usage Examples

```cpp
PermissionManager& perm_mgr = security_mgr.get_permission_manager();

// Create roles
perm_mgr.create_role("trader", {
    "trades:create", "trades:read", "orders:create", "orders:read",
    "portfolio:read", "market_data:read"
});

perm_mgr.create_role("admin", {
    "trades:*", "orders:*", "portfolio:*", "users:*", "system:*"
});

perm_mgr.create_role("viewer", {
    "trades:read", "orders:read", "portfolio:read", "market_data:read"
});

// Assign roles to users
perm_mgr.assign_role_to_user("trader123", "trader");
perm_mgr.assign_role_to_user("admin_user", "admin");

// Check permissions
if (perm_mgr.has_permission("trader123", "trades:create")) {
    // Allow trade execution
    execute_trade();
}

// Grant temporary elevated permissions
auto expires_at = std::chrono::system_clock::now() + std::chrono::hours(1);
perm_mgr.grant_temporary_permission("trader123", "withdrawals:create", expires_at);
```

---

## Security Best Practices

### Input Validation

```cpp
// Always validate and sanitize user input
bool is_safe = security_mgr.validate_input(user_input, R"(^[a-zA-Z0-9_-]{1,50}$)");
if (!is_safe) {
    security_mgr.log_audit_event(
        AuditEventType::SECURITY_VIOLATION,
        user_id,
        "input_validation",
        "malicious_input_detected",
        false,
        "Invalid input detected: " + user_input,
        ViolationSeverity::MEDIUM
    );
    return false;
}

std::string sanitized = security_mgr.sanitize_input(user_input);
```

### Secure Session Management

```cpp
// Always validate sessions for sensitive operations
if (!security_mgr.validate_session(session_id)) {
    return HttpResponse{401, "Session expired or invalid"};
}

// Extend session on activity
security_mgr.extend_session(session_id);

// Log out on security events
if (security_event_detected) {
    security_mgr.revoke_session(session_id);
    security_mgr.log_audit_event(
        AuditEventType::SECURITY_VIOLATION,
        user_id,
        "session",
        "forced_logout",
        true,
        "Session terminated due to security event"
    );
}
```

### Comprehensive Logging

```cpp
// Log all significant security events
void log_security_event(const std::string& event_description, 
                       const std::string& user_id,
                       ViolationSeverity severity = ViolationSeverity::LOW) {
    security_mgr.log_audit_event(
        AuditEventType::SECURITY_VIOLATION,
        user_id,
        "security",
        "event",
        true,
        event_description,
        severity
    );
    
    // Send real-time alerts for high-severity events
    if (severity >= ViolationSeverity::HIGH) {
        alert_system.send_security_alert({
            .message = event_description,
            .user_id = user_id,
            .severity = severity,
            .timestamp = std::chrono::system_clock::now()
        });
    }
}
```

---

**[← Back to API Overview](./API_Overview.md) | [Next: Monitoring APIs →](./Monitoring_APIs.md)**
