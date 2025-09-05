#include "real_time_data_aggregator.hpp"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <algorithm>
#include <random>
#include <regex>
#include <iostream>
#include <sstream>
#include <fstream>
#include <unordered_set>
#include <queue>

namespace hfx::ai {

struct RealTimeDataAggregator::Impl {
    // Core state management
    std::atomic<bool> is_running{false};
    std::atomic<bool> is_paused{false};
    std::atomic<bool> low_latency_mode{false};
    std::atomic<bool> auto_reconnect_enabled{true};
    
    // Processing threads for maximum throughput
    std::vector<std::thread> processing_threads;
    std::thread health_monitor_thread;
    std::thread signal_fusion_thread;
    std::thread quality_control_thread;
    
    // High-speed message queues for different data types
    HighSpeedMessageQueue<LiveMarketData> market_data_queue;
    HighSpeedMessageQueue<LiveSentimentData> sentiment_data_queue;
    HighSpeedMessageQueue<LiveSmartMoneyData> smart_money_queue;
    HighSpeedMessageQueue<AggregatedSignal> signal_output_queue;
    
    // Stream health monitoring
    std::unordered_map<StreamType, StreamHealth> stream_health;
    mutable std::mutex health_mutex;
    
    // Real-time data storage with TTL
    struct TimestampedData {
        std::chrono::microseconds timestamp;
        std::chrono::seconds ttl;
    };
    
    std::unordered_map<std::string, std::vector<LiveMarketData>> market_data_history;
    std::unordered_map<std::string, std::vector<LiveSentimentData>> sentiment_data_history;
    std::unordered_map<std::string, std::vector<LiveSmartMoneyData>> smart_money_history;
    mutable std::mutex data_history_mutex;
    
    // Configuration and settings
    int processing_priority = 8; // High priority by default
    std::chrono::seconds signal_ttl{30}; // 30 second signal TTL
    int data_validation_level = 3; // Medium validation
    size_t max_messages_per_second = 10000;
    bool backpressure_enabled = true;
    
    // Source reliability tracking
    std::unordered_map<std::string, double> source_reliability;
    std::unordered_map<std::string, double> source_weights;
    mutable std::mutex source_mutex;
    
    // Callbacks for real-time processing
    std::function<void(const LiveMarketData&)> market_data_callback;
    std::function<void(const LiveSentimentData&)> sentiment_callback;
    std::function<void(const LiveSmartMoneyData&)> smart_money_callback;
    std::function<void(const AggregatedSignal&)> signal_callback;
    
    // Performance metrics
    AggregatorMetrics metrics;
    mutable std::mutex metrics_mutex;
    
    // Smart wallets to monitor
    std::unordered_set<std::string> watched_smart_wallets;
    mutable std::mutex wallet_mutex;
    
    Impl() {
        initialize_default_settings();
        initialize_processing_threads();
    }
    
    ~Impl() {
        stop_all_processing();
    }
    
    void initialize_default_settings() {
        // Initialize source weights (inspired by fastest bot configurations)
        source_weights["twitter"] = 0.25;
        source_weights["gmgn"] = 0.30;
        source_weights["dexscreener"] = 0.35;
        source_weights["reddit"] = 0.15;
        source_weights["news"] = 0.20;
        
        // Initialize source reliability (will be updated based on performance)
        source_reliability["twitter"] = 0.75;
        source_reliability["gmgn"] = 0.90;
        source_reliability["dexscreener"] = 0.95;
        source_reliability["reddit"] = 0.65;
        source_reliability["news"] = 0.80;
        
        // Add known smart wallets (examples - would be real addresses in production)
        watched_smart_wallets.insert("smart_wallet_1");
        watched_smart_wallets.insert("smart_wallet_2");
        watched_smart_wallets.insert("smart_wallet_3");
    }
    
    void initialize_processing_threads() {
        int num_cores = std::thread::hardware_concurrency();
        processing_threads.reserve(num_cores);
        
        // Create specialized processing threads
        for (int i = 0; i < num_cores; ++i) {
            processing_threads.emplace_back(&Impl::processing_thread_loop, this, i);
        }
        
        HFX_LOG_INFO("[LOG] Message");
    }
    
