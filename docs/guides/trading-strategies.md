# Trading Strategies Guide

This guide covers the built-in trading strategies in HydraFlow-X and how to develop custom strategies.

## Built-in Strategies

### 1. Moving Average Crossover

**Description**: Generates buy/sell signals based on fast and slow moving average crossovers.

**Parameters**:
- `fast_period`: Short-term moving average period (default: 10)
- `slow_period`: Long-term moving average period (default: 30)
- `position_size`: Position size per trade (default: 1.0)

**Signal Logic**:
- **Buy Signal**: Fast MA crosses above slow MA
- **Sell Signal**: Fast MA crosses below slow MA

**Example**:
```cpp
auto strategy = StrategyFactory::create_moving_average_strategy(10, 30);
```

**Pros**:
- Simple and effective
- Works well in trending markets
- Clear entry/exit signals

**Cons**:
- Lagging indicators
- Poor performance in sideways markets
- May generate false signals

### 2. RSI Divergence

**Description**: Uses RSI (Relative Strength Index) to identify overbought/oversold conditions.

**Parameters**:
- `rsi_period`: RSI calculation period (default: 14)
- `overbought_level`: Overbought threshold (default: 70)
- `oversold_level`: Oversold threshold (default: 30)
- `position_size`: Position size per trade (default: 1.0)

**Signal Logic**:
- **Buy Signal**: RSI < oversold_level
- **Sell Signal**: RSI > overbought_level

**Example**:
```cpp
auto strategy = StrategyFactory::create_rsi_strategy(14, 70, 30);
```

### 3. Mean Reversion

**Description**: Trades based on the assumption that prices will revert to their mean.

**Parameters**:
- `lookback_period`: Period for mean calculation (default: 20)
- `entry_threshold`: Z-score threshold for entry (default: 2.0)
- `exit_threshold`: Z-score threshold for exit (default: 0.5)
- `position_size`: Position size per trade (default: 1.0)

**Signal Logic**:
- **Buy Signal**: Price < mean - (entry_threshold × std_dev)
- **Sell Signal**: Price > mean + (entry_threshold × std_dev)
- **Exit**: When price returns to mean ± (exit_threshold × std_dev)

### 4. Momentum Strategy

**Description**: Trades based on price momentum and rate of change.

**Parameters**:
- `momentum_period`: Period for momentum calculation (default: 10)
- `entry_threshold`: Momentum threshold for entry (default: 0.05)
- `exit_threshold`: Momentum threshold for exit (default: -0.02)
- `position_size`: Position size per trade (default: 1.0)

**Signal Logic**:
- **Buy Signal**: Momentum > entry_threshold
- **Sell Signal**: Momentum < exit_threshold

## Custom Strategy Development

### Basic Strategy Structure

