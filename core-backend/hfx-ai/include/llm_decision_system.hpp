/**
 * @file llm_decision_system.hpp
 * @brief LLM-powered ultra-low latency trading decision system
 * @version 1.0.0
 * 
 * Autonomous Jarvis-style trading AI that combines sentiment analysis,
 * market microstructure, and LLM reasoning for optimal trading decisions.
 */

#pragma once

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <atomic>
#include <chrono>
#include <functional>
#include <variant>

#include "sentiment_engine.hpp"
#include "event_engine.hpp"

namespace hfx::ai {

/**
 * @brief Trading decision types
 */
enum class DecisionType {
    HOLD,
    BUY_SPOT,
    SELL_SPOT,
    BUY_LONG_LEVERAGE,
    SELL_SHORT_LEVERAGE,
    CLOSE_POSITION,
    HEDGE,
    ARBITRAGE,
    SENTIMENT_MOMENTUM,
    CONTRARIAN,
    EMERGENCY_EXIT
};

/**
 * @brief Market context for decision making
 */
struct MarketContext {
    std::string symbol;
    double current_price;
    double price_change_1m;
    double price_change_5m;
    double price_change_1h;
    double volume_24h;
    double market_cap;
    double volatility;
    double liquidity_score;
    uint64_t timestamp_ns;
    
    // Technical indicators
    double rsi_14;
    double macd_signal;
    double bb_position;  // Bollinger Band position
    double support_level;
    double resistance_level;
};

/**
 * @brief Trading decision with reasoning
 */
struct TradingDecision {
    DecisionType action;
    std::string symbol;
    double size_usd;              // Position size in USD
    double confidence;            // [0.0, 1.0] confidence level
    double expected_return;       // Expected return %
    double risk_score;            // [0.0, 1.0] risk assessment
    double time_horizon_ms;       // Expected holding time
    uint64_t timestamp_ns;
    
    // LLM reasoning
    std::string reasoning;        // Human-readable reasoning
    std::string key_factors;      // Main decision factors
    std::string risk_factors;     // Identified risks
    std::string exit_strategy;    // Exit conditions
    
    // Supporting data
    SentimentSignal sentiment;
    MarketContext market_context;
    std::vector<std::string> news_catalysts;
    
    // Execution parameters
    double stop_loss_pct;
    double take_profit_pct;
    bool use_limit_order;
    double max_slippage_pct;
    uint32_t timeout_ms;
};

/**
 * @brief Trading strategy configuration
 */
struct StrategyConfig {
    std::string name;
    bool enabled;
    double max_position_size_usd;
    double sentiment_threshold;
    double confidence_threshold;
    double max_risk_per_trade;
    uint32_t max_positions;
    uint32_t cooldown_ms;
    std::vector<std::string> allowed_symbols;
    std::string strategy_prompt;  // LLM strategy prompt
};

/**
 * @brief AI decision system statistics
 */
struct DecisionStats {
    std::atomic<uint64_t> total_decisions{0};
    std::atomic<uint64_t> profitable_decisions{0};
    std::atomic<uint64_t> avg_decision_latency_ns{0};
    std::atomic<uint64_t> llm_inference_latency_ns{0};
    std::atomic<double> total_pnl_usd{0.0};
    std::atomic<double> win_rate{0.0};
    std::atomic<double> sharpe_ratio{0.0};
    std::atomic<double> max_drawdown{0.0};
    std::atomic<uint32_t> active_positions{0};
    std::atomic<uint64_t> emergency_exits{0};
    
    // Custom copy constructor and assignment operator for atomic members
    DecisionStats() = default;
    DecisionStats(const DecisionStats& other) :
        total_decisions(other.total_decisions.load()),
        profitable_decisions(other.profitable_decisions.load()),
        avg_decision_latency_ns(other.avg_decision_latency_ns.load()),
        llm_inference_latency_ns(other.llm_inference_latency_ns.load()),
        total_pnl_usd(other.total_pnl_usd.load()),
        win_rate(other.win_rate.load()),
        sharpe_ratio(other.sharpe_ratio.load()),
        max_drawdown(other.max_drawdown.load()),
        active_positions(other.active_positions.load()),
        emergency_exits(other.emergency_exits.load()) {}
    
