#pragma once

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <atomic>
#include <mutex>
#include <memory>
#include <chrono>
#include <deque>
#include <functional>
#include <vector>

namespace hfx {
namespace security {

// Rate limiting algorithms
enum class RateLimitAlgorithm {
    TOKEN_BUCKET = 0,    // Token bucket algorithm
    LEAKY_BUCKET,        // Leaky bucket algorithm
    FIXED_WINDOW,        // Fixed window counter
    SLIDING_WINDOW       // Sliding window counter
};

// Rate limit rule
struct RateLimitRule {
    std::string name;
    std::string endpoint_pattern;
    RateLimitAlgorithm algorithm;
    int requests_per_window;
    std::chrono::seconds window_size;
    std::chrono::seconds block_duration;
    bool enabled;
    std::vector<std::string> excluded_ips;
    std::vector<std::string> excluded_user_agents;

    RateLimitRule() : algorithm(RateLimitAlgorithm::TOKEN_BUCKET),
                     requests_per_window(100),
                     window_size(std::chrono::minutes(1)),
                     block_duration(std::chrono::minutes(5)),
                     enabled(true) {}
};

// Rate limit status
struct RateLimitStatus {
    bool allowed;
    int remaining_requests;
    std::chrono::seconds reset_time;
    std::string limit_type;
    bool blocked;

    RateLimitStatus() : allowed(true), remaining_requests(0),
                       reset_time(0), blocked(false) {}
};

// Rate limiting configuration
struct RateLimitConfig {
    bool enabled;
    int max_requests_per_minute;
    int max_requests_per_hour;
    int max_concurrent_connections;
    std::chrono::minutes block_duration;
    bool enable_ip_whitelist;
    bool enable_ip_blacklist;
    std::vector<std::string> whitelisted_ips;
    std::vector<std::string> blacklisted_ips;
    std::vector<RateLimitRule> custom_rules;

    RateLimitConfig() : enabled(true),
                       max_requests_per_minute(60),
                       max_requests_per_hour(1000),
                       max_concurrent_connections(100),
                       block_duration(std::chrono::minutes(15)),
                       enable_ip_whitelist(false),
                       enable_ip_blacklist(true) {}
};

// Token bucket implementation
class TokenBucket {
public:
    TokenBucket(int capacity, int refill_rate, std::chrono::seconds refill_interval);

    bool consume(int tokens = 1);
    void refill();
    int get_available_tokens() const;
    void reset();

private:
    int capacity_;
    int refill_rate_;
    std::chrono::seconds refill_interval_;
    std::atomic<int> tokens_;
    std::chrono::steady_clock::time_point last_refill_;
    mutable std::mutex mutex_;
};

// Leaky bucket implementation
class LeakyBucket {
public:
    LeakyBucket(int capacity, int leak_rate, std::chrono::seconds leak_interval);

    bool add_request();
    int get_queue_size() const;
    void reset();

private:
    int capacity_;
    int leak_rate_;
    std::chrono::seconds leak_interval_;
    std::deque<std::chrono::steady_clock::time_point> requests_;
    mutable std::mutex mutex_;
};

// Fixed window counter
class FixedWindowCounter {
public:
    FixedWindowCounter(int max_requests, std::chrono::seconds window_size);

    bool record_request();
    void reset_window();
    int get_current_count() const;

private:
    int max_requests_;
    std::chrono::seconds window_size_;
    std::atomic<int> current_count_;
    std::chrono::steady_clock::time_point window_start_;
    mutable std::mutex mutex_;
};

// Sliding window counter
class SlidingWindowCounter {
public:
    SlidingWindowCounter(int max_requests, std::chrono::seconds window_size);

    bool record_request();
    int get_current_count() const;

private:
    int max_requests_;
    std::chrono::seconds window_size_;
    std::deque<std::chrono::steady_clock::time_point> requests_;
    mutable std::mutex mutex_;
};

// Main rate limiter class
class RateLimiter {
public:
    explicit RateLimiter(const RateLimitConfig& config);
    ~RateLimiter();

    // Rate limiting methods
    RateLimitStatus check_rate_limit(const std::string& identifier,
                                   const std::string& endpoint = "",
                                   const std::string& ip_address = "",
                                   const std::string& user_agent = "");

    // IP filtering
    bool is_ip_allowed(const std::string& ip_address) const;
    bool is_ip_blocked(const std::string& ip_address) const;
    void add_to_blacklist(const std::string& ip_address);
    void remove_from_blacklist(const std::string& ip_address);
    void add_to_whitelist(const std::string& ip_address);
    void remove_from_whitelist(const std::string& ip_address);

    // Rule management
    void add_rule(const RateLimitRule& rule);
    void remove_rule(const std::string& rule_name);
    void enable_rule(const std::string& rule_name);
    void disable_rule(const std::string& rule_name);

    // Statistics and monitoring
    struct RateLimitStats {
        std::atomic<uint64_t> total_requests{0};
        std::atomic<uint64_t> blocked_requests{0};
        std::atomic<uint64_t> whitelisted_requests{0};
        std::atomic<uint64_t> blacklisted_requests{0};
        std::atomic<uint64_t> rate_limited_requests{0};
        std::atomic<size_t> active_connections{0};
        std::atomic<size_t> blacklisted_ips{0};
        std::atomic<size_t> whitelisted_ips{0};
        std::chrono::system_clock::time_point last_blocked_request;
    };

    RateLimitStats get_stats() const;
    void reset_stats();

    // Configuration
    void update_config(const RateLimitConfig& config);
    RateLimitConfig get_config() const;

    // Cleanup and maintenance
    void cleanup_expired_blocks();
    void cleanup_old_data();

private:
    RateLimitConfig config_;
    mutable std::mutex config_mutex_;

    // Rate limiting data structures
    std::unordered_map<std::string, std::unique_ptr<TokenBucket>> token_buckets_;
    std::unordered_map<std::string, std::unique_ptr<LeakyBucket>> leaky_buckets_;
    std::unordered_map<std::string, std::unique_ptr<FixedWindowCounter>> fixed_window_counters_;
    std::unordered_map<std::string, std::unique_ptr<SlidingWindowCounter>> sliding_window_counters_;

    // IP filtering
    std::unordered_set<std::string> whitelisted_ips_;
    std::unordered_set<std::string> blacklisted_ips_;

    // Rules
    std::unordered_map<std::string, RateLimitRule> rules_;

    // Statistics
    mutable RateLimitStats stats_;

    // Mutexes
    mutable std::mutex buckets_mutex_;
    mutable std::mutex ip_mutex_;
    mutable std::mutex rules_mutex_;

    // Internal methods
    std::string generate_key(const std::string& identifier, const std::string& endpoint = "") const;
    RateLimitAlgorithm get_algorithm_for_endpoint(const std::string& endpoint) const;
    const RateLimitRule* get_rule_for_endpoint(const std::string& endpoint) const;
    void update_stats(const RateLimitStatus& status);

    template<typename T>
    T* get_or_create_bucket(const std::string& key,
                           std::unordered_map<std::string, std::unique_ptr<T>>& buckets,
                           std::function<std::unique_ptr<T>()> factory);
};

// Utility functions
std::string rate_limit_algorithm_to_string(RateLimitAlgorithm algorithm);
RateLimitAlgorithm string_to_rate_limit_algorithm(const std::string& algorithm_str);
bool validate_ip_address(const std::string& ip);
bool validate_endpoint_pattern(const std::string& pattern);

} // namespace security
} // namespace hfx
