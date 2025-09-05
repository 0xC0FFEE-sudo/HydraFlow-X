/**
 * @file security_manager.cpp
 * @brief Comprehensive security management system for HydraFlow-X
 */

#include "security_manager.hpp"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <random>
#include <iomanip>
#include <regex>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/aes.h>

namespace hfx::ultra {

// Utility function implementations
namespace security_utils {
    std::string severity_to_string(ViolationSeverity severity) {
        switch (severity) {
            case ViolationSeverity::LOW: return "LOW";
            case ViolationSeverity::MEDIUM: return "MEDIUM";
            case ViolationSeverity::HIGH: return "HIGH";
            case ViolationSeverity::CRITICAL: return "CRITICAL";
            case ViolationSeverity::EMERGENCY: return "EMERGENCY";
            default: return "UNKNOWN";
        }
    }

    ViolationSeverity string_to_severity(const std::string& severity_str) {
        if (severity_str == "LOW") return ViolationSeverity::LOW;
        if (severity_str == "MEDIUM") return ViolationSeverity::MEDIUM;
        if (severity_str == "HIGH") return ViolationSeverity::HIGH;
        if (severity_str == "CRITICAL") return ViolationSeverity::CRITICAL;
        if (severity_str == "EMERGENCY") return ViolationSeverity::EMERGENCY;
        return ViolationSeverity::LOW;
    }

    std::string auth_method_to_string(AuthMethod method) {
        switch (method) {
            case AuthMethod::API_KEY: return "API_KEY";
            case AuthMethod::JWT_TOKEN: return "JWT_TOKEN";
            case AuthMethod::OAUTH2: return "OAUTH2";
            case AuthMethod::CERTIFICATE: return "CERTIFICATE";
            case AuthMethod::HARDWARE_TOKEN: return "HARDWARE_TOKEN";
            case AuthMethod::BIOMETRIC: return "BIOMETRIC";
            case AuthMethod::MULTI_FACTOR: return "MULTI_FACTOR";
            default: return "UNKNOWN";
        }
    }

    std::string security_level_to_string(SecurityLevel level) {
        switch (level) {
            case SecurityLevel::PUBLIC: return "PUBLIC";
            case SecurityLevel::AUTHENTICATED: return "AUTHENTICATED";
            case SecurityLevel::AUTHORIZED: return "AUTHORIZED";
            case SecurityLevel::ADMIN: return "ADMIN";
            case SecurityLevel::SYSTEM: return "SYSTEM";
            default: return "UNKNOWN";
        }
    }

    std::string event_type_to_string(AuditEventType event_type) {
        switch (event_type) {
            case AuditEventType::LOGIN_SUCCESS: return "LOGIN_SUCCESS";
            case AuditEventType::LOGIN_FAILURE: return "LOGIN_FAILURE";
            case AuditEventType::LOGOUT: return "LOGOUT";
            case AuditEventType::API_ACCESS: return "API_ACCESS";
            case AuditEventType::PERMISSION_DENIED: return "PERMISSION_DENIED";
            case AuditEventType::DATA_ACCESS: return "DATA_ACCESS";
            case AuditEventType::DATA_MODIFICATION: return "DATA_MODIFICATION";
            case AuditEventType::CONFIGURATION_CHANGE: return "CONFIGURATION_CHANGE";
            case AuditEventType::TRADE_EXECUTION: return "TRADE_EXECUTION";
            case AuditEventType::FUND_TRANSFER: return "FUND_TRANSFER";
            case AuditEventType::KEY_GENERATION: return "KEY_GENERATION";
            case AuditEventType::KEY_ACCESS: return "KEY_ACCESS";
            case AuditEventType::SYSTEM_COMMAND: return "SYSTEM_COMMAND";
            case AuditEventType::SECURITY_VIOLATION: return "SECURITY_VIOLATION";
            case AuditEventType::RATE_LIMIT_EXCEEDED: return "RATE_LIMIT_EXCEEDED";
            case AuditEventType::SUSPICIOUS_ACTIVITY: return "SUSPICIOUS_ACTIVITY";
            default: return "UNKNOWN";
        }
    }

    bool is_strong_password(const std::string& password) {
        if (password.length() < 12) return false;
        
        bool has_upper = false, has_lower = false, has_digit = false, has_special = false;
        
        for (char c : password) {
            if (std::isupper(c)) has_upper = true;
            else if (std::islower(c)) has_lower = true;
            else if (std::isdigit(c)) has_digit = true;
            else if (std::ispunct(c)) has_special = true;
        }
        
        return has_upper && has_lower && has_digit && has_special;
    }

    bool is_valid_session_id(const std::string& session_id) {
        // Session ID should be 32+ hex characters
        if (session_id.length() < 32) return false;
        return std::all_of(session_id.begin(), session_id.end(), 
                          [](char c) { return std::isxdigit(c); });
    }

    bool is_valid_api_key_format(const std::string& api_key) {
        // API key should start with "hfx_" followed by 40+ hex characters
        if (api_key.length() < 44 || !api_key.starts_with("hfx_")) return false;
        std::string key_part = api_key.substr(4);
        return std::all_of(key_part.begin(), key_part.end(), 
                          [](char c) { return std::isxdigit(c); });
    }

