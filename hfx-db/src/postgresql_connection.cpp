/**
 * @file postgresql_connection.cpp
 * @brief PostgreSQL database connection implementation
 */

#include "database_connection.hpp"
#include <libpq-fe.h>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <chrono>
#include <thread>
#include <memory>
#include <mutex>

namespace hfx::db {

// PostgreSQL result wrapper
class PostgreSQLResult : public DatabaseResult {
public:
    PostgreSQLResult(PGresult* result) : result_(result), row_(0), col_(0) {}
    ~PostgreSQLResult() { PQclear(result_); }

    bool isValid() const override {
        return result_ != nullptr && PQresultStatus(result_) == PGRES_TUPLES_OK;
    }

    size_t rowCount() const override {
        return result_ ? PQntuples(result_) : 0;
    }

    size_t columnCount() const override {
        return result_ ? PQnfields(result_) : 0;
    }

    std::vector<std::string> columnNames() const override {
        std::vector<std::string> names;
        if (!result_) return names;

        int nfields = PQnfields(result_);
        for (int i = 0; i < nfields; ++i) {
            names.push_back(PQfname(result_, i));
        }
        return names;
    }

    std::optional<std::string> getString(size_t row, size_t col) const override {
        if (!result_ || row >= rowCount() || col >= columnCount()) {
            return std::nullopt;
        }

        if (PQgetisnull(result_, row, col)) {
            return std::nullopt;
        }

        return std::string(PQgetvalue(result_, row, col));
    }

    std::optional<int64_t> getInt64(size_t row, size_t col) const override {
        auto str = getString(row, col);
        if (!str) return std::nullopt;

        try {
            return std::stoll(*str);
        } catch (const std::exception&) {
            return std::nullopt;
        }
    }

    std::optional<double> getDouble(size_t row, size_t col) const override {
        auto str = getString(row, col);
        if (!str) return std::nullopt;

        try {
            return std::stod(*str);
        } catch (const std::exception&) {
            return std::nullopt;
        }
    }

    std::optional<bool> getBool(size_t row, size_t col) const override {
        auto str = getString(row, col);
        if (!str) return std::nullopt;

        return (*str == "t" || *str == "true" || *str == "1");
    }

private:
    PGresult* result_;
    size_t row_;
    size_t col_;
};

// PostgreSQL connection implementation
class PostgreSQLConnection : public DatabaseConnection {
public:
    PostgreSQLConnection() : conn_(nullptr), in_transaction_(false) {}
    ~PostgreSQLConnection() { disconnect(); }

