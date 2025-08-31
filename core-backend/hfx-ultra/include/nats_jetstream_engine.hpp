#pragma once

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <atomic>
#include <mutex>
#include <thread>
#include <chrono>
#include <functional>
#include <queue>
#include <condition_variable>

namespace hfx::ultra {

// Message priorities for different trading scenarios
enum class MessagePriority {
    EMERGENCY = 0,      // Emergency stop, system alerts
    MEV_CRITICAL = 1,   // Time-critical MEV opportunities  
    TRADE_URGENT = 2,   // Urgent trade executions
    TRADE_NORMAL = 3,   // Normal trading operations
    MARKET_DATA = 4,    // Market data updates
    ANALYTICS = 5,      // Analytics and reporting
    AUDIT = 6          // Audit and compliance logs
};

// Message compression types for bandwidth optimization
enum class CompressionType {
    NONE,           // No compression
    LZ4_FAST,       // LZ4 fast compression
    ZSTD_FAST,      // Zstandard fast compression
    CUSTOM_DELTA    // Custom delta compression for market data
};

// Stream configuration for different data types
struct StreamConfig {
    std::string name;
    std::string description;
    std::vector<std::string> subjects;
    
    // Retention policy
    std::chrono::hours max_age{24};          // 24 hours default
    uint64_t max_messages = 1000000;         // 1M messages default
    uint64_t max_bytes = 1024 * 1024 * 1024; // 1GB default
    
    // Performance settings
    bool discard_on_full = true;             // Discard old messages when full
    uint32_t replicas = 1;                   // Number of replicas
    CompressionType compression = CompressionType::LZ4_FAST;
    
    // Trading-specific settings
    bool enable_deduplication = true;        // Prevent duplicate messages
    std::chrono::microseconds max_age_for_duplicates{1000}; // 1ms window
    MessagePriority min_priority = MessagePriority::ANALYTICS;
};

// High-performance message structure
struct alignas(64) FastMessage {
    uint64_t sequence_id;
    uint64_t timestamp_ns;
    MessagePriority priority;
    CompressionType compression;
    uint32_t payload_size;
    std::string subject;
    std::string reply_to;
    std::vector<uint8_t> headers;
    std::vector<uint8_t> payload;
    
    // Performance metadata
    std::chrono::nanoseconds publish_latency{0};
    std::chrono::nanoseconds delivery_latency{0};
    uint32_t retry_count = 0;
    bool acknowledged = false;
    
    // Constructor for fast message creation
    FastMessage() = default;
    FastMessage(const std::string& subj, const std::vector<uint8_t>& data, 
               MessagePriority prio = MessagePriority::TRADE_NORMAL);
};

// Consumer configuration for subscribing to streams
struct ConsumerConfig {
    std::string name;
    std::string stream_name;
    std::string filter_subject;
    
    // Delivery settings
    bool deliver_all = false;               // Start from beginning or latest
    std::chrono::microseconds ack_wait{30000}; // 30ms ack timeout
    uint32_t max_deliver = 3;               // Max delivery attempts
    
    // Performance settings
    uint32_t max_pending = 1000;            // Max pending acks
    uint32_t max_batch_size = 100;          // Batch size for efficiency
    bool push_mode = true;                  // Push vs pull mode
    
    // Priority filtering
    MessagePriority min_priority = MessagePriority::ANALYTICS;
    bool priority_based_ordering = true;
    
    // Flow control
    bool enable_flow_control = true;
    uint64_t idle_heartbeat_ms = 5000;      // 5 second heartbeat
};

// Connection configuration for NATS cluster
struct NATSConfig {
    std::vector<std::string> servers = {"nats://localhost:4222"};
    std::string cluster_name = "hfx-cluster";
    
    // Authentication
    std::string username;
    std::string password;
    std::string token;
    std::string credentials_file;
    
    // Connection settings
    std::chrono::seconds connect_timeout{5};
    std::chrono::seconds reconnect_wait{2};
    uint32_t max_reconnect_attempts = 10;
    uint64_t reconnect_buffer_size = 8 * 1024 * 1024; // 8MB
    
    // Performance settings
    bool no_echo = true;                    // Don't echo our own messages
    bool pedantic = false;                  // Fast mode
    bool verbose = false;                   // Reduce protocol overhead
    uint32_t ping_interval_sec = 120;       // Heartbeat interval
    uint32_t max_pings_out = 2;            // Max unanswered pings
    
