# Core Engine APIs

Ultra-low latency core components for event processing, networking, and system management.

---

## Event Engine (`hfx::core::EventEngine`)

Ultra-low latency event processing system with lock-free queues and time wheels.

### Class Interface

```cpp
#include "hfx-core/include/event_engine.hpp"

namespace hfx::core {

class EventEngine {
public:
    // Lifecycle Management
    bool initialize(const EventConfig& config = {});
    bool start();
    void stop();
    bool is_running() const;
    
    // Event Processing
    template<typename EventType>
    bool publish_event(EventType&& event, Priority priority = Priority::NORMAL);
    
    template<typename EventType, typename Handler>
    SubscriptionId subscribe(Handler&& handler, EventFilter filter = {});
    
    bool unsubscribe(SubscriptionId id);
    
    // Batch Processing
    template<typename EventType>
    bool publish_batch(const std::vector<EventType>& events);
    
    // Performance Monitoring
    EventStats get_statistics() const;
    void reset_statistics();
    
    // Advanced Configuration
    void set_event_batching(bool enabled, size_t batch_size = 1000);
    void configure_thread_affinity(const std::vector<int>& cpu_cores);
    void set_queue_capacity(size_t capacity);
};

} // namespace hfx::core
```

### Configuration

```cpp
struct EventConfig {
    size_t worker_threads = 4;
    size_t queue_capacity = 1000000;
    bool enable_batching = true;
    size_t batch_size = 1000;
    std::chrono::microseconds batch_timeout{100};
    bool enable_numa_optimization = true;
    std::vector<int> cpu_affinity;
};
```

### Event Types

```cpp
enum class EventType {
    MARKET_DATA_UPDATE,
    ORDER_EXECUTION,
    TRADE_SIGNAL,
    RISK_ALERT,
    MEV_OPPORTUNITY,
    SYSTEM_HEALTH,
    USER_ACTION,
    STRATEGY_UPDATE
};

enum class Priority {
    LOW = 0,
    NORMAL = 1,
    HIGH = 2,
    CRITICAL = 3
};
```

### Usage Examples

#### Basic Event Publishing

```cpp
#include "event_engine.hpp"

EventEngine engine;
EventConfig config;
config.worker_threads = 8;
config.enable_numa_optimization = true;

engine.initialize(config);
engine.start();

// Publish market data event
MarketDataEvent event;
event.symbol = "ETH/USDC";
event.price = 1500.25;
event.timestamp = std::chrono::high_resolution_clock::now();

bool success = engine.publish_event(std::move(event), Priority::HIGH);
```

#### Event Subscription

```cpp
// Subscribe to market data events
auto sub_id = engine.subscribe<MarketDataEvent>(
    [](const MarketDataEvent& event) {
        // Process with sub-microsecond latency
        strategy_engine.process_price_update(event.symbol, event.price);
    },
    EventFilter{.symbols = {"ETH/USDC", "BTC/USDT"}}
);

// Subscribe to trade execution events
engine.subscribe<TradeExecutionEvent>(
    [](const TradeExecutionEvent& event) {
        portfolio_manager.update_position(event);
        risk_manager.assess_exposure(event);
    }
);
```

#### Batch Processing

```cpp
// High-throughput batch processing
std::vector<MarketDataEvent> batch;
batch.reserve(1000);

// Collect events
for (const auto& update : market_updates) {
    batch.push_back(create_market_event(update));
}

// Publish entire batch atomically
engine.publish_batch(batch);
```

### Performance Monitoring

```cpp
struct EventStats {
    std::atomic<uint64_t> events_published{0};
    std::atomic<uint64_t> events_processed{0};
    std::atomic<uint64_t> events_dropped{0};
    std::atomic<double> avg_processing_time_ns{0.0};
    std::atomic<double> max_processing_time_ns{0.0};
    std::atomic<uint64_t> queue_size{0};
    std::atomic<uint64_t> subscriptions_active{0};
};

// Monitor performance
auto stats = engine.get_statistics();
std::cout << "Events/sec: " << stats.events_processed.load() << std::endl;
std::cout << "Avg latency: " << stats.avg_processing_time_ns.load() << "ns" << std::endl;
```

---

## Network Manager (`hfx::net::NetworkManager`)

High-performance networking with WebSocket and REST API support.

### Class Interface

```cpp
#include "hfx-net/include/network_manager.hpp"

namespace hfx::net {

class NetworkManager {
public:
    // Lifecycle
    bool initialize(const NetworkConfig& config);
    bool start();
    void stop();
    
    // Connection Management
    ConnectionId connect_websocket(const std::string& url, 
                                  const WebSocketConfig& ws_config = {});
    ConnectionId connect_rest(const std::string& base_url, 
                             const RestConfig& rest_config = {});
    
    bool disconnect(ConnectionId conn_id);
    bool is_connected(ConnectionId conn_id) const;
    
    // Data Transmission
    template<typename MessageType>
    bool send_message(ConnectionId conn_id, const MessageType& message);
    
    template<typename ResponseType>
    std::future<ResponseType> send_request(ConnectionId conn_id, 
                                          const Request& request);
    
    // Real-time Streaming
    template<typename DataType>
    void subscribe_stream(ConnectionId conn_id, const std::string& channel,
                         std::function<void(const DataType&)> callback);
    
    // Performance Optimization
    void enable_tcp_nodelay(ConnectionId conn_id);
    void set_receive_buffer_size(ConnectionId conn_id, size_t size);
    void configure_connection_pooling(size_t max_connections);
    
    // Statistics
    NetworkStats get_statistics() const;
    ConnectionStats get_connection_stats(ConnectionId conn_id) const;
};

} // namespace hfx::net
```

