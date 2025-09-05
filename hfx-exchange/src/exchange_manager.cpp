/**
 * @file exchange_manager.cpp
 * @brief Exchange manager implementation
 */

#include "../include/exchange_manager.hpp"
#include "../../hfx-log/include/simple_logger.hpp"
#include <algorithm>
#include <numeric>
#include <thread>
#include <future>

namespace hfx::exchange {

class ExchangeManager::ExchangeManagerImpl {
public:
    std::unordered_map<std::string, std::unique_ptr<BaseExchange>> exchanges_;
    std::unordered_map<std::string, ExchangeConfig> exchange_configs_;
    std::unordered_map<std::string, ExchangeStats> exchange_stats_;
    
    RiskLimits risk_limits_;
    double default_slippage_tolerance_ = 100.0; // 100 bps = 1%
    std::chrono::milliseconds execution_timeout_{30000}; // 30 seconds
    bool smart_routing_enabled_ = true;
    bool arbitrage_detection_enabled_ = true;
    
    mutable std::mutex manager_mutex_;
    
    // Order tracking
    std::unordered_map<std::string, Order> active_orders_;
    std::atomic<uint64_t> next_order_id_{1};
    
    ExchangeManagerImpl() {
        // Initialize default risk limits
        risk_limits_.max_order_size_usd = 10000.0;
        risk_limits_.max_daily_volume_usd = 100000.0;
        risk_limits_.max_position_size_percent = 20.0;
        risk_limits_.max_open_orders_per_exchange = 50;
        
        HFX_LOG_INFO("[ExchangeManager] Initialized with default risk limits");
    }
    
    bool add_exchange_impl(std::unique_ptr<BaseExchange> exchange, const ExchangeConfig& config) {
        if (!exchange) {
            return false;
        }
        
        std::string exchange_id = exchange->get_exchange_id();
        
        std::lock_guard<std::mutex> lock(manager_mutex_);
        
        if (exchanges_.count(exchange_id)) {
            HFX_LOG_WARN("[ExchangeManager] Exchange already exists: " + exchange_id);
            return false;
        }
        
        // Test connection
        if (!exchange->connect()) {
            HFX_LOG_ERROR("[ExchangeManager] Failed to connect to exchange: " + exchange_id);
            return false;
        }
        
        exchanges_[exchange_id] = std::move(exchange);
        exchange_configs_[exchange_id] = config;
        exchange_stats_[exchange_id] = ExchangeStats{};
        exchange_stats_[exchange_id].last_order_time = std::chrono::system_clock::now();
        
        HFX_LOG_INFO("[ExchangeManager] Added exchange: " + exchange_id);
        return true;
    }
    
    bool remove_exchange_impl(const std::string& exchange_id) {
        std::lock_guard<std::mutex> lock(manager_mutex_);
        
        auto exchange_it = exchanges_.find(exchange_id);
        if (exchange_it == exchanges_.end()) {
            return false;
        }
        
        // Disconnect and remove
        exchange_it->second->disconnect();
        exchanges_.erase(exchange_it);
        exchange_configs_.erase(exchange_id);
        exchange_stats_.erase(exchange_id);
        
        HFX_LOG_INFO("[ExchangeManager] Removed exchange: " + exchange_id);
        return true;
    }
    
    std::vector<MarketData> get_aggregated_ticker_impl(const std::string& symbol) {
        std::vector<MarketData> tickers;
        std::vector<std::future<MarketData>> futures;
        
        {
            std::lock_guard<std::mutex> lock(manager_mutex_);
            
            for (const auto& exchange_pair : exchanges_) {
                if (!exchange_pair.second->is_connected()) {
                    continue;
                }
                
                // Launch async ticker requests
                futures.push_back(std::async(std::launch::async, [&exchange_pair, &symbol]() {
                    try {
                        return exchange_pair.second->get_ticker(symbol);
                    } catch (const std::exception& e) {
                        HFX_LOG_ERROR("[ExchangeManager] Ticker fetch failed for " + 
                                     exchange_pair.first + ": " + e.what());
                        return MarketData{};
                    }
                }));
            }
        }
        
        // Collect results
        for (auto& future : futures) {
            try {
                auto ticker = future.get();
                if (!ticker.symbol.empty() && ticker.last_price > 0) {
                    tickers.push_back(ticker);
                }
            } catch (const std::exception& e) {
                HFX_LOG_ERROR("[ExchangeManager] Future get failed: " + std::string(e.what()));
            }
        }
        
        return tickers;
    }
    
