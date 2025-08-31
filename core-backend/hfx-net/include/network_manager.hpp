/**
 * @file network_manager.hpp
 * @brief Ultra-low latency network manager optimized for macOS kqueue
 * @version 1.0.0
 * 
 * High-performance networking layer for HFT applications with:
 * - kqueue-based event handling
 * - QUIC for builder connections
 * - WebSocket for mempool streams
 * - Zero-copy data paths where possible
 */

#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "event_engine.hpp"

#ifdef __APPLE__
#include <sys/event.h>
#include <sys/socket.h>
#include <netinet/in.h>
#endif

namespace hfx::net {

using TimeStamp = std::chrono::time_point<std::chrono::high_resolution_clock>;
using ConnectionId = std::uint64_t;

/**
 * @enum ConnectionType
 * @brief Types of network connections
 */
enum class ConnectionType : std::uint8_t {
    WEBSOCKET_MEMPOOL,    // Mempool streaming
    QUIC_BUILDER,         // Private builder API
    HTTP_ORACLE,          // Oracle price feeds
    TCP_EXCHANGE,         // Exchange connections
    UDP_MULTICAST         // Market data feeds
};

/**
 * @enum ConnectionState
 * @brief Connection lifecycle states
 */
enum class ConnectionState : std::uint8_t {
    DISCONNECTED,
    CONNECTING,
    CONNECTED,
    AUTHENTICATED,
    ERROR,
    CLOSING
};

/**
 * @struct NetworkMessage
 * @brief Lightweight message structure for zero-copy operations
 */
struct alignas(64) NetworkMessage {
    ConnectionId connection_id;
    ConnectionType type;
    TimeStamp received_time;
    std::uint32_t size;
    const char* data;  // Points to buffer, not owned
    bool needs_copy;   // Whether data needs to be copied before use
    
    NetworkMessage() = default;
    NetworkMessage(ConnectionId id, ConnectionType t, const char* d, 
                   std::uint32_t s, bool copy = false)
        : connection_id(id), type(t), 
          received_time(std::chrono::high_resolution_clock::now()),
          size(s), data(d), needs_copy(copy) {}
} __attribute__((packed));

/**
 * @struct ConnectionConfig
 * @brief Configuration for network connections
 */
struct ConnectionConfig {
    std::string endpoint;
    ConnectionType type;
    bool auto_reconnect{true};
    std::chrono::milliseconds connect_timeout{5000};
    std::chrono::milliseconds read_timeout{1000};
    std::size_t buffer_size{65536};
    bool use_tls{true};
    
    // QUIC-specific options
    bool enable_0rtt{false};
    std::string alpn_protocol{"h3"};
    
    // WebSocket-specific options
    std::string ws_path{"/"};
    std::unordered_map<std::string, std::string> headers;
};

/**
 * @class NetworkManager
 * @brief High-performance network manager with Apple Silicon optimizations
 * 
 * Features:
 * - kqueue-based event handling for sub-microsecond latency
 * - Connection pooling and management
 * - Automatic reconnection with exponential backoff
 * - Zero-copy message handling where possible
 * - Hardware timestamp support
 */
class NetworkManager {
public:
    using MessageCallback = std::function<void(const NetworkMessage&)>;
    using ConnectionCallback = std::function<void(ConnectionId, ConnectionState)>;
    using ErrorCallback = std::function<void(ConnectionId, const std::string&)>;

    NetworkManager();
    ~NetworkManager();

    // Non-copyable, non-movable
    NetworkManager(const NetworkManager&) = delete;
    NetworkManager& operator=(const NetworkManager&) = delete;
    NetworkManager(NetworkManager&&) = delete;
    NetworkManager& operator=(NetworkManager&&) = delete;

    /**
     * @brief Initialize the network manager
     * @return true on success
     */
    bool initialize();

    /**
     * @brief Shutdown all connections gracefully
     */
    void shutdown();

    /**
     * @brief Create a new connection
     * @param config Connection configuration
     * @return Connection ID, or 0 on failure
     */
    ConnectionId create_connection(const ConnectionConfig& config);

    /**
     * @brief Close and remove a connection
     * @param connection_id Connection to close
     * @return true if closed successfully
     */
    bool close_connection(ConnectionId connection_id);

    /**
     * @brief Send data on a connection
     * @param connection_id Target connection
     * @param data Data to send
     * @param size Data size in bytes
     * @return true if queued for sending
     */
    bool send_data(ConnectionId connection_id, const void* data, std::size_t size);

    /**
     * @brief Process network events (called by event loop)
     * @param timeout_us Timeout in microseconds
     * @return Number of events processed
     */
    std::size_t process_events(std::uint64_t timeout_us = 0);

