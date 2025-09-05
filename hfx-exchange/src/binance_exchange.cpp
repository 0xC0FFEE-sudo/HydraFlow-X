/**
 * @file binance_exchange.cpp
 * @brief Binance exchange integration implementation
 */

#include "../include/exchange_manager.hpp"
#include "../../hfx-log/include/simple_logger.hpp"
#include <curl/curl.h>
#include <openssl/hmac.h>
#include <openssl/sha.h>
#include <thread>
#include <sstream>
#include <iomanip>
#include <regex>
#include <chrono>
#include <atomic>

namespace hfx::exchange {

// Helper functions
namespace {
    size_t write_callback(void* contents, size_t size, size_t nmemb, std::string* s) {
        size_t newLength = size * nmemb;
        s->append((char*)contents, newLength);
        return newLength;
    }
    
    std::string hmac_sha256(const std::string& key, const std::string& data) {
        unsigned char hash[32];
        unsigned int hash_len;
        
        HMAC_CTX* hmac = HMAC_CTX_new();
        HMAC_Init_ex(hmac, key.c_str(), key.length(), EVP_sha256(), nullptr);
        HMAC_Update(hmac, reinterpret_cast<const unsigned char*>(data.c_str()), data.length());
        HMAC_Final(hmac, hash, &hash_len);
        HMAC_CTX_free(hmac);
        
        std::stringstream ss;
        for (unsigned int i = 0; i < hash_len; i++) {
            ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
        }
        
        return ss.str();
    }
    
    uint64_t get_current_timestamp_ms() {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    }
    
    std::string extract_json_string(const std::string& json, const std::string& key) {
        std::regex pattern("\"" + key + "\"\\s*:\\s*\"([^\"]+)\"");
        std::smatch match;
        if (std::regex_search(json, match, pattern)) {
            return match[1].str();
        }
        return "";
    }
    
    double extract_json_double(const std::string& json, const std::string& key) {
        std::regex pattern("\"" + key + "\"\\s*:\\s*\"?([0-9.]+)\"?");
        std::smatch match;
        if (std::regex_search(json, match, pattern)) {
            return std::stod(match[1].str());
        }
        return 0.0;
    }
    
    uint64_t extract_json_uint64(const std::string& json, const std::string& key) {
        std::regex pattern("\"" + key + "\"\\s*:\\s*([0-9]+)");
        std::smatch match;
        if (std::regex_search(json, match, pattern)) {
            return std::stoull(match[1].str());
        }
        return 0;
    }
}

class BinanceExchange : public BaseExchange {
private:
    ExchangeConfig config_;
    CURL* curl_;
    std::atomic<bool> connected_{false};
    std::atomic<bool> websocket_connected_{false};
    mutable std::mutex request_mutex_;
    std::thread websocket_thread_;
    std::atomic<bool> websocket_running_{false};
    
    // Rate limiting
    std::atomic<int> requests_this_second_{0};
    std::atomic<int> orders_this_second_{0};
    std::chrono::steady_clock::time_point last_rate_limit_reset_;
    