    bool connect(const DatabaseConfig& config) override {
        std::lock_guard<std::mutex> lock(mutex_);

        // Build connection string
        std::stringstream conn_str;
        conn_str << "host=" << config.host
                 << " port=" << config.port
                 << " dbname=" << config.database
                 << " user=" << config.username
                 << " password=" << config.password
                 << " connect_timeout=" << std::chrono::duration_cast<std::chrono::seconds>(config.connection_timeout).count();

        if (config.enable_ssl) {
            conn_str << " sslmode=require";
            if (!config.ssl_ca_file.empty()) {
                conn_str << " sslrootcert=" << config.ssl_ca_file;
            }
            if (!config.ssl_cert_file.empty()) {
                conn_str << " sslcert=" << config.ssl_cert_file;
            }
            if (!config.ssl_key_file.empty()) {
                conn_str << " sslkey=" << config.ssl_key_file;
            }
        }

        conn_ = PQconnectdb(conn_str.str().c_str());
        if (PQstatus(conn_) != CONNECTION_OK) {
            HFX_LOG_ERROR("[PostgreSQL] Connection failed: " << PQerrorMessage(conn_) << std::endl;
            disconnect();
            return false;
        }

        // Set client encoding to UTF8
        PQsetClientEncoding(conn_, "UTF8");

        return true;
    }

    void disconnect() override {
        std::lock_guard<std::mutex> lock(mutex_);
        if (conn_) {
            PQfinish(conn_);
            conn_ = nullptr;
        }
        in_transaction_ = false;
    }

    bool isConnected() const override {
        std::lock_guard<std::mutex> lock(mutex_);
        return conn_ && PQstatus(conn_) == CONNECTION_OK;
    }

    bool reconnect() override {
        disconnect();
        // Note: This would need the stored config, simplified for now
        return false;
    }

    std::unique_ptr<DatabaseResult> executeQuery(const std::string& query) override {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!isConnected()) return nullptr;

        PGresult* result = PQexec(conn_, query.c_str());
        if (!result) {
            HFX_LOG_ERROR("[PostgreSQL] Query execution failed: " << PQerrorMessage(conn_) << std::endl;
            return nullptr;
        }

        ExecStatusType status = PQresultStatus(result);
        if (status != PGRES_TUPLES_OK && status != PGRES_COMMAND_OK) {
            HFX_LOG_ERROR("[PostgreSQL] Query failed: " << PQerrorMessage(conn_) << std::endl;
            PQclear(result);
            return nullptr;
        }

        return std::make_unique<PostgreSQLResult>(result);
    }

    bool executeCommand(const std::string& command) override {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!isConnected()) return false;

        PGresult* result = PQexec(conn_, command.c_str());
        if (!result) {
            HFX_LOG_ERROR("[PostgreSQL] Command execution failed: " << PQerrorMessage(conn_) << std::endl;
            return false;
        }

        ExecStatusType status = PQresultStatus(result);
        PQclear(result);

        if (status != PGRES_COMMAND_OK) {
            HFX_LOG_ERROR("[PostgreSQL] Command failed: " << PQerrorMessage(conn_) << std::endl;
            return false;
        }

        return true;
    }

    std::string escapeString(const std::string& str) const override {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!isConnected()) return str;

        char* escaped = PQescapeLiteral(conn_, str.c_str(), str.length());
        if (!escaped) return str;

        std::string result(escaped);
        PQfreemem(escaped);
        return result;
    }

    bool beginTransaction() override {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!isConnected() || in_transaction_) return false;

        if (!executeCommand("BEGIN")) return false;

        in_transaction_ = true;
        return true;
    }

    bool commitTransaction() override {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!isConnected() || !in_transaction_) return false;

        bool success = executeCommand("COMMIT");
        in_transaction_ = false;
        return success;
    }

    bool rollbackTransaction() override {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!isConnected() || !in_transaction_) return false;

        bool success = executeCommand("ROLLBACK");
        in_transaction_ = false;
        return success;
    }

    bool inTransaction() const override {
        std::lock_guard<std::mutex> lock(mutex_);
        return in_transaction_;
    }

    bool prepareStatement(const std::string& name, const std::string& query) override {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!isConnected()) return false;

        PGresult* result = PQprepare(conn_, name.c_str(), query.c_str(), 0, nullptr);
        if (!result) {
            HFX_LOG_ERROR("[PostgreSQL] Prepare statement failed: " << PQerrorMessage(conn_) << std::endl;
            return false;
        }

        ExecStatusType status = PQresultStatus(result);
        PQclear(result);

        return status == PGRES_COMMAND_OK;
    }

    std::unique_ptr<DatabaseResult> executePrepared(
        const std::string& name,
        const std::vector<std::string>& params) override {

        std::lock_guard<std::mutex> lock(mutex_);
        if (!isConnected()) return nullptr;

        std::vector<const char*> param_values;
        for (const auto& param : params) {
            param_values.push_back(param.c_str());
        }

        PGresult* result = PQexecPrepared(conn_, name.c_str(),
                                        param_values.size(),
                                        param_values.data(),
                                        nullptr, nullptr, 0);

        if (!result) {
            HFX_LOG_ERROR("[PostgreSQL] Execute prepared failed: " << PQerrorMessage(conn_) << std::endl;
            return nullptr;
        }

        ExecStatusType status = PQresultStatus(result);
        if (status != PGRES_TUPLES_OK && status != PGRES_COMMAND_OK) {
            HFX_LOG_ERROR("[PostgreSQL] Execute prepared failed: " << PQerrorMessage(conn_) << std::endl;
            PQclear(result);
            return nullptr;
        }

        return std::make_unique<PostgreSQLResult>(result);
    }

private:
    PGconn* conn_;
    mutable std::mutex mutex_;
    bool in_transaction_;
};

// Factory functions
std::unique_ptr<DatabaseConnection> DatabaseFactory::createPostgreSQLConnection() {
    return std::make_unique<PostgreSQLConnection>();
}

} // namespace hfx::db