### Configuration

```cpp
struct NetworkConfig {
    size_t io_threads = 4;
    size_t max_connections = 1000;
    std::chrono::milliseconds connect_timeout{5000};
    std::chrono::milliseconds read_timeout{30000};
    bool enable_tcp_nodelay = true;
    size_t receive_buffer_size = 65536;
    bool enable_compression = false;
    std::string user_agent = "HydraFlow-X/1.0";
};

struct WebSocketConfig {
    std::map<std::string, std::string> headers;
    std::vector<std::string> protocols;
    bool enable_ping_pong = true;
    std::chrono::seconds ping_interval{30};
    size_t max_message_size = 1048576; // 1MB
};
```

### Usage Examples

#### WebSocket Real-time Data

```cpp
NetworkManager net_mgr;
NetworkConfig config;
config.io_threads = 8;
config.enable_tcp_nodelay = true;

net_mgr.initialize(config);
net_mgr.start();

// Connect to Binance WebSocket
auto binance_ws = net_mgr.connect_websocket(
    "wss://stream.binance.com:9443/ws/ethusdt@ticker"
);

// Subscribe to ticker data
net_mgr.subscribe_stream<TickerData>(binance_ws, "ethusdt@ticker", 
    [](const TickerData& data) {
        // Process with < 100μs latency
        trading_engine.process_ticker(data);
        strategy_engine.evaluate_signals(data);
    });
```

#### REST API Calls

```cpp
// Connect to exchange REST API
auto coinbase_rest = net_mgr.connect_rest("https://api.coinbase.com/v2");

// Async order placement
RestRequest order_request;
order_request.method = "POST";
order_request.endpoint = "/orders";
order_request.body = create_order_json("ETH-USD", "buy", 1000.0);

auto future_response = net_mgr.send_request<OrderResponse>(
    coinbase_rest, order_request
);

// Handle response
future_response.then([](const OrderResponse& response) {
    if (response.success) {
        order_manager.track_order(response.order_id);
    }
});
```

#### High-Frequency Trading Setup

```cpp
// Optimize for ultra-low latency
net_mgr.enable_tcp_nodelay(connection_id);
net_mgr.set_receive_buffer_size(connection_id, 1048576); // 1MB buffer

// Multiple exchange connections
auto exchanges = {
    net_mgr.connect_websocket("wss://ws.kraken.com"),
    net_mgr.connect_websocket("wss://stream.binance.com:9443/ws"),
    net_mgr.connect_websocket("wss://ws-feed.coinbase.com")
};

// Aggregate market data
for (auto exchange_id : exchanges) {
    net_mgr.subscribe_stream<MarketData>(exchange_id, "ETHUSD", 
        [](const MarketData& data) {
            arbitrage_engine.process_price(data);
        });
}
```

---

## Logger (`hfx::log::Logger`)

High-performance structured logging system.

### Class Interface

```cpp
#include "hfx-log/include/logger.hpp"

namespace hfx::log {

class Logger {
public:
    // Lifecycle
    bool initialize(const LogConfig& config);
    void shutdown();
    
    // Logging Methods
    template<typename... Args>
    void debug(const std::string& message, Args&&... args);
    
    template<typename... Args>
    void info(const std::string& message, Args&&... args);
    
    template<typename... Args>
    void warn(const std::string& message, Args&&... args);
    
    template<typename... Args>
    void error(const std::string& message, Args&&... args);
    
    template<typename... Args>
    void critical(const std::string& message, Args&&... args);
    
    // Structured Logging
    void log_trade_execution(const TradeData& trade);
    void log_performance_metric(const std::string& metric, double value);
    void log_security_event(const SecurityEvent& event);
    
    // Configuration
    void set_log_level(LogLevel level);
    void add_appender(std::unique_ptr<LogAppender> appender);
    
    // Statistics
    LogStats get_statistics() const;
};

} // namespace hfx::log
```

### Usage Examples

#### Basic Logging

```cpp
#include "logger.hpp"

Logger logger;
LogConfig config;
config.level = LogLevel::INFO;
config.enable_console = true;
config.enable_file = true;
config.file_path = "/var/log/hydraflow-x/app.log";

logger.initialize(config);

// Structured logging with context
logger.info("Trade executed successfully", 
    "symbol", "ETH/USDC",
    "price", 1500.25,
    "quantity", 10.0,
    "latency_ns", 45000);

// Performance-critical logging
logger.log_trade_execution({
    .symbol = "ETH/USDC",
    .price = 1500.25,
    .quantity = 10.0,
    .execution_time = std::chrono::high_resolution_clock::now(),
    .latency = std::chrono::nanoseconds(45000)
});
```

