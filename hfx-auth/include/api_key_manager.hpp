/**
 * @file api_key_manager.hpp
 * @brief API Key management system
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
#include <atomic>

namespace hfx {
namespace auth {

struct APIKeyConfig {
    std::chrono::days default_expiration_days = std::chrono::days(365);
    size_t max_keys_per_user = 10;
    size_t key_length = 32;
    std::string key_prefix = "hfx_";
    bool allow_unlimited_keys = false;
    bool track_usage_stats = true;
    std::chrono::hours cleanup_interval = std::chrono::hours(24);
};

/**
 * @brief API Key Manager for handling API key operations
 */
class APIKeyManager {
public:
    explicit APIKeyManager(const APIKeyConfig& config = APIKeyConfig{});
    ~APIKeyManager();

    // API Key CRUD operations
    std::string create_api_key(const std::string& user_id, const std::string& name,
                              const std::string& permissions = "{}");
    std::optional<APIKey> get_api_key(const std::string& key_id);
    std::optional<APIKey> get_api_key_by_hash(const std::string& key_hash);
    bool update_api_key(const std::string& key_id, const APIKey& updated_key);
    bool delete_api_key(const std::string& key_id);
    bool revoke_api_key(const std::string& key_id);
    bool activate_api_key(const std::string& key_id);

    // User API key management
    std::vector<APIKey> get_user_api_keys(const std::string& user_id);
    bool revoke_user_api_keys(const std::string& user_id);
    size_t get_user_api_key_count(const std::string& user_id);

    // API key validation and usage
    bool validate_api_key(const std::string& api_key_string);
    bool validate_api_key_permissions(const std::string& key_id, const std::string& permission);
    std::optional<std::string> get_user_id_from_api_key(const std::string& api_key_string);
    
    // Usage tracking
    bool record_api_key_usage(const std::string& key_id);
    bool update_last_used(const std::string& key_id);
    
    // Key management
    void cleanup_expired_keys();
    void rotate_api_key(const std::string& key_id);
    bool extend_expiration(const std::string& key_id, std::chrono::days additional_days);
    
    // Search and filtering
    std::vector<APIKey> find_keys_by_user(const std::string& user_id);
    std::vector<APIKey> find_expired_keys();
    std::vector<APIKey> find_unused_keys(std::chrono::days unused_days = std::chrono::days(90));
    
    // Statistics
    struct APIKeyStats {
        size_t total_keys{0};
        size_t active_keys{0};
        size_t expired_keys{0};
        size_t revoked_keys{0};
        std::atomic<uint64_t> total_requests{0};
        std::atomic<uint64_t> successful_validations{0};
        std::atomic<uint64_t> failed_validations{0};
        std::chrono::system_clock::time_point last_cleanup;
        double avg_keys_per_user{0.0};
    };
    
    APIKeyStats get_api_key_stats() const;

    // Configuration
    void update_config(const APIKeyConfig& config);
    const APIKeyConfig& get_config() const;

private:
    APIKeyConfig config_;
    mutable std::mutex keys_mutex_;
    std::unordered_map<std::string, APIKey> api_keys_;               // key_id -> APIKey
    std::unordered_map<std::string, std::string> key_hash_to_id_;    // key_hash -> key_id
    std::unordered_multimap<std::string, std::string> user_keys_;    // user_id -> key_id
    
    // Statistics
    mutable APIKeyStats stats_;
    
    // Helper methods
    std::string generate_api_key_string();
    std::string generate_key_id();
    std::string hash_api_key(const std::string& api_key_string);
    bool is_key_valid(const APIKey& key) const;
    void update_indexes(const APIKey& key);
    void remove_from_indexes(const std::string& key_id);
    void update_statistics();
    
    // Permission helpers
    bool check_permission_json(const std::string& permissions_json, const std::string& permission);
    std::vector<std::string> parse_permissions(const std::string& permissions_json);
};

} // namespace auth
} // namespace hfx