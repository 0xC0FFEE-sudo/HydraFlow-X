/**
 * @file llm_decision_system.cpp
 * @brief Implementation of LLM-powered trading decision system
 */

#include "llm_decision_system.hpp"
#include "../../hfx-log/include/simple_logger.hpp"
#include <algorithm>
#include <numeric>
#include <random>
#include <sstream>
#include <iomanip>

namespace hfx::ai {

// Advanced decision system implementation
class LLMDecisionSystem::DecisionImpl {
public:
    DecisionCallback callback_;
    std::atomic<bool> running_{false};
    DecisionStats stats_;
    DecisionConfig config_;
    
    // Decision models and weights
    std::unordered_map<std::string, double> sentiment_weights_;
    std::unordered_map<std::string, double> technical_weights_;
    std::unordered_map<std::string, TradingDecision> recent_decisions_;
    std::mutex decision_mutex_;
    
    // Performance tracking
    std::vector<TradingDecision> decision_history_;
    std::unordered_map<std::string, double> symbol_performance_;
    
    DecisionImpl() {
        initialize_default_weights();
        stats_.start_time = std::chrono::system_clock::now();
    }
    
    void initialize_default_weights() {
        // Sentiment source weights
        sentiment_weights_["twitter"] = 0.4;
        sentiment_weights_["reddit"] = 0.3;
        sentiment_weights_["news"] = 0.2;
        sentiment_weights_["telegram"] = 0.1;
        
        // Technical indicator weights  
        technical_weights_["momentum"] = 0.3;
        technical_weights_["volume"] = 0.25;
        technical_weights_["volatility"] = 0.2;
        technical_weights_["liquidity"] = 0.15;
        technical_weights_["support_resistance"] = 0.1;
    }
    
    TradingDecision process_sentiment_advanced(const SentimentSignal& signal) {
        std::lock_guard<std::mutex> lock(decision_mutex_);
        
        TradingDecision decision;
        decision.symbol = signal.symbol;
        decision.timestamp_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch()).count();
        
        // Multi-factor analysis
        double sentiment_score = calculate_weighted_sentiment_score(signal);
        double technical_score = calculate_technical_score(signal);
        double risk_score = calculate_risk_score(signal);
        double momentum_score = calculate_momentum_score(signal);
        
        // Combine all factors
        double combined_score = (sentiment_score * 0.4) + 
                               (technical_score * 0.3) + 
                               (momentum_score * 0.2) + 
                               (1.0 - risk_score) * 0.1;
        
        // Make decision based on combined score
        decision = make_trading_decision(signal, combined_score, sentiment_score, risk_score);
        
        // Update statistics and history
        update_decision_metrics(decision);
        
        return decision;
    }
    
    double calculate_weighted_sentiment_score(const SentimentSignal& signal) {
        double weighted_score = 0.0;
        double total_weight = 0.0;
        
        // Weight sentiment by source reliability
        for (const auto& source_score : signal.contributing_scores) {
            std::string source = extract_source_type(source_score.first);
            auto weight_it = sentiment_weights_.find(source);
            double weight = (weight_it != sentiment_weights_.end()) ? weight_it->second : 0.1;
            
            weighted_score += source_score.second * weight;
            total_weight += weight;
        }
        
        if (total_weight > 0) {
            weighted_score /= total_weight;
        }
        
        // Apply confidence adjustment
        weighted_score *= signal.confidence;
        
        // Apply time decay for older signals
        auto now = std::chrono::high_resolution_clock::now();
        auto signal_age = std::chrono::duration_cast<std::chrono::minutes>(
            now.time_since_epoch()).count() - signal.timestamp_ns / 1000000000 / 60;
        
        double decay_factor = std::exp(-signal_age / 30.0); // 30 minute half-life
        weighted_score *= decay_factor;
        
        return std::clamp(weighted_score, -1.0, 1.0);
    }
    
    double calculate_technical_score(const SentimentSignal& signal) {
        // Simulate technical analysis (would integrate with real TA system)
        double momentum = signal.momentum;
        double volume = signal.volume_change * 0.1; // Normalize volume impact
        double volatility = std::clamp(signal.divergence, 0.0, 1.0);
        
        // Combine technical factors
        double tech_score = (momentum * technical_weights_["momentum"]) +
                           (volume * technical_weights_["volume"]) +
                           ((1.0 - volatility) * technical_weights_["volatility"]);
        
        return std::clamp(tech_score, -1.0, 1.0);
    }
    