    void start_all_processing() {
        if (!is_running.load()) {
            is_running.store(true);
            
            // Start monitoring and fusion threads
            health_monitor_thread = std::thread(&Impl::health_monitoring_loop, this);
            signal_fusion_thread = std::thread(&Impl::signal_fusion_loop, this);
            quality_control_thread = std::thread(&Impl::quality_control_loop, this);
            
            HFX_LOG_INFO("RealTimeDataAggregator: All processing started (Ultra-Fast Mode)");
        }
    }
    
    void stop_all_processing() {
        if (is_running.load()) {
            is_running.store(false);
            
            // Wait for threads to finish
            if (health_monitor_thread.joinable()) {
                health_monitor_thread.join();
            }
            if (signal_fusion_thread.joinable()) {
                signal_fusion_thread.join();
            }
            if (quality_control_thread.joinable()) {
                quality_control_thread.join();
            }
            
            for (auto& thread : processing_threads) {
                if (thread.joinable()) {
                    thread.join();
                }
            }
            
            HFX_LOG_INFO("RealTimeDataAggregator: All processing stopped");
        }
    }
    
    void processing_thread_loop(int thread_id) {
        HFX_LOG_INFO("[LOG] Message");
        
        // Set thread priority for low-latency processing
        set_thread_priority();
        
        while (is_running.load()) {
            try {
                if (is_paused.load()) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    continue;
                }
                
                // Process different data types in parallel
                process_market_data_batch();
                process_sentiment_data_batch();
                process_smart_money_batch();
                
                // Yield if not in low-latency mode
                if (!low_latency_mode.load()) {
                    std::this_thread::sleep_for(std::chrono::microseconds(100));
                }
                
            } catch (const std::exception& e) {
                HFX_LOG_ERROR("[ERROR] Message");
            }
        }
        
        HFX_LOG_INFO("[LOG] Message");
    }
    
    void process_market_data_batch() {
        LiveMarketData data;
        int processed = 0;
        const int batch_size = low_latency_mode.load() ? 1 : 10;
        
        while (processed < batch_size && market_data_queue.try_pop(data)) {
            auto start_time = std::chrono::high_resolution_clock::now();
            
            // Validate data quality
            if (validate_market_data(data)) {
                // Store in history for context
                store_market_data_history(data);
                
                // Call registered callback
                if (market_data_callback) {
                    market_data_callback(data);
                }
                
                // Update metrics
                update_processing_metrics(start_time);
                processed++;
            }
        }
    }
    
    void process_sentiment_data_batch() {
        LiveSentimentData data;
        int processed = 0;
        const int batch_size = low_latency_mode.load() ? 1 : 10;
        
        while (processed < batch_size && sentiment_data_queue.try_pop(data)) {
            auto start_time = std::chrono::high_resolution_clock::now();
            
            // Enhanced sentiment processing
            if (validate_sentiment_data(data)) {
                // Detect viral content
                data.is_viral = detect_viral_content_impl(data);
                
                // Store in history
                store_sentiment_data_history(data);
                
                // Call registered callback
                if (sentiment_callback) {
                    sentiment_callback(data);
                }
                
                update_processing_metrics(start_time);
                processed++;
            }
        }
    }
    
    void process_smart_money_batch() {
        LiveSmartMoneyData data;
        int processed = 0;
        const int batch_size = low_latency_mode.load() ? 1 : 5; // Smaller batches for smart money
        
        while (processed < batch_size && smart_money_queue.try_pop(data)) {
            auto start_time = std::chrono::high_resolution_clock::now();
            
            // Enhanced smart money processing
            if (validate_smart_money_data(data)) {
                // Check if it's from a watched wallet
                {
                    std::lock_guard<std::mutex> lock(wallet_mutex);
                    data.is_insider_wallet = watched_smart_wallets.count(data.wallet_address) > 0;
                }
                
                // Store in history
                store_smart_money_history(data);
                
                // Call registered callback
                if (smart_money_callback) {
                    smart_money_callback(data);
                }
                
                update_processing_metrics(start_time);
                processed++;
            }
        }
    }
    
