/**
 * @file database_connection.hpp
 * @brief Database connection interfaces for HydraFlow-X
 */

#pragma once

#include <memory>
#include <string>
#include <vector>
#include <optional>
#include <chrono>
#include <functional>

namespace hfx::db {

/**
 * @brief Database configuration
 */
struct DatabaseConfig {
    std::string host = "localhost";
    uint16_t port = 5432;
    std::string database = "hydraflow";
    std::string username = "hydraflow";
    std::string password;
    uint32_t max_connections = 10;
    std::chrono::milliseconds connection_timeout = std::chrono::seconds(30);
    uint32_t max_retries = 3;
    bool enable_ssl = false;
    std::string ssl_ca_file;
    std::string ssl_cert_file;
    std::string ssl_key_file;
};

/**
 * @brief ClickHouse configuration
 */
struct ClickHouseConfig {
    std::string host = "localhost";
    uint16_t port = 9000;
    std::string database = "hydraflow_analytics";
    std::string username = "default";
    std::string password;
    uint32_t max_connections = 5;
    std::chrono::milliseconds connection_timeout = std::chrono::seconds(10);
};

/**
 * @brief Generic database result
 */
class DatabaseResult {
public:
    virtual ~DatabaseResult() = default;

    virtual bool isValid() const = 0;
    virtual size_t rowCount() const = 0;
    virtual size_t columnCount() const = 0;
    virtual std::vector<std::string> columnNames() const = 0;

    virtual std::optional<std::string> getString(size_t row, size_t col) const = 0;
    virtual std::optional<int64_t> getInt64(size_t row, size_t col) const = 0;
    virtual std::optional<double> getDouble(size_t row, size_t col) const = 0;
    virtual std::optional<bool> getBool(size_t row, size_t col) const = 0;
};

/**
 * @brief Database connection interface
 */
class DatabaseConnection {
public:
    virtual ~DatabaseConnection() = default;

    virtual bool connect(const DatabaseConfig& config) = 0;
    virtual void disconnect() = 0;
    virtual bool isConnected() const = 0;
    virtual bool reconnect() = 0;

    virtual std::unique_ptr<DatabaseResult> executeQuery(const std::string& query) = 0;
    virtual bool executeCommand(const std::string& command) = 0;
    virtual std::string escapeString(const std::string& str) const = 0;

    // Transaction support
    virtual bool beginTransaction() = 0;
    virtual bool commitTransaction() = 0;
    virtual bool rollbackTransaction() = 0;
    virtual bool inTransaction() const = 0;

    // Prepared statements
    virtual bool prepareStatement(const std::string& name, const std::string& query) = 0;
    virtual std::unique_ptr<DatabaseResult> executePrepared(
        const std::string& name,
        const std::vector<std::string>& params) = 0;
};

/**
 * @brief Connection pool for database connections
 */
class ConnectionPool {
public:
    virtual ~ConnectionPool() = default;

    virtual std::shared_ptr<DatabaseConnection> getConnection() = 0;
    virtual void returnConnection(std::shared_ptr<DatabaseConnection> conn) = 0;
    virtual size_t getActiveConnections() const = 0;
    virtual size_t getIdleConnections() const = 0;
    virtual size_t getMaxConnections() const = 0;
};

/**
 * @brief Database factory for creating connections
 */
class DatabaseFactory {
public:
    static std::unique_ptr<DatabaseConnection> createPostgreSQLConnection();
    static std::unique_ptr<DatabaseConnection> createClickHouseConnection();
    static std::unique_ptr<ConnectionPool> createConnectionPool(
        const DatabaseConfig& config,
        const std::string& type = "postgresql");
};

/**
 * @brief Database health check result
 */
struct HealthCheckResult {
    bool isHealthy = false;
    std::chrono::milliseconds responseTime = std::chrono::milliseconds(0);
    std::string errorMessage;
    std::chrono::system_clock::time_point lastCheck;
    uint64_t totalQueries = 0;
    uint64_t failedQueries = 0;
};

/**
 * @brief Database health checker
 */
class DatabaseHealthChecker {
public:
    virtual ~DatabaseHealthChecker() = default;
    virtual HealthCheckResult checkHealth() = 0;
    virtual void updateMetrics(uint64_t queries, uint64_t failures) = 0;
};

} // namespace hfx::db
