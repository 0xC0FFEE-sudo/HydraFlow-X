/**
 * @file coinbase_exchange.cpp
 * @brief Coinbase Pro exchange integration implementation
 */

#include "../include/coinbase_exchange.hpp"
#include "../../hfx-log/include/simple_logger.hpp"
#include <curl/curl.h>
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include "../../utils/include/simple_json.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <random>
#include <algorithm>

namespace hfx::exchanges {

// HTTP response structure for curl
struct HTTPResponse {
    std::string data;
    long response_code = 0;
    
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, HTTPResponse* response) {
        size_t total_size = size * nmemb;
        response->data.append(static_cast<char*>(contents), total_size);
        return total_size;
    }
};

CoinbaseExchange::CoinbaseExchange(const CoinbaseConfig& config) 
    : config_(config) {
    
    // Initialize curl
    curl_global_init(CURL_GLOBAL_DEFAULT);
    
    // Set initial rate limit reset time
    rate_limit_reset_time_ = std::chrono::system_clock::now() + std::chrono::seconds(1);
    rate_limit_.store(config.rate_limit_per_second);
    
    HFX_LOG_INFO("CoinbaseExchange initialized");
}

CoinbaseExchange::~CoinbaseExchange() {
    disconnect();
    curl_global_cleanup();
}

bool CoinbaseExchange::connect() {
    if (connected_.load()) {
        return true;
    }
    
    HFX_LOG_INFO("Connecting to Coinbase Pro...");
    
    // Test connection with a simple API call
    auto response = make_rest_request("GET", "/time");
    if (response.empty()) {
        HFX_LOG_ERROR("Failed to connect to Coinbase Pro API");
        return false;
    }
    
    connected_.store(true);
    running_.store(true);
    
    // Start WebSocket thread for real-time data
    websocket_thread_ = std::make_unique<std::thread>(&CoinbaseExchange::websocket_loop, this);
    
    HFX_LOG_INFO("Successfully connected to Coinbase Pro");
    return true;
}

void CoinbaseExchange::disconnect() {
    if (!connected_.load()) {
        return;
    }
    
    HFX_LOG_INFO("Disconnecting from Coinbase Pro...");
    
    running_.store(false);
    connected_.store(false);
    
    if (websocket_thread_ && websocket_thread_->joinable()) {
        websocket_thread_->join();
    }
    
    HFX_LOG_INFO("Disconnected from Coinbase Pro");
}

bool CoinbaseExchange::is_connected() const {
    return connected_.load();
}

bool CoinbaseExchange::subscribe_order_book(const std::string& symbol, OrderBookCallback callback) {
    if (!connected_.load()) {
        return false;
    }
    
    orderbook_callbacks_[symbol] = callback;
    HFX_LOG_INFO("Subscribed to order book for " + symbol);
    return true;
}

bool CoinbaseExchange::subscribe_ticker(const std::string& symbol, TickerCallback callback) {
    if (!connected_.load()) {
        return false;
    }
    
    ticker_callbacks_[symbol] = callback;
    HFX_LOG_INFO("Subscribed to ticker for " + symbol);
    return true;
}

bool CoinbaseExchange::subscribe_trades(const std::string& symbol, TradeCallback callback) {
    if (!connected_.load()) {
        return false;
    }
    
    trade_callbacks_[symbol] = callback;
    HFX_LOG_INFO("Subscribed to trades for " + symbol);
    return true;
}

OrderResponse CoinbaseExchange::place_order(const OrderRequest& request) {
    OrderResponse response;
    
    if (!connected_.load()) {
        response.error_message = "Not connected to exchange";
        return response;
    }
    
    if (!check_rate_limit()) {
        response.error_message = "Rate limit exceeded";
        return response;
    }
    
    // Build order JSON
    hfx::utils::JsonValue order_json;
    order_json["type"] = hfx::utils::JsonValue(request.type);
    order_json["side"] = hfx::utils::JsonValue(request.side);
    order_json["product_id"] = hfx::utils::JsonValue(request.symbol);
    order_json["size"] = hfx::utils::JsonValue(std::to_string(request.size));
    
    if (request.type == "limit") {
        order_json["price"] = hfx::utils::JsonValue(std::to_string(request.price));
    }
    
    if (!request.client_oid.empty()) {
        order_json["client_oid"] = hfx::utils::JsonValue(request.client_oid);
    }
    
    std::string order_body = hfx::utils::SimpleJson::stringify(order_json);
    
    // Make API call
    std::string api_response = make_rest_request("POST", "/orders", order_body);
    
    if (api_response.empty()) {
        response.error_message = "API request failed";
        return response;
    }
    
    // Parse response
    response = parse_order_response(api_response);
    
    HFX_LOG_INFO("Order placed: " + response.order_id);
    return response;
}

bool CoinbaseExchange::cancel_order(const std::string& order_id) {
    if (!connected_.load() || !check_rate_limit()) {
        return false;
    }
    
    std::string endpoint = "/orders/" + order_id;
    std::string response = make_rest_request("DELETE", endpoint);
    
    HFX_LOG_INFO("Order cancelled: " + order_id);
    return !response.empty();
}