    bool validate_market_data(const LiveMarketData& data) {
        // Basic validation checks
        if (data.symbol.empty() || data.price <= 0) return false;
        
        // Data freshness check
        if (!validate_data_freshness_impl(data.timestamp)) return false;
        
        // Additional validation based on level
        if (data_validation_level >= 3) {
            // Check for reasonable price movements
            if (std::abs(data.price_change_1h) > 1.0) { // >100% in 1 hour is suspicious
                return false;
            }
        }
        
        if (data_validation_level >= 4) {
            // Check volume sanity
            if (data.volume_24h < 0 || data.volume_24h > 1e12) { // Reasonable volume range
                return false;
            }
        }
        
        return true;
    }
    
    bool validate_sentiment_data(const LiveSentimentData& data) {
        if (data.symbol.empty() || data.source.empty()) return false;
        if (data.sentiment_score < -1.0 || data.sentiment_score > 1.0) return false;
        if (data.confidence < 0.0 || data.confidence > 1.0) return false;
        
        return validate_data_freshness_impl(data.timestamp);
    }
    
    bool validate_smart_money_data(const LiveSmartMoneyData& data) {
        if (data.symbol.empty() || data.wallet_address.empty()) return false;
        if (data.amount_usd <= 0) return false;
        if (data.direction != "buy" && data.direction != "sell") return false;
        
        return validate_data_freshness_impl(data.timestamp);
    }
    
    bool validate_data_freshness_impl(const std::chrono::microseconds& timestamp) {
        auto now = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch());
        auto age = now - timestamp;
        
