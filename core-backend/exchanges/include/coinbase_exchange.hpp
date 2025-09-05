/**
 * @file coinbase_exchange.hpp
 * @brief Coinbase Pro exchange integration
 */

#pragma once

#include <string>
#include <memory>
#include <functional>
#include <atomic>
#include <thread>
#include <queue>
#include <unordered_map>
#include <chrono>

namespace hfx::exchanges {

struct CoinbaseConfig {
    std::string api_key;
    std::string api_secret;
    std::string passphrase;
    std::string base_url = "https://api.exchange.coinbase.com";
    bool sandbox_mode = false;
    uint32_t rate_limit_per_second = 10;
};

struct OrderBookEntry {
    double price = 0.0;
    double size = 0.0;
    std::string order_id;
};

struct OrderBook {
    std::string symbol;
    std::vector<OrderBookEntry> bids;
    std::vector<OrderBookEntry> asks;
    uint64_t sequence = 0;
    std::chrono::system_clock::time_point timestamp;
};

struct Ticker {
    std::string symbol;
    double price = 0.0;
    double size = 0.0;
    double bid = 0.0;
    double ask = 0.0;
    double volume = 0.0;
    std::chrono::system_clock::time_point time;
};

struct Trade {
    std::string trade_id;
    std::string symbol;
    double price = 0.0;
    double size = 0.0;
    std::string side; // "buy" or "sell"
    std::chrono::system_clock::time_point time;
};

struct OrderRequest {
    std::string symbol;
    std::string side; // "buy" or "sell"
    std::string type; // "market" or "limit"
    double size = 0.0;
    double price = 0.0; // Only for limit orders
    std::string client_oid; // Optional client order ID
};

struct OrderResponse {
    std::string order_id;
    bool success = false;
    std::string error_message;
    std::string status; // "pending", "open", "done", "rejected"
    double filled_size = 0.0;
    double filled_price = 0.0;
};

struct Balance {
    std::string currency;
    double balance = 0.0;
    double available = 0.0;
    double hold = 0.0;
};

class CoinbaseExchange {
public:
    using OrderBookCallback = std::function<void(const OrderBook&)>;
    using TickerCallback = std::function<void(const Ticker&)>;
    using TradeCallback = std::function<void(const Trade&)>;
    using OrderUpdateCallback = std::function<void(const OrderResponse&)>;

    explicit CoinbaseExchange(const CoinbaseConfig& config);
    ~CoinbaseExchange();

    // Connection management
    bool connect();
    void disconnect();
    bool is_connected() const;

    // Market data
    bool subscribe_order_book(const std::string& symbol, OrderBookCallback callback);
    bool subscribe_ticker(const std::string& symbol, TickerCallback callback);
    bool subscribe_trades(const std::string& symbol, TradeCallback callback);
    
    bool unsubscribe_order_book(const std::string& symbol);
    bool unsubscribe_ticker(const std::string& symbol);
    bool unsubscribe_trades(const std::string& symbol);

    // Trading
    OrderResponse place_order(const OrderRequest& request);
    bool cancel_order(const std::string& order_id);
    OrderResponse get_order_status(const std::string& order_id);
    
    // Account
    std::vector<Balance> get_balances();
    double get_balance(const std::string& currency);
    
    // Market info
    std::vector<std::string> get_symbols();
    Ticker get_ticker(const std::string& symbol);
    OrderBook get_order_book(const std::string& symbol, int level = 2);

    // Rate limiting
    void set_rate_limit(uint32_t requests_per_second);
    bool is_rate_limited() const;

private:
    CoinbaseConfig config_;
    std::atomic<bool> connected_{false};
    std::atomic<bool> running_{false};
    
    // WebSocket connection for real-time data
    std::unique_ptr<std::thread> websocket_thread_;
    
    // REST API rate limiting
    std::atomic<uint32_t> requests_made_this_second_{0};
    std::chrono::system_clock::time_point rate_limit_reset_time_;
    std::atomic<uint32_t> rate_limit_{10};
    
    // Callbacks
    std::unordered_map<std::string, OrderBookCallback> orderbook_callbacks_;
    std::unordered_map<std::string, TickerCallback> ticker_callbacks_;
    std::unordered_map<std::string, TradeCallback> trade_callbacks_;
    OrderUpdateCallback order_update_callback_;
    
    // Internal methods
    void websocket_loop();
    std::string make_rest_request(const std::string& method, 
                                 const std::string& endpoint, 
                                 const std::string& body = "");
    std::string generate_signature(const std::string& timestamp,
                                  const std::string& method,
                                  const std::string& request_path,
                                  const std::string& body);
    bool check_rate_limit();
    void process_websocket_message(const std::string& message);
    
    // Utility methods
    std::string get_timestamp();
    OrderBook parse_order_book(const std::string& json);
    Ticker parse_ticker(const std::string& json);
    Trade parse_trade(const std::string& json);
    OrderResponse parse_order_response(const std::string& json);
    std::vector<Balance> parse_balances(const std::string& json);
};

} // namespace hfx::exchanges
