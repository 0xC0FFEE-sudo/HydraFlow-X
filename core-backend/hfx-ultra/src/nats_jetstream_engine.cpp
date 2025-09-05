/**
 * @file nats_jetstream_engine.cpp
 * @brief Ultra-low latency NATS JetStream implementation for high-frequency trading
 */

#include "nats_jetstream_engine.hpp"
#include "hfx-log/include/logger.hpp"
#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <random>
#include <sstream>
#include <iomanip>

// Mock NATS includes (in production, would use actual NATS C client)
#ifdef USE_REAL_NATS
#include <nats/nats.h>
#include <nats/js.h>
#endif

namespace hfx::ultra {

// FastMessage implementation
FastMessage::FastMessage(const std::string& subj, const std::vector<uint8_t>& data, MessagePriority prio)
    : sequence_id(0), timestamp_ns(std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()).count()),
      priority(prio), compression(CompressionType::NONE), payload_size(data.size()),
      subject(subj), payload(data) {}

// Internal implementation using PIMPL pattern
class NATSJetStreamEngine::Impl {
public:
    Impl(const NATSConfig& config) 
        : config_(config), next_sequence_id_(1), next_subscription_id_(1) {
        
        // Initialize priority queues
        for (auto& pq : priority_queues_) {
            pq.size.store(0);
        }
        
        HFX_LOG_INFO("🚀 Initializing NATS JetStream Engine...");
        std::string servers_list = "";
        for (const auto& server : config_.servers) {
            servers_list += server + " ";
        }
        HFX_LOG_INFO("   Servers: " + servers_list);
        HFX_LOG_INFO("   Cluster: " + config_.cluster_name);
    }

    ~Impl() {
        disconnect();
    }

    bool connect() {
        if (connected_.load()) {
            HFX_LOG_INFO("⚠️  Already connected to NATS JetStream");
            return true;
        }

        HFX_LOG_INFO("🔌 Connecting to NATS JetStream cluster...");

#ifdef USE_REAL_NATS
        // Real NATS implementation would go here
        // For now, using mock implementation
#endif
        
        // Mock implementation
        nats_connection_ = reinterpret_cast<void*>(0xDEADBEEF);
        jetstream_context_ = reinterpret_cast<void*>(0xCAFEBABE);
        jetstream_enabled_.store(true);

        connected_.store(true);
        threads_running_.store(true);

        // Start worker threads
        connection_monitor_ = std::thread(&Impl::connection_monitor_worker, this);
        message_processor_ = std::thread(&Impl::message_processor_worker, this);

        if (connection_handler_) {
            connection_handler_(true);
        }

        HFX_LOG_INFO("✅ NATS JetStream connected successfully");
        HFX_LOG_INFO("   JetStream enabled: " + std::string(jetstream_enabled_.load() ? "Yes" : "No"));
        
        return true;
    }

    bool disconnect() {
        if (!connected_.load()) {
            return true;
        }

        HFX_LOG_INFO("🔌 Disconnecting from NATS JetStream...");

        shutdown_requested_.store(true);
        
        // Stop all worker threads
        if (threads_running_.load()) {
            threads_running_.store(false);
            
            // Wake up all worker threads
            for (auto& pq : priority_queues_) {
                pq.cv.notify_all();
            }
            
            // Join threads
            if (connection_monitor_.joinable()) {
                connection_monitor_.join();
            }
            if (message_processor_.joinable()) {
                message_processor_.join();
            }
        }

        // Clean up subscriptions
        {
            std::lock_guard<std::mutex> lock(subscriptions_mutex_);
            subscriptions_.clear();
        }

        nats_connection_ = nullptr;
        jetstream_context_ = nullptr;
        connected_.store(false);
        jetstream_enabled_.store(false);

        if (connection_handler_) {
            connection_handler_(false);
        }

        HFX_LOG_INFO("✅ NATS JetStream disconnected");
        return true;
    }

    bool is_connected() const {
        return connected_.load();
    }
    
    bool is_jetstream_enabled() const {
        return jetstream_enabled_.load();
    }