        // Data should be less than 5 minutes old for real-time processing
        return age < std::chrono::minutes(5);
    }
    
    bool detect_viral_content_impl(const LiveSentimentData& data) {
        // Simple viral detection based on engagement
        if (data.engagement_count > 1000 && data.confidence > 0.7) {
            return true;
        }
        
        // Check for viral keywords
        std::vector<std::string> viral_keywords{"moon", "rocket", "pump", "breakout", "explosion"};
        for (const auto& keyword : viral_keywords) {
            if (data.raw_text.find(keyword) != std::string::npos) {
                return true;
            }
        }
        
        return false;
    }
    
    void store_market_data_history(const LiveMarketData& data) {
        std::lock_guard<std::mutex> lock(data_history_mutex);
        auto& history = market_data_history[data.symbol];
        history.push_back(data);
        
        // Keep only recent data (last 1000 entries or 24 hours)
        if (history.size() > 1000) {
            history.erase(history.begin(), history.begin() + 100);
        }
    }
    
    void store_sentiment_data_history(const LiveSentimentData& data) {
        std::lock_guard<std::mutex> lock(data_history_mutex);
        auto& history = sentiment_data_history[data.symbol];
        history.push_back(data);
        
        if (history.size() > 500) {
            history.erase(history.begin(), history.begin() + 50);
        }
    }
    
    void store_smart_money_history(const LiveSmartMoneyData& data) {
        std::lock_guard<std::mutex> lock(data_history_mutex);
        auto& history = smart_money_history[data.symbol];
        history.push_back(data);
        
        if (history.size() > 200) {
            history.erase(history.begin(), history.begin() + 20);
        }
    }
    
    void update_processing_metrics(const std::chrono::high_resolution_clock::time_point& start_time) {
        auto end_time = std::chrono::high_resolution_clock::now();
        auto latency = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
        
        std::lock_guard<std::mutex> lock(metrics_mutex);
        metrics.total_messages_processed.fetch_add(1);
        
        // Update rolling average latency
        auto current_avg = metrics.avg_processing_latency_us.load();
        auto total_processed = metrics.total_messages_processed.load();
        if (total_processed > 0) {
            double new_avg = ((current_avg * (total_processed - 1)) + latency.count()) / total_processed;
            metrics.avg_processing_latency_us.store(new_avg);
        }
    }
    
    void set_thread_priority() {
        // Set high priority for processing threads on macOS
        #ifdef __APPLE__
        pthread_t thread = pthread_self();
        struct sched_param param;
        param.sched_priority = processing_priority;
        pthread_setschedparam(thread, SCHED_FIFO, &param);
        #endif
    }
    
    void health_monitoring_loop() {
        HFX_LOG_INFO("Health monitoring started");
        
        while (is_running.load()) {
            try {
                monitor_all_streams();
                check_auto_reconnection();
                update_source_reliability();
                
            } catch (const std::exception& e) {
                HFX_LOG_ERROR("[ERROR] Message");
            }
            
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        
        HFX_LOG_INFO("Health monitoring stopped");
    }
    
    void signal_fusion_loop() {
        HFX_LOG_INFO("Signal fusion engine started");
        
        while (is_running.load()) {
            try {
                generate_real_time_signals();
                clean_expired_signals();
                
            } catch (const std::exception& e) {
                HFX_LOG_ERROR("[ERROR] Message");
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(100)); // 100ms fusion cycles
        }
        
        HFX_LOG_INFO("Signal fusion engine stopped");
    }
    
    void quality_control_loop() {
        HFX_LOG_INFO("Quality control started");
        
        while (is_running.load()) {
            try {
                validate_signal_quality();
                detect_anomalies();
                update_quality_metrics();
                
            } catch (const std::exception& e) {
                HFX_LOG_ERROR("[ERROR] Message");
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(500)); // 500ms QC cycles
        }
        
        HFX_LOG_INFO("Quality control stopped");
    }
    
    void monitor_all_streams() {
        std::lock_guard<std::mutex> lock(health_mutex);
        
        for (auto& [stream_type, health] : stream_health) {
            // Check if stream is receiving data
            auto now = std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
            
            if (now - health.last_message_time.load() > 60) { // No data for 60 seconds
                health.is_connected.store(false);
            }
            
            // Check message processing rate
            auto processing_rate = static_cast<double>(health.messages_processed.load()) / 
                                 std::max(1ULL, health.messages_received.load());
            
            if (processing_rate < 0.95) { // Less than 95% processing rate
                HFX_LOG_INFO("Warning: Stream processing rate low for stream type " 
                         << static_cast<int>(stream_type) << std::endl;
            }
        }
    }
    
    void check_auto_reconnection() {
        if (!auto_reconnect_enabled.load()) return;
        
        std::lock_guard<std::mutex> lock(health_mutex);
        for (auto& [stream_type, health] : stream_health) {
            if (!health.is_connected.load()) {
                // Attempt reconnection
                restart_stream_impl(stream_type);
                health.reconnection_count.fetch_add(1);
            }
        }
    }
    
    void restart_stream_impl(StreamType stream_type) {
        // Stream restart logic - would connect to actual APIs in production
        HFX_LOG_INFO("[LOG] Message");
        
        std::lock_guard<std::mutex> lock(health_mutex);
        auto& health = stream_health[stream_type];
        health.is_connected.store(true);
        health.last_message_time.store(
            std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::system_clock::now().time_since_epoch()).count());
    }
    
    void update_source_reliability() {
        std::lock_guard<std::mutex> lock(source_mutex);
        
        // Update reliability based on recent performance
        for (auto& [source, reliability] : source_reliability) {
            // Simulate reliability updates based on stream health
            auto health_it = stream_health.find(get_stream_type_for_source(source));
            if (health_it != stream_health.end()) {
                auto& health = health_it->second;
                
                if (health.is_connected.load()) {
                    reliability = std::min(1.0, reliability + 0.01); // Slowly increase
                } else {
                    reliability = std::max(0.1, reliability - 0.05); // Decrease faster
                }
            }
        }
    }
    
    StreamType get_stream_type_for_source(const std::string& source) {
        if (source == "twitter") return StreamType::SOCIAL_MEDIA;
        if (source == "gmgn") return StreamType::SMART_MONEY;
        if (source == "dexscreener") return StreamType::MARKET_DATA;
        if (source == "reddit") return StreamType::SOCIAL_MEDIA;
        if (source == "news") return StreamType::NEWS_FEED;
        return StreamType::MARKET_DATA; // Default
    }
    
    void generate_real_time_signals() {
        // Get recent data for all symbols
        std::lock_guard<std::mutex> lock(data_history_mutex);
        
        std::unordered_set<std::string> symbols;
        for (const auto& [symbol, history] : market_data_history) {
            symbols.insert(symbol);
        }
        
        for (const auto& symbol : symbols) {
            auto signal = generate_signal_for_symbol(symbol);
            if (signal.is_actionable) {
                signal_output_queue.push(signal);
                
                if (signal_callback) {
                    signal_callback(signal);
                }
                
                metrics.signals_generated.fetch_add(1);
            }
        }
    }
    
    AggregatedSignal generate_signal_for_symbol(const std::string& symbol) {
        AggregatedSignal signal;
        signal.symbol = symbol;
        signal.generated_at = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch());
        signal.expires_at = signal.generated_at + std::chrono::duration_cast<std::chrono::microseconds>(signal_ttl);
        
        // Get recent data for this symbol
        auto market_it = market_data_history.find(symbol);
        auto sentiment_it = sentiment_data_history.find(symbol);
        auto smart_money_it = smart_money_history.find(symbol);
        
        // Calculate individual scores
        signal.technical_score = calculate_technical_score(
            market_it != market_data_history.end() ? market_it->second : std::vector<LiveMarketData>{});
        signal.sentiment_score = calculate_sentiment_score(
            sentiment_it != sentiment_data_history.end() ? sentiment_it->second : std::vector<LiveSentimentData>{});
        signal.smart_money_score = calculate_smart_money_score(
            smart_money_it != smart_money_history.end() ? smart_money_it->second : std::vector<LiveSmartMoneyData>{});
        
        // Calculate momentum from technical data
        signal.momentum_score = signal.technical_score * 0.7 + signal.sentiment_score * 0.3;
        
        // Calculate overall score with source weights
        std::lock_guard<std::mutex> lock(source_mutex);
        signal.overall_score = (signal.technical_score * source_weights["dexscreener"] +
                               signal.sentiment_score * source_weights["twitter"] +
                               signal.smart_money_score * source_weights["gmgn"]) / 3.0;
        
        // Determine recommendation
        if (signal.overall_score > 0.8) signal.recommendation = "strong_buy";
        else if (signal.overall_score > 0.6) signal.recommendation = "buy";
        else if (signal.overall_score > 0.4) signal.recommendation = "hold";
        else if (signal.overall_score > 0.2) signal.recommendation = "sell";
        else signal.recommendation = "strong_sell";
        
        // Calculate confidence
        signal.confidence_level = std::min(1.0, signal.overall_score + 0.2);
        signal.is_actionable = signal.confidence_level > 0.6;
        
        // Add contributing sources
        if (market_it != market_data_history.end() && !market_it->second.empty()) {
            signal.contributing_sources.push_back("dexscreener");
        }
        if (sentiment_it != sentiment_data_history.end() && !sentiment_it->second.empty()) {
            signal.contributing_sources.push_back("twitter");
        }
        if (smart_money_it != smart_money_history.end() && !smart_money_it->second.empty()) {
            signal.contributing_sources.push_back("gmgn");
        }
        
        return signal;
    }
    
    double calculate_technical_score(const std::vector<LiveMarketData>& history) {
        if (history.empty()) return 0.5; // Neutral if no data
        
        double score = 0.5;
        const auto& latest = history.back();
        
        // Price momentum
        if (latest.price_change_1h > 0.05) score += 0.2;
        if (latest.price_change_24h > 0.1) score += 0.2;
        
        // Volume factor
        if (latest.volume_24h > 100000) score += 0.1;
        
        return std::max(0.0, std::min(1.0, score));
    }
    
    double calculate_sentiment_score(const std::vector<LiveSentimentData>& history) {
        if (history.empty()) return 0.5;
        
        double total_sentiment = 0.0;
        double total_confidence = 0.0;
        int viral_count = 0;
        
        // Look at recent sentiment (last 10 entries)
        int start_idx = std::max(0, static_cast<int>(history.size()) - 10);
        for (int i = start_idx; i < static_cast<int>(history.size()); ++i) {
            const auto& data = history[i];
            total_sentiment += data.sentiment_score * data.confidence;
            total_confidence += data.confidence;
            if (data.is_viral) viral_count++;
        }
        
        double avg_sentiment = total_confidence > 0 ? total_sentiment / total_confidence : 0.0;
        
        // Boost score if viral content detected
        if (viral_count > 0) {
            avg_sentiment *= 1.2;
        }
        
        // Convert from [-1, 1] to [0, 1]
        return (avg_sentiment + 1.0) / 2.0;
    }
    
    double calculate_smart_money_score(const std::vector<LiveSmartMoneyData>& history) {
        if (history.empty()) return 0.5;
        
        double buy_volume = 0.0;
        double sell_volume = 0.0;
        double insider_volume = 0.0;
        
        // Analyze recent smart money activity (last 5 transactions)
        int start_idx = std::max(0, static_cast<int>(history.size()) - 5);
        for (int i = start_idx; i < static_cast<int>(history.size()); ++i) {
            const auto& data = history[i];
            
            if (data.direction == "buy") {
                buy_volume += data.amount_usd;
            } else {
                sell_volume += data.amount_usd;
            }
            
            if (data.is_insider_wallet) {
                insider_volume += data.amount_usd;
            }
        }
        
        double total_volume = buy_volume + sell_volume;
        if (total_volume == 0) return 0.5;
        
        double buy_ratio = buy_volume / total_volume;
        double insider_ratio = insider_volume / total_volume;
        
        // Higher score for more buying and insider activity
        return buy_ratio * 0.7 + insider_ratio * 0.3;
    }
    
    void clean_expired_signals() {
        // Clean up expired signals from output queue
        // In a real implementation, would maintain a signal cache
    }
    
    void validate_signal_quality() {
        // Quality validation for generated signals
        metrics.consensus_validations.fetch_add(1);
    }
    
    void detect_anomalies() {
        // Anomaly detection in data streams
        // Would implement statistical analysis for outlier detection
    }
    
    void update_quality_metrics() {
        // Update quality-related metrics
    }
};

