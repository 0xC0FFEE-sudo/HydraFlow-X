#pragma once

#include <string>
#include <memory>
#include <thread>
#include <atomic>
#include <functional>
#include <map>
#include <vector>
#include <mutex>
#include <nlohmann/json.hpp>

namespace hfx {
namespace api {

// Forward declarations
class WebSocketManager;
class TradingController;
class ConfigController;
class MonitoringController;
class WebSocketServerImpl;

// HTTP Request/Response structures
struct HttpRequest {
    std::string method;
    std::string path;
    std::string query_string;
    std::map<std::string, std::string> headers;
    std::string body;
    std::map<std::string, std::string> params;
};

struct HttpResponse {
    int status_code = 200;
    std::map<std::string, std::string> headers;
    std::string body;
    std::string content_type = "application/json";
};

// Route handler type
using RouteHandler = std::function<HttpResponse(const HttpRequest&)>;

// Middleware type
using Middleware = std::function<bool(HttpRequest&, HttpResponse&)>;

class RestApiServer {
public:
    struct Config {
        std::string host = "0.0.0.0";
        int port = 8080;
        int worker_threads = 4;
        int max_connections = 1000;
        bool enable_cors = true;
        bool enable_ssl = false;
        std::string ssl_cert_path;
        std::string ssl_key_path;
        std::string static_files_path;
        bool enable_websocket = true;
        int websocket_port = 8081;
    };

    explicit RestApiServer(const Config& config);
    ~RestApiServer();

    // Server lifecycle
    bool start();
    void stop();
    bool is_running() const;

    // Route registration
    void register_route(const std::string& method, const std::string& path, RouteHandler handler);
    void register_middleware(Middleware middleware);

    // Controller registration
    void register_trading_controller(std::shared_ptr<TradingController> controller);
    void register_config_controller(std::shared_ptr<ConfigController> controller);
    void register_monitoring_controller(std::shared_ptr<MonitoringController> controller);

    // WebSocket management
    void set_websocket_manager(std::shared_ptr<WebSocketManager> ws_manager);

    // Health check
    bool health_check() const;

private:
    Config config_;
    std::atomic<bool> running_;
    std::vector<std::thread> worker_threads_;
    std::unique_ptr<class HttpServer> http_server_;
    std::shared_ptr<WebSocketManager> websocket_manager_;
    
    // Controllers
    std::shared_ptr<TradingController> trading_controller_;
    std::shared_ptr<ConfigController> config_controller_;
    std::shared_ptr<MonitoringController> monitoring_controller_;

    // Route management
    std::map<std::string, RouteHandler> routes_;
    std::vector<Middleware> middlewares_;
    std::mutex routes_mutex_;

    // Built-in route handlers
    void setup_default_routes();
    HttpResponse handle_health(const HttpRequest& request);
    HttpResponse handle_metrics(const HttpRequest& request);
    HttpResponse handle_static_file(const HttpRequest& request);
    HttpResponse handle_not_found(const HttpRequest& request);
    HttpResponse handle_cors_preflight(const HttpRequest& request);

    // Route matching
    RouteHandler find_route_handler(const std::string& method, const std::string& path);
    bool matches_route_pattern(const std::string& pattern, const std::string& path, 
                              std::map<std::string, std::string>& params);

    // Request processing
    HttpResponse process_request(const HttpRequest& request);
    bool apply_middlewares(HttpRequest& request, HttpResponse& response);

    // CORS handling
    void add_cors_headers(HttpResponse& response);

    // Error handling
    HttpResponse create_error_response(int status_code, const std::string& message);
    HttpResponse create_json_response(const nlohmann::json& data, int status_code = 200);

    // Client connection handling
    void handle_client_connection(int client_fd);

    // Logging
    void log_request(const HttpRequest& request, const HttpResponse& response);
};

// Trading API Controller
class TradingController {
public:
    explicit TradingController();
    ~TradingController();

    // Trading operations
    HttpResponse start_trading(const HttpRequest& request);
    HttpResponse stop_trading(const HttpRequest& request);
    HttpResponse get_trading_status(const HttpRequest& request);
    HttpResponse place_order(const HttpRequest& request);
    HttpResponse cancel_order(const HttpRequest& request);
    HttpResponse get_positions(const HttpRequest& request);
    HttpResponse get_trades(const HttpRequest& request);
    HttpResponse get_performance_metrics(const HttpRequest& request);

