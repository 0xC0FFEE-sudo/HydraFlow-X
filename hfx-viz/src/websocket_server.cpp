/**
 * @file websocket_server.cpp
 * @brief Implementation of high-performance WebSocket server for HFT data streaming using Mongoose
 */

#include "websocket_server.hpp"
#include "hfx-log/include/logger.hpp"
#include "mongoose.h"
#include <iostream>
#include <sstream>
#include <thread>
#include <chrono>
#include <unordered_map>
#include <mutex>
#include <atomic>

namespace hfx::viz {

// Global Mongoose manager and connections
static mg_mgr s_mgr;
static std::unordered_map<WebSocketConnection::ConnectionId, mg_connection*> s_connections;
static std::mutex s_connections_mutex;
static std::atomic<WebSocketConnection::ConnectionId> s_next_connection_id{1};

WebSocketConnection::WebSocketConnection(ConnectionId id) : id_(id) {
    update_activity();
}

void WebSocketConnection::send_binary(const std::vector<uint8_t>& data) {
    std::lock_guard<std::mutex> lock(s_connections_mutex);
    auto it = s_connections.find(id_);
    if (it != s_connections.end()) {
        mg_ws_send(it->second, data.data(), data.size(), WEBSOCKET_OP_BINARY);
        bytes_sent_ += data.size();
        update_activity();
    }
}

void WebSocketConnection::send_text(const std::string& data) {
    std::lock_guard<std::mutex> lock(s_connections_mutex);
    auto it = s_connections.find(id_);
    if (it != s_connections.end()) {
        mg_ws_send(it->second, data.c_str(), data.size(), WEBSOCKET_OP_TEXT);
        bytes_sent_ += data.size();
        update_activity();
    }
}

void WebSocketConnection::send_ping() {
    std::lock_guard<std::mutex> lock(s_connections_mutex);
    auto it = s_connections.find(id_);
    if (it != s_connections.end()) {
        mg_ws_send(it->second, "", 0, WEBSOCKET_OP_PING);
        update_activity();
    }
}

WebSocketServer::WebSocketServer(const WebSocketConfig& config)
    : config_(config) {
    stats_.start_time = std::chrono::steady_clock::now();
    mg_mgr_init(&s_mgr);
}

WebSocketServer::~WebSocketServer() {
    stop();
    mg_mgr_free(&s_mgr);
}

// Mongoose event handler
static void mongoose_event_handler(mg_connection *c, int ev, void *ev_data) {
    WebSocketServer *server = static_cast<WebSocketServer*>(c->fn_data);

    if (ev == MG_EV_WS_OPEN) {
        // WebSocket connection opened
        std::lock_guard<std::mutex> lock(s_connections_mutex);
        WebSocketConnection::ConnectionId id = s_next_connection_id.fetch_add(1);
        s_connections[id] = c;

        // Store connection ID in mongoose connection
        c->fn_data = reinterpret_cast<void*>(static_cast<uintptr_t>(id));

        HFX_LOG_INFO("[WebSocketServer] Client connected, ID: " + std::to_string(id));

        auto conn_callback = server->get_connection_callback();
        if (conn_callback) {
            conn_callback(id, true);
        }

    } else if (ev == MG_EV_WS_MSG) {
        // WebSocket message received
        mg_ws_message *wm = (mg_ws_message *) ev_data;
        WebSocketConnection::ConnectionId id = static_cast<WebSocketConnection::ConnectionId>(
            reinterpret_cast<uintptr_t>(c->fn_data)
        );

        std::string message(wm->data.buf, wm->data.len);
        auto msg_callback = server->get_message_callback();
        if (msg_callback) {
            msg_callback(id, message);
        }

    } else if (ev == MG_EV_CLOSE) {
        // Connection closed
        WebSocketConnection::ConnectionId id = static_cast<WebSocketConnection::ConnectionId>(
            reinterpret_cast<uintptr_t>(c->fn_data)
        );

        {
            std::lock_guard<std::mutex> lock(s_connections_mutex);
            s_connections.erase(id);
        }

        HFX_LOG_INFO("[WebSocketServer] Client disconnected, ID: " + std::to_string(id));

        auto conn_callback = server->get_connection_callback();
        if (conn_callback) {
            conn_callback(id, false);
        }

    } else if (ev == MG_EV_HTTP_MSG) {
        // HTTP request - handle WebSocket upgrade
        mg_http_message *hm = (mg_http_message *) ev_data;
        if (mg_strcmp(hm->uri, mg_str("/ws")) == 0) {
            mg_ws_upgrade(c, hm, NULL);
        } else {
            // Serve basic HTML for testing
            mg_http_reply(c, 200, "Content-Type: text/html\r\n",
                "<html><body><h1>HydraFlow-X WebSocket Server</h1>"
                "<script>"
                "var ws = new WebSocket('ws://' + window.location.host + '/ws');"
                "ws.onmessage = function(e) { console.log('Received:', e.data); };"
                "ws.onopen = function() { console.log('Connected'); };"
                "</script></body></html>");
        }
    }
}

bool WebSocketServer::start() {
    if (running_.load(std::memory_order_acquire)) {
        return true;
    }

    HFX_LOG_INFO("[WebSocketServer] Starting server on " + config_.bind_address
              + ":" + std::to_string(config_.port));

    std::string listen_addr = "http://" + config_.bind_address + ":" + std::to_string(config_.port);

    // Start Mongoose listener
    mg_connection *c = mg_http_listen(&s_mgr, listen_addr.c_str(), mongoose_event_handler, NULL);
    if (c == NULL) {
        HFX_LOG_ERROR("[WebSocketServer] Failed to start server on " + listen_addr);
        return false;
    }

    // Store server reference in connection
    c->fn_data = this;

    running_.store(true, std::memory_order_release);

    // Start server thread
    server_thread_ = std::make_unique<std::thread>(&WebSocketServer::server_loop, this);

    // Start streaming thread
    streaming_thread_ = std::make_unique<std::thread>(&WebSocketServer::streaming_loop, this);

    HFX_LOG_INFO("[WebSocketServer] Started successfully on " + listen_addr);

    return true;
}

void WebSocketServer::stop() {
    if (!running_.load(std::memory_order_acquire)) {
        return;
    }
    
    HFX_LOG_INFO("[WebSocketServer] Stopping server...");
    
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
        std::lock_guard<std::mutex> lock(s_connections_mutex);
        s_connections.clear();
    }
    
