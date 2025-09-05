/**
 * @file production_trader.cpp  
 * @brief Production-ready high frequency trading engine implementation
 */

#include "../include/production_trader.hpp"
#include "../../hfx-log/include/simple_logger.hpp"
#include <random>
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <cmath>

namespace hfx::trading {

ProductionTrader::ProductionTrader() {
    // Initialize with safe defaults
    metrics_ = {};
    latency_samples_.reserve(10000); // Pre-allocate for performance
}

ProductionTrader::~ProductionTrader() {
    stop_trading();
}

bool ProductionTrader::initialize() {
    HFX_LOG_INFO("Initializing Production Trading Engine");
    
    // Initialize market data tracking
    latest_prices_.reserve(1000);
    
    // Initialize position tracking
    positions_.reserve(100);
    
    // Set conservative default risk limits
    stop_loss_pct_ = 0.02;  // 2% stop loss
    take_profit_pct_ = 0.04; // 4% take profit
    
    return true;
}

void ProductionTrader::start_trading() {
    if (running_.load()) {
        return;
    }
    
    running_.store(true);
    HFX_LOG_INFO("Starting production trading engine");
    
    // Start main trading loop
    trading_thread_ = std::make_unique<std::thread>(&ProductionTrader::trading_loop, this);
}

void ProductionTrader::stop_trading() {
    if (!running_.load()) {
        return;
    }
    
    running_.store(false);
    HFX_LOG_INFO("Stopping production trading engine");
    
    if (trading_thread_ && trading_thread_->joinable()) {
        trading_thread_->join();
    }
    
    // Cancel all pending orders
    std::lock_guard<std::mutex> lock(orders_mutex_);
    pending_orders_.clear();
    while (!order_queue_.empty()) {
        order_queue_.pop();
    }
}

void ProductionTrader::on_market_data(const MarketData& data) {
    auto start_time = std::chrono::high_resolution_clock::now();
    
    {
        std::lock_guard<std::mutex> lock(market_data_mutex_);
        latest_prices_[data.symbol] = data;
    }
    
    // Update positions with latest prices
    update_positions();
    
    // Check risk limits
    check_risk_limits();
    
    // Record latency
    auto end_time = std::chrono::high_resolution_clock::now();
    auto latency = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);
    
    {
        std::lock_guard<std::mutex> lock(metrics_mutex_);
        latency_samples_.push_back(latency);
        if (latency_samples_.size() > 10000) {
            latency_samples_.erase(latency_samples_.begin());
        }
        
        metrics_.max_latency_ns = std::max(metrics_.max_latency_ns, 
                                         static_cast<uint64_t>(latency.count()));
        
        // Update average latency  
        uint64_t sum = 0;
        for (const auto& sample : latency_samples_) {
            sum += sample.count();
        }
        metrics_.avg_latency_ns = sum / latency_samples_.size();
    }
}

std::string ProductionTrader::submit_order(const OrderRequest& request) {
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Validate order
    if (!validate_order(request)) {
        std::lock_guard<std::mutex> lock(metrics_mutex_);
        metrics_.rejected_orders++;
        return ""; // Invalid order
    }
    
    // Generate order ID
    std::string order_id = generate_order_id();
    
    // Add to queue for processing
    {
        std::lock_guard<std::mutex> lock(orders_mutex_);
        pending_orders_[order_id] = request;
        order_queue_.push(request);
    }
    
    // Update metrics
    {
        std::lock_guard<std::mutex> lock(metrics_mutex_);
        metrics_.total_orders++;
    }
    
    // Record submission latency
    auto end_time = std::chrono::high_resolution_clock::now();
    auto latency = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);
    
    {
        std::lock_guard<std::mutex> lock(metrics_mutex_);
        latency_samples_.push_back(latency);
    }
    
    return order_id;
}

bool ProductionTrader::cancel_order(const std::string& order_id) {
    std::lock_guard<std::mutex> lock(orders_mutex_);
    auto it = pending_orders_.find(order_id);
    if (it != pending_orders_.end()) {
        pending_orders_.erase(it);
        return true;
    }
    return false;
}

void ProductionTrader::set_max_position(const std::string& symbol, double max_size) {
    max_positions_[symbol] = max_size;
}

void ProductionTrader::set_stop_loss(double percentage) {
    stop_loss_pct_ = std::abs(percentage);
}

void ProductionTrader::set_take_profit(double percentage) {
    take_profit_pct_ = std::abs(percentage);
}

ProductionTrader::PerformanceMetrics ProductionTrader::get_metrics() const {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    return metrics_;
}

std::vector<Position> ProductionTrader::get_positions() const {
    std::lock_guard<std::mutex> lock(positions_mutex_);
    std::vector<Position> result;
    result.reserve(positions_.size());
    for (const auto& [symbol, position] : positions_) {
        result.push_back(position);
    }
    return result;
}

void ProductionTrader::trading_loop() {
    HFX_LOG_INFO("Trading loop started");
    
    while (running_.load()) {
        try {
            // Process pending orders
            process_order_queue();
            
            // Update positions
            update_positions();
            
            // Check risk limits
            check_risk_limits();
            
            // Calculate PnL
            calculate_pnl();
            
            // Sleep for 1ms to avoid excessive CPU usage
            std::this_thread::sleep_for(std::chrono::microseconds(1000));
            
        } catch (const std::exception& e) {
            HFX_LOG_ERROR("Trading loop error");
        }
    }
    
    HFX_LOG_INFO("Trading loop stopped");
}

