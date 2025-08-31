#pragma once

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <atomic>
#include <mutex>
#include <chrono>
#include <functional>
#include <future>
#include <queue>

namespace hfx::ultra {

// Database backend types
enum class DatabaseBackend {
    POSTGRESQL,         // PostgreSQL with TimescaleDB
    CLICKHOUSE,        // ClickHouse for analytics
    SCYLLA_DB,         // ScyllaDB for ultra-high throughput
    REDIS_STREAMS,     // Redis Streams for real-time data
    SQLITE_MEMORY      // SQLite in-memory for testing
};

// Table partition strategies
enum class PartitionStrategy {
    TIME_BASED,        // Partition by time (hourly, daily, weekly)
    HASH_BASED,        // Partition by hash of key field
    RANGE_BASED,       // Partition by value ranges
    HYBRID             // Combination of time and hash
};

// Index types for optimization
enum class IndexType {
    BTREE,             // Standard B-tree index
    HASH,              // Hash index for equality lookups
    GIN,               // Generalized Inverted Index (PostgreSQL)
    BRIN,              // Block Range Index for large tables
    BLOOM,             // Bloom filter index
    SPATIAL,           // For geographical data
    PARTIAL            // Partial index with WHERE clause
};

// Connection pool configuration
struct ConnectionPoolConfig {
    std::string connection_string;
    uint32_t min_connections = 5;
    uint32_t max_connections = 50;
    uint32_t connection_timeout_ms = 5000;
    uint32_t idle_timeout_ms = 300000;  // 5 minutes
    uint32_t max_retries = 3;
    bool enable_prepared_statements = true;
    bool enable_connection_validation = true;
};

// Database schema configuration
struct SchemaConfig {
    std::string schema_name;
    std::string description;
    DatabaseBackend backend;
    bool enable_compression = true;
    bool enable_encryption = false;
    std::string encryption_key;
    
    // Performance settings
    uint32_t batch_size = 1000;
    uint32_t write_buffer_size_mb = 64;
    uint32_t cache_size_mb = 256;
    bool enable_wal = true;             // Write-ahead logging
    bool enable_fsync = false;          // Disable for performance (risk)
    
    // Partitioning settings
    PartitionStrategy partition_strategy = PartitionStrategy::TIME_BASED;
    std::chrono::hours partition_interval{24};  // Daily partitions
    uint32_t retention_days = 90;
    bool auto_drop_old_partitions = true;
};

// Table definition with partitioning
struct TableDefinition {
    std::string name;
    std::string description;
    PartitionStrategy partition_strategy;
    std::string partition_key;
    std::chrono::hours partition_interval{24};
    
    // Column definitions
    struct Column {
        std::string name;
        std::string type;
        bool nullable = true;
        bool primary_key = false;
        std::string default_value;
        std::string constraint;
    };
    std::vector<Column> columns;
    
    // Index definitions
    struct Index {
        std::string name;
        IndexType type;
        std::vector<std::string> columns;
        bool unique = false;
        std::string where_clause;  // For partial indexes
        std::string expression;    // For expression indexes
    };
    std::vector<Index> indexes;
    
    // Constraints
    std::vector<std::string> constraints;
    
    // Storage settings
    uint32_t fillfactor = 90;          // For PostgreSQL
    bool enable_compression = true;
    std::string compression_algorithm = "lz4";
};

// High-performance batch insertion interface
struct BatchInsertConfig {
    std::string table_name;
    uint32_t batch_size = 1000;
    uint32_t max_batch_time_ms = 100;   // Maximum time to wait for batch
    bool ignore_duplicates = false;
    bool enable_transactions = true;
    uint32_t parallel_workers = 4;
};

// Query result interface
struct QueryResult {
    bool success;
    std::string error_message;
    uint64_t rows_affected;
    std::chrono::milliseconds execution_time;
    std::vector<std::vector<std::string>> rows;
    std::vector<std::string> column_names;
    std::vector<std::string> column_types;
};

// Production database manager with high-performance features
class ProductionDatabase {
public:
    explicit ProductionDatabase(const SchemaConfig& config);
    ~ProductionDatabase();
    
    // Connection management
    bool connect();
    bool disconnect();
    bool is_connected() const { return connected_.load(); }
    
    // Schema management
    bool create_schema();
    bool drop_schema();
    bool create_table(const TableDefinition& table_def);
    bool drop_table(const std::string& table_name);
    bool create_partition(const std::string& table_name, const std::string& partition_name,
                         const std::string& start_range, const std::string& end_range);
    
    // Index management
    bool create_index(const std::string& table_name, const TableDefinition::Index& index_def);
    bool drop_index(const std::string& index_name);
    std::vector<std::string> list_indexes(const std::string& table_name) const;
    
