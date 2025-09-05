/**
 * @file production_database_manager.cpp
 * @brief Production database manager implementation
 */

#include "../include/production_database_manager.hpp"
#include "../../hfx-log/include/simple_logger.hpp"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <regex>
#include <fstream>

namespace hfx::db {

// Data model implementations
namespace models {
    
    std::string Trade::to_insert_sql() const {
        std::stringstream ss;
        ss << "INSERT INTO trades (id, order_id, platform, token_in, token_out, side, "
           << "amount_in, amount_out, price, slippage_percent, gas_used, gas_price, "
           << "transaction_hash, status, created_at, executed_at, wallet_address, chain_id) VALUES ("
           << "'" << db_utils::escape_sql_string(id) << "', "
           << "'" << db_utils::escape_sql_string(order_id) << "', "
           << "'" << db_utils::escape_sql_string(platform) << "', "
           << "'" << db_utils::escape_sql_string(token_in) << "', "
           << "'" << db_utils::escape_sql_string(token_out) << "', "
           << "'" << db_utils::escape_sql_string(side) << "', "
           << amount_in << ", " << amount_out << ", " << price << ", "
           << slippage_percent << ", " << gas_used << ", " << gas_price << ", "
           << "'" << db_utils::escape_sql_string(transaction_hash) << "', "
           << "'" << db_utils::escape_sql_string(status) << "', "
           << "'" << db_utils::format_timestamp(created_at) << "', "
           << "'" << db_utils::format_timestamp(executed_at) << "', "
           << "'" << db_utils::escape_sql_string(wallet_address) << "', "
           << "'" << db_utils::escape_sql_string(chain_id) << "')";
        return ss.str();
    }
    
    std::string Position::to_insert_sql() const {
        std::stringstream ss;
        ss << "INSERT INTO positions (id, symbol, wallet_address, quantity, average_price, "
           << "current_price, unrealized_pnl, realized_pnl, status, opened_at, updated_at) VALUES ("
           << "'" << db_utils::escape_sql_string(id) << "', "
           << "'" << db_utils::escape_sql_string(symbol) << "', "
           << "'" << db_utils::escape_sql_string(wallet_address) << "', "
           << quantity << ", " << average_price << ", " << current_price << ", "
           << unrealized_pnl << ", " << realized_pnl << ", "
           << "'" << db_utils::escape_sql_string(status) << "', "
           << "'" << db_utils::format_timestamp(opened_at) << "', "
           << "'" << db_utils::format_timestamp(updated_at) << "')";
        return ss.str();
    }
    
    std::string RiskMetric::to_insert_sql() const {
        std::stringstream ss;
        ss << "INSERT INTO risk_metrics (id, wallet_address, portfolio_value, daily_pnl, "
           << "var_95, max_drawdown, sharpe_ratio, leverage_ratio, timestamp) VALUES ("
           << "'" << db_utils::escape_sql_string(id) << "', "
           << "'" << db_utils::escape_sql_string(wallet_address) << "', "
           << portfolio_value << ", " << daily_pnl << ", " << var_95 << ", "
           << max_drawdown << ", " << sharpe_ratio << ", " << leverage_ratio << ", "
           << "'" << db_utils::format_timestamp(timestamp) << "')";
        return ss.str();
    }
    
    std::string Alert::to_insert_sql() const {
        std::stringstream ss;
        ss << "INSERT INTO alerts (id, level, type, message, symbol, acknowledged, created_at) VALUES ("
           << "'" << db_utils::escape_sql_string(id) << "', "
           << "'" << db_utils::escape_sql_string(level) << "', "
           << "'" << db_utils::escape_sql_string(type) << "', "
           << "'" << db_utils::escape_sql_string(message) << "', "
           << "'" << db_utils::escape_sql_string(symbol) << "', "
           << (acknowledged ? "true" : "false") << ", "
           << "'" << db_utils::format_timestamp(created_at) << "')";
        return ss.str();
    }
}

// ConnectionPool implementation
class ConnectionPool::ConnectionPoolImpl {
public:
    DatabaseConfig config_;
    std::atomic<bool> initialized_{false};
    std::atomic<uint32_t> active_connections_{0};
    std::queue<void*> available_connections_; // Placeholder for actual connection objects
    std::mutex pool_mutex_;
    PoolStats stats_;
    
