#include "sentiment_execution_pipeline.hpp"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <algorithm>
#include <random>
#include <regex>
#include <iostream>
#include <sstream>
#include <unordered_set>

namespace hfx::ai {

struct SentimentExecutionPipeline::Impl {
    // Core state
    std::atomic<bool> is_running{false};
    std::atomic<bool> is_paused{false};
    std::atomic<bool> emergency_stopped{false};
    
    // Processing threads for ultra-fast execution
    std::vector<std::thread> execution_threads;
    std::thread sentiment_monitor_thread;
    std::thread priority_fee_monitor_thread;
    
    // Signal queues with priority handling - inspired by fastest bots
    PrioritySignalQueue<SentimentSignal> signal_queue;
    mutable std::mutex execution_mutex;
    mutable std::mutex metrics_mutex;
    
    // Metrics and monitoring
    PipelineMetrics metrics;
    std::vector<ExecutionResult> recent_executions;
    
    // Configuration - tuned for microsecond execution like GMGN
    ExecutionUrgency default_urgency = ExecutionUrgency::HIGH_FREQUENCY;
    double max_position_usd = 100000.0;
    double max_daily_loss = 10000.0;
    double max_slippage_bps = 50.0; // 0.5%
    bool aggressive_mode = true; // For memecoin sniping
    
    // Pre-signed transaction pool - critical for speed like BonkBot
    std::unordered_map<std::string, std::vector<std::string>> pre_signed_transactions;
    mutable std::mutex pre_signed_mutex;
    
    // Real-time data caches - for instant access
    std::unordered_map<std::string, double> token_prices;
    std::unordered_map<std::string, double> gas_prices;
    std::unordered_map<std::string, bool> token_safety_cache;
    mutable std::mutex cache_mutex;
    
    // Network monitoring - for priority fee optimization
    std::atomic<double> network_congestion_level{0.5};
    std::atomic<double> current_priority_fee{1.0};
    std::atomic<uint64_t> pending_transaction_count{0};
    
    // Callbacks for real-time processing
    std::unordered_map<SignalType, std::function<void(const SentimentSignal&)>> signal_callbacks;
    std::function<void(const ExecutionResult&)> execution_callback;
    std::function<void(const std::string&, const std::string&)> error_callback;
    
    // Performance optimization data
    std::unordered_set<std::string> warmed_up_tokens;
    std::unordered_map<std::string, std::vector<std::string>> cached_routing_paths;
    
    Impl() {
        initialize_execution_threads();
        initialize_performance_caches();
    }
    
    ~Impl() {
        stop_all_processing();
    }
    
    void initialize_execution_threads() {
        // Create multiple execution threads for parallel processing
        int num_threads = std::thread::hardware_concurrency();
        execution_threads.reserve(num_threads);
        
        for (int i = 0; i < num_threads; ++i) {
            execution_threads.emplace_back(&Impl::execution_thread_loop, this, i);
        }
    }
    
    void initialize_performance_caches() {
        // Pre-warm common token data
        std::vector<std::string> common_tokens{
            "So11111111111111111111111111111111111111112", // SOL
            "EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v", // USDC
            "Es9vMFrzaCERmJfrF4H2FYD4KCoNkY11McCe8BenwNYB"  // USDT
        };
        
        for (const auto& token : common_tokens) {
            pre_warm_token_data(token);
        }
    }
    
    void pre_warm_token_data(const std::string& token_address) {
        std::lock_guard<std::mutex> lock(cache_mutex);
        
        // Cache token safety
        token_safety_cache[token_address] = verify_token_safety_impl(token_address);
        
        // Cache routing paths
        cached_routing_paths[token_address] = find_optimal_routes_impl(token_address);
        
        // Mark as warmed up
        warmed_up_tokens.insert(token_address);
    }
    
    void start_processing() {
        if (!is_running.load()) {
            is_running.store(true);
            emergency_stopped.store(false);
            
            // Start monitoring threads
            sentiment_monitor_thread = std::thread(&Impl::sentiment_monitoring_loop, this);
            priority_fee_monitor_thread = std::thread(&Impl::priority_fee_monitoring_loop, this);
            
            std::cout << "Sentiment-to-Execution Pipeline started (Microsecond Mode)" << std::endl;
        }
    }
    
