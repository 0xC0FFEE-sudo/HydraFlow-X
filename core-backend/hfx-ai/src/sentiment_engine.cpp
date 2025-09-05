/**
 * @file sentiment_engine.cpp
 * @brief Implementation of ultra-low latency sentiment analysis engine
 */

#include "sentiment_engine.hpp"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <numeric>
#include <regex>
#include <unordered_set>
#include <thread>
#include <chrono>
#include <cmath>
#include <queue>
#include <mutex>

#ifdef HFX_PYTHON_EMBED
#include <Python.h>
#endif

namespace hfx::ai {

// Sentiment scoring keywords and patterns
static const std::unordered_map<std::string, double> BULLISH_KEYWORDS = {
    {"moon", 0.8}, {"bullish", 0.7}, {"pump", 0.6}, {"buy", 0.5},
    {"rocket", 0.7}, {"diamond hands", 0.8}, {"hodl", 0.6}, {"bull run", 0.9},
    {"breakout", 0.7}, {"rally", 0.6}, {"surge", 0.7}, {"moon mission", 0.9},
    {"to the moon", 0.8}, {"lambo", 0.6}, {"ath", 0.7}, {"all time high", 0.7}
};

static const std::unordered_map<std::string, double> BEARISH_KEYWORDS = {
    {"dump", -0.7}, {"bearish", -0.7}, {"sell", -0.5}, {"crash", -0.9},
    {"rekt", -0.8}, {"paper hands", -0.6}, {"fud", -0.7}, {"bear market", -0.9},
    {"breakdown", -0.7}, {"dip", -0.4}, {"correction", -0.5}, {"bubble", -0.8},
    {"rugpull", -0.9}, {"scam", -0.9}, {"dead cat bounce", -0.6}
};

/**
 * @brief Fast keyword-based sentiment scoring
 */
class FastSentimentScorer {
public:
    static SentimentScore score_text(const std::string& text, const std::string& source, 
                                   const std::string& symbol) {
        SentimentScore score;
        score.source = source;
        score.symbol = symbol;
        score.timestamp_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch()).count();
        score.raw_text = text;
        
        // Convert to lowercase for matching
        std::string lower_text = text;
        std::transform(lower_text.begin(), lower_text.end(), lower_text.begin(), ::tolower);
        
        double sentiment_sum = 0.0;
        double weight_sum = 0.0;
        std::vector<std::string> found_keywords;
        
        // Check bullish keywords
        for (const auto& [keyword, weight] : BULLISH_KEYWORDS) {
            if (lower_text.find(keyword) != std::string::npos) {
                sentiment_sum += weight;
                weight_sum += std::abs(weight);
                found_keywords.push_back(keyword);
            }
        }
        
        // Check bearish keywords
        for (const auto& [keyword, weight] : BEARISH_KEYWORDS) {
            if (lower_text.find(keyword) != std::string::npos) {
                sentiment_sum += weight;
                weight_sum += std::abs(weight);
                found_keywords.push_back(keyword);
            }
        }
        
        // Calculate final sentiment and confidence
        if (weight_sum > 0) {
            score.sentiment = std::clamp(sentiment_sum / weight_sum, -1.0, 1.0);
            score.confidence = std::min(weight_sum / 3.0, 1.0); // Normalize confidence
        } else {
            score.sentiment = 0.0;
            score.confidence = 0.1; // Low confidence for neutral
        }
        
        // Calculate urgency based on exclamation marks, caps, etc.
        size_t exclamations = std::count(text.begin(), text.end(), '!');
        size_t caps_count = std::count_if(text.begin(), text.end(), ::isupper);
        score.urgency = std::min((exclamations + caps_count / 10.0) / text.length(), 1.0);
        
        // Join keywords
        if (!found_keywords.empty()) {
            std::ostringstream oss;
            for (size_t i = 0; i < found_keywords.size(); ++i) {
                if (i > 0) oss << ", ";
                oss << found_keywords[i];
            }
            score.keywords = oss.str();
        }
        
        return score;
    }
};

