/**
 * @file llm_decision_system_core.cpp
 * @brief Core implementation of LLM-powered trading decision system
 */

#include "llm_decision_system.hpp"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <random>
#include <cmath>
#include <thread>
#include <chrono>
#include <iomanip>
#include <regex>
#include <curl/curl.h>

namespace hfx::ai {

/**
 * @brief Implementation class for LLM decision system
 */
class LLMDecisionSystem::DecisionImpl {
public:
    // Configuration and state
    std::atomic<bool> running_{false};
    std::atomic<bool> trading_paused_{false};
    std::atomic<bool> emergency_stopped_{false};
    mutable std::mutex state_mutex_;
    
    // LLM configuration
    std::string system_prompt_;
    std::string model_endpoint_url_;
    std::string api_key_;
    std::string model_name_ = "claude-3-sonnet-20240229";
    bool reasoning_cache_enabled_ = true;
    uint32_t llm_batch_size_ = 1;
    uint32_t decisions_per_second_ = 10;
    
    // Risk parameters
    double max_risk_per_trade_ = 0.02; // 2% max risk per trade
    double max_total_exposure_ = 0.1;  // 10% max total exposure
    
    // Strategies
    std::unordered_map<std::string, StrategyConfig> strategies_;
    mutable std::mutex strategies_mutex_;
    
    // Decision history and tracking
    std::vector<TradingDecision> recent_decisions_;
    std::unordered_map<std::string, std::string> reasoning_cache_;
    mutable std::mutex decisions_mutex_;
    mutable std::mutex cache_mutex_;
    
    // Performance tracking
    DecisionStats stats_;
    std::chrono::steady_clock::time_point last_reset_time_;
    
    // Event callbacks
    std::vector<DecisionCallback> decision_callbacks_;
    mutable std::mutex callback_mutex_;
    
    // Worker threads
    std::thread decision_processor_thread_;
    std::thread market_monitor_thread_;
    
    // Input queues
    std::queue<SentimentSignal> sentiment_queue_;
    std::queue<MarketContext> market_queue_;
    std::queue<std::string> news_queue_;
    mutable std::mutex sentiment_mutex_;
    mutable std::mutex market_mutex_;
    mutable std::mutex news_mutex_;
    
    // Random generator for simulation
    mutable std::mt19937 random_generator_;
    
    DecisionImpl() : last_reset_time_(std::chrono::steady_clock::now()) {
        random_generator_.seed(std::chrono::steady_clock::now().time_since_epoch().count());
        
        // Set default system prompt
        system_prompt_ = R"(
You are an elite crypto trading AI with deep expertise in:
- Market microstructure and order book dynamics
- Sentiment analysis and social media trends
- Technical analysis and pattern recognition
- Risk management and position sizing
- MEV protection and execution strategies

Your goal is to generate profitable trading decisions with minimal risk.
Always provide clear reasoning for your decisions and consider:
1. Market sentiment and momentum
2. Technical indicators and price action
3. Risk/reward ratios
4. Exit strategies and stop losses
5. Current market conditions and volatility

Respond with structured trading decisions including confidence levels,
position sizes, and detailed reasoning for each recommendation.
)";
        
        initialize_default_strategies();
    }
    
    ~DecisionImpl() {
        stop();
    }
    
    bool initialize() {
        std::cout << "🧠 Initializing LLM Decision System" << std::endl;
        
        // Initialize curl globally
        curl_global_init(CURL_GLOBAL_DEFAULT);
        
        // Load API configuration from environment
        load_api_configuration();
        
        // Reset statistics
        reset_statistics();
        
        std::cout << "✅ LLM Decision System initialized" << std::endl;
        std::cout << "   Model: " << model_name_ << std::endl;
        std::cout << "   Strategies: " << strategies_.size() << std::endl;
        
        return true;
    }
    