    void stop_all_processing() {
        if (is_running.load()) {
            is_running.store(false);
            signal_queue.stop();
            
            // Wait for threads to finish
            if (sentiment_monitor_thread.joinable()) {
                sentiment_monitor_thread.join();
            }
            if (priority_fee_monitor_thread.joinable()) {
                priority_fee_monitor_thread.join();
            }
            
            for (auto& thread : execution_threads) {
                if (thread.joinable()) {
                    thread.join();
                }
            }
            
            std::cout << "Pipeline stopped" << std::endl;
        }
    }
    
    void execution_thread_loop(int thread_id) {
        std::cout << "Execution thread " << thread_id << " started" << std::endl;
        
        while (is_running.load()) {
            try {
                if (emergency_stopped.load() || is_paused.load()) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    continue;
                }
                
                SentimentSignal signal = signal_queue.pop();
                
                if (!is_running.load()) break;
                
                auto start_time = std::chrono::high_resolution_clock::now();
                
                // Process signal with microsecond precision
                ExecutionResult result = execute_signal_fast(signal);
                
                auto end_time = std::chrono::high_resolution_clock::now();
                auto execution_latency = std::chrono::duration_cast<std::chrono::microseconds>(
                    end_time - start_time);
                
                result.execution_latency = execution_latency;
                result.total_pipeline_latency = std::chrono::duration_cast<std::chrono::microseconds>(
                    end_time - std::chrono::high_resolution_clock::time_point(signal.timestamp));
                
                // Update metrics
                update_execution_metrics(result);
                
                // Store result
                {
                    std::lock_guard<std::mutex> lock(execution_mutex);
                    recent_executions.push_back(result);
                    if (recent_executions.size() > 1000) {
                        recent_executions.erase(recent_executions.begin(), 
                                              recent_executions.begin() + 100);
                    }
                }
                
                // Call callback if registered
                if (execution_callback) {
                    execution_callback(result);
                }
                
                if (result.success) {
                    std::cout << "✅ Executed " << signal.token_symbol 
                             << " in " << execution_latency.count() << "μs" << std::endl;
                } else {
                    std::cout << "❌ Failed " << signal.token_symbol 
                             << ": " << result.error_message << std::endl;
                }
                
            } catch (const std::exception& e) {
                std::cerr << "Execution thread error: " << e.what() << std::endl;
                if (error_callback) {
                    error_callback("execution_thread", e.what());
                }
            }
        }
        
        std::cout << "Execution thread " << thread_id << " stopped" << std::endl;
    }
    
    ExecutionResult execute_signal_fast(const SentimentSignal& signal) {
        ExecutionResult result;
        result.signal_id = signal.signal_id;
        result.success = false;
        
        // Pre-execution validation - must be ultra-fast
        if (!validate_signal_pre_execution(signal)) {
            result.error_message = "Signal validation failed";
            return result;
        }
        
        // Check if we have a pre-signed transaction for instant execution
        if (has_pre_signed_transaction_impl(signal.token_address, signal.direction)) {
            return execute_pre_signed_impl(signal);
        }
        
        // Execute based on urgency level
        switch (signal.urgency) {
            case ExecutionUrgency::MICROSECOND:
                return execute_microsecond_impl(signal);
            case ExecutionUrgency::ULTRA_FAST:
                return execute_ultra_fast_impl(signal);
            case ExecutionUrgency::HIGH_FREQUENCY:
                return execute_high_frequency_impl(signal);
            default:
                return execute_normal_impl(signal);
        }
    }
    
    bool validate_signal_pre_execution(const SentimentSignal& signal) {
        // Ultra-fast validation checks - must complete in microseconds
        
        // Check if signal is stale
        auto now = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch());
        if (now - signal.timestamp > signal.ttl) {
            return false;
        }
        
        // Check emergency stop
        if (emergency_stopped.load()) {
            return false;
        }
        
        // Check position size limits
        if (signal.position_size > max_position_usd) {
            return false;
        }
        
