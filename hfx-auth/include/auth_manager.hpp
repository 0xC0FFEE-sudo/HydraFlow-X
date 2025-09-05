#pragma once

#include <string>
#include <memory>
#include <unordered_map>
#include <mutex>
#include <chrono>
#include <optional>
#include <functional>

namespace hfx {
namespace auth {

// Forward declarations
struct User;
struct Session;
struct APIKey;
struct AuthToken;
class JWTManager;
class APIKeyManager;
class UserManager;
class SessionManager;
class AuthMiddleware;

// Authentication methods
enum class AuthMethod {
    PASSWORD = 0,
    API_KEY,
    JWT_TOKEN,
    OAUTH2,
    SAML
};

// User roles for RBAC
enum class UserRole {
    ADMIN = 0,
    TRADER,
    ANALYST,
    VIEWER,
    API_USER
};

// Authentication result
enum class AuthResult {
    SUCCESS = 0,
    INVALID_CREDENTIALS,
    EXPIRED_TOKEN,
    INSUFFICIENT_PERMISSIONS,
    ACCOUNT_LOCKED,
    RATE_LIMIT_EXCEEDED,
    SYSTEM_ERROR
};

// User information
struct User {
    std::string user_id;
    std::string username;
    std::string email;
    std::string password_hash;
    UserRole role;
    bool is_active;
    bool is_locked;
    std::chrono::system_clock::time_point created_at;
    std::chrono::system_clock::time_point last_login;
    std::chrono::system_clock::time_point password_changed_at;
    int failed_login_attempts;
    std::chrono::system_clock::time_point lockout_until;

    User() : role(UserRole::VIEWER), is_active(true), is_locked(false), failed_login_attempts(0) {}
};

// Session information
struct Session {
    std::string session_id;
    std::string user_id;
    std::string ip_address;
    std::string user_agent;
    std::chrono::system_clock::time_point created_at;
    std::chrono::system_clock::time_point last_activity;
    std::chrono::system_clock::time_point expires_at;
    bool is_active;

    Session() : is_active(true) {}
};

// API Key information
struct APIKey {
    std::string key_id;
    std::string user_id;
    std::string name;
    std::string key_hash;
    std::string permissions; // JSON string of permissions
    bool is_active;
    std::chrono::system_clock::time_point created_at;
    std::chrono::system_clock::time_point last_used;
    std::chrono::system_clock::time_point expires_at;
    int usage_count;

    APIKey() : is_active(true), usage_count(0) {}
};

// JWT Token structure
struct AuthToken {
    std::string token;
    std::string user_id;
    UserRole role;
    std::chrono::system_clock::time_point issued_at;
    std::chrono::system_clock::time_point expires_at;
    std::string issuer;
    std::string audience;

    AuthToken() : role(UserRole::VIEWER) {}
};

// Authentication configuration
struct AuthConfig {
    // JWT settings
    std::string jwt_secret;
    std::string jwt_issuer;
    std::chrono::seconds jwt_expiration_time = std::chrono::hours(24);
    std::chrono::seconds jwt_refresh_expiration_time = std::chrono::hours(168); // 7 days

    // Session settings
    std::chrono::minutes session_timeout = std::chrono::minutes(30);
    std::chrono::hours session_cleanup_interval = std::chrono::hours(1);

    // Security settings
    int max_login_attempts = 5;
    std::chrono::minutes lockout_duration = std::chrono::minutes(15);
    int min_password_length = 8;

    // Rate limiting
    int max_requests_per_minute = 60;
    int max_requests_per_hour = 1000;

    // API Key settings
    bool allow_api_keys = true;
    std::chrono::days api_key_expiration_days = std::chrono::days(365);

    // OAuth2 settings (future)
    std::string oauth2_client_id;
    std::string oauth2_client_secret;
    std::vector<std::string> oauth2_providers;
};

// Main authentication manager
class AuthManager {
public:
    explicit AuthManager(const AuthConfig& config);
    ~AuthManager();

    // Core authentication methods
    AuthResult authenticate(const std::string& username, const std::string& password,
                           AuthToken& token);
    AuthResult authenticate_api_key(const std::string& api_key, AuthToken& token);
    AuthResult authenticate_jwt(const std::string& jwt_token, AuthToken& token);