/**
 * @brief Sentiment aggregator for combining multiple scores
 */
class SentimentAggregator {
private:
    mutable std::mutex scores_mutex_;
    std::unordered_map<std::string, std::vector<SentimentScore>> symbol_scores_;
    std::unordered_map<std::string, SentimentSignal> cached_signals_;
    uint32_t aggregation_window_ms_ = 5000; // 5 second window
    
public:
    void add_score(const SentimentScore& score) {
        std::lock_guard<std::mutex> lock(scores_mutex_);
        
        auto& scores = symbol_scores_[score.symbol];
        scores.push_back(score);
        
        // Remove old scores outside the window
        auto now = std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch()).count();
        auto cutoff = now - (aggregation_window_ms_ * 1000000ULL);
        
        scores.erase(std::remove_if(scores.begin(), scores.end(),
            [cutoff](const SentimentScore& s) { return s.timestamp_ns < cutoff; }), scores.end());
        
        // Update cached signal
        if (!scores.empty()) {
            cached_signals_[score.symbol] = aggregate_scores(score.symbol, scores);
        }
    }
    
    SentimentSignal get_signal(const std::string& symbol) const {
        std::lock_guard<std::mutex> lock(scores_mutex_);
        auto it = cached_signals_.find(symbol);
        return it != cached_signals_.end() ? it->second : SentimentSignal{};
    }
    
    void set_aggregation_window(uint32_t window_ms) {
        aggregation_window_ms_ = window_ms;
    }
    
private:
    SentimentSignal aggregate_scores(const std::string& symbol, 
                                   const std::vector<SentimentScore>& scores) {
        SentimentSignal signal;
        signal.symbol = symbol;
        signal.timestamp_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch()).count();
        
        if (scores.empty()) return signal;
        
        // Calculate weighted sentiment
        double weighted_sum = 0.0;
        double weight_sum = 0.0;
        
        for (const auto& score : scores) {
            double weight = score.confidence;
            weighted_sum += score.sentiment * weight;
            weight_sum += weight;
        }
        
        signal.weighted_sentiment = weight_sum > 0 ? weighted_sum / weight_sum : 0.0;
        
        // Calculate momentum (sentiment change over time)
        if (scores.size() >= 2) {
            double early_sentiment = 0.0;
            double late_sentiment = 0.0;
            int early_count = 0, late_count = 0;
            
            auto mid_time = (scores.front().timestamp_ns + scores.back().timestamp_ns) / 2;
            
            for (const auto& score : scores) {
                if (score.timestamp_ns < mid_time) {
                    early_sentiment += score.sentiment;
                    early_count++;
                } else {
                    late_sentiment += score.sentiment;
                    late_count++;
                }
            }
            
            if (early_count > 0 && late_count > 0) {
                signal.momentum = (late_sentiment / late_count) - (early_sentiment / early_count);
            }
        }
        
        // Calculate divergence (variance in sentiment)
        if (scores.size() > 1) {
            double mean_sentiment = signal.weighted_sentiment;
            double variance = 0.0;
            
            for (const auto& score : scores) {
                variance += std::pow(score.sentiment - mean_sentiment, 2);
            }
            
            signal.divergence = std::sqrt(variance / scores.size());
        }
        
        // Volume factor (number of messages)
        signal.volume_factor = std::min(static_cast<double>(scores.size()) / 10.0, 2.0);
        
        // Store contributing scores
        signal.contributing_scores = scores;
        
        return signal;
    }
};

/**
 * @brief Implementation class for SentimentEngine
 */
class SentimentEngine::Impl {
public:
    std::unique_ptr<SentimentAggregator> aggregator_;
    SentimentCallback callback_;
    SentimentStats stats_;
    std::vector<DataSource> data_sources_;
    std::atomic<bool> running_{false};
    std::unique_ptr<std::thread> processing_thread_;
    
    // Processing queue (using standard queue for simplicity)
    std::queue<std::tuple<std::string, std::string, std::string>> message_queue_;
    mutable std::mutex queue_mutex_;
    
