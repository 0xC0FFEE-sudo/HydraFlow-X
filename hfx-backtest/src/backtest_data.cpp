/**
 * @file backtest_data.cpp
 * @brief Backtesting Data Structures Implementation
 */

#include "../include/backtest_data.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cmath>
#include <numeric>

namespace hfx {
namespace backtest {

// Utility functions implementation
std::string order_type_to_string(OrderType type) {
    switch (type) {
        case OrderType::MARKET: return "market";
        case OrderType::LIMIT: return "limit";
        case OrderType::STOP: return "stop";
        case OrderType::STOP_LIMIT: return "stop_limit";
        case OrderType::TRAILING_STOP: return "trailing_stop";
        default: return "unknown";
    }
}

OrderType string_to_order_type(const std::string& type_str) {
    if (type_str == "market") return OrderType::MARKET;
    if (type_str == "limit") return OrderType::LIMIT;
    if (type_str == "stop") return OrderType::STOP;
    if (type_str == "stop_limit") return OrderType::STOP_LIMIT;
    if (type_str == "trailing_stop") return OrderType::TRAILING_STOP;
    return OrderType::MARKET;
}

std::string order_side_to_string(OrderSide side) {
    switch (side) {
        case OrderSide::BUY: return "buy";
        case OrderSide::SELL: return "sell";
        default: return "unknown";
    }
}

OrderSide string_to_order_side(const std::string& side_str) {
    if (side_str == "buy") return OrderSide::BUY;
    if (side_str == "sell") return OrderSide::SELL;
    return OrderSide::BUY;
}

double calculate_returns_percentage(double start_value, double end_value) {
    if (start_value == 0.0) return 0.0;
    return ((end_value - start_value) / start_value) * 100.0;
}

double calculate_annualized_return(double total_return, int days) {
    if (days <= 0) return 0.0;
    return std::pow(1.0 + total_return, 365.0 / days) - 1.0;
}

double calculate_volatility(const std::vector<double>& returns) {
    if (returns.empty()) return 0.0;

    double mean = std::accumulate(returns.begin(), returns.end(), 0.0) / returns.size();
    double variance = 0.0;

    for (double ret : returns) {
        variance += std::pow(ret - mean, 2);
    }

    variance /= returns.size();
    return std::sqrt(variance * 252); // Annualized volatility
}

double calculate_sharpe_ratio(double annualized_return, double volatility) {
    // Assuming risk-free rate of 2%
    const double risk_free_rate = 0.02;
    if (volatility == 0.0) return 0.0;
    return (annualized_return - risk_free_rate) / volatility;
}

double calculate_max_drawdown(const std::vector<EquityPoint>& equity_curve) {
    if (equity_curve.empty()) return 0.0;

    double max_drawdown = 0.0;
    double peak = equity_curve[0].portfolio_value;

    for (const auto& point : equity_curve) {
        if (point.portfolio_value > peak) {
            peak = point.portfolio_value;
        }

        double drawdown = (peak - point.portfolio_value) / peak;
        max_drawdown = std::max(max_drawdown, drawdown);
    }

    return max_drawdown * 100.0; // Convert to percentage
}

double calculate_value_at_risk(const std::vector<double>& returns, double confidence_level) {
    if (returns.empty()) return 0.0;

    std::vector<double> sorted_returns = returns;
    std::sort(sorted_returns.begin(), sorted_returns.end());

    size_t index = static_cast<size_t>((1.0 - confidence_level) * sorted_returns.size());
    if (index >= sorted_returns.size()) index = sorted_returns.size() - 1;

    return -sorted_returns[index]; // VaR is positive, so negate the loss
}

// CSV Data Source Implementation
CSVDataSource::CSVDataSource(const std::string& data_directory)
    : data_directory_(data_directory) {
}

std::vector<MarketData> CSVDataSource::load_data(const std::string& symbol,
                                               std::chrono::system_clock::time_point start,
                                               std::chrono::system_clock::time_point end) {
    std::string file_path = data_directory_ + "/" + symbol + ".csv";

    auto all_data = parse_csv_file(file_path);
    std::vector<MarketData> filtered_data;

    for (const auto& data : all_data) {
        if (data.timestamp >= start && data.timestamp <= end) {
            filtered_data.push_back(data);
        }
    }

    // Sort by timestamp
    std::sort(filtered_data.begin(), filtered_data.end(),
              [](const MarketData& a, const MarketData& b) {
                  return a.timestamp < b.timestamp;
              });

    return filtered_data;
}

std::vector<std::string> CSVDataSource::get_available_symbols() {
    // This would scan the data directory for CSV files
    // For now, return a placeholder
    return {"BTC", "ETH", "SOL", "ADA", "DOT"};
}

bool CSVDataSource::symbol_exists(const std::string& symbol) {
    auto available = get_available_symbols();
    return std::find(available.begin(), available.end(), symbol) != available.end();
}

std::vector<MarketData> CSVDataSource::parse_csv_file(const std::string& file_path) {
    std::vector<MarketData> data;
    std::ifstream file(file_path);

    if (!file.is_open()) {
        HFX_LOG_ERROR("[ERROR] Message");
        return data;
    }

    std::string line;
    bool is_header = true;

    while (std::getline(file, line)) {
        if (is_header) {
            is_header = false;
            continue;
        }

        try {
            MarketData market_data = parse_csv_line(line);
            data.push_back(market_data);
        } catch (const std::exception& e) {
            HFX_LOG_ERROR("[ERROR] Message");
        }
    }

    return data;
}

MarketData CSVDataSource::parse_csv_line(const std::string& line) {
    MarketData data;
    std::stringstream ss(line);
    std::string token;

    // Expected CSV format: timestamp,symbol,open,high,low,close,volume,vwap
    std::vector<std::string> tokens;
    while (std::getline(ss, token, ',')) {
        tokens.push_back(token);
    }

    if (tokens.size() < 8) {
        throw std::runtime_error("Invalid CSV format: insufficient columns");
    }

    // Parse timestamp (assuming Unix timestamp)
    auto timestamp_ms = std::stoll(tokens[0]);
    data.timestamp = std::chrono::system_clock::time_point(
        std::chrono::milliseconds(timestamp_ms));

    data.symbol = tokens[1];
    data.open = std::stod(tokens[2]);
    data.high = std::stod(tokens[3]);
    data.low = std::stod(tokens[4]);
    data.close = std::stod(tokens[5]);
    data.volume = std::stod(tokens[6]);
    data.vwap = std::stod(tokens[7]);

    return data;
}

// Memory Data Source Implementation
MemoryDataSource::MemoryDataSource() = default;

void MemoryDataSource::add_data(const std::string& symbol, const std::vector<MarketData>& data) {
    data_[symbol] = data;
}

std::vector<MarketData> MemoryDataSource::load_data(const std::string& symbol,
                                                   std::chrono::system_clock::time_point start,
                                                   std::chrono::system_clock::time_point end) {
    auto it = data_.find(symbol);
    if (it == data_.end()) {
        return {};
    }

    const auto& all_data = it->second;
    std::vector<MarketData> filtered_data;

    for (const auto& data : all_data) {
        if (data.timestamp >= start && data.timestamp <= end) {
            filtered_data.push_back(data);
        }
    }

    return filtered_data;
}

std::vector<std::string> MemoryDataSource::get_available_symbols() {
    std::vector<std::string> symbols;
    symbols.reserve(data_.size());

    for (const auto& pair : data_) {
        symbols.push_back(pair.first);
    }

    return symbols;
}

bool MemoryDataSource::symbol_exists(const std::string& symbol) {
    return data_.find(symbol) != data_.end();
}

} // namespace backtest
} // namespace hfx