    ConnectionPoolImpl(const DatabaseConfig& config) : config_(config) {
        stats_.last_activity = std::chrono::system_clock::now();
    }
    
    bool initialize() {
        std::lock_guard<std::mutex> lock(pool_mutex_);
        
        if (initialized_) return true;
        
        try {
            // Create minimum number of connections
            for (uint32_t i = 0; i < config_.min_connections; ++i) {
                // In real implementation, would create actual database connections
                // For now, using placeholder
                available_connections_.push(nullptr);
                stats_.total_connections++;
            }
            
            initialized_ = true;
            HFX_LOG_INFO("[ConnectionPool] Initialized with " + std::to_string(config_.min_connections) + " connections");
            
            return true;
        } catch (const std::exception& e) {
            HFX_LOG_ERROR("[ConnectionPool] Initialization failed: " + std::string(e.what()));
            return false;
        }
    }
    
    void shutdown() {
        std::lock_guard<std::mutex> lock(pool_mutex_);
        
        if (!initialized_) return;
        
        // Close all connections
        while (!available_connections_.empty()) {
            // In real implementation, would close actual database connections
            available_connections_.pop();
            stats_.total_connections--;
        }
        
        initialized_ = false;
        HFX_LOG_INFO("[ConnectionPool] Shutdown completed");
    }
    
    std::future<QueryResult> execute_query(const std::string& query) {
        auto promise = std::make_shared<std::promise<QueryResult>>();
        auto future = promise->get_future();
        
        // Simulate async query execution
        std::thread([this, query, promise]() {
            try {
                auto start_time = std::chrono::high_resolution_clock::now();
                
                // Simulate query execution
                QueryResult result(QueryStatus::SUCCESS);
                
                // Parse simple SELECT queries for demonstration
                if (query.find("SELECT") != std::string::npos) {
                    // Simulate returning some data
                    std::unordered_map<std::string, std::string> row;
                    row["id"] = "1";
                    row["status"] = "success";
                    result.rows.push_back(row);
                }
                
                auto end_time = std::chrono::high_resolution_clock::now();
                result.execution_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
                
                // Update statistics
                stats_.total_queries++;
                stats_.successful_queries++;
                stats_.last_activity = std::chrono::system_clock::now();
                
                promise->set_value(result);
                
            } catch (const std::exception& e) {
                QueryResult error_result(QueryStatus::ERROR);
                error_result.error_message = e.what();
                stats_.failed_queries++;
                promise->set_value(error_result);
            }
        }).detach();
        
        return future;
    }
    