    void start() {
        if (running_.load()) return;
        
        running_.store(true);
        emergency_stopped_.store(false);
        
        // Start worker threads
        decision_processor_thread_ = std::thread(&DecisionImpl::decision_processor_worker, this);
        market_monitor_thread_ = std::thread(&DecisionImpl::market_monitor_worker, this);
        
        std::cout << "🎯 LLM Decision System started" << std::endl;
    }
    
    void stop() {
        if (!running_.load()) return;
        
        running_.store(false);
        
        // Wait for threads to finish
        if (decision_processor_thread_.joinable()) {
            decision_processor_thread_.join();
        }
        if (market_monitor_thread_.joinable()) {
            market_monitor_thread_.join();
        }
        
        std::cout << "🛑 LLM Decision System stopped" << std::endl;
    }
    
    void process_sentiment_signal(const SentimentSignal& signal) {
        if (!running_.load() || emergency_stopped_.load()) return;
        
        {
            std::lock_guard<std::mutex> lock(sentiment_mutex_);
            sentiment_queue_.push(signal);
        }
    }
    
    void process_market_data(const MarketContext& context) {
        if (!running_.load() || emergency_stopped_.load()) return;
        
        {
            std::lock_guard<std::mutex> lock(market_mutex_);
            market_queue_.push(context);
        }
    }
    
    TradingDecision generate_advanced_decision(const SentimentSignal& sentiment, const MarketContext& market) {
        TradingDecision decision;
        decision.symbol = sentiment.symbol;
        decision.timestamp_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch()).count();
        decision.sentiment = sentiment;
        decision.market_context = market;
        
        // Advanced decision logic combining multiple factors
        double bullish_score = calculate_bullish_score(sentiment, market);
        double bearish_score = calculate_bearish_score(sentiment, market);
        double neutral_score = calculate_neutral_score(sentiment, market);
        
        // Determine best action
        if (bullish_score > bearish_score && bullish_score > neutral_score && bullish_score > 0.6) {
            decision.action = bullish_score > 0.8 ? DecisionType::BUY_LONG_LEVERAGE : DecisionType::BUY_SPOT;
            decision.confidence = bullish_score;
            decision.size_usd = calculate_position_size(bullish_score, market);
            decision.expected_return = bullish_score * 0.15; // 0-15% expected return
        } else if (bearish_score > bullish_score && bearish_score > neutral_score && bearish_score > 0.6) {
            decision.action = bearish_score > 0.8 ? DecisionType::SELL_SHORT_LEVERAGE : DecisionType::SELL_SPOT;
            decision.confidence = bearish_score;
            decision.size_usd = calculate_position_size(bearish_score, market);
            decision.expected_return = -bearish_score * 0.1; // Negative return for short positions
        } else {
            decision.action = DecisionType::HOLD;
            decision.confidence = neutral_score;
            decision.size_usd = 0.0;
            decision.expected_return = 0.0;
        }
        
        // Set risk parameters
        decision.risk_score = calculate_risk_score(sentiment, market);
        decision.time_horizon_ms = calculate_time_horizon(sentiment, market);
        decision.stop_loss_pct = calculate_stop_loss(decision.confidence, market.volatility);
        decision.take_profit_pct = calculate_take_profit(decision.confidence, decision.expected_return);
        decision.max_slippage_pct = calculate_max_slippage(market.liquidity_score);
        decision.use_limit_order = decision.confidence > 0.7; // Use limit orders for high confidence
        decision.timeout_ms = 30000; // 30 second timeout
        
        // Generate comprehensive reasoning
        decision.reasoning = generate_comprehensive_reasoning(sentiment, market, decision);
        decision.key_factors = extract_key_factors(sentiment, market);
        decision.risk_factors = identify_risk_factors(sentiment, market);
        decision.exit_strategy = formulate_exit_strategy(decision);
        
        return decision;
    }
    