OrderResponse CoinbaseExchange::get_order_status(const std::string& order_id) {
    OrderResponse response;
    
    if (!connected_.load() || !check_rate_limit()) {
        response.error_message = "Cannot check order status";
        return response;
    }
    
    std::string endpoint = "/orders/" + order_id;
    std::string api_response = make_rest_request("GET", endpoint);
    
    if (!api_response.empty()) {
        response = parse_order_response(api_response);
    }
    
    return response;
}

std::vector<Balance> CoinbaseExchange::get_balances() {
    std::vector<Balance> balances;
    
    if (!connected_.load() || !check_rate_limit()) {
        return balances;
    }
    
    std::string response = make_rest_request("GET", "/accounts");
    if (!response.empty()) {
        balances = parse_balances(response);
    }
    
    return balances;
}

double CoinbaseExchange::get_balance(const std::string& currency) {
    auto balances = get_balances();
    auto it = std::find_if(balances.begin(), balances.end(),
        [&currency](const Balance& b) { return b.currency == currency; });
    
    return (it != balances.end()) ? it->available : 0.0;
}

std::vector<std::string> CoinbaseExchange::get_symbols() {
    std::vector<std::string> symbols;
    
    if (!connected_.load() || !check_rate_limit()) {
        return symbols;
    }
    
    std::string response = make_rest_request("GET", "/products");
    if (!response.empty()) {
        // Parse JSON response to extract symbols (simplified)
        hfx::utils::JsonValue root = hfx::utils::SimpleJson::parse(response);
        if (root.isArray()) {
            for (size_t i = 0; i < root.size(); ++i) {
                auto product = root[i];
                if (product.isMember("id")) {
                    symbols.push_back(product["id"].asString());
                }
            }
        }
    }
    
    return symbols;
}

Ticker CoinbaseExchange::get_ticker(const std::string& symbol) {
    Ticker ticker;
    ticker.symbol = symbol;
    
    if (!connected_.load() || !check_rate_limit()) {
        return ticker;
    }
    
    std::string endpoint = "/products/" + symbol + "/ticker";
    std::string response = make_rest_request("GET", endpoint);
    
    if (!response.empty()) {
        ticker = parse_ticker(response);
    }
    
    return ticker;
}

OrderBook CoinbaseExchange::get_order_book(const std::string& symbol, int level) {
    OrderBook book;
    book.symbol = symbol;
    
    if (!connected_.load() || !check_rate_limit()) {
        return book;
    }
    
    std::string endpoint = "/products/" + symbol + "/book?level=" + std::to_string(level);
    std::string response = make_rest_request("GET", endpoint);
    
    if (!response.empty()) {
        book = parse_order_book(response);
    }
    
    return book;
}

void CoinbaseExchange::set_rate_limit(uint32_t requests_per_second) {
    rate_limit_.store(requests_per_second);
}

bool CoinbaseExchange::is_rate_limited() const {
    auto now = std::chrono::system_clock::now();
    if (now >= rate_limit_reset_time_) {
        return false;
    }
    return requests_made_this_second_.load() >= rate_limit_.load();
}

// Private methods

void CoinbaseExchange::websocket_loop() {
    // Simplified WebSocket implementation (would use proper WebSocket library in production)
    HFX_LOG_INFO("WebSocket loop started");
    
    while (running_.load()) {
        // Simulate receiving market data
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // Generate sample ticker data for subscribed symbols
        for (const auto& [symbol, callback] : ticker_callbacks_) {
            Ticker ticker;
            ticker.symbol = symbol;
            ticker.price = 50000.0 + (rand() % 1000 - 500); // Sample BTC price
            ticker.bid = ticker.price - 0.5;
            ticker.ask = ticker.price + 0.5;
            ticker.volume = 1000.0 + (rand() % 100);
            ticker.time = std::chrono::system_clock::now();
            
            callback(ticker);
        }
    }
    
    HFX_LOG_INFO("WebSocket loop stopped");
}

std::string CoinbaseExchange::make_rest_request(const std::string& method, 
                                               const std::string& endpoint, 
                                               const std::string& body) {
    CURL* curl = curl_easy_init();
    HTTPResponse response;
    
    if (!curl) {
        return "";
    }
    
    std::string url = config_.base_url + endpoint;
    std::string timestamp = get_timestamp();
    
    // Generate signature
    std::string signature = generate_signature(timestamp, method, endpoint, body);
    
    // Set headers
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, ("CB-ACCESS-KEY: " + config_.api_key).c_str());
    headers = curl_slist_append(headers, ("CB-ACCESS-SIGN: " + signature).c_str());
    headers = curl_slist_append(headers, ("CB-ACCESS-TIMESTAMP: " + timestamp).c_str());
    headers = curl_slist_append(headers, ("CB-ACCESS-PASSPHRASE: " + config_.passphrase).c_str());
    headers = curl_slist_append(headers, "Content-Type: application/json");
    
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, HTTPResponse::WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
    
    if (method == "POST") {
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
    } else if (method == "DELETE") {
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
    }
    
    CURLcode res = curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response.response_code);
    
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    
    // Update rate limiting
    auto now = std::chrono::system_clock::now();
    if (now >= rate_limit_reset_time_) {
        requests_made_this_second_.store(1);
        rate_limit_reset_time_ = now + std::chrono::seconds(1);
    } else {
        requests_made_this_second_.fetch_add(1);
    }
    
    if (res != CURLE_OK || response.response_code >= 400) {
        HFX_LOG_ERROR("HTTP request failed");
        return "";
    }
    
    return response.data;
}