    bool create_stream(const StreamConfig& stream_config) {
        if (!is_connected() || !jetstream_enabled_.load()) {
            handle_error("Cannot create stream: not connected to JetStream");
            return false;
        }

        HFX_LOG_INFO("📊 Creating JetStream: " + stream_config.name);

        // Mock implementation
        std::lock_guard<std::mutex> lock(streams_mutex_);
        streams_[stream_config.name] = stream_config;
        HFX_LOG_INFO("✅ Mock stream created: " + stream_config.name);

        return true;
    }

    uint64_t publish(const std::string& subject, const std::vector<uint8_t>& data, MessagePriority priority) {
        if (!is_connected()) {
            handle_error("Cannot publish: not connected");
            return 0;
        }

        auto start_time = std::chrono::high_resolution_clock::now();
        uint64_t seq_id = next_sequence_id_.fetch_add(1);

        // Mock publish - just simulate network latency
        std::this_thread::sleep_for(std::chrono::microseconds(50));

        auto end_time = std::chrono::high_resolution_clock::now();
        auto latency = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
        
        // Update metrics
        metrics_.messages_published.fetch_add(1);
        metrics_.bytes_sent.fetch_add(data.size());
        
        double current_avg = metrics_.avg_publish_latency_us.load();
        double new_avg = (current_avg * 0.9) + (latency.count() * 0.1);
        metrics_.avg_publish_latency_us.store(new_avg);

        return seq_id;
    }

    std::string subscribe(const std::string& subject, MessageHandler handler) {
        if (!is_connected()) {
            handle_error("Cannot subscribe: not connected");
            return "";
        }

        std::string sub_id = generate_subscription_id();
        
        Subscription sub;
        sub.id = sub_id;
        sub.subject = subject;
        sub.handler = handler;
        sub.active.store(true);
        sub.nats_subscription = reinterpret_cast<void*>(next_subscription_id_.fetch_add(1));

        {
            std::lock_guard<std::mutex> lock(subscriptions_mutex_);
            subscriptions_[sub_id] = std::move(sub);
        }

        metrics_.active_subscriptions.fetch_add(1);
        HFX_LOG_INFO("📨 Mock subscribed to: " + subject + " (ID: " + std::to_string(sub_id) + ")");
        
        return sub_id;
    }

    bool setup_trading_streams(const TradingStreams& streams) {
        HFX_LOG_INFO("🏗️  Setting up trading streams...");

        std::vector<StreamConfig> configs = {
            create_market_data_stream_config(streams.market_data),
            create_trades_stream_config(streams.trade_orders),
            create_mev_stream_config(streams.mev_opportunities),
            create_risk_stream_config(streams.risk_alerts),
            create_execution_stream_config(streams.execution_reports),
            create_audit_stream_config(streams.audit_logs)
        };

        bool all_success = true;
        for (const auto& config : configs) {
            if (!create_stream(config)) {
                HFX_LOG_ERROR("❌ Failed to create stream: " + config.name);
                all_success = false;
            }
        }

        if (all_success) {
            HFX_LOG_INFO("✅ All trading streams created successfully");
        }

        return all_success;
    }
    
    bool delete_stream(const std::string& stream_name) {
        HFX_LOG_INFO("🗑️  Deleting JetStream: " + stream_name);
        std::lock_guard<std::mutex> lock(streams_mutex_);
        streams_.erase(stream_name);
        return true;
    }
    
    bool update_stream(const StreamConfig& config) {
        return create_stream(config); // For mock implementation
    }
    
    std::vector<std::string> list_streams() const {
        std::lock_guard<std::mutex> lock(streams_mutex_);
        std::vector<std::string> stream_names;
        for (const auto& [name, config] : streams_) {
            stream_names.push_back(name);
        }
        return stream_names;
    }

    void set_connection_handler(ConnectionHandler handler) {
        connection_handler_ = handler;
    }

    void set_error_handler(ErrorHandler handler) {
        error_handler_ = handler;
    }

    const Metrics& get_metrics() const {
        return metrics_;
    }

    void reset_metrics() {
        HFX_LOG_INFO("📊 Resetting NATS metrics");
        
        metrics_.messages_published.store(0);
        metrics_.messages_received.store(0);
        metrics_.messages_acknowledged.store(0);
        metrics_.publish_errors.store(0);
        metrics_.connection_drops.store(0);
        metrics_.avg_publish_latency_us.store(0.0);
        metrics_.avg_delivery_latency_us.store(0.0);
        metrics_.bytes_sent.store(0);
        metrics_.bytes_received.store(0);
        metrics_.active_subscriptions.store(0);
        metrics_.pending_acks.store(0);
    }

