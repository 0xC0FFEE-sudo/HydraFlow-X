/**
 * @file trading_strategy.cpp
 * @brief Trading Strategy Implementations
 */

#include "../include/trading_strategy.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>

namespace hfx {
namespace backtest {

// Utility functions
std::string signal_type_to_string(SignalType type) {
    switch (type) {
        case SignalType::BUY: return "buy";
        case SignalType::SELL: return "sell";
        case SignalType::HOLD: return "hold";
        case SignalType::CLOSE_POSITION: return "close_position";
        default: return "unknown";
    }
}

SignalType string_to_signal_type(const std::string& type_str) {
    if (type_str == "buy") return SignalType::BUY;
    if (type_str == "sell") return SignalType::SELL;
    if (type_str == "hold") return SignalType::HOLD;
    if (type_str == "close_position") return SignalType::CLOSE_POSITION;
    return SignalType::HOLD;
}

// Moving Average Crossover Strategy Implementation
MovingAverageCrossoverStrategy::MovingAverageCrossoverStrategy()
    : has_position_(false) {
    params_.name = "Moving Average Crossover";
}

void MovingAverageCrossoverStrategy::initialize(const StrategyParameters& params) {
    params_ = params;
}

std::vector<TradingSignal> MovingAverageCrossoverStrategy::process_data(
    const MarketData& data, const PortfolioSnapshot& portfolio) {

    std::vector<TradingSignal> signals;

    // Add current price to price history
    prices_.push_back(data.close);

    // Maintain price history size
    int max_history = std::max(params_.get_numeric_param("fast_period", 10),
                              params_.get_numeric_param("slow_period", 30)) * 2;
    if (prices_.size() > max_history) {
        prices_.erase(prices_.begin());
    }

    // Update current symbol
    current_symbol_ = data.symbol;

    // Check for crossover signals
    auto crossover_signals = check_crossover(data);
    signals.insert(signals.end(), crossover_signals.begin(), crossover_signals.end());

    return signals;
}

void MovingAverageCrossoverStrategy::on_order_fill(const Order& order) {
    has_position_ = (order.side == OrderSide::BUY);
}

void MovingAverageCrossoverStrategy::on_position_update(const Position& position) {
    has_position_ = (position.quantity != 0.0);
}

std::string MovingAverageCrossoverStrategy::get_name() const {
    return "Moving Average Crossover";
}

std::string MovingAverageCrossoverStrategy::get_description() const {
    return "Generates buy/sell signals based on fast and slow moving average crossovers";
}

StrategyParameters MovingAverageCrossoverStrategy::get_parameters() const {
    return params_;
}

void MovingAverageCrossoverStrategy::reset() {
    prices_.clear();
    fast_ma_.clear();
    slow_ma_.clear();
    has_position_ = false;
    current_symbol_.clear();
}

double MovingAverageCrossoverStrategy::calculate_sma(const std::vector<double>& prices, int period) {
    if (prices.size() < period) return 0.0;

    double sum = 0.0;
    for (int i = prices.size() - period; i < prices.size(); ++i) {
        sum += prices[i];
    }

    return sum / period;
}

std::vector<TradingSignal> MovingAverageCrossoverStrategy::check_crossover(const MarketData& data) {
    std::vector<TradingSignal> signals;

    int fast_period = static_cast<int>(params_.get_numeric_param("fast_period", 10));
    int slow_period = static_cast<int>(params_.get_numeric_param("slow_period", 30));

    if (prices_.size() < slow_period) return signals;

    // Calculate moving averages
    double fast_ma = calculate_sma(prices_, fast_period);
    double slow_ma = calculate_sma(prices_, slow_period);

    // Store for trend analysis
    fast_ma_.push_back(fast_ma);
    slow_ma_.push_back(slow_ma);

    if (fast_ma_.size() < 2 || slow_ma_.size() < 2) return signals;

    // Check for crossover
    double prev_fast = fast_ma_[fast_ma_.size() - 2];
    double prev_slow = slow_ma_[slow_ma_.size() - 2];
    double curr_fast = fast_ma_[fast_ma_.size() - 1];
    double curr_slow = slow_ma_[slow_ma_.size() - 1];

    // Bullish crossover (fast MA crosses above slow MA)
    if (prev_fast <= prev_slow && curr_fast > curr_slow && !has_position_) {
        TradingSignal signal;
        signal.type = SignalType::BUY;
        signal.symbol = data.symbol;
        signal.quantity = params_.get_numeric_param("position_size", 1.0);
        signal.price = data.close;
        signal.reason = "Fast MA crossed above slow MA";
        signal.timestamp = data.timestamp;
        signals.push_back(signal);
    }

    // Bearish crossover (fast MA crosses below slow MA)
    if (prev_fast >= prev_slow && curr_fast < curr_slow && has_position_) {
        TradingSignal signal;
        signal.type = SignalType::SELL;
        signal.symbol = data.symbol;
        signal.quantity = params_.get_numeric_param("position_size", 1.0);
        signal.price = data.close;
        signal.reason = "Fast MA crossed below slow MA";
        signal.timestamp = data.timestamp;
        signals.push_back(signal);
    }

    return signals;
}

// RSI Divergence Strategy Implementation
RSIDivergenceStrategy::RSIDivergenceStrategy() : has_position_(false) {
    params_.name = "RSI Divergence";
}

void RSIDivergenceStrategy::initialize(const StrategyParameters& params) {
    params_ = params;
}

std::vector<TradingSignal> RSIDivergenceStrategy::process_data(
    const MarketData& data, const PortfolioSnapshot& portfolio) {

    std::vector<TradingSignal> signals;

    prices_.push_back(data.close);

    int rsi_period = static_cast<int>(params_.get_numeric_param("rsi_period", 14));
    int max_history = rsi_period * 3;

    if (prices_.size() > max_history) {
        prices_.erase(prices_.begin());
    }

    current_symbol_ = data.symbol;

    if (prices_.size() >= rsi_period) {
        double rsi = calculate_rsi(rsi_period);

        double overbought = params_.get_numeric_param("overbought_level", 70.0);
        double oversold = params_.get_numeric_param("oversold_level", 30.0);

        // Simple RSI strategy
        if (rsi <= oversold && !has_position_) {
            TradingSignal signal;
            signal.type = SignalType::BUY;
            signal.symbol = data.symbol;
            signal.quantity = params_.get_numeric_param("position_size", 1.0);
            signal.price = data.close;
            signal.reason = "RSI oversold signal";
            signal.timestamp = data.timestamp;
            signals.push_back(signal);
        } else if (rsi >= overbought && has_position_) {
            TradingSignal signal;
            signal.type = SignalType::SELL;
            signal.symbol = data.symbol;
            signal.quantity = params_.get_numeric_param("position_size", 1.0);
            signal.price = data.close;
            signal.reason = "RSI overbought signal";
            signal.timestamp = data.timestamp;
            signals.push_back(signal);
        }
    }

    return signals;
}

void RSIDivergenceStrategy::on_order_fill(const Order& order) {
    has_position_ = (order.side == OrderSide::BUY);
}

void RSIDivergenceStrategy::on_position_update(const Position& position) {
    has_position_ = (position.quantity != 0.0);
}

std::string RSIDivergenceStrategy::get_name() const {
    return "RSI Divergence Strategy";
}

std::string RSIDivergenceStrategy::get_description() const {
    return "Generates signals based on RSI overbought/oversold levels";
}

StrategyParameters RSIDivergenceStrategy::get_parameters() const {
    return params_;
}

void RSIDivergenceStrategy::reset() {
    prices_.clear();
    rsi_values_.clear();
    has_position_ = false;
    current_symbol_.clear();
}

double RSIDivergenceStrategy::calculate_rsi(int period) {
    if (prices_.size() < period + 1) return 50.0;

    std::vector<double> gains, losses;

    for (size_t i = 1; i < prices_.size(); ++i) {
        double change = prices_[i] - prices_[i - 1];
        if (change > 0) {
            gains.push_back(change);
            losses.push_back(0);
        } else {
            gains.push_back(0);
            losses.push_back(-change);
        }
    }

    // Calculate average gains and losses
    double avg_gain = 0, avg_loss = 0;
    for (size_t i = gains.size() - period; i < gains.size(); ++i) {
        avg_gain += gains[i];
        avg_loss += losses[i];
    }

    avg_gain /= period;
    avg_loss /= period;

    if (avg_loss == 0) return 100.0;

    double rs = avg_gain / avg_loss;
    double rsi = 100.0 - (100.0 / (1.0 + rs));

    rsi_values_.push_back(rsi);
    return rsi;
}

double RSIDivergenceStrategy::calculate_price_change(const std::vector<double>& prices) {
    if (prices.size() < 2) return 0.0;
    return prices.back() - prices.front();
}

bool RSIDivergenceStrategy::detect_divergence(const std::vector<double>& prices,
                                            const std::vector<double>& rsi) {
    // Simplified divergence detection
    if (prices.size() < 4 || rsi.size() < 4) return false;

    // Check for bearish divergence (price up, RSI down)
    bool price_up = prices.back() > prices[prices.size() - 4];
    bool rsi_down = rsi.back() < rsi[rsi.size() - 4];

    // Check for bullish divergence (price down, RSI up)
    bool price_down = prices.back() < prices[prices.size() - 4];
    bool rsi_up = rsi.back() > rsi[rsi.size() - 4];

    return (price_up && rsi_down) || (price_down && rsi_up);
}

// Strategy Factory Implementation
std::unique_ptr<ITradingStrategy> StrategyFactory::create_strategy(
    const std::string& strategy_type, const StrategyParameters& params) {

    if (strategy_type == "moving_average_crossover") {
        auto strategy = std::make_unique<MovingAverageCrossoverStrategy>();
        strategy->initialize(params);
        return strategy;
    } else if (strategy_type == "rsi_divergence") {
        auto strategy = std::make_unique<RSIDivergenceStrategy>();
        strategy->initialize(params);
        return strategy;
    }

    return nullptr;
}

std::vector<std::string> StrategyFactory::get_available_strategies() {
    return {
        "moving_average_crossover",
        "rsi_divergence",
        "mean_reversion",
        "momentum",
        "bollinger_bands",
        "macd",
        "stochastic_oscillator"
    };
}

std::unique_ptr<ITradingStrategy> StrategyFactory::create_moving_average_strategy(
    int fast_period, int slow_period) {

    StrategyParameters params;
    params.name = "Moving Average Crossover";
    params.numeric_params["fast_period"] = fast_period;
    params.numeric_params["slow_period"] = slow_period;
    params.numeric_params["position_size"] = 1.0;

    return create_strategy("moving_average_crossover", params);
}

std::unique_ptr<ITradingStrategy> StrategyFactory::create_rsi_strategy(
    int rsi_period, double overbought_level, double oversold_level) {

    StrategyParameters params;
    params.name = "RSI Strategy";
    params.numeric_params["rsi_period"] = rsi_period;
    params.numeric_params["overbought_level"] = overbought_level;
    params.numeric_params["oversold_level"] = oversold_level;
    params.numeric_params["position_size"] = 1.0;

    return create_strategy("rsi_divergence", params);
}

} // namespace backtest
} // namespace hfx