    double calculate_risk_score(const SentimentSignal& signal) {
        double risk = 0.0;
        
        // Volatility risk
        risk += std::clamp(signal.divergence * 0.5, 0.0, 0.3);
        
        // Confidence risk (lower confidence = higher risk)
        risk += (1.0 - signal.confidence) * 0.3;
        
        // Volume risk (abnormal volume)
        if (std::abs(signal.volume_change) > 2.0) {
            risk += 0.2;
        }
        
        // Sentiment extremes risk (potential reversal)
        if (std::abs(signal.weighted_sentiment) > 0.8) {
            risk += 0.2;
        }
        
        return std::clamp(risk, 0.0, 1.0);
    }
    
    double calculate_momentum_score(const SentimentSignal& signal) {
        // Momentum alignment between sentiment and price movement
        double momentum_alignment = signal.momentum * signal.weighted_sentiment;
        
        // Sustained momentum bonus
        if (std::abs(momentum_alignment) > 0.3 && signal.momentum > 0.2) {
            momentum_alignment *= 1.2;
        }
        
        return std::clamp(momentum_alignment, -1.0, 1.0);
    }
    
    TradingDecision make_trading_decision(const SentimentSignal& signal, 
                                        double combined_score,
                                        double sentiment_score, 
                                        double risk_score) {
        TradingDecision decision;
        decision.symbol = signal.symbol;
        decision.timestamp_ns = signal.timestamp_ns;
        decision.confidence = std::max(0.0, 1.0 - risk_score);
        decision.risk_score = risk_score;
        
        // Decision thresholds
        double strong_threshold = config_.strong_signal_threshold;
        double weak_threshold = config_.weak_signal_threshold;
        
        // Determine action
        if (combined_score >= strong_threshold) {
            decision.action = DecisionType::BUY_SPOT;
            decision.position_size_pct = calculate_position_size(combined_score, risk_score, 0.05, 0.15);
        } else if (combined_score >= weak_threshold) {
            decision.action = DecisionType::BUY_SPOT;
            decision.position_size_pct = calculate_position_size(combined_score, risk_score, 0.02, 0.08);
        } else if (combined_score <= -strong_threshold) {
            decision.action = DecisionType::SELL_SPOT;
            decision.position_size_pct = calculate_position_size(-combined_score, risk_score, 0.05, 0.15);
        } else if (combined_score <= -weak_threshold) {
            decision.action = DecisionType::SELL_SPOT;
            decision.position_size_pct = calculate_position_size(-combined_score, risk_score, 0.02, 0.08);
        } else {
            decision.action = DecisionType::HOLD;
            decision.position_size_pct = 0.0;
        }
        
        // Expected return estimation
        decision.expected_return = estimate_expected_return(combined_score, sentiment_score, signal.momentum);
        
        // Generate reasoning
        decision.reasoning = generate_decision_reasoning(signal, combined_score, sentiment_score, risk_score);
        
        return decision;
    }
    
    double calculate_position_size(double signal_strength, double risk_score, 
                                 double min_size, double max_size) {
        // Kelly criterion inspired position sizing
        double base_size = min_size + (signal_strength * (max_size - min_size));
        
        // Risk adjustment
        double risk_adjustment = 1.0 - (risk_score * 0.5);
        
        return std::clamp(base_size * risk_adjustment, min_size, max_size);
    }
    
    double estimate_expected_return(double combined_score, double sentiment_score, double momentum) {
        // Base expected return from signal strength
        double base_return = std::abs(combined_score) * config_.max_expected_return;
        
        // Momentum boost
        if (momentum > 0.2) {
            base_return *= (1.0 + momentum * 0.2);
        }
        
        // Sentiment clarity boost
        if (std::abs(sentiment_score) > 0.6) {
            base_return *= 1.1;
        }
        
        return std::copysign(base_return, combined_score);
    }
    
    std::string generate_decision_reasoning(const SentimentSignal& signal,
                                          double combined_score,
                                          double sentiment_score,
                                          double risk_score) {
        std::stringstream reasoning;
        
        reasoning << "Decision based on multi-factor analysis: ";
        
        // Sentiment analysis
        if (std::abs(sentiment_score) > 0.3) {
            reasoning << "Strong " << (sentiment_score > 0 ? "positive" : "negative") 
                     << " sentiment (" << std::fixed << std::setprecision(2) << sentiment_score << ") ";
            reasoning << "from " << signal.contributing_scores.size() << " sources. ";
        }
        
        // Momentum
        if (signal.momentum > 0.2) {
            reasoning << "Strong upward momentum (" << signal.momentum << "). ";
        } else if (signal.momentum < -0.2) {
            reasoning << "Strong downward momentum (" << signal.momentum << "). ";
        }
        
        // Volume
        if (signal.volume_change > 1.5) {
            reasoning << "High volume spike (+" << (signal.volume_change - 1.0) * 100 << "%). ";
        }
        
        // Risk assessment
        if (risk_score > 0.6) {
            reasoning << "High risk environment - reduced position size. ";
        } else if (risk_score < 0.3) {
            reasoning << "Low risk environment - increased confidence. ";
        }
        
        // Confidence
        reasoning << "Overall confidence: " << std::fixed << std::setprecision(1) 
                 << (1.0 - risk_score) * 100 << "%.";
        
        return reasoning.str();
    }
    