    ConnectionStats get_connection_stats() const {
        ConnectionStats stats;
        stats.is_connected = connected_.load();
        stats.reconnect_count = 0;
        stats.last_reconnect = std::chrono::system_clock::now() - std::chrono::minutes(10);
        stats.round_trip_time = std::chrono::milliseconds(1);
        stats.in_msgs = metrics_.messages_received.load();
        stats.out_msgs = metrics_.messages_published.load();
        stats.in_bytes = metrics_.bytes_received.load();
        stats.out_bytes = metrics_.bytes_sent.load();
        stats.server_info = "Mock NATS Server v2.10.0";
        stats.cluster_info = config_.cluster_name;
        return stats;
    }

private:
    NATSConfig config_;
    std::atomic<bool> connected_{false};
    std::atomic<bool> jetstream_enabled_{false};
    std::atomic<bool> shutdown_requested_{false};
    std::atomic<bool> threads_running_{false};
    
    mutable Metrics metrics_;
    
    void* nats_connection_ = nullptr;
    void* jetstream_context_ = nullptr;
    
    std::thread connection_monitor_;
    std::thread message_processor_;
    
    struct PriorityQueue {
        std::queue<FastMessage> queue;
        std::mutex mutex;
        std::condition_variable cv;
        std::atomic<uint32_t> size{0};
    };
    std::array<PriorityQueue, 7> priority_queues_;
    
    struct Subscription {
        std::string id;
        std::string subject;
        MessageHandler handler;
        void* nats_subscription = nullptr;
        std::atomic<bool> active{true};
        
        // Custom move constructor and assignment for atomic member
        Subscription() = default;
        Subscription(Subscription&& other) noexcept 
            : id(std::move(other.id)), subject(std::move(other.subject)), 
              handler(std::move(other.handler)), nats_subscription(other.nats_subscription),
              active(other.active.load()) {
            other.nats_subscription = nullptr;
        }
        
        Subscription& operator=(Subscription&& other) noexcept {
            if (this != &other) {
                id = std::move(other.id);
                subject = std::move(other.subject);
                handler = std::move(other.handler);
                nats_subscription = other.nats_subscription;
                active.store(other.active.load());
                other.nats_subscription = nullptr;
            }
            return *this;
        }
        
        // Delete copy constructor and assignment
        Subscription(const Subscription&) = delete;
        Subscription& operator=(const Subscription&) = delete;
    };
    std::unordered_map<std::string, Subscription> subscriptions_;
    mutable std::mutex subscriptions_mutex_;
    
    std::unordered_map<std::string, StreamConfig> streams_;
    mutable std::mutex streams_mutex_;
    
    ConnectionHandler connection_handler_;
    ErrorHandler error_handler_;
    
    std::atomic<uint64_t> next_sequence_id_{1};
    std::atomic<uint64_t> next_subscription_id_{1};

    void handle_error(const std::string& error) {
        if (error_handler_) {
            error_handler_(error);
        }
        HFX_LOG_ERROR("❌ NATS Error: " + error);
    }

    std::string generate_subscription_id() {
        static std::atomic<uint64_t> counter{0};
        return "sub_" + std::to_string(counter.fetch_add(1));
    }

    void connection_monitor_worker() {
        HFX_LOG_INFO("🔍 Starting connection monitor worker");
        
        while (threads_running_.load()) {
            std::this_thread::sleep_for(std::chrono::seconds(5));
            
            if (connected_.load()) {
                // Perform health checks in real implementation
            }
        }
        
        HFX_LOG_INFO("🔍 Connection monitor worker stopped");
    }

    void message_processor_worker() {
        HFX_LOG_INFO("📨 Starting message processor worker");
        
        while (threads_running_.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            // Process incoming messages in real implementation
        }
        
        HFX_LOG_INFO("📨 Message processor worker stopped");
    }

    StreamConfig create_market_data_stream_config(const std::string& prefix) {
        StreamConfig config;
        config.name = "MARKET_DATA";
        config.description = "High-frequency market data stream";
        config.subjects = {prefix + "price.*", prefix + "orderbook.*", prefix + "trades.*"};
        config.max_age = std::chrono::hours(1);
        config.max_messages = 10000000;
        config.compression = CompressionType::LZ4_FAST;
        config.min_priority = MessagePriority::MARKET_DATA;
        return config;
    }

