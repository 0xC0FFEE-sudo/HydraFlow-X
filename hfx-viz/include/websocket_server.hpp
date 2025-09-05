/**
 * @file websocket_server.hpp
 * @brief High-performance WebSocket server for real-time HFT data streaming
 * @version 1.0.0
 * 
 * Streams telemetry data to web dashboards with microsecond precision timestamps.
 * Supports multiple concurrent connections with adaptive bitrate streaming.
 */

#pragma once

#include "telemetry_engine.hpp"
#include <memory>
#include <string>
#include <vector>
#include <unordered_set>
#include <functional>
#include <atomic>
#include <mutex>
#include <thread>

namespace hfx::viz {

/**
 * @brief Configuration for WebSocket server
 */
struct WebSocketConfig {
    std::string bind_address = "0.0.0.0";
    uint16_t port = 8080;
    std::string document_root = "./web";  // For serving static files
    
    // Connection limits
    size_t max_connections = 100;
    size_t max_message_size = 1024 * 1024;  // 1MB
    
    // Streaming configuration
    float update_frequency_hz = 10.0f;  // 10 updates per second
    bool enable_compression = true;
    bool enable_binary_protocol = true;  // More efficient than JSON
    
    // Security
    bool enable_cors = true;
    std::string allowed_origins = "*";  // In production, specify allowed origins
    bool enable_auth = false;
    std::string auth_token = "";
    
    // Performance tuning
    size_t io_thread_pool_size = 4;
    size_t send_buffer_size = 64 * 1024;  // 64KB
    size_t receive_buffer_size = 16 * 1024;  // 16KB
};

/**
 * @brief WebSocket message types for the HFT streaming protocol
 */
enum class MessageType : uint8_t {
    // Client -> Server
    SUBSCRIBE_METRICS = 0x01,
    SUBSCRIBE_TRADES = 0x02,
    SUBSCRIBE_MARKET_DATA = 0x03,
    SUBSCRIBE_RISK_DATA = 0x04,
    UNSUBSCRIBE = 0x10,
    GET_SNAPSHOT = 0x20,
    GET_HISTORY = 0x21,
    
    // Server -> Client
    METRICS_UPDATE = 0x81,
    TRADE_UPDATE = 0x82,
    MARKET_UPDATE = 0x83,
    RISK_UPDATE = 0x84,
    SNAPSHOT_DATA = 0xA0,
    HISTORY_DATA = 0xA1,
    HEARTBEAT = 0xFF
};

/**
 * @brief Client subscription preferences
 */
struct ClientSubscription {
    bool metrics = false;
    bool trades = false;
    bool market_data = false;
    bool risk_data = false;
    float update_rate_hz = 10.0f;  // Per-client rate limiting
    std::chrono::steady_clock::time_point last_update;
};

/**
 * @brief WebSocket connection wrapper
 */
class WebSocketConnection {
public:
    using ConnectionId = uint64_t;
    
    WebSocketConnection(ConnectionId id);
    ~WebSocketConnection() = default;
    
    ConnectionId get_id() const { return id_; }
    
    bool is_connected() const { return connected_; }
    void set_connected(bool connected) { connected_ = connected; }
    
    const ClientSubscription& get_subscription() const { return subscription_; }
    void set_subscription(const ClientSubscription& sub) { subscription_ = sub; }
    
    void send_binary(const std::vector<uint8_t>& data);
    void send_text(const std::string& data);
    void send_ping();
    
    std::chrono::steady_clock::time_point get_last_activity() const { return last_activity_; }
    void update_activity() { last_activity_ = std::chrono::steady_clock::now(); }
    
    size_t get_bytes_sent() const { return bytes_sent_; }
    size_t get_bytes_received() const { return bytes_received_; }
    
private:
    ConnectionId id_;
    std::atomic<bool> connected_{false};
    ClientSubscription subscription_;
    std::chrono::steady_clock::time_point last_activity_;
    std::atomic<size_t> bytes_sent_{0};
    std::atomic<size_t> bytes_received_{0};
    
