/**
 * @file user_manager.cpp
 * @brief User manager implementation
 */

#include "../include/user_manager.hpp"
#include "../../hfx-log/include/simple_logger.hpp"
#include <algorithm>

namespace hfx {
namespace auth {

UserManager::UserManager() {
    initialize_default_users();
    HFX_LOG_INFO("[UserManager] Initialized with default users");
}

UserManager::~UserManager() = default;

bool UserManager::create_user(const User& user) {
    std::lock_guard<std::mutex> lock(users_mutex_);
    
    if (user_exists(user.user_id) || username_exists(user.username) || email_exists(user.email)) {
        HFX_LOG_WARN("[UserManager] User creation failed - user/username/email already exists");
        return false;
    }
    
    users_[user.user_id] = user;
    update_indexes(user);
    
    HFX_LOG_INFO("[UserManager] Created user: " + user.username + " with role: " + user_role_to_string(user.role));
    return true;
}

std::optional<User> UserManager::get_user(const std::string& user_id) {
    std::lock_guard<std::mutex> lock(users_mutex_);
    
    auto it = users_.find(user_id);
    if (it != users_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::optional<User> UserManager::get_user_by_username(const std::string& username) {
    std::lock_guard<std::mutex> lock(users_mutex_);
    
    auto it = username_to_id_.find(username);
    if (it != username_to_id_.end()) {
        auto user_it = users_.find(it->second);
        if (user_it != users_.end()) {
            return user_it->second;
        }
    }
    return std::nullopt;
}

std::optional<User> UserManager::get_user_by_email(const std::string& email) {
    std::lock_guard<std::mutex> lock(users_mutex_);
    
    auto it = email_to_id_.find(email);
    if (it != email_to_id_.end()) {
        auto user_it = users_.find(it->second);
        if (user_it != users_.end()) {
            return user_it->second;
        }
    }
    return std::nullopt;
}

bool UserManager::update_user(const std::string& user_id, const User& updated_user) {
    std::lock_guard<std::mutex> lock(users_mutex_);
    
    auto it = users_.find(user_id);
    if (it == users_.end()) {
        return false;
    }
    
    // Remove old indexes
    remove_from_indexes(user_id);
    
    // Update user
    users_[user_id] = updated_user;
    users_[user_id].user_id = user_id;  // Ensure user_id remains unchanged
    
    // Update indexes
    update_indexes(updated_user);
    
    HFX_LOG_INFO("[UserManager] Updated user: " + updated_user.username);
    return true;
}

bool UserManager::delete_user(const std::string& user_id) {
    std::lock_guard<std::mutex> lock(users_mutex_);
    
    auto it = users_.find(user_id);
    if (it == users_.end()) {
        return false;
    }
    
    std::string username = it->second.username;
    remove_from_indexes(user_id);
    users_.erase(it);
    
    HFX_LOG_INFO("[UserManager] Deleted user: " + username);
    return true;
}

std::vector<User> UserManager::get_all_users() {
    std::lock_guard<std::mutex> lock(users_mutex_);
    
    std::vector<User> result;
    result.reserve(users_.size());
    
    for (const auto& pair : users_) {
        result.push_back(pair.second);
    }
    
    return result;
}

bool UserManager::activate_user(const std::string& user_id) {
    std::lock_guard<std::mutex> lock(users_mutex_);
    
    auto it = users_.find(user_id);
    if (it == users_.end()) {
        return false;
    }
    
    it->second.is_active = true;
    HFX_LOG_INFO("[UserManager] Activated user: " + it->second.username);
    return true;
}

bool UserManager::deactivate_user(const std::string& user_id) {
    std::lock_guard<std::mutex> lock(users_mutex_);
    
    auto it = users_.find(user_id);
    if (it == users_.end()) {
        return false;
    }
    
    it->second.is_active = false;
    HFX_LOG_INFO("[UserManager] Deactivated user: " + it->second.username);
    return true;
}

bool UserManager::lock_user(const std::string& user_id, std::chrono::minutes duration) {
    std::lock_guard<std::mutex> lock(users_mutex_);
    
    auto it = users_.find(user_id);
    if (it == users_.end()) {
        return false;
    }
    
    it->second.is_locked = true;
    it->second.lockout_until = std::chrono::system_clock::now() + duration;
    
    HFX_LOG_WARN("[UserManager] Locked user: " + it->second.username + " for " + std::to_string(duration.count()) + " minutes");
    return true;
}

bool UserManager::unlock_user(const std::string& user_id) {
    std::lock_guard<std::mutex> lock(users_mutex_);
    
    auto it = users_.find(user_id);
    if (it == users_.end()) {
        return false;
    }
    
    it->second.is_locked = false;
    it->second.lockout_until = std::chrono::system_clock::time_point{};
    it->second.failed_login_attempts = 0;
    
    HFX_LOG_INFO("[UserManager] Unlocked user: " + it->second.username);
    return true;
}

bool UserManager::update_password_hash(const std::string& user_id, const std::string& password_hash) {
    std::lock_guard<std::mutex> lock(users_mutex_);
    
    auto it = users_.find(user_id);
    if (it == users_.end()) {
        return false;
    }
    
    it->second.password_hash = password_hash;
    it->second.password_changed_at = std::chrono::system_clock::now();
    return true;
}

bool UserManager::update_last_login(const std::string& user_id) {
    std::lock_guard<std::mutex> lock(users_mutex_);
    
    auto it = users_.find(user_id);
    if (it == users_.end()) {
        return false;
    }
    
    it->second.last_login = std::chrono::system_clock::now();
    it->second.failed_login_attempts = 0;  // Reset failed attempts on successful login
    return true;
}

bool UserManager::increment_failed_attempts(const std::string& user_id) {
    std::lock_guard<std::mutex> lock(users_mutex_);
    
    auto it = users_.find(user_id);
    if (it == users_.end()) {
        return false;
    }
    
    it->second.failed_login_attempts++;
    
    // Auto-lock if too many failed attempts
    if (it->second.failed_login_attempts >= 5) {
        it->second.is_locked = true;
        it->second.lockout_until = std::chrono::system_clock::now() + std::chrono::minutes(15);
        HFX_LOG_WARN("[UserManager] Auto-locked user due to failed attempts: " + it->second.username);
    }
    
    return true;
}

bool UserManager::reset_failed_attempts(const std::string& user_id) {
    std::lock_guard<std::mutex> lock(users_mutex_);
    
    auto it = users_.find(user_id);
    if (it == users_.end()) {
        return false;
    }
    
    it->second.failed_login_attempts = 0;
    return true;
}

std::vector<User> UserManager::find_users_by_role(UserRole role) {
    std::lock_guard<std::mutex> lock(users_mutex_);
    
    std::vector<User> result;
    for (const auto& pair : users_) {
        if (pair.second.role == role) {
            result.push_back(pair.second);
        }
    }
    
    return result;
}

std::vector<User> UserManager::find_active_users() {
    std::lock_guard<std::mutex> lock(users_mutex_);
    
    std::vector<User> result;
    for (const auto& pair : users_) {
        if (pair.second.is_active) {
            result.push_back(pair.second);
        }
    }
    
    return result;
}

std::vector<User> UserManager::find_locked_users() {
    std::lock_guard<std::mutex> lock(users_mutex_);
    
    std::vector<User> result;
    for (const auto& pair : users_) {
        if (pair.second.is_locked) {
            result.push_back(pair.second);
        }
    }
    
    return result;
}

UserManager::UserStats UserManager::get_user_stats() const {
    std::lock_guard<std::mutex> lock(users_mutex_);
    
    UserStats stats;
    stats.total_users = users_.size();
    stats.last_updated = std::chrono::system_clock::now();
    
    for (const auto& pair : users_) {
        const User& user = pair.second;
        
        if (user.is_active) {
            stats.active_users++;
        }
        
        if (user.is_locked) {
            stats.locked_users++;
        }
        
        // Count by role
        switch (user.role) {
            case UserRole::ADMIN:
                stats.users_by_role[0]++;
                break;
            case UserRole::TRADER:
                stats.users_by_role[1]++;
                break;
            case UserRole::ANALYST:
                stats.users_by_role[2]++;
                break;
            case UserRole::VIEWER:
                stats.users_by_role[3]++;
                break;
            case UserRole::API_USER:
                stats.users_by_role[4]++;
                break;
        }
    }
    
    return stats;
}

void UserManager::cleanup_inactive_users(std::chrono::days inactive_days) {
    std::lock_guard<std::mutex> lock(users_mutex_);
    
    auto cutoff_time = std::chrono::system_clock::now() - inactive_days;
    
    std::vector<std::string> users_to_remove;
    for (const auto& pair : users_) {
        if (pair.second.last_login < cutoff_time && pair.second.role != UserRole::ADMIN) {
            users_to_remove.push_back(pair.first);
        }
    }
    
    for (const auto& user_id : users_to_remove) {
        auto it = users_.find(user_id);
        if (it != users_.end()) {
            HFX_LOG_INFO("[UserManager] Cleaning up inactive user: " + it->second.username);
            remove_from_indexes(user_id);
            users_.erase(it);
        }
    }
}

void UserManager::unlock_expired_lockouts() {
    std::lock_guard<std::mutex> lock(users_mutex_);
    
    auto now = std::chrono::system_clock::now();
    
    for (auto& pair : users_) {
        User& user = pair.second;
        if (user.is_locked && now > user.lockout_until) {
            user.is_locked = false;
            user.lockout_until = std::chrono::system_clock::time_point{};
            user.failed_login_attempts = 0;
            HFX_LOG_INFO("[UserManager] Auto-unlocked expired lockout for user: " + user.username);
        }
    }
}

// Private helper methods
bool UserManager::user_exists(const std::string& user_id) const {
    return users_.find(user_id) != users_.end();
}

bool UserManager::username_exists(const std::string& username) const {
    return username_to_id_.find(username) != username_to_id_.end();
}

bool UserManager::email_exists(const std::string& email) const {
    return email_to_id_.find(email) != email_to_id_.end();
}

void UserManager::update_indexes(const User& user) {
    username_to_id_[user.username] = user.user_id;
    email_to_id_[user.email] = user.user_id;
}

void UserManager::remove_from_indexes(const std::string& user_id) {
    auto it = users_.find(user_id);
    if (it != users_.end()) {
        username_to_id_.erase(it->second.username);
        email_to_id_.erase(it->second.email);
    }
}

void UserManager::initialize_default_users() {
    // Create default admin user
    User admin;
    admin.user_id = "admin-001";
    admin.username = "admin";
    admin.email = "admin@hydraflow.com";
    admin.password_hash = "hashed_admin_password_123"; // This should be properly hashed
    admin.role = UserRole::ADMIN;
    admin.is_active = true;
    admin.is_locked = false;
    admin.created_at = std::chrono::system_clock::now();
    admin.failed_login_attempts = 0;
    
    users_[admin.user_id] = admin;
    update_indexes(admin);
    
    // Create default trader user
    User trader;
    trader.user_id = "trader-001";
    trader.username = "trader";
    trader.email = "trader@hydraflow.com";
    trader.password_hash = "hashed_trader_password_123"; // This should be properly hashed
    trader.role = UserRole::TRADER;
    trader.is_active = true;
    trader.is_locked = false;
    trader.created_at = std::chrono::system_clock::now();
    trader.failed_login_attempts = 0;
    
    users_[trader.user_id] = trader;
    update_indexes(trader);
}

} // namespace auth
} // namespace hfx
