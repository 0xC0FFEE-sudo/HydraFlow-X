#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <memory>

namespace hfx {
namespace backtest {

// Market data structure for backtesting
struct MarketData {
    std::string symbol;
    std::chrono::system_clock::time_point timestamp;
    double open;
    double high;
    double low;
    double close;
    double volume;
    double vwap; // Volume Weighted Average Price
    std::unordered_map<std::string, double> indicators; // Technical indicators

    MarketData() : open(0.0), high(0.0), low(0.0), close(0.0), volume(0.0), vwap(0.0) {}
};

// Order types for backtesting
enum class OrderType {
    MARKET = 0,
    LIMIT,
    STOP,
    STOP_LIMIT,
    TRAILING_STOP
};

// Order sides
enum class OrderSide {
    BUY = 0,
    SELL
};

// Order structure
struct Order {
    std::string order_id;
    std::string symbol;
    OrderType type;
    OrderSide side;
    double quantity;
    double price; // For limit orders
    double stop_price; // For stop orders
    std::chrono::system_clock::time_point timestamp;
    bool filled;
    double filled_price;
    double filled_quantity;
    std::chrono::system_clock::time_point fill_timestamp;

    Order() : quantity(0.0), price(0.0), stop_price(0.0), filled(false),
              filled_price(0.0), filled_quantity(0.0) {}
};

// Position structure
struct Position {
    std::string symbol;
    double quantity;
    double entry_price;
    double current_price;
    double unrealized_pnl;
    double realized_pnl;
    std::chrono::system_clock::time_point entry_time;
    std::chrono::system_clock::time_point last_update;

    Position() : quantity(0.0), entry_price(0.0), current_price(0.0),
                unrealized_pnl(0.0), realized_pnl(0.0) {}
};

// Trade execution record
struct Trade {
    std::string trade_id;
    std::string symbol;
    OrderSide side;
    double quantity;
    double price;
    double value; // quantity * price
    double fees;
    std::chrono::system_clock::time_point timestamp;
    std::string strategy_name;

    Trade() : quantity(0.0), price(0.0), value(0.0), fees(0.0) {}
};

// Portfolio snapshot
struct PortfolioSnapshot {
    std::chrono::system_clock::time_point timestamp;
    double total_value;
    double cash;
    double equity;
    std::unordered_map<std::string, Position> positions;
    double unrealized_pnl;
    double realized_pnl;
    double total_pnl;

    PortfolioSnapshot() : total_value(0.0), cash(0.0), equity(0.0),
                         unrealized_pnl(0.0), realized_pnl(0.0), total_pnl(0.0) {}
};

// Backtesting configuration
struct BacktestConfig {
    std::chrono::system_clock::time_point start_date;
    std::chrono::system_clock::time_point end_date;
    double initial_capital;
    double commission_per_trade; // Fixed commission per trade
    double commission_rate; // Commission as percentage
    bool enable_slippage;
    double slippage_rate; // Slippage as percentage
    std::vector<std::string> symbols;
    std::string data_source;
    bool enable_transaction_costs;
    bool enable_market_impact;

    BacktestConfig() : initial_capital(100000.0),
                      commission_per_trade(0.0),
                      commission_rate(0.001), // 0.1%
                      enable_slippage(true),
                      slippage_rate(0.0005), // 0.05%
                      enable_transaction_costs(true),
                      enable_market_impact(false) {}
};

// Performance metrics
struct PerformanceMetrics {
    // Basic metrics
    double total_return;
    double annualized_return;
    double volatility;
    double sharpe_ratio;
    double max_drawdown;
    double win_rate;
    int total_trades;
    int winning_trades;
    int losing_trades;

    // Risk metrics
    double var_95; // Value at Risk 95%
    double cvar_95; // Conditional VaR 95%
    double beta; // Market beta
    double alpha; // Jensen's alpha

    // Trade metrics
    double avg_win;
    double avg_loss;
    double largest_win;
    double largest_loss;
    double profit_factor; // Gross profit / Gross loss
    double recovery_factor; // Net profit / Max drawdown

