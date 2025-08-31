#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include <optional>
#include <nlohmann/json.hpp>

namespace hfx::config {

enum class ConfigSource {
    ENVIRONMENT,
    JSON_FILE,
    WEB_INTERFACE,
    DEFAULT
};

struct APIConfig {
    std::string provider;
    std::string api_key;
    std::string secret_key;
    std::string base_url;
    int rate_limit_per_second = 100;
    bool enabled = true;
    std::unordered_map<std::string, std::string> custom_headers;
    std::unordered_map<std::string, std::string> additional_params;
};

struct RPCConfig {
    std::string chain;
    std::string provider;
    std::string endpoint;
    std::string api_key;
    int max_connections = 10;
    int timeout_ms = 5000;
    bool websocket_enabled = true;
    std::string websocket_endpoint;
    bool backup_enabled = false;
    std::string backup_endpoint;
};

struct TradingConfig {
    std::string primary_chain = "ethereum";
    std::vector<std::string> supported_chains = {"ethereum", "solana", "base", "arbitrum"};
    double max_position_size_usd = 10000.0;
    double max_slippage_percent = 2.0;
    double min_profit_threshold_percent = 0.5;
    int max_concurrent_trades = 5;
    bool enable_mev_protection = true;
    bool enable_frontrun_protection = true;
    bool enable_sandwich_protection = true;
    std::string default_token_out = "USDC";
    double gas_price_multiplier = 1.2;
    int max_gas_price_gwei = 100;
};

struct SecurityConfig {
    bool enable_hsm = false;
    std::string hsm_provider = "software";
    std::string encryption_algorithm = "AES-256-GCM";
    bool enable_audit_logging = true;
    std::string audit_log_path = "./logs/audit.log";
    bool enable_rate_limiting = true;
    bool enable_ip_whitelist = false;
    std::vector<std::string> allowed_ips;
    std::string jwt_secret = "";
    int jwt_expiry_hours = 24;
};

struct DatabaseConfig {
    std::string type = "postgresql";
    std::string host = "localhost";
    int port = 5432;
    std::string database = "hydraflow";
    std::string username = "hydraflow";
    std::string password = "";
    int max_connections = 20;
    bool enable_ssl = true;
    
    // ClickHouse for analytics
    std::string clickhouse_host = "localhost";
    int clickhouse_port = 8123;
    std::string clickhouse_database = "hydraflow_analytics";
    std::string clickhouse_username = "default";
    std::string clickhouse_password = "";
    
    // Redis for caching
    std::string redis_host = "localhost";
    int redis_port = 6379;
    std::string redis_password = "";
    int redis_db = 0;
};

struct MonitoringConfig {
    bool enable_prometheus = true;
    int prometheus_port = 9090;
    bool enable_grafana = true;
    int grafana_port = 3000;
    std::string log_level = "INFO";
    std::string log_format = "JSON";
    std::string log_output = "file";
    std::string log_file_path = "./logs/hydraflow.log";
    int max_log_file_size_mb = 100;
    int max_log_files = 10;
};

struct WebDashboardConfig {
    bool enabled = true;
    int port = 8080;
    std::string host = "0.0.0.0";
    bool enable_ssl = false;
    std::string ssl_cert_path = "";
    std::string ssl_key_path = "";
    bool enable_auth = true;
    std::string default_username = "admin";
    std::string default_password = "changeme";
    std::string static_files_path = "./web/static";
    bool enable_api = true;
    std::string api_prefix = "/api/v1";
};

class ConfigManager {
public:
    static ConfigManager& instance();
    
    // Core configuration loading
    bool load_config(const std::string& config_file_path = "");
    bool save_config(const std::string& config_file_path = "");
    bool reload_config();
    
    // API configuration
    void add_api_config(const std::string& name, const APIConfig& config);
    std::optional<APIConfig> get_api_config(const std::string& name) const;
    std::vector<std::string> get_configured_apis() const;
    bool remove_api_config(const std::string& name);
    
