#pragma once

#include "backtest_data.hpp"
#include <functional>
#include <memory>

namespace hfx {
namespace backtest {

// Forward declarations
class BacktestEngine;

// Strategy signal types
enum class SignalType {
    BUY = 0,
    SELL,
    HOLD,
    CLOSE_POSITION
};

// Trading signal
struct TradingSignal {
    SignalType type;
    std::string symbol;
    double quantity; // For BUY/SELL signals
    double price; // Target price for limit orders
    std::string reason; // Reason for the signal
    std::chrono::system_clock::time_point timestamp;
    std::unordered_map<std::string, double> metadata; // Additional strategy data

    TradingSignal() : quantity(0.0), price(0.0) {}
};

// Strategy parameters
struct StrategyParameters {
    std::string name;
    std::unordered_map<std::string, double> numeric_params;
    std::unordered_map<std::string, std::string> string_params;
    std::unordered_map<std::string, bool> bool_params;

    // Helper methods
    double get_numeric_param(const std::string& key, double default_value = 0.0) const {
        auto it = numeric_params.find(key);
        return it != numeric_params.end() ? it->second : default_value;
    }

    std::string get_string_param(const std::string& key, const std::string& default_value = "") const {
        auto it = string_params.find(key);
        return it != string_params.end() ? it->second : default_value;
    }

    bool get_bool_param(const std::string& key, bool default_value = false) const {
        auto it = bool_params.find(key);
        return it != bool_params.end() ? it->second : default_value;
    }
};

// Trading strategy interface
class ITradingStrategy {
public:
    virtual ~ITradingStrategy() = default;

    // Initialize strategy with parameters
    virtual void initialize(const StrategyParameters& params) = 0;

    // Process market data and generate signals
    virtual std::vector<TradingSignal> process_data(const MarketData& data,
                                                   const PortfolioSnapshot& portfolio) = 0;

    // Handle order fills
    virtual void on_order_fill(const Order& order) = 0;

    // Handle position updates
    virtual void on_position_update(const Position& position) = 0;

    // Get strategy name
    virtual std::string get_name() const = 0;

    // Get strategy description
    virtual std::string get_description() const = 0;

    // Get current parameters
    virtual StrategyParameters get_parameters() const = 0;

    // Reset strategy state
    virtual void reset() = 0;
};

// Moving Average Crossover Strategy
class MovingAverageCrossoverStrategy : public ITradingStrategy {
public:
    MovingAverageCrossoverStrategy();
    ~MovingAverageCrossoverStrategy() override = default;

    void initialize(const StrategyParameters& params) override;
    std::vector<TradingSignal> process_data(const MarketData& data,
                                          const PortfolioSnapshot& portfolio) override;
    void on_order_fill(const Order& order) override;
    void on_position_update(const Position& position) override;
    std::string get_name() const override;
    std::string get_description() const override;
    StrategyParameters get_parameters() const override;
    void reset() override;

private:
    StrategyParameters params_;
    std::vector<double> prices_;
    std::vector<double> fast_ma_;
    std::vector<double> slow_ma_;
    bool has_position_;
    std::string current_symbol_;

    double calculate_sma(const std::vector<double>& prices, int period);
    std::vector<TradingSignal> check_crossover(const MarketData& data);
};

// RSI Divergence Strategy
class RSIDivergenceStrategy : public ITradingStrategy {
public:
    RSIDivergenceStrategy();
    ~RSIDivergenceStrategy() override = default;

    void initialize(const StrategyParameters& params) override;
    std::vector<TradingSignal> process_data(const MarketData& data,
                                          const PortfolioSnapshot& portfolio) override;
    void on_order_fill(const Order& order) override;
    void on_position_update(const Position& position) override;
    std::string get_name() const override;
    std::string get_description() const override;
    StrategyParameters get_parameters() const override;
    void reset() override;

private:
    StrategyParameters params_;
    std::vector<double> prices_;
    std::vector<double> rsi_values_;
    bool has_position_;
    std::string current_symbol_;