    DecisionStats& operator=(const DecisionStats& other) {
        if (this != &other) {
            total_decisions.store(other.total_decisions.load());
            profitable_decisions.store(other.profitable_decisions.load());
            avg_decision_latency_ns.store(other.avg_decision_latency_ns.load());
            llm_inference_latency_ns.store(other.llm_inference_latency_ns.load());
            total_pnl_usd.store(other.total_pnl_usd.load());
            win_rate.store(other.win_rate.load());
            sharpe_ratio.store(other.sharpe_ratio.load());
            max_drawdown.store(other.max_drawdown.load());
            active_positions.store(other.active_positions.load());
            emergency_exits.store(other.emergency_exits.load());
        }
        return *this;
    }
};

/**
 * @brief Decision callback
 */
using DecisionCallback = std::function<void(const TradingDecision&)>;

/**
 * @brief Ultra-low latency LLM-powered trading decision system
 * 
 * Features:
 * - Multi-modal input processing (sentiment, price, news, social)
 * - LLM-based reasoning with financial context
 * - Microsecond-level decision generation
 * - Risk-aware position sizing
 * - Autonomous strategy adaptation
 * - Real-time performance monitoring
 * - Emergency risk controls
 */
class LLMDecisionSystem {
public:
    explicit LLMDecisionSystem();
    ~LLMDecisionSystem();
    
    // Core lifecycle
    bool initialize();
    void shutdown();
    
    // LLM configuration
    bool load_trading_model(const std::string& model_path);
    void set_system_prompt(const std::string& prompt);
    void set_risk_parameters(double max_risk_per_trade, double max_total_exposure);
    
    // Strategy management
    void add_strategy(const StrategyConfig& strategy);
    void remove_strategy(const std::string& name);
    void enable_strategy(const std::string& name, bool enabled);
    void update_strategy_config(const std::string& name, const StrategyConfig& config);
    
    // Input processing
    void process_sentiment_signal(const SentimentSignal& signal);
    void process_market_data(const MarketContext& context);
    void process_news_event(const std::string& headline, const std::string& content, 
                           const std::vector<std::string>& symbols);
    
    // Decision callbacks
    void register_decision_callback(DecisionCallback callback);
    
    // Queries and monitoring
    std::vector<TradingDecision> get_recent_decisions(uint32_t count) const;
    void get_statistics(DecisionStats& stats_out) const;
    std::vector<std::string> get_active_strategies() const;
    
    // Control functions
    void emergency_stop();
    void pause_trading(bool paused);
    void force_exit_all_positions();
    
    // Performance tuning
    void set_decision_frequency(uint32_t decisions_per_second);
    void set_llm_batch_size(uint32_t batch_size);
    void enable_reasoning_cache(bool enabled);
    
private:
    class DecisionImpl;
    std::unique_ptr<DecisionImpl> pimpl_;
};

/**
 * @brief Research agent for autonomous strategy discovery
 */
class ResearchAgent {
public:
    struct ResearchFinding {
        std::string topic;
        std::string paper_title;
        std::string key_insight;
        std::string trading_application;
        double relevance_score;
        std::string implementation_suggestion;
        uint64_t timestamp_ns;
    };
    
    struct MarketIntelligence {
        std::string symbol;
        std::vector<std::string> trending_narratives;
        std::vector<std::string> catalyst_events;
        std::vector<std::string> risk_factors;
        double narrative_strength;
        double catalyst_probability;
        uint64_t timestamp_ns;
    };
    
    explicit ResearchAgent();
    ~ResearchAgent();
    
    bool initialize();
    
    // Research functions
    void start_continuous_research();
    void stop_research();
    