    // High-performance data operations
    QueryResult execute_query(const std::string& query, const std::vector<std::string>& params = {});
    std::future<QueryResult> execute_query_async(const std::string& query, 
                                                const std::vector<std::string>& params = {});
    
    // Batch operations for high throughput
    bool batch_insert(const std::string& table_name, const std::vector<std::vector<std::string>>& rows);
    bool batch_update(const std::string& table_name, const std::vector<std::vector<std::string>>& rows,
                     const std::vector<std::string>& key_columns);
    bool batch_upsert(const std::string& table_name, const std::vector<std::vector<std::string>>& rows,
                     const std::vector<std::string>& conflict_columns);
    
    // Streaming insert for real-time data
    class StreamInserter {
    public:
        StreamInserter(ProductionDatabase* db, const BatchInsertConfig& config);
        ~StreamInserter();
        
        bool insert_row(const std::vector<std::string>& row);
        bool flush();
        
        struct Metrics {
            std::atomic<uint64_t> rows_inserted{0};
            std::atomic<uint64_t> batches_processed{0};
            std::atomic<uint64_t> errors{0};
            std::atomic<double> avg_batch_time_ms{0.0};
        };
        const Metrics& get_metrics() const { return metrics_; }
        
    private:
        ProductionDatabase* db_;
        BatchInsertConfig config_;
        std::vector<std::vector<std::string>> batch_buffer_;
        std::chrono::steady_clock::time_point batch_start_time_;
        std::thread flush_thread_;
        std::atomic<bool> running_{false};
        mutable Metrics metrics_;
        std::mutex buffer_mutex_;
        std::condition_variable flush_condition_;
        
        void flush_worker();
        void flush_batch();
    };
    
    std::unique_ptr<StreamInserter> create_stream_inserter(const BatchInsertConfig& config);
    
    // Transaction management
    bool begin_transaction();
    bool commit_transaction();
    bool rollback_transaction();
    
    // Partition management
    bool create_time_partition(const std::string& table_name, 
                              std::chrono::system_clock::time_point start_time,
                              std::chrono::system_clock::time_point end_time);
    bool drop_old_partitions(const std::string& table_name, std::chrono::hours older_than);
    std::vector<std::string> list_partitions(const std::string& table_name) const;
    
    // Performance monitoring
    struct DatabaseMetrics {
        std::atomic<uint64_t> total_queries{0};
        std::atomic<uint64_t> successful_queries{0};
        std::atomic<uint64_t> failed_queries{0};
        std::atomic<uint64_t> total_connections{0};
        std::atomic<uint64_t> active_connections{0};
        std::atomic<double> avg_query_time_ms{0.0};
        std::atomic<uint64_t> bytes_written{0};
        std::atomic<uint64_t> bytes_read{0};
        std::atomic<uint64_t> cache_hits{0};
        std::atomic<uint64_t> cache_misses{0};
    };
    
    const DatabaseMetrics& get_metrics() const { return metrics_; }
    void reset_metrics();
    
    // Database health and maintenance
    struct DatabaseHealth {
        bool is_connected;
        bool is_writable;
        double cpu_usage_percent;
        double memory_usage_percent;
        double disk_usage_percent;
        uint64_t active_queries;
        uint64_t blocked_queries;
        std::chrono::milliseconds avg_response_time;
        std::string database_version;
        std::chrono::system_clock::time_point last_vacuum;
    };
    
    DatabaseHealth get_health_status() const;
    bool perform_maintenance();
    bool vacuum_analyze(const std::string& table_name = "");
    bool reindex_table(const std::string& table_name);
    
    // Connection pool management
    bool configure_connection_pool(const ConnectionPoolConfig& pool_config);
    uint32_t get_active_connections() const;
    uint32_t get_available_connections() const;
    
    // Backup and recovery
    bool create_backup(const std::string& backup_path, bool compress = true);
    bool restore_backup(const std::string& backup_path);
    bool create_point_in_time_recovery_point(const std::string& label);
    
private:
    SchemaConfig config_;
    std::atomic<bool> connected_{false};
    mutable DatabaseMetrics metrics_;
    
    // Database connection
    void* db_connection_;                // Database-specific connection handle
    std::mutex connection_mutex_;
    
    // Connection pool
    ConnectionPoolConfig pool_config_;
    std::vector<void*> connection_pool_;
    std::queue<void*> available_connections_;
    std::mutex pool_mutex_;
    std::condition_variable pool_condition_;
    
    // Transaction state
    std::atomic<bool> in_transaction_{false};
    std::string current_transaction_id_;
    
    // Internal methods
    bool connect_to_database();
    void disconnect_from_database();
    void* get_connection();
    void return_connection(void* connection);
    
