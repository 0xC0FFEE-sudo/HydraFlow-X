#pragma once

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <atomic>
#include <chrono>
#include <functional>

namespace hfx::ai {

// API Response structures
struct TwitterData {
    std::string tweet_id;
    std::string text;
    std::string author;
    int64_t timestamp;
    int likes;
    int retweets;
    double sentiment_score;
    std::vector<std::string> hashtags;
    std::vector<std::string> mentions;
};

struct GMGNData {
    std::string token_address;
    std::string symbol;
    double smart_money_score;
    double price_usd;
    double volume_24h;
    double price_change_1h;
    double price_change_24h;
    std::vector<std::string> smart_wallets;
    double insider_confidence;
    int64_t last_updated;
};

struct DexScreenerData {
    std::string pair_address;
    std::string token_address;
    std::string symbol;
    std::string name;
    double price_usd;
    double volume_24h;
    double liquidity_usd;
    double fdv;
    double price_change_1h;
    double price_change_24h;
    std::string dex;
    bool verified;
    double audit_score;
    int64_t created_at;
};

struct RedditData {
    std::string post_id;
    std::string title;
    std::string content;
    std::string subreddit;
    int upvotes;
    int comments;
    double sentiment_score;
    std::vector<std::string> mentioned_tokens;
    int64_t timestamp;
};

struct NewsData {
    std::string article_id;
    std::string title;
    std::string content;
    std::string source;
    std::string url;
    double sentiment_score;
    double relevance_score;
    std::vector<std::string> mentioned_tokens;
    int64_t published_at;
    std::string category;
};

struct APIMetrics {
    std::atomic<uint64_t> total_requests{0};
    std::atomic<uint64_t> successful_requests{0};
    std::atomic<uint64_t> failed_requests{0};
    std::atomic<uint64_t> rate_limits_hit{0};
    std::atomic<double> avg_response_time_ms{0.0};
    std::atomic<uint64_t> last_request_timestamp{0};
    
    // Copy constructor and assignment for atomic members
    APIMetrics(const APIMetrics& other) {
        total_requests.store(other.total_requests.load());
        successful_requests.store(other.successful_requests.load());
        failed_requests.store(other.failed_requests.load());
        rate_limits_hit.store(other.rate_limits_hit.load());
        avg_response_time_ms.store(other.avg_response_time_ms.load());
        last_request_timestamp.store(other.last_request_timestamp.load());
    }
    
    APIMetrics& operator=(const APIMetrics& other) {
        if (this != &other) {
            total_requests.store(other.total_requests.load());
            successful_requests.store(other.successful_requests.load());
            failed_requests.store(other.failed_requests.load());
            rate_limits_hit.store(other.rate_limits_hit.load());
            avg_response_time_ms.store(other.avg_response_time_ms.load());
            last_request_timestamp.store(other.last_request_timestamp.load());
        }
        return *this;
    }
    
    APIMetrics() = default;
    APIMetrics(APIMetrics&&) = delete;
    APIMetrics& operator=(APIMetrics&&) = delete;
};

// Main API Integration Manager - inspired by fastest bot architectures
class APIIntegrationManager {
private:
    struct Impl;
    std::unique_ptr<Impl> pimpl_;

public:
    APIIntegrationManager();
    ~APIIntegrationManager();

    // Core initialization - like GMGN's multi-source setup
    bool initialize();
    void start_real_time_feeds();
    void stop_real_time_feeds();
    
    // Twitter/X API integration - inspired by fastest bots' sentiment analysis
    bool configure_twitter_api(const std::string& bearer_token, 
                              const std::string& api_key,
                              const std::string& api_secret);
    std::vector<TwitterData> fetch_crypto_tweets(const std::vector<std::string>& keywords,
                                                 int limit = 100);
    std::vector<TwitterData> stream_real_time_tweets(const std::vector<std::string>& keywords);
    double analyze_twitter_sentiment(const std::vector<TwitterData>& tweets);
    
    // GMGN API integration - for smart money tracking like top bots
    bool configure_gmgn_api(const std::string& api_key = "");
    GMGNData fetch_token_smart_money_data(const std::string& token_address);
    std::vector<GMGNData> get_trending_smart_money_tokens(int limit = 50);
    std::vector<std::string> track_smart_wallets(const std::vector<std::string>& wallet_addresses);
    double calculate_smart_money_momentum(const std::string& token_address);
    
    // DexScreener API - for token discovery like Photon's scanning
    bool configure_dexscreener_api();
    DexScreenerData fetch_token_data(const std::string& token_address);
    std::vector<DexScreenerData> get_new_pairs(const std::string& chain = "solana",
                                               std::chrono::minutes age_limit = std::chrono::minutes(5));
    std::vector<DexScreenerData> scan_trending_tokens(const std::string& chain = "solana");
    std::vector<DexScreenerData> find_potential_gems(double min_liquidity = 10000.0,
                                                    double max_age_hours = 24.0);
    
