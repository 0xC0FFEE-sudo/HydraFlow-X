#pragma once

#include <string>
#include <memory>
#include <chrono>
#include <unordered_map>
#include <mutex>
#include <optional>
#include "auth_manager.hpp"

namespace hfx {
namespace auth {

// JWT Claims structure
struct JWTClaims {
    std::string issuer;
    std::string subject; // user_id
    std::string audience;
    std::chrono::system_clock::time_point issued_at;
    std::chrono::system_clock::time_point expires_at;
    std::chrono::system_clock::time_point not_before;
    std::string jwt_id;
    UserRole role;
    std::unordered_map<std::string, std::string> custom_claims;

    JWTClaims() : role(UserRole::VIEWER) {}
};

// JWT Header structure
struct JWTHeader {
    std::string algorithm;
    std::string type;
    std::string key_id;

    JWTHeader() : algorithm("HS256"), type("JWT") {}
};

// JWT Token structure
struct JWTToken {
    JWTHeader header;
    JWTClaims payload;
    std::string signature;

    JWTToken() = default;
};

// Refresh token structure
struct RefreshToken {
    std::string token_id;
    std::string user_id;
    std::chrono::system_clock::time_point issued_at;
    std::chrono::system_clock::time_point expires_at;
    bool is_revoked;
    int usage_count;

    RefreshToken() : is_revoked(false), usage_count(0) {}
};

// JWT Manager configuration
struct JWTConfig {
    std::string secret_key;
    std::string issuer;
    std::string audience;
    std::chrono::seconds access_token_expiration = std::chrono::hours(1);
    std::chrono::seconds refresh_token_expiration = std::chrono::days(7);
    std::string algorithm = "HS256";
    size_t key_size = 256; // bits
};

// JWT Manager class
class JWTManager {
public:
    explicit JWTManager(const JWTConfig& config);
    ~JWTManager();

    // Token generation
    std::string generate_access_token(const std::string& user_id, UserRole role,
                                     const std::unordered_map<std::string, std::string>& custom_claims = {});
    std::string generate_refresh_token(const std::string& user_id);
    std::pair<std::string, std::string> generate_token_pair(const std::string& user_id, UserRole role);

    // Token validation
    std::optional<JWTClaims> validate_access_token(const std::string& token);
    std::optional<RefreshToken> validate_refresh_token(const std::string& token);
    bool is_token_expired(const std::string& token);
    bool is_token_revoked(const std::string& token_id);

    // Token refresh
    std::pair<std::string, std::string> refresh_tokens(const std::string& refresh_token);
    std::string refresh_access_token(const std::string& refresh_token);

    // Token revocation
    void revoke_token(const std::string& token_id);
    void revoke_all_user_tokens(const std::string& user_id);
    void revoke_refresh_token(const std::string& token);

    // Token inspection
    std::optional<JWTClaims> inspect_token(const std::string& token);
    std::string get_token_user_id(const std::string& token);
    UserRole get_token_user_role(const std::string& token);
    std::chrono::system_clock::time_point get_token_expiration(const std::string& token);

    // Token management
    void cleanup_expired_tokens();
    void set_token_blacklist(const std::string& token_id, std::chrono::system_clock::time_point expiration);
    bool is_token_blacklisted(const std::string& token_id);

    // Configuration
    void update_config(const JWTConfig& config);
    const JWTConfig& get_config() const;

    // Statistics
    struct JWTStats {
        std::atomic<uint64_t> tokens_generated{0};
        std::atomic<uint64_t> tokens_validated{0};
        std::atomic<uint64_t> tokens_revoked{0};
        std::atomic<uint64_t> validation_failures{0};
        std::atomic<uint64_t> refresh_attempts{0};
        std::atomic<uint64_t> blacklisted_tokens{0};
        std::chrono::system_clock::time_point last_cleanup;
    };

    const JWTStats& get_jwt_stats() const;
    void reset_jwt_stats();

private:
    JWTConfig config_;
    mutable std::mutex config_mutex_;

    // Token storage
    std::unordered_map<std::string, RefreshToken> refresh_tokens_;
    mutable std::mutex refresh_tokens_mutex_;

    // Blacklisted tokens
    std::unordered_map<std::string, std::chrono::system_clock::time_point> token_blacklist_;
    mutable std::mutex blacklist_mutex_;

    // Statistics
    mutable JWTStats stats_;

    // Internal methods
    std::string encode_base64url(const std::string& data);
    std::string decode_base64url(const std::string& data);
    std::string create_signature(const std::string& header, const std::string& payload);
    bool verify_signature(const std::string& header, const std::string& payload, const std::string& signature);

    std::string generate_jwt_header();
    std::string generate_jwt_payload(const JWTClaims& claims);
    JWTClaims parse_jwt_payload(const std::string& payload);

    std::string generate_token_id();
    std::string generate_secure_random(size_t length);

    bool validate_token_format(const std::string& token);
    std::vector<std::string> split_token(const std::string& token);

    void cleanup_blacklist();
    void update_refresh_token_usage(const std::string& token_id);

    // HMAC-SHA256 implementation
    std::string hmac_sha256(const std::string& key, const std::string& data);
    std::string sha256(const std::string& data);
};

// Utility functions
std::string generate_jwt_secret(size_t key_size = 256);
bool validate_jwt_secret(const std::string& secret);
std::chrono::seconds get_default_token_expiration(UserRole role);

} // namespace auth
} // namespace hfx
