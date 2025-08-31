#pragma once

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <atomic>
#include <chrono>
#include <functional>
#include <queue>

namespace hfx::ai {

// Enhanced real-time data structures for ultra-fast processing
struct LiveMarketData {
    std::string symbol;
    std::string source;
    double price;
    double volume_24h;
    double price_change_1h;
    double price_change_24h;
    double liquidity;
    std::chrono::microseconds timestamp;
    uint64_t sequence_number;
    bool is_validated;
};

struct LiveSentimentData {
    std::string symbol;
    std::string source;
    double sentiment_score;
    double confidence;
    int64_t engagement_count;
    std::vector<std::string> keywords;
    std::chrono::microseconds timestamp;
    std::string raw_text;
    bool is_viral; // Trending/viral detection
};

struct LiveSmartMoneyData {
    std::string symbol;
    std::string wallet_address;
    std::string transaction_hash;
    double amount_usd;
    std::string direction; // "buy", "sell"
    double smart_money_score;
    std::chrono::microseconds timestamp;
    bool is_insider_wallet;
    double portfolio_size;
};

struct AggregatedSignal {
    std::string symbol;
    double overall_score;
    double technical_score;
    double sentiment_score;
    double smart_money_score;
    double momentum_score;
    std::string recommendation;
    std::chrono::microseconds generated_at;
    std::chrono::microseconds expires_at;
    std::vector<std::string> contributing_sources;
    double confidence_level;
    bool is_actionable;
};

struct StreamHealth {
    std::atomic<bool> is_connected{false};
    std::atomic<uint64_t> messages_received{0};
    std::atomic<uint64_t> messages_processed{0};
    std::atomic<uint64_t> messages_dropped{0};
    std::atomic<double> latency_ms{0.0};
    std::atomic<uint64_t> last_message_time{0};
    std::atomic<uint64_t> reconnection_count{0};
    std::string last_error;
    
    StreamHealth() = default;
    StreamHealth(const StreamHealth&) = delete;
    StreamHealth& operator=(const StreamHealth&) = delete;
};

enum class StreamType {
    MARKET_DATA,
    SENTIMENT_DATA,
    SMART_MONEY,
    NEWS_FEED,
    SOCIAL_MEDIA,
    TECHNICAL_INDICATORS
};

// Real-time data aggregator for ultra-fast signal fusion
class RealTimeDataAggregator {
private:
    struct Impl;
    std::unique_ptr<Impl> pimpl_;

public:
    RealTimeDataAggregator();
    ~RealTimeDataAggregator();

    // Core streaming initialization
    bool initialize();
    void start_all_streams();
    void stop_all_streams();
    
    // Stream management - inspired by fastest bot streaming architectures
    bool start_stream(StreamType type, const std::string& config);
    void stop_stream(StreamType type);
    bool is_stream_healthy(StreamType type) const;
    void restart_stream(StreamType type);
    
    // Twitter/X real-time streaming - like fastest bots use
    void start_twitter_stream(const std::vector<std::string>& keywords,
                             const std::vector<std::string>& usernames = {});
    void configure_twitter_filters(const std::vector<std::string>& keywords,
                                  int min_followers = 1000,
                                  double min_engagement_rate = 0.01);
    
    // GMGN smart money streaming - for insider tracking
    void start_smart_money_stream(const std::vector<std::string>& watched_wallets = {},
                                 double min_transaction_size = 10000.0);
    void add_smart_wallet_to_watch(const std::string& wallet_address);
    void remove_smart_wallet_from_watch(const std::string& wallet_address);
    
    // DexScreener real-time pair monitoring
    void start_dexscreener_stream(const std::string& chain = "solana",
                                 double min_liquidity = 50000.0);
    void monitor_new_pairs(bool enabled, std::chrono::minutes max_age = std::chrono::minutes(5));
    void set_volume_threshold(double min_volume_24h);
    
    // Reddit social sentiment streaming
    void start_reddit_stream(const std::vector<std::string>& subreddits,
                            const std::vector<std::string>& keywords);
    void configure_reddit_filters(int min_upvotes = 10, int min_comments = 5);
    
    // News feed streaming for market catalysts
    void start_news_stream(const std::vector<std::string>& sources,
                          const std::vector<std::string>& keywords);
    
    // Data processing and aggregation - ultra-fast signal fusion
    void process_live_market_data(const LiveMarketData& data);
    void process_live_sentiment_data(const LiveSentimentData& data);
    void process_live_smart_money_data(const LiveSmartMoneyData& data);
    
    // Advanced signal generation - inspired by GMGN's AI analysis
    AggregatedSignal generate_real_time_signal(const std::string& symbol);
    std::vector<AggregatedSignal> get_top_signals_live(int limit = 10);
    std::vector<AggregatedSignal> scan_emerging_opportunities();
    
    // Multi-source consensus and validation
    bool validate_signal_consensus(const AggregatedSignal& signal, int min_sources = 2);
    double calculate_source_reliability(const std::string& source);
    void update_source_weights(const std::unordered_map<std::string, double>& weights);
    
