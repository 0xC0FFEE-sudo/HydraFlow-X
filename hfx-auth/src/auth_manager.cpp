/**
 * @file auth_manager.cpp
 * @brief Main Authentication Manager Implementation
 */

#include "../include/auth_manager.hpp"
#include "../include/jwt_manager.hpp"
#include "../include/user_manager.hpp"
#include "../include/session_manager.hpp"
#include "../include/api_key_manager.hpp"
#include "hfx-log/include/logger.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <random>
#include <chrono>
#include <openssl/sha.h>
#include <openssl/hmac.h>
#include <openssl/rand.h>
#include <regex>

namespace hfx {
namespace auth {

// Forward declarations for internal classes
class JWTManager;
class APIKeyManager;
class UserManager;
class SessionManager;
class AuthMiddleware;

// Utility functions implementation
std::string auth_result_to_string(AuthResult result) {
    switch (result) {
        case AuthResult::SUCCESS: return "SUCCESS";
        case AuthResult::INVALID_CREDENTIALS: return "INVALID_CREDENTIALS";
        case AuthResult::EXPIRED_TOKEN: return "EXPIRED_TOKEN";
        case AuthResult::INSUFFICIENT_PERMISSIONS: return "INSUFFICIENT_PERMISSIONS";
        case AuthResult::ACCOUNT_LOCKED: return "ACCOUNT_LOCKED";
        case AuthResult::RATE_LIMIT_EXCEEDED: return "RATE_LIMIT_EXCEEDED";
        case AuthResult::SYSTEM_ERROR: return "SYSTEM_ERROR";
        default: return "UNKNOWN";
    }
}

std::string user_role_to_string(UserRole role) {
    switch (role) {
        case UserRole::ADMIN: return "ADMIN";
        case UserRole::TRADER: return "TRADER";
        case UserRole::ANALYST: return "ANALYST";
        case UserRole::VIEWER: return "VIEWER";
        case UserRole::API_USER: return "API_USER";
        default: return "UNKNOWN";
    }
}

UserRole string_to_user_role(const std::string& role_str) {
    if (role_str == "ADMIN") return UserRole::ADMIN;
    if (role_str == "TRADER") return UserRole::TRADER;
    if (role_str == "ANALYST") return UserRole::ANALYST;
    if (role_str == "VIEWER") return UserRole::VIEWER;
    if (role_str == "API_USER") return UserRole::API_USER;
    return UserRole::VIEWER;
}

std::string hash_string(const std::string& input) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, input.c_str(), input.size());
    SHA256_Final(hash, &sha256);

    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    }
    return ss.str();
}

bool validate_email(const std::string& email) {
    const std::regex email_pattern(R"([a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,})");
    return std::regex_match(email, email_pattern);
}

// AuthManager implementation
AuthManager::AuthManager(const AuthConfig& config) : config_(config) {
    stats_.last_reset = std::chrono::system_clock::now();

    // Initialize sub-managers
    jwt_manager_ = std::make_unique<JWTManager>(JWTConfig{
        config.jwt_secret,
        config.jwt_issuer,
        std::to_string(config.jwt_expiration_time.count()),
        config.jwt_refresh_expiration_time
    });

    // Initialize other managers
    api_key_manager_ = std::make_unique<APIKeyManager>(APIKeyConfig{
        config.api_key_expiration_days,
        10, // max_keys_per_user
        32, // key_length
        "hfx_"
    });
    
    user_manager_ = std::make_unique<UserManager>();
    
    session_manager_ = std::make_unique<SessionManager>(SessionConfig{
        config.session_timeout,
        std::chrono::minutes(5), // cleanup_interval
        std::chrono::hours(8) // extended_timeout
    });

    HFX_LOG_INFO("[AuthManager] Initialized with all sub-managers");
}

AuthManager::~AuthManager() = default;