    // Timing metrics
    double avg_holding_period_days;
    std::chrono::system_clock::time_point best_trade_date;
    std::chrono::system_clock::time_point worst_trade_date;

    PerformanceMetrics() : total_return(0.0), annualized_return(0.0), volatility(0.0),
                          sharpe_ratio(0.0), max_drawdown(0.0), win_rate(0.0),
                          total_trades(0), winning_trades(0), losing_trades(0),
                          var_95(0.0), cvar_95(0.0), beta(0.0), alpha(0.0),
                          avg_win(0.0), avg_loss(0.0), largest_win(0.0), largest_loss(0.0),
                          profit_factor(0.0), recovery_factor(0.0), avg_holding_period_days(0.0) {}
};

// Equity curve point
struct EquityPoint {
    std::chrono::system_clock::time_point timestamp;
    double portfolio_value;
    double drawdown;
    double peak_value;

    EquityPoint() : portfolio_value(0.0), drawdown(0.0), peak_value(0.0) {}
};

// Monthly performance
struct MonthlyPerformance {
    int year;
    int month;
    double return_percentage;
    double starting_value;
    double ending_value;
    int num_trades;

    MonthlyPerformance() : year(0), month(0), return_percentage(0.0),
                          starting_value(0.0), ending_value(0.0), num_trades(0) {}
};

// Backtesting result
struct BacktestResult {
    BacktestConfig config;
    PerformanceMetrics metrics;
    std::vector<EquityPoint> equity_curve;
    std::vector<Trade> trades;
    std::vector<MonthlyPerformance> monthly_performance;
    std::chrono::system_clock::time_point start_time;
    std::chrono::system_clock::time_point end_time;
    std::chrono::milliseconds execution_time;
    bool success;
    std::string error_message;

    BacktestResult() : execution_time(0), success(true) {}
};

// Data source interface
class IDataSource {
public:
    virtual ~IDataSource() = default;
    virtual std::vector<MarketData> load_data(const std::string& symbol,
                                            std::chrono::system_clock::time_point start,
                                            std::chrono::system_clock::time_point end) = 0;
    virtual std::vector<std::string> get_available_symbols() = 0;
    virtual bool symbol_exists(const std::string& symbol) = 0;
};

// CSV data source
class CSVDataSource : public IDataSource {
public:
    explicit CSVDataSource(const std::string& data_directory);
    ~CSVDataSource() override = default;

    std::vector<MarketData> load_data(const std::string& symbol,
                                    std::chrono::system_clock::time_point start,
                                    std::chrono::system_clock::time_point end) override;
    std::vector<std::string> get_available_symbols() override;
    bool symbol_exists(const std::string& symbol) override;

private:
    std::string data_directory_;
    std::vector<MarketData> parse_csv_file(const std::string& file_path);
    MarketData parse_csv_line(const std::string& line);
};

// In-memory data source for testing
class MemoryDataSource : public IDataSource {
public:
    MemoryDataSource();
    ~MemoryDataSource() override = default;

    void add_data(const std::string& symbol, const std::vector<MarketData>& data);
    std::vector<MarketData> load_data(const std::string& symbol,
                                    std::chrono::system_clock::time_point start,
                                    std::chrono::system_clock::time_point end) override;
    std::vector<std::string> get_available_symbols() override;
    bool symbol_exists(const std::string& symbol) override;

private:
    std::unordered_map<std::string, std::vector<MarketData>> data_;
};

// Utility functions
std::string order_type_to_string(OrderType type);
OrderType string_to_order_type(const std::string& type_str);
std::string order_side_to_string(OrderSide side);
OrderSide string_to_order_side(const std::string& side_str);
double calculate_returns_percentage(double start_value, double end_value);
double calculate_annualized_return(double total_return, int days);
double calculate_volatility(const std::vector<double>& returns);
double calculate_sharpe_ratio(double annualized_return, double volatility);
double calculate_max_drawdown(const std::vector<EquityPoint>& equity_curve);
double calculate_value_at_risk(const std::vector<double>& returns, double confidence_level);

} // namespace backtest
} // namespace hfx