// High-speed message queue implementation
template<typename T>
HighSpeedMessageQueue<T>::HighSpeedMessageQueue() {
    auto dummy = new QueueNode;
    head_.store(dummy);
    tail_.store(dummy);
}

template<typename T>
HighSpeedMessageQueue<T>::~HighSpeedMessageQueue() {
    clear();
    delete head_.load();
}

template<typename T>
void HighSpeedMessageQueue<T>::push(const T& item) {
    auto new_node = new QueueNode;
    new_node->data = item;
    
    auto prev_tail = tail_.exchange(new_node);
    prev_tail->next.store(new_node);
    size_.fetch_add(1);
}

template<typename T>
bool HighSpeedMessageQueue<T>::try_pop(T& item) {
    auto head = head_.load();
    auto next = head->next.load();
    
    if (next == nullptr) {
        return false; // Queue is empty
    }
    
    item = next->data;
    head_.store(next);
    delete head;
    size_.fetch_sub(1);
    
    return true;
}

template<typename T>
size_t HighSpeedMessageQueue<T>::size() const {
    return size_.load();
}

template<typename T>
bool HighSpeedMessageQueue<T>::empty() const {
    return size() == 0;
}

template<typename T>
void HighSpeedMessageQueue<T>::clear() {
    T item;
    while (try_pop(item)) {
        // Empty the queue
    }
}

