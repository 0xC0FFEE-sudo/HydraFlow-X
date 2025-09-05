/**
 * @file exchange_manager.hpp
 * @brief Unified exchange API manager for CEX and DEX integrations
 * @version 1.0.0
 */

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <functional>
#include <atomic>
#include <chrono>
#include <mutex>

namespace hfx::exchange {

// Exchange types
enum class ExchangeType {
    CENTRALIZED,    // CEX (Binance, Coinbase, etc.)
    DECENTRALIZED, // DEX (Uniswap, Raydium, etc.)
    HYBRID         // Mixed functionality
};

// Order types
enum class OrderType {
    MARKET,
    LIMIT,
    STOP_LOSS,
    TAKE_PROFIT,
    OCO,           // One-Cancels-Other
    ICEBERG,
    POST_ONLY
};

// Order side
enum class OrderSide {
    BUY,
    SELL
};

// Order status
enum class OrderStatus {
    PENDING,
    OPEN,
    PARTIALLY_FILLED,
    FILLED,
    CANCELLED,
    REJECTED,
    EXPIRED
};

// Trading pair information
struct TradingPair {
    std::string symbol;           // e.g., "BTC/USDT", "ETH/SOL"
    std::string base_asset;       // e.g., "BTC", "ETH"
    std::string quote_asset;      // e.g., "USDT", "SOL"
    double min_quantity;          // Minimum order quantity
    double max_quantity;          // Maximum order quantity
    double tick_size;             // Price tick size
    double step_size;             // Quantity step size
    bool is_active;
    std::vector<OrderType> supported_order_types;
};

// Market data
struct MarketData {
    std::string symbol;
    double bid_price;
    double ask_price;
    double last_price;
    double volume_24h;
    double price_change_24h;
    double price_change_percent_24h;
    uint64_t timestamp;
    double high_24h;
    double low_24h;
};

// Order book entry
struct OrderBookEntry {
    double price;
    double quantity;
};

// Order book
struct OrderBook {
    std::string symbol;
    std::vector<OrderBookEntry> bids;
    std::vector<OrderBookEntry> asks;
    uint64_t timestamp;
    uint64_t last_update_id;
};

// Trade information
struct Trade {
    std::string id;
    std::string symbol;
    double price;
    double quantity;
    OrderSide side;
    uint64_t timestamp;
    bool is_maker;
    double commission;
    std::string commission_asset;
};

// Order information
struct Order {
    std::string id;
    std::string client_order_id;
    std::string symbol;
    OrderType type;
    OrderSide side;
    double quantity;
    double price;
    double filled_quantity;
    double remaining_quantity;
    OrderStatus status;
    uint64_t timestamp;
    uint64_t update_time;
    std::vector<Trade> fills;
    std::string exchange_id;
    
    // Advanced fields
    double stop_price;
    double iceberg_quantity;
    std::chrono::milliseconds time_in_force;
};

// Account balance
struct Balance {
    std::string asset;
    double free;
    double locked;
    double total;
};

// Exchange configuration
struct ExchangeConfig {
    std::string exchange_id;
    std::string api_key;
    std::string api_secret;
    std::string passphrase;        // For some exchanges
    std::string base_url;
    std::string websocket_url;
    bool sandbox_mode = false;
    bool enable_websocket = true;
    std::chrono::milliseconds request_timeout = std::chrono::milliseconds(10000);
    int max_retries = 3;
    std::chrono::milliseconds retry_delay = std::chrono::milliseconds(1000);
    double maker_fee = 0.001;      // 0.1%
    double taker_fee = 0.001;      // 0.1%
};

// Exchange capabilities
struct ExchangeCapabilities {
    bool supports_spot_trading = false;
    bool supports_futures_trading = false;
    bool supports_margin_trading = false;
    bool supports_options_trading = false;
    bool supports_websocket = false;
    bool supports_order_book = false;
    bool supports_klines = false;
    bool supports_account_info = false;
    std::vector<OrderType> supported_order_types;
    std::vector<std::string> supported_intervals;
    int rate_limit_requests_per_second = 10;
    int rate_limit_orders_per_second = 5;
};

// Base exchange interface
class BaseExchange {
public:
    virtual ~BaseExchange() = default;
    
    // Connection management
    virtual bool connect() = 0;
    virtual void disconnect() = 0;
    virtual bool is_connected() const = 0;
    
    // Exchange information
    virtual std::string get_exchange_id() const = 0;
    virtual ExchangeType get_exchange_type() const = 0;
    virtual ExchangeCapabilities get_capabilities() const = 0;
    virtual std::vector<TradingPair> get_trading_pairs() = 0;
    
    // Market data
    virtual MarketData get_ticker(const std::string& symbol) = 0;
    virtual OrderBook get_order_book(const std::string& symbol, int depth = 20) = 0;
    virtual std::vector<Trade> get_recent_trades(const std::string& symbol, int limit = 100) = 0;
    