AuthResult AuthManager::authenticate(const std::string& username, const std::string& password,
                                   AuthToken& token) {
    // Rate limiting check
    if (!check_rate_limit("login_" + username, config_.max_requests_per_minute, std::chrono::minutes(1))) {
        return AuthResult::RATE_LIMIT_EXCEEDED;
    }

    // Get user by username
    auto user_opt = get_user_by_username(username);
    if (!user_opt) {
        log_failed_login_attempt(username, "");
        return AuthResult::INVALID_CREDENTIALS;
    }

    User user = *user_opt;

    // Check account status
    AuthResult status_result = check_account_status(user);
    if (status_result != AuthResult::SUCCESS) {
        return status_result;
    }

    // Verify password
    if (!verify_password(password, user.password_hash)) {
        handle_failed_login(user.user_id);
        log_failed_login_attempt(username, "");
        return AuthResult::INVALID_CREDENTIALS;
    }

    // Authentication successful
    log_successful_login(user.user_id, "");
    update_user_last_login(user.user_id);

    // Generate JWT token
    if (jwt_manager_) {
        token.token = jwt_manager_->generate_access_token(user.user_id, user.role);
        token.user_id = user.user_id;
        token.role = user.role;
        token.issued_at = std::chrono::system_clock::now();
        token.expires_at = token.issued_at + config_.jwt_expiration_time;
        token.issuer = config_.jwt_issuer;
        token.audience = "hydraflow-api";
    }

    stats_.total_logins.fetch_add(1, std::memory_order_relaxed);
    return AuthResult::SUCCESS;
}

AuthResult AuthManager::authenticate_api_key(const std::string& api_key, AuthToken& token) {
    // Rate limiting check
    if (!check_rate_limit("api_" + hash_string(api_key).substr(0, 8), config_.max_requests_per_hour, std::chrono::hours(1))) {
        return AuthResult::RATE_LIMIT_EXCEEDED;
    }

    // API key validation
    if (!api_key_manager_ || !api_key_manager_->validate_api_key(api_key)) {
        return AuthResult::INVALID_CREDENTIALS;
    }

    // Get user ID from API key
    auto user_id_opt = api_key_manager_->get_user_id_from_api_key(api_key);
    if (!user_id_opt) {
        return AuthResult::INVALID_CREDENTIALS;
    }

    // Get user details
    auto user_opt = get_user(*user_id_opt);
    if (!user_opt) {
        return AuthResult::INVALID_CREDENTIALS;
    }

    // Create token
    token.token = "api_" + hash_string(api_key);
    token.user_id = *user_id_opt;
    token.role = user_opt->role;
    token.issued_at = std::chrono::system_clock::now();
    token.expires_at = token.issued_at + std::chrono::hours(24);
    token.issuer = config_.jwt_issuer;
    token.audience = "hydraflow-api";

    stats_.total_logins.fetch_add(1, std::memory_order_relaxed);
    return AuthResult::SUCCESS;
}

AuthResult AuthManager::authenticate_jwt(const std::string& jwt_token, AuthToken& token) {
    if (!jwt_manager_) {
        return AuthResult::SYSTEM_ERROR;
    }

    auto claims_opt = jwt_manager_->validate_access_token(jwt_token);
    if (!claims_opt) {
        return AuthResult::INVALID_CREDENTIALS;
    }

    JWTClaims claims = *claims_opt;

    // Check if token is expired
    if (std::chrono::system_clock::now() > claims.expires_at) {
        return AuthResult::EXPIRED_TOKEN;
    }

    token.token = jwt_token;
    token.user_id = claims.subject;
    token.role = claims.role;
    token.issued_at = claims.issued_at;
    token.expires_at = claims.expires_at;
    token.issuer = claims.issuer;
    token.audience = claims.audience;

    stats_.total_logins.fetch_add(1, std::memory_order_relaxed);
    return AuthResult::SUCCESS;
}

