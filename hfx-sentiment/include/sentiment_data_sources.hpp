#pragma once

#include "sentiment_analyzer.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <curl/curl.h>

namespace hfx {
namespace sentiment {

// Data source types
enum class DataSourceType {
    NEWS_API = 0,
    TWITTER_API,
    REDDIT_API,
    TELEGRAM_API,
    DISCORD_API,
    RSS_FEEDS,
    WEB_SCRAPING,
    FILE_SYSTEM
};

// News article structure
struct NewsArticle {
    std::string article_id;
    std::string title;
    std::string content;
    std::string summary;
    std::string source;
    std::string author;
    std::string url;
    std::chrono::system_clock::time_point published_at;
    std::vector<std::string> tags;
    std::vector<std::string> symbols_mentioned;
    std::unordered_map<std::string, std::string> metadata;

    NewsArticle() : published_at(std::chrono::system_clock::now()) {}
};

// Social media post structure
struct SocialMediaPost {
    std::string post_id;
    std::string content;
    std::string author;
    std::string platform; // twitter, reddit, telegram, etc.
    std::string url;
    std::chrono::system_clock::time_point posted_at;
    int likes_count;
    int retweets_count;
    int replies_count;
    std::vector<std::string> hashtags;
    std::vector<std::string> symbols_mentioned;
    std::string sentiment_context;
    std::unordered_map<std::string, std::string> metadata;

    SocialMediaPost() : posted_at(std::chrono::system_clock::now()),
                       likes_count(0), retweets_count(0), replies_count(0) {}
};

// Data source configuration
struct DataSourceConfig {
    DataSourceType type;
    std::string name;
    std::string api_key;
    std::string api_secret;
    std::string base_url;
    int request_rate_limit; // requests per minute
    int max_results_per_request;
    std::vector<std::string> monitored_keywords;
    std::vector<std::string> monitored_symbols;
    bool enable_streaming;
    std::unordered_map<std::string, std::string> additional_params;

    DataSourceConfig() : type(DataSourceType::NEWS_API),
                        request_rate_limit(60),
                        max_results_per_request(100),
                        enable_streaming(false) {}
};

// Base data source interface
class IDataSource {
public:
    virtual ~IDataSource() = default;

    virtual DataSourceType get_type() const = 0;
    virtual std::string get_name() const = 0;

    virtual bool connect() = 0;
    virtual void disconnect() = 0;
    virtual bool is_connected() const = 0;

    virtual std::vector<NewsArticle> fetch_news(const std::vector<std::string>& symbols,
                                              std::chrono::system_clock::time_point since) = 0;

    virtual std::vector<SocialMediaPost> fetch_social_posts(const std::vector<std::string>& symbols,
                                                          std::chrono::system_clock::time_point since) = 0;

    virtual void start_streaming() = 0;
    virtual void stop_streaming() = 0;
    virtual bool is_streaming() const = 0;

    // Real-time data callbacks
    using NewsCallback = std::function<void(const NewsArticle&)>;
    using SocialPostCallback = std::function<void(const SocialMediaPost&)>;

    virtual void set_news_callback(NewsCallback callback) = 0;
    virtual void set_social_post_callback(SocialPostCallback callback) = 0;
};

// News API data source
class NewsAPIDataSource : public IDataSource {
public:
    explicit NewsAPIDataSource(const DataSourceConfig& config);
    ~NewsAPIDataSource() override;

    DataSourceType get_type() const override { return DataSourceType::NEWS_API; }
    std::string get_name() const override { return config_.name; }

    bool connect() override;
    void disconnect() override;
    bool is_connected() const override;

    std::vector<NewsArticle> fetch_news(const std::vector<std::string>& symbols,
                                      std::chrono::system_clock::time_point since) override;

    std::vector<SocialMediaPost> fetch_social_posts(const std::vector<std::string>& symbols,
                                                  std::chrono::system_clock::time_point since) override;

    void start_streaming() override;
    void stop_streaming() override;
    bool is_streaming() const override;

    void set_news_callback(NewsCallback callback) override;
    void set_social_post_callback(SocialPostCallback callback) override;

private:
    DataSourceConfig config_;
    CURL* curl_handle_;
    bool connected_;
    bool streaming_;
    NewsCallback news_callback_;
    SocialPostCallback social_callback_;

    std::string build_news_api_url(const std::vector<std::string>& symbols,
                                 std::chrono::system_clock::time_point since) const;
    NewsArticle parse_news_article(const std::string& json_data) const;
    std::vector<NewsArticle> parse_news_response(const std::string& response) const;
    static size_t curl_write_callback(void* contents, size_t size, size_t nmemb, void* userp);
    std::string make_http_request(const std::string& url) const;
};

// Twitter API data source
class TwitterAPIDataSource : public IDataSource {
public:
    explicit TwitterAPIDataSource(const DataSourceConfig& config);
    ~TwitterAPIDataSource() override;

    DataSourceType get_type() const override { return DataSourceType::TWITTER_API; }
    std::string get_name() const override { return config_.name; }

    bool connect() override;
    void disconnect() override;
    bool is_connected() const override;

    std::vector<NewsArticle> fetch_news(const std::vector<std::string>& symbols,
                                      std::chrono::system_clock::time_point since) override;

    std::vector<SocialMediaPost> fetch_social_posts(const std::vector<std::string>& symbols,
                                                  std::chrono::system_clock::time_point since) override;

    void start_streaming() override;
    void stop_streaming() override;
    bool is_streaming() const override;