    std::unordered_map<std::string, MarketData> get_best_prices_impl(const std::string& symbol) {
        auto all_tickers = get_aggregated_ticker_impl(symbol);
        std::unordered_map<std::string, MarketData> best_prices;
        
        if (all_tickers.empty()) {
            return best_prices;
        }
        
        // Find best bid and ask across all exchanges
        auto best_bid_it = std::max_element(all_tickers.begin(), all_tickers.end(),
            [](const MarketData& a, const MarketData& b) {
                return a.bid_price < b.bid_price;
            });
        
        auto best_ask_it = std::min_element(all_tickers.begin(), all_tickers.end(),
            [](const MarketData& a, const MarketData& b) {
                return a.ask_price < b.ask_price;
            });
        
        best_prices["best_bid"] = *best_bid_it;
        best_prices["best_ask"] = *best_ask_it;
        
        return best_prices;
    }
    
    RouteResult find_best_execution_route_impl(const std::string& symbol, OrderSide side, double quantity) {
        auto all_tickers = get_aggregated_ticker_impl(symbol);
        RouteResult best_route;
        
        if (all_tickers.empty()) {
            return best_route;
        }
        
        double best_price = (side == OrderSide::BUY) ? std::numeric_limits<double>::max() : 0.0;
        
        for (const auto& ticker : all_tickers) {
            RouteResult route;
            route.exchange_id = ""; // Would need to map ticker back to exchange
            route.quantity = quantity;
            
            if (side == OrderSide::BUY) {
                route.price = ticker.ask_price;
                if (route.price < best_price && route.price > 0) {
                    best_price = route.price;
                    best_route = route;
                }
            } else {
                route.price = ticker.bid_price;
                if (route.price > best_price) {
                    best_price = route.price;
                    best_route = route;
                }
            }
            
            // Calculate estimates
            route.estimated_slippage = calculate_estimated_slippage(quantity, ticker);
            route.estimated_fees = calculate_estimated_fees(route.price * quantity, route.exchange_id);
            route.estimated_execution_time = std::chrono::milliseconds(1000); // Default 1s
        }
        
        return best_route;
    }
    
    std::vector<RouteResult> find_arbitrage_opportunities_impl(const std::string& symbol, double min_profit_bps) {
        std::vector<RouteResult> opportunities;
        auto best_prices = get_best_prices_impl(symbol);
        
        if (best_prices.size() < 2) {
            return opportunities;
        }
        
        auto best_bid = best_prices["best_bid"];
        auto best_ask = best_prices["best_ask"];
        
        if (best_bid.bid_price > best_ask.ask_price) {
            double profit_bps = ((best_bid.bid_price - best_ask.ask_price) / best_ask.ask_price) * 10000;
            
            if (profit_bps >= min_profit_bps) {
                RouteResult arb_opportunity;
                arb_opportunity.exchange_id = "arbitrage";
                arb_opportunity.price = best_bid.bid_price - best_ask.ask_price;
                arb_opportunity.estimated_slippage = 0.0;
                arb_opportunity.estimated_fees = calculate_estimated_fees(10000, ""); // $10k example
                arb_opportunity.estimated_execution_time = std::chrono::milliseconds(2000);
                
                opportunities.push_back(arb_opportunity);
                
                HFX_LOG_INFO("[ExchangeManager] Arbitrage opportunity found: " + 
                           std::to_string(profit_bps) + " bps profit for " + symbol);
            }
        }
        
        return opportunities;
    }
    