    ConnectionPool::PoolStats get_stats() const {
        std::lock_guard<std::mutex> lock(pool_mutex_);
        
        PoolStats current_stats = stats_;
        current_stats.active_connections = active_connections_.load();
        current_stats.idle_connections = stats_.total_connections - current_stats.active_connections;
        
        if (stats_.total_queries > 0) {
            // Simulate average query time calculation
            current_stats.avg_query_time = std::chrono::milliseconds(50); // 50ms average
        }
        
        return current_stats;
    }
};

ConnectionPool::ConnectionPool(const DatabaseConfig& config) 
    : pimpl_(std::make_unique<ConnectionPoolImpl>(config)) {
}

ConnectionPool::~ConnectionPool() = default;

bool ConnectionPool::initialize() {
    return pimpl_->initialize();
}

void ConnectionPool::shutdown() {
    pimpl_->shutdown();
}

bool ConnectionPool::is_healthy() const {
    return pimpl_->initialized_.load() && pimpl_->stats_.total_connections > 0;
}

std::future<QueryResult> ConnectionPool::execute_query(const std::string& query) {
    return pimpl_->execute_query(query);
}

std::future<QueryResult> ConnectionPool::execute_prepared(const std::string& statement_name, 
                                                        const std::vector<std::string>& params) {
    // Simulate prepared statement execution
    std::string full_query = statement_name + " with " + std::to_string(params.size()) + " parameters";
    return execute_query(full_query);
}

std::string ConnectionPool::begin_transaction() {
    return "tx_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
}

bool ConnectionPool::commit_transaction(const std::string& transaction_id) {
    HFX_LOG_DEBUG("[ConnectionPool] Committing transaction: " + transaction_id);
    return true;
}

bool ConnectionPool::rollback_transaction(const std::string& transaction_id) {
    HFX_LOG_DEBUG("[ConnectionPool] Rolling back transaction: " + transaction_id);
    return true;
}

std::future<std::vector<QueryResult>> ConnectionPool::execute_batch(const std::vector<std::string>& queries) {
    auto promise = std::make_shared<std::promise<std::vector<QueryResult>>>();
    auto future = promise->get_future();
    
    std::thread([this, queries, promise]() {
        std::vector<QueryResult> results;
        
        for (const auto& query : queries) {
            auto result_future = execute_query(query);
            results.push_back(result_future.get());
        }
        
        promise->set_value(results);
    }).detach();
    
    return future;
}

ConnectionPool::PoolStats ConnectionPool::get_stats() const {
    return pimpl_->get_stats();
}

// MigrationManager implementation
MigrationManager::MigrationManager(std::shared_ptr<ConnectionPool> pool) : pool_(pool) {
    // Initialize with predefined migrations
    migrations_ = DatabaseFactory::create_trading_migrations();
    
    auto analytics_migrations = DatabaseFactory::create_analytics_migrations();
    migrations_.insert(migrations_.end(), analytics_migrations.begin(), analytics_migrations.end());
    
    auto monitoring_migrations = DatabaseFactory::create_monitoring_migrations();
    migrations_.insert(migrations_.end(), monitoring_migrations.begin(), monitoring_migrations.end());
}

MigrationManager::~MigrationManager() = default;

bool MigrationManager::add_migration(const Migration& migration) {
    std::lock_guard<std::mutex> lock(migrations_mutex_);
    migrations_.push_back(migration);
    return true;
}

bool MigrationManager::apply_migrations() {
    if (!create_migration_table()) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(migrations_mutex_);
    
    bool all_successful = true;
    
    for (auto& migration : migrations_) {
        if (!migration.applied) {
            try {
                // Check if migration was already applied
                std::string check_query = "SELECT id FROM schema_migrations WHERE id = '" + migration.id + "'";
                auto check_result = pool_->execute_query(check_query).get();
                
                if (check_result.empty()) {
                    // Apply migration
                    auto result = pool_->execute_query(migration.sql_up).get();
                    
                    if (result.success()) {
                        // Record migration as applied
                        std::string record_query = "INSERT INTO schema_migrations (id, name, applied_at) VALUES ('" +
                                                 migration.id + "', '" + migration.name + "', NOW())";
                        
                        auto record_result = pool_->execute_query(record_query).get();
                        
                        if (record_result.success()) {
                            migration.applied = true;
                            HFX_LOG_INFO("[MigrationManager] Applied migration: " + migration.name);
                        } else {
                            HFX_LOG_ERROR("[MigrationManager] Failed to record migration: " + migration.name);
                            all_successful = false;
                        }
                    } else {
                        HFX_LOG_ERROR("[MigrationManager] Failed to apply migration: " + migration.name + 
                                     " - " + result.error_message);
                        all_successful = false;
                    }
                } else {
                    migration.applied = true;
                }
                
            } catch (const std::exception& e) {
                HFX_LOG_ERROR("[MigrationManager] Migration error: " + std::string(e.what()));
                all_successful = false;
            }
        }
    }
    
    return all_successful;
}

bool MigrationManager::create_migration_table() {
    std::string create_table_sql = R"(
        CREATE TABLE IF NOT EXISTS schema_migrations (
            id VARCHAR(255) PRIMARY KEY,
            name VARCHAR(255) NOT NULL,
            applied_at TIMESTAMP WITH TIME ZONE DEFAULT NOW()
        )
    )";
    