    HFX_LOG_INFO("[WebSocketServer] Server stopped");
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
    std::lock_guard<std::mutex> lock(s_connections_mutex);
    return s_connections.size();
}

std::vector<WebSocketConnection::ConnectionId> WebSocketServer::get_active_connections() const {
    std::lock_guard<std::mutex> lock(s_connections_mutex);
    std::vector<WebSocketConnection::ConnectionId> ids;
    for (const auto& [id, conn] : s_connections) {
        ids.push_back(id);
    }
    return ids;
}

void WebSocketServer::disconnect_client(WebSocketConnection::ConnectionId id) {
    std::lock_guard<std::mutex> lock(s_connections_mutex);
    auto it = s_connections.find(id);
    if (it != s_connections.end()) {
        mg_ws_send(it->second, "", 0, WEBSOCKET_OP_CLOSE);
        s_connections.erase(it);

        if (connection_callback_) {
            connection_callback_(id, false);
        }
    }
}

void WebSocketServer::broadcast_message(const std::vector<uint8_t>& data, MessageType type) {
    std::lock_guard<std::mutex> lock(s_connections_mutex);

    for (const auto& [id, conn] : s_connections) {
        // Add message type header
        std::vector<uint8_t> message;
        message.push_back(static_cast<uint8_t>(type));
        message.insert(message.end(), data.begin(), data.end());

        mg_ws_send(conn, message.data(), message.size(), WEBSOCKET_OP_BINARY);
    }

    // Update statistics
    {
        std::lock_guard<std::mutex> stats_lock(stats_mutex_);
        stats_.total_messages_sent += s_connections.size();
        stats_.total_bytes_sent += data.size() * s_connections.size();
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
    HFX_LOG_INFO("[WebSocketServer] Server loop started");

    while (running_.load(std::memory_order_acquire)) {
        // Poll for events (non-blocking)
        mg_mgr_poll(&s_mgr, 100); // Poll every 100ms

        // Check for connection timeouts
        auto now = std::chrono::steady_clock::now();
        {
            std::lock_guard<std::mutex> lock(s_connections_mutex);
            auto it = s_connections.begin();
            while (it != s_connections.end()) {
                auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                    now - std::chrono::steady_clock::now()).count(); // Simplified timeout check

                if (elapsed > 30) { // 30 second timeout
                    if (connection_callback_) {
                        connection_callback_(it->first, false);
                    }
                    it = s_connections.erase(it);
                } else {
                    ++it;
                }
            }
        }

        // Small delay to prevent busy waiting
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    HFX_LOG_INFO("[WebSocketServer] Server loop ended");
}

void WebSocketServer::streaming_loop() {
    HFX_LOG_INFO("[WebSocketServer] Streaming loop started");

    const auto update_interval = std::chrono::microseconds(
        static_cast<int64_t>(1000000.0f / config_.update_frequency_hz));

    while (running_.load(std::memory_order_acquire)) {
        const auto start_time = std::chrono::steady_clock::now();

        // Broadcast updates if telemetry is available and we have active connections
        if (telemetry_ && get_connection_count() > 0) {
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

    HFX_LOG_INFO("[WebSocketServer] Streaming loop ended");
}

void WebSocketServer::worker_loop(int worker_id) {
    HFX_LOG_INFO("[WebSocketServer] Worker " + std::to_string(worker_id) + " started");
    
    while (running_.load(std::memory_order_acquire)) {
        // Process WebSocket messages, handle I/O, etc.
        // For demo, just sleep
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    HFX_LOG_INFO("[WebSocketServer] Worker " + std::to_string(worker_id) + " ended");
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
    
    HFX_LOG_INFO("[WebSocketServer] Client " + std::to_string(id) + " connected (total: " + std::to_string(connections_.size()) + ")"); 
              << get_connection_count() << ")" << std::endl;
    
    return id;
}

void WebSocketServer::remove_connection(WebSocketConnection::ConnectionId id) {
    std::lock_guard<std::mutex> lock(connections_mutex_);
    connections_.erase(id);
    
    std::lock_guard<std::mutex> stats_lock(stats_mutex_);
    stats_.active_connections = connections_.size();
    
    HFX_LOG_INFO("[WebSocketServer] Client " + std::to_string(id) + " disconnected");
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
    HFX_LOG_INFO("[WebSocketServer] Error: " + error);
    
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
    HFX_LOG_INFO("[WebSocketServer] " + get_client_info(id) + ": " + event);
}

} // namespace hfx::viz