    // Real-time filtering and quality control
    bool is_signal_actionable(const AggregatedSignal& signal);
    bool detect_market_manipulation(const std::string& symbol);
    bool validate_data_freshness(const std::chrono::microseconds& timestamp);
    
    // Callback registration for real-time processing
    void register_market_data_callback(std::function<void(const LiveMarketData&)> callback);
    void register_sentiment_callback(std::function<void(const LiveSentimentData&)> callback);
    void register_smart_money_callback(std::function<void(const LiveSmartMoneyData&)> callback);
    void register_signal_callback(std::function<void(const AggregatedSignal&)> callback);
    
    // Performance optimization for high-frequency processing
    void enable_low_latency_mode(bool enabled);
    void set_processing_priority(int priority); // 1-10, 10 = highest
    void configure_batch_processing(int batch_size = 100, std::chrono::milliseconds interval = std::chrono::milliseconds(10));
    
    // Stream health monitoring and auto-recovery
    void get_stream_health(StreamType type, StreamHealth& health_out) const;
    void get_all_stream_health(std::unordered_map<StreamType, StreamHealth>& health_out) const;
    void set_auto_reconnect(bool enabled, std::chrono::seconds retry_interval = std::chrono::seconds(5));
    
    // Data quality and validation
    void set_data_validation_level(int level); // 1-5, 5 = strictest
    void enable_anomaly_detection(bool enabled);
    void set_signal_ttl(std::chrono::seconds ttl);
    
    // Rate limiting and backpressure management
    void set_max_messages_per_second(int limit);
    void enable_backpressure_control(bool enabled);
    void set_queue_size_limits(const std::unordered_map<StreamType, size_t>& limits);
    
    // Historical data integration for context
    void enable_historical_context(bool enabled, std::chrono::hours lookback = std::chrono::hours(24));
    std::vector<LiveMarketData> get_historical_market_data(const std::string& symbol, 
                                                          std::chrono::minutes lookback);
    
    // Emergency controls
    void emergency_stop_all_streams();
    void pause_stream_processing();
    void resume_stream_processing();
    void clear_all_queues();
    
    // Metrics and diagnostics
    struct AggregatorMetrics {
        std::atomic<uint64_t> total_messages_processed{0};
        std::atomic<uint64_t> signals_generated{0};
        std::atomic<double> avg_processing_latency_us{0.0};
        std::atomic<double> signal_generation_rate{0.0};
        std::atomic<uint64_t> consensus_validations{0};
        std::atomic<uint64_t> quality_rejections{0};
    };
    
    void get_metrics(AggregatorMetrics& metrics_out) const;
    std::string get_performance_summary() const;
    void run_latency_benchmark();
    
    // Configuration management
    void load_configuration(const std::string& config_file);
    void save_configuration(const std::string& config_file) const;
    void apply_fastest_bot_settings(); // Pre-configured for maximum speed
};

// Specialized stream processors for different data types
class TwitterStreamProcessor {
public:
    static LiveSentimentData process_tweet(const std::string& tweet_json);
    static bool detect_viral_content(const LiveSentimentData& data);
    static double calculate_influence_score(const std::string& username, int followers, int engagement);
    static std::vector<std::string> extract_crypto_mentions(const std::string& text);
};

class SmartMoneyStreamProcessor {
public:
    static LiveSmartMoneyData process_transaction(const std::string& transaction_json);
    static bool is_known_smart_wallet(const std::string& wallet_address);
    static double calculate_smart_money_confidence(const LiveSmartMoneyData& data);
    static std::vector<LiveSmartMoneyData> detect_coordinated_buying(
        const std::vector<LiveSmartMoneyData>& transactions);
};

class MarketDataStreamProcessor {
public:
    static LiveMarketData process_price_update(const std::string& price_json);
    static bool detect_unusual_volume(const LiveMarketData& data);
    static double calculate_momentum_score(const std::vector<LiveMarketData>& history);
    static bool validate_price_movement(const LiveMarketData& current, const LiveMarketData& previous);
};

// High-performance message queue for real-time processing
template<typename T>
class HighSpeedMessageQueue {
private:
    struct alignas(64) QueueNode {
        std::atomic<QueueNode*> next{nullptr};
        T data;
    };
    
    alignas(64) std::atomic<QueueNode*> head_{nullptr};
    alignas(64) std::atomic<QueueNode*> tail_{nullptr};
    alignas(64) std::atomic<size_t> size_{0};
    
public:
    HighSpeedMessageQueue();
    ~HighSpeedMessageQueue();
    
    void push(const T& item);
    bool try_pop(T& item);
    size_t size() const;
    bool empty() const;
    void clear();
};

// Real-time signal fusion engine
class SignalFusionEngine {
public:
    static AggregatedSignal fuse_multi_source_signals(
        const std::vector<LiveMarketData>& market_data,
        const std::vector<LiveSentimentData>& sentiment_data,
        const std::vector<LiveSmartMoneyData>& smart_money_data);
    
    static double calculate_signal_confidence(const AggregatedSignal& signal);
    static bool validate_signal_quality(const AggregatedSignal& signal);
    static std::chrono::microseconds estimate_signal_lifetime(const AggregatedSignal& signal);
};

} // namespace hfx::ai