    // Strategy management
    HttpResponse get_strategies(const HttpRequest& request);
    HttpResponse create_strategy(const HttpRequest& request);
    HttpResponse update_strategy(const HttpRequest& request);
    HttpResponse delete_strategy(const HttpRequest& request);

    // Wallet management
    HttpResponse get_wallets(const HttpRequest& request);
    HttpResponse add_wallet(const HttpRequest& request);
    HttpResponse remove_wallet(const HttpRequest& request);
    HttpResponse update_wallet(const HttpRequest& request);

private:
    std::atomic<bool> trading_active_;
    std::string current_strategy_;
    mutable std::mutex state_mutex_;

    // Helper methods
    nlohmann::json get_trading_state() const;
    nlohmann::json serialize_position(const struct Position& position) const;
    nlohmann::json serialize_trade(const struct Trade& trade) const;
    bool validate_order_request(const nlohmann::json& order_data) const;
    
    // Response helpers
    HttpResponse create_json_response(const nlohmann::json& data, int status_code = 200);
};

// Configuration API Controller
class ConfigController {
public:
    explicit ConfigController();
    ~ConfigController();

    // Configuration management
    HttpResponse get_config(const HttpRequest& request);
    HttpResponse update_config(const HttpRequest& request);
    HttpResponse test_connection(const HttpRequest& request);
    HttpResponse get_api_status(const HttpRequest& request);
    HttpResponse update_api_config(const HttpRequest& request);
    HttpResponse get_rpc_status(const HttpRequest& request);
    HttpResponse update_rpc_config(const HttpRequest& request);

private:
    mutable std::mutex config_mutex_;
    nlohmann::json current_config_;

    // Helper methods
    bool validate_config(const nlohmann::json& config) const;
    bool test_api_connection(const std::string& provider, const nlohmann::json& config) const;
    bool test_rpc_connection(const std::string& chain, const nlohmann::json& config) const;
    void save_config_to_file() const;
    void load_config_from_file();
    
    // Response helpers
    HttpResponse create_json_response(const nlohmann::json& data, int status_code = 200);
};

// Monitoring API Controller
class MonitoringController {
public:
    explicit MonitoringController();
    ~MonitoringController();

    // System monitoring
    HttpResponse get_system_status(const HttpRequest& request);
    HttpResponse get_performance_metrics(const HttpRequest& request);
    HttpResponse get_latency_metrics(const HttpRequest& request);
    HttpResponse get_mev_protection_status(const HttpRequest& request);
    HttpResponse get_alerts(const HttpRequest& request);
    HttpResponse acknowledge_alert(const HttpRequest& request);

    // Real-time data
    HttpResponse get_market_data(const HttpRequest& request);
    HttpResponse get_sentiment_data(const HttpRequest& request);

private:
    mutable std::mutex metrics_mutex_;
    
    struct SystemMetrics {
        double cpu_usage;
        double memory_usage;
        int active_connections;
        double avg_latency;
        int total_trades;
        double success_rate;
        double uptime_hours;
    };

    SystemMetrics current_metrics_;

    // Helper methods
    SystemMetrics collect_system_metrics() const;
    nlohmann::json serialize_metrics(const SystemMetrics& metrics) const;
    std::vector<nlohmann::json> get_recent_alerts() const;
    
    // Response helpers
    HttpResponse create_json_response(const nlohmann::json& data, int status_code = 200);
};

// WebSocket Manager for real-time communication
class WebSocketManager {
public:
    struct Config {
        int port = 8081;
        int max_connections = 500;
        int ping_interval_ms = 30000;
        int max_message_size = 1024 * 1024; // 1MB
    };

    explicit WebSocketManager(const Config& config);
    ~WebSocketManager();

    // Lifecycle
    bool start();
    void stop();
    bool is_running() const;

    // Broadcasting
    void broadcast_system_metrics(const nlohmann::json& metrics);
    void broadcast_trading_update(const nlohmann::json& update);
    void broadcast_market_data(const nlohmann::json& data);
    void broadcast_alert(const nlohmann::json& alert);

    // Connection management
    size_t get_connection_count() const;
    void disconnect_all();

private:
    Config config_;
    std::atomic<bool> running_;
    std::thread server_thread_;
    std::unique_ptr<class WebSocketServerImpl> server_impl_;

    // Message handling
    void handle_client_message(int connection_id, const std::string& message);
    void handle_client_connect(int connection_id);
    void handle_client_disconnect(int connection_id);

    // Broadcasting implementation
    void broadcast_to_all(const std::string& message);
    void send_to_connection(int connection_id, const std::string& message);
};

} // namespace api
} // namespace hfx