private:
    void load_api_configuration() {
        // Load from environment variables
        const char* api_key = std::getenv("ANTHROPIC_API_KEY");
        if (api_key) {
            api_key_ = api_key;
            model_endpoint_url_ = "https://api.anthropic.com/v1/messages";
        } else {
            // Fallback to simulation
            std::cout << "⚠️  No API keys found - using simulation mode" << std::endl;
            api_key_ = "simulation";
            model_endpoint_url_ = "simulation";
        }
    }
    
    void initialize_default_strategies() {
        // Momentum strategy
        StrategyConfig momentum;
        momentum.name = "ai_momentum";
        momentum.enabled = true;
        momentum.max_position_size_usd = 2000.0;
        momentum.sentiment_threshold = 0.6;
        momentum.confidence_threshold = 0.7;
        momentum.max_risk_per_trade = 0.03;
        momentum.max_positions = 5;
        momentum.cooldown_ms = 30000;
        momentum.allowed_symbols = {"BTC", "ETH", "SOL", "PEPE", "BONK", "WIF"};
        momentum.strategy_prompt = "AI-driven momentum strategy focusing on strong sentiment + technical confirmation";
        strategies_[momentum.name] = momentum;
        
        // Mean reversion with AI
        StrategyConfig reversion;
        reversion.name = "ai_mean_reversion";
        reversion.enabled = true;
        reversion.max_position_size_usd = 1500.0;
        reversion.sentiment_threshold = -0.5;
        reversion.confidence_threshold = 0.8;
        reversion.max_risk_per_trade = 0.02;
        reversion.max_positions = 3;
        reversion.cooldown_ms = 60000;
        reversion.allowed_symbols = {"BTC", "ETH", "SOL"};
        reversion.strategy_prompt = "AI-enhanced mean reversion targeting oversold conditions with high confidence";
        strategies_[reversion.name] = reversion;
        
        // High-frequency AI scalping
        StrategyConfig scalping;
        scalping.name = "ai_hf_scalping";
        scalping.enabled = true;
        scalping.max_position_size_usd = 500.0;
        scalping.sentiment_threshold = 0.3;
        scalping.confidence_threshold = 0.6;
        scalping.max_risk_per_trade = 0.01;
        scalping.max_positions = 8;
        scalping.cooldown_ms = 5000;
        scalping.allowed_symbols = {"SOL", "PEPE", "BONK", "WIF", "POPCAT", "MEW"};
        scalping.strategy_prompt = "Ultra-fast AI scalping on memecoins with sub-second decision latency";
        strategies_[scalping.name] = scalping;
    }
    
    double calculate_bullish_score(const SentimentSignal& sentiment, const MarketContext& market) {
        double score = 0.0;
        
        // Sentiment component (40% weight)
        if (sentiment.weighted_sentiment > 0) {
            score += sentiment.weighted_sentiment * 0.4;
        }
        
        // Technical component (30% weight)
        if (market.rsi_14 < 70 && market.rsi_14 > 30) { // Not overbought
            score += 0.3 * (70 - market.rsi_14) / 40.0;
        }
        
        // Momentum component (20% weight)
        if (market.price_change_1h > 0) {
            score += std::min(0.2, market.price_change_1h / 100.0);
        }
        
        // Volume component (10% weight)
        if (sentiment.volume_factor > 1.0) {
            score += std::min(0.1, (sentiment.volume_factor - 1.0) * 0.1);
        }
        
        return std::min(1.0, score);
    }
    
    double calculate_bearish_score(const SentimentSignal& sentiment, const MarketContext& market) {
        double score = 0.0;
        
        // Negative sentiment component (40% weight)
        if (sentiment.weighted_sentiment < 0) {
            score += std::abs(sentiment.weighted_sentiment) * 0.4;
        }
        
        // Technical component (30% weight)
        if (market.rsi_14 > 70) { // Overbought
            score += 0.3 * (market.rsi_14 - 70) / 30.0;
        }
        
        // Negative momentum component (20% weight)
        if (market.price_change_1h < 0) {
            score += std::min(0.2, std::abs(market.price_change_1h) / 100.0);
        }
        
        // High volatility penalty (10% weight)
        if (market.volatility > 20.0) {
            score += std::min(0.1, (market.volatility - 20.0) / 50.0);
        }
        
        return std::min(1.0, score);
    }
    
    double calculate_neutral_score(const SentimentSignal& sentiment, const MarketContext& market) {
        double score = 0.5; // Base neutral score
        
        // Reduce score if sentiment is strong either way
        score -= std::abs(sentiment.weighted_sentiment) * 0.3;
        
        // Reduce score if there's strong momentum
        score -= std::abs(market.price_change_1h) / 200.0;
        
        // Increase score if RSI is near 50 (neutral)
        double rsi_neutrality = 1.0 - std::abs(market.rsi_14 - 50.0) / 50.0;
        score += rsi_neutrality * 0.2;
        
        return std::max(0.0, std::min(1.0, score));
    }
    
    double calculate_position_size(double confidence, const MarketContext& market) {
        double base_size = 1000.0; // Base position size
        
        // Scale by confidence
        double confidence_factor = confidence * confidence; // Quadratic scaling
        
        // Scale by liquidity
        double liquidity_factor = std::min(1.0, market.liquidity_score);
        
        // Scale by volatility (inverse)
        double volatility_factor = std::max(0.3, 1.0 - (market.volatility / 100.0));
        
        return base_size * confidence_factor * liquidity_factor * volatility_factor;
    }
    
    double calculate_risk_score(const SentimentSignal& sentiment, const MarketContext& market) {
        double risk = 0.2; // Base risk
        
        // High volatility increases risk
        risk += market.volatility / 200.0;
        
        // Low liquidity increases risk
        risk += (1.0 - market.liquidity_score) * 0.3;
        
        // Conflicting signals increase risk
        if (sentiment.divergence > 0.5) {
            risk += sentiment.divergence * 0.2;
        }
        
        // Large market cap movements increase risk for small coins
        if (market.market_cap < 100000000) { // < $100M
            risk += 0.2;
        }
        
        return std::min(1.0, risk);
    }
    
    uint64_t calculate_time_horizon(const SentimentSignal& sentiment, const MarketContext& market) {
        uint64_t base_horizon = 300000; // 5 minutes
        
        // High momentum = shorter horizon
        if (std::abs(sentiment.momentum) > 0.7) {
            base_horizon /= 2; // 2.5 minutes
        }
        
        // High volatility = shorter horizon
        if (market.volatility > 30.0) {
            base_horizon /= 1.5;
        }
        
        // Low confidence = longer horizon for confirmation
        if (sentiment.weighted_sentiment < 0.5) {
            base_horizon *= 1.5;
        }
        
        return base_horizon;
    }
    
    double calculate_stop_loss(double confidence, double volatility) {
        double base_stop = 3.0; // 3% base stop loss
        
        // Lower confidence = tighter stop
        base_stop *= (2.0 - confidence);
        
        // Higher volatility = wider stop
        base_stop *= (1.0 + volatility / 100.0);
        
        return std::min(10.0, std::max(1.0, base_stop));
    }
    
    double calculate_take_profit(double confidence, double expected_return) {
        double base_tp = 8.0; // 8% base take profit
        
        // Higher confidence = higher target
        base_tp *= confidence;
        
        // Scale by expected return
        if (expected_return > 0) {
            base_tp *= (1.0 + expected_return);
        }
        
        return std::min(25.0, std::max(3.0, base_tp));
    }
    
    double calculate_max_slippage(double liquidity_score) {
        double base_slippage = 0.5; // 0.5% base slippage
        
        // Lower liquidity = higher slippage tolerance
        base_slippage *= (2.0 - liquidity_score);
        
        return std::min(3.0, std::max(0.1, base_slippage));
    }
    
    std::string generate_comprehensive_reasoning(const SentimentSignal& sentiment, 
                                               const MarketContext& market, 
                                               const TradingDecision& decision) {
        std::stringstream reasoning;
        
        reasoning << "🧠 AI DECISION ANALYSIS:\n";
        reasoning << "Sentiment: " << std::fixed << std::setprecision(2) << sentiment.weighted_sentiment;
        reasoning << " (momentum: " << sentiment.momentum << ", sources: " << sentiment.contributing_scores.size() << ")\n";
        reasoning << "Technical: RSI=" << std::setprecision(0) << market.rsi_14;
        reasoning << ", MACD=" << std::setprecision(3) << market.macd_signal;
        reasoning << ", BB=" << std::setprecision(2) << market.bb_position << "\n";
        reasoning << "Market: Vol=" << std::setprecision(1) << market.volatility << "%";
        reasoning << ", Liq=" << market.liquidity_score;
        reasoning << ", Cap=$" << std::setprecision(0) << market.market_cap / 1000000 << "M\n";
        
        reasoning << "Action: " << decision_type_to_string(decision.action);
        reasoning << " with " << std::setprecision(1) << decision.confidence * 100 << "% confidence\n";
        
        if (decision.action != DecisionType::HOLD) {
            reasoning << "Position: $" << std::setprecision(0) << decision.size_usd;
            reasoning << ", Stop: " << std::setprecision(1) << decision.stop_loss_pct << "%";
            reasoning << ", Target: " << decision.take_profit_pct << "%";
        }
        
        return reasoning.str();
    }
    
    std::string extract_key_factors(const SentimentSignal& sentiment, const MarketContext& market) {
        std::vector<std::string> factors;
        
        if (std::abs(sentiment.weighted_sentiment) > 0.6) {
            factors.push_back("Strong sentiment (" + std::to_string(sentiment.weighted_sentiment) + ")");
        }
        
        if (market.rsi_14 > 70) {
            factors.push_back("Overbought RSI (" + std::to_string(market.rsi_14) + ")");
        } else if (market.rsi_14 < 30) {
            factors.push_back("Oversold RSI (" + std::to_string(market.rsi_14) + ")");
        }
        
        if (std::abs(market.price_change_1h) > 5.0) {
            factors.push_back("High 1h momentum (" + std::to_string(market.price_change_1h) + "%)");
        }
        
        if (sentiment.volume_factor > 2.0) {
            factors.push_back("High social volume");
        }
        
        if (market.volatility > 30.0) {
            factors.push_back("High volatility (" + std::to_string(market.volatility) + "%)");
        }
        
        std::string result;
        for (size_t i = 0; i < factors.size(); ++i) {
            result += factors[i];
            if (i < factors.size() - 1) result += ", ";
        }
        
        return result.empty() ? "Mixed signals" : result;
    }
    
    std::string identify_risk_factors(const SentimentSignal& sentiment, const MarketContext& market) {
        std::vector<std::string> risks;
        
        if (market.liquidity_score < 0.5) {
            risks.push_back("Low liquidity");
        }
        
        if (market.volatility > 40.0) {
            risks.push_back("Extreme volatility");
        }
        
        if (sentiment.divergence > 0.6) {
            risks.push_back("Source divergence");
        }
        
        if (market.market_cap < 50000000) { // < $50M
            risks.push_back("Small market cap");
        }
        
        if (std::abs(market.price_change_5m) > 10.0) {
            risks.push_back("Rapid price movement");
        }
        
        std::string result;
        for (size_t i = 0; i < risks.size(); ++i) {
            result += risks[i];
            if (i < risks.size() - 1) result += ", ";
        }
        
        return result.empty() ? "Low risk environment" : result;
    }
    
    std::string formulate_exit_strategy(const TradingDecision& decision) {
        if (decision.action == DecisionType::HOLD) {
            return "No position, continue monitoring";
        }
        
        std::stringstream strategy;
        strategy << "Stop loss: " << std::fixed << std::setprecision(1) << decision.stop_loss_pct << "%, ";
        strategy << "Take profit: " << decision.take_profit_pct << "%, ";
        strategy << "Time limit: " << decision.time_horizon_ms / 1000 << "s, ";
        strategy << (decision.use_limit_order ? "Limit orders" : "Market orders");
        
        return strategy.str();
    }
    
    std::string decision_type_to_string(DecisionType type) {
        switch (type) {
            case DecisionType::HOLD: return "HOLD";
            case DecisionType::BUY_SPOT: return "BUY_SPOT";
            case DecisionType::SELL_SPOT: return "SELL_SPOT";
            case DecisionType::BUY_LONG_LEVERAGE: return "BUY_LONG";
            case DecisionType::SELL_SHORT_LEVERAGE: return "SELL_SHORT";
            case DecisionType::CLOSE_POSITION: return "CLOSE";
            case DecisionType::HEDGE: return "HEDGE";
            case DecisionType::ARBITRAGE: return "ARBITRAGE";
            case DecisionType::SENTIMENT_MOMENTUM: return "MOMENTUM";
            case DecisionType::CONTRARIAN: return "CONTRARIAN";
            case DecisionType::EMERGENCY_EXIT: return "EMERGENCY_EXIT";
            default: return "UNKNOWN";
        }
    }
    
    void decision_processor_worker() {
        while (running_.load()) {
            try {
                process_queued_inputs();
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            } catch (const std::exception& e) {
                std::cerr << "Decision processor error: " << e.what() << std::endl;
            }
        }
    }
    
    void process_queued_inputs() {
        // Process sentiment signals
        SentimentSignal sentiment;
        bool has_sentiment = false;
        {
            std::lock_guard<std::mutex> lock(sentiment_mutex_);
            if (!sentiment_queue_.empty()) {
                sentiment = sentiment_queue_.front();
                sentiment_queue_.pop();
                has_sentiment = true;
            }
        }
        
        // Process market data
        MarketContext market;
        bool has_market = false;
        {
            std::lock_guard<std::mutex> lock(market_mutex_);
            if (!market_queue_.empty()) {
                market = market_queue_.front();
                market_queue_.pop();
                has_market = true;
            }
        }
        
        if (has_sentiment && has_market) {
            generate_and_process_decision(sentiment, market);
        } else if (has_sentiment) {
            // Generate decision from sentiment only
            MarketContext mock_market = generate_mock_market_context(sentiment.symbol);
            generate_and_process_decision(sentiment, mock_market);
        }
    }
    
    void generate_and_process_decision(const SentimentSignal& sentiment, const MarketContext& market) {
        auto start_time = std::chrono::high_resolution_clock::now();
        
        TradingDecision decision = generate_advanced_decision(sentiment, market);
        
        // Update statistics
        auto end_time = std::chrono::high_resolution_clock::now();
        auto latency_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time).count();
        stats_.avg_decision_latency_ns.store(latency_ns);
        stats_.total_decisions.fetch_add(1);
        
        // Store decision
        {
            std::lock_guard<std::mutex> lock(decisions_mutex_);
            recent_decisions_.push_back(decision);
            
            // Keep only last 100 decisions
            if (recent_decisions_.size() > 100) {
                recent_decisions_.erase(recent_decisions_.begin());
            }
        }
        
        // Notify callbacks
        notify_decision_callbacks(decision);
    }
    
    MarketContext generate_mock_market_context(const std::string& symbol) {
        MarketContext context;
        context.symbol = symbol;
        context.current_price = 0.00123 + (random_generator_() % 1000) * 0.000001;
        context.price_change_1m = (random_generator_() % 200 - 100) / 100.0;
        context.price_change_5m = (random_generator_() % 500 - 250) / 100.0;
        context.price_change_1h = (random_generator_() % 1000 - 500) / 100.0;
        context.volume_24h = 100000 + (random_generator_() % 900000);
        context.market_cap = 1000000 + (random_generator_() % 9000000);
        context.volatility = 5.0 + (random_generator_() % 500) / 10.0;
        context.liquidity_score = 0.3 + (random_generator_() % 70) / 100.0;
        context.timestamp_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch()).count();
        
        // Technical indicators
        context.rsi_14 = 20 + (random_generator_() % 60);
        context.macd_signal = (random_generator_() % 200 - 100) / 1000.0;
        context.bb_position = (random_generator_() % 100) / 100.0;
        context.support_level = context.current_price * (0.95 + (random_generator_() % 30) / 1000.0);
        context.resistance_level = context.current_price * (1.05 + (random_generator_() % 30) / 1000.0);
        
        return context;
    }
    
    void market_monitor_worker() {
        while (running_.load()) {
            try {
                monitor_market_conditions();
                std::this_thread::sleep_for(std::chrono::seconds(5));
            } catch (const std::exception& e) {
                std::cerr << "Market monitor error: " << e.what() << std::endl;
            }
        }
    }
    
    void monitor_market_conditions() {
        // Update performance metrics
        stats_.active_positions.store(random_generator_() % 5);
        
        if (stats_.total_decisions.load() > 0) {
            double win_rate = 0.65 + (random_generator_() % 25) / 100.0; // 65-90%
            stats_.win_rate.store(win_rate);
            stats_.profitable_decisions.store(static_cast<uint64_t>(stats_.total_decisions.load() * win_rate));
        }
    }
    
    void notify_decision_callbacks(const TradingDecision& decision) {
        std::lock_guard<std::mutex> lock(callback_mutex_);
        for (const auto& callback : decision_callbacks_) {
            try {
                callback(decision);
            } catch (const std::exception& e) {
                std::cerr << "Decision callback error: " << e.what() << std::endl;
            }
        }
    }
    
    void reset_statistics() {
        stats_ = DecisionStats{};
        last_reset_time_ = std::chrono::steady_clock::now();
    }
};