        // Check token safety (use cache for speed)
        {
            std::lock_guard<std::mutex> lock(cache_mutex);
            auto safety_it = token_safety_cache.find(signal.token_address);
            if (safety_it != token_safety_cache.end() && !safety_it->second) {
                return false;
            }
        }
        
        return true;
    }
    
    ExecutionResult execute_pre_signed_impl(const SentimentSignal& signal) {
        ExecutionResult result;
        result.signal_id = signal.signal_id;
        
        std::lock_guard<std::mutex> lock(pre_signed_mutex);
        auto it = pre_signed_transactions.find(signal.token_address);
        
        if (it != pre_signed_transactions.end() && !it->second.empty()) {
            // Get pre-signed transaction
            std::string signed_tx = it->second.back();
            it->second.pop_back();
            
            // Simulate instant execution (in production, would send to Solana)
            result.success = true;
            result.transaction_hash = "pre_signed_" + generate_random_hash();
            result.executed_price = get_current_price(signal.token_address);
            result.executed_quantity = signal.position_size / result.executed_price;
            result.actual_slippage_bps = 2.0; // Minimal slippage for pre-signed
            result.gas_cost = 0.001; // SOL
            result.execution_venue = "Raydium_PreSigned";
            
            return result;
        }
        
        result.error_message = "No pre-signed transaction available";
        return result;
    }
    
    ExecutionResult execute_microsecond_impl(const SentimentSignal& signal) {
        ExecutionResult result;
        result.signal_id = signal.signal_id;
        
        // Ultra-fast execution path - no complex routing
        result.success = true;
        result.transaction_hash = "microsec_" + generate_random_hash();
        result.executed_price = get_current_price(signal.token_address);
        result.executed_quantity = signal.position_size / result.executed_price;
        result.actual_slippage_bps = 5.0; // Higher slippage for speed
        result.gas_cost = 0.005; // Higher priority fee
        result.execution_venue = "Raydium_UltraFast";
        
        return result;
    }
    
    ExecutionResult execute_ultra_fast_impl(const SentimentSignal& signal) {
        ExecutionResult result;
        result.signal_id = signal.signal_id;
        
        // Fast execution with minimal optimization
        result.success = simulate_execution_success(signal);
        
        if (result.success) {
            result.transaction_hash = "ultra_" + generate_random_hash();
            result.executed_price = get_current_price(signal.token_address) * 
                                   (1.0 + (signal.direction == "buy" ? 0.002 : -0.002));
            result.executed_quantity = signal.position_size / result.executed_price;
            result.actual_slippage_bps = 8.0;
            result.gas_cost = 0.003;
            result.execution_venue = "Jupiter_Fast";
        } else {
            result.error_message = "Network congestion - execution failed";
        }
        
        return result;
    }
    
    ExecutionResult execute_high_frequency_impl(const SentimentSignal& signal) {
        ExecutionResult result;
        result.signal_id = signal.signal_id;
        
        // Balanced execution with some optimization
        std::vector<std::string> routing_path = find_optimal_routes_impl(signal.token_address);
        
        result.success = simulate_execution_success(signal, 0.85); // Higher success rate
        
        if (result.success) {
            result.transaction_hash = "hf_" + generate_random_hash();
            result.executed_price = get_optimal_execution_price(signal);
            result.executed_quantity = signal.position_size / result.executed_price;
            result.actual_slippage_bps = 12.0;
            result.gas_cost = 0.002;
            result.execution_venue = routing_path.empty() ? "Orca" : routing_path[0];
        } else {
            result.error_message = "Optimal routing failed";
        }
        
        return result;
    }
    
    ExecutionResult execute_normal_impl(const SentimentSignal& signal) {
        ExecutionResult result;
        result.signal_id = signal.signal_id;
        
        // Full optimization execution
        result.success = simulate_execution_success(signal, 0.95); // Highest success rate
        
        if (result.success) {
            result.transaction_hash = "normal_" + generate_random_hash();
            result.executed_price = get_optimal_execution_price(signal) * 0.999; // Better price
            result.executed_quantity = signal.position_size / result.executed_price;
            result.actual_slippage_bps = 15.0;
            result.gas_cost = 0.001;
            result.execution_venue = "Jupiter_Optimized";
        } else {
            result.error_message = "Market impact too high";
        }
        
        return result;
    }
    
    bool simulate_execution_success(const SentimentSignal& signal, double base_success_rate = 0.8) {
        // Simulate execution success based on market conditions
        static std::mt19937 rng(std::chrono::steady_clock::now().time_since_epoch().count());
        std::uniform_real_distribution<> success_dist(0.0, 1.0);
        
        double success_rate = base_success_rate;
        
        // Adjust based on signal strength
        success_rate += static_cast<double>(signal.strength) * 0.05;
        
        // Adjust based on network congestion
        success_rate -= network_congestion_level.load() * 0.2;
        
        // Adjust based on urgency (faster = less reliable)
        switch (signal.urgency) {
            case ExecutionUrgency::MICROSECOND:
                success_rate -= 0.15;
                break;
            case ExecutionUrgency::ULTRA_FAST:
                success_rate -= 0.1;
                break;
            case ExecutionUrgency::HIGH_FREQUENCY:
                success_rate -= 0.05;
                break;
            default:
                break;
        }
        
        return success_dist(rng) < std::max(0.1, std::min(1.0, success_rate));
    }
    
    double get_current_price(const std::string& token_address) {
        std::lock_guard<std::mutex> lock(cache_mutex);
        auto it = token_prices.find(token_address);
        if (it != token_prices.end()) {
            return it->second;
        }
        
        // Generate synthetic price
        static std::mt19937 rng(std::chrono::steady_clock::now().time_since_epoch().count());
        std::uniform_real_distribution<> price_dist(0.001, 10.0);
        double price = price_dist(rng);
        
        token_prices[token_address] = price;
        return price;
    }
    
    double get_optimal_execution_price(const SentimentSignal& signal) {
        double base_price = get_current_price(signal.token_address);
        
        // Apply price impact estimation
        double impact = signal.expected_price_impact;
        if (signal.direction == "buy") {
            return base_price * (1.0 + impact);
        } else {
            return base_price * (1.0 - impact);
        }
    }
    
    std::vector<std::string> find_optimal_routes_impl(const std::string& token_address) {
        std::lock_guard<std::mutex> lock(cache_mutex);
        auto it = cached_routing_paths.find(token_address);
        if (it != cached_routing_paths.end()) {
            return it->second;
        }
        
        // Generate synthetic routing paths
        std::vector<std::string> routes{"Raydium", "Orca", "Serum"};
        cached_routing_paths[token_address] = routes;
        return routes;
    }
    
    bool verify_token_safety_impl(const std::string& token_address) {
        // Simulate token safety check (in production, would check against rug databases)
        static std::mt19937 rng(std::chrono::steady_clock::now().time_since_epoch().count());
        std::uniform_real_distribution<> safety_dist(0.0, 1.0);
        
        return safety_dist(rng) > 0.1; // 90% of tokens are "safe"
    }
    
    bool has_pre_signed_transaction_impl(const std::string& token_address, 
                                        const std::string& direction) {
        std::lock_guard<std::mutex> lock(pre_signed_mutex);
        auto it = pre_signed_transactions.find(token_address);
        return it != pre_signed_transactions.end() && !it->second.empty();
    }
    
    std::string generate_random_hash() {
        static std::mt19937 rng(std::chrono::steady_clock::now().time_since_epoch().count());
        std::uniform_int_distribution<> hex_dist(0, 15);
        
        std::string hash;
        hash.reserve(16);
        
        for (int i = 0; i < 16; ++i) {
            int val = hex_dist(rng);
            hash += (val < 10) ? ('0' + val) : ('a' + val - 10);
        }
        
        return hash;
    }
    
    void update_execution_metrics(const ExecutionResult& result) {
        std::lock_guard<std::mutex> lock(metrics_mutex);
        
        metrics.signals_processed.fetch_add(1);
        
        if (result.success) {
            metrics.signals_executed.fetch_add(1);
        } else {
            metrics.signals_rejected.fetch_add(1);
        }
        
        // Update average latencies
        auto total_processed = metrics.signals_processed.load();
        if (total_processed > 0) {
            auto current_exec_avg = metrics.avg_execution_latency_us.load();
            auto new_exec_avg = ((current_exec_avg * (total_processed - 1)) + 
                                result.execution_latency.count()) / total_processed;
            metrics.avg_execution_latency_us.store(new_exec_avg);
            
            auto current_pipeline_avg = metrics.avg_pipeline_latency_us.load();
            auto new_pipeline_avg = ((current_pipeline_avg * (total_processed - 1)) + 
                                   result.total_pipeline_latency.count()) / total_processed;
            metrics.avg_pipeline_latency_us.store(new_pipeline_avg);
            
            // Update success rate
            double success_rate = static_cast<double>(metrics.signals_executed.load()) / total_processed;
            metrics.success_rate.store(success_rate);
        }
    }
    
    void sentiment_monitoring_loop() {
        std::cout << "Sentiment monitoring started" << std::endl;
        
        while (is_running.load()) {
            try {
                // Monitor various sentiment sources
                monitor_twitter_sentiment();
                monitor_smart_money_flows();
                monitor_technical_signals();
                monitor_news_catalysts();
                
            } catch (const std::exception& e) {
                std::cerr << "Sentiment monitoring error: " << e.what() << std::endl;
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(100)); // 100ms cycles
        }
        
        std::cout << "Sentiment monitoring stopped" << std::endl;
    }
    
    void monitor_twitter_sentiment() {
        // Simulate real-time Twitter sentiment monitoring
        static std::mt19937 rng(std::chrono::steady_clock::now().time_since_epoch().count());
        std::uniform_real_distribution<> signal_chance(0.0, 1.0);
        
        if (signal_chance(rng) < 0.05) { // 5% chance per cycle
            SentimentSignal signal = generate_twitter_sentiment_signal();
            if (signal.confidence_level > 0.7) {
                process_signal_immediate(signal);
            }
        }
    }
    
    void monitor_smart_money_flows() {
        // Simulate smart money flow detection
        static std::mt19937 rng(std::chrono::steady_clock::now().time_since_epoch().count());
        std::uniform_real_distribution<> signal_chance(0.0, 1.0);
        
        if (signal_chance(rng) < 0.03) { // 3% chance per cycle
            SentimentSignal signal = generate_smart_money_signal();
            if (signal.confidence_level > 0.8) {
                process_signal_immediate(signal);
            }
        }
    }
    
    void monitor_technical_signals() {
        // Simulate technical signal detection
        static std::mt19937 rng(std::chrono::steady_clock::now().time_since_epoch().count());
        std::uniform_real_distribution<> signal_chance(0.0, 1.0);
        
        if (signal_chance(rng) < 0.04) { // 4% chance per cycle
            SentimentSignal signal = generate_technical_signal();
            if (signal.confidence_level > 0.6) {
                process_signal_immediate(signal);
            }
        }
    }
    
    void monitor_news_catalysts() {
        // Simulate news catalyst detection
        static std::mt19937 rng(std::chrono::steady_clock::now().time_since_epoch().count());
        std::uniform_real_distribution<> signal_chance(0.0, 1.0);
        
        if (signal_chance(rng) < 0.02) { // 2% chance per cycle
            SentimentSignal signal = generate_news_catalyst_signal();
            if (signal.confidence_level > 0.75) {
                process_signal_immediate(signal);
            }
        }
    }
    
    SentimentSignal generate_twitter_sentiment_signal() {
        static std::mt19937 rng(std::chrono::steady_clock::now().time_since_epoch().count());
        
        SentimentSignal signal;
        signal.signal_id = "twitter_" + generate_random_hash();
        signal.token_address = "sample_token_" + std::to_string(rng() % 100);
        signal.token_symbol = "TOK" + std::to_string(rng() % 100);
        signal.type = SignalType::TWITTER_SENTIMENT;
        signal.strength = static_cast<SignalStrength>(1 + (rng() % 4));
        signal.urgency = ExecutionUrgency::HIGH_FREQUENCY;
        
        std::uniform_real_distribution<> sentiment_dist(-1.0, 1.0);
        std::uniform_real_distribution<> confidence_dist(0.3, 0.95);
        std::uniform_real_distribution<> impact_dist(0.01, 0.1);
        std::uniform_real_distribution<> size_dist(1000.0, 50000.0);
        
        signal.sentiment_score = sentiment_dist(rng);
        signal.confidence_level = confidence_dist(rng);
        signal.expected_price_impact = impact_dist(rng);
        signal.position_size = size_dist(rng);
        signal.direction = signal.sentiment_score > 0 ? "buy" : "sell";
        
        signal.timestamp = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch());
        signal.ttl = std::chrono::microseconds(5000000); // 5 seconds TTL
        
        signal.data_sources = {"twitter_stream", "sentiment_ai"};
        signal.max_slippage_bps = 30.0;
        signal.priority_fee_multiplier = 2.0;
        signal.use_mev_protection = true;
        
        return signal;
    }
    
    SentimentSignal generate_smart_money_signal() {
        // Similar to twitter signal but with different characteristics
        auto signal = generate_twitter_sentiment_signal();
        signal.signal_id = "smart_money_" + generate_random_hash();
        signal.type = SignalType::SMART_MONEY_FLOW;
        signal.urgency = ExecutionUrgency::ULTRA_FAST; // Smart money signals are urgent
        signal.confidence_level = std::min(1.0, signal.confidence_level + 0.2); // Higher confidence
        signal.data_sources = {"gmgn_api", "whale_tracker"};
        signal.ttl = std::chrono::microseconds(2000000); // 2 seconds TTL
        
        return signal;
    }
    
    SentimentSignal generate_technical_signal() {
        auto signal = generate_twitter_sentiment_signal();
        signal.signal_id = "technical_" + generate_random_hash();
        signal.type = SignalType::TECHNICAL_BREAKOUT;
        signal.urgency = ExecutionUrgency::HIGH_FREQUENCY;
        signal.data_sources = {"price_feed", "volume_analyzer"};
        signal.ttl = std::chrono::microseconds(10000000); // 10 seconds TTL
        
        return signal;
    }
    
    SentimentSignal generate_news_catalyst_signal() {
        auto signal = generate_twitter_sentiment_signal();
        signal.signal_id = "news_" + generate_random_hash();
        signal.type = SignalType::NEWS_CATALYST;
        signal.urgency = ExecutionUrgency::MICROSECOND; // News can be very urgent
        signal.confidence_level = std::min(1.0, signal.confidence_level + 0.1);
        signal.data_sources = {"news_feed", "crypto_news_ai"};
        signal.ttl = std::chrono::microseconds(1000000); // 1 second TTL
        
        return signal;
    }
    
    void process_signal_immediate(const SentimentSignal& signal) {
        // Add signal to priority queue for execution
        signal_queue.push(signal);
        
        // Call signal callback if registered
        auto callback_it = signal_callbacks.find(signal.type);
        if (callback_it != signal_callbacks.end()) {
            callback_it->second(signal);
        }
        
        std::cout << "🔔 Signal: " << signal.token_symbol 
                 << " " << signal.direction 
                 << " (Confidence: " << signal.confidence_level << ")" << std::endl;
    }
    
    void priority_fee_monitoring_loop() {
        std::cout << "Priority fee monitoring started" << std::endl;
        
        while (is_running.load()) {
            try {
                // Monitor network conditions and adjust priority fees
                update_network_congestion();
                update_priority_fee_recommendations();
                
            } catch (const std::exception& e) {
                std::cerr << "Priority fee monitoring error: " << e.what() << std::endl;
            }
            
            std::this_thread::sleep_for(std::chrono::seconds(1)); // 1-second updates
        }
        
        std::cout << "Priority fee monitoring stopped" << std::endl;
    }
    
    void update_network_congestion() {
        // Simulate network congestion monitoring
        static std::mt19937 rng(std::chrono::steady_clock::now().time_since_epoch().count());
        std::uniform_real_distribution<> congestion_dist(0.1, 0.9);
        
        network_congestion_level.store(congestion_dist(rng));
        
        // Simulate pending transaction count
        std::uniform_int_distribution<> pending_dist(100, 10000);
        pending_transaction_count.store(pending_dist(rng));
    }
    
    void update_priority_fee_recommendations() {
        // Calculate optimal priority fee based on network conditions
        double base_fee = 0.000001; // Base Solana fee
        double congestion = network_congestion_level.load();
        
        // Higher congestion = higher priority fee needed
        double recommended_fee = base_fee * (1.0 + congestion * 10.0);
        current_priority_fee.store(recommended_fee);
    }
};

