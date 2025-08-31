/**
 * @file backtesting_engine.hpp
 * @brief Comprehensive backtesting and paper trading system for AI-driven HFT strategies
 * @version 1.0.0
 * 
 * Features:
 * - Historical data replay with microsecond precision
 * - Real-time strategy validation
 * - Risk metrics calculation
 * - Performance attribution analysis
 * - Paper trading mode for live testing
 */

#pragma once

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <functional>
#include <random>
#include <atomic>
#include <mutex>

#include "sentiment_engine.hpp"
#include "llm_decision_system.hpp"

namespace hfx::ai {

/**
 * @brief Historical market data point
 */
struct HistoricalDataPoint {
    uint64_t timestamp_ns;
    std::string symbol;
    double open;
    double high;
    double low;
    double close;
    double volume;
    double market_cap;
    
    // Sentiment data
    double twitter_sentiment{0.0};
    double reddit_sentiment{0.0};
    double news_sentiment{0.0};
    double whale_sentiment{0.0};
    
    // Technical indicators
    double rsi_14{50.0};
    double macd_signal{0.0};
    double bb_position{0.5};
};

/**
 * @brief Backtesting trade execution
 */
struct BacktestTrade {
    uint64_t timestamp_ns;
    std::string symbol;
    std::string trade_id;
    TradingDecision decision;
    
    // Execution details
    double entry_price;
    double exit_price;
    double quantity;
    double commission;
    double slippage;
    
    // Performance
    double pnl;
    double return_pct;
    uint64_t holding_time_ms;
    
    // Attribution
    std::string exit_reason; // "take_profit", "stop_loss", "timeout", "signal_reversal"
    bool was_profitable;
};

/**
 * @brief Backtesting performance metrics
 */
struct BacktestResults {
    // Time period
    uint64_t start_timestamp_ns;
    uint64_t end_timestamp_ns;
    uint32_t total_duration_days;
    
    // Trade statistics
    uint32_t total_trades;
    uint32_t winning_trades;
    uint32_t losing_trades;
    double win_rate_pct;
    
    // Returns
    double total_return_pct;
    double annualized_return_pct;
    double total_pnl;
    double max_profit;
    double max_loss;
    
    // Risk metrics
    double sharpe_ratio;
    double sortino_ratio;
    double calmar_ratio;
    double max_drawdown_pct;
    double volatility_pct;
    
    // Trade analysis
    double avg_trade_return_pct;
    double avg_winning_trade_pct;
    double avg_losing_trade_pct;
    double avg_holding_time_minutes;
    double profit_factor; // Gross profit / Gross loss
    
    // Latency analysis
    double avg_decision_latency_ms;
    double p95_decision_latency_ms;
    double p99_decision_latency_ms;
    
    // AI performance
    double sentiment_accuracy_pct;
    double llm_confidence_correlation;
    uint32_t false_positives;
    uint32_t false_negatives;
    
    std::vector<BacktestTrade> trades;
    std::vector<double> daily_returns;
    std::vector<double> equity_curve;
};

/**
 * @brief Backtesting configuration
 */
struct BacktestConfig {
    // Time period
    uint64_t start_timestamp_ns;
    uint64_t end_timestamp_ns;
    
    // Initial conditions
    double initial_capital;
    double max_position_size_pct; // % of capital per trade
    double commission_rate; // 0.001 = 0.1%
    double slippage_bps; // Slippage in basis points
    
    // Data sources
    std::vector<std::string> symbols;
    std::string data_source_path;
    bool include_sentiment_data;
    bool include_technical_indicators;
    
    // Simulation parameters
    uint64_t tick_resolution_ms; // Minimum time between decisions
    bool enable_realistic_latency; // Add realistic execution delays
    bool enable_market_impact; // Model market impact from trades
    
    // Risk management
    double max_drawdown_stop_pct; // Stop if drawdown exceeds this
    double daily_loss_limit_pct; // Stop trading for day if loss exceeds
    uint32_t max_positions_per_symbol;
    
    // AI configuration
    bool enable_ai_decisions;
    bool enable_sentiment_analysis;
    std::string strategy_config_path;
};

/**
 * @brief Paper trading configuration for live testing
 */
struct PaperTradingConfig {
    double virtual_capital;
    bool use_real_market_data;
    bool use_real_latency;
    bool log_decisions;
    std::string log_output_path;
    
    // Risk limits for paper trading
    double max_daily_loss;
    double max_position_size;
    uint32_t max_trades_per_hour;
};

/**
 * @brief Backtesting engine implementation
 */
class BacktestingEngine {
public:
    explicit BacktestingEngine();
    ~BacktestingEngine();
    