    // Connection-specific state would be stored here
    // (actual WebSocket handle, send queue, etc.)
};

/**
 * @class WebSocketServer
 * @brief High-performance WebSocket server for real-time HFT data streaming
 * 
 * Features:
 * - Ultra-low latency data streaming
 * - Binary protocol for efficiency
 * - Adaptive bitrate based on client capabilities
 * - Connection pooling and management
 * - Rate limiting and backpressure handling
 * - Comprehensive metrics and monitoring
 */
class WebSocketServer {
public:
    explicit WebSocketServer(const WebSocketConfig& config = {});
    ~WebSocketServer();
    
    // Server lifecycle
    bool start();
    void stop();
    bool is_running() const { return running_; }
    
    // Data source binding
    void set_telemetry_engine(std::shared_ptr<TelemetryEngine> telemetry);
    
    // Connection management
    size_t get_connection_count() const;
    std::vector<WebSocketConnection::ConnectionId> get_active_connections() const;
    void disconnect_client(WebSocketConnection::ConnectionId id);
    void broadcast_message(const std::vector<uint8_t>& data, MessageType type);
    
    // Statistics and monitoring
    struct ServerStats {
        size_t total_connections = 0;
        size_t active_connections = 0;
        size_t total_messages_sent = 0;
        size_t total_messages_received = 0;
        size_t total_bytes_sent = 0;
        size_t total_bytes_received = 0;
        float messages_per_second = 0.0f;
        float bytes_per_second = 0.0f;
        std::chrono::steady_clock::time_point start_time;
    };
    
    ServerStats get_stats() const { return stats_; }
    
    // Event callbacks
    using ConnectionCallback = std::function<void(WebSocketConnection::ConnectionId, bool connected)>;
    using MessageCallback = std::function<void(WebSocketConnection::ConnectionId, const std::string&)>;
    using ErrorCallback = std::function<void(const std::string& error)>;

    void set_connection_callback(ConnectionCallback callback) { connection_callback_ = callback; }
    void set_message_callback(MessageCallback callback) { message_callback_ = callback; }
    void set_error_callback(ErrorCallback callback) { error_callback_ = callback; }

    // Public accessors for event handler
    ConnectionCallback get_connection_callback() const { return connection_callback_; }
    MessageCallback get_message_callback() const { return message_callback_; }
    
private:
    WebSocketConfig config_;
    std::shared_ptr<TelemetryEngine> telemetry_;
    std::atomic<bool> running_{false};
    
    // Connection management
    std::unordered_map<WebSocketConnection::ConnectionId, std::unique_ptr<WebSocketConnection>> connections_;
    mutable std::mutex connections_mutex_;
    std::atomic<WebSocketConnection::ConnectionId> next_connection_id_{1};
    
    // Threading
    std::unique_ptr<std::thread> server_thread_;
    std::unique_ptr<std::thread> streaming_thread_;
    std::vector<std::unique_ptr<std::thread>> worker_threads_;
    
    // Statistics
    mutable ServerStats stats_;
    mutable std::mutex stats_mutex_;
    
    // Callbacks
    ConnectionCallback connection_callback_;
    MessageCallback message_callback_;
    ErrorCallback error_callback_;
    
    // Message serialization
    std::vector<uint8_t> serialize_metrics(const HFTMetrics& metrics);
    std::vector<uint8_t> serialize_market_state(const MarketState& state);
    std::vector<uint8_t> serialize_snapshot(const TelemetryEngine::PerformanceSnapshot& snapshot);
    
    ClientSubscription parse_subscription_message(const std::string& message);
    std::string create_heartbeat_message();
    
    // Server implementation
    void server_loop();
    void streaming_loop();
    void worker_loop(int worker_id);
    
    // Connection handling
    WebSocketConnection::ConnectionId add_connection();
    void remove_connection(WebSocketConnection::ConnectionId id);
    void handle_client_message(WebSocketConnection::ConnectionId id, const std::string& message);
    
    // Rate limiting and backpressure
    bool should_send_update(const WebSocketConnection& connection) const;
    void apply_rate_limiting();
    
    // Error handling
    void handle_error(const std::string& error);
    
    // Statistics updates
    void update_stats();
    
    // Message broadcasting
    void broadcast_metrics_update();
    void broadcast_market_update();
    void send_heartbeats();
    
    // Utility functions
    std::string get_client_info(WebSocketConnection::ConnectionId id) const;
    void log_connection_event(WebSocketConnection::ConnectionId id, const std::string& event);
};

} // namespace hfx::viz