std::string CoinbaseExchange::generate_signature(const std::string& timestamp,
                                                const std::string& method,
                                                const std::string& request_path,
                                                const std::string& body) {
    std::string message = timestamp + method + request_path + body;
    
    // Decode base64 secret
    BIO* b64 = BIO_new(BIO_f_base64());
    BIO* mem = BIO_new_mem_buf(config_.api_secret.c_str(), -1);
    BIO_push(b64, mem);
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    
    char decoded_key[256];
    int decoded_len = BIO_read(b64, decoded_key, sizeof(decoded_key));
    BIO_free_all(b64);
    
    // Generate HMAC
    unsigned char hmac_result[EVP_MAX_MD_SIZE];
    unsigned int hmac_len;
    
    HMAC(EVP_sha256(), decoded_key, decoded_len, 
         reinterpret_cast<const unsigned char*>(message.c_str()), 
         message.length(), hmac_result, &hmac_len);
    
    // Encode to base64
    BIO* b64_out = BIO_new(BIO_f_base64());
    BIO* mem_out = BIO_new(BIO_s_mem());
    BIO_push(b64_out, mem_out);
    BIO_set_flags(b64_out, BIO_FLAGS_BASE64_NO_NL);
    
    BIO_write(b64_out, hmac_result, hmac_len);
    BIO_flush(b64_out);
    
    BUF_MEM* buffer;
    BIO_get_mem_ptr(b64_out, &buffer);
    
    std::string signature(buffer->data, buffer->length);
    BIO_free_all(b64_out);
    
    return signature;
}

bool CoinbaseExchange::check_rate_limit() {
    auto now = std::chrono::system_clock::now();
    if (now >= rate_limit_reset_time_) {
        requests_made_this_second_.store(0);
        rate_limit_reset_time_ = now + std::chrono::seconds(1);
        return true;
    }
    
    return requests_made_this_second_.load() < rate_limit_.load();
}

std::string CoinbaseExchange::get_timestamp() {
    auto now = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()
    );
    return std::to_string(now.count());
}

// Parsing methods (simplified implementations)
OrderBook CoinbaseExchange::parse_order_book(const std::string& json) {
    OrderBook book;
    hfx::utils::JsonValue root = hfx::utils::SimpleJson::parse(json);
    
    if (root.isMember("sequence")) {
        book.sequence = root["sequence"].asUInt64();
    }
    
    return book;
}

Ticker CoinbaseExchange::parse_ticker(const std::string& json) {
    Ticker ticker;
    hfx::utils::JsonValue root = hfx::utils::SimpleJson::parse(json);
    
    ticker.price = root.get("price", hfx::utils::JsonValue("0")).asDouble();
    ticker.bid = root.get("bid", hfx::utils::JsonValue("0")).asDouble();
    ticker.ask = root.get("ask", hfx::utils::JsonValue("0")).asDouble();
    ticker.volume = root.get("volume", hfx::utils::JsonValue("0")).asDouble();
    
    return ticker;
}

Trade CoinbaseExchange::parse_trade(const std::string& json) {
    Trade trade;
    // Simplified parsing implementation
    return trade;
}

OrderResponse CoinbaseExchange::parse_order_response(const std::string& json) {
    OrderResponse response;
    hfx::utils::JsonValue root = hfx::utils::SimpleJson::parse(json);
    
    response.order_id = root.get("id", hfx::utils::JsonValue("")).asString();
    response.status = root.get("status", hfx::utils::JsonValue("")).asString();
    response.success = !response.order_id.empty();
    
    if (root.isMember("filled_size")) {
        response.filled_size = root["filled_size"].asDouble();
    }
    
    return response;
}

std::vector<Balance> CoinbaseExchange::parse_balances(const std::string& json) {
    std::vector<Balance> balances;
    hfx::utils::JsonValue root = hfx::utils::SimpleJson::parse(json);
    
    if (root.isArray()) {
        for (size_t i = 0; i < root.size(); ++i) {
            auto account = root[i];
            Balance balance;
            balance.currency = account.get("currency", hfx::utils::JsonValue("")).asString();
            balance.balance = account.get("balance", hfx::utils::JsonValue("0")).asDouble();
            balance.available = account.get("available", hfx::utils::JsonValue("0")).asDouble();
            balance.hold = account.get("hold", hfx::utils::JsonValue("0")).asDouble();
            balances.push_back(balance);
        }
    }
    
    return balances;
}

} // namespace hfx::exchanges