    // Configuration
    void set_config(const BacktestConfig& config);
    void set_sentiment_engine(std::shared_ptr<SentimentEngine> engine);
    void set_llm_decision_system(std::shared_ptr<LLMDecisionSystem> system);
    
    // Data loading
    bool load_historical_data(const std::string& data_path);
    bool load_sentiment_data(const std::string& sentiment_path);
    void add_data_point(const HistoricalDataPoint& data);
    
    // Backtesting execution
    BacktestResults run_backtest();
    BacktestResults run_backtest_parallel(); // Multi-threaded version
    
    // Strategy testing
    BacktestResults test_strategy(const std::string& strategy_name,
                                const std::unordered_map<std::string, std::string>& parameters);
    
    // Paper trading
    bool start_paper_trading(const PaperTradingConfig& config);
    void stop_paper_trading();
    bool is_paper_trading() const;
    
    // Analysis and reporting
    void generate_report(const BacktestResults& results, const std::string& output_path);
    void generate_equity_curve_chart(const BacktestResults& results, const std::string& output_path);
    void analyze_trade_attribution(const BacktestResults& results);
    
    // Real-time monitoring (for paper trading)
    using PerformanceCallback = std::function<void(const BacktestResults&)>;
    void register_performance_callback(PerformanceCallback callback);
    
    // Utilities
    std::vector<std::string> get_available_symbols() const;
    std::pair<uint64_t, uint64_t> get_data_time_range() const;
    
private:
    class BacktestImpl;
    std::unique_ptr<BacktestImpl> pimpl_;
};

/**
 * @brief Historical data provider interface
 */
class HistoricalDataProvider {
public:
    virtual ~HistoricalDataProvider() = default;
    
    virtual bool load_data(const std::string& symbol, 
                          uint64_t start_time, 
                          uint64_t end_time) = 0;
    
    virtual std::vector<HistoricalDataPoint> get_data(const std::string& symbol,
                                                     uint64_t start_time,
                                                     uint64_t end_time) = 0;
    
    virtual bool has_sentiment_data() const = 0;
    virtual bool has_technical_indicators() const = 0;
};

/**
 * @brief CSV-based historical data provider
 */
class CSVDataProvider : public HistoricalDataProvider {
public:
    explicit CSVDataProvider(const std::string& data_directory);
    ~CSVDataProvider() override;
    
    bool load_data(const std::string& symbol, 
                  uint64_t start_time, 
                  uint64_t end_time) override;
    
    std::vector<HistoricalDataPoint> get_data(const std::string& symbol,
                                             uint64_t start_time,
                                             uint64_t end_time) override;
    
    bool has_sentiment_data() const override;
    bool has_technical_indicators() const override;
    
private:
    class CSVImpl;
    std::unique_ptr<CSVImpl> pimpl_;
};

/**
 * @brief Strategy performance analyzer
 */
class PerformanceAnalyzer {
public:
    static BacktestResults calculate_metrics(const std::vector<BacktestTrade>& trades,
                                           double initial_capital);
    
    static double calculate_sharpe_ratio(const std::vector<double>& returns, 
                                       double risk_free_rate = 0.02);
    
    static double calculate_max_drawdown(const std::vector<double>& equity_curve);
    
    static double calculate_var(const std::vector<double>& returns, 
                              double confidence_level = 0.05);
    
    static std::vector<double> calculate_rolling_sharpe(const std::vector<double>& returns,
                                                       uint32_t window_days);
    
    // AI-specific metrics
    static double calculate_sentiment_accuracy(const std::vector<BacktestTrade>& trades);
    static double calculate_decision_latency_stats(const std::vector<BacktestTrade>& trades);
};

/**
 * @brief Market data simulator for realistic backtesting
 */
class MarketSimulator {
public:
    struct SimulationConfig {
        double base_spread_bps = 10.0; // Base bid-ask spread
        double volatility_multiplier = 1.0;
        double liquidity_impact_factor = 0.001; // Price impact per $1M traded
        bool enable_slippage = true;
        bool enable_partial_fills = true;
        double fill_probability = 0.95; // Probability of successful execution
    };
    
    explicit MarketSimulator(const SimulationConfig& config);
    
    // Simulate realistic order execution
    struct ExecutionResult {
        bool filled;
        double execution_price;
        double slippage_bps;
        uint64_t execution_delay_ns;
        std::string rejection_reason;
    };
    
    ExecutionResult simulate_market_order(const std::string& symbol,
                                        double quantity,
                                        double market_price,
                                        uint64_t timestamp_ns);
    
    ExecutionResult simulate_limit_order(const std::string& symbol,
                                       double quantity,
                                       double limit_price,
                                       double market_price,
                                       uint64_t timestamp_ns);
    
private:
    SimulationConfig config_;
    std::mt19937 rng_;
};

} // namespace hfx::ai