// Priority Queue Implementation
template<typename T>
void PrioritySignalQueue<T>::push(const T& signal) {
    std::lock_guard<std::mutex> lock(mutex_);
    queue_.push(signal);
    cv_.notify_one();
}

template<typename T>
T PrioritySignalQueue<T>::pop() {
    std::unique_lock<std::mutex> lock(mutex_);
    cv_.wait(lock, [this] { return !queue_.empty() || stop_flag_.load(); });
    
    if (stop_flag_.load() && queue_.empty()) {
        throw std::runtime_error("Queue stopped");
    }
    
    T signal = queue_.top();
    queue_.pop();
    return signal;
}

template<typename T>
bool PrioritySignalQueue<T>::try_pop(T& signal) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (queue_.empty()) {
        return false;
    }
    
    signal = queue_.top();
    queue_.pop();
    return true;
}

template<typename T>
size_t PrioritySignalQueue<T>::size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.size();
}

template<typename T>
void PrioritySignalQueue<T>::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    while (!queue_.empty()) {
        queue_.pop();
    }
}

template<typename T>
void PrioritySignalQueue<T>::stop() {
    stop_flag_.store(true);
    cv_.notify_all();
}

// Main class implementations
SentimentExecutionPipeline::SentimentExecutionPipeline() 
    : pimpl_(std::make_unique<Impl>()) {}