// Main class implementations
RealTimeDataAggregator::RealTimeDataAggregator() 
    : pimpl_(std::make_unique<Impl>()) {}

RealTimeDataAggregator::~RealTimeDataAggregator() {
    if (pimpl_) {
        stop_all_streams();
    }
}

bool RealTimeDataAggregator::initialize() {
    HFX_LOG_INFO("Initializing Real-Time Data Aggregator (Ultra-Fast Processing)");
    return true;
}

void RealTimeDataAggregator::start_all_streams() {
    pimpl_->start_all_processing();
}

void RealTimeDataAggregator::stop_all_streams() {
    pimpl_->stop_all_processing();
}

bool RealTimeDataAggregator::start_stream(StreamType type, const std::string& config) {
    std::lock_guard<std::mutex> lock(pimpl_->health_mutex);
    auto& health = pimpl_->stream_health[type];
    health.is_connected.store(true);
    health.last_message_time.store(
        std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count());
    
    HFX_LOG_INFO("[LOG] Message");
    return true;
}

void RealTimeDataAggregator::stop_stream(StreamType type) {
    std::lock_guard<std::mutex> lock(pimpl_->health_mutex);
    auto& health = pimpl_->stream_health[type];
    health.is_connected.store(false);
    
    HFX_LOG_INFO("[LOG] Message");
}