    // RPC configuration
    void add_rpc_config(const std::string& name, const RPCConfig& config);
    std::optional<RPCConfig> get_rpc_config(const std::string& name) const;
    std::vector<std::string> get_configured_rpcs() const;
    bool remove_rpc_config(const std::string& name);
    
    // Core configurations
    TradingConfig& get_trading_config() { return trading_config_; }
    const TradingConfig& get_trading_config() const { return trading_config_; }
    
    SecurityConfig& get_security_config() { return security_config_; }
    const SecurityConfig& get_security_config() const { return security_config_; }
    
    DatabaseConfig& get_database_config() { return database_config_; }
    const DatabaseConfig& get_database_config() const { return database_config_; }
    
    MonitoringConfig& get_monitoring_config() { return monitoring_config_; }
    const MonitoringConfig& get_monitoring_config() const { return monitoring_config_; }
    
    WebDashboardConfig& get_web_dashboard_config() { return web_dashboard_config_; }
    const WebDashboardConfig& get_web_dashboard_config() const { return web_dashboard_config_; }
    
    // Environment variable helpers
    std::string get_env_var(const std::string& name, const std::string& default_value = "") const;
    bool get_env_bool(const std::string& name, bool default_value = false) const;
    int get_env_int(const std::string& name, int default_value = 0) const;
    double get_env_double(const std::string& name, double default_value = 0.0) const;
    
    // Validation
    bool validate_config() const;
    std::vector<std::string> get_validation_errors() const;
    
    // Configuration export/import
    nlohmann::json export_config() const;
    bool import_config(const nlohmann::json& config_json);
    
    // Web interface support
    nlohmann::json get_config_schema() const;
    bool update_config_from_web(const nlohmann::json& updates);
    
    // Security features
    bool encrypt_sensitive_data();
    bool decrypt_sensitive_data();
    std::string get_config_hash() const;
    
private:
    ConfigManager() = default;
    
    void load_from_environment();
    void load_from_json(const nlohmann::json& json);
    void apply_defaults();
    bool validate_api_config(const APIConfig& config) const;
    bool validate_rpc_config(const RPCConfig& config) const;
    
    std::unordered_map<std::string, APIConfig> api_configs_;
    std::unordered_map<std::string, RPCConfig> rpc_configs_;
    
    TradingConfig trading_config_;
    SecurityConfig security_config_;
    DatabaseConfig database_config_;
    MonitoringConfig monitoring_config_;
    WebDashboardConfig web_dashboard_config_;
    
    std::string config_file_path_;
    mutable std::vector<std::string> validation_errors_;
    bool config_loaded_ = false;
};

// Configuration templates
namespace templates {
    APIConfig create_twitter_api_template();
    APIConfig create_reddit_api_template();
    APIConfig create_dexscreener_api_template();
    APIConfig create_gmgn_api_template();
    
    RPCConfig create_ethereum_rpc_template(const std::string& provider = "alchemy");
    RPCConfig create_solana_rpc_template(const std::string& provider = "helius");
    RPCConfig create_base_rpc_template();
    RPCConfig create_arbitrum_rpc_template();
    
    nlohmann::json create_full_config_template();
}

// Utility macros for easy config access
#define GET_CONFIG() hfx::config::ConfigManager::instance()
#define GET_API_CONFIG(name) GET_CONFIG().get_api_config(name)
#define GET_RPC_CONFIG(name) GET_CONFIG().get_rpc_config(name)
#define GET_TRADING_CONFIG() GET_CONFIG().get_trading_config()
#define GET_SECURITY_CONFIG() GET_CONFIG().get_security_config()
#define GET_DATABASE_CONFIG() GET_CONFIG().get_database_config()
#define GET_MONITORING_CONFIG() GET_CONFIG().get_monitoring_config()
#define GET_WEB_CONFIG() GET_CONFIG().get_web_dashboard_config()

} // namespace hfx::config