    StreamConfig create_trades_stream_config(const std::string& prefix) {
        StreamConfig config;
        config.name = "TRADES";
        config.description = "Trading orders and executions";
        config.subjects = {prefix + "order.*", prefix + "execution.*"};
        config.max_age = std::chrono::hours(24);
        config.max_messages = 1000000;
        config.compression = CompressionType::NONE;
        config.min_priority = MessagePriority::TRADE_URGENT;
        return config;
    }

    StreamConfig create_mev_stream_config(const std::string& prefix) {
        StreamConfig config;
        config.name = "MEV";
        config.description = "MEV opportunities and execution";
        config.subjects = {prefix + "opportunity.*", prefix + "arbitrage.*"};
        config.max_age = std::chrono::duration_cast<std::chrono::hours>(std::chrono::minutes(10));
        config.max_messages = 100000;
        config.compression = CompressionType::NONE;
        config.min_priority = MessagePriority::MEV_CRITICAL;
        return config;
    }

    StreamConfig create_risk_stream_config(const std::string& prefix) {
        StreamConfig config;
        config.name = "RISK";
        config.description = "Risk management alerts and limits";
        config.subjects = {prefix + "alert.*", prefix + "limit.*"};
        config.max_age = std::chrono::hours(24);
        config.max_messages = 500000;
        config.compression = CompressionType::LZ4_FAST;
        config.min_priority = MessagePriority::EMERGENCY;
        return config;
    }

    StreamConfig create_execution_stream_config(const std::string& prefix) {
        StreamConfig config;
        config.name = "EXECUTION";
        config.description = "Trade execution reports";
        config.subjects = {prefix + "fill.*", prefix + "report.*"};
        config.max_age = std::chrono::hours(24);
        config.max_messages = 2000000;
        config.compression = CompressionType::LZ4_FAST;
        config.min_priority = MessagePriority::TRADE_NORMAL;
        return config;
    }

    StreamConfig create_audit_stream_config(const std::string& prefix) {
        StreamConfig config;
        config.name = "AUDIT";
        config.description = "Audit trail and compliance logs";
        config.subjects = {prefix + "trade.*", prefix + "access.*"};
        config.max_age = std::chrono::hours(168); // 7 days
        config.max_messages = 5000000;
        config.compression = CompressionType::ZSTD_FAST;
        config.min_priority = MessagePriority::AUDIT;
        return config;
    }
};

// Main class implementation
NATSJetStreamEngine::NATSJetStreamEngine(const NATSConfig& config) 
    : config_(config), pimpl_(std::make_unique<Impl>(config)) {
}

NATSJetStreamEngine::~NATSJetStreamEngine() = default;

bool NATSJetStreamEngine::connect() { 
    return pimpl_->connect(); 
}

bool NATSJetStreamEngine::disconnect() { 
    return pimpl_->disconnect(); 
}

bool NATSJetStreamEngine::is_connected() const { 
    return pimpl_->is_connected(); 
}

bool NATSJetStreamEngine::is_jetstream_enabled() const {
    return pimpl_->is_jetstream_enabled();
}

bool NATSJetStreamEngine::create_stream(const StreamConfig& config) {
    return pimpl_->create_stream(config);
}

bool NATSJetStreamEngine::delete_stream(const std::string& stream_name) {
    return pimpl_->delete_stream(stream_name);
}

bool NATSJetStreamEngine::update_stream(const StreamConfig& config) {
    return pimpl_->update_stream(config);
}

std::vector<std::string> NATSJetStreamEngine::list_streams() const {
    return pimpl_->list_streams();
}

uint64_t NATSJetStreamEngine::publish(const std::string& subject, const std::vector<uint8_t>& data, MessagePriority priority) {
    return pimpl_->publish(subject, data, priority);
}

std::string NATSJetStreamEngine::subscribe(const std::string& subject, MessageHandler handler) {
    return pimpl_->subscribe(subject, handler);
}

bool NATSJetStreamEngine::setup_trading_streams(const TradingStreams& streams) {
    return pimpl_->setup_trading_streams(streams);
}

