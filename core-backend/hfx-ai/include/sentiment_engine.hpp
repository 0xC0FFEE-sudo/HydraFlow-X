/**
 * @file sentiment_engine.hpp
 * @brief Ultra-low latency sentiment analysis engine for crypto trading
 * @version 1.0.0
 * 
 * Multi-source sentiment analysis with microsecond-level processing.
 * Integrates FinGPT models, social media feeds, news sources, and DeFi data.
 */

#pragma once

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <atomic>
#include <chrono>
#include <functional>
#include <thread>
#include <mutex>
#include <queue>

#include "event_engine.hpp"
#include "lockfree_queue.hpp"

namespace hfx::ai {

/**
 * @brief Sentiment score with confidence and source attribution
 */
struct SentimentScore {
    double sentiment;        // [-1.0, 1.0] where -1 = bearish, 1 = bullish
    double confidence;       // [0.0, 1.0] confidence level
    std::string source;      // Source identifier (twitter, reddit, news, etc.)
    std::string symbol;      // Crypto symbol (BTC, ETH, etc.)
    uint64_t timestamp_ns;   // Nanosecond timestamp
    std::string raw_text;    // Original text (for debugging)
    std::string keywords;    // Extracted keywords
    double urgency;          // [0.0, 1.0] time sensitivity
};

/**
 * @brief Aggregated sentiment signal ready for trading decisions
 */
struct SentimentSignal {
    std::string symbol;
    double weighted_sentiment;  // Volume-weighted sentiment
    double momentum;           // Rate of sentiment change
    double divergence;         // Cross-source sentiment divergence
    double volume_factor;      // Social volume multiplier
    uint64_t timestamp_ns;
    std::vector<SentimentScore> contributing_scores;
};

/**
 * @brief Data source configuration
 */
struct DataSource {
    std::string name;
    std::string endpoint_url;
    std::string api_key;
    bool enabled;
    double weight;           // Source weight in aggregation
    uint32_t rate_limit_ms;  // Rate limiting
    std::string model_type;  // FinGPT model to use
};

/**
 * @brief Real-time sentiment processing statistics
 */
struct SentimentStats {
    std::atomic<uint64_t> total_messages_processed{0};
    std::atomic<uint64_t> total_sentiment_signals{0};
    std::atomic<uint64_t> avg_processing_latency_ns{0};
    std::atomic<uint64_t> model_inference_latency_ns{0};
    std::atomic<uint64_t> data_fetch_latency_ns{0};
    std::atomic<double> current_sentiment_btc{0.0};
    std::atomic<double> current_sentiment_eth{0.0};
    
    // Performance metrics
    std::atomic<uint32_t> messages_per_second{0};
    std::atomic<uint32_t> cache_hit_rate{0};
    std::atomic<uint32_t> model_accuracy{0};
};

/**
 * @brief Callback for sentiment signals
 */
using SentimentCallback = std::function<void(const SentimentSignal&)>;

/**
 * @brief Ultra-low latency sentiment analysis engine
 * 
 * Features:
 * - Multi-source data ingestion (Twitter, Reddit, News, GMGN, DexScreener)
 * - FinGPT model integration for financial sentiment analysis
 * - Microsecond-level signal processing
 * - Real-time sentiment aggregation and momentum tracking
 * - Cache-optimized inference pipeline
 * - Anomaly detection for sentiment manipulation
 */
class SentimentEngine {
public:
    explicit SentimentEngine();
    ~SentimentEngine();
    
    // Core lifecycle
    bool initialize();
    void shutdown();
    
    // Data source management
    void add_data_source(const DataSource& source);
    void remove_data_source(const std::string& name);
    void update_source_weight(const std::string& name, double weight);
    
    // Model management
    bool load_fingpt_model(const std::string& model_path);
    bool load_custom_model(const std::string& model_name, const std::string& config);
    void set_inference_batch_size(uint32_t batch_size);
    
    // Signal processing
    void register_sentiment_callback(SentimentCallback callback);
    void process_raw_text(const std::string& text, const std::string& source, 
                         const std::string& symbol = "");
    
    // Queries
    SentimentSignal get_current_sentiment(const std::string& symbol) const;
    std::vector<SentimentSignal> get_sentiment_history(const std::string& symbol, 
                                                      uint32_t lookback_seconds) const;
    void get_statistics(SentimentStats& stats_out) const;
    