    // Reddit API integration - for social sentiment
    bool configure_reddit_api(const std::string& client_id,
                             const std::string& client_secret,
                             const std::string& user_agent);
    std::vector<RedditData> fetch_crypto_posts(const std::vector<std::string>& subreddits,
                                              int limit = 100);
    double analyze_reddit_sentiment(const std::vector<RedditData>& posts);
    std::vector<std::string> detect_trending_tokens_reddit();
    
    // News API integration - for fundamental analysis
    bool configure_news_apis(const std::unordered_map<std::string, std::string>& api_configs);
    std::vector<NewsData> fetch_crypto_news(const std::vector<std::string>& keywords,
                                           std::chrono::hours lookback = std::chrono::hours(24));
    double analyze_news_sentiment(const std::vector<NewsData>& articles);
    std::vector<std::string> detect_market_moving_events();
    
    // Exchange APIs - for trading execution like fastest bots
    bool configure_exchange_apis(const std::unordered_map<std::string, std::unordered_map<std::string, std::string>>& configs);
    bool place_order(const std::string& exchange, const std::string& symbol,
                    const std::string& side, double quantity, double price);
    bool cancel_order(const std::string& exchange, const std::string& order_id);
    std::vector<std::string> get_supported_exchanges() const;
    
    // Real-time WebSocket feeds - inspired by BonkBot's speed
    void subscribe_to_price_updates(const std::vector<std::string>& symbols,
                                   std::function<void(const std::string&, double)> callback);
    void subscribe_to_order_book_updates(const std::string& symbol,
                                        std::function<void(const std::string&, const std::string&)> callback);
    void subscribe_to_trade_updates(const std::vector<std::string>& symbols,
                                   std::function<void(const std::string&, double, double)> callback);
    
    // Advanced analytics - like GMGN's AI analysis
    struct TokenSignal {
        std::string token_address;
        double overall_score;
        double sentiment_score;
        double smart_money_score;
        double technical_score;
        double momentum_score;
        std::string recommendation; // "strong_buy", "buy", "hold", "sell", "strong_sell"
        std::chrono::system_clock::time_point generated_at;
    };
    
    TokenSignal generate_unified_token_signal(const std::string& token_address);
    std::vector<TokenSignal> scan_for_opportunities();
    std::vector<TokenSignal> get_top_signals(int limit = 10);
    
    // Rate limiting and error handling - critical for production
    void set_rate_limits(const std::unordered_map<std::string, int>& limits_per_minute);
    bool is_rate_limited(const std::string& api_name);
    void handle_api_error(const std::string& api_name, const std::string& error);
    
    // Metrics and monitoring
    APIMetrics get_api_metrics(const std::string& api_name) const;
    std::unordered_map<std::string, APIMetrics> get_all_metrics() const;
    bool health_check() const;
    
    // Configuration and management
    void set_api_timeout(std::chrono::milliseconds timeout);
    void set_retry_config(int max_retries, std::chrono::milliseconds retry_delay);
    void enable_caching(bool enabled, std::chrono::seconds cache_ttl = std::chrono::seconds(30));
};

// Specialized API clients for different sources
class TwitterAPIClient {
public:
    static std::vector<TwitterData> fetch_tweets_by_keywords(const std::vector<std::string>& keywords,
                                                            const std::string& bearer_token);
    static std::vector<TwitterData> fetch_tweets_by_users(const std::vector<std::string>& usernames,
                                                         const std::string& bearer_token);
    static double calculate_sentiment_aggregate(const std::vector<TwitterData>& tweets);
    static std::vector<std::string> extract_mentioned_tokens(const std::vector<TwitterData>& tweets);
};

class GMGNAPIClient {
public:
    static GMGNData fetch_token_analysis(const std::string& token_address);
    static std::vector<GMGNData> fetch_trending_tokens(int limit = 50);
    static std::vector<std::string> fetch_smart_money_wallets(const std::string& token_address);
    static double calculate_insider_confidence(const GMGNData& data);
};

class DexScreenerAPIClient {
public:
    static DexScreenerData fetch_pair_data(const std::string& pair_address);
    static std::vector<DexScreenerData> search_tokens(const std::string& query);
    static std::vector<DexScreenerData> fetch_new_pairs(const std::string& chain, int limit = 100);
    static bool is_potential_rug(const DexScreenerData& data);
    static double calculate_opportunity_score(const DexScreenerData& data);
};

// WebSocket managers for real-time data - like fastest bots use
class RealTimeDataManager {
public:
    void start_price_streams(const std::vector<std::string>& symbols);
    void start_social_streams(const std::vector<std::string>& keywords);
    void start_smart_money_streams();
    void register_callback(const std::string& stream_type, 
                          std::function<void(const std::string&)> callback);
    bool is_stream_active(const std::string& stream_type) const;
    void stop_all_streams();
};

} // namespace hfx::ai