SentimentExecutionPipeline::~SentimentExecutionPipeline() {
    if (pimpl_) {
        stop_pipeline();
    }
}

bool SentimentExecutionPipeline::initialize() {
    std::cout << "Initializing Sentiment-to-Execution Pipeline (Microsecond Precision)" << std::endl;
    return true;
}

void SentimentExecutionPipeline::start_pipeline() {
    pimpl_->start_processing();
}

void SentimentExecutionPipeline::stop_pipeline() {
    pimpl_->stop_all_processing();
}

void SentimentExecutionPipeline::process_sentiment_signal(const SentimentSignal& signal) {
    pimpl_->process_signal_immediate(signal);
}

bool SentimentExecutionPipeline::validate_signal_quality(const SentimentSignal& signal) {
    return pimpl_->validate_signal_pre_execution(signal);
}

double SentimentExecutionPipeline::calculate_execution_priority(const SentimentSignal& signal) {
    double priority = static_cast<double>(signal.strength) * signal.confidence_level;
    
    // Boost priority for urgent signals
    switch (signal.urgency) {
        case ExecutionUrgency::MICROSECOND:
            priority *= 4.0;
            break;
        case ExecutionUrgency::ULTRA_FAST:
            priority *= 3.0;
            break;
        case ExecutionUrgency::HIGH_FREQUENCY:
            priority *= 2.0;
            break;
        default:
            break;
    }
    
    return priority;
}

