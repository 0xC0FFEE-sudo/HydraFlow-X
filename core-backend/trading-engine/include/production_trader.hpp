/**
 * @file production_trader.hpp
 * @brief Production-ready high frequency trading engine
 */

#pragma once

#include <memory>
#include <atomic>
#include <thread>
#include <mutex>
#include <vector>
#include <queue>
#include <unordered_map>
#include <string>
#include <functional>
#include <chrono>

namespace hfx::trading {

enum class OrderType { MARKET, LIMIT, STOP_LOSS, TAKE_PROFIT };
enum class OrderSide { BUY, SELL };
enum class OrderStatus { PENDING, FILLED, CANCELLED, REJECTED };

struct MarketData {
    std::string symbol;
    double bid_price = 0.0;
    double ask_price = 0.0;
    double volume = 0.0;
    uint64_t timestamp_ns = 0;
};

struct OrderRequest {
    std::string symbol;
    OrderType type;
    OrderSide side;  
    double price;
    double quantity;
    uint64_t timestamp_ns;
};

struct OrderResult {
    std::string order_id;
    OrderStatus status;
    double filled_price = 0.0;
    double filled_quantity = 0.0;
    std::string error_message;
};

struct Position {
    std::string symbol;
    double quantity = 0.0;
    double avg_price = 0.0;
    double unrealized_pnl = 0.0;
    double realized_pnl = 0.0;
};

class ProductionTrader {
public:
    ProductionTrader();
    ~ProductionTrader();

    // Core trading operations
    bool initialize();
    void start_trading();
    void stop_trading();
    
    // Market data
    void on_market_data(const MarketData& data);
    
    // Order management
    std::string submit_order(const OrderRequest& request);
    bool cancel_order(const std::string& order_id);
    
    // Risk management
    void set_max_position(const std::string& symbol, double max_size);
    void set_stop_loss(double percentage);
    void set_take_profit(double percentage);
    
    // Performance monitoring
    struct PerformanceMetrics {
        uint64_t total_orders = 0;
        uint64_t filled_orders = 0;
        uint64_t rejected_orders = 0;
        double total_pnl = 0.0;
        uint64_t avg_latency_ns = 0;
        uint64_t max_latency_ns = 0;
        double sharpe_ratio = 0.0;
        double max_drawdown = 0.0;
    };
    
    PerformanceMetrics get_metrics() const;
    std::vector<Position> get_positions() const;

private:
    std::atomic<bool> running_{false};
    std::unique_ptr<std::thread> trading_thread_;
    
    // Market data
    mutable std::mutex market_data_mutex_;
    std::unordered_map<std::string, MarketData> latest_prices_;
    
    // Orders
    mutable std::mutex orders_mutex_;
    std::unordered_map<std::string, OrderRequest> pending_orders_;
    std::queue<OrderRequest> order_queue_;
    
    // Positions  
    mutable std::mutex positions_mutex_;
    std::unordered_map<std::string, Position> positions_;
    
    // Risk limits
    std::unordered_map<std::string, double> max_positions_;
    double stop_loss_pct_ = 0.02; // 2% stop loss
    double take_profit_pct_ = 0.04; // 4% take profit
    
    // Performance tracking
    mutable std::mutex metrics_mutex_;
    PerformanceMetrics metrics_;
    std::vector<std::chrono::nanoseconds> latency_samples_;
    
    // Core trading logic
    void trading_loop();
    void process_order_queue();
    void check_risk_limits();
    void update_positions();
    void calculate_pnl();
    
    // Utility functions
    uint64_t get_timestamp_ns() const;
    std::string generate_order_id();
    bool validate_order(const OrderRequest& request);
    
    std::atomic<uint64_t> order_id_counter_{1};
};

// Factory for creating different trading strategies
class TradingStrategyFactory {
public:
    enum class StrategyType { 
        MARKET_MAKING, 
        ARBITRAGE, 
        MOMENTUM, 
        MEAN_REVERSION 
    };
    
    static std::unique_ptr<ProductionTrader> create_trader(StrategyType type);
};

} // namespace hfx::trading