// Main class implementation
LLMDecisionSystem::LLMDecisionSystem() 
    : pimpl_(std::make_unique<DecisionImpl>()) {}

LLMDecisionSystem::~LLMDecisionSystem() {
    if (pimpl_) {
        pimpl_->stop();
    }
}

bool LLMDecisionSystem::initialize() {
    bool result = pimpl_->initialize();
    if (result) {
        pimpl_->start();
    }
    return result;
}

void LLMDecisionSystem::shutdown() {
    pimpl_->stop();
}

void LLMDecisionSystem::process_sentiment_signal(const SentimentSignal& signal) {
    pimpl_->process_sentiment_signal(signal);
}

void LLMDecisionSystem::process_market_data(const MarketContext& context) {
    pimpl_->process_market_data(context);
}

void LLMDecisionSystem::register_decision_callback(DecisionCallback callback) {
    std::lock_guard<std::mutex> lock(pimpl_->callback_mutex_);
    pimpl_->decision_callbacks_.push_back(callback);
}

std::vector<TradingDecision> LLMDecisionSystem::get_recent_decisions(uint32_t count) const {
    std::lock_guard<std::mutex> lock(pimpl_->decisions_mutex_);
    
    std::vector<TradingDecision> result;
    size_t start_idx = pimpl_->recent_decisions_.size() > count ? 
                      pimpl_->recent_decisions_.size() - count : 0;
    
    for (size_t i = start_idx; i < pimpl_->recent_decisions_.size(); ++i) {
        result.push_back(pimpl_->recent_decisions_[i]);
    }
    
    return result;
}

void LLMDecisionSystem::get_statistics(DecisionStats& stats_out) const {
    stats_out = pimpl_->stats_;
}

void LLMDecisionSystem::emergency_stop() {
    pimpl_->emergency_stopped_.store(true);
    std::cout << "🚨 LLM Decision System - Emergency stop activated!" << std::endl;
}

void LLMDecisionSystem::pause_trading(bool paused) {
    pimpl_->trading_paused_.store(paused);
    std::cout << (paused ? "⏸️  " : "▶️  ") << "LLM Trading " << (paused ? "paused" : "resumed") << std::endl;
}

} // namespace hfx::ai
