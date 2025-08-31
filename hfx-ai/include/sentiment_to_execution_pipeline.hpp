/**
 * @file sentiment_to_execution_pipeline.hpp
 * @brief Complete sentiment-to-execution pipeline for ultra-fast crypto trading
 * @version 1.0.0
 * 
 * This pipeline connects sentiment analysis directly to trade execution,
 * creating an end-to-end automated trading system with sub-second latency.
 */

#pragma once

#include "sentiment_engine.hpp"
#include "hfx-ultra/include/smart_trading_engine.hpp"
#include "hfx-ultra/include/mev_shield.hpp"
#include "hfx-ultra/include/v3_tick_engine.hpp"
#include <memory>
#include <atomic>
#include <thread>
#include <mutex>
#include <queue>
#include <unordered_map>
#include <functional>

namespace hfx::ai {

/**
 * @brief Trading signal generated from sentiment analysis
 */
struct TradingSignal {
    std::string symbol;              // Token symbol (e.g., "PEPE", "SOL")
    std::string token_address;       // Contract address
    double sentiment_score;          // -1.0 to 1.0
    double confidence;              // 0.0 to 1.0
    double urgency;                 // 0.0 to 1.0 (how fast to act)
    
    // Trade recommendations
    enum Action { BUY, SELL, HOLD, STRONG_BUY, STRONG_SELL } action;
    double suggested_amount_usd;     // USD amount to trade
    double max_slippage_bps;        // Maximum acceptable slippage
    uint32_t execution_timeout_ms;   // Max time to execute
    
    // Risk parameters
    double stop_loss_pct;           // Stop loss percentage
    double take_profit_pct;         // Take profit percentage
    double position_size_pct;       // % of portfolio to allocate
    
    // Supporting data
    std::vector<std::string> supporting_sources;  // Twitter, Reddit, etc.
    uint64_t timestamp_ns;
    std::string reasoning;          // Human-readable explanation
    
    // Technical indicators
    double momentum_score;          // Price momentum alignment
    double volume_score;           // Volume analysis score
    double liquidity_score;        // DEX liquidity assessment
    double mev_risk_score;         // MEV vulnerability assessment
};

/**
 * @brief Execution result with comprehensive tracking
 */
struct ExecutionResult {
    std::string signal_id;
    std::string transaction_hash;
    bool success;
    double actual_price;
    double actual_amount;
    double actual_slippage_bps;
    uint32_t execution_latency_ms;
    double gas_cost_usd;
    double total_cost_usd;
    
    // Performance tracking
    double unrealized_pnl_usd;      // Current P&L
    double realized_pnl_usd;        // Realized P&L (if closed)
    uint64_t execution_timestamp_ns;
    
    std::string error_message;      // If failed
    
    // MEV protection metrics
    bool mev_protection_used;
    std::string protection_method;  // "jito_bundle", "flashbots", etc.
    double protection_cost_usd;
};

/**
 * @brief Real-time pipeline performance metrics
 */
struct PipelineMetrics {
    // Signal generation
    std::atomic<uint64_t> total_signals_generated{0};
    std::atomic<uint64_t> signals_executed{0};
    std::atomic<uint64_t> signals_filtered{0};
    std::atomic<uint64_t> avg_signal_latency_ns{0};
    
    // Execution performance
    std::atomic<uint64_t> successful_trades{0};
    std::atomic<uint64_t> failed_trades{0};
    std::atomic<uint64_t> avg_execution_latency_ms{0};
    std::atomic<double> total_pnl_usd{0.0};
    std::atomic<double> total_volume_usd{0.0};
    
    // Risk metrics
    std::atomic<double> max_drawdown_pct{0.0};
    std::atomic<double> sharpe_ratio{0.0};
    std::atomic<double> win_rate_pct{0.0};
    std::atomic<uint32_t> current_open_positions{0};
    
    // Real-time status
    std::atomic<bool> pipeline_active{false};
    std::atomic<uint64_t> last_signal_timestamp{0};
    std::atomic<uint64_t> last_execution_timestamp{0};
    
