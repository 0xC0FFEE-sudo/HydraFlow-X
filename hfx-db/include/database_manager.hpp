/**
 * @file database_manager.hpp
 * @brief Database manager for coordinating database operations
 */

#pragma once

#include "database_connection.hpp"
#include "repositories.hpp"
#include "data_models.hpp"
#include <memory>
#include <string>
#include <vector>
#include <functional>

namespace hfx::db {

/**
 * @brief Database manager configuration
 */
struct DatabaseManagerConfig {
    DatabaseConfig postgresql_config;
    ClickHouseConfig clickhouse_config;
    bool enable_clickhouse = false;
    bool enable_connection_pooling = true;
    std::chrono::seconds health_check_interval = std::chrono::seconds(30);
    bool enable_metrics_collection = true;
};

/**
 * @brief Database operation callback for monitoring
 */
using DatabaseCallback = std::function<void(const std::string& operation,
                                          std::chrono::milliseconds duration,
                                          bool success,
                                          const std::string& error_message)>;

/**
 * @brief Database manager - main interface for all database operations
 */
class DatabaseManager {
public:
    explicit DatabaseManager(const DatabaseManagerConfig& config);
    ~DatabaseManager();

    // Initialization and connection management
    bool initialize();
    void shutdown();
    bool isHealthy() const;
    HealthCheckResult getHealthStatus() const;

    // Repository access
    std::shared_ptr<TradeRepository> getTradeRepository();
    std::shared_ptr<PositionRepository> getPositionRepository();
    std::shared_ptr<MarketDataRepository> getMarketDataRepository();
    std::shared_ptr<LiquidityPoolRepository> getLiquidityPoolRepository();
    std::shared_ptr<AnalyticsRepository> getAnalyticsRepository();
    std::shared_ptr<RiskMetricsRepository> getRiskMetricsRepository();
    std::shared_ptr<PerformanceMetricsRepository> getPerformanceMetricsRepository();

    // Direct database operations (convenience methods)
    bool saveTrade(const Trade& trade);
    bool savePosition(const Position& position);
    bool saveMarketData(const MarketData& data);
    bool saveAnalyticsData(const AnalyticsData& data);
    bool savePerformanceMetrics(const PerformanceMetrics& metrics);

    // Batch operations for performance
    bool saveTradesBatch(const std::vector<Trade>& trades);
    bool saveMarketDataBatch(const std::vector<MarketData>& market_data);
    bool saveAnalyticsDataBatch(const std::vector<AnalyticsData>& analytics_data);

    // Query operations
    std::optional<Trade> getTradeById(const std::string& trade_id);
    std::vector<Trade> getRecentTrades(size_t limit = 100);
    std::optional<double> getTokenPrice(const std::string& token_address);
    std::vector<std::pair<std::string, double>> getAllTokenPrices();
    double getTotalPortfolioValue(const std::string& wallet_address);
    double getTotalPnL24h();

    // Monitoring and callbacks
    void setOperationCallback(DatabaseCallback callback);
    void removeOperationCallback();

    // Database maintenance
    bool createTables();
    bool runMigrations();
    bool backupDatabase(const std::string& backup_path);
    bool optimizeTables();

    // Statistics and metrics
    struct DatabaseStats {
        uint64_t total_connections = 0;
        uint64_t active_connections = 0;
        uint64_t total_queries = 0;
        uint64_t failed_queries = 0;
        std::chrono::milliseconds avg_query_time = std::chrono::milliseconds(0);
        std::chrono::system_clock::time_point last_health_check;
        bool postgresql_healthy = false;
        bool clickhouse_healthy = false;
    };

    DatabaseStats getStatistics() const;

private:
    class DatabaseManagerImpl;
    std::unique_ptr<DatabaseManagerImpl> pimpl_;
};

/**
 * @brief Database transaction wrapper for atomic operations
 */
class DatabaseTransaction {
public:
    explicit DatabaseTransaction(std::shared_ptr<DatabaseConnection> conn);
    ~DatabaseTransaction();

    // Transaction control
    bool begin();
    bool commit();
    bool rollback();
    bool isActive() const;

    // Execute operations within transaction
    std::unique_ptr<DatabaseResult> executeQuery(const std::string& query);
    bool executeCommand(const std::string& command);

private:
    std::shared_ptr<DatabaseConnection> connection_;
    bool active_ = false;
};

/**
 * @brief Database migration manager
 */
class DatabaseMigrationManager {
public:
    explicit DatabaseMigrationManager(std::shared_ptr<DatabaseConnection> conn);
    ~DatabaseMigrationManager() = default;

    bool runMigrations();
    bool createInitialSchema();
    uint32_t getCurrentVersion() const;
    std::vector<std::string> getPendingMigrations() const;

private:
    std::shared_ptr<DatabaseConnection> connection_;
    std::vector<std::string> migration_files_;
};

/**
 * @brief Database backup manager
 */
class DatabaseBackupManager {
public:
    explicit DatabaseBackupManager(std::shared_ptr<DatabaseConnection> conn);
    ~DatabaseBackupManager() = default;

    bool createBackup(const std::string& backup_path);
    bool restoreBackup(const std::string& backup_path);
    std::vector<std::string> listBackups(const std::string& backup_dir) const;

private:
    std::shared_ptr<DatabaseConnection> connection_;
};

} // namespace hfx::db