    // User management
    bool create_user(const std::string& username, const std::string& email,
                    const std::string& password, UserRole role = UserRole::VIEWER);
    bool update_user(const std::string& user_id, const User& updated_user);
    bool delete_user(const std::string& user_id);
    std::optional<User> get_user(const std::string& user_id);
    std::optional<User> get_user_by_username(const std::string& username);
    std::vector<User> get_all_users();

    // Session management
    std::string create_session(const std::string& user_id, const std::string& ip_address,
                              const std::string& user_agent);
    bool validate_session(const std::string& session_id);
    bool invalidate_session(const std::string& session_id);
    bool invalidate_user_sessions(const std::string& user_id);
    std::vector<Session> get_user_sessions(const std::string& user_id);

    // API Key management
    std::string create_api_key(const std::string& user_id, const std::string& name,
                              const std::string& permissions = "{}");
    bool revoke_api_key(const std::string& key_id);
    std::vector<APIKey> get_user_api_keys(const std::string& user_id);
    bool validate_api_key_permissions(const std::string& key_id, const std::string& permission);

    // JWT token management
    std::string generate_jwt_token(const User& user);
    std::string generate_refresh_token(const User& user);
    std::optional<AuthToken> validate_jwt_token(const std::string& token);
    std::optional<AuthToken> refresh_jwt_token(const std::string& refresh_token);

    // Permission and authorization
    bool has_permission(const std::string& user_id, const std::string& permission);
    bool has_role(const std::string& user_id, UserRole role);
    std::vector<std::string> get_user_permissions(const std::string& user_id);

    // Password management
    bool change_password(const std::string& user_id, const std::string& old_password,
                        const std::string& new_password);
    bool reset_password(const std::string& user_id);
    bool validate_password_strength(const std::string& password);

    // Security monitoring
    void log_failed_login_attempt(const std::string& username, const std::string& ip_address);
    void log_successful_login(const std::string& user_id, const std::string& ip_address);
    void log_security_event(const std::string& event_type, const std::string& user_id,
                           const std::string& details);

    // Rate limiting
    bool check_rate_limit(const std::string& identifier, int max_requests, std::chrono::seconds window);
    void record_request(const std::string& identifier);

    // Configuration and statistics
    void update_config(const AuthConfig& config);
    const AuthConfig& get_config() const;

    struct AuthStats {
        std::atomic<uint64_t> total_logins{0};
        std::atomic<uint64_t> failed_logins{0};
        std::atomic<uint64_t> active_sessions{0};
        std::atomic<uint64_t> active_api_keys{0};
        std::atomic<uint64_t> rate_limit_hits{0};
        std::chrono::system_clock::time_point last_reset;
    };

    const AuthStats& get_auth_stats() const;
    void reset_auth_stats();

    // Cleanup and maintenance
    void cleanup_expired_sessions();
    void cleanup_expired_api_keys();
    void cleanup_rate_limits();

private:
    AuthConfig config_;
    std::unique_ptr<JWTManager> jwt_manager_;
    std::unique_ptr<APIKeyManager> api_key_manager_;
    std::unique_ptr<UserManager> user_manager_;
    std::unique_ptr<SessionManager> session_manager_;
    // std::unique_ptr<AuthMiddleware> auth_middleware_;  // TODO: Implement later

    mutable std::mutex config_mutex_;

    // Statistics
    mutable AuthStats stats_;

    // Rate limiting data
    struct RateLimitEntry {
        int request_count;
        std::chrono::system_clock::time_point window_start;
        std::chrono::system_clock::time_point last_request;
    };

    std::unordered_map<std::string, RateLimitEntry> rate_limits_;
    mutable std::mutex rate_limit_mutex_;

    // Internal helper methods
    std::string hash_password(const std::string& password);
    bool verify_password(const std::string& password, const std::string& hash);
    std::string generate_secure_token(size_t length = 32);
    std::string generate_session_id();
    std::string generate_api_key();

    AuthResult check_account_status(const User& user);
    void update_user_last_login(const std::string& user_id);
    void handle_failed_login(const std::string& user_id);

    // Permission helpers
    std::vector<std::string> get_role_permissions(UserRole role);
    bool check_permission(const std::vector<std::string>& user_permissions,
                         const std::string& required_permission);
};

// Utility functions
std::string auth_result_to_string(AuthResult result);
std::string user_role_to_string(UserRole role);
UserRole string_to_user_role(const std::string& role_str);
std::string hash_string(const std::string& input);
bool validate_email(const std::string& email);

} // namespace auth
} // namespace hfx