    double calculate_password_strength(const std::string& password) {
        double score = 0.0;
        
        // Length bonus
        score += std::min(password.length() * 2.0, 20.0);
        
        // Character diversity
        bool has_upper = false, has_lower = false, has_digit = false, has_special = false;
        std::unordered_set<char> unique_chars;
        
        for (char c : password) {
            unique_chars.insert(c);
            if (std::isupper(c)) has_upper = true;
            else if (std::islower(c)) has_lower = true;
            else if (std::isdigit(c)) has_digit = true;
            else if (std::ispunct(c)) has_special = true;
        }
        
        if (has_upper) score += 10;
        if (has_lower) score += 10;
        if (has_digit) score += 10;
        if (has_special) score += 15;
        
        // Unique character bonus
        score += unique_chars.size() * 2;
        
        // Penalty for common patterns
        if (password.find("123") != std::string::npos) score -= 10;
        if (password.find("password") != std::string::npos) score -= 20;
        if (password.find("admin") != std::string::npos) score -= 15;
        
        return std::max(0.0, std::min(100.0, score));
    }

    std::string format_session_info(const UserSession& session) {
        std::stringstream ss;
        ss << "Session[" << session.session_id.substr(0, 8) << "...] "
           << "User: " << session.user_id << " "
           << "IP: " << session.client_ip << " "
           << "Level: " << security_level_to_string(session.clearance_level) << " "
           << "Active: " << (session.active.load() ? "Yes" : "No") << " "
           << "Requests: " << session.request_count.load();
        return ss.str();
    }
}

// PermissionManager implementation
bool PermissionManager::grant_permission(const std::string& user_id, const std::string& permission) {
    std::lock_guard<std::mutex> lock(permissions_mutex_);
    user_permissions_[user_id].insert(permission);
    return true;
}

bool PermissionManager::revoke_permission(const std::string& user_id, const std::string& permission) {
    std::lock_guard<std::mutex> lock(permissions_mutex_);
    auto it = user_permissions_.find(user_id);
    if (it != user_permissions_.end()) {
        it->second.erase(permission);
        return true;
    }
    return false;
}

bool PermissionManager::has_permission(const std::string& user_id, const std::string& permission) const {
    std::lock_guard<std::mutex> lock(permissions_mutex_);
    
    // Check direct permissions
    auto user_it = user_permissions_.find(user_id);
    if (user_it != user_permissions_.end() && user_it->second.count(permission)) {
        return true;
    }
    
    // Check role-based permissions
    auto roles_it = user_roles_.find(user_id);
    if (roles_it != user_roles_.end()) {
        for (const auto& role : roles_it->second) {
            auto role_perms_it = role_permissions_.find(role);
            if (role_perms_it != role_permissions_.end() && role_perms_it->second.count(permission)) {
                return true;
            }
        }
    }
    
    return false;
}

bool PermissionManager::create_role(const std::string& role_name, 
                                   const std::unordered_set<std::string>& permissions) {
    std::lock_guard<std::mutex> lock(permissions_mutex_);
    role_permissions_[role_name] = permissions;
    return true;
}

bool PermissionManager::assign_role(const std::string& user_id, const std::string& role_name) {
    std::lock_guard<std::mutex> lock(permissions_mutex_);
    if (role_permissions_.count(role_name)) {
        user_roles_[user_id].insert(role_name);
        return true;
    }
    return false;
}

bool PermissionManager::can_access_resource(const std::string& user_id, const std::string& resource, 
                                           const std::string& action) const {
    std::string permission = resource + ":" + action;
    return has_permission(user_id, permission) || has_permission(user_id, resource + ":*");
}

// SecurityManager implementation
SecurityManager::SecurityManager(const SecurityConfig& config) 
    : config_(config) {
    HFX_LOG_INFO("🔒 Initializing Security Manager...");
    HFX_LOG_INFO("[LOG] Message");
    HFX_LOG_INFO("[LOG] Message");
    HFX_LOG_INFO("[LOG] Message");
    HFX_LOG_INFO("[LOG] Message");
}

SecurityManager::~SecurityManager() {
    stop();
}

bool SecurityManager::initialize() {
    HFX_LOG_INFO("🔧 Initializing security system components...");
    
    // Set up default roles and permissions
    permission_manager_.create_role("trader", {
        "trades:create", "trades:read", "orders:create", "orders:read", 
        "portfolio:read", "market_data:read"
    });
    
    permission_manager_.create_role("admin", {
        "users:*", "system:*", "config:*", "audit:*", "keys:*"
    });
    
    permission_manager_.create_role("viewer", {
        "trades:read", "orders:read", "portfolio:read", "market_data:read"
    });
    
    // Initialize OpenSSL
    if (!RAND_status()) {
        HFX_LOG_ERROR("⚠️  OpenSSL random number generator not properly seeded");
    }
    
    HFX_LOG_INFO("✅ Security system initialized");
    return true;
}

bool SecurityManager::start() {
    if (running_.load()) {
        HFX_LOG_INFO("⚠️  Security system already running");
        return true;
    }
    
    HFX_LOG_INFO("🚀 Starting security system...");
    
    running_.store(true);
    shutdown_requested_.store(false);
    
    // Start worker threads
    session_cleanup_worker_ = std::thread(&SecurityManager::session_cleanup_worker, this);
    audit_log_writer_ = std::thread(&SecurityManager::audit_log_writer_worker, this);
    rate_limit_refill_worker_ = std::thread(&SecurityManager::rate_limit_refill_worker, this);
    security_monitor_ = std::thread(&SecurityManager::security_monitor_worker, this);
    
    HFX_LOG_INFO("✅ Security system started");
    return true;
}

bool SecurityManager::stop() {
    if (!running_.load()) {
        return true;
    }
    
    HFX_LOG_INFO("🛑 Stopping security system...");
    
    shutdown_requested_.store(true);
    running_.store(false);
    
    // Join worker threads
    if (session_cleanup_worker_.joinable()) {
        session_cleanup_worker_.join();
    }
    if (audit_log_writer_.joinable()) {
        audit_log_writer_.join();
    }
    if (rate_limit_refill_worker_.joinable()) {
        rate_limit_refill_worker_.join();
    }
    if (security_monitor_.joinable()) {
        security_monitor_.join();
    }
    
    HFX_LOG_INFO("✅ Security system stopped");
    return true;
}

std::string SecurityManager::create_session(const std::string& user_id, const std::string& client_ip,
                                           const std::string& user_agent, AuthMethod auth_method,
                                           SecurityLevel clearance_level) {
    if (security_lockdown_.load()) {
        throw std::runtime_error("System is in security lockdown");
    }
    
    // Check IP whitelist/blacklist
    if (!is_ip_allowed(client_ip)) {
        log_audit_event(AuditEventType::LOGIN_FAILURE, user_id, "session", "create", 
                       false, "IP address not allowed: " + client_ip, ViolationSeverity::HIGH);
        throw std::runtime_error("IP address not allowed");
    }
    
    std::string session_id = generate_session_id();
    
    UserSession session;
    session.session_id = session_id;
    session.user_id = user_id;
    session.client_ip = client_ip;
    session.user_agent = user_agent;
    session.auth_method = auth_method;
    session.clearance_level = clearance_level;
    session.created_at = std::chrono::system_clock::now();
    session.last_access = session.created_at;
    session.expires_at = session.created_at + config_.session_timeout;
    session.active.store(true);
    
    // Get user permissions
    session.permissions = permission_manager_.get_user_permissions(user_id);
    
    {
        std::lock_guard<std::mutex> lock(sessions_mutex_);
        
        // Check concurrent session limit
        uint32_t user_session_count = 0;
        for (const auto& [id, existing_session] : active_sessions_) {
            if (existing_session.user_id == user_id && existing_session.active.load()) {
                user_session_count++;
            }
        }
        
        if (user_session_count >= config_.max_concurrent_sessions_per_user) {
            throw std::runtime_error("Maximum concurrent sessions exceeded");
        }
        
        active_sessions_[session_id] = session;
    }
    
    stats_.total_sessions_created.fetch_add(1);
    stats_.active_sessions.fetch_add(1);
    
    log_audit_event(AuditEventType::LOGIN_SUCCESS, user_id, "session", "create", 
                   true, "Session created with " + security_utils::auth_method_to_string(auth_method));
    
    HFX_LOG_INFO("[LOG] Message");
    
    return session_id;
}

bool SecurityManager::validate_session(const std::string& session_id) {
    if (security_lockdown_.load()) {
        return false;
    }
    
    if (!security_utils::is_valid_session_id(session_id)) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    
    auto it = active_sessions_.find(session_id);
    if (it == active_sessions_.end()) {
        return false;
    }
    
    UserSession& session = it->second;
    
    if (!session.active.load() || is_session_expired(session)) {
        session.active.store(false);
        return false;
    }
    
    // Update last access time
    session.last_access = std::chrono::system_clock::now();
    session.request_count.fetch_add(1);
    
    return true;
}

bool SecurityManager::extend_session(const std::string& session_id) {
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    
    auto it = active_sessions_.find(session_id);
    if (it == active_sessions_.end() || !it->second.active.load()) {
        return false;
    }
    
    UserSession& session = it->second;
    auto now = std::chrono::system_clock::now();
    
    // Check if session can be extended (not exceeded max duration)
    if (now - session.created_at >= config_.max_session_duration) {
        return false;
    }
    
    session.expires_at = now + config_.session_timeout;
    session.last_access = now;
    
    log_audit_event(AuditEventType::API_ACCESS, session.user_id, "session", "extend", 
                   true, "Session extended");
    
    return true;
}

bool SecurityManager::terminate_session(const std::string& session_id) {
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    
    auto it = active_sessions_.find(session_id);
    if (it == active_sessions_.end()) {
        return false;
    }
    
    UserSession& session = it->second;
    session.active.store(false);
    
    stats_.active_sessions.fetch_sub(1);
    
    log_audit_event(AuditEventType::LOGOUT, session.user_id, "session", "terminate", 
                   true, "Session terminated");
    
    HFX_LOG_INFO("[LOG] Message");
    
    return true;
}

std::string SecurityManager::create_api_key(const std::string& user_id, const std::string& description,
                                           SecurityLevel max_level, 
                                           const std::unordered_set<std::string>& permissions,
                                           std::chrono::system_clock::time_point expires_at) {
    std::string api_key = generate_api_key();
    std::string key_hash = compute_hash(api_key);
    std::string key_id = generate_secure_token(16);
    
    APIKey key;
    key.key_id = key_id;
    key.key_hash = key_hash;
    key.user_id = user_id;
    key.description = description;
    key.max_level = max_level;
    key.permissions = permissions;
    key.created_at = std::chrono::system_clock::now();
    key.expires_at = expires_at;
    key.active.store(true);
    
    {
        std::lock_guard<std::mutex> lock(api_keys_mutex_);
        api_keys_[key_id] = std::move(key);
        key_hash_to_id_[key_hash] = key_id;
    }
    
    stats_.api_keys_created.fetch_add(1);
    
    log_audit_event(AuditEventType::KEY_GENERATION, user_id, "api_key", "create", 
                   true, "API key created: " + description);
    
    HFX_LOG_INFO("[LOG] Message");
              << " (" << description << ")" << std::endl;
    
    return api_key;
}

bool SecurityManager::validate_api_key(const std::string& api_key, const std::string& client_ip) {
    if (security_lockdown_.load()) {
        return false;
    }
    
    if (!security_utils::is_valid_api_key_format(api_key)) {
        return false;
    }
    
    // Check IP restrictions if provided
    if (!client_ip.empty() && !is_ip_allowed(client_ip)) {
        return false;
    }
    
    std::string key_hash = compute_hash(api_key);
    
    std::lock_guard<std::mutex> lock(api_keys_mutex_);
    
    auto hash_it = key_hash_to_id_.find(key_hash);
    if (hash_it == key_hash_to_id_.end()) {
        return false;
    }
    
    auto key_it = api_keys_.find(hash_it->second);
    if (key_it == api_keys_.end()) {
        return false;
    }
    
    APIKey& key = key_it->second;
    
    if (!key.active.load() || is_api_key_expired(key)) {
        return false;
    }
    
    // Check IP whitelist for this specific key
    if (!key.source_ip_whitelist.empty() && !client_ip.empty()) {
        bool ip_allowed = false;
        std::stringstream ss(key.source_ip_whitelist);
        std::string ip_range;
        
        while (std::getline(ss, ip_range, ',')) {
            if (is_ip_in_range(client_ip, ip_range)) {
                ip_allowed = true;
                break;
            }
        }
        
        if (!ip_allowed) {
            return false;
        }
    }
    
    // Update usage statistics
    key.last_used = std::chrono::system_clock::now();
    key.usage_count.fetch_add(1);
    
    return true;
}

bool SecurityManager::authorize_action(const std::string& session_id, const std::string& resource,
                                      const std::string& action, SecurityLevel required_level) {
    if (security_lockdown_.load()) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    
    auto it = active_sessions_.find(session_id);
    if (it == active_sessions_.end()) {
        return false;
    }
    
    const UserSession& session = it->second;
    
    if (!session.active.load() || is_session_expired(session)) {
        return false;
    }
    
    // Check security level
    if (session.clearance_level < required_level) {
        log_audit_event(AuditEventType::PERMISSION_DENIED, session.user_id, resource, action, 
                       false, "Insufficient security clearance", ViolationSeverity::MEDIUM);
        return false;
    }
    
    // Check specific permissions
    if (!permission_manager_.can_access_resource(session.user_id, resource, action)) {
        log_audit_event(AuditEventType::PERMISSION_DENIED, session.user_id, resource, action, 
                       false, "Permission denied", ViolationSeverity::MEDIUM);
        return false;
    }
    
    log_audit_event(AuditEventType::API_ACCESS, session.user_id, resource, action, 
                   true, "Action authorized");
    
    return true;
}

bool SecurityManager::check_rate_limit(const std::string& identifier, const std::string& endpoint) {
    if (!config_.enable_rate_limiting) {
        return true;
    }
    
    std::lock_guard<std::mutex> lock(rate_limits_mutex_);
    
    std::string bucket_key = identifier + ":" + endpoint;
    auto it = rate_limit_buckets_.find(bucket_key);
    
    if (it == rate_limit_buckets_.end()) {
        // Create new rate limit bucket
        uint32_t max_tokens = config_.per_user_requests_per_minute;
        auto refill_interval = std::chrono::minutes(1);
        
        rate_limit_buckets_[bucket_key] = std::make_unique<RateLimitBucket>(max_tokens, refill_interval);
        return true;
    }
    
    RateLimitBucket& bucket = *it->second;
    std::lock_guard<std::mutex> bucket_lock(bucket.bucket_mutex);
    
    // Refill tokens if needed
    auto now = std::chrono::steady_clock::now();
    auto time_passed = now - bucket.last_refill;
    
    if (time_passed >= bucket.refill_interval) {
        bucket.current_tokens.store(bucket.max_tokens);
        bucket.last_refill = now;
    }
    
    uint32_t current = bucket.current_tokens.load();
    if (current > 0) {
        bucket.current_tokens.fetch_sub(1);
        return true;
    }
    
    // Rate limit exceeded
    stats_.rate_limit_violations.fetch_add(1);
    
    // Extract user ID from identifier for logging
    std::string user_id = identifier;
    size_t colon_pos = identifier.find(':');
    if (colon_pos != std::string::npos) {
        user_id = identifier.substr(0, colon_pos);
    }
    
    log_audit_event(AuditEventType::RATE_LIMIT_EXCEEDED, user_id, endpoint, "access", 
                   false, "Rate limit exceeded", ViolationSeverity::MEDIUM);
    
    return false;
}

bool SecurityManager::validate_input(const std::string& input, const std::string& pattern) {
    try {
        std::regex regex_pattern(pattern);
        return std::regex_match(input, regex_pattern);
    } catch (const std::exception& e) {
        HFX_LOG_ERROR("[ERROR] Message");
        return false;
    }
}

std::string SecurityManager::sanitize_input(const std::string& input) {
    std::string sanitized = input;
    
    // Remove or escape dangerous characters
    const std::string dangerous_chars = "<>\"'&;(){}[]|`$";
    for (char c : dangerous_chars) {
        sanitized.erase(std::remove(sanitized.begin(), sanitized.end(), c), sanitized.end());
    }
    
    // Limit length
    if (sanitized.length() > 1000) {
        sanitized = sanitized.substr(0, 1000);
    }
    
    return sanitized;
}

bool SecurityManager::is_safe_filename(const std::string& filename) const {
    // Check for path traversal attempts
    if (filename.find("..") != std::string::npos) return false;
    if (filename.find("/") != std::string::npos) return false;
    if (filename.find("\\") != std::string::npos) return false;
    
    // Check file extension if configured
    if (!config_.allowed_file_extensions.empty()) {
        size_t dot_pos = filename.find_last_of('.');
        if (dot_pos == std::string::npos) return false;
        
        std::string ext = filename.substr(dot_pos);
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        
        return std::find(config_.allowed_file_extensions.begin(), 
                        config_.allowed_file_extensions.end(), ext) != 
               config_.allowed_file_extensions.end();
    }
    
    return true;
}

bool SecurityManager::is_valid_email(const std::string& email) const {
    std::regex email_regex(R"([a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,})");
    return std::regex_match(email, email_regex);
}

bool SecurityManager::is_valid_ip_address(const std::string& ip) const {
    std::regex ipv4_regex(R"(^(\d{1,3}\.){3}\d{1,3}$)");
    if (!std::regex_match(ip, ipv4_regex)) return false;
    
    // Check each octet is 0-255
    std::stringstream ss(ip);
    std::string octet;
    
    while (std::getline(ss, octet, '.')) {
        try {
            int value = std::stoi(octet);
            if (value < 0 || value > 255) return false;
        } catch (...) {
            return false;
        }
    }
    
    return true;
}

bool SecurityManager::is_ip_allowed(const std::string& ip_address) const {
    if (!is_valid_ip_address(ip_address)) return false;
    
    // Check blacklist first
    for (const auto& blocked_range : config_.blocked_ip_ranges) {
        if (is_ip_in_range(ip_address, blocked_range)) {
            return false;
        }
    }
    
    // If whitelist is enabled, check it
    if (config_.enable_ip_whitelist && !config_.allowed_ip_ranges.empty()) {
        for (const auto& allowed_range : config_.allowed_ip_ranges) {
            if (is_ip_in_range(ip_address, allowed_range)) {
                return true;
            }
        }
        return false; // Not in whitelist
    }
    
    return true; // No whitelist restriction
}

void SecurityManager::log_audit_event(AuditEventType event_type, const std::string& user_id,
                                     const std::string& resource, const std::string& action,
                                     bool success, const std::string& details,
                                     ViolationSeverity severity) {
    if (!config_.enable_audit_logging) return;
    
    AuditLogEntry entry;
    entry.entry_id = generate_audit_entry_id();
    entry.timestamp = std::chrono::system_clock::now();
    entry.event_type = event_type;
    entry.severity = severity;
    entry.user_id = user_id;
    entry.resource = resource;
    entry.action = action;
    entry.details = details;
    entry.success = success;
    
    // Try to get session and client IP
    std::lock_guard<std::mutex> sessions_lock(sessions_mutex_);
    for (const auto& [session_id, session] : active_sessions_) {
        if (session.user_id == user_id && session.active.load()) {
            entry.session_id = session_id;
            entry.client_ip = session.client_ip;
            break;
        }
    }
    
    {
        std::lock_guard<std::mutex> lock(audit_logs_mutex_);
        audit_log_queue_.push(entry);
    }
    
    stats_.audit_log_entries.fetch_add(1);
    
    // Report security violations
    if (severity >= ViolationSeverity::HIGH || !success) {
        report_violation(severity, security_utils::event_type_to_string(event_type),
                        user_id, entry.client_ip, details);
    }
}

void SecurityManager::report_violation(ViolationSeverity severity, const std::string& violation_type,
                                      const std::string& user_id, const std::string& client_ip,
                                      const std::string& description,
                                      const std::unordered_map<std::string, std::string>& details) {
    SecurityViolation violation;
    violation.violation_id = generate_violation_id();
    violation.timestamp = std::chrono::system_clock::now();
    violation.severity = severity;
    violation.violation_type = violation_type;
    violation.user_id = user_id;
    violation.client_ip = client_ip;
    violation.description = description;
    violation.details = details;
    
    {
        std::lock_guard<std::mutex> lock(violations_mutex_);
        security_violations_.push_back(violation);
    }
    
    stats_.security_violations.fetch_add(1);
    
    HFX_LOG_INFO("🚨 Security Violation [" 
              << security_utils::severity_to_string(severity) << "] " 
              << violation_type << ": " << description << std::endl;
}

std::string SecurityManager::hash_password(const std::string& password, const std::string& salt) {
    std::string actual_salt = salt.empty() ? generate_salt() : salt;
    return compute_hash(password + actual_salt);
}

bool SecurityManager::verify_password(const std::string& password, const std::string& hash) {
    // For this implementation, we assume the salt is appended to the password before hashing
    // In a real implementation, you'd store the salt separately
    return compute_hash(password) == hash;
}

std::string SecurityManager::encrypt_data(const std::string& data, const std::string& key) {
    // Mock implementation - in production, use proper AES encryption
    std::string encrypted = data;
    std::reverse(encrypted.begin(), encrypted.end());
    return "ENC:" + encrypted;
}

std::string SecurityManager::decrypt_data(const std::string& encrypted_data, const std::string& key) {
    // Mock implementation - in production, use proper AES decryption
    if (encrypted_data.starts_with("ENC:")) {
        std::string data = encrypted_data.substr(4);
        std::reverse(data.begin(), data.end());
        return data;
    }
    return encrypted_data;
}

std::string SecurityManager::generate_secure_token(size_t length) {
    return generate_random_string(length);
}

std::string SecurityManager::generate_salt(size_t length) {
    return generate_random_string(length);
}

std::unordered_map<std::string, std::string> SecurityManager::get_security_headers() const {
    std::unordered_map<std::string, std::string> headers;
    
    headers["X-Content-Type-Options"] = "nosniff";
    headers["X-Frame-Options"] = "DENY";
    headers["X-XSS-Protection"] = "1; mode=block";
    headers["Referrer-Policy"] = "strict-origin-when-cross-origin";
    headers["Content-Security-Policy"] = "default-src 'self'";
    
    if (config_.enforce_https) {
        headers["Strict-Transport-Security"] = "max-age=31536000; includeSubDomains";
    }
    
    if (config_.enable_cors && !config_.allowed_origins.empty()) {
        std::string origins;
        for (size_t i = 0; i < config_.allowed_origins.size(); ++i) {
            if (i > 0) origins += ",";
            origins += config_.allowed_origins[i];
        }
        headers["Access-Control-Allow-Origin"] = origins;
    }
    
    return headers;
}

// Trading-specific security methods
bool SecurityManager::authorize_trade(const std::string& session_id, const std::string& symbol, 
                                     double amount, const std::string& side) {
    if (!authorize_action(session_id, "trades", "create", SecurityLevel::AUTHORIZED)) {
        return false;
    }
    
    // Additional trade-specific validations
    if (amount <= 0 || amount > 1000000) { // Max $1M per trade
        log_audit_event(AuditEventType::SECURITY_VIOLATION, "", "trades", "create", 
                       false, "Invalid trade amount: " + std::to_string(amount), 
                       ViolationSeverity::HIGH);
        return false;
    }
    
    if (side != "buy" && side != "sell") {
        return false;
    }
    
    return true;
}

bool SecurityManager::authorize_withdrawal(const std::string& session_id, double amount, 
                                          const std::string& destination) {
    if (!authorize_action(session_id, "funds", "withdraw", SecurityLevel::ADMIN)) {
        return false;
    }
    
    // Additional withdrawal validations
    if (amount <= 0 || amount > 100000) { // Max $100K withdrawal
        return false;
    }
    
    // Validate destination address format (simplified)
    if (destination.length() < 20 || destination.length() > 100) {
        return false;
    }
    
    return true;
}

void SecurityManager::log_trade_execution(const std::string& user_id, const std::string& symbol,
                                         double amount, const std::string& side, bool success) {
    std::unordered_map<std::string, std::string> metadata;
    metadata["symbol"] = symbol;
    metadata["amount"] = std::to_string(amount);
    metadata["side"] = side;
    
    log_audit_event(AuditEventType::TRADE_EXECUTION, user_id, "trades", "execute", 
                   success, symbol + " " + side + " " + std::to_string(amount), 
                   ViolationSeverity::LOW);
}

void SecurityManager::trigger_security_lockdown(const std::string& reason) {
    security_lockdown_.store(true);
    
    log_audit_event(AuditEventType::SECURITY_VIOLATION, "SYSTEM", "security", "lockdown", 
                   true, "Security lockdown triggered: " + reason, ViolationSeverity::EMERGENCY);
    
    HFX_LOG_INFO("[LOG] Message");
}

void SecurityManager::emergency_revoke_all_sessions() {
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    
    size_t revoked_count = 0;
    for (auto& [session_id, session] : active_sessions_) {
        if (session.active.load()) {
            session.active.store(false);
            revoked_count++;
        }
    }
    
    stats_.active_sessions.store(0);
    
    log_audit_event(AuditEventType::SECURITY_VIOLATION, "SYSTEM", "sessions", "emergency_revoke", 
                   true, "Revoked " + std::to_string(revoked_count) + " sessions", 
                   ViolationSeverity::EMERGENCY);
    
    HFX_LOG_INFO("[LOG] Message");
}

// Worker thread implementations
void SecurityManager::session_cleanup_worker() {
    HFX_LOG_INFO("🧹 Starting session cleanup worker");
    
    while (running_.load()) {
        std::this_thread::sleep_for(std::chrono::minutes(1));
        
        if (!running_.load()) break;
        
        cleanup_expired_sessions();
    }
    
    HFX_LOG_INFO("🧹 Session cleanup worker stopped");
}

void SecurityManager::audit_log_writer_worker() {
    HFX_LOG_INFO("📝 Starting audit log writer");
    
    while (running_.load()) {
        std::queue<AuditLogEntry> entries_to_write;
        
        {
            std::lock_guard<std::mutex> lock(audit_logs_mutex_);
            entries_to_write.swap(audit_log_queue_);
        }
        
        while (!entries_to_write.empty()) {
            write_audit_log_to_file(entries_to_write.front());
            
            {
                std::lock_guard<std::mutex> lock(audit_logs_mutex_);
                audit_logs_.push_back(entries_to_write.front());
            }
            
            entries_to_write.pop();
        }
        
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    HFX_LOG_INFO("📝 Audit log writer stopped");
}

void SecurityManager::rate_limit_refill_worker() {
    HFX_LOG_INFO("⏱️  Starting rate limit refill worker");
    
    while (running_.load()) {
        std::this_thread::sleep_for(std::chrono::seconds(10));
        
        if (!running_.load()) break;
        
        std::lock_guard<std::mutex> lock(rate_limits_mutex_);
        
        auto now = std::chrono::steady_clock::now();
        
        for (auto& [key, bucket] : rate_limit_buckets_) {
            std::lock_guard<std::mutex> bucket_lock(bucket->bucket_mutex);
            
            auto time_passed = now - bucket->last_refill;
            if (time_passed >= bucket->refill_interval) {
                bucket->current_tokens.store(bucket->max_tokens);
                bucket->last_refill = now;
            }
        }
    }
    
    HFX_LOG_INFO("⏱️  Rate limit refill worker stopped");
}

void SecurityManager::security_monitor_worker() {
    HFX_LOG_INFO("👁️  Starting security monitor");
    
    while (running_.load()) {
        std::this_thread::sleep_for(std::chrono::minutes(1));
        
        if (!running_.load()) break;
        
        detect_suspicious_activity();
        check_failed_login_attempts();
    }
    
    HFX_LOG_INFO("👁️  Security monitor stopped");
}

// Helper method implementations
void SecurityManager::cleanup_expired_sessions() {
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    
    auto now = std::chrono::system_clock::now();
    size_t cleaned_count = 0;
    
    for (auto it = active_sessions_.begin(); it != active_sessions_.end();) {
        if (is_session_expired(it->second) || !it->second.active.load()) {
            if (it->second.active.load()) {
                it->second.active.store(false);
                stats_.active_sessions.fetch_sub(1);
            }
            
            // Calculate session duration for statistics
            auto duration = now - it->second.created_at;
            auto duration_minutes = std::chrono::duration_cast<std::chrono::minutes>(duration).count();
            
            double current_avg = stats_.avg_session_duration_minutes.load();
            double new_avg = (current_avg * 0.9) + (duration_minutes * 0.1);
            stats_.avg_session_duration_minutes.store(new_avg);
            
            it = active_sessions_.erase(it);
            cleaned_count++;
        } else {
            ++it;
        }
    }
    
    if (cleaned_count > 0) {
        HFX_LOG_INFO("[LOG] Message");
    }
}

std::string SecurityManager::generate_session_id() {
    return "sess_" + generate_random_string(32);
}

std::string SecurityManager::generate_api_key() {
    return "hfx_" + generate_random_string(40);
}

std::string SecurityManager::generate_violation_id() {
    return "viol_" + generate_random_string(16);
}

std::string SecurityManager::generate_audit_entry_id() {
    return "audit_" + generate_random_string(16);
}

bool SecurityManager::is_session_expired(const UserSession& session) const {
    auto now = std::chrono::system_clock::now();
    return now > session.expires_at;
}

bool SecurityManager::is_api_key_expired(const APIKey& key) const {
    auto now = std::chrono::system_clock::now();
    return now > key.expires_at;
}

bool SecurityManager::is_ip_in_range(const std::string& ip, const std::string& range) const {
    // Simplified IP range check - in production, use proper CIDR matching
    if (range == ip) return true;
    
    // Check for wildcard patterns like "192.168.*.*"
    if (range.find('*') != std::string::npos) {
        std::string pattern = std::regex_replace(range, std::regex("\\*"), "\\d+");
        return std::regex_match(ip, std::regex(pattern));
    }
    
    return false;
}

void SecurityManager::write_audit_log_to_file(const AuditLogEntry& entry) {
    if (config_.audit_log_path.empty()) return;
    
    try {
        std::ofstream log_file(config_.audit_log_path, std::ios::app);
        if (log_file.is_open()) {
            // Write in JSON format
            log_file << "{"
                     << "\"timestamp\":\"" << std::chrono::duration_cast<std::chrono::seconds>(
                         entry.timestamp.time_since_epoch()).count() << "\","
                     << "\"event_type\":\"" << security_utils::event_type_to_string(entry.event_type) << "\","
                     << "\"user_id\":\"" << entry.user_id << "\","
                     << "\"resource\":\"" << entry.resource << "\","
                     << "\"action\":\"" << entry.action << "\","
                     << "\"success\":" << (entry.success ? "true" : "false") << ","
                     << "\"details\":\"" << entry.details << "\","
                     << "\"client_ip\":\"" << entry.client_ip << "\""
                     << "}\n";
            log_file.close();
        }
    } catch (const std::exception& e) {
        HFX_LOG_ERROR("[ERROR] Message");
    }
}

void SecurityManager::detect_suspicious_activity() {
    // Implement suspicious activity detection algorithms
    // This is a simplified version
    
    auto now = std::chrono::system_clock::now();
    auto window_start = now - std::chrono::minutes(15);
    
    std::unordered_map<std::string, uint32_t> user_failed_attempts;
    std::unordered_map<std::string, uint32_t> ip_failed_attempts;
    
    {
        std::lock_guard<std::mutex> lock(audit_logs_mutex_);
        
        for (const auto& entry : audit_logs_) {
            if (entry.timestamp >= window_start && 
                entry.event_type == AuditEventType::LOGIN_FAILURE) {
                user_failed_attempts[entry.user_id]++;
                ip_failed_attempts[entry.client_ip]++;
            }
        }
    }
    
    // Report suspicious patterns
    for (const auto& [user_id, count] : user_failed_attempts) {
        if (count >= config_.suspicious_activity_threshold) {
            report_violation(ViolationSeverity::HIGH, "SUSPICIOUS_LOGIN_PATTERN", 
                           user_id, "", 
                           std::to_string(count) + " failed login attempts in 15 minutes");
        }
    }
    
    for (const auto& [ip, count] : ip_failed_attempts) {
        if (count >= config_.suspicious_activity_threshold) {
            report_violation(ViolationSeverity::HIGH, "SUSPICIOUS_IP_ACTIVITY", 
                           "", ip, 
                           std::to_string(count) + " failed login attempts from IP in 15 minutes");
        }
    }
}

void SecurityManager::check_failed_login_attempts() {
    // This would implement account lockout logic
    // For now, just log patterns
    
    auto now = std::chrono::system_clock::now();
    auto window_start = now - config_.failed_login_window;
    
    std::unordered_map<std::string, uint32_t> failed_attempts;
    
    {
        std::lock_guard<std::mutex> lock(audit_logs_mutex_);
        
        for (const auto& entry : audit_logs_) {
            if (entry.timestamp >= window_start && 
                entry.event_type == AuditEventType::LOGIN_FAILURE) {
                failed_attempts[entry.user_id]++;
            }
        }
    }
    
    for (const auto& [user_id, count] : failed_attempts) {
        if (count >= config_.failed_login_threshold) {
            HFX_LOG_INFO("[LOG] Message");
                      << " failed login attempts in the last " 
                      << config_.failed_login_window.count() << " minutes" << std::endl;
        }
    }
}

std::string SecurityManager::compute_hash(const std::string& data, const std::string& salt) {
    // Simplified hash implementation using EVP interface
    // In production, use proper key stretching (PBKDF2, bcrypt, Argon2)
    
    std::string to_hash = data + salt;
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hash_len = 0;
    
    EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
    if (mdctx == nullptr) {
        return "";
    }
    
    if (EVP_DigestInit_ex(mdctx, EVP_sha256(), nullptr) != 1) {
        EVP_MD_CTX_free(mdctx);
        return "";
    }
    
    if (EVP_DigestUpdate(mdctx, to_hash.c_str(), to_hash.length()) != 1) {
        EVP_MD_CTX_free(mdctx);
        return "";
    }
    
    if (EVP_DigestFinal_ex(mdctx, hash, &hash_len) != 1) {
        EVP_MD_CTX_free(mdctx);
        return "";
    }
    
    EVP_MD_CTX_free(mdctx);
    
    std::stringstream ss;
    for (unsigned int i = 0; i < hash_len; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    
    return ss.str();
}

std::string SecurityManager::generate_random_string(size_t length) {
    const char charset[] = "0123456789abcdef";
    std::string result;
    result.reserve(length);
    
    unsigned char buffer[256];
    if (RAND_bytes(buffer, sizeof(buffer)) != 1) {
        // Fallback to standard random
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, sizeof(charset) - 2);
        
        for (size_t i = 0; i < length; ++i) {
            result += charset[dis(gen)];
        }
    } else {
        for (size_t i = 0; i < length; ++i) {
            result += charset[buffer[i % sizeof(buffer)] % (sizeof(charset) - 1)];
        }
    }
    
    return result;
}

void SecurityManager::reset_stats() {
    stats_.total_sessions_created.store(0);
    stats_.active_sessions.store(0);
    stats_.failed_authentications.store(0);
    stats_.successful_authentications.store(0);
    stats_.rate_limit_violations.store(0);
    stats_.security_violations.store(0);
    stats_.audit_log_entries.store(0);
    stats_.api_keys_created.store(0);
    stats_.api_keys_revoked.store(0);
    stats_.avg_session_duration_minutes.store(0.0);
    
    HFX_LOG_INFO("🔒 Security statistics reset");
}

// Factory implementations
std::unique_ptr<SecurityManager> SecurityManagerFactory::create_production_security() {
    SecurityConfig config;
    config.session_timeout = std::chrono::minutes(15);
    config.require_strong_passwords = true;
    config.require_mfa = true;
    config.enable_rate_limiting = true;
    config.enable_ip_whitelist = true;
    config.enable_audit_logging = true;
    config.encrypt_audit_logs = true;
    config.enforce_https = true;
    config.encrypt_sensitive_data = true;
    config.enable_security_monitoring = true;
    config.failed_login_threshold = 3;
    config.per_user_requests_per_minute = 60;
    
    auto manager = std::make_unique<SecurityManager>(config);
    return manager;
}

std::unique_ptr<SecurityManager> SecurityManagerFactory::create_trading_security() {
    SecurityConfig config;
    config.session_timeout = std::chrono::minutes(30);
    config.max_session_duration = std::chrono::hours(8);
    config.require_strong_passwords = true;
    config.require_mfa = true;
    config.enable_rate_limiting = true;
    config.per_user_requests_per_minute = 1000; // Higher for trading
    config.enable_audit_logging = true;
    config.encrypt_audit_logs = true;
    config.enforce_https = true;
    config.encrypt_sensitive_data = true;
    config.enable_security_monitoring = true;
    config.failed_login_threshold = 5;
    config.suspicious_activity_threshold = 10;
    
    return std::make_unique<SecurityManager>(config);
}

std::unique_ptr<SecurityManager> SecurityManagerFactory::create_development_security() {
    SecurityConfig config;
    config.session_timeout = std::chrono::hours(8);
    config.require_strong_passwords = false;
    config.require_mfa = false;
    config.enable_rate_limiting = false;
    config.enable_ip_whitelist = false;
    config.enable_audit_logging = true;
    config.encrypt_audit_logs = false;
    config.enforce_https = false;
    config.encrypt_sensitive_data = false;
    config.enable_security_monitoring = false;
    
    return std::make_unique<SecurityManager>(config);
}

} // namespace hfx::ultra