    auto result = pool_->execute_query(create_table_sql).get();
    return result.success();
}

std::vector<Migration> MigrationManager::get_pending_migrations() {
    std::lock_guard<std::mutex> lock(migrations_mutex_);
    
    std::vector<Migration> pending;
    for (const auto& migration : migrations_) {
        if (!migration.applied) {
            pending.push_back(migration);
        }
    }
    
    return pending;
}

std::vector<Migration> MigrationManager::get_applied_migrations() {
    std::lock_guard<std::mutex> lock(migrations_mutex_);
    
    std::vector<Migration> applied;
    for (const auto& migration : migrations_) {
        if (migration.applied) {
            applied.push_back(migration);
        }
    }
    
    return applied;
}

// ProductionDatabaseManager implementation
class ProductionDatabaseManager::DatabaseManagerImpl {
public:
    std::unordered_map<DatabaseType, DatabaseConfig> configs_;
    std::unordered_map<DatabaseType, std::shared_ptr<ConnectionPool>> pools_;
    std::unique_ptr<MigrationManager> migration_manager_;
    
    std::atomic<bool> initialized_{false};
    std::atomic<bool> health_monitoring_{false};
    std::thread health_monitor_thread_;
    
    // Callbacks
    std::function<void(const models::Trade&)> trade_callback_;
    std::function<void(const models::Position&)> position_callback_;
    std::function<void(const models::Alert&)> alert_callback_;
    ErrorCallback error_callback_;
    
    DatabaseManagerImpl(const std::unordered_map<DatabaseType, DatabaseConfig>& configs)
        : configs_(configs) {
    }
    
    ~DatabaseManagerImpl() {
        shutdown();
    }
    
    bool initialize() {
        if (initialized_) return true;
        
        try {
            // Initialize connection pools
            for (const auto& config_pair : configs_) {
                auto pool = std::make_shared<ConnectionPool>(config_pair.second);
                
                if (pool->initialize()) {
                    pools_[config_pair.first] = pool;
                    HFX_LOG_INFO("[DatabaseManager] Initialized " + database_type_to_string(config_pair.first) + " pool");
                } else {
                    HFX_LOG_ERROR("[DatabaseManager] Failed to initialize " + database_type_to_string(config_pair.first) + " pool");
                    return false;
                }
            }
            
            // Initialize migration manager with primary database
            if (pools_.count(DatabaseType::POSTGRESQL)) {
                migration_manager_ = std::make_unique<MigrationManager>(pools_[DatabaseType::POSTGRESQL]);
            }
            
            initialized_ = true;
            HFX_LOG_INFO("[DatabaseManager] Initialization completed");
            
            return true;
            
        } catch (const std::exception& e) {
            HFX_LOG_ERROR("[DatabaseManager] Initialization failed: " + std::string(e.what()));
            return false;
        }
    }
    
    void shutdown() {
        if (!initialized_) return;
        
        stop_health_monitoring();
        
        // Shutdown all pools
        for (auto& pool_pair : pools_) {
            pool_pair.second->shutdown();
        }
        pools_.clear();
        
        initialized_ = false;
        HFX_LOG_INFO("[DatabaseManager] Shutdown completed");
    }
    
    bool is_healthy() const {
        if (!initialized_) return false;
        
        // Check if at least one pool is healthy
        for (const auto& pool_pair : pools_) {
            if (pool_pair.second->is_healthy()) {
                return true;
            }
        }
        
        return false;
    }
    
    void start_health_monitoring() {
        if (health_monitoring_) return;
        
        health_monitoring_ = true;
        health_monitor_thread_ = std::thread(&DatabaseManagerImpl::health_monitor_worker, this);
        HFX_LOG_INFO("[DatabaseManager] Health monitoring started");
    }
    