    // Buffer sizes for high throughput
    uint64_t write_buffer_size = 32 * 1024 * 1024;  // 32MB write buffer
    uint64_t read_buffer_size = 32 * 1024 * 1024;   // 32MB read buffer
    
    // TLS settings
    bool use_tls = false;
    std::string ca_file;
    std::string cert_file;
    std::string key_file;
    bool verify_certificates = true;
};

// Message handler callback types
using MessageHandler = std::function<void(const FastMessage&)>;
using ErrorHandler = std::function<void(const std::string& error)>;
using ConnectionHandler = std::function<void(bool connected)>;

// Ultra-low latency NATS JetStream engine
class NATSJetStreamEngine {
public:
    explicit NATSJetStreamEngine(const NATSConfig& config);
    ~NATSJetStreamEngine();
    
    // Connection management
    bool connect();
    bool disconnect();
    bool is_connected() const;
    bool is_jetstream_enabled() const;
    
    // Stream management
    bool create_stream(const StreamConfig& config);
    bool delete_stream(const std::string& stream_name);
    bool update_stream(const StreamConfig& config);
    std::vector<std::string> list_streams() const;
    
    // Ultra-fast publishing (target < 100 microseconds)
    uint64_t publish(const std::string& subject, const std::vector<uint8_t>& data,
                    MessagePriority priority = MessagePriority::TRADE_NORMAL);
    
    uint64_t publish_with_reply(const std::string& subject, const std::string& reply_to,
                               const std::vector<uint8_t>& data,
                               MessagePriority priority = MessagePriority::TRADE_NORMAL);
    
    // Batch publishing for efficiency
    std::vector<uint64_t> publish_batch(const std::vector<FastMessage>& messages);
    
    // Request-reply pattern (optimized for trading)
    std::vector<uint8_t> request(const std::string& subject, const std::vector<uint8_t>& data,
                                std::chrono::milliseconds timeout = std::chrono::milliseconds(1000));
    
    // Async request-reply
    using ReplyHandler = std::function<void(const std::vector<uint8_t>& reply, bool success)>;
    std::string request_async(const std::string& subject, const std::vector<uint8_t>& data,
                             ReplyHandler handler, 
                             std::chrono::milliseconds timeout = std::chrono::milliseconds(1000));
    
    // Consumer management
    std::string create_consumer(const ConsumerConfig& config);
    bool delete_consumer(const std::string& stream_name, const std::string& consumer_name);
    
    // Subscription management (push-based)
    std::string subscribe(const std::string& subject, MessageHandler handler);
    std::string subscribe_queue(const std::string& subject, const std::string& queue_group, 
                               MessageHandler handler);
    bool unsubscribe(const std::string& subscription_id);
    
    // JetStream consumer subscriptions
    std::string subscribe_consumer(const std::string& stream_name, const std::string& consumer_name,
                                  MessageHandler handler);
    
    // Message acknowledgment
    bool ack_message(uint64_t sequence_id);
    bool nack_message(uint64_t sequence_id, std::chrono::milliseconds delay = std::chrono::milliseconds(1000));
    
    // Trading-specific high-level APIs
    struct TradingStreams {
        std::string market_data;        // Market data stream
        std::string trade_orders;       // Trade orders
        std::string mev_opportunities;  // MEV opportunities
        std::string risk_alerts;        // Risk management alerts
        std::string execution_reports;  // Execution reports
        std::string audit_logs;         // Audit trail
        
        TradingStreams() 
            : market_data("MARKET.>"), trade_orders("TRADES.>"), mev_opportunities("MEV.>"),
              risk_alerts("RISK.>"), execution_reports("EXEC.>"), audit_logs("AUDIT.>") {}
    };
    
    bool setup_trading_streams(const TradingStreams& streams = TradingStreams{});
    
    // Market data publishing (optimized for speed)
    uint64_t publish_price_update(const std::string& symbol, double price, double volume, 
                                 uint64_t timestamp_us);
    uint64_t publish_orderbook_update(const std::string& symbol, const std::vector<uint8_t>& orderbook_data);
    
    // Trade execution messaging
    uint64_t publish_trade_order(const std::string& order_data, MessagePriority priority = MessagePriority::TRADE_URGENT);
    uint64_t publish_execution_report(const std::string& execution_data);
    
