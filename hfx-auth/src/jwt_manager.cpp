/**
 * @file jwt_manager.cpp
 * @brief JWT Token Manager Implementation
 */

#include "../include/jwt_manager.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <random>
#include <chrono>
#include <openssl/sha.h>
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <algorithm>
#include <openssl/rand.h>

namespace hfx {
namespace auth {

// Utility functions
std::string generate_jwt_secret(size_t key_size) {
    const std::string chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*";
    std::string secret;
    secret.reserve(key_size / 8); // Convert bits to bytes

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, chars.size() - 1);

    for (size_t i = 0; i < key_size / 8; ++i) {
        secret += chars[dis(gen)];
    }

    return secret;
}

bool validate_jwt_secret(const std::string& secret) {
    return secret.length() >= 32; // Minimum 256 bits
}

std::chrono::seconds get_default_token_expiration(UserRole role) {
    switch (role) {
        case UserRole::ADMIN: return std::chrono::hours(1);
        case UserRole::TRADER: return std::chrono::hours(8);
        case UserRole::ANALYST: return std::chrono::hours(12);
        case UserRole::VIEWER: return std::chrono::hours(24);
        case UserRole::API_USER: return std::chrono::hours(168); // 7 days
        default: return std::chrono::hours(24);
    }
}

// JWTManager implementation
JWTManager::JWTManager(const JWTConfig& config) : config_(config) {
    stats_.last_cleanup = std::chrono::system_clock::now();
}

JWTManager::~JWTManager() = default;

std::string JWTManager::generate_access_token(const std::string& user_id, UserRole role,
                                             const std::unordered_map<std::string, std::string>& custom_claims) {
    JWTClaims claims;
    claims.issuer = config_.issuer;
    claims.subject = user_id;
    claims.audience = config_.audience;
    claims.issued_at = std::chrono::system_clock::now();
    claims.expires_at = claims.issued_at + config_.access_token_expiration;
    claims.not_before = claims.issued_at;
    claims.jwt_id = generate_token_id();
    claims.role = role;
    claims.custom_claims = custom_claims;

    std::string header = generate_jwt_header();
    std::string payload = generate_jwt_payload(claims);
    std::string signature = create_signature(header, payload);

    stats_.tokens_generated.fetch_add(1, std::memory_order_relaxed);

    return header + "." + payload + "." + signature;
}

std::string JWTManager::generate_refresh_token(const std::string& user_id) {
    std::string token_id = generate_token_id();

    RefreshToken refresh_token;
    refresh_token.token_id = token_id;
    refresh_token.user_id = user_id;
    refresh_token.issued_at = std::chrono::system_clock::now();
    refresh_token.expires_at = refresh_token.issued_at + config_.refresh_token_expiration;
    refresh_token.is_revoked = false;

    std::lock_guard<std::mutex> lock(refresh_tokens_mutex_);
    refresh_tokens_[token_id] = refresh_token;

    return token_id;
}

std::pair<std::string, std::string> JWTManager::generate_token_pair(const std::string& user_id, UserRole role) {
    std::string access_token = generate_access_token(user_id, role);
    std::string refresh_token = generate_refresh_token(user_id);
    return {access_token, refresh_token};
}

std::optional<JWTClaims> JWTManager::validate_access_token(const std::string& token) {
    stats_.tokens_validated.fetch_add(1, std::memory_order_relaxed);

    if (!validate_token_format(token)) {
        stats_.validation_failures.fetch_add(1, std::memory_order_relaxed);
        return std::nullopt;
    }

    auto parts = split_token(token);
    if (parts.size() != 3) {
        stats_.validation_failures.fetch_add(1, std::memory_order_relaxed);
        return std::nullopt;
    }

    const std::string& header = parts[0];
    const std::string& payload = parts[1];
    const std::string& signature = parts[2];

    // Verify signature
    if (!verify_signature(header, payload, signature)) {
        stats_.validation_failures.fetch_add(1, std::memory_order_relaxed);
        return std::nullopt;
    }

    // Parse and validate claims
    JWTClaims claims = parse_jwt_payload(payload);

    // Check expiration
    auto now = std::chrono::system_clock::now();
    if (now > claims.expires_at || now < claims.not_before) {
        stats_.validation_failures.fetch_add(1, std::memory_order_relaxed);
        return std::nullopt;
    }

    return claims;
}

std::optional<RefreshToken> JWTManager::validate_refresh_token(const std::string& token) {
    std::lock_guard<std::mutex> lock(refresh_tokens_mutex_);

    auto it = refresh_tokens_.find(token);
    if (it == refresh_tokens_.end()) {
        return std::nullopt;
    }

    RefreshToken& refresh_token = it->second;

    if (refresh_token.is_revoked) {
        return std::nullopt;
    }

    auto now = std::chrono::system_clock::now();
    if (now > refresh_token.expires_at) {
        return std::nullopt;
    }

    refresh_token.usage_count++;
    return refresh_token;
}

std::pair<std::string, std::string> JWTManager::refresh_tokens(const std::string& refresh_token) {
    stats_.refresh_attempts.fetch_add(1, std::memory_order_relaxed);

    auto refresh_opt = validate_refresh_token(refresh_token);
    if (!refresh_opt) {
        return {"", ""};
    }

    RefreshToken refresh = *refresh_opt;

    // Generate new token pair
    return generate_token_pair(refresh.user_id, UserRole::VIEWER); // Would get role from user data
}

void JWTManager::revoke_token(const std::string& token_id) {
    std::lock_guard<std::mutex> lock(blacklist_mutex_);
    auto now = std::chrono::system_clock::now();
    token_blacklist_[token_id] = now + std::chrono::hours(24); // Blacklist for 24 hours
    stats_.tokens_revoked.fetch_add(1, std::memory_order_relaxed);
}

void JWTManager::revoke_refresh_token(const std::string& token) {
    std::lock_guard<std::mutex> lock(refresh_tokens_mutex_);
    auto it = refresh_tokens_.find(token);
    if (it != refresh_tokens_.end()) {
        it->second.is_revoked = true;
        stats_.tokens_revoked.fetch_add(1, std::memory_order_relaxed);
    }
}

std::optional<JWTClaims> JWTManager::inspect_token(const std::string& token) {
    if (!validate_token_format(token)) {
        return std::nullopt;
    }

    auto parts = split_token(token);
    if (parts.size() != 3) {
        return std::nullopt;
    }

    return parse_jwt_payload(parts[1]);
}

std::string JWTManager::get_token_user_id(const std::string& token) {
    auto claims_opt = inspect_token(token);
    return claims_opt ? claims_opt->subject : "";
}

UserRole JWTManager::get_token_user_role(const std::string& token) {
    auto claims_opt = inspect_token(token);
    return claims_opt ? claims_opt->role : UserRole::VIEWER;
}

std::chrono::system_clock::time_point JWTManager::get_token_expiration(const std::string& token) {
    auto claims_opt = inspect_token(token);
    return claims_opt ? claims_opt->expires_at : std::chrono::system_clock::time_point{};
}

void JWTManager::cleanup_expired_tokens() {
    auto now = std::chrono::system_clock::now();

    // Clean up refresh tokens
    {
        std::lock_guard<std::mutex> lock(refresh_tokens_mutex_);
        for (auto it = refresh_tokens_.begin(); it != refresh_tokens_.end();) {
            if (now > it->second.expires_at) {
                it = refresh_tokens_.erase(it);
            } else {
                ++it;
            }
        }
    }

    // Clean up blacklist
    cleanup_blacklist();

    stats_.last_cleanup = now;
}

void JWTManager::cleanup_blacklist() {
    std::lock_guard<std::mutex> lock(blacklist_mutex_);
    auto now = std::chrono::system_clock::now();

    for (auto it = token_blacklist_.begin(); it != token_blacklist_.end();) {
        if (now > it->second) {
            it = token_blacklist_.erase(it);
        } else {
            ++it;
        }
    }
}

bool JWTManager::is_token_blacklisted(const std::string& token_id) {
    std::lock_guard<std::mutex> lock(blacklist_mutex_);
    auto now = std::chrono::system_clock::now();

    auto it = token_blacklist_.find(token_id);
    if (it != token_blacklist_.end()) {
        if (now < it->second) {
            return true;
        } else {
            // Token expired from blacklist, remove it
            token_blacklist_.erase(it);
        }
    }
    return false;
}

void JWTManager::update_config(const JWTConfig& config) {
    std::lock_guard<std::mutex> lock(config_mutex_);
    config_ = config;
}

const JWTConfig& JWTManager::get_config() const {
    std::lock_guard<std::mutex> lock(config_mutex_);
    return config_;
}

const JWTManager::JWTStats& JWTManager::get_jwt_stats() const {
    return stats_;
}

void JWTManager::reset_jwt_stats() {
    stats_.tokens_generated.store(0, std::memory_order_relaxed);
    stats_.tokens_validated.store(0, std::memory_order_relaxed);
    stats_.tokens_revoked.store(0, std::memory_order_relaxed);
    stats_.validation_failures.store(0, std::memory_order_relaxed);
    stats_.refresh_attempts.store(0, std::memory_order_relaxed);
    stats_.blacklisted_tokens.store(0, std::memory_order_relaxed);
    stats_.last_cleanup = std::chrono::system_clock::now();
}

// Private helper methods
std::string JWTManager::generate_jwt_header() {
    std::stringstream ss;
    ss << R"({"alg":")" << config_.algorithm << R"(","typ":"JWT")";
    if (!config_.issuer.empty()) {
        ss << R"(","iss":")" << config_.issuer << R"(")";
    }
    ss << "}";