    void stop_health_monitoring() {
        if (!health_monitoring_) return;
        
        health_monitoring_ = false;
        
        if (health_monitor_thread_.joinable()) {
            health_monitor_thread_.join();
        }
        
        HFX_LOG_INFO("[DatabaseManager] Health monitoring stopped");
    }

private:
    std::string database_type_to_string(DatabaseType type) {
        switch (type) {
            case DatabaseType::POSTGRESQL: return "PostgreSQL";
            case DatabaseType::CLICKHOUSE: return "ClickHouse";
            case DatabaseType::REDIS: return "Redis";
            case DatabaseType::SQLITE_MEMORY: return "SQLite";
            case DatabaseType::TIMESCALEDB: return "TimescaleDB";
            default: return "Unknown";
        }
    }
    
    void health_monitor_worker() {
        while (health_monitoring_) {
            try {
                // Check health of all pools
                for (const auto& pool_pair : pools_) {
                    if (!pool_pair.second->is_healthy()) {
                        if (error_callback_) {
                            error_callback_("Connection pool unhealthy", pool_pair.first);
                        }
                        
                        HFX_LOG_WARN("[DatabaseManager] " + database_type_to_string(pool_pair.first) + " pool is unhealthy");
                    }
                }
                
                // Sleep for 30 seconds before next check
                std::this_thread::sleep_for(std::chrono::seconds(30));
                
            } catch (const std::exception& e) {
                HFX_LOG_ERROR("[DatabaseManager] Health monitor error: " + std::string(e.what()));
                std::this_thread::sleep_for(std::chrono::seconds(60));
            }
        }
    }
};

ProductionDatabaseManager::ProductionDatabaseManager(
    const std::unordered_map<DatabaseType, DatabaseConfig>& configs)
    : pimpl_(std::make_unique<DatabaseManagerImpl>(configs)) {
}

ProductionDatabaseManager::~ProductionDatabaseManager() = default;

bool ProductionDatabaseManager::initialize() {
    return pimpl_->initialize();
}

void ProductionDatabaseManager::shutdown() {
    pimpl_->shutdown();
}

bool ProductionDatabaseManager::is_healthy() const {
    return pimpl_->is_healthy();
}

std::shared_ptr<ConnectionPool> ProductionDatabaseManager::get_pool(DatabaseType type) {
    auto it = pimpl_->pools_.find(type);
    return (it != pimpl_->pools_.end()) ? it->second : nullptr;
}

std::future<QueryResult> ProductionDatabaseManager::execute_query(DatabaseType type, const std::string& query) {
    auto pool = get_pool(type);
    if (pool) {
        return pool->execute_query(query);
    }
    
    // Return failed result
    auto promise = std::make_shared<std::promise<QueryResult>>();
    promise->set_value(QueryResult(QueryStatus::CONNECTION_LOST));
    return promise->get_future();
}

bool ProductionDatabaseManager::apply_all_migrations() {
    if (!pimpl_->migration_manager_) {
        HFX_LOG_ERROR("[DatabaseManager] Migration manager not initialized");
        return false;
    }
    
    return pimpl_->migration_manager_->apply_migrations();
}

void ProductionDatabaseManager::start_health_monitoring() {
    pimpl_->start_health_monitoring();
}

void ProductionDatabaseManager::stop_health_monitoring() {
    pimpl_->stop_health_monitoring();
}

void ProductionDatabaseManager::register_error_callback(ErrorCallback callback) {
    pimpl_->error_callback_ = callback;
}

