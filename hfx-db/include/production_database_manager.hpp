/**
 * @file production_database_manager.hpp
 * @brief Production-grade database management system
 * @version 1.0.0
 */

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <atomic>
#include <functional>
#include <unordered_map>
#include <chrono>
#include <mutex>
#include <thread>
#include <future>
#include <queue>

namespace hfx::db {

// Database backend types
enum class DatabaseType {
    POSTGRESQL,         // Primary OLTP database
    CLICKHOUSE,         // Analytics and time-series
    REDIS,              // Caching and sessions
    SQLITE_MEMORY,      // Testing and development
    TIMESCALEDB         // Time-series extension for PostgreSQL
};

// Connection status
enum class ConnectionStatus {
    DISCONNECTED,
    CONNECTING,
    CONNECTED,
    ERROR,
    MAINTENANCE
};

// Query result status
enum class QueryStatus {
    SUCCESS,
    ERROR,
    TIMEOUT,
    CONNECTION_LOST,
    CONSTRAINT_VIOLATION,
    SYNTAX_ERROR
};

// Database configuration
struct DatabaseConfig {
    std::string host = "localhost";
    uint16_t port = 5432;
    std::string database = "hydraflow";
    std::string username = "hydraflow";
    std::string password;
    std::string connection_string;
    
    // Connection pool settings
    uint32_t min_connections = 5;
    uint32_t max_connections = 50;
    std::chrono::seconds connection_timeout{30};
    std::chrono::seconds idle_timeout{300}; // 5 minutes
    std::chrono::seconds query_timeout{30};
    
    // Advanced settings
    bool enable_ssl = true;
    bool enable_compression = true;
    bool enable_prepared_statements = true;
    std::string ssl_mode = "require";
    uint32_t statement_cache_size = 100;
    
    DatabaseConfig() = default;
};

// Schema migration
struct Migration {
    std::string id;
    std::string name;
    std::string sql_up;
    std::string sql_down;
    std::chrono::system_clock::time_point created_at;
    bool applied = false;
    
    Migration() = default;
    Migration(const std::string& id, const std::string& name, 
             const std::string& sql_up, const std::string& sql_down)
        : id(id), name(name), sql_up(sql_up), sql_down(sql_down),
          created_at(std::chrono::system_clock::now()) {}
};

// Query result
struct QueryResult {
    QueryStatus status;
    std::vector<std::unordered_map<std::string, std::string>> rows;
    uint64_t affected_rows = 0;
    std::string error_message;
    std::chrono::milliseconds execution_time{0};
    
    QueryResult(QueryStatus s = QueryStatus::SUCCESS) : status(s) {}
    
    bool success() const { return status == QueryStatus::SUCCESS; }
    size_t row_count() const { return rows.size(); }
    bool empty() const { return rows.empty(); }
};

// Data models for trading system
namespace models {
    
    struct Trade {
        std::string id;
        std::string order_id;
        std::string platform;
        std::string token_in;
        std::string token_out;
        std::string side;
        int64_t amount_in;
        int64_t amount_out;
        double price;
        double slippage_percent;
        int64_t gas_used;
        int64_t gas_price;
        std::string transaction_hash;
        std::string status;
        std::chrono::system_clock::time_point created_at;
        std::chrono::system_clock::time_point executed_at;
        std::string wallet_address;
        std::string chain_id;
        
        Trade() = default;
        
        // Convert to SQL INSERT values
        std::string to_insert_sql() const;
    };
    
    struct Position {
        std::string id;
        std::string symbol;
        std::string wallet_address;
        double quantity;
        double average_price;
        double current_price;
        double unrealized_pnl;
        double realized_pnl;
        std::string status;
        std::chrono::system_clock::time_point opened_at;
        std::chrono::system_clock::time_point updated_at;
        
        Position() : quantity(0), average_price(0), current_price(0), 
                    unrealized_pnl(0), realized_pnl(0) {}
        
        std::string to_insert_sql() const;
    };
    