    std::string extract_source_type(const std::string& source_id) {
        if (source_id.find("twitter") != std::string::npos) return "twitter";
        if (source_id.find("reddit") != std::string::npos) return "reddit";
        if (source_id.find("news") != std::string::npos) return "news";
        if (source_id.find("telegram") != std::string::npos) return "telegram";
        return "unknown";
    }
    
    void update_decision_metrics(const TradingDecision& decision) {
        stats_.total_decisions.fetch_add(1);
        stats_.last_decision_time = std::chrono::system_clock::now();
        
        if (decision.action != DecisionType::HOLD) {
            if (decision.action == DecisionType::BUY_SPOT || decision.action == DecisionType::BUY_PERP) {
                stats_.buy_decisions.fetch_add(1);
            } else {
                stats_.sell_decisions.fetch_add(1);
            }
        }
        
        // Track by symbol
        symbol_performance_[decision.symbol] = decision.expected_return;
        
        // Keep limited history
        decision_history_.push_back(decision);
        if (decision_history_.size() > 1000) {
            decision_history_.erase(decision_history_.begin());
        }
        
        // Store recent decision for duplicate checking
        recent_decisions_[decision.symbol] = decision;
    }
    
    // Simple process_sentiment for backwards compatibility
    void process_sentiment(const SentimentSignal& signal) {
        if (!running_.load()) return;
        
        TradingDecision decision = process_sentiment_advanced(signal);
        
        if (callback_ && decision.action != DecisionType::HOLD) {
            callback_(decision);
        }
    }
};

LLMDecisionSystem::LLMDecisionSystem() : pimpl_(std::make_unique<DecisionImpl>()) {}

LLMDecisionSystem::~LLMDecisionSystem() = default;

bool LLMDecisionSystem::initialize(const DecisionConfig& config) {
    HFX_LOG_INFO("[LLMDecisionSystem] Initializing advanced AI decision engine...");
    
    pimpl_->config_ = config;
    pimpl_->running_.store(true);
    
    // Initialize decision models and calibration
    initialize_decision_models();
    
    HFX_LOG_INFO("[LLMDecisionSystem] ✅ Advanced decision system ready with " + 
                 std::to_string(pimpl_->sentiment_weights_.size()) + " sentiment sources");
    
    return true;
}

void LLMDecisionSystem::shutdown() {
    HFX_LOG_INFO("[LLMDecisionSystem] Shutting down decision system...");
    pimpl_->running_.store(false);
    
    // Save performance metrics
    auto total_decisions = pimpl_->stats_.total_decisions.load();
    if (total_decisions > 0) {
        HFX_LOG_INFO("[LLMDecisionSystem] Processed " + std::to_string(total_decisions) + " decisions");
    }
}

void LLMDecisionSystem::initialize_decision_models() {
    HFX_LOG_INFO("[LLMDecisionSystem] Initializing decision models and weights");
    
    // Load or initialize model weights (simplified)
    // In production, this would load trained ML models
    pimpl_->initialize_default_weights();
}

TradingDecision LLMDecisionSystem::process_comprehensive_signal(const SentimentSignal& signal) {
    if (!pimpl_->running_.load()) {
        TradingDecision empty_decision;
        empty_decision.action = DecisionType::HOLD;
        return empty_decision;
    }
    
    return pimpl_->process_sentiment_advanced(signal);
}

void LLMDecisionSystem::update_model_weights(const std::unordered_map<std::string, double>& weights) {
    std::lock_guard<std::mutex> lock(pimpl_->decision_mutex_);
    
    for (const auto& weight : weights) {
        if (pimpl_->sentiment_weights_.count(weight.first)) {
            pimpl_->sentiment_weights_[weight.first] = weight.second;
        }
        if (pimpl_->technical_weights_.count(weight.first)) {
            pimpl_->technical_weights_[weight.first] = weight.second;
        }
    }
    
    HFX_LOG_INFO("[LLMDecisionSystem] Updated " + std::to_string(weights.size()) + " model weights");
}