ExecutionResult SentimentExecutionPipeline::execute_signal_immediate(const SentimentSignal& signal) {
    return pimpl_->execute_signal_fast(signal);
}

void SentimentExecutionPipeline::pre_sign_transactions(const std::string& token_address, int quantity) {
    std::lock_guard<std::mutex> lock(pimpl_->pre_signed_mutex);
    
    auto& transactions = pimpl_->pre_signed_transactions[token_address];
    for (int i = 0; i < quantity; ++i) {
        transactions.push_back("signed_tx_" + token_address + "_" + std::to_string(i));
    }
    
    std::cout << "Pre-signed " << quantity << " transactions for " << token_address << std::endl;
}

bool SentimentExecutionPipeline::has_pre_signed_transaction(const std::string& token_address, 
                                                           const std::string& direction) {
    return pimpl_->has_pre_signed_transaction_impl(token_address, direction);
}

void SentimentExecutionPipeline::monitor_real_time_sentiment() {
    // This is handled by the background monitoring threads
    std::cout << "Real-time sentiment monitoring is active" << std::endl;
}

double SentimentExecutionPipeline::calculate_optimal_priority_fee(const SentimentSignal& signal) {
    double base_fee = pimpl_->current_priority_fee.load();
    double multiplier = signal.priority_fee_multiplier;
    
    // Adjust based on urgency
    switch (signal.urgency) {
        case ExecutionUrgency::MICROSECOND:
            multiplier *= 5.0;
            break;
        case ExecutionUrgency::ULTRA_FAST:
            multiplier *= 3.0;
            break;
        case ExecutionUrgency::HIGH_FREQUENCY:
            multiplier *= 2.0;
            break;
        default:
            break;
    }
    
    return base_fee * multiplier;
}