// DatabaseFactory implementation
std::unordered_map<DatabaseType, DatabaseConfig> DatabaseFactory::create_production_config() {
    std::unordered_map<DatabaseType, DatabaseConfig> configs;
    
    // PostgreSQL config
    DatabaseConfig pg_config;
    pg_config.host = "localhost";
    pg_config.port = 5432;
    pg_config.database = "hydraflow_prod";
    pg_config.username = "hydraflow";
    pg_config.min_connections = 10;
    pg_config.max_connections = 100;
    pg_config.enable_ssl = true;
    configs[DatabaseType::POSTGRESQL] = pg_config;
    
    // ClickHouse config for analytics
    DatabaseConfig ch_config;
    ch_config.host = "localhost";
    ch_config.port = 9000;
    ch_config.database = "hydraflow_analytics";
    ch_config.username = "hydraflow";
    ch_config.min_connections = 5;
    ch_config.max_connections = 25;
    configs[DatabaseType::CLICKHOUSE] = ch_config;
    
    return configs;
}

std::unordered_map<DatabaseType, DatabaseConfig> DatabaseFactory::create_development_config() {
    auto configs = create_production_config();
    
    // Reduce connection pool sizes for development
    for (auto& config : configs) {
        config.second.min_connections = 2;
        config.second.max_connections = 10;
        config.second.database += "_dev";
    }
    
    return configs;
}

std::vector<Migration> DatabaseFactory::create_trading_migrations() {
    std::vector<Migration> migrations;
    
    // Create trades table migration
    migrations.emplace_back(
        "001_create_trades_table",
        "Create trades table",
        R"(
            CREATE TABLE IF NOT EXISTS trades (
                id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
                order_id VARCHAR(255) NOT NULL,
                platform VARCHAR(50) NOT NULL,
                token_in VARCHAR(255) NOT NULL,
                token_out VARCHAR(255) NOT NULL,
                side VARCHAR(10) NOT NULL,
                amount_in BIGINT NOT NULL,
                amount_out BIGINT NOT NULL,
                price DECIMAL(36, 18),
                slippage_percent DECIMAL(10, 4),
                gas_used BIGINT,
                gas_price BIGINT,
                transaction_hash VARCHAR(255) UNIQUE,
                status VARCHAR(20) NOT NULL DEFAULT 'PENDING',
                created_at TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
                executed_at TIMESTAMP WITH TIME ZONE,
                wallet_address VARCHAR(255) NOT NULL,
                chain_id VARCHAR(50) NOT NULL
            );
            
            CREATE INDEX IF NOT EXISTS idx_trades_wallet ON trades(wallet_address);
            CREATE INDEX IF NOT EXISTS idx_trades_created_at ON trades(created_at);
            CREATE INDEX IF NOT EXISTS idx_trades_status ON trades(status);
        )",
        "DROP TABLE IF EXISTS trades;"
    );
    
    // Create positions table migration
    migrations.emplace_back(
        "002_create_positions_table",
        "Create positions table", 
        R"(
            CREATE TABLE IF NOT EXISTS positions (
                id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
                symbol VARCHAR(50) NOT NULL,
                wallet_address VARCHAR(255) NOT NULL,
                quantity DECIMAL(36, 18) NOT NULL,
                average_price DECIMAL(36, 18) NOT NULL,
                current_price DECIMAL(36, 18) NOT NULL,
                unrealized_pnl DECIMAL(36, 18) DEFAULT 0,
                realized_pnl DECIMAL(36, 18) DEFAULT 0,
                status VARCHAR(20) DEFAULT 'OPEN',
                opened_at TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
                updated_at TIMESTAMP WITH TIME ZONE DEFAULT NOW()
            );
            
            CREATE UNIQUE INDEX IF NOT EXISTS idx_positions_wallet_symbol ON positions(wallet_address, symbol);
            CREATE INDEX IF NOT EXISTS idx_positions_status ON positions(status);
        )",
        "DROP TABLE IF EXISTS positions;"
    );
    
    // Create risk metrics table migration
    migrations.emplace_back(
        "003_create_risk_metrics_table",
        "Create risk metrics table",
        R"(
            CREATE TABLE IF NOT EXISTS risk_metrics (
                id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
                wallet_address VARCHAR(255) NOT NULL,
                portfolio_value DECIMAL(36, 18) DEFAULT 0,
                daily_pnl DECIMAL(36, 18) DEFAULT 0,
                var_95 DECIMAL(36, 18) DEFAULT 0,
                max_drawdown DECIMAL(10, 4) DEFAULT 0,
                sharpe_ratio DECIMAL(10, 4) DEFAULT 0,
                leverage_ratio DECIMAL(10, 4) DEFAULT 0,
                timestamp TIMESTAMP WITH TIME ZONE DEFAULT NOW()
            );
            
            CREATE INDEX IF NOT EXISTS idx_risk_metrics_wallet ON risk_metrics(wallet_address);
            CREATE INDEX IF NOT EXISTS idx_risk_metrics_timestamp ON risk_metrics(timestamp);
        )",
        "DROP TABLE IF EXISTS risk_metrics;"
    );
    
    return migrations;
}