    // Trading operations
    virtual std::string place_order(const std::string& symbol, OrderType type, OrderSide side,
                                  double quantity, double price = 0.0) = 0;
    virtual bool cancel_order(const std::string& order_id, const std::string& symbol = "") = 0;
    virtual Order get_order_status(const std::string& order_id, const std::string& symbol = "") = 0;
    virtual std::vector<Order> get_open_orders(const std::string& symbol = "") = 0;
    virtual std::vector<Order> get_order_history(const std::string& symbol = "", int limit = 100) = 0;
    
    // Account information
    virtual std::vector<Balance> get_account_balance() = 0;
    virtual std::vector<Trade> get_trade_history(const std::string& symbol = "", int limit = 100) = 0;
    
    // WebSocket subscriptions
    virtual bool subscribe_ticker(const std::string& symbol, std::function<void(const MarketData&)> callback) = 0;
    virtual bool subscribe_order_book(const std::string& symbol, std::function<void(const OrderBook&)> callback) = 0;
    virtual bool subscribe_trades(const std::string& symbol, std::function<void(const Trade&)> callback) = 0;
    virtual bool subscribe_user_data(std::function<void(const Order&)> order_callback,
                                   std::function<void(const Trade&)> trade_callback) = 0;
    
    // Configuration
    virtual void update_config(const ExchangeConfig& config) = 0;
    virtual ExchangeConfig get_config() const = 0;
};

// Exchange manager for coordinating multiple exchanges
class ExchangeManager {
public:
    ExchangeManager();
    ~ExchangeManager();
    
    // Exchange management
    bool add_exchange(std::unique_ptr<BaseExchange> exchange, const ExchangeConfig& config);
    bool remove_exchange(const std::string& exchange_id);
    std::vector<std::string> get_available_exchanges() const;
    BaseExchange* get_exchange(const std::string& exchange_id);
    
    // Market data aggregation
    std::vector<MarketData> get_aggregated_ticker(const std::string& symbol);
    std::unordered_map<std::string, MarketData> get_best_prices(const std::string& symbol);
    std::vector<OrderBook> get_aggregated_order_books(const std::string& symbol);
    
    // Smart routing
    struct RouteResult {
        std::string exchange_id;
        double price;
        double quantity;
        double estimated_slippage;
        double estimated_fees;
        std::chrono::milliseconds estimated_execution_time;
    };
    
    RouteResult find_best_execution_route(const std::string& symbol, OrderSide side, double quantity);
    std::vector<RouteResult> find_arbitrage_opportunities(const std::string& symbol, double min_profit_bps = 50);
    
    // Order management
    std::string place_smart_order(const std::string& symbol, OrderType type, OrderSide side,
                                double quantity, double price = 0.0, const std::string& preferred_exchange = "");
    bool cancel_order_on_exchange(const std::string& exchange_id, const std::string& order_id);
    std::vector<Order> get_all_open_orders();
    
    // Portfolio management
    std::unordered_map<std::string, double> get_aggregated_portfolio();
    double get_total_portfolio_value_usd();
    
    // Risk management
    struct RiskLimits {
        double max_order_size_usd = 10000.0;
        double max_daily_volume_usd = 100000.0;
        double max_position_size_percent = 20.0; // % of portfolio
        int max_open_orders_per_exchange = 50;
        std::vector<std::string> blacklisted_symbols;
    };
    
    void set_risk_limits(const RiskLimits& limits);
    bool check_risk_limits(const std::string& symbol, OrderSide side, double quantity, double price);
    
    // Analytics
    struct ExchangeStats {
        std::atomic<uint64_t> total_orders{0};
        std::atomic<uint64_t> successful_orders{0};
        std::atomic<uint64_t> failed_orders{0};
        std::atomic<double> total_volume_usd{0.0};
        std::atomic<double> total_fees_paid{0.0};
        std::chrono::system_clock::time_point last_order_time;
        std::unordered_map<std::string, double> symbol_volumes;
    };
    
    ExchangeStats get_exchange_stats(const std::string& exchange_id) const;
    std::unordered_map<std::string, ExchangeStats> get_all_exchange_stats() const;
    
    // Configuration
    void set_default_slippage_tolerance(double slippage_bps);
    void set_execution_timeout(std::chrono::milliseconds timeout);
    void enable_smart_routing(bool enabled);
    void enable_arbitrage_detection(bool enabled);
    
private:
    class ExchangeManagerImpl;
    std::unique_ptr<ExchangeManagerImpl> pimpl_;
};

// Factory for creating exchange instances
class ExchangeFactory {
public:
    static std::unique_ptr<BaseExchange> create_binance_exchange(const ExchangeConfig& config);
    static std::unique_ptr<BaseExchange> create_coinbase_exchange(const ExchangeConfig& config);
    static std::unique_ptr<BaseExchange> create_uniswap_exchange(const ExchangeConfig& config);
    static std::unique_ptr<BaseExchange> create_raydium_exchange(const ExchangeConfig& config);
    static std::unique_ptr<BaseExchange> create_jupiter_exchange(const ExchangeConfig& config);
    
    static std::vector<std::string> get_supported_exchanges();
    static bool is_exchange_supported(const std::string& exchange_id);
};

} // namespace hfx::exchange