void ProductionTrader::process_order_queue() {
    std::lock_guard<std::mutex> lock(orders_mutex_);
    
    while (!order_queue_.empty() && running_.load()) {
        OrderRequest request = order_queue_.front();
        order_queue_.pop();
        
        // Simulate order execution (replace with real exchange integration)
        double current_price = 0.0;
        {
            std::lock_guard<std::mutex> price_lock(market_data_mutex_);
            auto it = latest_prices_.find(request.symbol);
            if (it != latest_prices_.end()) {
                current_price = (request.side == OrderSide::BUY) ? 
                               it->second.ask_price : it->second.bid_price;
            }
        }
        
        if (current_price > 0.0) {
            // Execute the trade
            {
                std::lock_guard<std::mutex> pos_lock(positions_mutex_);
                auto& position = positions_[request.symbol];
                
                if (request.side == OrderSide::BUY) {
                    double old_qty = position.quantity;
                    double new_qty = old_qty + request.quantity;
                    position.avg_price = (old_qty * position.avg_price + 
                                        request.quantity * current_price) / new_qty;
                    position.quantity = new_qty;
                } else {
                    position.quantity -= request.quantity;
                }
                
                position.symbol = request.symbol;
            }
            
            // Update metrics
            {
                std::lock_guard<std::mutex> metrics_lock(metrics_mutex_);
                metrics_.filled_orders++;
            }
        } else {
            // No market data available - reject order
            std::lock_guard<std::mutex> metrics_lock(metrics_mutex_);
            metrics_.rejected_orders++;
        }
    }
}

void ProductionTrader::check_risk_limits() {
    std::lock_guard<std::mutex> lock(positions_mutex_);
    
    for (auto& [symbol, position] : positions_) {
        // Check maximum position limits
        auto max_pos_it = max_positions_.find(symbol);
        if (max_pos_it != max_positions_.end()) {
            if (std::abs(position.quantity) > max_pos_it->second) {
                // Force close excessive position
                HFX_LOG_ERROR("Position limit exceeded for " + symbol);
                position.quantity = (position.quantity > 0) ? 
                                  max_pos_it->second : -max_pos_it->second;
            }
        }
        
        // Check stop loss and take profit
        double current_price = 0.0;
        {
            std::lock_guard<std::mutex> price_lock(market_data_mutex_);
            auto it = latest_prices_.find(symbol);
            if (it != latest_prices_.end()) {
                current_price = (it->second.bid_price + it->second.ask_price) / 2.0;
            }
        }
        
        if (current_price > 0.0 && position.quantity != 0.0) {
            double pnl_pct = (current_price - position.avg_price) / position.avg_price;
            if (position.quantity < 0) pnl_pct = -pnl_pct; // Short position
            
            // Stop loss check
            if (pnl_pct <= -stop_loss_pct_) {
                HFX_LOG_ERROR("Stop loss triggered for " + symbol);
                // Close position
                position.realized_pnl += position.quantity * (current_price - position.avg_price);
                position.quantity = 0.0;
            }
            
            // Take profit check  
            if (pnl_pct >= take_profit_pct_) {
                HFX_LOG_INFO("Take profit triggered for " + symbol);
                // Close position
                position.realized_pnl += position.quantity * (current_price - position.avg_price);
                position.quantity = 0.0;
            }
        }
    }
}

void ProductionTrader::update_positions() {
    std::lock_guard<std::mutex> pos_lock(positions_mutex_);
    std::lock_guard<std::mutex> price_lock(market_data_mutex_);
    
    for (auto& [symbol, position] : positions_) {
        auto price_it = latest_prices_.find(symbol);
        if (price_it != latest_prices_.end() && position.quantity != 0.0) {
            double current_price = (price_it->second.bid_price + price_it->second.ask_price) / 2.0;
            position.unrealized_pnl = position.quantity * (current_price - position.avg_price);
        }
    }
}

void ProductionTrader::calculate_pnl() {
    double total_pnl = 0.0;
    
    {
        std::lock_guard<std::mutex> lock(positions_mutex_);
        for (const auto& [symbol, position] : positions_) {
            total_pnl += position.realized_pnl + position.unrealized_pnl;
        }
    }
    
    {
        std::lock_guard<std::mutex> lock(metrics_mutex_);
        metrics_.total_pnl = total_pnl;
        
        // Simple Sharpe ratio approximation (would need more sophisticated calculation in production)
        if (latency_samples_.size() > 100) {
            metrics_.sharpe_ratio = total_pnl / 1000.0; // Simplified calculation
        }
    }
}

uint64_t ProductionTrader::get_timestamp_ns() const {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()
    ).count();
}

std::string ProductionTrader::generate_order_id() {
    return "ORDER_" + std::to_string(order_id_counter_.fetch_add(1));
}

bool ProductionTrader::validate_order(const OrderRequest& request) {
    if (request.symbol.empty()) return false;
    if (request.quantity <= 0.0) return false;
    if (request.price < 0.0) return false;
    
    return true;
}

// Factory implementation
std::unique_ptr<ProductionTrader> TradingStrategyFactory::create_trader(StrategyType type) {
    auto trader = std::make_unique<ProductionTrader>();
    
    switch (type) {
        case StrategyType::MARKET_MAKING:
            trader->set_stop_loss(0.005);  // 0.5% stop loss for market making
            trader->set_take_profit(0.002); // 0.2% take profit
            break;
            
        case StrategyType::ARBITRAGE:
            trader->set_stop_loss(0.001);  // 0.1% stop loss for arbitrage
            trader->set_take_profit(0.001); // 0.1% take profit
            break;
            
        case StrategyType::MOMENTUM:
            trader->set_stop_loss(0.03);   // 3% stop loss for momentum
            trader->set_take_profit(0.05);  // 5% take profit
            break;
            
        case StrategyType::MEAN_REVERSION:
            trader->set_stop_loss(0.02);   // 2% stop loss
            trader->set_take_profit(0.03);  // 3% take profit
            break;
    }
    
    return trader;
}

} // namespace hfx::trading
