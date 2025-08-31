#pragma once

#include <memory>
#include <string>
#include <vector>
#include <atomic>
#include <chrono>
#include <functional>
#include <queue>

namespace hfx::ai {

// Signal types from fastest bot research
enum class SignalType {
    TWITTER_SENTIMENT,
    SMART_MONEY_FLOW,
    TECHNICAL_BREAKOUT,
    NEWS_CATALYST,
    WHALE_MOVEMENT,
    MEMECOIN_MOMENTUM,
    ARBITRAGE_OPPORTUNITY,
    LIQUIDATION_CASCADE
};

enum class SignalStrength {
    WEAK = 1,
    MODERATE = 2,
    STRONG = 3,
    EXTREME = 4
};

enum class ExecutionUrgency {
    LOW_LATENCY,        // ~100ms - normal execution
    HIGH_FREQUENCY,     // ~10ms - like BonkBot speed
    ULTRA_FAST,         // ~1ms - GMGN-level speed  
    MICROSECOND         // <1ms - fastest possible
};

struct SentimentSignal {
    std::string signal_id;
    std::string token_address;
    std::string token_symbol;
    SignalType type;
    SignalStrength strength;
    ExecutionUrgency urgency;
    
    double sentiment_score;      // -1.0 to 1.0
    double confidence_level;     // 0.0 to 1.0
    double expected_price_impact; // Estimated %
    double position_size;        // USD amount
    std::string direction;       // "buy", "sell", "hold"
    
    std::chrono::microseconds timestamp;
    std::chrono::microseconds ttl;
    std::vector<std::string> data_sources;
    
    // Execution constraints
    double max_slippage_bps;
    double priority_fee_multiplier;
    bool use_mev_protection;
    bool allow_partial_fill;
    
    // Smart routing options - inspired by Jupiter integration
    std::vector<std::string> preferred_dexes;
    bool enable_split_routing;
    int max_route_hops;
};

struct ExecutionResult {
    std::string signal_id;
    std::string transaction_hash;
    bool success;
    double executed_price;
    double executed_quantity;
    double actual_slippage_bps;
    std::chrono::microseconds execution_latency;
    std::chrono::microseconds total_pipeline_latency;
    std::string error_message;
    double gas_cost;
    std::string execution_venue;
};

struct PipelineMetrics {
    std::atomic<uint64_t> signals_processed{0};
    std::atomic<uint64_t> signals_executed{0};
    std::atomic<uint64_t> signals_rejected{0};
    std::atomic<double> avg_execution_latency_us{0.0};
    std::atomic<double> avg_pipeline_latency_us{0.0};
    std::atomic<double> success_rate{0.0};
    std::atomic<uint64_t> mev_attacks_blocked{0};
    
    // Copy constructor and assignment for atomic members
    PipelineMetrics(const PipelineMetrics& other) {
        signals_processed.store(other.signals_processed.load());
        signals_executed.store(other.signals_executed.load());
        signals_rejected.store(other.signals_rejected.load());
        avg_execution_latency_us.store(other.avg_execution_latency_us.load());
        avg_pipeline_latency_us.store(other.avg_pipeline_latency_us.load());
        success_rate.store(other.success_rate.load());
        mev_attacks_blocked.store(other.mev_attacks_blocked.load());
    }
    
    PipelineMetrics& operator=(const PipelineMetrics& other) {
        if (this != &other) {
            signals_processed.store(other.signals_processed.load());
            signals_executed.store(other.signals_executed.load());
            signals_rejected.store(other.signals_rejected.load());
            avg_execution_latency_us.store(other.avg_execution_latency_us.load());
            avg_pipeline_latency_us.store(other.avg_pipeline_latency_us.load());
            success_rate.store(other.success_rate.load());
            mev_attacks_blocked.store(other.mev_attacks_blocked.load());
        }
        return *this;
    }
    
    PipelineMetrics() = default;
    PipelineMetrics(PipelineMetrics&&) = delete;
    PipelineMetrics& operator=(PipelineMetrics&&) = delete;
};

// Main Sentiment-to-Execution Pipeline - inspired by fastest bot architectures
class SentimentExecutionPipeline {
private:
    struct Impl;
    std::unique_ptr<Impl> pimpl_;

public:
    SentimentExecutionPipeline();
    ~SentimentExecutionPipeline();

    // Core pipeline initialization - like GMGN's ultra-fast setup
    bool initialize();
    void start_pipeline();
    void stop_pipeline();
    
    // Signal processing - inspired by BonkBot's speed
    void process_sentiment_signal(const SentimentSignal& signal);
    bool validate_signal_quality(const SentimentSignal& signal);
    double calculate_execution_priority(const SentimentSignal& signal);
    
    // Pre-execution analysis - like fastest bots do
    bool check_risk_limits(const SentimentSignal& signal);
    bool verify_token_safety(const std::string& token_address);
    double estimate_price_impact(const SentimentSignal& signal);
    std::vector<std::string> find_optimal_execution_path(const SentimentSignal& signal);
    
    // Ultra-fast execution - microsecond level like top bots
    ExecutionResult execute_signal_immediate(const SentimentSignal& signal);
    ExecutionResult execute_with_mev_protection(const SentimentSignal& signal);
    ExecutionResult execute_through_multiple_venues(const SentimentSignal& signal);
    