    return encode_base64url(ss.str());
}

std::string JWTManager::generate_jwt_payload(const JWTClaims& claims) {
    std::stringstream ss;
    ss << R"({"iss":")" << claims.issuer << R"(","sub":")" << claims.subject << R"(","aud":")" << claims.audience;
    ss << R"(","iat":)" << std::chrono::duration_cast<std::chrono::seconds>(claims.issued_at.time_since_epoch()).count();
    ss << R"(,"exp":)" << std::chrono::duration_cast<std::chrono::seconds>(claims.expires_at.time_since_epoch()).count();
    ss << R"(,"nbf":)" << std::chrono::duration_cast<std::chrono::seconds>(claims.not_before.time_since_epoch()).count();
    ss << R"(,"jti":")" << claims.jwt_id << R"(","role":")" << user_role_to_string(claims.role) << R"("})";

    return encode_base64url(ss.str());
}

JWTClaims JWTManager::parse_jwt_payload(const std::string& payload) {
    JWTClaims claims;

    // Simplified JSON parsing (would use proper JSON parser in production)
    std::string decoded = decode_base64url(payload);

    // Extract basic fields (simplified)
    size_t sub_pos = decoded.find("\"sub\":\"");
    if (sub_pos != std::string::npos) {
        size_t start = sub_pos + 7;
        size_t end = decoded.find("\"", start);
        if (end != std::string::npos) {
            claims.subject = decoded.substr(start, end - start);
        }
    }

    size_t role_pos = decoded.find("\"role\":\"");
    if (role_pos != std::string::npos) {
        size_t start = role_pos + 8;
        size_t end = decoded.find("\"", start);
        if (end != std::string::npos) {
            claims.role = string_to_user_role(decoded.substr(start, end - start));
        }
    }

    return claims;
}