    Impl() : aggregator_(std::make_unique<SentimentAggregator>()) {
    }
    
    void start_processing() {
        running_.store(true);
        processing_thread_ = std::make_unique<std::thread>(&Impl::processing_loop, this);
    }
    
    void stop_processing() {
        running_.store(false);
        if (processing_thread_ && processing_thread_->joinable()) {
            processing_thread_->join();
        }
    }
    
    void processing_loop() {
        while (running_.load()) {
            std::tuple<std::string, std::string, std::string> message;
            bool has_message = false;
            
            {
                std::lock_guard<std::mutex> lock(queue_mutex_);
                if (!message_queue_.empty()) {
                    message = message_queue_.front();
                    message_queue_.pop();
                    has_message = true;
                }
            }
            
            if (has_message) {
                auto start_time = std::chrono::high_resolution_clock::now();
                
                const auto& [text, source, symbol] = message;
                
                // Fast sentiment scoring
                auto score = FastSentimentScorer::score_text(text, source, symbol);
                aggregator_->add_score(score);
                
                // Generate signal if callback is set
                if (callback_) {
                    auto signal = aggregator_->get_signal(symbol);
                    if (!signal.symbol.empty()) {
                        callback_(signal);
                    }
                }
                
                // Update stats
                auto end_time = std::chrono::high_resolution_clock::now();
                auto processing_time = std::chrono::duration_cast<std::chrono::nanoseconds>(
                    end_time - start_time).count();
                
                stats_.total_messages_processed.fetch_add(1);
                stats_.avg_processing_latency_ns.store(processing_time);
                
                if (symbol == "BTC") {
                    stats_.current_sentiment_btc.store(score.sentiment);
                } else if (symbol == "ETH") {
                    stats_.current_sentiment_eth.store(score.sentiment);
                }
            } else {
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            }
        }
    }
};

// SentimentEngine implementation
SentimentEngine::SentimentEngine() : pimpl_(std::make_unique<Impl>()) {}

SentimentEngine::~SentimentEngine() {
    shutdown();
}

bool SentimentEngine::initialize() {
    HFX_LOG_INFO("[SentimentEngine] Initializing ultra-low latency sentiment analysis...");
    
#ifdef HFX_PYTHON_EMBED
    // Initialize Python for FinGPT model integration
    Py_Initialize();
    if (!Py_IsInitialized()) {
        HFX_LOG_INFO("[SentimentEngine] Failed to initialize Python interpreter");
        return false;
    }
#endif
    
    pimpl_->start_processing();
    
    HFX_LOG_INFO("[SentimentEngine] ✅ Sentiment analysis engine ready");
    HFX_LOG_INFO("[SentimentEngine]    • Fast keyword scoring: ON");
    HFX_LOG_INFO("[SentimentEngine]    • Real-time aggregation: ON");
    HFX_LOG_INFO("[SentimentEngine]    • Multi-source support: ON");
    
    return true;
}

void SentimentEngine::shutdown() {
    pimpl_->stop_processing();
    
#ifdef HFX_PYTHON_EMBED
    if (Py_IsInitialized()) {
        Py_Finalize();
    }
#endif
    
    HFX_LOG_INFO("[SentimentEngine] Sentiment analysis engine stopped");
}