std::vector<TradingDecision> LLMDecisionSystem::get_recent_decisions(size_t count) const {
    std::lock_guard<std::mutex> lock(pimpl_->decision_mutex_);
    
    std::vector<TradingDecision> recent;
    size_t start_idx = pimpl_->decision_history_.size() > count ? 
                       pimpl_->decision_history_.size() - count : 0;
    
    for (size_t i = start_idx; i < pimpl_->decision_history_.size(); ++i) {
        recent.push_back(pimpl_->decision_history_[i]);
    }
    
    return recent;
}

double LLMDecisionSystem::get_symbol_performance(const std::string& symbol) const {
    std::lock_guard<std::mutex> lock(pimpl_->decision_mutex_);
    
    auto it = pimpl_->symbol_performance_.find(symbol);
    return (it != pimpl_->symbol_performance_.end()) ? it->second : 0.0;
}

void LLMDecisionSystem::update_config(const DecisionConfig& config) {
    std::lock_guard<std::mutex> lock(pimpl_->decision_mutex_);
    pimpl_->config_ = config;
    HFX_LOG_INFO("[LLMDecisionSystem] Configuration updated");
}

void LLMDecisionSystem::register_decision_callback(DecisionCallback callback) {
    pimpl_->callback_ = callback;
}

void LLMDecisionSystem::process_sentiment_signal(const SentimentSignal& signal) {
    pimpl_->process_sentiment(signal);
}

void LLMDecisionSystem::get_statistics(DecisionStats& stats_out) const {
    stats_out.total_decisions.store(pimpl_->stats_.total_decisions.load());
    stats_out.profitable_decisions.store(pimpl_->stats_.profitable_decisions.load());
    stats_out.avg_decision_latency_ns.store(pimpl_->stats_.avg_decision_latency_ns.load());
    stats_out.llm_inference_latency_ns.store(pimpl_->stats_.llm_inference_latency_ns.load());
    stats_out.total_pnl_usd.store(pimpl_->stats_.total_pnl_usd.load());
    stats_out.win_rate.store(pimpl_->stats_.win_rate.load());
    stats_out.sharpe_ratio.store(pimpl_->stats_.sharpe_ratio.load());
    stats_out.max_drawdown.store(pimpl_->stats_.max_drawdown.load());
    stats_out.active_positions.store(pimpl_->stats_.active_positions.load());
    stats_out.emergency_exits.store(pimpl_->stats_.emergency_exits.load());
}

// ResearchAgent stub implementation
class ResearchAgent::ResearchImpl {
public:
    std::atomic<bool> running_{false};
};

ResearchAgent::ResearchAgent() : pimpl_(std::make_unique<ResearchImpl>()) {}
ResearchAgent::~ResearchAgent() = default;

bool ResearchAgent::initialize() {
    HFX_LOG_INFO("[ResearchAgent] Research agent initialized (demo mode)");
    return true;
}

void ResearchAgent::start_continuous_research() {
    pimpl_->running_.store(true);
}

void ResearchAgent::stop_research() {
    pimpl_->running_.store(false);
}

// JarvisInterface stub implementation  
class JarvisInterface::JarvisImpl {
public:
    std::atomic<bool> running_{false};
};

JarvisInterface::JarvisInterface() : pimpl_(std::make_unique<JarvisImpl>()) {}
JarvisInterface::~JarvisInterface() = default;

bool JarvisInterface::initialize() {
    HFX_LOG_INFO("[JarvisInterface] Conversational AI interface initialized");
    return true;
}

JarvisInterface::JarvisResponse JarvisInterface::process_user_input(const std::string& input, const std::string& user_id) {
    JarvisResponse response;
    response.response_text = "Hello! I'm HydraFlow-X AI. Demo mode active.";
    response.system_status = "All systems operational";
    response.requires_confirmation = false;
    return response;
}

// AIStrategyGenerator stub implementation
class AIStrategyGenerator::GeneratorImpl {
public:
    std::atomic<bool> running_{false};
};

AIStrategyGenerator::AIStrategyGenerator() : pimpl_(std::make_unique<GeneratorImpl>()) {}
AIStrategyGenerator::~AIStrategyGenerator() = default;

bool AIStrategyGenerator::initialize() {
    HFX_LOG_INFO("[AIStrategyGenerator] AI strategy generator initialized");
    return true;
}

AIStrategyGenerator::GeneratedStrategy AIStrategyGenerator::generate_strategy_from_prompt(const std::string& prompt) {
    GeneratedStrategy strategy;
    strategy.name = "Demo Strategy";
    strategy.description = "AI-generated demonstration strategy";
    strategy.backtested_sharpe = 1.5;
    strategy.created_timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    return strategy;
}

} // namespace hfx::ai