std::string JWTManager::create_signature(const std::string& header, const std::string& payload) {
    std::string data = header + "." + payload;
    return encode_base64url(hmac_sha256(config_.secret_key, data));
}

bool JWTManager::verify_signature(const std::string& header, const std::string& payload, const std::string& signature) {
    std::string expected_signature = create_signature(header, payload);
    return signature == expected_signature;
}

std::string JWTManager::encode_base64url(const std::string& data) {
    BIO *bio, *b64;
    BUF_MEM *bufferPtr;

    b64 = BIO_new(BIO_f_base64());
    bio = BIO_new(BIO_s_mem());
    bio = BIO_push(b64, bio);

    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
    BIO_write(bio, data.c_str(), data.length());
    BIO_flush(bio);
    BIO_get_mem_ptr(bio, &bufferPtr);

    std::string result(bufferPtr->data, bufferPtr->length);
    BIO_free_all(bio);

    // Convert to base64url
    std::replace(result.begin(), result.end(), '+', '-');
    std::replace(result.begin(), result.end(), '/', '_');

    return result;
}

std::string JWTManager::decode_base64url(const std::string& data) {
    std::string base64 = data;
    std::replace(base64.begin(), base64.end(), '-', '+');
    std::replace(base64.begin(), base64.end(), '_', '/');

    // Add padding if needed
    while (base64.length() % 4 != 0) {
        base64 += '=';
    }

    BIO *bio, *b64;
    char* buffer = new char[base64.length()];

    bio = BIO_new_mem_buf(base64.c_str(), base64.length());
    b64 = BIO_new(BIO_f_base64());
    bio = BIO_push(b64, bio);

    int decoded_length = BIO_read(bio, buffer, base64.length());
    BIO_free_all(bio);

    std::string result(buffer, decoded_length);
    delete[] buffer;

    return result;
}