    // Custom copy constructor and assignment operator for atomic members
    PipelineMetrics() = default;
    PipelineMetrics(const PipelineMetrics& other) :
        total_signals_generated(other.total_signals_generated.load()),
        signals_executed(other.signals_executed.load()),
        signals_filtered(other.signals_filtered.load()),
        avg_signal_latency_ns(other.avg_signal_latency_ns.load()),
        successful_trades(other.successful_trades.load()),
        failed_trades(other.failed_trades.load()),
        avg_execution_latency_ms(other.avg_execution_latency_ms.load()),
        total_pnl_usd(other.total_pnl_usd.load()),
        total_volume_usd(other.total_volume_usd.load()),
        max_drawdown_pct(other.max_drawdown_pct.load()),
        sharpe_ratio(other.sharpe_ratio.load()),
        win_rate_pct(other.win_rate_pct.load()),
        current_open_positions(other.current_open_positions.load()),
        pipeline_active(other.pipeline_active.load()),
        last_signal_timestamp(other.last_signal_timestamp.load()),
        last_execution_timestamp(other.last_execution_timestamp.load()) {}
    
    PipelineMetrics& operator=(const PipelineMetrics& other) {
        if (this != &other) {
            total_signals_generated.store(other.total_signals_generated.load());
            signals_executed.store(other.signals_executed.load());
            signals_filtered.store(other.signals_filtered.load());
            avg_signal_latency_ns.store(other.avg_signal_latency_ns.load());
            successful_trades.store(other.successful_trades.load());
            failed_trades.store(other.failed_trades.load());
            avg_execution_latency_ms.store(other.avg_execution_latency_ms.load());
            total_pnl_usd.store(other.total_pnl_usd.load());
            total_volume_usd.store(other.total_volume_usd.load());
            max_drawdown_pct.store(other.max_drawdown_pct.load());
            sharpe_ratio.store(other.sharpe_ratio.load());
            win_rate_pct.store(other.win_rate_pct.load());
            current_open_positions.store(other.current_open_positions.load());
            pipeline_active.store(other.pipeline_active.load());
            last_signal_timestamp.store(other.last_signal_timestamp.load());
            last_execution_timestamp.store(other.last_execution_timestamp.load());
        }
        return *this;
    }
};

/**
 * @brief Configuration for the sentiment-to-execution pipeline
 */
struct PipelineConfig {
    // Signal filtering
    double min_sentiment_threshold = 0.3;      // Minimum sentiment to trigger
    double min_confidence_threshold = 0.6;     // Minimum confidence to trade
    double min_urgency_threshold = 0.4;        // Minimum urgency to trade
    
    // Risk management
    double max_position_size_usd = 1000.0;     // Max single position size
    double max_total_exposure_usd = 10000.0;   // Max total exposure
    double max_daily_loss_usd = 500.0;         // Daily loss limit
    uint32_t max_concurrent_trades = 5;        // Max simultaneous trades
    
    // Execution parameters
    uint32_t signal_execution_timeout_ms = 5000;  // Max time to execute signal
    double default_slippage_tolerance_bps = 100;  // 1% default slippage
    bool enable_mev_protection = true;
    bool enable_paper_trading = false;          // For testing
    
    // Portfolio management
    double portfolio_rebalance_threshold_pct = 5.0;  // When to rebalance
    uint32_t position_check_interval_ms = 1000;      // Position monitoring
    bool auto_take_profit = true;
    bool auto_stop_loss = true;
    
    // AI/ML features
    bool enable_momentum_analysis = true;
    bool enable_cross_source_validation = true;
    bool enable_whale_tracking = true;
    bool enable_technical_confirmation = true;
    
    // Chains and DEXes to trade on
    std::vector<std::string> enabled_chains = {"solana", "ethereum"};
    std::vector<std::string> enabled_dexes = {"raydium", "jupiter", "uniswap_v3"};
};

/**
 * @brief Callbacks for pipeline events
 */
using SignalCallback = std::function<void(const TradingSignal&)>;
using ExecutionCallback = std::function<void(const ExecutionResult&)>;
using AlertCallback = std::function<void(const std::string& alert_type, const std::string& message)>;

/**
 * @brief Complete sentiment-to-execution pipeline
 * 
 * This class orchestrates the entire flow from sentiment analysis to trade execution:
 * 1. Receives sentiment signals from multiple sources
 * 2. Applies AI/ML analysis and filters
 * 3. Generates trading signals with risk parameters
 * 4. Executes trades via the smart trading engine
 * 5. Monitors positions and manages risk
 * 6. Provides real-time performance analytics
 */
class SentimentToExecutionPipeline {
public:
    explicit SentimentToExecutionPipeline(const PipelineConfig& config = PipelineConfig{});
    ~SentimentToExecutionPipeline();
    