    std::string place_smart_order_impl(const std::string& symbol, OrderType type, OrderSide side,
                                     double quantity, double price, const std::string& preferred_exchange) {
        if (!check_risk_limits_impl(symbol, side, quantity, price)) {
            HFX_LOG_ERROR("[ExchangeManager] Order rejected by risk limits");
            return "";
        }
        
        std::string target_exchange = preferred_exchange;
        
        // Smart routing if enabled and no preference specified
        if (smart_routing_enabled_ && target_exchange.empty()) {
            auto best_route = find_best_execution_route_impl(symbol, side, quantity);
            target_exchange = best_route.exchange_id;
        }
        
        // Fallback to first available exchange
        if (target_exchange.empty()) {
            std::lock_guard<std::mutex> lock(manager_mutex_);
            for (const auto& exchange_pair : exchanges_) {
                if (exchange_pair.second->is_connected()) {
                    target_exchange = exchange_pair.first;
                    break;
                }
            }
        }
        
        if (target_exchange.empty()) {
            HFX_LOG_ERROR("[ExchangeManager] No available exchanges for order execution");
            return "";
        }
        
        // Execute order
        std::lock_guard<std::mutex> lock(manager_mutex_);
        auto exchange_it = exchanges_.find(target_exchange);
        if (exchange_it == exchanges_.end() || !exchange_it->second->is_connected()) {
            HFX_LOG_ERROR("[ExchangeManager] Target exchange not available: " + target_exchange);
            return "";
        }
        
        std::string order_id = exchange_it->second->place_order(symbol, type, side, quantity, price);
        
        if (!order_id.empty()) {
            // Update statistics
            update_exchange_stats(target_exchange, quantity * price);
            
            // Track order
            Order order;
            order.id = order_id;
            order.symbol = symbol;
            order.type = type;
            order.side = side;
            order.quantity = quantity;
            order.price = price;
            order.status = OrderStatus::PENDING;
            order.exchange_id = target_exchange;
            order.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
            
            active_orders_[order_id] = order;
            
            HFX_LOG_INFO("[ExchangeManager] Order placed on " + target_exchange + ": " + order_id);
        }
        
        return order_id;
    }
    
    bool check_risk_limits_impl(const std::string& symbol, OrderSide side, double quantity, double price) {
        double order_value_usd = quantity * price;
        
        // Check order size limit
        if (order_value_usd > risk_limits_.max_order_size_usd) {
            HFX_LOG_WARN("[ExchangeManager] Order size exceeds limit: $" + std::to_string(order_value_usd));
            return false;
        }
        
        // Check blacklisted symbols
        if (std::find(risk_limits_.blacklisted_symbols.begin(), 
                     risk_limits_.blacklisted_symbols.end(), symbol) != risk_limits_.blacklisted_symbols.end()) {
            HFX_LOG_WARN("[ExchangeManager] Symbol is blacklisted: " + symbol);
            return false;
        }
        
        // Check daily volume limit (simplified)
        double total_daily_volume = 0.0;
        for (const auto& stats : exchange_stats_) {
            total_daily_volume += stats.second.total_volume_usd.load();
        }
        
        if (total_daily_volume + order_value_usd > risk_limits_.max_daily_volume_usd) {
            HFX_LOG_WARN("[ExchangeManager] Daily volume limit would be exceeded");
            return false;
        }
        
        return true;
    }
    
    void update_exchange_stats(const std::string& exchange_id, double order_value_usd) {
        auto stats_it = exchange_stats_.find(exchange_id);
        if (stats_it != exchange_stats_.end()) {
            stats_it->second.total_orders.fetch_add(1);
            stats_it->second.total_volume_usd.fetch_add(order_value_usd);
            stats_it->second.last_order_time = std::chrono::system_clock::now();
        }
    }
    