    // Advanced execution strategies - inspired by fastest bot research
    ExecutionResult snipe_new_listing(const SentimentSignal& signal);
    ExecutionResult arbitrage_execution(const SentimentSignal& signal);
    ExecutionResult momentum_chase_execution(const SentimentSignal& signal);
    ExecutionResult smart_money_copy_execution(const SentimentSignal& signal);
    
    // Pre-signed transaction management - critical for speed
    void pre_sign_transactions(const std::string& token_address, int quantity = 10);
    bool has_pre_signed_transaction(const std::string& token_address, const std::string& direction);
    std::string get_pre_signed_transaction(const std::string& token_address, const std::string& direction);
    void refresh_pre_signed_pool();
    
    // Real-time sentiment monitoring - like GMGN's AI analysis
    void monitor_real_time_sentiment();
    void process_twitter_sentiment_stream();
    void process_smart_money_movements();
    void process_technical_signals();
    void process_news_catalysts();
    
    // Priority fee optimization - for fastest execution
    double calculate_optimal_priority_fee(const SentimentSignal& signal);
    void monitor_network_congestion();
    double estimate_gas_price_for_speed(ExecutionUrgency urgency);
    
    // MEV protection - like BonkBot's MEV protection
    bool detect_mev_attack_risk(const SentimentSignal& signal);
    std::string bundle_transaction_for_protection(const SentimentSignal& signal);
    bool use_private_mempool(const SentimentSignal& signal);
    
    // Signal aggregation and filtering
    SentimentSignal aggregate_multi_source_signals(const std::vector<SentimentSignal>& signals);
    std::vector<SentimentSignal> filter_conflicting_signals(const std::vector<SentimentSignal>& signals);
    bool is_signal_obsolete(const SentimentSignal& signal);
    
    // Performance optimization - inspired by Photon's high volume
    void warm_up_execution_paths();
    void cache_token_metadata(const std::string& token_address);
    void pre_load_dex_routing_data();
    void optimize_rpc_connections();
    
    // Callback registration for real-time processing
    void register_signal_callback(SignalType type, 
                                 std::function<void(const SentimentSignal&)> callback);
    void register_execution_callback(std::function<void(const ExecutionResult&)> callback);
    void register_error_callback(std::function<void(const std::string&, const std::string&)> callback);
    
    // Configuration and tuning
    void set_execution_mode(ExecutionUrgency default_urgency);
    void set_risk_limits(double max_position_usd, double max_daily_loss);
    void set_slippage_tolerance(double max_slippage_bps);
    void enable_aggressive_mode(bool enabled); // For memecoin sniping
    
    // Monitoring and metrics
    PipelineMetrics get_pipeline_metrics() const;
    std::vector<ExecutionResult> get_recent_executions(int limit = 100) const;
    std::vector<SentimentSignal> get_pending_signals() const;
    double get_current_success_rate() const;
    
    // Health and diagnostics
    bool health_check() const;
    std::string get_pipeline_status() const;
    void run_latency_benchmarks();
    void calibrate_execution_timings();
    
    // Emergency controls
    void emergency_stop();
    void pause_signal_processing();
    void resume_signal_processing();
    void clear_pending_signals();
};

// Specialized signal processors for different sources
class TwitterSentimentProcessor {
public:
    static SentimentSignal process_tweet_stream(const std::string& tweet_data);
    static double calculate_viral_momentum(const std::vector<std::string>& tweets);
    static std::vector<std::string> extract_trending_tokens(const std::vector<std::string>& tweets);
    static SignalStrength assess_sentiment_strength(double sentiment_score, int engagement);
};

class SmartMoneyProcessor {
public:
    static SentimentSignal process_whale_movement(const std::string& transaction_data);
    static double calculate_insider_confidence(const std::string& wallet_address);
    static std::vector<SentimentSignal> detect_smart_money_patterns();
    static bool is_known_smart_wallet(const std::string& wallet_address);
};

class TechnicalSignalProcessor {
public:
    static SentimentSignal process_price_breakout(const std::string& price_data);
    static SentimentSignal process_volume_surge(const std::string& volume_data);
    static bool detect_momentum_shift(const std::string& token_address);
    static double calculate_technical_strength(const std::string& price_history);
};

// Ultra-fast execution engine - inspired by fastest bot architectures
class MicrosecondExecutionEngine {
public:
    static ExecutionResult execute_pre_signed_transaction(const std::string& signed_tx);
    static ExecutionResult execute_with_priority_fee(const SentimentSignal& signal, double priority_fee);
    static ExecutionResult execute_through_flashloan_if_needed(const SentimentSignal& signal);
    static bool can_execute_within_block(const SentimentSignal& signal);
    
    // Solana-specific optimizations
    static void warm_up_solana_connection();
    static void pre_load_token_accounts();
    static void optimize_transaction_size();
    static double estimate_solana_execution_time(const SentimentSignal& signal);
};

// Real-time signal queue with priority handling
template<typename T>
class PrioritySignalQueue {
private:
    struct SignalComparator {
        bool operator()(const T& a, const T& b) const {
            if (a.urgency != b.urgency) {
                return static_cast<int>(a.urgency) < static_cast<int>(b.urgency);
            }
            return a.confidence_level < b.confidence_level;
        }
    };
    
    std::priority_queue<T, std::vector<T>, SignalComparator> queue_;
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    std::atomic<bool> stop_flag_{false};

public:
    void push(const T& signal);
    T pop(); // Blocks until signal available
    bool try_pop(T& signal); // Non-blocking
    size_t size() const;
    void clear();
    void stop();
};

} // namespace hfx::ai
