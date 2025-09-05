/**
 * @file api_key_manager.cpp
 * @brief API Key manager implementation
 */

#include "../include/api_key_manager.hpp"
#include "../../hfx-log/include/simple_logger.hpp"
#include <random>
#include <algorithm>
#include <openssl/sha.h>
#include <sstream>
#include <iomanip>
// #include <nlohmann/json.hpp>  // TODO: Add proper JSON library or implement simple JSON parsing

namespace hfx {
namespace auth {

APIKeyManager::APIKeyManager(const APIKeyConfig& config) : config_(config) {
    stats_.last_cleanup = std::chrono::system_clock::now();
    HFX_LOG_INFO("[APIKeyManager] Initialized with " + std::to_string(config_.max_keys_per_user) + " max keys per user");
}

APIKeyManager::~APIKeyManager() = default;

std::string APIKeyManager::create_api_key(const std::string& user_id, const std::string& name,
                                        const std::string& permissions) {
    std::lock_guard<std::mutex> lock(keys_mutex_);
    
    // Check user key limit
    if (!config_.allow_unlimited_keys && get_user_api_key_count(user_id) >= config_.max_keys_per_user) {
        HFX_LOG_WARN("[APIKeyManager] API key limit exceeded for user: " + user_id);
        return "";
    }
    
    // Generate API key
    std::string api_key_string = generate_api_key_string();
    std::string key_hash = hash_api_key(api_key_string);

    APIKey new_key;
    new_key.key_id = generate_key_id();
    new_key.user_id = user_id;
    new_key.name = name;
    new_key.key_hash = key_hash;
    new_key.permissions = permissions;
    new_key.is_active = true;
    new_key.created_at = std::chrono::system_clock::now();
    new_key.expires_at = new_key.created_at + config_.default_expiration_days;
    new_key.usage_count = 0;

    // Store API key
    api_keys_[new_key.key_id] = new_key;
    update_indexes(new_key);
    
    // Update statistics
    stats_.total_keys++;
    stats_.active_keys++;
    
    HFX_LOG_INFO("[APIKeyManager] Created API key '" + name + "' for user: " + user_id);
    return api_key_string;
}

std::optional<APIKey> APIKeyManager::get_api_key(const std::string& key_id) {
    std::lock_guard<std::mutex> lock(keys_mutex_);
    
    auto it = api_keys_.find(key_id);
    if (it != api_keys_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::optional<APIKey> APIKeyManager::get_api_key_by_hash(const std::string& key_hash) {
    std::lock_guard<std::mutex> lock(keys_mutex_);

    auto hash_it = key_hash_to_id_.find(key_hash);
    if (hash_it != key_hash_to_id_.end()) {
        auto key_it = api_keys_.find(hash_it->second);
        if (key_it != api_keys_.end()) {
            return key_it->second;
        }
    }
    return std::nullopt;
}

bool APIKeyManager::update_api_key(const std::string& key_id, const APIKey& updated_key) {
    std::lock_guard<std::mutex> lock(keys_mutex_);
    
    auto it = api_keys_.find(key_id);
    if (it == api_keys_.end()) {
        return false;
    }

    // Remove old indexes
    remove_from_indexes(key_id);
    
    // Update key (preserve key_id and key_hash)
    APIKey new_key = updated_key;
    new_key.key_id = key_id;
    new_key.key_hash = it->second.key_hash;  // Preserve original hash
    
    api_keys_[key_id] = new_key;
    update_indexes(new_key);
    
    HFX_LOG_INFO("[APIKeyManager] Updated API key: " + updated_key.name);
    return true;
}

bool APIKeyManager::delete_api_key(const std::string& key_id) {
    std::lock_guard<std::mutex> lock(keys_mutex_);
    
    auto it = api_keys_.find(key_id);
    if (it == api_keys_.end()) {
        return false;
    }

    std::string key_name = it->second.name;
    std::string user_id = it->second.user_id;
    
    remove_from_indexes(key_id);
    api_keys_.erase(it);
    
    // Update statistics
    if (stats_.total_keys > 0) stats_.total_keys--;
    if (stats_.active_keys > 0) stats_.active_keys--;
    
    HFX_LOG_INFO("[APIKeyManager] Deleted API key '" + key_name + "' for user: " + user_id);
    return true;
}

bool APIKeyManager::revoke_api_key(const std::string& key_id) {
    std::lock_guard<std::mutex> lock(keys_mutex_);
    
    auto it = api_keys_.find(key_id);
    if (it == api_keys_.end()) {
        return false;
    }
    
    it->second.is_active = false;
    stats_.revoked_keys++;
    if (stats_.active_keys > 0) stats_.active_keys--;
    
    HFX_LOG_INFO("[APIKeyManager] Revoked API key: " + it->second.name);
    return true;
}

bool APIKeyManager::activate_api_key(const std::string& key_id) {
    std::lock_guard<std::mutex> lock(keys_mutex_);
    
    auto it = api_keys_.find(key_id);
    if (it == api_keys_.end()) {
        return false;
    }
    
    if (!it->second.is_active && is_key_valid(it->second)) {
        it->second.is_active = true;
        stats_.active_keys++;
        if (stats_.revoked_keys > 0) stats_.revoked_keys--;
        
        HFX_LOG_INFO("[APIKeyManager] Activated API key: " + it->second.name);
        return true;
    }
    
    return false;
}

std::vector<APIKey> APIKeyManager::get_user_api_keys(const std::string& user_id) {
    std::lock_guard<std::mutex> lock(keys_mutex_);
    
    std::vector<APIKey> result;
    auto range = user_keys_.equal_range(user_id);
    
    for (auto it = range.first; it != range.second; ++it) {
        auto key_it = api_keys_.find(it->second);
        if (key_it != api_keys_.end()) {
            result.push_back(key_it->second);
        }
    }
    
    return result;
}

bool APIKeyManager::revoke_user_api_keys(const std::string& user_id) {
    std::lock_guard<std::mutex> lock(keys_mutex_);
    
    auto range = user_keys_.equal_range(user_id);
    int revoked_count = 0;
    
    for (auto it = range.first; it != range.second; ++it) {
        auto key_it = api_keys_.find(it->second);
        if (key_it != api_keys_.end() && key_it->second.is_active) {
            key_it->second.is_active = false;
            revoked_count++;
        }
    }
    
    stats_.revoked_keys += revoked_count;
    stats_.active_keys -= revoked_count;
    
    HFX_LOG_INFO("[APIKeyManager] Revoked " + std::to_string(revoked_count) + " API keys for user: " + user_id);
    return revoked_count > 0;
}

size_t APIKeyManager::get_user_api_key_count(const std::string& user_id) {
    auto range = user_keys_.equal_range(user_id);
    size_t count = 0;
    
    for (auto it = range.first; it != range.second; ++it) {
        auto key_it = api_keys_.find(it->second);
        if (key_it != api_keys_.end() && key_it->second.is_active && is_key_valid(key_it->second)) {
            count++;
        }
    }
    
    return count;
}

bool APIKeyManager::validate_api_key(const std::string& api_key_string) {
    std::lock_guard<std::mutex> lock(keys_mutex_);
    
    if (api_key_string.empty() || !api_key_string.starts_with(config_.key_prefix)) {
        stats_.failed_validations.fetch_add(1, std::memory_order_relaxed);
        return false;
    }
    
    std::string key_hash = hash_api_key(api_key_string);
    auto key_opt = get_api_key_by_hash(key_hash);
    
    if (!key_opt || !is_key_valid(*key_opt)) {
        stats_.failed_validations.fetch_add(1, std::memory_order_relaxed);
        return false;
    }
    
    // Update usage statistics
    if (config_.track_usage_stats) {
        auto& key = api_keys_[key_opt->key_id];
        key.usage_count++;
        key.last_used = std::chrono::system_clock::now();
    }
    
    stats_.successful_validations.fetch_add(1, std::memory_order_relaxed);
    stats_.total_requests.fetch_add(1, std::memory_order_relaxed);
    
    return true;
}

bool APIKeyManager::validate_api_key_permissions(const std::string& key_id, const std::string& permission) {
    std::lock_guard<std::mutex> lock(keys_mutex_);
    
    auto it = api_keys_.find(key_id);
    if (it == api_keys_.end() || !is_key_valid(it->second)) {
        return false;
    }
    
    return check_permission_json(it->second.permissions, permission);
}

std::optional<std::string> APIKeyManager::get_user_id_from_api_key(const std::string& api_key_string) {
    std::lock_guard<std::mutex> lock(keys_mutex_);
    
    std::string key_hash = hash_api_key(api_key_string);
    auto key_opt = get_api_key_by_hash(key_hash);
    
    if (key_opt && is_key_valid(*key_opt)) {
        return key_opt->user_id;
    }
    
    return std::nullopt;
}

bool APIKeyManager::record_api_key_usage(const std::string& key_id) {
    std::lock_guard<std::mutex> lock(keys_mutex_);

    auto it = api_keys_.find(key_id);
    if (it != api_keys_.end()) {
        it->second.usage_count++;
        it->second.last_used = std::chrono::system_clock::now();
        stats_.total_requests.fetch_add(1, std::memory_order_relaxed);
        return true;
    }

    return false;
}

bool APIKeyManager::update_last_used(const std::string& key_id) {
    std::lock_guard<std::mutex> lock(keys_mutex_);

    auto it = api_keys_.find(key_id);
    if (it != api_keys_.end()) {
        it->second.last_used = std::chrono::system_clock::now();
        return true;
    }

    return false;
}

void APIKeyManager::cleanup_expired_keys() {
    std::lock_guard<std::mutex> lock(keys_mutex_);
    
    auto now = std::chrono::system_clock::now();
    std::vector<std::string> expired_keys;

    // Find expired keys
    for (const auto& pair : api_keys_) {
        if (now > pair.second.expires_at) {
            expired_keys.push_back(pair.first);
        }
    }
    
    // Remove expired keys
    for (const auto& key_id : expired_keys) {
        auto it = api_keys_.find(key_id);
        if (it != api_keys_.end()) {
            remove_from_indexes(key_id);
            api_keys_.erase(it);
            stats_.expired_keys++;
            if (stats_.total_keys > 0) stats_.total_keys--;
            if (stats_.active_keys > 0) stats_.active_keys--;
        }
    }
    
    stats_.last_cleanup = now;
    
    if (!expired_keys.empty()) {
        HFX_LOG_INFO("[APIKeyManager] Cleaned up " + std::to_string(expired_keys.size()) + " expired API keys");
    }
}

void APIKeyManager::rotate_api_key(const std::string& key_id) {
    std::lock_guard<std::mutex> lock(keys_mutex_);
    
    auto it = api_keys_.find(key_id);
    if (it == api_keys_.end()) {
        return;
    }
    
    // Generate new API key string and hash
    std::string new_api_key_string = generate_api_key_string();
    std::string new_key_hash = hash_api_key(new_api_key_string);
    
    // Update the key hash in indexes
    key_hash_to_id_.erase(it->second.key_hash);
    key_hash_to_id_[new_key_hash] = key_id;
    
    // Update the key
    it->second.key_hash = new_key_hash;
    it->second.created_at = std::chrono::system_clock::now();
    it->second.expires_at = it->second.created_at + config_.default_expiration_days;
    
    HFX_LOG_INFO("[APIKeyManager] Rotated API key: " + it->second.name);
}

bool APIKeyManager::extend_expiration(const std::string& key_id, std::chrono::days additional_days) {
    std::lock_guard<std::mutex> lock(keys_mutex_);

    auto it = api_keys_.find(key_id);
    if (it == api_keys_.end()) {
        return false;
    }
    
    it->second.expires_at += additional_days;
    HFX_LOG_INFO("[APIKeyManager] Extended expiration for API key: " + it->second.name + " by " + std::to_string(additional_days.count()) + " days");
    return true;
}

std::vector<APIKey> APIKeyManager::find_keys_by_user(const std::string& user_id) {
    return get_user_api_keys(user_id);
}

std::vector<APIKey> APIKeyManager::find_expired_keys() {
    std::lock_guard<std::mutex> lock(keys_mutex_);
    
    auto now = std::chrono::system_clock::now();
    std::vector<APIKey> result;

    for (const auto& pair : api_keys_) {
        if (now > pair.second.expires_at) {
            result.push_back(pair.second);
        }
    }

    return result;
}

std::vector<APIKey> APIKeyManager::find_unused_keys(std::chrono::days unused_days) {
    std::lock_guard<std::mutex> lock(keys_mutex_);

    auto cutoff_time = std::chrono::system_clock::now() - unused_days;
    std::vector<APIKey> result;
    
    for (const auto& pair : api_keys_) {
        if (pair.second.last_used < cutoff_time || 
            (pair.second.last_used == std::chrono::system_clock::time_point{} && pair.second.created_at < cutoff_time)) {
            result.push_back(pair.second);
        }
    }
    
    return result;
}

APIKeyManager::APIKeyStats APIKeyManager::get_api_key_stats() const {
    std::lock_guard<std::mutex> lock(keys_mutex_);
    
    APIKeyStats current_stats = stats_;
    
    // Calculate average keys per user
    std::unordered_set<std::string> unique_users;
    for (const auto& pair : user_keys_) {
        unique_users.insert(pair.first);
    }
    
    if (!unique_users.empty()) {
        current_stats.avg_keys_per_user = static_cast<double>(stats_.total_keys) / unique_users.size();
    }
    
    return current_stats;
}

void APIKeyManager::update_config(const APIKeyConfig& config) {
    std::lock_guard<std::mutex> lock(keys_mutex_);
    config_ = config;
    HFX_LOG_INFO("[APIKeyManager] Configuration updated");
}

const APIKeyConfig& APIKeyManager::get_config() const {
    std::lock_guard<std::mutex> lock(keys_mutex_);
    return config_;
}

// Private helper methods
std::string APIKeyManager::generate_api_key_string() {
    const std::string chars = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(0, chars.size() - 1);
    
    std::string api_key = config_.key_prefix;
    api_key.reserve(config_.key_prefix.length() + config_.key_length);
    
    for (size_t i = 0; i < config_.key_length; ++i) {
        api_key += chars[dist(gen)];
    }
    
    return api_key;
}

std::string APIKeyManager::generate_key_id() {
    const std::string chars = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(0, chars.size() - 1);
    
    std::string key_id = "key_";
    key_id.reserve(20);
    
    for (int i = 0; i < 16; ++i) {
        key_id += chars[dist(gen)];
    }
    
    return key_id;
}

std::string APIKeyManager::hash_api_key(const std::string& api_key_string) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, api_key_string.c_str(), api_key_string.size());
    SHA256_Final(hash, &sha256);

    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    }
    return ss.str();
}

bool APIKeyManager::is_key_valid(const APIKey& key) const {
    if (!key.is_active) {
        return false;
    }
    
    auto now = std::chrono::system_clock::now();
    return now < key.expires_at;
}

void APIKeyManager::update_indexes(const APIKey& key) {
    key_hash_to_id_[key.key_hash] = key.key_id;
    user_keys_.emplace(key.user_id, key.key_id);
}

void APIKeyManager::remove_from_indexes(const std::string& key_id) {
    auto key_it = api_keys_.find(key_id);
    if (key_it != api_keys_.end()) {
        const APIKey& key = key_it->second;
        
        // Remove from hash index
        key_hash_to_id_.erase(key.key_hash);
        
        // Remove from user index
        auto range = user_keys_.equal_range(key.user_id);
        for (auto it = range.first; it != range.second; ++it) {
            if (it->second == key_id) {
                user_keys_.erase(it);
                break;
            }
        }
    }
}

void APIKeyManager::update_statistics() {
    // Statistics are updated in real-time during operations
    // This method can be used for periodic cleanup or recalculation
}

bool APIKeyManager::check_permission_json(const std::string& permissions_json, const std::string& permission) {
    // Simplified JSON parsing for now - in production would use proper JSON library
    if (permissions_json.empty() || permissions_json == "{}") {
    return false;
    }
    
    // Simple check if permission appears in the string
    return permissions_json.find("\"" + permission + "\"") != std::string::npos;
}

std::vector<std::string> APIKeyManager::parse_permissions(const std::string& permissions_json) {
    std::vector<std::string> result;
    
    // Simplified JSON parsing for now - in production would use proper JSON library
    if (permissions_json.empty() || permissions_json == "{}") {
        return result;
    }
    
    // Simple extraction of quoted strings - this is a simplified implementation
    size_t pos = 0;
    while ((pos = permissions_json.find("\"", pos)) != std::string::npos) {
        size_t start = pos + 1;
        size_t end = permissions_json.find("\"", start);
        if (end != std::string::npos) {
            std::string permission = permissions_json.substr(start, end - start);
            if (!permission.empty() && permission != "true" && permission != "false") {
                result.push_back(permission);
            }
            pos = end + 1;
        } else {
            break;
        }
    }
    
    return result;
}

} // namespace auth
} // namespace hfx