    double calculate_estimated_slippage(double quantity, const MarketData& ticker) {
        // Simple slippage estimation based on spread
        double spread = ticker.ask_price - ticker.bid_price;
        double mid_price = (ticker.ask_price + ticker.bid_price) / 2.0;
        
        if (mid_price > 0) {
            double spread_bps = (spread / mid_price) * 10000;
            // Assume slippage scales with quantity
            return spread_bps * std::min(1.0, quantity / 1000.0);
        }
        
        return default_slippage_tolerance_;
    }
    
    double calculate_estimated_fees(double order_value_usd, const std::string& exchange_id) {
        // Get exchange-specific fees
        std::lock_guard<std::mutex> lock(manager_mutex_);
        
        auto config_it = exchange_configs_.find(exchange_id);
        if (config_it != exchange_configs_.end()) {
            return order_value_usd * config_it->second.taker_fee; // Use taker fee as default
        }
        
        return order_value_usd * 0.001; // Default 0.1% fee
    }
    
    std::unordered_map<std::string, double> get_aggregated_portfolio_impl() {
        std::unordered_map<std::string, double> portfolio;
        std::vector<std::future<std::vector<Balance>>> futures;
        
        {
            std::lock_guard<std::mutex> lock(manager_mutex_);
            
            for (const auto& exchange_pair : exchanges_) {
                if (!exchange_pair.second->is_connected()) {
                    continue;
                }
                
                futures.push_back(std::async(std::launch::async, [&exchange_pair]() {
                    try {
                        return exchange_pair.second->get_account_balance();
                    } catch (const std::exception& e) {
                        HFX_LOG_ERROR("[ExchangeManager] Balance fetch failed: " + std::string(e.what()));
                        return std::vector<Balance>{};
                    }
                }));
            }
        }
        
        // Aggregate balances
        for (auto& future : futures) {
            try {
                auto balances = future.get();
                for (const auto& balance : balances) {
                    portfolio[balance.asset] += balance.total;
                }
            } catch (const std::exception& e) {
                HFX_LOG_ERROR("[ExchangeManager] Portfolio aggregation failed: " + std::string(e.what()));
            }
        }
        
        return portfolio;
    }
    