bool RealTimeDataAggregator::is_stream_healthy(StreamType type) const {
    std::lock_guard<std::mutex> lock(pimpl_->health_mutex);
    auto it = pimpl_->stream_health.find(type);
    return it != pimpl_->stream_health.end() && it->second.is_connected.load();
}

void RealTimeDataAggregator::restart_stream(StreamType type) {
    pimpl_->restart_stream_impl(type);
}

void RealTimeDataAggregator::start_twitter_stream(const std::vector<std::string>& keywords,
                                                 const std::vector<std::string>& usernames) {
    start_stream(StreamType::SOCIAL_MEDIA, "twitter_config");
    HFX_LOG_INFO("[LOG] Message");
              << usernames.size() << " usernames" << std::endl;
}

void RealTimeDataAggregator::start_smart_money_stream(const std::vector<std::string>& watched_wallets,
                                                     double min_transaction_size) {
    start_stream(StreamType::SMART_MONEY, "smart_money_config");
    
    {
        std::lock_guard<std::mutex> lock(pimpl_->wallet_mutex);
        for (const auto& wallet : watched_wallets) {
            pimpl_->watched_smart_wallets.insert(wallet);
        }
    }
    
    HFX_LOG_INFO("[LOG] Message");
              << " wallets (min size: $" << min_transaction_size << ")" << std::endl;
}

void RealTimeDataAggregator::start_dexscreener_stream(const std::string& chain, double min_liquidity) {
    start_stream(StreamType::MARKET_DATA, "dexscreener_config");
    HFX_LOG_INFO("[LOG] Message");
              << " (min liquidity: $" << min_liquidity << ")" << std::endl;
}

void RealTimeDataAggregator::start_reddit_stream(const std::vector<std::string>& subreddits,
                                                const std::vector<std::string>& keywords) {
    start_stream(StreamType::SOCIAL_MEDIA, "reddit_config");
    HFX_LOG_INFO("[LOG] Message");
}

void RealTimeDataAggregator::start_news_stream(const std::vector<std::string>& sources,
                                              const std::vector<std::string>& keywords) {
    start_stream(StreamType::NEWS_FEED, "news_config");
    HFX_LOG_INFO("[LOG] Message");
}

void RealTimeDataAggregator::process_live_market_data(const LiveMarketData& data) {
    pimpl_->market_data_queue.push(data);
}

void RealTimeDataAggregator::process_live_sentiment_data(const LiveSentimentData& data) {
    pimpl_->sentiment_data_queue.push(data);
}

void RealTimeDataAggregator::process_live_smart_money_data(const LiveSmartMoneyData& data) {
    pimpl_->smart_money_queue.push(data);
}

AggregatedSignal RealTimeDataAggregator::generate_real_time_signal(const std::string& symbol) {
    return pimpl_->generate_signal_for_symbol(symbol);
}

std::vector<AggregatedSignal> RealTimeDataAggregator::get_top_signals_live(int limit) {
    std::vector<AggregatedSignal> signals;
    AggregatedSignal signal;
    
    while (signals.size() < static_cast<size_t>(limit) && 
           pimpl_->signal_output_queue.try_pop(signal)) {
        signals.push_back(signal);
    }
    
    // Sort by overall score
    std::sort(signals.begin(), signals.end(),
              [](const AggregatedSignal& a, const AggregatedSignal& b) {
                  return a.overall_score > b.overall_score;
              });
    
    return signals;
}

void RealTimeDataAggregator::register_market_data_callback(std::function<void(const LiveMarketData&)> callback) {
    pimpl_->market_data_callback = callback;
}

void RealTimeDataAggregator::register_sentiment_callback(std::function<void(const LiveSentimentData&)> callback) {
    pimpl_->sentiment_callback = callback;
}