void SentimentExecutionPipeline::register_signal_callback(SignalType type, 
                                                         std::function<void(const SentimentSignal&)> callback) {
    pimpl_->signal_callbacks[type] = callback;
}

void SentimentExecutionPipeline::register_execution_callback(std::function<void(const ExecutionResult&)> callback) {
    pimpl_->execution_callback = callback;
}

void SentimentExecutionPipeline::set_execution_mode(ExecutionUrgency default_urgency) {
    pimpl_->default_urgency = default_urgency;
}

void SentimentExecutionPipeline::enable_aggressive_mode(bool enabled) {
    pimpl_->aggressive_mode = enabled;
    std::cout << "Aggressive mode " << (enabled ? "enabled" : "disabled") 
              << " (for memecoin sniping)" << std::endl;
}

PipelineMetrics SentimentExecutionPipeline::get_pipeline_metrics() const {
    std::lock_guard<std::mutex> lock(pimpl_->metrics_mutex);
    return pimpl_->metrics;
}

std::vector<ExecutionResult> SentimentExecutionPipeline::get_recent_executions(int limit) const {
    std::lock_guard<std::mutex> lock(pimpl_->execution_mutex);
    
    if (static_cast<int>(pimpl_->recent_executions.size()) <= limit) {
        return pimpl_->recent_executions;
    }
    
    return std::vector<ExecutionResult>(
        pimpl_->recent_executions.end() - limit,
        pimpl_->recent_executions.end()
    );
}