    // Callbacks
    std::function<void(const MarketData&)> ticker_callback_;
    std::function<void(const OrderBook&)> orderbook_callback_;
    std::function<void(const Trade&)> trade_callback_;
    std::function<void(const Order&)> order_callback_;
    
public:
    explicit BinanceExchange(const ExchangeConfig& config) : config_(config) {
        curl_ = curl_easy_init();
        if (!curl_) {
            throw std::runtime_error("Failed to initialize CURL for Binance");
        }
        
        // Setup CURL options
        curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl_, CURLOPT_TIMEOUT_MS, config_.request_timeout.count());
        curl_easy_setopt(curl_, CURLOPT_CONNECTTIMEOUT_MS, 5000);
        curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYPEER, 1L);
        curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYHOST, 2L);
        curl_easy_setopt(curl_, CURLOPT_USERAGENT, "HydraFlow-X-Binance/1.0");
        
        last_rate_limit_reset_ = std::chrono::steady_clock::now();
        
        HFX_LOG_INFO("[BinanceExchange] Initialized with API endpoint: " + config_.base_url);
    }
    
    ~BinanceExchange() {
        disconnect();
        if (curl_) {
            curl_easy_cleanup(curl_);
        }
    }
    
    bool connect() override {
        if (connected_) {
            return true;
        }
        
        // Test connection with server time
        std::string response = make_public_request("GET", "/api/v3/time", "");
        if (response.empty()) {
            HFX_LOG_ERROR("[BinanceExchange] Failed to connect to Binance API");
            return false;
        }
        
        connected_ = true;
        
        // Start WebSocket connection if enabled
        if (config_.enable_websocket) {
            start_websocket();
        }
        
        HFX_LOG_INFO("[BinanceExchange] Connected successfully");
        return true;
    }
    
    void disconnect() override {
        connected_ = false;
        stop_websocket();
        HFX_LOG_INFO("[BinanceExchange] Disconnected");
    }
    
    bool is_connected() const override {
        return connected_;
    }
    
    std::string get_exchange_id() const override {
        return "binance";
    }
    
    ExchangeType get_exchange_type() const override {
        return ExchangeType::CENTRALIZED;
    }
    
    ExchangeCapabilities get_capabilities() const override {
        ExchangeCapabilities caps;
        caps.supports_spot_trading = true;
        caps.supports_futures_trading = true;
        caps.supports_margin_trading = true;
        caps.supports_websocket = true;
        caps.supports_order_book = true;
        caps.supports_klines = true;
        caps.supports_account_info = true;
        caps.supported_order_types = {OrderType::MARKET, OrderType::LIMIT, OrderType::STOP_LOSS, 
                                     OrderType::TAKE_PROFIT, OrderType::OCO};
        caps.supported_intervals = {"1m", "3m", "5m", "15m", "30m", "1h", "2h", "4h", "6h", "8h", "12h", "1d", "3d", "1w", "1M"};
        caps.rate_limit_requests_per_second = 20;
        caps.rate_limit_orders_per_second = 10;
        return caps;
    }
    
    std::vector<TradingPair> get_trading_pairs() override {
        std::vector<TradingPair> pairs;
        
        std::string response = make_public_request("GET", "/api/v3/exchangeInfo", "");
        if (response.empty()) {
            return pairs;
        }
        
        // Parse trading pairs from JSON (simplified)
        std::regex symbol_pattern("\"symbol\"\\s*:\\s*\"([^\"]+)\"");
        std::regex base_pattern("\"baseAsset\"\\s*:\\s*\"([^\"]+)\"");
        std::regex quote_pattern("\"quoteAsset\"\\s*:\\s*\"([^\"]+)\"");
        std::regex status_pattern("\"status\"\\s*:\\s*\"([^\"]+)\"");
        
        std::sregex_iterator symbol_iter(response.begin(), response.end(), symbol_pattern);
        std::sregex_iterator base_iter(response.begin(), response.end(), base_pattern);
        std::sregex_iterator quote_iter(response.begin(), response.end(), quote_pattern);
        std::sregex_iterator status_iter(response.begin(), response.end(), status_pattern);
        std::sregex_iterator end;
        
        while (symbol_iter != end && base_iter != end && quote_iter != end && status_iter != end) {
            TradingPair pair;
            pair.symbol = symbol_iter->str(1);
            pair.base_asset = base_iter->str(1);
            pair.quote_asset = quote_iter->str(1);
            pair.is_active = (status_iter->str(1) == "TRADING");
            
            // Set default values (would parse from filters in real implementation)
            pair.min_quantity = 0.001;
            pair.max_quantity = 10000000.0;
            pair.tick_size = 0.01;
            pair.step_size = 0.001;
            pair.supported_order_types = {OrderType::MARKET, OrderType::LIMIT, OrderType::STOP_LOSS};
            
            pairs.push_back(pair);
            
            ++symbol_iter;
            ++base_iter;
            ++quote_iter;
            ++status_iter;
        }
        
        HFX_LOG_INFO("[BinanceExchange] Retrieved " + std::to_string(pairs.size()) + " trading pairs");
        return pairs;
    }
    
    MarketData get_ticker(const std::string& symbol) override {
        MarketData data;
        data.symbol = symbol;
        
        std::string params = "symbol=" + symbol;
        std::string response = make_public_request("GET", "/api/v3/ticker/24hr", params);
        
        if (!response.empty()) {
            data.last_price = extract_json_double(response, "lastPrice");
            data.bid_price = extract_json_double(response, "bidPrice");
            data.ask_price = extract_json_double(response, "askPrice");
            data.volume_24h = extract_json_double(response, "volume");
            data.price_change_24h = extract_json_double(response, "priceChange");
            data.price_change_percent_24h = extract_json_double(response, "priceChangePercent");
            data.high_24h = extract_json_double(response, "highPrice");
            data.low_24h = extract_json_double(response, "lowPrice");
            data.timestamp = get_current_timestamp_ms();
        }
        
        return data;
    }
    
    OrderBook get_order_book(const std::string& symbol, int depth) override {
        OrderBook book;
        book.symbol = symbol;
        
        std::string params = "symbol=" + symbol + "&limit=" + std::to_string(depth);
        std::string response = make_public_request("GET", "/api/v3/depth", params);
        
        if (!response.empty()) {
            book.timestamp = get_current_timestamp_ms();
            book.last_update_id = extract_json_uint64(response, "lastUpdateId");
            
            // Parse bids and asks (simplified)
            parse_order_book_entries(response, "bids", book.bids);
            parse_order_book_entries(response, "asks", book.asks);
        }
        
        return book;
    }
    
    std::vector<Trade> get_recent_trades(const std::string& symbol, int limit) override {
        std::vector<Trade> trades;
        
        std::string params = "symbol=" + symbol + "&limit=" + std::to_string(limit);
        std::string response = make_public_request("GET", "/api/v3/trades", params);
        
        if (!response.empty()) {
            // Parse trades (simplified implementation)
            std::regex trade_pattern(R"(\{"id":([0-9]+),"price":"([^"]+)","qty":"([^"]+)","quoteQty":"[^"]+","time":([0-9]+),"isBuyerMaker":(true|false),"isBestMatch":(true|false)\})");
            std::sregex_iterator iter(response.begin(), response.end(), trade_pattern);
            std::sregex_iterator end;
            
            for (; iter != end; ++iter) {
                Trade trade;
                trade.id = iter->str(1);
                trade.symbol = symbol;
                trade.price = std::stod(iter->str(2));
                trade.quantity = std::stod(iter->str(3));
                trade.timestamp = std::stoull(iter->str(4));
                trade.side = (iter->str(5) == "true") ? OrderSide::SELL : OrderSide::BUY; // isBuyerMaker means seller initiated
                trade.is_maker = (iter->str(5) == "true");
                
                trades.push_back(trade);
            }
        }
        
        return trades;
    }
    
    std::string place_order(const std::string& symbol, OrderType type, OrderSide side,
                          double quantity, double price) override {
        if (!check_rate_limits()) {
            HFX_LOG_WARN("[BinanceExchange] Rate limit exceeded for order placement");
            return "";
        }
        
        std::stringstream params;
        params << "symbol=" << symbol;
        params << "&side=" << (side == OrderSide::BUY ? "BUY" : "SELL");
        params << "&type=" << order_type_to_string(type);
        params << "&quantity=" << std::fixed << std::setprecision(8) << quantity;
        
        if (type == OrderType::LIMIT) {
            params << "&price=" << std::fixed << std::setprecision(8) << price;
            params << "&timeInForce=GTC"; // Good Till Cancelled
        }
        
        params << "&timestamp=" << get_current_timestamp_ms();
        
        std::string response = make_private_request("POST", "/api/v3/order", params.str());
        
        if (!response.empty()) {
            std::string order_id = extract_json_string(response, "orderId");
            if (!order_id.empty()) {
                HFX_LOG_INFO("[BinanceExchange] Order placed successfully: " + order_id);
                return order_id;
            }
        }
        
        HFX_LOG_ERROR("[BinanceExchange] Failed to place order for " + symbol);
        return "";
    }
    
    bool cancel_order(const std::string& order_id, const std::string& symbol) override {
        if (!check_rate_limits()) {
            return false;
        }
        
        std::stringstream params;
        params << "symbol=" << symbol;
        params << "&orderId=" << order_id;
        params << "&timestamp=" << get_current_timestamp_ms();
        
        std::string response = make_private_request("DELETE", "/api/v3/order", params.str());
        
        if (!response.empty()) {
            std::string status = extract_json_string(response, "status");
            bool success = (status == "CANCELED");
            
            if (success) {
                HFX_LOG_INFO("[BinanceExchange] Order cancelled successfully: " + order_id);
            }
            
            return success;
        }
        
        return false;
    }
    
    Order get_order_status(const std::string& order_id, const std::string& symbol) override {
        Order order;
        order.id = order_id;
        order.symbol = symbol;
        
        std::stringstream params;
        params << "symbol=" << symbol;
        params << "&orderId=" << order_id;
        params << "&timestamp=" << get_current_timestamp_ms();
        
        std::string response = make_private_request("GET", "/api/v3/order", params.str());
        
        if (!response.empty()) {
            parse_order_from_json(response, order);
        }
        
        return order;
    }
    
    std::vector<Order> get_open_orders(const std::string& symbol) override {
        std::vector<Order> orders;
        
        std::stringstream params;
        if (!symbol.empty()) {
            params << "symbol=" << symbol << "&";
        }
        params << "timestamp=" << get_current_timestamp_ms();
        
        std::string response = make_private_request("GET", "/api/v3/openOrders", params.str());
        
        if (!response.empty()) {
            parse_orders_from_json(response, orders);
        }
        
        return orders;
    }
    
    std::vector<Order> get_order_history(const std::string& symbol, int limit) override {
        std::vector<Order> orders;
        
        std::stringstream params;
        params << "symbol=" << symbol;
        params << "&limit=" << limit;
        params << "&timestamp=" << get_current_timestamp_ms();
        
        std::string response = make_private_request("GET", "/api/v3/allOrders", params.str());
        
        if (!response.empty()) {
            parse_orders_from_json(response, orders);
        }
        
        return orders;
    }
    
    std::vector<Balance> get_account_balance() override {
        std::vector<Balance> balances;
        
        std::string params = "timestamp=" + std::to_string(get_current_timestamp_ms());
        std::string response = make_private_request("GET", "/api/v3/account", params);
        
        if (!response.empty()) {
            // Parse balances from JSON (simplified)
            std::regex balance_pattern(R"(\{"asset":"([^"]+)","free":"([^"]+)","locked":"([^"]+)"\})");
            std::sregex_iterator iter(response.begin(), response.end(), balance_pattern);
            std::sregex_iterator end;
            
            for (; iter != end; ++iter) {
                Balance balance;
                balance.asset = iter->str(1);
                balance.free = std::stod(iter->str(2));
                balance.locked = std::stod(iter->str(3));
                balance.total = balance.free + balance.locked;
                
                if (balance.total > 0.0) { // Only include non-zero balances
                    balances.push_back(balance);
                }
            }
        }
        
        return balances;
    }
    
    std::vector<Trade> get_trade_history(const std::string& symbol, int limit) override {
        std::vector<Trade> trades;
        
        std::stringstream params;
        params << "symbol=" << symbol;
        params << "&limit=" << limit;
        params << "&timestamp=" << get_current_timestamp_ms();
        
        std::string response = make_private_request("GET", "/api/v3/myTrades", params.str());
        
        if (!response.empty()) {
            parse_trades_from_json(response, trades);
        }
        
        return trades;
    }
    
    // WebSocket subscription methods (simplified implementations)
    bool subscribe_ticker(const std::string& symbol, std::function<void(const MarketData&)> callback) override {
        ticker_callback_ = callback;
        // In real implementation, would send WebSocket subscription message
        HFX_LOG_INFO("[BinanceExchange] Subscribed to ticker updates for " + symbol);
        return websocket_connected_;
    }
    
    bool subscribe_order_book(const std::string& symbol, std::function<void(const OrderBook&)> callback) override {
        orderbook_callback_ = callback;
        HFX_LOG_INFO("[BinanceExchange] Subscribed to orderbook updates for " + symbol);
        return websocket_connected_;
    }
    
    bool subscribe_trades(const std::string& symbol, std::function<void(const Trade&)> callback) override {
        trade_callback_ = callback;
        HFX_LOG_INFO("[BinanceExchange] Subscribed to trade updates for " + symbol);
        return websocket_connected_;
    }
    
    bool subscribe_user_data(std::function<void(const Order&)> order_callback,
                           std::function<void(const Trade&)> trade_callback) override {
        order_callback_ = order_callback;
        trade_callback_ = trade_callback;
        HFX_LOG_INFO("[BinanceExchange] Subscribed to user data updates");
        return websocket_connected_;
    }
    
    void update_config(const ExchangeConfig& config) override {
        config_ = config;
        HFX_LOG_INFO("[BinanceExchange] Configuration updated");
    }
    
    ExchangeConfig get_config() const override {
        return config_;
    }

private:
    std::string make_public_request(const std::string& method, const std::string& endpoint, const std::string& params) {
        std::lock_guard<std::mutex> lock(request_mutex_);
        
        std::string url = config_.base_url + endpoint;
        if (!params.empty()) {
            url += "?" + params;
        }
        
        return execute_request(method, url, "");
    }
    
    std::string make_private_request(const std::string& method, const std::string& endpoint, const std::string& params) {
        std::lock_guard<std::mutex> lock(request_mutex_);
        
        if (config_.api_key.empty() || config_.api_secret.empty()) {
            HFX_LOG_ERROR("[BinanceExchange] API credentials not configured for private request");
            return "";
        }
        
        // Create signature
        std::string signature = hmac_sha256(config_.api_secret, params);
        std::string signed_params = params + "&signature=" + signature;
        
        std::string url = config_.base_url + endpoint;
        if (method == "GET" || method == "DELETE") {
            url += "?" + signed_params;
        }
        
        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, ("X-MBX-APIKEY: " + config_.api_key).c_str());
        
        curl_easy_setopt(curl_, CURLOPT_HTTPHEADER, headers);
        
        std::string response;
        if (method == "POST") {
            curl_easy_setopt(curl_, CURLOPT_POSTFIELDS, signed_params.c_str());
            response = execute_request("POST", url, "");
        } else {
            response = execute_request(method, url, "");
        }
        
        curl_slist_free_all(headers);
        return response;
    }
    
    std::string execute_request(const std::string& method, const std::string& url, const std::string& payload) {
        std::string response;
        
        curl_easy_setopt(curl_, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl_, CURLOPT_WRITEDATA, &response);
        
        if (method == "POST") {
            curl_easy_setopt(curl_, CURLOPT_POST, 1L);
            if (!payload.empty()) {
                curl_easy_setopt(curl_, CURLOPT_POSTFIELDS, payload.c_str());
            }
        } else if (method == "DELETE") {
            curl_easy_setopt(curl_, CURLOPT_CUSTOMREQUEST, "DELETE");
        }
        
        CURLcode res = curl_easy_perform(curl_);
        
        if (res != CURLE_OK) {
            HFX_LOG_ERROR("[BinanceExchange] Request failed: " + std::string(curl_easy_strerror(res)));
            return "";
        }
        
        long response_code;
        curl_easy_getinfo(curl_, CURLINFO_RESPONSE_CODE, &response_code);
        
        if (response_code != 200) {
            HFX_LOG_ERROR("[BinanceExchange] HTTP error " + std::to_string(response_code) + ": " + response);
            return "";
        }
        
        return response;
    }
    
    bool check_rate_limits() {
        auto now = std::chrono::steady_clock::now();
        
        // Reset counters every second
        if (now - last_rate_limit_reset_ >= std::chrono::seconds(1)) {
            requests_this_second_ = 0;
            orders_this_second_ = 0;
            last_rate_limit_reset_ = now;
        }
        
        int current_requests = requests_this_second_.fetch_add(1);
        
        if (current_requests >= get_capabilities().rate_limit_requests_per_second) {
            return false;
        }
        
        return true;
    }
    
    std::string order_type_to_string(OrderType type) {
        switch (type) {
            case OrderType::MARKET: return "MARKET";
            case OrderType::LIMIT: return "LIMIT";
            case OrderType::STOP_LOSS: return "STOP_LOSS";
            case OrderType::TAKE_PROFIT: return "TAKE_PROFIT";
            case OrderType::OCO: return "OCO";
            default: return "LIMIT";
        }
    }
    
    void parse_order_book_entries(const std::string& json, const std::string& type, std::vector<OrderBookEntry>& entries) {
        std::string pattern_str = "\"" + type + "\"\\s*:\\s*\\[(.*?)\\]";
        std::regex pattern(pattern_str);
        std::smatch match;
        
        if (std::regex_search(json, match, pattern)) {
            std::string entries_str = match[1].str();
            std::regex entry_pattern(R"(\["([^"]+)","([^"]+)"\])");
            std::sregex_iterator iter(entries_str.begin(), entries_str.end(), entry_pattern);
            std::sregex_iterator end;
            
            for (; iter != end; ++iter) {
                OrderBookEntry entry;
                entry.price = std::stod(iter->str(1));
                entry.quantity = std::stod(iter->str(2));
                entries.push_back(entry);
            }
        }
    }
    
    void parse_order_from_json(const std::string& json, Order& order) {
        order.symbol = extract_json_string(json, "symbol");
        order.id = extract_json_string(json, "orderId");
        order.client_order_id = extract_json_string(json, "clientOrderId");
        order.price = extract_json_double(json, "price");
        order.quantity = extract_json_double(json, "origQty");
        order.filled_quantity = extract_json_double(json, "executedQty");
        order.remaining_quantity = order.quantity - order.filled_quantity;
        order.timestamp = extract_json_uint64(json, "time");
        order.update_time = extract_json_uint64(json, "updateTime");
        
        std::string status = extract_json_string(json, "status");
        order.status = parse_order_status(status);
        
        std::string side = extract_json_string(json, "side");
        order.side = (side == "BUY") ? OrderSide::BUY : OrderSide::SELL;
        
        std::string type = extract_json_string(json, "type");
        order.type = parse_order_type(type);
    }
    
    void parse_orders_from_json(const std::string& json, std::vector<Order>& orders) {
        // Parse array of orders (simplified)
        std::regex order_pattern(R"(\{[^}]*"orderId":"([^"]+)"[^}]*\})");
        std::sregex_iterator iter(json.begin(), json.end(), order_pattern);
        std::sregex_iterator end;
        
        for (; iter != end; ++iter) {
            Order order;
            parse_order_from_json(iter->str(), order);
            orders.push_back(order);
        }
    }
    
    void parse_trades_from_json(const std::string& json, std::vector<Trade>& trades) {
        // Parse array of trades (simplified)
        std::regex trade_pattern(R"(\{[^}]*"id":"([^"]+)"[^}]*\})");
        std::sregex_iterator iter(json.begin(), json.end(), trade_pattern);
        std::sregex_iterator end;
        
        for (; iter != end; ++iter) {
            Trade trade;
            // Parse individual trade fields
            trade.id = extract_json_string(iter->str(), "id");
            trade.price = extract_json_double(iter->str(), "price");
            trade.quantity = extract_json_double(iter->str(), "qty");
            trade.timestamp = extract_json_uint64(iter->str(), "time");
            trade.commission = extract_json_double(iter->str(), "commission");
            trade.commission_asset = extract_json_string(iter->str(), "commissionAsset");
            
            std::string is_buyer = extract_json_string(iter->str(), "isBuyer");
            trade.side = (is_buyer == "true") ? OrderSide::BUY : OrderSide::SELL;
            
            std::string is_maker = extract_json_string(iter->str(), "isMaker");
            trade.is_maker = (is_maker == "true");
            
            trades.push_back(trade);
        }
    }
    
    OrderStatus parse_order_status(const std::string& status) {
        if (status == "NEW") return OrderStatus::OPEN;
        else if (status == "PARTIALLY_FILLED") return OrderStatus::PARTIALLY_FILLED;
        else if (status == "FILLED") return OrderStatus::FILLED;
        else if (status == "CANCELED") return OrderStatus::CANCELLED;
        else if (status == "REJECTED") return OrderStatus::REJECTED;
        else if (status == "EXPIRED") return OrderStatus::EXPIRED;
        else return OrderStatus::PENDING;
    }
    
    OrderType parse_order_type(const std::string& type) {
        if (type == "MARKET") return OrderType::MARKET;
        else if (type == "LIMIT") return OrderType::LIMIT;
        else if (type == "STOP_LOSS") return OrderType::STOP_LOSS;
        else if (type == "TAKE_PROFIT") return OrderType::TAKE_PROFIT;
        else if (type == "OCO") return OrderType::OCO;
        else return OrderType::LIMIT;
    }
    
    void start_websocket() {
        if (websocket_running_) return;
        
        websocket_running_ = true;
        websocket_connected_ = true;
        websocket_thread_ = std::thread(&BinanceExchange::websocket_worker, this);
        HFX_LOG_INFO("[BinanceExchange] WebSocket thread started");
    }
    
    void stop_websocket() {
        if (websocket_running_) {
            websocket_running_ = false;
            websocket_connected_ = false;
            
            if (websocket_thread_.joinable()) {
                websocket_thread_.join();
            }
            
            HFX_LOG_INFO("[BinanceExchange] WebSocket thread stopped");
        }
    }
    
    void websocket_worker() {
        // Simplified WebSocket worker - in production would use proper WebSocket library
        while (websocket_running_) {
            try {
                // Simulate receiving WebSocket data and invoking callbacks
                std::this_thread::sleep_for(std::chrono::seconds(1));
                
                // In real implementation, would parse WebSocket messages and invoke callbacks
                
            } catch (const std::exception& e) {
                HFX_LOG_ERROR("[BinanceExchange] WebSocket error: " + std::string(e.what()));
                std::this_thread::sleep_for(std::chrono::seconds(5));
            }
        }
    }
};

// Factory method
std::unique_ptr<BaseExchange> ExchangeFactory::create_binance_exchange(const ExchangeConfig& config) {
    ExchangeConfig binance_config = config;
    if (binance_config.base_url.empty()) {
        binance_config.base_url = binance_config.sandbox_mode 
            ? "https://testnet.binance.vision" 
            : "https://api.binance.com";
    }
    if (binance_config.websocket_url.empty()) {
        binance_config.websocket_url = "wss://stream.binance.com:9443/ws/";
    }
    
    return std::make_unique<BinanceExchange>(binance_config);
}

} // namespace hfx::exchange