// User management methods
bool AuthManager::create_user(const std::string& username, const std::string& email,
                            const std::string& password, UserRole role) {
    // Validate input
    if (username.empty() || email.empty() || password.empty()) {
        return false;
    }

    if (!validate_email(email)) {
        return false;
    }

    if (!validate_password_strength(password)) {
        return false;
    }

    // Check if user already exists
    if (get_user_by_username(username)) {
        return false;
    }

    User new_user;
    new_user.user_id = generate_secure_token(16);
    new_user.username = username;
    new_user.email = email;
    new_user.password_hash = hash_password(password);
    new_user.role = role;
    new_user.created_at = std::chrono::system_clock::now();

    // Store user using user manager
    if (user_manager_) {
        return user_manager_->create_user(new_user);
    }
    return false;
}

std::optional<User> AuthManager::get_user(const std::string& user_id) {
    if (user_manager_) {
        return user_manager_->get_user(user_id);
    }
    return std::nullopt;
}

std::optional<User> AuthManager::get_user_by_username(const std::string& username) {
    if (user_manager_) {
        return user_manager_->get_user_by_username(username);
    }
    return std::nullopt;
}

// JWT token management
std::string AuthManager::generate_jwt_token(const User& user) {
    if (!jwt_manager_) {
        return "";
    }
    return jwt_manager_->generate_access_token(user.user_id, user.role);
}

std::optional<AuthToken> AuthManager::validate_jwt_token(const std::string& token) {
    AuthToken auth_token;
    AuthResult result = authenticate_jwt(token, auth_token);

    if (result == AuthResult::SUCCESS) {
        return auth_token;
    }
    return std::nullopt;
}

// Permission checking
bool AuthManager::has_permission(const std::string& user_id, const std::string& permission) {
    auto user_opt = get_user(user_id);
    if (!user_opt) {
        return false;
    }

    auto user_permissions = get_user_permissions(user_id);
    return check_permission(user_permissions, permission);
}

bool AuthManager::has_role(const std::string& user_id, UserRole role) {
    auto user_opt = get_user(user_id);
    if (!user_opt) {
        return false;
    }

    return user_opt->role == role || user_opt->role == UserRole::ADMIN;
}

std::vector<std::string> AuthManager::get_user_permissions(const std::string& user_id) {
    auto user_opt = get_user(user_id);
    if (!user_opt) {
        return {};
    }

    return get_role_permissions(user_opt->role);
}

// Rate limiting
bool AuthManager::check_rate_limit(const std::string& identifier, int max_requests, std::chrono::seconds window) {
    std::lock_guard<std::mutex> lock(rate_limit_mutex_);

    auto now = std::chrono::system_clock::now();
    auto& entry = rate_limits_[identifier];

    // Reset window if needed
    if (now - entry.window_start > window) {
        entry.request_count = 0;
        entry.window_start = now;
    }

    if (entry.request_count >= max_requests) {
        stats_.rate_limit_hits.fetch_add(1, std::memory_order_relaxed);
        return false;
    }

    entry.request_count++;
    entry.last_request = now;
    return true;
}

void AuthManager::record_request(const std::string& identifier) {
    std::lock_guard<std::mutex> lock(rate_limit_mutex_);
    rate_limits_[identifier].request_count++;
}

// Password management
bool AuthManager::validate_password_strength(const std::string& password) {
    if (password.length() < config_.min_password_length) {
        return false;
    }

    // Check for at least one uppercase, lowercase, and digit
    bool has_upper = false, has_lower = false, has_digit = false;
    for (char c : password) {
        if (std::isupper(c)) has_upper = true;
        if (std::islower(c)) has_lower = true;
        if (std::isdigit(c)) has_digit = true;
    }

    return has_upper && has_lower && has_digit;
}

// Security logging
void AuthManager::log_failed_login_attempt(const std::string& username, const std::string& ip_address) {
    stats_.failed_logins.fetch_add(1, std::memory_order_relaxed);

    HFX_LOG_WARN("[AUTH] Failed login attempt for user: " + username + " from IP: " + ip_address);
}

void AuthManager::log_successful_login(const std::string& user_id, const std::string& ip_address) {
    HFX_LOG_INFO("[AUTH] Successful login for user: " + user_id + " from IP: " + ip_address);
}

void AuthManager::log_security_event(const std::string& event_type, const std::string& user_id,
                                   const std::string& details) {
    HFX_LOG_WARN("[AUTH] Security event [" + event_type + "] for user: " + user_id + " - " + details);
}