    QueryResult execute_internal(const std::string& query, const std::vector<std::string>& params);
    std::string build_create_table_sql(const TableDefinition& table_def) const;
    std::string build_create_index_sql(const std::string& table_name, 
                                      const TableDefinition::Index& index_def) const;
    std::string build_partition_sql(const std::string& table_name, const std::string& partition_name,
                                   const std::string& start_range, const std::string& end_range) const;
    
    void update_query_metrics(std::chrono::milliseconds execution_time, bool success);
    std::string generate_transaction_id();
    
    // Backend-specific implementations
    bool create_postgresql_table(const TableDefinition& table_def);
    bool create_clickhouse_table(const TableDefinition& table_def);
    bool create_scylla_table(const TableDefinition& table_def);
};

// Pre-defined table schemas for trading data
namespace trading_schemas {
    
    // Market data tables
    TableDefinition create_price_data_table();
    TableDefinition create_orderbook_table();
    TableDefinition create_trades_table();
    TableDefinition create_volume_data_table();
    
    // Trading execution tables
    TableDefinition create_orders_table();
    TableDefinition create_executions_table();
    TableDefinition create_positions_table();
    TableDefinition create_portfolio_table();
    
    // MEV and arbitrage tables
    TableDefinition create_mev_opportunities_table();
    TableDefinition create_arbitrage_trades_table();
    TableDefinition create_sandwich_attacks_table();
    
    // Risk management tables
    TableDefinition create_risk_metrics_table();
    TableDefinition create_position_limits_table();
    TableDefinition create_alerts_table();
    
    // Audit and compliance tables
    TableDefinition create_audit_log_table();
    TableDefinition create_compliance_events_table();
    TableDefinition create_transaction_history_table();
    
    // Performance monitoring tables
    TableDefinition create_system_metrics_table();
    TableDefinition create_latency_measurements_table();
    TableDefinition create_error_logs_table();
}

// Factory for creating database instances
class DatabaseFactory {
public:
    static std::unique_ptr<ProductionDatabase> create_postgresql_database(const std::string& connection_string);
    static std::unique_ptr<ProductionDatabase> create_clickhouse_database(const std::string& connection_string);
    static std::unique_ptr<ProductionDatabase> create_scylla_database(const std::string& connection_string);
    static std::unique_ptr<ProductionDatabase> create_redis_database(const std::string& connection_string);
    static std::unique_ptr<ProductionDatabase> create_memory_database(); // For testing
    
    // High-performance configurations
    static std::unique_ptr<ProductionDatabase> create_high_frequency_database();
    static std::unique_ptr<ProductionDatabase> create_analytics_database();
    static std::unique_ptr<ProductionDatabase> create_audit_database();
};

// Database migration utilities
class DatabaseMigration {
public:
    struct Migration {
        std::string version;
        std::string description;
        std::vector<std::string> up_scripts;
        std::vector<std::string> down_scripts;
        std::chrono::system_clock::time_point created_at;
    };
    
    explicit DatabaseMigration(std::shared_ptr<ProductionDatabase> db);
    
    bool create_migration_table();
    bool apply_migration(const Migration& migration);
    bool rollback_migration(const std::string& version);
    std::vector<Migration> get_pending_migrations() const;
    std::string get_current_version() const;
    
    // Pre-defined migrations for trading system
    std::vector<Migration> get_trading_system_migrations() const;
    
private:
    std::shared_ptr<ProductionDatabase> db_;
    std::vector<Migration> applied_migrations_;
    
    bool is_migration_applied(const std::string& version) const;
    bool record_migration(const Migration& migration);
};

// Utility functions for database operations
namespace db_utils {
    std::string escape_sql_string(const std::string& input);
    std::string build_insert_sql(const std::string& table_name, 
                                const std::vector<std::string>& columns,
                                const std::vector<std::vector<std::string>>& rows);
    std::string build_upsert_sql(const std::string& table_name,
                                const std::vector<std::string>& columns,
                                const std::vector<std::string>& conflict_columns,
                                const std::vector<std::vector<std::string>>& rows);
    
    // Time-based utilities
    std::string format_timestamp(std::chrono::system_clock::time_point time_point);
    std::string get_partition_name(const std::string& table_name, 
                                  std::chrono::system_clock::time_point time_point);
    std::vector<std::string> get_partition_range(std::chrono::system_clock::time_point start_time,
                                                std::chrono::hours interval);
    
    // Performance utilities
    std::string optimize_query(const std::string& query, DatabaseBackend backend);
    std::vector<std::string> analyze_query_plan(const std::string& query, ProductionDatabase* db);
    std::string suggest_indexes(const std::string& table_name, const std::string& query,
                               ProductionDatabase* db);
}

} // namespace hfx::ultra