    // Core lifecycle
    bool initialize();
    void start();
    void stop();
    void shutdown();
    
    // Component integration
    void set_sentiment_engine(std::shared_ptr<SentimentEngine> engine);
    void set_trading_engine(std::shared_ptr<hfx::ultra::SmartTradingEngine> engine);
    void set_mev_shield(std::shared_ptr<hfx::ultra::MEVShield> shield);
    void set_v3_engine(std::shared_ptr<hfx::ultra::V3TickEngine> engine);
    
    // Signal processing
    void process_sentiment_signal(const SentimentSignal& sentiment);
    void manual_trading_signal(const TradingSignal& signal);  // For manual override
    
    // Configuration management
    void update_config(const PipelineConfig& config);
    PipelineConfig get_config() const;
    
    // Risk management
    void set_daily_loss_limit(double limit_usd);
    void set_position_size_limit(double limit_usd);
    void emergency_stop_all_trading();
    void pause_trading(const std::string& reason);
    void resume_trading();
    
    // Portfolio management
    std::vector<ExecutionResult> get_open_positions() const;
    std::vector<ExecutionResult> get_trade_history(uint32_t lookback_hours = 24) const;
    double get_portfolio_value() const;
    double get_unrealized_pnl() const;
    double get_realized_pnl() const;
    
    // Performance monitoring
    void get_metrics(PipelineMetrics& metrics_out) const;
    void reset_metrics();
    
    // Event callbacks
    void register_signal_callback(SignalCallback callback);
    void register_execution_callback(ExecutionCallback callback);
    void register_alert_callback(AlertCallback callback);
    
    // Advanced features
    void enable_smart_routing(bool enabled);
    void enable_cross_dex_arbitrage(bool enabled);
    void set_sentiment_model_weights(const std::unordered_map<std::string, double>& weights);
    
    // Analytics and insights
    std::vector<std::string> get_top_performing_signals(uint32_t count = 10) const;
    std::vector<std::string> get_worst_performing_signals(uint32_t count = 10) const;
    double calculate_signal_accuracy() const;
    std::string generate_performance_report() const;

private:
    class PipelineImpl;
    std::unique_ptr<PipelineImpl> pimpl_;
};

/**
 * @brief Factory for creating optimized pipeline configurations
 */
class PipelineConfigFactory {
public:
    // Preset configurations
    static PipelineConfig create_conservative_config();
    static PipelineConfig create_aggressive_config(); 
    static PipelineConfig create_scalping_config();
    static PipelineConfig create_memecoin_config();
    static PipelineConfig create_paper_trading_config();
    
    // Chain-specific configurations
    static PipelineConfig create_solana_config();
    static PipelineConfig create_ethereum_config();
    static PipelineConfig create_multi_chain_config();
    
    // Custom configurations
    static PipelineConfig create_custom_config(
        double max_position_usd,
        double sentiment_threshold,
        uint32_t max_trades,
        const std::vector<std::string>& chains
    );
};

/**
 * @brief Real-time signal analyzer with AI/ML integration
 */
class AdvancedSignalAnalyzer {
public:
    // Technical analysis integration
    static double calculate_momentum_score(const std::string& token_address);
    static double calculate_volume_score(const std::string& token_address);
    static double calculate_liquidity_score(const std::string& token_address);
    
    // Cross-source validation
    static double validate_signal_across_sources(const TradingSignal& signal);
    static bool detect_pump_and_dump(const TradingSignal& signal);
    static bool detect_whale_manipulation(const TradingSignal& signal);
    
    // Risk scoring
    static double calculate_mev_risk_score(const TradingSignal& signal);
    static double calculate_rugpull_risk(const std::string& token_address);
    static double calculate_overall_risk_score(const TradingSignal& signal);
    
    // AI/ML predictions
    static double predict_price_movement(const TradingSignal& signal, uint32_t timeframe_minutes);
    static double predict_optimal_exit_time(const TradingSignal& signal);
    static std::vector<std::string> generate_trade_reasoning(const TradingSignal& signal);
};

} // namespace hfx::ai
