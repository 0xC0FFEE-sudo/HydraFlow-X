#include "config_manager.hpp"
#include <fstream>
#include <iostream>
#include <cstdlib>
#include <regex>
#include <filesystem>
#include <openssl/sha.h>
#include <openssl/evp.h>
#include <openssl/rand.h>

namespace hfx::config {

ConfigManager& ConfigManager::instance() {
    static ConfigManager instance;
    return instance;
}

bool ConfigManager::load_config(const std::string& config_file_path) {
    validation_errors_.clear();
    
    // Set config file path
    if (!config_file_path.empty()) {
        config_file_path_ = config_file_path;
    } else if (config_file_path_.empty()) {
        config_file_path_ = "config/hydraflow.json";
    }
    
    // Apply defaults first
    apply_defaults();
    
    // Load from environment variables
    load_from_environment();
    
    // Load from JSON file if it exists
    if (std::filesystem::exists(config_file_path_)) {
        try {
            std::ifstream file(config_file_path_);
            nlohmann::json config_json;
            file >> config_json;
            load_from_json(config_json);
        } catch (const std::exception& e) {
            validation_errors_.push_back("Failed to load JSON config: " + std::string(e.what()));
            return false;
        }
    } else {
        HFX_LOG_INFO("Config file not found at " << config_file_path_ << ", using environment variables and defaults." << std::endl;
    }
    
    // Validate configuration
    bool is_valid = validate_config();
    if (is_valid) {
        config_loaded_ = true;
        HFX_LOG_INFO("Configuration loaded successfully.");
    } else {
        HFX_LOG_INFO("Configuration validation failed:");
        for (const auto& error : validation_errors_) {
            HFX_LOG_INFO("  - " << error << std::endl;
        }
    }
    
    return is_valid;
}

bool ConfigManager::save_config(const std::string& config_file_path) {
    std::string path = config_file_path.empty() ? config_file_path_ : config_file_path;
    
    try {
        // Create directory if it doesn't exist
        std::filesystem::create_directories(std::filesystem::path(path).parent_path());
        
        nlohmann::json config_json = export_config();
        
        std::ofstream file(path);
        file << config_json.dump(4);
        
        HFX_LOG_INFO("Configuration saved to " << path << std::endl;
        return true;
    } catch (const std::exception& e) {
        HFX_LOG_ERROR("Failed to save config: " << e.what() << std::endl;
        return false;
    }
}

bool ConfigManager::reload_config() {
    return load_config(config_file_path_);
}

void ConfigManager::add_api_config(const std::string& name, const APIConfig& config) {
    if (validate_api_config(config)) {
        api_configs_[name] = config;
    }
}

std::optional<APIConfig> ConfigManager::get_api_config(const std::string& name) const {
    auto it = api_configs_.find(name);
    if (it != api_configs_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::vector<std::string> ConfigManager::get_configured_apis() const {
    std::vector<std::string> names;
    for (const auto& [name, config] : api_configs_) {
        names.push_back(name);
    }
    return names;
}

bool ConfigManager::remove_api_config(const std::string& name) {
    return api_configs_.erase(name) > 0;
}

void ConfigManager::add_rpc_config(const std::string& name, const RPCConfig& config) {
    if (validate_rpc_config(config)) {
        rpc_configs_[name] = config;
    }
}

std::optional<RPCConfig> ConfigManager::get_rpc_config(const std::string& name) const {
    auto it = rpc_configs_.find(name);
    if (it != rpc_configs_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::vector<std::string> ConfigManager::get_configured_rpcs() const {
    std::vector<std::string> names;
    for (const auto& [name, config] : rpc_configs_) {
        names.push_back(name);
    }
    return names;
}

bool ConfigManager::remove_rpc_config(const std::string& name) {
    return rpc_configs_.erase(name) > 0;
}

std::string ConfigManager::get_env_var(const std::string& name, const std::string& default_value) const {
    const char* value = std::getenv(name.c_str());
    return value ? std::string(value) : default_value;
}

bool ConfigManager::get_env_bool(const std::string& name, bool default_value) const {
    std::string value = get_env_var(name);
    if (value.empty()) return default_value;
    
    std::transform(value.begin(), value.end(), value.begin(), ::tolower);
    return value == "true" || value == "1" || value == "yes" || value == "on";
}

int ConfigManager::get_env_int(const std::string& name, int default_value) const {
    std::string value = get_env_var(name);
    if (value.empty()) return default_value;
    
    try {
        return std::stoi(value);
    } catch (const std::exception&) {
        return default_value;
    }
}

double ConfigManager::get_env_double(const std::string& name, double default_value) const {
    std::string value = get_env_var(name);
    if (value.empty()) return default_value;
    
    try {
        return std::stod(value);
    } catch (const std::exception&) {
        return default_value;
    }
}

void ConfigManager::load_from_environment() {
    // Trading configuration
    trading_config_.primary_chain = get_env_var("HFX_PRIMARY_CHAIN", trading_config_.primary_chain);
    trading_config_.max_position_size_usd = get_env_double("HFX_MAX_POSITION_SIZE", trading_config_.max_position_size_usd);
    trading_config_.max_slippage_percent = get_env_double("HFX_MAX_SLIPPAGE", trading_config_.max_slippage_percent);
    trading_config_.min_profit_threshold_percent = get_env_double("HFX_MIN_PROFIT", trading_config_.min_profit_threshold_percent);
    trading_config_.max_concurrent_trades = get_env_int("HFX_MAX_CONCURRENT_TRADES", trading_config_.max_concurrent_trades);
    trading_config_.enable_mev_protection = get_env_bool("HFX_ENABLE_MEV_PROTECTION", trading_config_.enable_mev_protection);
    
    // Database configuration
    database_config_.host = get_env_var("HFX_DB_HOST", database_config_.host);
    database_config_.port = get_env_int("HFX_DB_PORT", database_config_.port);
    database_config_.database = get_env_var("HFX_DB_NAME", database_config_.database);
    database_config_.username = get_env_var("HFX_DB_USER", database_config_.username);
    database_config_.password = get_env_var("HFX_DB_PASSWORD", database_config_.password);
    
    // Web dashboard configuration
    web_dashboard_config_.port = get_env_int("HFX_WEB_PORT", web_dashboard_config_.port);
    web_dashboard_config_.host = get_env_var("HFX_WEB_HOST", web_dashboard_config_.host);
    web_dashboard_config_.enable_auth = get_env_bool("HFX_WEB_AUTH", web_dashboard_config_.enable_auth);
    
    // Security configuration
    security_config_.jwt_secret = get_env_var("HFX_JWT_SECRET", security_config_.jwt_secret);
    security_config_.enable_hsm = get_env_bool("HFX_ENABLE_HSM", security_config_.enable_hsm);
    
    // Load API configurations from environment
    std::vector<std::string> api_providers = {"twitter", "reddit", "dexscreener", "gmgn"};
    for (const auto& provider : api_providers) {
        std::string prefix = "HFX_" + provider;
        std::transform(prefix.begin(), prefix.end(), prefix.begin(), ::toupper);
        
        std::string api_key = get_env_var(prefix + "_API_KEY");
        if (!api_key.empty()) {
            APIConfig config;
            config.provider = provider;
            config.api_key = api_key;
            config.secret_key = get_env_var(prefix + "_SECRET_KEY");
            config.base_url = get_env_var(prefix + "_BASE_URL");
            config.rate_limit_per_second = get_env_int(prefix + "_RATE_LIMIT", 100);
            config.enabled = get_env_bool(prefix + "_ENABLED", true);
            
            api_configs_[provider] = config;
        }
    }
    
    // Load RPC configurations from environment
    std::vector<std::string> rpc_providers = {"ethereum", "solana", "base", "arbitrum"};
    for (const auto& chain : rpc_providers) {
        std::string prefix = "HFX_" + chain + "_RPC";
        std::transform(prefix.begin(), prefix.end(), prefix.begin(), ::toupper);
        
        std::string endpoint = get_env_var(prefix + "_ENDPOINT");
        if (!endpoint.empty()) {
            RPCConfig config;
            config.chain = chain;
            config.endpoint = endpoint;
            config.api_key = get_env_var(prefix + "_API_KEY");
            config.websocket_endpoint = get_env_var(prefix + "_WS_ENDPOINT");
            config.timeout_ms = get_env_int(prefix + "_TIMEOUT", 5000);
            config.max_connections = get_env_int(prefix + "_MAX_CONNECTIONS", 10);
            
            rpc_configs_[chain] = config;
        }
    }
}

void ConfigManager::load_from_json(const nlohmann::json& json) {
    try {
        // Load API configurations
        if (json.contains("apis")) {
            for (const auto& [name, api_json] : json["apis"].items()) {
                APIConfig config;
                config.provider = api_json.value("provider", name);
                config.api_key = api_json.value("api_key", "");
                config.secret_key = api_json.value("secret_key", "");
                config.base_url = api_json.value("base_url", "");
                config.rate_limit_per_second = api_json.value("rate_limit_per_second", 100);
                config.enabled = api_json.value("enabled", true);
                
                if (api_json.contains("custom_headers")) {
                    config.custom_headers = api_json["custom_headers"];
                }
                
                api_configs_[name] = config;
            }
        }
        
        // Load RPC configurations
        if (json.contains("rpcs")) {
            for (const auto& [name, rpc_json] : json["rpcs"].items()) {
                RPCConfig config;
                config.chain = rpc_json.value("chain", name);
                config.provider = rpc_json.value("provider", "");
                config.endpoint = rpc_json.value("endpoint", "");
                config.api_key = rpc_json.value("api_key", "");
                config.websocket_endpoint = rpc_json.value("websocket_endpoint", "");
                config.timeout_ms = rpc_json.value("timeout_ms", 5000);
                config.max_connections = rpc_json.value("max_connections", 10);
                config.websocket_enabled = rpc_json.value("websocket_enabled", true);
                
                rpc_configs_[name] = config;
            }
        }
        
        // Load trading configuration
        if (json.contains("trading")) {
            const auto& trading_json = json["trading"];
            trading_config_.primary_chain = trading_json.value("primary_chain", trading_config_.primary_chain);
            trading_config_.max_position_size_usd = trading_json.value("max_position_size_usd", trading_config_.max_position_size_usd);
            trading_config_.max_slippage_percent = trading_json.value("max_slippage_percent", trading_config_.max_slippage_percent);
            trading_config_.min_profit_threshold_percent = trading_json.value("min_profit_threshold_percent", trading_config_.min_profit_threshold_percent);
            trading_config_.max_concurrent_trades = trading_json.value("max_concurrent_trades", trading_config_.max_concurrent_trades);
            trading_config_.enable_mev_protection = trading_json.value("enable_mev_protection", trading_config_.enable_mev_protection);
            trading_config_.enable_frontrun_protection = trading_json.value("enable_frontrun_protection", trading_config_.enable_frontrun_protection);
            trading_config_.enable_sandwich_protection = trading_json.value("enable_sandwich_protection", trading_config_.enable_sandwich_protection);
            
            if (trading_json.contains("supported_chains")) {
                trading_config_.supported_chains = trading_json["supported_chains"];
            }
        }
        
        // Load other configurations (database, security, monitoring, web)
        if (json.contains("database")) {
            const auto& db_json = json["database"];
            database_config_.type = db_json.value("type", database_config_.type);
            database_config_.host = db_json.value("host", database_config_.host);
            database_config_.port = db_json.value("port", database_config_.port);
            database_config_.database = db_json.value("database", database_config_.database);
            database_config_.username = db_json.value("username", database_config_.username);
            database_config_.password = db_json.value("password", database_config_.password);
        }
        
        if (json.contains("web_dashboard")) {
            const auto& web_json = json["web_dashboard"];
            web_dashboard_config_.enabled = web_json.value("enabled", web_dashboard_config_.enabled);
            web_dashboard_config_.port = web_json.value("port", web_dashboard_config_.port);
            web_dashboard_config_.host = web_json.value("host", web_dashboard_config_.host);
            web_dashboard_config_.enable_auth = web_json.value("enable_auth", web_dashboard_config_.enable_auth);
            web_dashboard_config_.default_username = web_json.value("default_username", web_dashboard_config_.default_username);
            web_dashboard_config_.default_password = web_json.value("default_password", web_dashboard_config_.default_password);
        }
        
    } catch (const std::exception& e) {
        validation_errors_.push_back("JSON parsing error: " + std::string(e.what()));
    }
}

void ConfigManager::apply_defaults() {
    // Set up default API templates
    if (api_configs_.empty()) {
        api_configs_["twitter"] = templates::create_twitter_api_template();
        api_configs_["reddit"] = templates::create_reddit_api_template();
        api_configs_["dexscreener"] = templates::create_dexscreener_api_template();
        api_configs_["gmgn"] = templates::create_gmgn_api_template();
    }
    
    // Set up default RPC templates
    if (rpc_configs_.empty()) {
        rpc_configs_["ethereum"] = templates::create_ethereum_rpc_template();
        rpc_configs_["solana"] = templates::create_solana_rpc_template();
        rpc_configs_["base"] = templates::create_base_rpc_template();
        rpc_configs_["arbitrum"] = templates::create_arbitrum_rpc_template();
    }
    
    // Generate default JWT secret if not set
    if (security_config_.jwt_secret.empty()) {
        security_config_.jwt_secret = "change_this_in_production_" + std::to_string(std::time(nullptr));
    }
}

bool ConfigManager::validate_config() const {
    validation_errors_.clear();
    
    // Validate API configurations
    for (const auto& [name, config] : api_configs_) {
        if (!validate_api_config(config)) {
            validation_errors_.push_back("Invalid API config for " + name);
        }
    }
    
    // Validate RPC configurations
    for (const auto& [name, config] : rpc_configs_) {
        if (!validate_rpc_config(config)) {
            validation_errors_.push_back("Invalid RPC config for " + name);
        }
    }
    
    // Validate trading configuration
    if (trading_config_.max_position_size_usd <= 0) {
        validation_errors_.push_back("max_position_size_usd must be positive");
    }
    if (trading_config_.max_slippage_percent < 0 || trading_config_.max_slippage_percent > 100) {
        validation_errors_.push_back("max_slippage_percent must be between 0 and 100");
    }
    if (trading_config_.max_concurrent_trades <= 0) {
        validation_errors_.push_back("max_concurrent_trades must be positive");
    }
    
    // Validate web dashboard configuration
    if (web_dashboard_config_.port <= 0 || web_dashboard_config_.port > 65535) {
        validation_errors_.push_back("web dashboard port must be between 1 and 65535");
    }
    
    // Validate database configuration
    if (database_config_.port <= 0 || database_config_.port > 65535) {
        validation_errors_.push_back("database port must be between 1 and 65535");
    }
    
    return validation_errors_.empty();
}

std::vector<std::string> ConfigManager::get_validation_errors() const {
    return validation_errors_;
}

bool ConfigManager::validate_api_config(const APIConfig& config) const {
    if (config.provider.empty()) return false;
    if (config.api_key.empty() && config.provider != "reddit") return false;  // Reddit can work without API key
    if (config.rate_limit_per_second <= 0) return false;
    return true;
}

bool ConfigManager::validate_rpc_config(const RPCConfig& config) const {
    if (config.chain.empty()) return false;
    if (config.endpoint.empty()) return false;
    if (config.timeout_ms <= 0) return false;
    if (config.max_connections <= 0) return false;
    return true;
}

nlohmann::json ConfigManager::export_config() const {
    nlohmann::json config_json;
    
    // Export API configurations
    for (const auto& [name, config] : api_configs_) {
        config_json["apis"][name] = {
            {"provider", config.provider},
            {"api_key", config.api_key.empty() ? "YOUR_API_KEY_HERE" : config.api_key},
            {"secret_key", config.secret_key.empty() ? "" : "YOUR_SECRET_KEY_HERE"},
            {"base_url", config.base_url},
            {"rate_limit_per_second", config.rate_limit_per_second},
            {"enabled", config.enabled},
            {"custom_headers", config.custom_headers}
        };
    }
    
    // Export RPC configurations
    for (const auto& [name, config] : rpc_configs_) {
        config_json["rpcs"][name] = {
            {"chain", config.chain},
            {"provider", config.provider},
            {"endpoint", config.endpoint.empty() ? "YOUR_RPC_ENDPOINT_HERE" : config.endpoint},
            {"api_key", config.api_key.empty() ? "YOUR_API_KEY_HERE" : config.api_key},
            {"websocket_endpoint", config.websocket_endpoint},
            {"timeout_ms", config.timeout_ms},
            {"max_connections", config.max_connections},
            {"websocket_enabled", config.websocket_enabled}
        };
    }
    
    // Export trading configuration
    config_json["trading"] = {
        {"primary_chain", trading_config_.primary_chain},
        {"supported_chains", trading_config_.supported_chains},
        {"max_position_size_usd", trading_config_.max_position_size_usd},
        {"max_slippage_percent", trading_config_.max_slippage_percent},
        {"min_profit_threshold_percent", trading_config_.min_profit_threshold_percent},
        {"max_concurrent_trades", trading_config_.max_concurrent_trades},
        {"enable_mev_protection", trading_config_.enable_mev_protection},
        {"enable_frontrun_protection", trading_config_.enable_frontrun_protection},
        {"enable_sandwich_protection", trading_config_.enable_sandwich_protection},
        {"default_token_out", trading_config_.default_token_out},
        {"gas_price_multiplier", trading_config_.gas_price_multiplier},
        {"max_gas_price_gwei", trading_config_.max_gas_price_gwei}
    };
    
    // Export database configuration
    config_json["database"] = {
        {"type", database_config_.type},
        {"host", database_config_.host},
        {"port", database_config_.port},
        {"database", database_config_.database},
        {"username", database_config_.username},
        {"password", database_config_.password.empty() ? "YOUR_DB_PASSWORD_HERE" : "***"},
        {"max_connections", database_config_.max_connections},
        {"enable_ssl", database_config_.enable_ssl}
    };
    
    // Export web dashboard configuration
    config_json["web_dashboard"] = {
        {"enabled", web_dashboard_config_.enabled},
        {"port", web_dashboard_config_.port},
        {"host", web_dashboard_config_.host},
        {"enable_auth", web_dashboard_config_.enable_auth},
        {"default_username", web_dashboard_config_.default_username},
        {"default_password", web_dashboard_config_.default_password == "changeme" ? "changeme" : "***"}
    };
    
    // Export security configuration
    config_json["security"] = {
        {"enable_hsm", security_config_.enable_hsm},
        {"hsm_provider", security_config_.hsm_provider},
        {"enable_audit_logging", security_config_.enable_audit_logging},
        {"enable_rate_limiting", security_config_.enable_rate_limiting},
        {"jwt_expiry_hours", security_config_.jwt_expiry_hours}
    };
    
    // Export monitoring configuration
    config_json["monitoring"] = {
        {"enable_prometheus", monitoring_config_.enable_prometheus},
        {"prometheus_port", monitoring_config_.prometheus_port},
        {"log_level", monitoring_config_.log_level},
        {"log_format", monitoring_config_.log_format},
        {"log_output", monitoring_config_.log_output}
    };
    
    return config_json;
}

// Template implementations
namespace templates {

APIConfig create_twitter_api_template() {
    APIConfig config;
    config.provider = "twitter";
    config.base_url = "https://api.twitter.com/2";
    config.rate_limit_per_second = 50;
    config.enabled = false;
    config.custom_headers["User-Agent"] = "HydraFlow-X/1.0";
    return config;
}

APIConfig create_reddit_api_template() {
    APIConfig config;
    config.provider = "reddit";
    config.base_url = "https://oauth.reddit.com";
    config.rate_limit_per_second = 30;
    config.enabled = false;
    config.custom_headers["User-Agent"] = "HydraFlow-X:1.0 (by /u/hydraflow)";
    return config;
}

APIConfig create_dexscreener_api_template() {
    APIConfig config;
    config.provider = "dexscreener";
    config.base_url = "https://api.dexscreener.com/latest";
    config.rate_limit_per_second = 100;
    config.enabled = false;
    return config;
}

APIConfig create_gmgn_api_template() {
    APIConfig config;
    config.provider = "gmgn";
    config.base_url = "https://gmgn.ai/api";
    config.rate_limit_per_second = 50;
    config.enabled = false;
    return config;
}

RPCConfig create_ethereum_rpc_template(const std::string& provider) {
    RPCConfig config;
    config.chain = "ethereum";
    config.provider = provider;
    if (provider == "alchemy") {
        config.endpoint = "https://eth-mainnet.g.alchemy.com/v2/YOUR_API_KEY";
        config.websocket_endpoint = "wss://eth-mainnet.g.alchemy.com/v2/YOUR_API_KEY";
    } else if (provider == "infura") {
        config.endpoint = "https://mainnet.infura.io/v3/YOUR_API_KEY";
        config.websocket_endpoint = "wss://mainnet.infura.io/ws/v3/YOUR_API_KEY";
    }
    config.timeout_ms = 5000;
    config.max_connections = 10;
    config.websocket_enabled = true;
    return config;
}

RPCConfig create_solana_rpc_template(const std::string& provider) {
    RPCConfig config;
    config.chain = "solana";
    config.provider = provider;
    if (provider == "helius") {
        config.endpoint = "https://rpc.helius.xyz/?api-key=YOUR_API_KEY";
        config.websocket_endpoint = "wss://rpc.helius.xyz/?api-key=YOUR_API_KEY";
    } else if (provider == "quicknode") {
        config.endpoint = "https://YOUR_ENDPOINT.solana-mainnet.quiknode.pro/YOUR_API_KEY/";
        config.websocket_endpoint = "wss://YOUR_ENDPOINT.solana-mainnet.quiknode.pro/YOUR_API_KEY/";
    }
    config.timeout_ms = 3000;
    config.max_connections = 20;
    config.websocket_enabled = true;
    return config;
}

RPCConfig create_base_rpc_template() {
    RPCConfig config;
    config.chain = "base";
    config.provider = "base";
    config.endpoint = "https://mainnet.base.org";
    config.websocket_endpoint = "wss://mainnet.base.org";
    config.timeout_ms = 5000;
    config.max_connections = 10;
    config.websocket_enabled = true;
    return config;
}

RPCConfig create_arbitrum_rpc_template() {
    RPCConfig config;
    config.chain = "arbitrum";
    config.provider = "arbitrum";
    config.endpoint = "https://arb1.arbitrum.io/rpc";
    config.websocket_endpoint = "wss://arb1.arbitrum.io/ws";
    config.timeout_ms = 5000;
    config.max_connections = 10;
    config.websocket_enabled = true;
    return config;
}

nlohmann::json create_full_config_template() {
    ConfigManager temp_config;
    temp_config.apply_defaults();
    return temp_config.export_config();
}

} // namespace templates

} // namespace hfx::config