#### Async Logging for High-Frequency Trading

```cpp
// Configure for minimal latency impact
LogConfig hft_config;
hft_config.async_logging = true;
hft_config.buffer_size = 1048576; // 1MB buffer
hft_config.flush_interval = std::chrono::milliseconds(100);

logger.initialize(hft_config);

// Log without blocking trading thread
logger.info("Order placed", "order_id", order.id, "timestamp_ns", 
           std::chrono::high_resolution_clock::now().time_since_epoch().count());
```

---

## REST API Server (`hfx::api::RestApiServer`)

High-performance REST API server with authentication and rate limiting.

### Class Interface

```cpp
#include "hfx-api/include/rest_api_server.hpp"

namespace hfx::api {

class RestApiServer {
public:
    // Lifecycle
    bool initialize(const ApiConfig& config);
    bool start();
    void stop();
    
    // Route Registration
    template<typename Handler>
    void register_route(HttpMethod method, const std::string& path, Handler handler);
    
    void register_middleware(std::unique_ptr<Middleware> middleware);
    
    // WebSocket Support
    void enable_websocket(const std::string& path, 
                         std::unique_ptr<WebSocketHandler> handler);
    
    // Statistics
    ApiStats get_statistics() const;
    
private:
    void handle_client_connection(std::shared_ptr<ClientConnection> connection);
    HttpResponse create_json_response(const json& data, int status_code = 200);
};

} // namespace hfx::api
```

### Usage Examples

#### API Route Registration

```cpp
RestApiServer api_server;
ApiConfig config;
config.port = 8080;
config.num_threads = 16;
config.enable_cors = true;

api_server.initialize(config);

// Trading endpoints
api_server.register_route(HttpMethod::POST, "/api/v1/trades", 
    [&](const HttpRequest& req) -> HttpResponse {
        auto trade_request = parse_trade_request(req.body);
        
        // Validate authentication
        if (!auth_service.validate_session(req.headers["Authorization"])) {
            return create_error_response(401, "Unauthorized");
        }
        
        // Execute trade
        auto result = trading_engine.execute_trade(trade_request);
        return create_json_response(result.to_json());
    });

// Portfolio endpoint
api_server.register_route(HttpMethod::GET, "/api/v1/portfolio",
    [&](const HttpRequest& req) -> HttpResponse {
        auto user_id = auth_service.get_user_from_session(req.headers["Authorization"]);
        auto portfolio = portfolio_service.get_portfolio(user_id);
        return create_json_response(portfolio.to_json());
    });

api_server.start();
```

#### WebSocket Real-time Updates

```cpp
// Enable WebSocket for real-time data
api_server.enable_websocket("/ws/stream", 
    std::make_unique<TradingWebSocketHandler>());

class TradingWebSocketHandler : public WebSocketHandler {
public:
    void on_connect(WebSocketConnection& conn) override {
        // Authenticate WebSocket connection
        auto session = extract_session_from_query(conn.get_query_params());
        if (auth_service.validate_session(session)) {
            active_connections_.insert(&conn);
        } else {
            conn.close(4001, "Unauthorized");
        }
    }
    
    void on_message(WebSocketConnection& conn, const std::string& message) override {
        auto request = json::parse(message);
        
        if (request["action"] == "subscribe") {
            // Subscribe to real-time updates
            for (const auto& channel : request["channels"]) {
                subscribe_channel(conn, channel);
            }
        }
    }
    
    void broadcast_trade_update(const TradeUpdate& update) {
        auto message = update.to_json().dump();
        for (auto* conn : active_connections_) {
            conn->send(message);
        }
    }
};
```

---

## Performance Considerations

### Event Engine Optimization

- **Lock-free Queues**: SPSC and MPMC queues for zero-allocation event processing
- **NUMA Awareness**: Thread and memory locality optimization
- **CPU Affinity**: Pin worker threads to specific CPU cores
- **Batch Processing**: Reduce syscall overhead with event batching

### Network Optimization

- **TCP_NODELAY**: Disable Nagle's algorithm for low latency
- **Large Buffers**: Optimize buffer sizes for high-throughput scenarios
- **Connection Pooling**: Reuse connections to reduce setup overhead
- **IO Threads**: Dedicated threads for network I/O operations

### Memory Management

- **Memory Pools**: Pre-allocated memory for frequent allocations
- **NUMA-Aware Allocation**: Allocate memory on the same NUMA node as the thread
- **Stack Allocation**: Prefer stack allocation for small, short-lived objects
- **Cache-line Alignment**: Align data structures to cache boundaries

---

**[← Back to API Overview](./API_Overview.md) | [Next: Ultra-Low Latency APIs →](./Ultra_Latency_APIs.md)**