    void set_news_callback(NewsCallback callback) override;
    void set_social_post_callback(SocialPostCallback callback) override;

private:
    DataSourceConfig config_;
    std::string bearer_token_;
    CURL* curl_handle_;
    bool connected_;
    bool streaming_;
    NewsCallback news_callback_;
    SocialPostCallback social_callback_;

    std::string authenticate() const;
    std::string build_search_url(const std::vector<std::string>& symbols,
                               std::chrono::system_clock::time_point since) const;
    SocialMediaPost parse_tweet(const std::string& json_data) const;
    std::vector<SocialMediaPost> parse_tweets_response(const std::string& response) const;
    std::string make_authenticated_request(const std::string& url) const;
};

// Reddit API data source
class RedditAPIDataSource : public IDataSource {
public:
    explicit RedditAPIDataSource(const DataSourceConfig& config);
    ~RedditAPIDataSource() override;

    DataSourceType get_type() const override { return DataSourceType::REDDIT_API; }
    std::string get_name() const override { return config_.name; }

    bool connect() override;
    void disconnect() override;
    bool is_connected() const override;

    std::vector<NewsArticle> fetch_news(const std::vector<std::string>& symbols,
                                      std::chrono::system_clock::time_point since) override;

    std::vector<SocialMediaPost> fetch_social_posts(const std::vector<std::string>& symbols,
                                                  std::chrono::system_clock::time_point since) override;

    void start_streaming() override;
    void stop_streaming() override;
    bool is_streaming() const override;

    void set_news_callback(NewsCallback callback) override;
    void set_social_post_callback(SocialPostCallback callback) override;

private:
    DataSourceConfig config_;
    CURL* curl_handle_;
    bool connected_;
    bool streaming_;
    NewsCallback news_callback_;
    SocialPostCallback social_callback_;

    std::string build_reddit_url(const std::string& subreddit,
                               const std::vector<std::string>& symbols) const;
    SocialMediaPost parse_reddit_post(const std::string& json_data) const;
    std::vector<SocialMediaPost> parse_reddit_response(const std::string& response) const;
    std::string make_request(const std::string& url) const;
};

// RSS Feed data source
class RSSFeedDataSource : public IDataSource {
public:
    explicit RSSFeedDataSource(const DataSourceConfig& config);
    ~RSSFeedDataSource() override;

    DataSourceType get_type() const override { return DataSourceType::RSS_FEEDS; }
    std::string get_name() const override { return config_.name; }

    bool connect() override;
    void disconnect() override;
    bool is_connected() const override;

    std::vector<NewsArticle> fetch_news(const std::vector<std::string>& symbols,
                                      std::chrono::system_clock::time_point since) override;

    std::vector<SocialMediaPost> fetch_social_posts(const std::vector<std::string>& symbols,
                                                  std::chrono::system_clock::time_point since) override;

    void start_streaming() override;
    void stop_streaming() override;
    bool is_streaming() const override;

    void set_news_callback(NewsCallback callback) override;
    void set_social_post_callback(SocialPostCallback callback) override;

private:
    DataSourceConfig config_;
    CURL* curl_handle_;
    bool connected_;
    bool streaming_;
    NewsCallback news_callback_;
    SocialPostCallback social_callback_;
    std::unordered_map<std::string, std::chrono::system_clock::time_point> last_fetch_times_;

    NewsArticle parse_rss_item(const std::string& xml_data) const;
    std::vector<NewsArticle> parse_rss_feed(const std::string& xml_data) const;
    std::string make_request(const std::string& url) const;
    bool is_crypto_related(const std::string& title, const std::string& description,
                          const std::vector<std::string>& symbols) const;
};

// Data source manager
class DataSourceManager {
public:
    explicit DataSourceManager(const std::vector<DataSourceConfig>& configs);
    ~DataSourceManager();

    void add_data_source(std::unique_ptr<IDataSource> data_source);
    void remove_data_source(const std::string& name);

    std::vector<NewsArticle> fetch_all_news(const std::vector<std::string>& symbols,
                                          std::chrono::system_clock::time_point since);

    std::vector<SocialMediaPost> fetch_all_social_posts(const std::vector<std::string>& symbols,
                                                     std::chrono::system_clock::time_point since);

    void start_all_streaming();
    void stop_all_streaming();

    std::vector<std::string> get_available_sources() const;
    std::vector<DataSourceType> get_source_types() const;

    // Statistics
    struct DataSourceStats {
        std::atomic<uint64_t> total_news_fetched{0};
        std::atomic<uint64_t> total_posts_fetched{0};
        std::atomic<uint64_t> connection_errors{0};
        std::atomic<uint64_t> rate_limit_hits{0};
        std::chrono::system_clock::time_point last_fetch;
    };

    const DataSourceStats& get_stats() const;
    void reset_stats();

private:
    std::vector<std::unique_ptr<IDataSource>> data_sources_;
    mutable std::mutex sources_mutex_;
    mutable DataSourceStats stats_;
};

// Factory functions
std::unique_ptr<IDataSource> create_data_source(const DataSourceConfig& config);

// Utility functions
std::string data_source_type_to_string(DataSourceType type);
DataSourceType string_to_data_source_type(const std::string& type_str);
std::vector<std::string> extract_crypto_symbols(const std::string& text);
bool contains_crypto_keywords(const std::string& text);
std::string format_timestamp(std::chrono::system_clock::time_point timestamp);

} // namespace sentiment
} // namespace hfx