void SentimentEngine::add_data_source(const DataSource& source) {
    pimpl_->data_sources_.push_back(source);
    HFX_LOG_INFO("[SentimentEngine] Added data source: " << source.name 
              << " (weight: " << source.weight << ")" << std::endl;
}

void SentimentEngine::register_sentiment_callback(SentimentCallback callback) {
    pimpl_->callback_ = callback;
}

void SentimentEngine::process_raw_text(const std::string& text, const std::string& source, 
                                     const std::string& symbol) {
    // Queue message for processing
    {
        std::lock_guard<std::mutex> lock(pimpl_->queue_mutex_);
        pimpl_->message_queue_.push(std::make_tuple(text, source, symbol));
    }
}

SentimentSignal SentimentEngine::get_current_sentiment(const std::string& symbol) const {
    return pimpl_->aggregator_->get_signal(symbol);
}

void SentimentEngine::get_statistics(SentimentStats& stats_out) const {
    stats_out.total_messages_processed.store(pimpl_->stats_.total_messages_processed.load());
    stats_out.total_sentiment_signals.store(pimpl_->stats_.total_sentiment_signals.load());
    stats_out.avg_processing_latency_ns.store(pimpl_->stats_.avg_processing_latency_ns.load());
    stats_out.model_inference_latency_ns.store(pimpl_->stats_.model_inference_latency_ns.load());
    stats_out.data_fetch_latency_ns.store(pimpl_->stats_.data_fetch_latency_ns.load());
    stats_out.current_sentiment_btc.store(pimpl_->stats_.current_sentiment_btc.load());
    stats_out.current_sentiment_eth.store(pimpl_->stats_.current_sentiment_eth.load());
    stats_out.messages_per_second.store(pimpl_->stats_.messages_per_second.load());
    stats_out.cache_hit_rate.store(pimpl_->stats_.cache_hit_rate.load());
    stats_out.model_accuracy.store(pimpl_->stats_.model_accuracy.load());
}

void SentimentEngine::set_aggregation_window(uint32_t window_ms) {
    pimpl_->aggregator_->set_aggregation_window(window_ms);
}

// DataFeedsManager implementation class
class DataFeedsManager::FeedImpl {
public:
    std::vector<FeedConfig> feeds_;
    std::unordered_map<std::string, std::unique_ptr<std::thread>> feed_threads_;
    std::unordered_map<std::string, FeedStats> feed_stats_;
    DataCallback callback_;
    std::atomic<bool> running_{false};
    
    void start_feed_thread(const std::string& name) {
        auto it = std::find_if(feeds_.begin(), feeds_.end(),
            [&name](const FeedConfig& f) { return f.name == name; });
        
        if (it != feeds_.end()) {
            feed_threads_[name] = std::make_unique<std::thread>(
                &FeedImpl::feed_worker, this, *it);
        }
    }
    
    void feed_worker(const FeedConfig& config) {
        HFX_LOG_INFO("[DataFeeds] Starting feed: " << config.name << std::endl;
        
        auto& stats = feed_stats_[config.name];
        stats.is_connected.store(true);
        
        // Demo implementation - generate sample data
        while (running_.load()) {
            if (config.type == "twitter") {
                simulate_twitter_feed(config, stats);
            } else if (config.type == "reddit") {
                simulate_reddit_feed(config, stats);
            } else if (config.type == "dexscreener") {
                simulate_dexscreener_feed(config, stats);
            } else if (config.type == "gmgn") {
                simulate_gmgn_feed(config, stats);
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(config.polling_interval_ms));
        }
        
        stats.is_connected.store(false);
        HFX_LOG_INFO("[DataFeeds] Stopped feed: " << config.name << std::endl;
    }
    
private:
    void simulate_twitter_feed(const FeedConfig& config, FeedStats& stats) {
        // Simulate Twitter sentiment data
        std::vector<std::string> sample_tweets = {
            "BTC looking bullish! 🚀 Moon mission incoming!",
            "ETH breaking resistance, time to buy the dip",
            "This market is so bearish, everything dumping",
            "Diamond hands holding through this volatility 💎",
            "New ATH incoming for crypto, bullish signals everywhere"
        };
        
        for (const auto& symbol : config.symbols_filter) {
            auto tweet = sample_tweets[rand() % sample_tweets.size()];
            tweet += " #" + symbol;
            
            if (callback_) {
                callback_(config.name, symbol, tweet, 
                         std::chrono::duration_cast<std::chrono::nanoseconds>(
                             std::chrono::high_resolution_clock::now().time_since_epoch()).count());
            }
            
            stats.messages_received.fetch_add(1);
            stats.last_message_time.store(std::time(nullptr));
        }
    }
    
    void simulate_reddit_feed(const FeedConfig& config, FeedStats& stats) {
        std::vector<std::string> sample_posts = {
            "Technical analysis shows strong support at current levels",
            "Whales are accumulating, smart money moving in",
            "FUD spreading but fundamentals remain strong",
            "This correction is healthy for long term growth",
            "Major announcement coming, insider sources confirm"
        };
        
        for (const auto& symbol : config.symbols_filter) {
            auto post = sample_posts[rand() % sample_posts.size()];
            
            if (callback_) {
                callback_(config.name, symbol, post, 
                         std::chrono::duration_cast<std::chrono::nanoseconds>(
                             std::chrono::high_resolution_clock::now().time_since_epoch()).count());
            }
            
            stats.messages_received.fetch_add(1);
        }
    }
    
    void simulate_dexscreener_feed(const FeedConfig& config, FeedStats& stats) {
        std::vector<std::string> sample_data = {
            "High volume surge detected, price momentum building",
            "Liquidity pool showing unusual activity patterns", 
            "New token pair launched, early volume indicators",
            "Whale transaction detected, significant price impact",
            "Arbitrage opportunity identified across DEX pairs"
        };
        
        for (const auto& symbol : config.symbols_filter) {
            auto data = sample_data[rand() % sample_data.size()];
            
            if (callback_) {
                callback_(config.name, symbol, data, 
                         std::chrono::duration_cast<std::chrono::nanoseconds>(
                             std::chrono::high_resolution_clock::now().time_since_epoch()).count());
            }
            
            stats.messages_received.fetch_add(1);
        }
    }
    
    void simulate_gmgn_feed(const FeedConfig& config, FeedStats& stats) {
        std::vector<std::string> sample_data = {
            "Smart money wallet made large purchase",
            "Insider trading patterns detected in recent transactions",
            "Whale wallet showing accumulation behavior",
            "Large transfer to exchange detected - possible sell pressure",
            "New whale wallet created with significant holdings"
        };
        
        for (const auto& symbol : config.symbols_filter) {
            auto data = sample_data[rand() % sample_data.size()];
            
            if (callback_) {
                callback_(config.name, symbol, data, 
                         std::chrono::duration_cast<std::chrono::nanoseconds>(
                             std::chrono::high_resolution_clock::now().time_since_epoch()).count());
            }
            
            stats.messages_received.fetch_add(1);
        }
    }
};

// DataFeedsManager implementation
DataFeedsManager::DataFeedsManager() : pimpl_(std::make_unique<FeedImpl>()) {}

DataFeedsManager::~DataFeedsManager() {
    shutdown();
}

bool DataFeedsManager::initialize() {
    HFX_LOG_INFO("[DataFeeds] Initializing multi-source data feeds...");
    pimpl_->running_.store(true);
    return true;
}

void DataFeedsManager::shutdown() {
    pimpl_->running_.store(false);
    for (auto& [name, thread] : pimpl_->feed_threads_) {
        if (thread && thread->joinable()) {
            thread->join();
        }
    }
    pimpl_->feed_threads_.clear();
}

void DataFeedsManager::add_feed(const FeedConfig& config) {
    pimpl_->feeds_.push_back(config);
    HFX_LOG_INFO("[DataFeeds] Added feed: " << config.name 
              << " (" << config.type << ")" << std::endl;
}

void DataFeedsManager::start_feed(const std::string& name) {
    pimpl_->start_feed_thread(name);
}

void DataFeedsManager::register_data_callback(DataCallback callback) {
    pimpl_->callback_ = callback;
}

void DataFeedsManager::get_feed_stats(const std::string& name, FeedStats& stats_out) const {
    auto it = pimpl_->feed_stats_.find(name);
    if (it != pimpl_->feed_stats_.end()) {
        stats_out.messages_received.store(it->second.messages_received.load());
        stats_out.messages_filtered.store(it->second.messages_filtered.load());
        stats_out.avg_latency_ms.store(it->second.avg_latency_ms.load());
        stats_out.is_connected.store(it->second.is_connected.load());
        stats_out.last_message_time.store(it->second.last_message_time.load());
    }
}

} // namespace hfx::ai