std::vector<Migration> DatabaseFactory::create_analytics_migrations() {
    std::vector<Migration> migrations;
    
    // Create alerts table migration
    migrations.emplace_back(
        "101_create_alerts_table",
        "Create alerts table",
        R"(
            CREATE TABLE IF NOT EXISTS alerts (
                id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
                level VARCHAR(20) NOT NULL,
                type VARCHAR(50) NOT NULL,
                message TEXT NOT NULL,
                symbol VARCHAR(50),
                acknowledged BOOLEAN DEFAULT FALSE,
                created_at TIMESTAMP WITH TIME ZONE DEFAULT NOW()
            );
            
            CREATE INDEX IF NOT EXISTS idx_alerts_level ON alerts(level);
            CREATE INDEX IF NOT EXISTS idx_alerts_created_at ON alerts(created_at);
            CREATE INDEX IF NOT EXISTS idx_alerts_acknowledged ON alerts(acknowledged);
        )",
        "DROP TABLE IF EXISTS alerts;"
    );
    
    return migrations;
}

std::vector<Migration> DatabaseFactory::create_monitoring_migrations() {
    std::vector<Migration> migrations;
    
    // Add any monitoring-specific tables here
    
    return migrations;
}

// Utility functions
namespace db_utils {
    
    std::string escape_sql_string(const std::string& input) {
        std::string escaped;
        escaped.reserve(input.length() * 2);
        
        for (char c : input) {
            if (c == '\'') {
                escaped += "''";
            } else if (c == '\\') {
                escaped += "\\\\";
            } else {
                escaped += c;
            }
        }
        
        return escaped;
    }
    
    std::string format_timestamp(const std::chrono::system_clock::time_point& time) {
        auto time_t = std::chrono::system_clock::to_time_t(time);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            time.time_since_epoch()) % 1000;
        
        std::stringstream ss;
        ss << std::put_time(std::gmtime(&time_t), "%Y-%m-%d %H:%M:%S");
        ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
        ss << " UTC";
        
        return ss.str();
    }
    
    std::string build_postgresql_connection_string(const DatabaseConfig& config) {
        std::stringstream ss;
        ss << "host=" << config.host
           << " port=" << config.port
           << " dbname=" << config.database
           << " user=" << config.username;
        
        if (!config.password.empty()) {
            ss << " password=" << config.password;
        }
        
        if (config.enable_ssl) {
            ss << " sslmode=" << config.ssl_mode;
        }
        
        return ss.str();
    }
    
    std::string build_select_query(const std::string& table, 
                                  const std::vector<std::string>& columns,
                                  const std::string& where_clause,
                                  const std::string& order_by,
                                  size_t limit) {
        std::stringstream ss;
        ss << "SELECT ";
        
        if (columns.empty()) {
            ss << "*";
        } else {
            for (size_t i = 0; i < columns.size(); ++i) {
                if (i > 0) ss << ", ";
                ss << columns[i];
            }
        }
        
        ss << " FROM " << table;
        
        if (!where_clause.empty()) {
            ss << " WHERE " << where_clause;
        }
        
        if (!order_by.empty()) {
            ss << " ORDER BY " << order_by;
        }
        
        if (limit > 0) {
            ss << " LIMIT " << limit;
        }
        
        return ss.str();
    }
}

} // namespace hfx::db