    double calculate_rsi(int period);
    double calculate_price_change(const std::vector<double>& prices);
    bool detect_divergence(const std::vector<double>& prices, const std::vector<double>& rsi);
};

// Mean Reversion Strategy
class MeanReversionStrategy : public ITradingStrategy {
public:
    MeanReversionStrategy();
    ~MeanReversionStrategy() override = default;

    void initialize(const StrategyParameters& params) override;
    std::vector<TradingSignal> process_data(const MarketData& data,
                                          const PortfolioSnapshot& portfolio) override;
    void on_order_fill(const Order& order) override;
    void on_position_update(const Position& position) override;
    std::string get_name() const override;
    std::string get_description() const override;
    StrategyParameters get_parameters() const override;
    void reset() override;

private:
    StrategyParameters params_;
    std::vector<double> prices_;
    bool has_position_;
    std::string current_symbol_;

    double calculate_mean(const std::vector<double>& prices);
    double calculate_std_dev(const std::vector<double>& prices, double mean);
    double calculate_z_score(double price, double mean, double std_dev);
};

// Momentum Strategy
class MomentumStrategy : public ITradingStrategy {
public:
    MomentumStrategy();
    ~MomentumStrategy() override = default;

    void initialize(const StrategyParameters& params) override;
    std::vector<TradingSignal> process_data(const MarketData& data,
                                          const PortfolioSnapshot& portfolio) override;
    void on_order_fill(const Order& order) override;
    void on_position_update(const Position& position) override;
    std::string get_name() const override;
    std::string get_description() const override;
    StrategyParameters get_parameters() const override;
    void reset() override;

private:
    StrategyParameters params_;
    std::vector<double> prices_;
    bool has_position_;
    std::string current_symbol_;

    double calculate_momentum(int period);
    double calculate_rate_of_change(int period);
};

// Custom Strategy Builder (allows creating strategies programmatically)
class CustomStrategy : public ITradingStrategy {
public:
    using SignalGenerator = std::function<std::vector<TradingSignal>(
        const MarketData& data, const PortfolioSnapshot& portfolio)>;

    CustomStrategy(const std::string& name, const std::string& description,
                  SignalGenerator signal_generator);
    ~CustomStrategy() override = default;

    void initialize(const StrategyParameters& params) override;
    std::vector<TradingSignal> process_data(const MarketData& data,
                                          const PortfolioSnapshot& portfolio) override;
    void on_order_fill(const Order& order) override;
    void on_position_update(const Position& position) override;
    std::string get_name() const override;
    std::string get_description() const override;
    StrategyParameters get_parameters() const override;
    void reset() override;

private:
    std::string name_;
    std::string description_;
    StrategyParameters params_;
    SignalGenerator signal_generator_;
    bool has_position_;
};

// Strategy factory
class StrategyFactory {
public:
    static std::unique_ptr<ITradingStrategy> create_strategy(const std::string& strategy_type,
                                                           const StrategyParameters& params);

    static std::vector<std::string> get_available_strategies();

    static std::unique_ptr<ITradingStrategy> create_moving_average_strategy(
        int fast_period = 10, int slow_period = 30);

    static std::unique_ptr<ITradingStrategy> create_rsi_strategy(
        int rsi_period = 14, double overbought_level = 70.0, double oversold_level = 30.0);

    static std::unique_ptr<ITradingStrategy> create_mean_reversion_strategy(
        int lookback_period = 20, double entry_threshold = 2.0, double exit_threshold = 0.5);

    static std::unique_ptr<ITradingStrategy> create_momentum_strategy(
        int momentum_period = 10, double entry_threshold = 0.05, double exit_threshold = -0.02);
};

// Utility functions
std::string signal_type_to_string(SignalType type);
SignalType string_to_signal_type(const std::string& type_str);

} // namespace backtest
} // namespace hfx