std::string JWTManager::hmac_sha256(const std::string& key, const std::string& data) {
    unsigned char* result;
    unsigned int result_len;

    result = HMAC(EVP_sha256(), key.c_str(), key.length(),
                  reinterpret_cast<const unsigned char*>(data.c_str()), data.length(),
                  nullptr, &result_len);

    return std::string(reinterpret_cast<char*>(result), result_len);
}

std::string JWTManager::generate_token_id() {
    return generate_secure_random(16);
}

std::string JWTManager::generate_secure_random(size_t length) {
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

bool JWTManager::validate_token_format(const std::string& token) {
    if (token.empty()) return false;

    size_t dot1 = token.find('.');
    if (dot1 == std::string::npos) return false;

    size_t dot2 = token.find('.', dot1 + 1);
    if (dot2 == std::string::npos) return false;

    return dot2 < token.length() - 1; // Ensure there's content after the last dot
}

std::vector<std::string> JWTManager::split_token(const std::string& token) {
    std::vector<std::string> parts;
    std::stringstream ss(token);
    std::string part;

    while (std::getline(ss, part, '.')) {
        parts.push_back(part);
    }

    return parts;
}

bool JWTManager::is_token_expired(const std::string& token) {
    auto expiration = get_token_expiration(token);
    return std::chrono::system_clock::now() > expiration;
}

bool JWTManager::is_token_revoked(const std::string& token_id) {
    return is_token_blacklisted(token_id);
}

std::string JWTManager::refresh_access_token(const std::string& refresh_token) {
    auto token_pair = refresh_tokens(refresh_token);
    return token_pair.first;
}

void JWTManager::revoke_all_user_tokens(const std::string& user_id) {
    std::lock_guard<std::mutex> lock(refresh_tokens_mutex_);

    for (auto& [token_id, refresh_token] : refresh_tokens_) {
        if (refresh_token.user_id == user_id) {
            refresh_token.is_revoked = true;
        }
    }
}

void JWTManager::set_token_blacklist(const std::string& token_id, std::chrono::system_clock::time_point expiration) {
    std::lock_guard<std::mutex> lock(blacklist_mutex_);
    token_blacklist_[token_id] = expiration;
    stats_.blacklisted_tokens.fetch_add(1, std::memory_order_relaxed);
}

} // namespace auth
} // namespace hfx