```cpp
#include "trading_strategy.hpp"

class MyCustomStrategy : public ITradingStrategy {
public:
    MyCustomStrategy() = default;
    ~MyCustomStrategy() override = default;

    void initialize(const StrategyParameters& params) override {
        params_ = params;

        // Initialize your strategy parameters
        fast_period_ = params.get_numeric_param("fast_period", 10);
        slow_period_ = params.get_numeric_param("slow_period", 30);
        position_size_ = params.get_numeric_param("position_size", 1.0);
    }

    std::vector<TradingSignal> process_data(
        const MarketData& data,
        const PortfolioSnapshot& portfolio) override {

        std::vector<TradingSignal> signals;

        // Add price to history
        prices_.push_back(data.close);

        // Maintain price history size
        if (prices_.size() > slow_period_ * 2) {
            prices_.erase(prices_.begin());
        }

        // Implement your strategy logic
        if (prices_.size() >= slow_period_) {
            // Calculate indicators
            double fast_ma = calculate_moving_average(prices_, fast_period_);
            double slow_ma = calculate_moving_average(prices_, slow_period_);

            // Generate signals
            if (fast_ma > slow_ma && !has_position_) {
                TradingSignal signal;
                signal.type = SignalType::BUY;
                signal.symbol = data.symbol;
                signal.quantity = position_size_;
                signal.price = data.close;
                signal.reason = "Fast MA crossed above slow MA";
                signal.timestamp = data.timestamp;
                signals.push_back(signal);
                has_position_ = true;
            } else if (fast_ma < slow_ma && has_position_) {
                TradingSignal signal;
                signal.type = SignalType::SELL;
                signal.symbol = data.symbol;
                signal.quantity = position_size_;
                signal.price = data.close;
                signal.reason = "Fast MA crossed below slow MA";
                signal.timestamp = data.timestamp;
                signals.push_back(signal);
                has_position_ = false;
            }
        }

        return signals;
    }

    void on_order_fill(const Order& order) override {
        has_position_ = (order.side == OrderSide::BUY);
    }

    void on_position_update(const Position& position) override {
        has_position_ = (position.quantity != 0.0);
    }

    std::string get_name() const override {
        return "My Custom Strategy";
    }

    std::string get_description() const override {
        return "A custom trading strategy implementation";
    }

    StrategyParameters get_parameters() const override {
        return params_;
    }

    void reset() override {
        prices_.clear();
        has_position_ = false;
        params_ = StrategyParameters();
    }

private:
    StrategyParameters params_;
    std::vector<double> prices_;
    int fast_period_;
    int slow_period_;
    double position_size_;
    bool has_position_;

    double calculate_moving_average(const std::vector<double>& prices, int period) {
        if (prices.size() < period) return 0.0;

        double sum = 0.0;
        for (size_t i = prices.size() - period; i < prices.size(); ++i) {
            sum += prices[i];
        }

        return sum / period;
    }
};
```

### Advanced Strategy Features

#### Multi-Timeframe Analysis

```cpp
std::vector<TradingSignal> process_multi_timeframe_data(
    const std::vector<MarketData>& data_1m,
    const std::vector<MarketData>& data_5m,
    const std::vector<MarketData>& data_1h) {

    // Analyze different timeframes
    auto short_term_trend = analyze_trend(data_1m);
    auto medium_term_trend = analyze_trend(data_5m);
    auto long_term_trend = analyze_trend(data_1h);

    // Combine signals from multiple timeframes
    if (short_term_trend == "bullish" &&
        medium_term_trend == "bullish" &&
        long_term_trend != "bearish") {
        // Generate buy signal
    }
}
```

#### Risk Management Integration

```cpp
std::vector<TradingSignal> apply_risk_management(
    const std::vector<TradingSignal>& signals,
    const PortfolioSnapshot& portfolio) {

    std::vector<TradingSignal> filtered_signals;

    for (const auto& signal : signals) {
        // Check position size limits
        double current_exposure = calculate_portfolio_exposure(portfolio);
        double max_exposure = get_max_exposure_limit();

        if (current_exposure + signal.quantity > max_exposure) {
            continue; // Skip signal if it exceeds exposure limits
        }

        // Check drawdown limits
        double current_drawdown = calculate_current_drawdown(portfolio);
        double max_drawdown = get_max_drawdown_limit();

        if (current_drawdown > max_drawdown) {
            continue; // Skip signal if drawdown limit exceeded
        }

        filtered_signals.push_back(signal);
    }

    return filtered_signals;
}
```

#### Machine Learning Integration

```cpp
class MLBasedStrategy : public ITradingStrategy {
public:
    void initialize(const StrategyParameters& params) override {
        // Load ML model
        model_ = load_ml_model(params.get_string_param("model_path"));

        // Initialize feature engineering
        feature_engineer_ = std::make_unique<FeatureEngineer>();
    }

    std::vector<TradingSignal> process_data(
        const MarketData& data,
        const PortfolioSnapshot& portfolio) override {

        // Extract features
        auto features = feature_engineer_->extract_features(data, historical_data_);

        // Make prediction
        auto prediction = model_->predict(features);

        // Generate signal based on prediction
        if (prediction.confidence > 0.8) {
            if (prediction.direction == "buy") {
                return generate_buy_signal(data);
            } else if (prediction.direction == "sell") {
                return generate_sell_signal(data);
            }
        }

        return {};
    }

private:
    std::unique_ptr<MLModel> model_;
    std::unique_ptr<FeatureEngineer> feature_engineer_;
    std::vector<MarketData> historical_data_;
};
```