    // Configuration
    void set_sentiment_threshold(double threshold);
    void set_momentum_window(uint32_t window_ms);
    void set_aggregation_window(uint32_t window_ms);
    void enable_anomaly_detection(bool enabled);
    
    // Performance tuning
    void set_thread_count(uint32_t threads);
    void set_cache_size(uint32_t cache_entries);
    void enable_gpu_acceleration(bool enabled);
    
private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

/**
 * @brief Multi-source data feed manager
 */
class DataFeedsManager {
public:
    struct FeedConfig {
        std::string name;
        std::string type;        // twitter, reddit, news, dexscreener, gmgn
        std::string endpoint;
        std::string credentials;
        std::vector<std::string> symbols_filter;
        uint32_t polling_interval_ms;
        bool real_time_streaming;
    };
    
    explicit DataFeedsManager();
    ~DataFeedsManager();
    
    bool initialize();
    void shutdown();
    
    // Feed management
    void add_feed(const FeedConfig& config);
    void remove_feed(const std::string& name);
    void start_feed(const std::string& name);
    void stop_feed(const std::string& name);
    
    // Data callbacks
    using DataCallback = std::function<void(const std::string& source, 
                                          const std::string& symbol,
                                          const std::string& text,
                                          uint64_t timestamp)>;
    void register_data_callback(DataCallback callback);
    
    // Statistics
    struct FeedStats {
        std::atomic<uint64_t> messages_received{0};
        std::atomic<uint64_t> messages_filtered{0};
        std::atomic<uint64_t> avg_latency_ms{0};
        std::atomic<bool> is_connected{false};
        std::atomic<uint64_t> last_message_time{0};
    };
    
    void get_feed_stats(const std::string& name, FeedStats& stats_out) const;
    std::vector<std::string> get_active_feeds() const;
    
private:
    class FeedImpl;
    std::unique_ptr<FeedImpl> pimpl_;
};

/**
 * @brief Specialized crypto market sentiment analyzer
 */
class CryptoSentimentAnalyzer {
public:
    struct CryptoSignal {
        std::string symbol;
        double price_sentiment;      // Price action sentiment
        double social_sentiment;     // Social media sentiment
        double news_sentiment;       // News sentiment
        double defi_sentiment;       // DeFi protocol sentiment
        double whale_sentiment;      // Large holder activity sentiment
        double fear_greed_index;     // Market fear/greed
        double momentum_score;       // Combined momentum
        uint64_t timestamp_ns;
    };
    
    explicit CryptoSentimentAnalyzer();
    ~CryptoSentimentAnalyzer();
    
    bool initialize();
    void set_sentiment_engine(std::shared_ptr<SentimentEngine> engine);
    
    // Crypto-specific analysis
    CryptoSignal analyze_crypto_sentiment(const std::string& symbol);
    double calculate_fear_greed_index();
    double analyze_whale_movements(const std::string& symbol);
    double analyze_defi_sentiment(const std::string& symbol);
    
    // Market event detection
    bool detect_fomo_event(const std::string& symbol);
    bool detect_fud_event(const std::string& symbol);
    bool detect_pump_dump(const std::string& symbol);
    
private:
    class AnalyzerImpl;
    std::unique_ptr<AnalyzerImpl> pimpl_;
};

/**
 * @brief FinGPT model wrapper for ultra-low latency inference
 */
class FinGPTModel {
public:
    struct ModelConfig {
        std::string model_name;     // llama2-7b, chatglm2-6b, etc.
        std::string model_path;
        uint32_t max_tokens;
        float temperature;
        uint32_t batch_size;
        bool use_quantization;
        bool use_flash_attention;
    };
    
    explicit FinGPTModel(const ModelConfig& config);
    ~FinGPTModel();
    
    bool load_model();
    void unload_model();
    
    // Fast inference
    SentimentScore analyze_sentiment(const std::string& text, const std::string& symbol);
    std::vector<SentimentScore> batch_analyze(const std::vector<std::string>& texts,
                                            const std::vector<std::string>& symbols);
    
    // Model statistics
    struct ModelStats {
        std::atomic<uint64_t> total_inferences{0};
        std::atomic<uint64_t> avg_inference_time_us{0};
        std::atomic<uint64_t> cache_hits{0};
        std::atomic<uint64_t> model_load_time_ms{0};
        std::atomic<double> memory_usage_mb{0.0};
    };
    
    ModelStats get_stats() const;
    
private:
    class ModelImpl;
    std::unique_ptr<ModelImpl> pimpl_;
};

} // namespace hfx::ai