// Private helper methods
std::string AuthManager::hash_password(const std::string& password) {
    // Add salt for security
    std::string salt = generate_secure_token(16);
    return hash_string(salt + password);
}

bool AuthManager::verify_password(const std::string& password, const std::string& hash) {
    // Simplified password verification
    return hash_string(password) == hash;
}

std::string AuthManager::generate_secure_token(size_t length) {
    const std::string chars = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    std::string result;
    result.reserve(length);

    for (size_t i = 0; i < length; ++i) {
        unsigned char random_byte;
        RAND_bytes(&random_byte, 1);
        result += chars[random_byte % chars.size()];
    }

    return result;
}

AuthResult AuthManager::check_account_status(const User& user) {
    if (!user.is_active) {
        return AuthResult::ACCOUNT_LOCKED;
    }

    if (user.is_locked) {
        auto now = std::chrono::system_clock::now();
        if (now < user.lockout_until) {
            return AuthResult::ACCOUNT_LOCKED;
        }
        // Unlock account if lockout period has passed
        // (would update database in real implementation)
    }

    return AuthResult::SUCCESS;
}

std::vector<std::string> AuthManager::get_role_permissions(UserRole role) {
    switch (role) {
        case UserRole::ADMIN:
            return {"read", "write", "delete", "admin", "trade", "view", "analyze"};
        case UserRole::TRADER:
            return {"read", "write", "trade", "view"};
        case UserRole::ANALYST:
            return {"read", "analyze", "view"};
        case UserRole::VIEWER:
            return {"read", "view"};
        case UserRole::API_USER:
            return {"read", "trade"};
        default:
            return {"view"};
    }
}

bool AuthManager::check_permission(const std::vector<std::string>& user_permissions,
                                 const std::string& required_permission) {
    return std::find(user_permissions.begin(), user_permissions.end(), required_permission)
           != user_permissions.end();
}

void AuthManager::handle_failed_login(const std::string& user_id) {
    if (user_manager_) {
        user_manager_->increment_failed_attempts(user_id);
    }
    log_security_event("FAILED_LOGIN", user_id, "Invalid credentials");
}

void AuthManager::update_user_last_login(const std::string& user_id) {
    if (user_manager_) {
        user_manager_->update_last_login(user_id);
    }
}

// Configuration and statistics
void AuthManager::update_config(const AuthConfig& config) {
    std::lock_guard<std::mutex> lock(config_mutex_);
    config_ = config;
}

const AuthConfig& AuthManager::get_config() const {
    std::lock_guard<std::mutex> lock(config_mutex_);
    return config_;
}

const AuthManager::AuthStats& AuthManager::get_auth_stats() const {
    return stats_;
}

void AuthManager::reset_auth_stats() {
    stats_.total_logins.store(0, std::memory_order_relaxed);
    stats_.failed_logins.store(0, std::memory_order_relaxed);
    stats_.active_sessions.store(0, std::memory_order_relaxed);
    stats_.active_api_keys.store(0, std::memory_order_relaxed);
    stats_.rate_limit_hits.store(0, std::memory_order_relaxed);
    stats_.last_reset = std::chrono::system_clock::now();
}

// Complete implementations using sub-managers
bool AuthManager::update_user(const std::string& user_id, const User& updated_user) {
    if (user_manager_) {
        return user_manager_->update_user(user_id, updated_user);
    }
    return false;
}

bool AuthManager::delete_user(const std::string& user_id) {
    if (user_manager_) {
        return user_manager_->delete_user(user_id);
    }
    return false;
}

std::vector<User> AuthManager::get_all_users() {
    if (user_manager_) {
        return user_manager_->get_all_users();
    }
    return {};
}

std::string AuthManager::create_session(const std::string& user_id, const std::string& ip_address, const std::string& user_agent) {
    if (session_manager_) {
        return session_manager_->create_session(user_id, ip_address, user_agent);
    }
    return "";
}