## Strategy Testing and Validation

### Backtesting

```cpp
void backtest_strategy() {
    // Create backtesting engine
    BacktestEngine engine;

    // Configure backtest
    BacktestConfig config;
    config.symbols = {"BTC", "ETH"};
    config.initial_capital = 100000.0;
    config.start_date = std::chrono::sys_days{2023y/1/1};
    config.end_date = std::chrono::sys_days{2023y/12/31};

    // Create and configure strategy
    StrategyParameters params;
    params.name = "My Custom Strategy";
    params.numeric_params["fast_period"] = 10;
    params.numeric_params["slow_period"] = 30;

    auto strategy = std::make_unique<MyCustomStrategy>();
    strategy->initialize(params);
    engine.add_strategy(std::move(strategy));

    // Load data source
    auto data_source = std::make_unique<CSVDataSource>("/path/to/data");
    engine.set_data_source(std::move(data_source));

    // Run backtest
    BacktestResult result = engine.run_backtest();

    // Analyze results
    std::cout << "=== Backtest Results ===" << std::endl;
    std::cout << "Total Return: " << result.metrics.total_return * 100 << "%" << std::endl;
    std::cout << "Sharpe Ratio: " << result.metrics.sharpe_ratio << std::endl;
    std::cout << "Max Drawdown: " << result.metrics.max_drawdown << "%" << std::endl;
    std::cout << "Win Rate: " << result.metrics.win_rate * 100 << "%" << std::endl;
    std::cout << "Total Trades: " << result.metrics.total_trades << std::endl;
    std::cout << "Profit Factor: " << result.metrics.profit_factor << std::endl;

    // Export results
    export_results_to_csv(result, "backtest_results.csv");
}
```

### Walk-Forward Analysis

```cpp
void walk_forward_analysis() {
    BacktestEngine engine;
    WalkForwardConfig wf_config;
    wf_config.training_window = std::chrono::days(365);
    wf_config.testing_window = std::chrono::days(30);
    wf_config.step_size = std::chrono::days(30);

    auto strategy = std::make_unique<MyCustomStrategy>();
    std::vector<BacktestResult> results = engine.run_walk_forward_analysis(
        std::move(strategy), wf_config);

    // Analyze walk-forward results
    double avg_sharpe = 0.0;
    for (const auto& result : results) {
        avg_sharpe += result.metrics.sharpe_ratio;
    }
    avg_sharpe /= results.size();

    std::cout << "Average Walk-Forward Sharpe Ratio: " << avg_sharpe << std::endl;
}
```

### Monte Carlo Simulation

```cpp
void monte_carlo_simulation() {
    BacktestEngine engine;
    MonteCarloConfig mc_config;
    mc_config.num_simulations = 1000;
    mc_config.randomize_entry_timing = true;
    mc_config.randomize_transaction_costs = true;

    auto strategy = std::make_unique<MyCustomStrategy>();
    std::vector<BacktestResult> results = engine.run_monte_carlo_analysis(
        std::move(strategy), mc_config);

    // Analyze Monte Carlo results
    std::vector<double> returns;
    for (const auto& result : results) {
        returns.push_back(result.metrics.total_return);
    }

    // Calculate statistics
    double mean_return = calculate_mean(returns);
    double std_dev = calculate_std_dev(returns, mean_return);
    double var_95 = calculate_percentile(returns, 5); // 5th percentile

    std::cout << "Monte Carlo Results:" << std::endl;
    std::cout << "Mean Return: " << mean_return * 100 << "%" << std::endl;
    std::cout << "Std Dev: " << std_dev * 100 << "%" << std::endl;
    std::cout << "VaR 95%: " << var_95 * 100 << "%" << std::endl;
}
```

## Performance Optimization

### Strategy Optimization

