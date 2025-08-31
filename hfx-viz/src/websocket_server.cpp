/**
 * @file websocket_server.cpp
 * @brief Implementation of high-performance WebSocket server for HFT data streaming
 */

#include "websocket_server.hpp"
#include <iostream>
#include <sstream>
#include <thread>
#include <chrono>

namespace hfx::viz {

WebSocketConnection::WebSocketConnection(ConnectionId id) : id_(id) {
    update_activity();
}

void WebSocketConnection::send_binary(const std::vector<uint8_t>& data) {
    // In a real implementation, this would send via WebSocket connection
    bytes_sent_ += data.size();
    update_activity();
}

void WebSocketConnection::send_text(const std::string& data) {
    // In a real implementation, this would send via WebSocket connection
    bytes_sent_ += data.size();
    update_activity();
}

void WebSocketConnection::send_ping() {
    // In a real implementation, this would send a WebSocket ping frame
    update_activity();
}

WebSocketServer::WebSocketServer(const WebSocketConfig& config) 
    : config_(config) {
    stats_.start_time = std::chrono::steady_clock::now();
}

WebSocketServer::~WebSocketServer() {
    stop();
}

bool WebSocketServer::start() {
    if (running_.load(std::memory_order_acquire)) {
        return true;
    }
    
    std::cout << "[WebSocketServer] Starting server on " << config_.bind_address 
              << ":" << config_.port << std::endl;
    
    running_.store(true, std::memory_order_release);
    
    // Start server thread
    server_thread_ = std::make_unique<std::thread>(&WebSocketServer::server_loop, this);
    
    // Start streaming thread
    streaming_thread_ = std::make_unique<std::thread>(&WebSocketServer::streaming_loop, this);
    
    // Start worker threads
    for (size_t i = 0; i < config_.io_thread_pool_size; ++i) {
        worker_threads_.emplace_back(
            std::make_unique<std::thread>(&WebSocketServer::worker_loop, this, static_cast<int>(i))
        );
    }
    
    std::cout << "[WebSocketServer] Started with " << config_.io_thread_pool_size 
              << " worker threads" << std::endl;
    
    return true;
}

void WebSocketServer::stop() {
    if (!running_.load(std::memory_order_acquire)) {
        return;
    }
    
    std::cout << "[WebSocketServer] Stopping server..." << std::endl;
    
    running_.store(false, std::memory_order_release);
    
    // Join all threads
    if (server_thread_ && server_thread_->joinable()) {
        server_thread_->join();
    }
    
    if (streaming_thread_ && streaming_thread_->joinable()) {
        streaming_thread_->join();
    }
    
    for (auto& worker : worker_threads_) {
        if (worker && worker->joinable()) {
            worker->join();
        }
    }
    
    worker_threads_.clear();
    
    // Disconnect all clients
    {
        std::lock_guard<std::mutex> lock(connections_mutex_);
        connections_.clear();
    }
    
    std::cout << "[WebSocketServer] Server stopped" << std::endl;
}

void WebSocketServer::set_telemetry_engine(std::shared_ptr<TelemetryEngine> telemetry) {
    telemetry_ = telemetry;
    
    if (telemetry_) {
        // Register callback for real-time updates
        telemetry_->register_callback([this](const HFTMetrics& metrics, const MarketState& market) {
            // Queue updates for broadcasting
            broadcast_metrics_update();
            broadcast_market_update();
        });
    }
}

size_t WebSocketServer::get_connection_count() const {
    std::lock_guard<std::mutex> lock(connections_mutex_);
    return connections_.size();
}

std::vector<WebSocketConnection::ConnectionId> WebSocketServer::get_active_connections() const {
    std::lock_guard<std::mutex> lock(connections_mutex_);
    std::vector<WebSocketConnection::ConnectionId> ids;
    for (const auto& [id, conn] : connections_) {
        if (conn->is_connected()) {
            ids.push_back(id);
        }
    }
    return ids;
}

void WebSocketServer::disconnect_client(WebSocketConnection::ConnectionId id) {
    std::lock_guard<std::mutex> lock(connections_mutex_);
    auto it = connections_.find(id);
    if (it != connections_.end()) {
        it->second->set_connected(false);
        connections_.erase(it);
        
        if (connection_callback_) {
            connection_callback_(id, false);
        }
    }
}

void WebSocketServer::broadcast_message(const std::vector<uint8_t>& data, MessageType type) {
    std::lock_guard<std::mutex> lock(connections_mutex_);
    
    for (const auto& [id, conn] : connections_) {
        if (conn->is_connected()) {
            // Add message type header
            std::vector<uint8_t> message;
            message.push_back(static_cast<uint8_t>(type));
            message.insert(message.end(), data.begin(), data.end());
            
            conn->send_binary(message);
        }
    }
    
    // Update statistics
    {
        std::lock_guard<std::mutex> stats_lock(stats_mutex_);
        stats_.total_messages_sent += connections_.size();
        stats_.total_bytes_sent += data.size() * connections_.size();
    }
}

std::vector<uint8_t> WebSocketServer::serialize_metrics(const HFTMetrics& metrics) {
    // Simple binary serialization for demo
    // In production, this would use a proper serialization format like Protocol Buffers
    std::vector<uint8_t> data;
    
    // Serialize key metrics as fixed-size values
    auto append_uint64 = [&](uint64_t value) {
        for (int i = 0; i < 8; ++i) {
            data.push_back(static_cast<uint8_t>(value >> (i * 8)));
        }
    };
    
    auto append_double = [&](double value) {
        uint64_t bits = *reinterpret_cast<const uint64_t*>(&value);
        append_uint64(bits);
    };
    
    // Serialize timing metrics
    append_uint64(metrics.timing.market_data_latency_ns.load());
    append_uint64(metrics.timing.order_execution_latency_ns.load());
    append_uint64(metrics.timing.arbitrage_detection_latency_ns.load());
    
    // Serialize trading metrics
    append_uint64(metrics.trading.total_trades.load());
    append_uint64(metrics.trading.successful_arbitrages.load());
    append_double(metrics.trading.total_pnl_usd.load());
    
    // Serialize risk metrics
    append_double(metrics.risk.current_var_usd.load());
    append_double(metrics.risk.position_exposure_usd.load());
    append_double(metrics.risk.sharpe_ratio.load());
    
    return data;
}

std::vector<uint8_t> WebSocketServer::serialize_market_state(const MarketState& state) {
    // Simple serialization for demo
    std::vector<uint8_t> data;
    
    // Serialize gas market
    auto append_double = [&](double value) {
        uint64_t bits = *reinterpret_cast<const uint64_t*>(&value);
        for (int i = 0; i < 8; ++i) {
            data.push_back(static_cast<uint8_t>(bits >> (i * 8)));
        }
    };
    
    append_double(state.gas_market.standard_gwei);
    append_double(state.gas_market.fast_gwei);
    append_double(state.gas_market.instant_gwei);
    
    return data;
}

std::vector<uint8_t> WebSocketServer::serialize_snapshot(const TelemetryEngine::PerformanceSnapshot& snapshot) {
    // Combine metrics and market state
    auto metrics_data = serialize_metrics(snapshot.metrics);
    auto market_data = serialize_market_state(snapshot.market_state);
    
    std::vector<uint8_t> data;
    data.insert(data.end(), metrics_data.begin(), metrics_data.end());
    data.insert(data.end(), market_data.begin(), market_data.end());
    
    return data;
}

ClientSubscription WebSocketServer::parse_subscription_message(const std::string& message) {
    ClientSubscription subscription;
    
    // Simple JSON-like parsing for demo
    // In production, use proper JSON library
    if (message.find("\"metrics\":true") != std::string::npos) {
        subscription.metrics = true;
    }
    if (message.find("\"trades\":true") != std::string::npos) {
        subscription.trades = true;
    }
    if (message.find("\"market_data\":true") != std::string::npos) {
        subscription.market_data = true;
    }
    if (message.find("\"risk_data\":true") != std::string::npos) {
        subscription.risk_data = true;
    }
    
    return subscription;
}

std::string WebSocketServer::create_heartbeat_message() {
    return R"({"type":"heartbeat","timestamp":)" + std::to_string(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count()) + "}";
}

void WebSocketServer::server_loop() {
    std::cout << "[WebSocketServer] Server loop started" << std::endl;
    
    // Demo implementation - in production, this would handle WebSocket connections
    auto last_connection_simulation = std::chrono::steady_clock::now();
    
    while (running_.load(std::memory_order_acquire)) {
        // Simulate new connections periodically
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::seconds>(now - last_connection_simulation).count() >= 5) {
            // Simulate a new connection every 5 seconds (for demo)
            auto id = add_connection();
            if (connection_callback_) {
                connection_callback_(id, true);
            }
            last_connection_simulation = now;
        }
        
        // Check for disconnections and timeouts
        {
            std::lock_guard<std::mutex> lock(connections_mutex_);
            auto it = connections_.begin();
            while (it != connections_.end()) {
                auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                    now - it->second->get_last_activity()).count();
                
                if (elapsed > 30) { // 30 second timeout
                    if (connection_callback_) {
                        connection_callback_(it->first, false);
                    }
                    it = connections_.erase(it);
                } else {
                    ++it;
                }
            }
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    std::cout << "[WebSocketServer] Server loop ended" << std::endl;
}

void WebSocketServer::streaming_loop() {
    std::cout << "[WebSocketServer] Streaming loop started" << std::endl;
    
    const auto update_interval = std::chrono::microseconds(
        static_cast<int64_t>(1000000.0f / config_.update_frequency_hz));
    
    while (running_.load(std::memory_order_acquire)) {
        const auto start_time = std::chrono::steady_clock::now();
        
        // Broadcast updates if telemetry is available
        if (telemetry_) {
            broadcast_metrics_update();
            
            // Send heartbeats periodically
            static auto last_heartbeat = std::chrono::steady_clock::now();
            if (std::chrono::duration_cast<std::chrono::seconds>(start_time - last_heartbeat).count() >= 10) {
                send_heartbeats();
                last_heartbeat = start_time;
            }
        }
        
        // Update statistics
        update_stats();
        
        // Sleep until next update
        const auto elapsed = std::chrono::steady_clock::now() - start_time;
        const auto sleep_duration = update_interval - elapsed;
        if (sleep_duration > std::chrono::nanoseconds::zero()) {
            std::this_thread::sleep_for(sleep_duration);
        }
    }
    
    std::cout << "[WebSocketServer] Streaming loop ended" << std::endl;
}

void WebSocketServer::worker_loop(int worker_id) {
    std::cout << "[WebSocketServer] Worker " << worker_id << " started" << std::endl;
    
    while (running_.load(std::memory_order_acquire)) {
        // Process WebSocket messages, handle I/O, etc.
        // For demo, just sleep
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    std::cout << "[WebSocketServer] Worker " << worker_id << " ended" << std::endl;
}

WebSocketConnection::ConnectionId WebSocketServer::add_connection() {
    auto id = next_connection_id_.fetch_add(1, std::memory_order_relaxed);
    auto connection = std::make_unique<WebSocketConnection>(id);
    connection->set_connected(true);
    
    {
        std::lock_guard<std::mutex> lock(connections_mutex_);
        connections_[id] = std::move(connection);
        
        std::lock_guard<std::mutex> stats_lock(stats_mutex_);
        stats_.total_connections++;
        stats_.active_connections = connections_.size();
    }
    
    std::cout << "[WebSocketServer] Client " << id << " connected (total: " 
              << get_connection_count() << ")" << std::endl;
    
    return id;
}

void WebSocketServer::remove_connection(WebSocketConnection::ConnectionId id) {
    std::lock_guard<std::mutex> lock(connections_mutex_);
    connections_.erase(id);
    
    std::lock_guard<std::mutex> stats_lock(stats_mutex_);
    stats_.active_connections = connections_.size();
    
    std::cout << "[WebSocketServer] Client " << id << " disconnected" << std::endl;
}

void WebSocketServer::handle_client_message(WebSocketConnection::ConnectionId id, const std::string& message) {
    auto it = connections_.find(id);
    if (it != connections_.end()) {
        // Parse subscription changes
        auto subscription = parse_subscription_message(message);
        it->second->set_subscription(subscription);
        
        if (message_callback_) {
            message_callback_(id, message);
        }
    }
}

void WebSocketServer::broadcast_metrics_update() {
    if (!telemetry_) return;
    
    auto snapshot = telemetry_->get_snapshot();
    auto data = serialize_snapshot(snapshot);
    broadcast_message(data, MessageType::METRICS_UPDATE);
}

void WebSocketServer::broadcast_market_update() {
    if (!telemetry_) return;
    
    auto market_state = telemetry_->get_market_state();
    auto data = serialize_market_state(market_state);
    broadcast_message(data, MessageType::MARKET_UPDATE);
}

void WebSocketServer::send_heartbeats() {
    auto heartbeat = create_heartbeat_message();
    std::vector<uint8_t> data(heartbeat.begin(), heartbeat.end());
    broadcast_message(data, MessageType::HEARTBEAT);
}

void WebSocketServer::update_stats() {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - stats_.start_time).count();
    
    if (elapsed > 0) {
        stats_.messages_per_second = static_cast<float>(stats_.total_messages_sent) / elapsed;
        stats_.bytes_per_second = static_cast<float>(stats_.total_bytes_sent) / elapsed;
    }
}

void WebSocketServer::handle_error(const std::string& error) {
    std::cout << "[WebSocketServer] Error: " << error << std::endl;
    
    if (error_callback_) {
        error_callback_(error);
    }
}

std::string WebSocketServer::get_client_info(WebSocketConnection::ConnectionId id) const {
    std::ostringstream oss;
    oss << "Client " << id;
    return oss.str();
}

void WebSocketServer::log_connection_event(WebSocketConnection::ConnectionId id, const std::string& event) {
    std::cout << "[WebSocketServer] " << get_client_info(id) << ": " << event << std::endl;
}

} // namespace hfx::viz