    std::vector<Order> get_all_open_orders_impl() {
        std::vector<Order> all_orders;
        std::vector<std::future<std::vector<Order>>> futures;
        
        {
            std::lock_guard<std::mutex> lock(manager_mutex_);
            
            for (const auto& exchange_pair : exchanges_) {
                if (!exchange_pair.second->is_connected()) {
                    continue;
                }
                
                futures.push_back(std::async(std::launch::async, [&exchange_pair]() {
                    try {
                        return exchange_pair.second->get_open_orders();
                    } catch (const std::exception& e) {
                        HFX_LOG_ERROR("[ExchangeManager] Open orders fetch failed: " + std::string(e.what()));
                        return std::vector<Order>{};
                    }
                }));
            }
        }
        
        // Collect all orders
        for (auto& future : futures) {
            try {
                auto orders = future.get();
                all_orders.insert(all_orders.end(), orders.begin(), orders.end());
            } catch (const std::exception& e) {
                HFX_LOG_ERROR("[ExchangeManager] Order collection failed: " + std::string(e.what()));
            }
        }
        
        return all_orders;
    }
};

// ExchangeManager public interface implementation
ExchangeManager::ExchangeManager() : pimpl_(std::make_unique<ExchangeManagerImpl>()) {
    HFX_LOG_INFO("[ExchangeManager] Exchange manager initialized");
}

ExchangeManager::~ExchangeManager() = default;

bool ExchangeManager::add_exchange(std::unique_ptr<BaseExchange> exchange, const ExchangeConfig& config) {
    return pimpl_->add_exchange_impl(std::move(exchange), config);
}

bool ExchangeManager::remove_exchange(const std::string& exchange_id) {
    return pimpl_->remove_exchange_impl(exchange_id);
}

std::vector<std::string> ExchangeManager::get_available_exchanges() const {
    std::lock_guard<std::mutex> lock(pimpl_->manager_mutex_);
    
    std::vector<std::string> exchanges;
    for (const auto& exchange_pair : pimpl_->exchanges_) {
        exchanges.push_back(exchange_pair.first);
    }
    
    return exchanges;
}

BaseExchange* ExchangeManager::get_exchange(const std::string& exchange_id) {
    std::lock_guard<std::mutex> lock(pimpl_->manager_mutex_);
    
    auto it = pimpl_->exchanges_.find(exchange_id);
    return (it != pimpl_->exchanges_.end()) ? it->second.get() : nullptr;
}

std::vector<MarketData> ExchangeManager::get_aggregated_ticker(const std::string& symbol) {
    return pimpl_->get_aggregated_ticker_impl(symbol);
}

std::unordered_map<std::string, MarketData> ExchangeManager::get_best_prices(const std::string& symbol) {
    return pimpl_->get_best_prices_impl(symbol);
}

std::vector<OrderBook> ExchangeManager::get_aggregated_order_books(const std::string& symbol) {
    std::vector<OrderBook> order_books;
    
    std::lock_guard<std::mutex> lock(pimpl_->manager_mutex_);
    
    for (const auto& exchange_pair : pimpl_->exchanges_) {
        if (!exchange_pair.second->is_connected()) {
            continue;
        }
        
        try {
            auto order_book = exchange_pair.second->get_order_book(symbol);
            if (!order_book.bids.empty() || !order_book.asks.empty()) {
                order_books.push_back(order_book);
            }
        } catch (const std::exception& e) {
            HFX_LOG_ERROR("[ExchangeManager] Order book fetch failed for " + 
                         exchange_pair.first + ": " + e.what());
        }
    }
    
    return order_books;
}

ExchangeManager::RouteResult ExchangeManager::find_best_execution_route(const std::string& symbol, OrderSide side, double quantity) {
    return pimpl_->find_best_execution_route_impl(symbol, side, quantity);
}

std::vector<ExchangeManager::RouteResult> ExchangeManager::find_arbitrage_opportunities(const std::string& symbol, double min_profit_bps) {
    return pimpl_->find_arbitrage_opportunities_impl(symbol, min_profit_bps);
}

std::string ExchangeManager::place_smart_order(const std::string& symbol, OrderType type, OrderSide side,
                                             double quantity, double price, const std::string& preferred_exchange) {
    return pimpl_->place_smart_order_impl(symbol, type, side, quantity, price, preferred_exchange);
}

bool ExchangeManager::cancel_order_on_exchange(const std::string& exchange_id, const std::string& order_id) {
    std::lock_guard<std::mutex> lock(pimpl_->manager_mutex_);
    
    auto exchange_it = pimpl_->exchanges_.find(exchange_id);
    if (exchange_it == pimpl_->exchanges_.end()) {
        return false;
    }
    
    bool success = exchange_it->second->cancel_order(order_id);
    
    if (success) {
        // Update tracked orders
        auto order_it = pimpl_->active_orders_.find(order_id);
        if (order_it != pimpl_->active_orders_.end()) {
            order_it->second.status = OrderStatus::CANCELLED;
        }
        
        HFX_LOG_INFO("[ExchangeManager] Order cancelled: " + order_id);
    }
    
    return success;
}

std::vector<Order> ExchangeManager::get_all_open_orders() {
    return pimpl_->get_all_open_orders_impl();
}

std::unordered_map<std::string, double> ExchangeManager::get_aggregated_portfolio() {
    return pimpl_->get_aggregated_portfolio_impl();
}

double ExchangeManager::get_total_portfolio_value_usd() {
    auto portfolio = get_aggregated_portfolio();
    double total_value = 0.0;
    
    // Simple USD calculation - would use real price feeds in production
    for (const auto& holding : portfolio) {
        if (holding.first == "USD" || holding.first == "USDT" || holding.first == "USDC") {
            total_value += holding.second;
        } else if (holding.first == "BTC") {
            total_value += holding.second * 45000.0; // Placeholder BTC price
        } else if (holding.first == "ETH") {
            total_value += holding.second * 3000.0;  // Placeholder ETH price
        } else {
            total_value += holding.second * 1.0;     // Default $1 for unknown assets
        }
    }
    
    return total_value;
}

void ExchangeManager::set_risk_limits(const RiskLimits& limits) {
    std::lock_guard<std::mutex> lock(pimpl_->manager_mutex_);
    pimpl_->risk_limits_ = limits;
    HFX_LOG_INFO("[ExchangeManager] Risk limits updated");
}

bool ExchangeManager::check_risk_limits(const std::string& symbol, OrderSide side, double quantity, double price) {
    return pimpl_->check_risk_limits_impl(symbol, side, quantity, price);
}

ExchangeManager::ExchangeStats ExchangeManager::get_exchange_stats(const std::string& exchange_id) const {
    std::lock_guard<std::mutex> lock(pimpl_->manager_mutex_);
    
    auto it = pimpl_->exchange_stats_.find(exchange_id);
    return (it != pimpl_->exchange_stats_.end()) ? it->second : ExchangeStats{};
}

std::unordered_map<std::string, ExchangeManager::ExchangeStats> ExchangeManager::get_all_exchange_stats() const {
    std::lock_guard<std::mutex> lock(pimpl_->manager_mutex_);
    return pimpl_->exchange_stats_;
}

void ExchangeManager::set_default_slippage_tolerance(double slippage_bps) {
    pimpl_->default_slippage_tolerance_ = slippage_bps;
}

void ExchangeManager::set_execution_timeout(std::chrono::milliseconds timeout) {
    pimpl_->execution_timeout_ = timeout;
}

void ExchangeManager::enable_smart_routing(bool enabled) {
    pimpl_->smart_routing_enabled_ = enabled;
    HFX_LOG_INFO("[ExchangeManager] Smart routing " + std::string(enabled ? "enabled" : "disabled"));
}

void ExchangeManager::enable_arbitrage_detection(bool enabled) {
    pimpl_->arbitrage_detection_enabled_ = enabled;
    HFX_LOG_INFO("[ExchangeManager] Arbitrage detection " + std::string(enabled ? "enabled" : "disabled"));
}

// ExchangeFactory implementation
std::vector<std::string> ExchangeFactory::get_supported_exchanges() {
    return {"binance", "coinbase", "uniswap_v3", "raydium", "jupiter"};
}

bool ExchangeFactory::is_exchange_supported(const std::string& exchange_id) {
    auto supported = get_supported_exchanges();
    return std::find(supported.begin(), supported.end(), exchange_id) != supported.end();
}

// Additional factory methods would be implemented similarly to binance and uniswap
std::unique_ptr<BaseExchange> ExchangeFactory::create_coinbase_exchange(const ExchangeConfig& config) {
    // TODO: Implement Coinbase exchange
    HFX_LOG_WARN("[ExchangeFactory] Coinbase exchange not yet implemented");
    return nullptr;
}

std::unique_ptr<BaseExchange> ExchangeFactory::create_raydium_exchange(const ExchangeConfig& config) {
    // TODO: Implement Raydium DEX exchange
    HFX_LOG_WARN("[ExchangeFactory] Raydium exchange not yet implemented");
    return nullptr;
}

std::unique_ptr<BaseExchange> ExchangeFactory::create_jupiter_exchange(const ExchangeConfig& config) {
    // TODO: Implement Jupiter aggregator exchange
    HFX_LOG_WARN("[ExchangeFactory] Jupiter exchange not yet implemented");
    return nullptr;
}

} // namespace hfx::exchange