    struct RiskMetric {
        std::string id;
        std::string wallet_address;
        double portfolio_value;
        double daily_pnl;
        double var_95;
        double max_drawdown;
        double sharpe_ratio;
        double leverage_ratio;
        std::chrono::system_clock::time_point timestamp;
        
        RiskMetric() : portfolio_value(0), daily_pnl(0), var_95(0),
                      max_drawdown(0), sharpe_ratio(0), leverage_ratio(0) {}
        
        std::string to_insert_sql() const;
    };
    
    struct Alert {
        std::string id;
        std::string level;
        std::string type;
        std::string message;
        std::string symbol;
        bool acknowledged = false;
        std::chrono::system_clock::time_point created_at;
        
        Alert() = default;
        
        std::string to_insert_sql() const;
    };
}

// Repository pattern for data access
template<typename T>
class Repository {
public:
    virtual ~Repository() = default;
    
    virtual std::future<QueryResult> create(const T& entity) = 0;
    virtual std::future<QueryResult> update(const std::string& id, const T& entity) = 0;
    virtual std::future<QueryResult> delete_by_id(const std::string& id) = 0;
    virtual std::future<std::vector<T>> find_all() = 0;
    virtual std::future<std::optional<T>> find_by_id(const std::string& id) = 0;
    virtual std::future<std::vector<T>> find_by_criteria(const std::string& where_clause) = 0;
    virtual std::future<size_t> count() = 0;
};

// Connection pool for high-performance database access
class ConnectionPool {
public:
    explicit ConnectionPool(const DatabaseConfig& config);
    ~ConnectionPool();
    
    // Connection management
    bool initialize();
    void shutdown();
    bool is_healthy() const;
    
    // Query execution
    std::future<QueryResult> execute_query(const std::string& query);
    std::future<QueryResult> execute_prepared(const std::string& statement_name, 
                                            const std::vector<std::string>& params);
    
    // Transaction support
    std::string begin_transaction();
    bool commit_transaction(const std::string& transaction_id);
    bool rollback_transaction(const std::string& transaction_id);
    
    // Batch operations
    std::future<std::vector<QueryResult>> execute_batch(const std::vector<std::string>& queries);
    
    // Connection statistics
    struct PoolStats {
        uint32_t total_connections;
        uint32_t active_connections;
        uint32_t idle_connections;
        uint64_t total_queries;
        uint64_t successful_queries;
        uint64_t failed_queries;
        std::chrono::milliseconds avg_query_time{0};
        std::chrono::system_clock::time_point last_activity;
    };
    
    PoolStats get_stats() const;
    
private:
    class ConnectionPoolImpl;
    std::unique_ptr<ConnectionPoolImpl> pimpl_;
};

// Migration manager for database schema evolution
class MigrationManager {
public:
    explicit MigrationManager(std::shared_ptr<ConnectionPool> pool);
    ~MigrationManager();
    
    // Migration operations
    bool add_migration(const Migration& migration);
    bool apply_migrations();
    bool rollback_migration(const std::string& migration_id);
    std::vector<Migration> get_pending_migrations();
    std::vector<Migration> get_applied_migrations();
    
    // Schema operations
    bool create_migration_table();
    bool validate_schema();
    bool backup_schema(const std::string& backup_path);
    
private:
    std::shared_ptr<ConnectionPool> pool_;
    std::vector<Migration> migrations_;
    std::mutex migrations_mutex_;
};

// Main production database manager
class ProductionDatabaseManager {
public:
    explicit ProductionDatabaseManager(const std::unordered_map<DatabaseType, DatabaseConfig>& configs);
    ~ProductionDatabaseManager();
    
    // Lifecycle management
    bool initialize();
    void shutdown();
    bool is_healthy() const;
    
    // Database access
    std::shared_ptr<ConnectionPool> get_pool(DatabaseType type);
    std::future<QueryResult> execute_query(DatabaseType type, const std::string& query);
    
    // Repository access
    std::shared_ptr<Repository<models::Trade>> get_trade_repository();
    std::shared_ptr<Repository<models::Position>> get_position_repository();
    std::shared_ptr<Repository<models::RiskMetric>> get_risk_metric_repository();
    std::shared_ptr<Repository<models::Alert>> get_alert_repository();
    