    // Paper analysis
    ResearchFinding analyze_paper(const std::string& paper_url);
    std::vector<ResearchFinding> search_papers(const std::string& query);
    
    // Market intelligence
    MarketIntelligence analyze_market_narrative(const std::string& symbol);
    std::vector<std::string> detect_trending_topics();
    std::vector<std::string> identify_market_catalysts();
    
    // Strategy suggestions
    StrategyConfig generate_strategy_from_research(const ResearchFinding& finding);
    void suggest_strategy_improvements(const std::string& strategy_name);
    
    // Callbacks
    using ResearchCallback = std::function<void(const ResearchFinding&)>;
    using IntelligenceCallback = std::function<void(const MarketIntelligence&)>;
    
    void register_research_callback(ResearchCallback callback);
    void register_intelligence_callback(IntelligenceCallback callback);
    
private:
    class ResearchImpl;
    std::unique_ptr<ResearchImpl> pimpl_;
};

/**
 * @brief Jarvis-style conversational AI interface
 */
class JarvisInterface {
public:
    struct ConversationContext {
        std::string user_id;
        std::vector<std::string> conversation_history;
        std::unordered_map<std::string, std::string> user_preferences;
        uint64_t session_start_time;
    };
    
    struct JarvisResponse {
        std::string response_text;
        std::vector<TradingDecision> suggested_actions;
        std::vector<std::string> market_insights;
        std::string system_status;
        bool requires_confirmation;
    };
    
    explicit JarvisInterface();
    ~JarvisInterface();
    
    bool initialize();
    
    // Conversation interface
    JarvisResponse process_user_input(const std::string& input, const std::string& user_id);
    void start_conversation_session(const std::string& user_id);
    void end_conversation_session(const std::string& user_id);
    
    // System integration
    void set_decision_system(std::shared_ptr<LLMDecisionSystem> system);
    void set_sentiment_engine(std::shared_ptr<SentimentEngine> engine);
    void set_research_agent(std::shared_ptr<ResearchAgent> agent);
    
    // Voice interface (future)
    void enable_voice_interface(bool enabled);
    JarvisResponse process_voice_input(const std::vector<uint8_t>& audio_data);
    
    // Proactive notifications
    void enable_proactive_alerts(bool enabled);
    void set_alert_preferences(const std::string& user_id, 
                              const std::unordered_map<std::string, std::string>& preferences);
    
private:
    class JarvisImpl;
    std::unique_ptr<JarvisImpl> pimpl_;
};

/**
 * @brief AI strategy generator using LLM creativity
 */
class AIStrategyGenerator {
public:
    struct GeneratedStrategy {
        std::string name;
        std::string description;
        std::string entry_logic;
        std::string exit_logic;
        std::string risk_management;
        StrategyConfig config;
        double backtested_sharpe;
        double estimated_capacity;
        std::vector<std::string> required_data_sources;
        std::string code_implementation;
        uint64_t created_timestamp;
    };
    
    explicit AIStrategyGenerator();
    ~AIStrategyGenerator();
    
    bool initialize();
    
    // Strategy generation
    GeneratedStrategy generate_strategy_from_prompt(const std::string& prompt);
    GeneratedStrategy generate_strategy_from_market_conditions();
    std::vector<GeneratedStrategy> generate_strategy_variations(const std::string& base_strategy);
    
    // Strategy optimization
    StrategyConfig optimize_strategy_parameters(const StrategyConfig& base_config);
    GeneratedStrategy combine_strategies(const std::vector<std::string>& strategy_names);
    
    // Market adaptation
    void adapt_strategies_to_regime(const std::string& market_regime);
    void evolve_underperforming_strategies();
    
    // Strategy evaluation
    double backtest_strategy(const GeneratedStrategy& strategy, uint32_t days_lookback);
    double estimate_strategy_capacity(const GeneratedStrategy& strategy);
    
private:
    class GeneratorImpl;
    std::unique_ptr<GeneratorImpl> pimpl_;
};

} // namespace hfx::ai
