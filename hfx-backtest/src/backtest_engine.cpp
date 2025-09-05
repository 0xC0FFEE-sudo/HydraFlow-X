/**
 * @file backtest_engine.cpp
 * @brief Backtesting Engine Implementation
 */

#include "../include/backtest_engine.hpp"
#include <algorithm>
#include <random>
#include <future>
#include <iostream>
#include <fstream>
#include <map>
#include <utility>

namespace hfx {
namespace backtest {

// BacktestEngine Implementation
BacktestEngine::BacktestEngine(const BacktestEngineConfig& config)
    : engine_config_(config), is_running_(false) {
}

BacktestEngine::~BacktestEngine() {
    // Clean up any running operations
}

void BacktestEngine::set_data_source(std::unique_ptr<IDataSource> data_source) {
    data_source_ = std::move(data_source);
}

void BacktestEngine::add_strategy(std::unique_ptr<ITradingStrategy> strategy) {
    strategies_.push_back(std::move(strategy));
}

void BacktestEngine::set_config(const BacktestConfig& config) {
    backtest_config_ = config;
}

BacktestResult BacktestEngine::run_backtest() {
    if (!data_source_ || strategies_.empty()) {
        BacktestResult result;
        result.success = false;
        result.error_message = "No data source or strategies configured";
        return result;
    }

    is_running_ = true;
    auto start_time = std::chrono::system_clock::now();

    try {
        BacktestResult result = execute_backtest(strategies_[0].get(), backtest_config_);

        auto end_time = std::chrono::system_clock::now();
        result.execution_time = std::chrono::duration_cast<std::chrono::milliseconds>(
            end_time - start_time);

        result.start_time = start_time;
        result.end_time = end_time;
        result.success = true;

        is_running_ = false;
        return result;

    } catch (const std::exception& e) {
        BacktestResult result;
        result.success = false;
        result.error_message = std::string("Backtest execution failed: ") + e.what();
        result.start_time = start_time;
        result.end_time = std::chrono::system_clock::now();

        is_running_ = false;
        return result;
    }
}

BacktestResult BacktestEngine::run_backtest_async(ProgressCallback progress_callback) {
    return run_backtest(); // Simplified synchronous implementation
}

BacktestResult BacktestEngine::execute_backtest(ITradingStrategy* strategy,
                                              const BacktestConfig& config,
                                              ProgressCallback progress_callback) {
    BacktestResult result;
    result.config = config;

    // Validate configuration
    if (!validate_backtest_config(config)) {
        result.success = false;
        result.error_message = "Invalid backtest configuration";
        return result;
    }

    // Validate data availability
    validate_data_availability(config);

    // Initialize portfolio
    PortfolioSnapshot portfolio;
    initialize_portfolio(portfolio, config);

    // Load market data
    std::vector<MarketData> market_data;
    for (const std::string& symbol : config.symbols) {
        auto symbol_data = load_market_data(symbol, config);
        market_data.insert(market_data.end(), symbol_data.begin(), symbol_data.end());
    }

    // Sort all market data by timestamp
    std::sort(market_data.begin(), market_data.end(),
              [](const MarketData& a, const MarketData& b) {
                  return a.timestamp < b.timestamp;
              });

    // Initialize equity curve
    EquityPoint initial_point;
    initial_point.timestamp = market_data.empty() ? config.start_date : market_data[0].timestamp;
    initial_point.portfolio_value = config.initial_capital;
    initial_point.drawdown = 0.0;
    initial_point.peak_value = config.initial_capital;
    result.equity_curve.push_back(initial_point);

    // Process each data point
    std::vector<Trade> trades;
    size_t processed_count = 0;

    for (const auto& data : market_data) {
        // Update portfolio with current market data
        update_portfolio(portfolio, data);

        // Get signals from strategy
        auto signals = strategy->process_data(data, portfolio);

        // Process signals into orders
        auto orders = process_signals(signals, portfolio, data);

        // Execute orders
        execute_orders(orders, portfolio, data, trades, config);

        // Record equity point
        EquityPoint equity_point;
        equity_point.timestamp = data.timestamp;
        equity_point.portfolio_value = portfolio.total_value;
        equity_point.peak_value = result.equity_curve.back().peak_value;
        equity_point.drawdown = (equity_point.peak_value - equity_point.portfolio_value) /
                               equity_point.peak_value;

        if (equity_point.portfolio_value > equity_point.peak_value) {
            equity_point.peak_value = equity_point.portfolio_value;
        }

        result.equity_curve.push_back(equity_point);

        // Report progress
        processed_count++;
        if (progress_callback && processed_count % 100 == 0) {
            double progress = static_cast<double>(processed_count) / market_data.size();
            progress_callback(progress, "Processing market data...");
        }
    }

    // Store trades
    result.trades = trades;

    // Calculate performance metrics
    calculate_performance_metrics(result);

    // Calculate monthly performance
    calculate_monthly_performance(result);

    if (progress_callback) {
        progress_callback(1.0, "Backtest completed");
    }

    return result;
}

void BacktestEngine::initialize_portfolio(PortfolioSnapshot& portfolio,
                                        const BacktestConfig& config) {
    portfolio.cash = config.initial_capital;
    portfolio.total_value = config.initial_capital;
    portfolio.timestamp = config.start_date;
}

void BacktestEngine::update_portfolio(PortfolioSnapshot& portfolio,
                                    const MarketData& data) {
    // Update position values with current market data
    auto it = portfolio.positions.find(data.symbol);
    if (it != portfolio.positions.end()) {
        Position& position = it->second;
        position.current_price = data.close;
        position.unrealized_pnl = position.quantity * (data.close - position.entry_price);
        position.last_update = data.timestamp;
    }

    // Recalculate total portfolio value
    portfolio.equity = portfolio.cash;
    portfolio.unrealized_pnl = 0.0;
    portfolio.realized_pnl = 0.0;

    for (const auto& [symbol, position] : portfolio.positions) {
        portfolio.equity += position.quantity * position.current_price;
        portfolio.unrealized_pnl += position.unrealized_pnl;
        portfolio.realized_pnl += position.realized_pnl;
    }

    portfolio.total_pnl = portfolio.unrealized_pnl + portfolio.realized_pnl;
    portfolio.total_value = portfolio.equity;
    portfolio.timestamp = data.timestamp;
}

std::vector<Order> BacktestEngine::process_signals(const std::vector<TradingSignal>& signals,
                                                 PortfolioSnapshot& portfolio,
                                                 const MarketData& data) {
    std::vector<Order> orders;

    for (const auto& signal : signals) {
        Order order;
        order.order_id = "order_" + std::to_string(orders.size() + 1);
        order.symbol = signal.symbol;
        order.timestamp = signal.timestamp;

        switch (signal.type) {
            case SignalType::BUY:
                order.type = OrderType::MARKET;
                order.side = OrderSide::BUY;
                order.quantity = signal.quantity;
                order.price = data.close;
                break;

            case SignalType::SELL:
                order.type = OrderType::MARKET;
                order.side = OrderSide::SELL;
                order.quantity = signal.quantity;
                order.price = data.close;
                break;

            case SignalType::CLOSE_POSITION: {
                // Close existing position
                auto it = portfolio.positions.find(signal.symbol);
                if (it != portfolio.positions.end()) {
                    order.type = OrderType::MARKET;
                    order.side = (it->second.quantity > 0) ? OrderSide::SELL : OrderSide::BUY;
                    order.quantity = std::abs(it->second.quantity);
                    order.price = data.close;
                }
                break;
            }

            default:
                continue;
        }

        orders.push_back(order);
    }

    return orders;
}

void BacktestEngine::execute_orders(std::vector<Order>& orders,
                                   PortfolioSnapshot& portfolio,
                                   const MarketData& data,
                                   std::vector<Trade>& trades,
                                   const BacktestConfig& config) {
    for (auto& order : orders) {
        double execution_price = calculate_slippage(order, data, config);
        double commission = calculate_commission(order, config);
        double total_cost = execution_price * order.quantity + commission;

        // Check if we have sufficient funds
        if (order.side == OrderSide::BUY) {
            if (portfolio.cash < total_cost) {
                continue; // Insufficient funds
            }
            portfolio.cash -= total_cost;
        } else {
            // Check if we have the position to sell
            auto it = portfolio.positions.find(order.symbol);
            if (it == portfolio.positions.end() || it->second.quantity < order.quantity) {
                continue; // Insufficient position
            }
        }

        // Execute the order
        Trade trade;
        trade.trade_id = "trade_" + std::to_string(trades.size() + 1);
        trade.symbol = order.symbol;
        trade.side = order.side;
        trade.quantity = order.quantity;
        trade.price = execution_price;
        trade.value = execution_price * order.quantity;
        trade.fees = commission;
        trade.timestamp = data.timestamp;
        trade.strategy_name = "backtest_strategy";

        // Update position
        auto& position = portfolio.positions[order.symbol];
        if (order.side == OrderSide::BUY) {
            double new_quantity = position.quantity + order.quantity;
            double new_entry_price = (position.quantity * position.entry_price +
                                    order.quantity * execution_price) / new_quantity;
            position.quantity = new_quantity;
            position.entry_price = new_entry_price;
        } else {
            position.realized_pnl += order.quantity * (execution_price - position.entry_price);
            position.quantity -= order.quantity;

            // Remove position if fully closed
            if (std::abs(position.quantity) < 0.0001) {
                portfolio.positions.erase(order.symbol);
            }
        }

        position.current_price = data.close;
        position.last_update = data.timestamp;

        order.filled = true;
        order.filled_price = execution_price;
        order.filled_quantity = order.quantity;
        order.fill_timestamp = data.timestamp;

        trades.push_back(trade);
    }
}

double BacktestEngine::calculate_slippage(const Order& order,
                                        const MarketData& data,
                                        const BacktestConfig& config) {
    if (!config.enable_slippage) {
        return order.price;
    }

    // Simple slippage model
    double slippage = data.close * config.slippage_rate;
    return order.side == OrderSide::BUY ?
           data.close + slippage : data.close - slippage;
}

double BacktestEngine::calculate_commission(const Order& order,
                                          const BacktestConfig& config) {
    return config.commission_per_trade +
           (order.quantity * order.price * config.commission_rate);
}

void BacktestEngine::calculate_performance_metrics(BacktestResult& result) {
    if (result.equity_curve.empty()) return;

    const auto& equity_curve = result.equity_curve;
    const auto& trades = result.trades;

    // Basic metrics
    double initial_value = equity_curve[0].portfolio_value;
    double final_value = equity_curve.back().portfolio_value;

    result.metrics.total_return = (final_value - initial_value) / initial_value;

    auto days = std::chrono::duration_cast<std::chrono::hours>(
        equity_curve.back().timestamp - equity_curve[0].timestamp).count() / 24.0;

    result.metrics.annualized_return = calculate_annualized_return(
        result.metrics.total_return, static_cast<int>(days));

    // Calculate volatility from equity curve
    std::vector<double> returns;
    for (size_t i = 1; i < equity_curve.size(); ++i) {
        double ret = (equity_curve[i].portfolio_value - equity_curve[i-1].portfolio_value) /
                     equity_curve[i-1].portfolio_value;
        returns.push_back(ret);
    }

    result.metrics.volatility = calculate_volatility(returns);
    result.metrics.sharpe_ratio = calculate_sharpe_ratio(
        result.metrics.annualized_return, result.metrics.volatility);
    result.metrics.max_drawdown = calculate_max_drawdown(equity_curve);

    // Trade metrics
    result.metrics.total_trades = trades.size();
    result.metrics.winning_trades = 0;
    result.metrics.losing_trades = 0;

    double total_win_pnl = 0.0;
    double total_loss_pnl = 0.0;

    for (const auto& trade : trades) {
        double pnl = (trade.side == OrderSide::BUY) ?
                     (trade.price - trade.price) : (trade.price - trade.price); // Simplified

        if (pnl > 0) {
            result.metrics.winning_trades++;
            total_win_pnl += pnl;
            result.metrics.largest_win = std::max(result.metrics.largest_win, pnl);
        } else {
            result.metrics.losing_trades++;
            total_loss_pnl += std::abs(pnl);
            result.metrics.largest_loss = std::max(result.metrics.largest_loss, std::abs(pnl));
        }
    }

    if (result.metrics.winning_trades > 0) {
        result.metrics.avg_win = total_win_pnl / result.metrics.winning_trades;
    }
    if (result.metrics.losing_trades > 0) {
        result.metrics.avg_loss = total_loss_pnl / result.metrics.losing_trades;
    }

    result.metrics.win_rate = result.metrics.total_trades > 0 ?
                             static_cast<double>(result.metrics.winning_trades) / result.metrics.total_trades : 0.0;

    result.metrics.profit_factor = total_loss_pnl > 0 ?
                                  total_win_pnl / total_loss_pnl : 0.0;
}

void BacktestEngine::calculate_monthly_performance(BacktestResult& result) {
    // Group equity points by month and calculate monthly returns
    std::map<std::pair<int, int>, std::vector<EquityPoint>> monthly_data;

    for (const auto& point : result.equity_curve) {
        auto time_t = std::chrono::system_clock::to_time_t(point.timestamp);
        std::tm* tm = std::localtime(&time_t);

        std::pair<int, int> month_key = {tm->tm_year + 1900, tm->tm_mon + 1};
        monthly_data[month_key].push_back(point);
    }

    for (const auto& [month_key, points] : monthly_data) {
        if (points.size() < 2) continue;

        MonthlyPerformance monthly;
        monthly.year = month_key.first;
        monthly.month = month_key.second;
        monthly.starting_value = points.front().portfolio_value;
        monthly.ending_value = points.back().portfolio_value;
        monthly.return_percentage = calculate_returns_percentage(
            monthly.starting_value, monthly.ending_value);
        monthly.num_trades = 0; // Would need to count trades in this month

        result.monthly_performance.push_back(monthly);
    }
}

std::vector<MarketData> BacktestEngine::load_market_data(const std::string& symbol,
                                                       const BacktestConfig& config) {
    if (!data_source_) return {};

    return data_source_->load_data(symbol, config.start_date, config.end_date);
}

void BacktestEngine::validate_data_availability(const BacktestConfig& config) {
    if (!data_source_) return;

    for (const std::string& symbol : config.symbols) {
        if (!data_source_->symbol_exists(symbol)) {
            HFX_LOG_ERROR("Warning: Symbol " << symbol << " not found in data source" << std::endl;
        }
    }
}

bool BacktestEngine::validate_backtest_config(const BacktestConfig& config) {
    if (config.symbols.empty()) return false;
    if (config.initial_capital <= 0) return false;
    if (config.start_date >= config.end_date) return false;
    return true;
}

void BacktestEngine::report_progress(double progress, const std::string& message,
                                   ProgressCallback callback) {
    if (callback) {
        callback(progress, message);
    }
}

} // namespace backtest
} // namespace hfx