bool AuthManager::validate_session(const std::string& session_id) {
    if (session_manager_) {
        return session_manager_->validate_session(session_id);
    }
    return false;
}

bool AuthManager::invalidate_session(const std::string& session_id) {
    if (session_manager_) {
        return session_manager_->invalidate_session(session_id);
    }
    return false;
}

bool AuthManager::invalidate_user_sessions(const std::string& user_id) {
    if (session_manager_) {
        return session_manager_->invalidate_user_sessions(user_id);
    }
    return false;
}

std::vector<Session> AuthManager::get_user_sessions(const std::string& user_id) {
    if (session_manager_) {
        return session_manager_->get_user_sessions(user_id);
    }
    return {};
}

std::string AuthManager::create_api_key(const std::string& user_id, const std::string& name, const std::string& permissions) {
    if (api_key_manager_) {
        return api_key_manager_->create_api_key(user_id, name, permissions);
    }
    return "";
}

bool AuthManager::revoke_api_key(const std::string& key_id) {
    if (api_key_manager_) {
        return api_key_manager_->revoke_api_key(key_id);
    }
    return false;
}

std::vector<APIKey> AuthManager::get_user_api_keys(const std::string& user_id) {
    if (api_key_manager_) {
        return api_key_manager_->get_user_api_keys(user_id);
    }
    return {};
}

bool AuthManager::validate_api_key_permissions(const std::string& key_id, const std::string& permission) {
    if (api_key_manager_) {
        return api_key_manager_->validate_api_key_permissions(key_id, permission);
    }
    return false;
}

std::string AuthManager::generate_refresh_token(const User& user) {
    if (jwt_manager_) {
        return jwt_manager_->generate_refresh_token(user.user_id, user.role);
    }
    return "";
}

std::optional<AuthToken> AuthManager::refresh_jwt_token(const std::string& refresh_token) {
    if (jwt_manager_) {
        auto claims = jwt_manager_->validate_refresh_token(refresh_token);
        if (claims) {
            AuthToken token;
            token.token = jwt_manager_->generate_access_token(claims->subject, claims->role);
            token.user_id = claims->subject;
            token.role = claims->role;
            token.issued_at = std::chrono::system_clock::now();
            token.expires_at = token.issued_at + config_.jwt_expiration_time;
            token.issuer = config_.jwt_issuer;
            return token;
        }
    }
    return std::nullopt;
}

bool AuthManager::change_password(const std::string& user_id, const std::string& old_password, const std::string& new_password) {
    if (!user_manager_) return false;
    
    auto user = user_manager_->get_user(user_id);
    if (!user) return false;
    
    // Verify old password
    if (!verify_password(old_password, user->password_hash)) {
        return false;
    }
    
    // Validate new password strength
    if (!validate_password_strength(new_password)) {
        return false;
    }
    
    // Update password
    return user_manager_->update_password_hash(user_id, hash_password(new_password));
}

bool AuthManager::reset_password(const std::string& user_id) {
    if (user_manager_) {
        // Generate temporary password
        std::string temp_password = generate_secure_token(12);
        std::string hashed_temp = hash_password(temp_password);
        
        if (user_manager_->update_password_hash(user_id, hashed_temp)) {
            HFX_LOG_INFO("[AuthManager] Password reset for user: " + user_id + " - Temporary password: " + temp_password);
            return true;
        }
    }
    return false;
}

void AuthManager::cleanup_expired_sessions() {
    if (session_manager_) {
        session_manager_->cleanup_expired_sessions();
    }
}

void AuthManager::cleanup_expired_api_keys() {
    if (api_key_manager_) {
        api_key_manager_->cleanup_expired_keys();
    }
}

void AuthManager::cleanup_rate_limits() {
    std::lock_guard<std::mutex> lock(rate_limit_mutex_);
    auto now = std::chrono::system_clock::now();
    
    for (auto it = rate_limits_.begin(); it != rate_limits_.end();) {
        if (now - it->second.last_request > std::chrono::hours(24)) {
            it = rate_limits_.erase(it);
        } else {
            ++it;
        }
    }
}

} // namespace auth
} // namespace hfx