```cpp
BacktestResult optimize_strategy_parameters() {
    BacktestEngine engine;

    // Define parameter ranges
    std::vector<ParameterRange> param_ranges = {
        {"fast_period", 5, 20, 1},
        {"slow_period", 20, 50, 5},
        {"position_size", 0.5, 2.0, 0.1}
    };

    // Create strategy template
    auto strategy_template = std::make_unique<MyCustomStrategy>();

    // Run optimization
    BacktestResult best_result = engine.optimize_strategy_parameters(
        std::move(strategy_template),
        param_ranges,
        [](const BacktestResult& r) { return r.metrics.sharpe_ratio; }
    );

    std::cout << "Best Parameters:" << std::endl;
    std::cout << "Fast Period: " << best_result.config.parameters.get_numeric_param("fast_period") << std::endl;
    std::cout << "Slow Period: " << best_result.config.parameters.get_numeric_param("slow_period") << std::endl;
    std::cout << "Position Size: " << best_result.config.parameters.get_numeric_param("position_size") << std::endl;
    std::cout << "Best Sharpe Ratio: " << best_result.metrics.sharpe_ratio << std::endl;

    return best_result;
}
```

### Multi-Strategy Portfolios

```cpp
class MultiStrategyPortfolio : public ITradingStrategy {
public:
    void initialize(const StrategyParameters& params) override {
        // Initialize multiple strategies
        strategies_.push_back(StrategyFactory::create_moving_average_strategy(10, 30));
        strategies_.push_back(StrategyFactory::create_rsi_strategy(14, 70, 30));
        strategies_.push_back(StrategyFactory::create_momentum_strategy(10, 0.05, -0.02));

        // Set allocation weights
        allocation_weights_ = {0.4, 0.3, 0.3}; // 40%, 30%, 30%
    }

    std::vector<TradingSignal> process_data(
        const MarketData& data,
        const PortfolioSnapshot& portfolio) override {

        std::vector<TradingSignal> all_signals;

        // Get signals from all strategies
        for (size_t i = 0; i < strategies_.size(); ++i) {
            auto signals = strategies_[i]->process_data(data, portfolio);

            // Adjust signal quantities based on allocation weights
            for (auto& signal : signals) {
                signal.quantity *= allocation_weights_[i];
            }

            all_signals.insert(all_signals.end(), signals.begin(), signals.end());
        }

        // Risk management and signal filtering
        return apply_risk_management(all_signals, portfolio);
    }

private:
    std::vector<std::unique_ptr<ITradingStrategy>> strategies_;
    std::vector<double> allocation_weights_;
};
```

## Best Practices

### Strategy Development

1. **Start Simple**: Begin with basic strategies and gradually add complexity
2. **Backtest Thoroughly**: Use walk-forward analysis and Monte Carlo simulations
3. **Include Transaction Costs**: Account for fees, slippage, and market impact
4. **Risk Management**: Always include position sizing and stop-loss logic
5. **Overfitting Prevention**: Validate on out-of-sample data

### Performance Monitoring

1. **Track Key Metrics**: Sharpe ratio, maximum drawdown, win rate, profit factor
2. **Regular Rebalancing**: Review and adjust strategy parameters periodically
3. **Market Regime Adaptation**: Strategies may perform differently in various market conditions
4. **Live Testing**: Use paper trading before deploying with real capital

### Risk Management

1. **Position Sizing**: Use Kelly criterion or fixed percentage allocation
2. **Stop Losses**: Implement trailing stops and maximum drawdown limits
3. **Diversification**: Spread risk across multiple strategies and assets
4. **Stress Testing**: Test strategies under extreme market conditions

## Common Pitfalls

1. **Overfitting**: Optimizing too many parameters on historical data
2. **Ignoring Transaction Costs**: Underestimating the impact of fees and slippage
3. **Survivorship Bias**: Testing only on currently successful assets
4. **Look-Ahead Bias**: Using future information in backtesting
5. **Data Snooping**: Repeated testing until finding profitable results by chance

Remember: Past performance does not guarantee future results. Always use proper risk management and consider your risk tolerance before deploying strategies with real capital.