    // MEV opportunity broadcasting
    uint64_t publish_mev_opportunity(const std::vector<uint8_t>& opportunity_data);
    
    // Risk management alerts
    uint64_t publish_risk_alert(const std::string& alert_type, const std::string& details,
                               MessagePriority priority = MessagePriority::EMERGENCY);
    
    // Event handlers
    void set_connection_handler(ConnectionHandler handler);
    void set_error_handler(ErrorHandler handler);
    
    // Performance monitoring
    struct Metrics {
        std::atomic<uint64_t> messages_published{0};
        std::atomic<uint64_t> messages_received{0};
        std::atomic<uint64_t> messages_acknowledged{0};
        std::atomic<uint64_t> publish_errors{0};
        std::atomic<uint64_t> connection_drops{0};
        std::atomic<double> avg_publish_latency_us{0.0};
        std::atomic<double> avg_delivery_latency_us{0.0};
        std::atomic<uint64_t> bytes_sent{0};
        std::atomic<uint64_t> bytes_received{0};
        std::atomic<uint32_t> active_subscriptions{0};
        std::atomic<uint32_t> pending_acks{0};
    };
    
    const Metrics& get_metrics() const;
    void reset_metrics();
    
    // Connection statistics
    struct ConnectionStats {
        bool is_connected;
        uint32_t reconnect_count;
        std::chrono::system_clock::time_point last_reconnect;
        std::chrono::milliseconds round_trip_time;
        uint64_t in_msgs;
        uint64_t out_msgs;
        uint64_t in_bytes;
        uint64_t out_bytes;
        std::string server_info;
        std::string cluster_info;
    };
    
    ConnectionStats get_connection_stats() const;
    
    // Stream statistics
    struct StreamStats {
        std::string name;
        uint64_t messages;
        uint64_t bytes;
        uint64_t first_sequence;
        uint64_t last_sequence;
        std::chrono::system_clock::time_point first_timestamp;
        std::chrono::system_clock::time_point last_timestamp;
        uint32_t consumer_count;
    };
    
    std::vector<StreamStats> get_stream_stats() const;
    
    // Advanced features
    bool enable_message_tracing(bool enable);
    std::vector<std::string> get_message_trace(uint64_t sequence_id) const;
    
    // Priority-based message processing
    void set_priority_processing(bool enable);
    void set_priority_queue_sizes(const std::unordered_map<MessagePriority, uint32_t>& sizes);
    
    // Flow control and backpressure
    bool is_backpressure_active() const;
    void set_backpressure_threshold(uint64_t threshold_bytes);
    
private:
    NATSConfig config_;
    
    // PIMPL pattern for implementation details
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

// Factory for creating NATS engines with different configurations
class NATSEngineFactory {
public:
    // Pre-configured engines for different trading scenarios
    static std::unique_ptr<NATSJetStreamEngine> create_high_frequency_engine();
    static std::unique_ptr<NATSJetStreamEngine> create_market_data_engine();
    static std::unique_ptr<NATSJetStreamEngine> create_mev_engine();
    static std::unique_ptr<NATSJetStreamEngine> create_risk_management_engine();
    
    // Cluster configurations
    static std::unique_ptr<NATSJetStreamEngine> create_clustered_engine(const std::vector<std::string>& servers);
    static std::unique_ptr<NATSJetStreamEngine> create_from_config(const NATSConfig& config);
    
    // Testing configurations
    static std::unique_ptr<NATSJetStreamEngine> create_embedded_engine(); // For testing
};

// Utility functions for NATS integration
namespace nats_utils {
    std::string priority_to_string(MessagePriority priority);
    MessagePriority string_to_priority(const std::string& priority_str);
    
    std::string compression_to_string(CompressionType compression);
    CompressionType string_to_compression(const std::string& compression_str);
    
    // Subject validation and utilities
    bool is_valid_subject(const std::string& subject);
    std::string sanitize_subject(const std::string& subject);
    std::vector<std::string> tokenize_subject(const std::string& subject);
    
    // Message utilities
    std::vector<uint8_t> serialize_message(const FastMessage& message);
    FastMessage deserialize_message(const std::vector<uint8_t>& data);
    
    // Performance utilities
    std::chrono::nanoseconds get_timestamp_ns();
    double calculate_throughput_mbps(uint64_t bytes, std::chrono::nanoseconds duration);
}

} // namespace hfx::ultra