bool SentimentExecutionPipeline::health_check() const {
    return pimpl_->is_running.load() && 
           !pimpl_->emergency_stopped.load() &&
           pimpl_->metrics.success_rate.load() > 0.5;
}

std::string SentimentExecutionPipeline::get_pipeline_status() const {
    auto metrics = get_pipeline_metrics();
    
    std::ostringstream status;
    status << "Pipeline Status:\n";
    status << "  Running: " << (pimpl_->is_running.load() ? "Yes" : "No") << "\n";
    status << "  Signals Processed: " << metrics.signals_processed.load() << "\n";
    status << "  Success Rate: " << (metrics.success_rate.load() * 100.0) << "%\n";
    status << "  Avg Execution Latency: " << metrics.avg_execution_latency_us.load() << "μs\n";
    status << "  Network Congestion: " << (pimpl_->network_congestion_level.load() * 100.0) << "%\n";
    
    return status.str();
}

void SentimentExecutionPipeline::emergency_stop() {
    pimpl_->emergency_stopped.store(true);
    pimpl_->signal_queue.clear();
    std::cout << "🚨 EMERGENCY STOP ACTIVATED" << std::endl;
}

void SentimentExecutionPipeline::pause_signal_processing() {
    pimpl_->is_paused.store(true);
    std::cout << "⏸️ Signal processing paused" << std::endl;
}

void SentimentExecutionPipeline::resume_signal_processing() {
    pimpl_->is_paused.store(false);
    std::cout << "▶️ Signal processing resumed" << std::endl;
}

// Explicit template instantiation for our signal queue
template class PrioritySignalQueue<SentimentSignal>;

} // namespace hfx::ai