    // Migration management
    bool apply_all_migrations();
    bool create_all_schemas();
    
    // Monitoring and maintenance
    void start_health_monitoring();
    void stop_health_monitoring();
    void cleanup_old_data(std::chrono::hours retention_period = std::chrono::hours(24 * 90)); // 90 days
    
    // Analytics and reporting
    std::future<QueryResult> get_trading_statistics(const std::string& time_period);
    std::future<QueryResult> get_performance_metrics(const std::string& wallet_address);
    std::future<QueryResult> get_risk_summary();
    
    // Real-time data streaming
    void enable_real_time_updates(bool enabled);
    void register_trade_callback(std::function<void(const models::Trade&)> callback);
    void register_position_callback(std::function<void(const models::Position&)> callback);
    void register_alert_callback(std::function<void(const models::Alert&)> callback);
    
    // Backup and recovery
    bool create_backup(const std::string& backup_path);
    bool restore_from_backup(const std::string& backup_path);
    bool verify_data_integrity();
    
    // Performance optimization
    bool optimize_tables();
    bool update_statistics();
    bool rebuild_indexes();
    
    // Configuration management
    void update_config(DatabaseType type, const DatabaseConfig& config);
    DatabaseConfig get_config(DatabaseType type) const;
    
    // Error handling and logging
    using ErrorCallback = std::function<void(const std::string& error, DatabaseType type)>;
    void register_error_callback(ErrorCallback callback);

private:
    class DatabaseManagerImpl;
    std::unique_ptr<DatabaseManagerImpl> pimpl_;
};

// Factory for creating database managers with preset configurations
class DatabaseFactory {
public:
    // Preset configurations
    static std::unordered_map<DatabaseType, DatabaseConfig> create_development_config();
    static std::unordered_map<DatabaseType, DatabaseConfig> create_production_config();
    static std::unordered_map<DatabaseType, DatabaseConfig> create_high_performance_config();
    static std::unordered_map<DatabaseType, DatabaseConfig> create_testing_config();
    
    // Component factories
    static std::unique_ptr<ProductionDatabaseManager> create_database_manager(
        const std::unordered_map<DatabaseType, DatabaseConfig>& configs);
    static std::shared_ptr<ConnectionPool> create_connection_pool(
        const DatabaseConfig& config);
    static std::unique_ptr<MigrationManager> create_migration_manager(
        std::shared_ptr<ConnectionPool> pool);
    
    // Schema generators
    static std::vector<Migration> create_trading_migrations();
    static std::vector<Migration> create_analytics_migrations();
    static std::vector<Migration> create_monitoring_migrations();
};

// Database utilities
namespace db_utils {
    
    // SQL query builders
    std::string build_select_query(const std::string& table, 
                                  const std::vector<std::string>& columns = {},
                                  const std::string& where_clause = "",
                                  const std::string& order_by = "",
                                  size_t limit = 0);
    
    std::string build_insert_query(const std::string& table,
                                  const std::unordered_map<std::string, std::string>& values);
    
    std::string build_update_query(const std::string& table,
                                  const std::unordered_map<std::string, std::string>& values,
                                  const std::string& where_clause);
    
    std::string build_delete_query(const std::string& table,
                                  const std::string& where_clause);
    
    // Data conversion utilities
    std::string escape_sql_string(const std::string& input);
    std::string format_timestamp(const std::chrono::system_clock::time_point& time);
    std::chrono::system_clock::time_point parse_timestamp(const std::string& timestamp_str);
    
    // Connection string builders
    std::string build_postgresql_connection_string(const DatabaseConfig& config);
    std::string build_clickhouse_connection_string(const DatabaseConfig& config);
    
    // Performance utilities
    std::string generate_partition_sql(const std::string& table, 
                                      const std::string& partition_column,
                                      const std::string& partition_type);
    std::string generate_index_sql(const std::string& table,
                                  const std::vector<std::string>& columns,
                                  const std::string& index_name = "");
}

} // namespace hfx::db