void RealTimeDataAggregator::register_smart_money_callback(std::function<void(const LiveSmartMoneyData&)> callback) {
    pimpl_->smart_money_callback = callback;
}

void RealTimeDataAggregator::register_signal_callback(std::function<void(const AggregatedSignal&)> callback) {
    pimpl_->signal_callback = callback;
}

void RealTimeDataAggregator::enable_low_latency_mode(bool enabled) {
    pimpl_->low_latency_mode.store(enabled);
    HFX_LOG_INFO("[LOG] Message");
}

void RealTimeDataAggregator::set_processing_priority(int priority) {
    pimpl_->processing_priority = std::max(1, std::min(10, priority));
}

void RealTimeDataAggregator::get_stream_health(StreamType type, StreamHealth& health_out) const {
    std::lock_guard<std::mutex> lock(pimpl_->health_mutex);
    auto it = pimpl_->stream_health.find(type);
    if (it != pimpl_->stream_health.end()) {
        health_out.is_connected.store(it->second.is_connected.load());
        health_out.messages_received.store(it->second.messages_received.load());
        health_out.messages_processed.store(it->second.messages_processed.load());
        health_out.messages_dropped.store(it->second.messages_dropped.load());
        health_out.latency_ms.store(it->second.latency_ms.load());
        health_out.last_message_time.store(it->second.last_message_time.load());
        health_out.reconnection_count.store(it->second.reconnection_count.load());
        health_out.last_error = it->second.last_error;
    }
}

void RealTimeDataAggregator::get_metrics(AggregatorMetrics& metrics_out) const {
    std::lock_guard<std::mutex> lock(pimpl_->metrics_mutex);
    metrics_out.total_messages_processed.store(pimpl_->metrics.total_messages_processed.load());
    metrics_out.signals_generated.store(pimpl_->metrics.signals_generated.load());
    metrics_out.avg_processing_latency_us.store(pimpl_->metrics.avg_processing_latency_us.load());
    metrics_out.signal_generation_rate.store(pimpl_->metrics.signal_generation_rate.load());
    metrics_out.consensus_validations.store(pimpl_->metrics.consensus_validations.load());
    metrics_out.quality_rejections.store(pimpl_->metrics.quality_rejections.load());
}

std::string RealTimeDataAggregator::get_performance_summary() const {
    AggregatorMetrics metrics;
    get_metrics(metrics);
    
    std::ostringstream summary;
    summary << "Real-Time Data Aggregator Performance:\n";
    summary << "  Total Messages Processed: " << metrics.total_messages_processed.load() << "\n";
    summary << "  Signals Generated: " << metrics.signals_generated.load() << "\n";
    summary << "  Avg Processing Latency: " << metrics.avg_processing_latency_us.load() << "μs\n";
    summary << "  Signal Generation Rate: " << metrics.signal_generation_rate.load() << "/s\n";
    
    return summary.str();
}

void RealTimeDataAggregator::apply_fastest_bot_settings() {
    enable_low_latency_mode(true);
    set_processing_priority(10);
    pimpl_->signal_ttl = std::chrono::seconds(15); // Shorter TTL for speed
    pimpl_->data_validation_level = 2; // Lower validation for speed
    pimpl_->max_messages_per_second = 50000; // Higher throughput
    
    HFX_LOG_INFO("Applied fastest bot settings (Maximum Speed Mode)");
}

void RealTimeDataAggregator::emergency_stop_all_streams() {
    stop_all_streams();
    pimpl_->market_data_queue.clear();
    pimpl_->sentiment_data_queue.clear();
    pimpl_->smart_money_queue.clear();
    pimpl_->signal_output_queue.clear();
    
    HFX_LOG_INFO("🚨 EMERGENCY STOP: All streams stopped and queues cleared");
}

// Explicit template instantiations
template class HighSpeedMessageQueue<LiveMarketData>;
template class HighSpeedMessageQueue<LiveSentimentData>;
template class HighSpeedMessageQueue<LiveSmartMoneyData>;
template class HighSpeedMessageQueue<AggregatedSignal>;

} // namespace hfx::ai