    /**
     * @brief Set callback for incoming messages
     * @param callback Function to call on message receipt
     */
    void set_data_callback(MessageCallback callback) {
        message_callback_ = std::move(callback);
    }

    /**
     * @brief Set callback for connection state changes
     * @param callback Function to call on state change
     */
    void set_connection_callback(ConnectionCallback callback) {
        connection_callback_ = std::move(callback);
    }

    /**
     * @brief Set callback for connection errors
     * @param callback Function to call on errors
     */
    void set_error_callback(ErrorCallback callback) {
        error_callback_ = std::move(callback);
    }

    /**
     * @brief Get connection statistics
     */
    struct Statistics {
        std::uint64_t total_connections{0};
        std::uint64_t active_connections{0};
        std::uint64_t total_bytes_sent{0};
        std::uint64_t total_bytes_received{0};
        std::uint64_t total_messages_sent{0};
        std::uint64_t total_messages_received{0};
        double avg_latency_us{0.0};
        std::uint64_t reconnection_count{0};
        std::uint64_t error_count{0};
    };

    Statistics get_statistics() const;

    /**
     * @brief Check if manager is running
     */
    bool is_running() const noexcept {
        return running_.load(std::memory_order_acquire);
    }

    /**
     * @brief Get hardware timestamp if available
     * @return Hardware timestamp in nanoseconds
     */
    static std::uint64_t get_hardware_timestamp();

private:
    class Connection;  // Forward declaration
    class KQueueHandler;  // Forward declaration

    std::atomic<bool> running_{false};
    std::atomic<ConnectionId> next_connection_id_{1};

    // Platform-specific event handling
#ifdef __APPLE__
    std::unique_ptr<KQueueHandler> kqueue_handler_;
#endif

    // Connection management
    std::unordered_map<ConnectionId, std::unique_ptr<Connection>> connections_;
    
    // Callbacks
    MessageCallback message_callback_;
    ConnectionCallback connection_callback_;
    ErrorCallback error_callback_;

    // Statistics
    mutable std::atomic<std::uint64_t> total_bytes_sent_{0};
    mutable std::atomic<std::uint64_t> total_bytes_received_{0};
    mutable std::atomic<std::uint64_t> total_messages_sent_{0};
    mutable std::atomic<std::uint64_t> total_messages_received_{0};
    mutable std::atomic<std::uint64_t> total_latency_us_{0};
    mutable std::atomic<std::uint64_t> reconnection_count_{0};
    mutable std::atomic<std::uint64_t> error_count_{0};

    /**
     * @brief Initialize platform-specific networking
     */
    bool initialize_platform();

    /**
     * @brief Handle incoming message
     * @param message Received message
     */
    void handle_message(const NetworkMessage& message);

    /**
     * @brief Handle connection state change
     * @param connection_id Connection ID
     * @param new_state New connection state
     */
    void handle_connection_state_change(ConnectionId connection_id, ConnectionState new_state);

    /**
     * @brief Handle connection error
     * @param connection_id Connection ID
     * @param error Error description
     */
    void handle_connection_error(ConnectionId connection_id, const std::string& error);

    /**
     * @brief Update performance metrics
     * @param bytes_received Bytes received in this update
     * @param latency_us Processing latency in microseconds
     */
    void update_statistics(std::uint64_t bytes_received, double latency_us);
};

/**
 * @brief Network endpoint configuration for DeFi protocols
 */
struct DeFiEndpoints {
    // Ethereum L1
    static constexpr const char* ETH_MAINNET_WSS = "wss://mainnet.infura.io/ws/v3/YOUR_KEY";
    static constexpr const char* ETH_MEMPOOL_WSS = "wss://api.blocknative.com/v0";
    
    // Layer 2s
    static constexpr const char* ARBITRUM_WSS = "wss://arbitrum-mainnet.infura.io/ws/v3/YOUR_KEY";
    static constexpr const char* OPTIMISM_WSS = "wss://optimism-mainnet.infura.io/ws/v3/YOUR_KEY";
    static constexpr const char* BASE_WSS = "wss://base-mainnet.infura.io/ws/v3/YOUR_KEY";
    
    // MEV Infrastructure
    static constexpr const char* FLASHBOTS_BUILDER = "https://relay.flashbots.net";
    static constexpr const char* BLOXXYZ_BUILDER = "https://bloxxyz.max-profit.xyz";
    
    // Oracle feeds
    static constexpr const char* CHAINLINK_WSS = "wss://cl-adapter.linkfollowprotocol.com";
    static constexpr const char* PYTH_WSS = "wss://pythnet.rpcpool.com";
};

} // namespace hfx::net
