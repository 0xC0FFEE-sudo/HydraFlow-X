/**
 * @file user_manager.hpp
 * @brief User management system for authentication
 * @version 1.0.0
 */

#pragma once

#include "auth_manager.hpp"
#include <unordered_map>
#include <mutex>
#include <vector>
#include <optional>
#include <string>
#include <chrono>

namespace hfx {
namespace auth {

/**
 * @brief User Manager for handling user operations
 */
class UserManager {
public:
    UserManager();
    ~UserManager();

    // User CRUD operations
    bool create_user(const User& user);
    std::optional<User> get_user(const std::string& user_id);
    std::optional<User> get_user_by_username(const std::string& username);
    std::optional<User> get_user_by_email(const std::string& email);
    bool update_user(const std::string& user_id, const User& updated_user);
    bool delete_user(const std::string& user_id);
    std::vector<User> get_all_users();

    // User status management  
    bool activate_user(const std::string& user_id);
    bool deactivate_user(const std::string& user_id);
    bool lock_user(const std::string& user_id, std::chrono::minutes duration = std::chrono::minutes(15));
    bool unlock_user(const std::string& user_id);

    // Password management
    bool update_password_hash(const std::string& user_id, const std::string& password_hash);
    bool update_last_login(const std::string& user_id);
    bool increment_failed_attempts(const std::string& user_id);
    bool reset_failed_attempts(const std::string& user_id);

    // User search and filtering
    std::vector<User> find_users_by_role(UserRole role);
    std::vector<User> find_active_users();
    std::vector<User> find_locked_users();
    
    // Statistics
    struct UserStats {
        size_t total_users{0};
        size_t active_users{0};
        size_t locked_users{0};
        size_t users_by_role[5]{0}; // ADMIN, TRADER, ANALYST, VIEWER, API_USER
        std::chrono::system_clock::time_point last_updated;
    };
    
    UserStats get_user_stats() const;

    // Cleanup operations
    void cleanup_inactive_users(std::chrono::days inactive_days = std::chrono::days(365));
    void unlock_expired_lockouts();

private:
    mutable std::mutex users_mutex_;
    std::unordered_map<std::string, User> users_;           // user_id -> User
    std::unordered_map<std::string, std::string> username_to_id_; // username -> user_id
    std::unordered_map<std::string, std::string> email_to_id_;    // email -> user_id
    
    // Helper methods
    bool user_exists(const std::string& user_id) const;
    bool username_exists(const std::string& username) const;
    bool email_exists(const std::string& email) const;
    void update_indexes(const User& user);
    void remove_from_indexes(const std::string& user_id);
    void initialize_default_users();
};

} // namespace auth
} // namespace hfx