void NATSJetStreamEngine::set_connection_handler(ConnectionHandler handler) {
    pimpl_->set_connection_handler(handler);
}

void NATSJetStreamEngine::set_error_handler(ErrorHandler handler) {
    pimpl_->set_error_handler(handler);
}

const NATSJetStreamEngine::Metrics& NATSJetStreamEngine::get_metrics() const {
    return pimpl_->get_metrics();
}

void NATSJetStreamEngine::reset_metrics() {
    pimpl_->reset_metrics();
}

NATSJetStreamEngine::ConnectionStats NATSJetStreamEngine::get_connection_stats() const {
    return pimpl_->get_connection_stats();
}

// Factory implementations
std::unique_ptr<NATSJetStreamEngine> NATSEngineFactory::create_high_frequency_engine() {
    NATSConfig config;
    config.servers = {"nats://localhost:4222"};
    config.cluster_name = "hfx-hft";
    config.no_echo = true;
    config.pedantic = false;
    config.verbose = false;
    config.write_buffer_size = 64 * 1024 * 1024; // 64MB
    config.read_buffer_size = 64 * 1024 * 1024;  // 64MB
    config.ping_interval_sec = 30;
    config.max_pings_out = 5;
    
    return std::make_unique<NATSJetStreamEngine>(config);
}

std::unique_ptr<NATSJetStreamEngine> NATSEngineFactory::create_market_data_engine() {
    NATSConfig config;
    config.servers = {"nats://localhost:4222"};
    config.cluster_name = "hfx-market";
    config.write_buffer_size = 128 * 1024 * 1024;
    config.read_buffer_size = 128 * 1024 * 1024;
    
    return std::make_unique<NATSJetStreamEngine>(config);
}

std::unique_ptr<NATSJetStreamEngine> NATSEngineFactory::create_embedded_engine() {
    NATSConfig config;
    config.servers = {"nats://localhost:4222"};
    config.cluster_name = "hfx-test";
    config.write_buffer_size = 1024 * 1024;
    config.read_buffer_size = 1024 * 1024;
    
    return std::make_unique<NATSJetStreamEngine>(config);
}

// Utility functions
namespace nats_utils {
    std::string priority_to_string(MessagePriority priority) {
        switch (priority) {
            case MessagePriority::EMERGENCY: return "EMERGENCY";
            case MessagePriority::MEV_CRITICAL: return "MEV_CRITICAL";
            case MessagePriority::TRADE_URGENT: return "TRADE_URGENT";
            case MessagePriority::TRADE_NORMAL: return "TRADE_NORMAL";
            case MessagePriority::MARKET_DATA: return "MARKET_DATA";
            case MessagePriority::ANALYTICS: return "ANALYTICS";
            case MessagePriority::AUDIT: return "AUDIT";
            default: return "UNKNOWN";
        }
    }

    MessagePriority string_to_priority(const std::string& priority_str) {
        if (priority_str == "EMERGENCY") return MessagePriority::EMERGENCY;
        if (priority_str == "MEV_CRITICAL") return MessagePriority::MEV_CRITICAL;
        if (priority_str == "TRADE_URGENT") return MessagePriority::TRADE_URGENT;
        if (priority_str == "TRADE_NORMAL") return MessagePriority::TRADE_NORMAL;
        if (priority_str == "MARKET_DATA") return MessagePriority::MARKET_DATA;
        if (priority_str == "ANALYTICS") return MessagePriority::ANALYTICS;
        if (priority_str == "AUDIT") return MessagePriority::AUDIT;
        return MessagePriority::TRADE_NORMAL;
    }

    bool is_valid_subject(const std::string& subject) {
        if (subject.empty() || subject.length() > 256) {
            return false;
        }
        
        for (char c : subject) {
            if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
                return false;
            }
        }
        
        return true;
    }

    std::chrono::nanoseconds get_timestamp_ns() {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch());
    }

    double calculate_throughput_mbps(uint64_t bytes, std::chrono::nanoseconds duration) {
        if (duration.count() == 0) return 0.0;
        
        double seconds = duration.count() / 1e9;
        double megabytes = bytes / (1024.0 * 1024.0);
        return megabytes / seconds;
    }
}

} // namespace hfx::ultra
