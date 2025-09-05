/**
 * @file clickhouse_connection.cpp
 * @brief ClickHouse database connection implementation
 */

#include "database_connection.hpp"
#include <iostream>
#include <sstream>
#include <curl/curl.h>
#include <nlohmann/json.hpp>

namespace hfx::db {

// ClickHouse result wrapper
class ClickHouseResult : public DatabaseResult {
public:
    ClickHouseResult(const nlohmann::json& data) : data_(data) {}
    ~ClickHouseResult() = default;

    bool isValid() const override {
        return !data_.is_null();
    }

    size_t rowCount() const override {
        if (!data_.is_array()) return 0;
        return data_.size();
    }

    size_t columnCount() const override {
        if (!data_.is_array() || data_.empty()) return 0;
        if (!data_[0].is_object()) return 0;
        return data_[0].size();
    }

    std::vector<std::string> columnNames() const override {
        std::vector<std::string> names;
        if (!data_.is_array() || data_.empty()) return names;
        if (!data_[0].is_object()) return names;

        for (const auto& [key, value] : data_[0].items()) {
            names.push_back(key);
        }
        return names;
    }

    std::optional<std::string> getString(size_t row, size_t col) const override {
        if (row >= rowCount()) return std::nullopt;

        const auto& row_data = data_[row];
        if (!row_data.is_object()) return std::nullopt;

        auto names = columnNames();
        if (col >= names.size()) return std::nullopt;

        const std::string& col_name = names[col];
        if (!row_data.contains(col_name)) return std::nullopt;

        const auto& value = row_data[col_name];
        if (value.is_null()) return std::nullopt;

        return value.dump(); // JSON representation
    }

    std::optional<int64_t> getInt64(size_t row, size_t col) const override {
        if (row >= rowCount()) return std::nullopt;

        const auto& row_data = data_[row];
        if (!row_data.is_object()) return std::nullopt;

        auto names = columnNames();
        if (col >= names.size()) return std::nullopt;

        const std::string& col_name = names[col];
        if (!row_data.contains(col_name)) return std::nullopt;

        const auto& value = row_data[col_name];
        if (!value.is_number_integer()) return std::nullopt;

        return value.get<int64_t>();
    }

    std::optional<double> getDouble(size_t row, size_t col) const override {
        if (row >= rowCount()) return std::nullopt;

        const auto& row_data = data_[row];
        if (!row_data.is_object()) return std::nullopt;

        auto names = columnNames();
        if (col >= names.size()) return std::nullopt;

        const std::string& col_name = names[col];
        if (!row_data.contains(col_name)) return std::nullopt;

        const auto& value = row_data[col_name];
        if (!value.is_number()) return std::nullopt;

        return value.get<double>();
    }

    std::optional<bool> getBool(size_t row, size_t col) const override {
        if (row >= rowCount()) return std::nullopt;

        const auto& row_data = data_[row];
        if (!row_data.is_object()) return std::nullopt;

        auto names = columnNames();
        if (col >= names.size()) return std::nullopt;

        const std::string& col_name = names[col];
        if (!row_data.contains(col_name)) return std::nullopt;

        const auto& value = row_data[col_name];
        if (!value.is_boolean()) return std::nullopt;

        return value.get<bool>();
    }

private:
    nlohmann::json data_;
};

// ClickHouse connection implementation
class ClickHouseConnection : public DatabaseConnection {
public:
    ClickHouseConnection() : config_(), connected_(false) {}
    ~ClickHouseConnection() = default;

    bool connect(const DatabaseConfig& config) override {
        config_ = config;
        connected_ = true; // ClickHouse HTTP is stateless, so we consider it always "connected"
        return true;
    }

    void disconnect() override {
        connected_ = false;
    }

    bool isConnected() const override {
        return connected_;
    }

    bool reconnect() override {
        return connected_;
    }

    std::unique_ptr<DatabaseResult> executeQuery(const std::string& query) override {
        if (!isConnected()) return nullptr;

        // Build ClickHouse HTTP URL
        std::stringstream url;
        url << "http://" << config_.host << ":" << config_.port
            << "/?database=" << config_.database
            << "&user=" << config_.username
            << "&password=" << config_.password;

        // Execute query via HTTP POST
        std::string response = makeHttpRequest(url.str(), query);
        if (response.empty()) {
            return nullptr;
        }

        try {
            // ClickHouse returns JSON format data
            nlohmann::json json_data = nlohmann::json::parse(response);
            return std::make_unique<ClickHouseResult>(json_data);
        } catch (const std::exception& e) {
            HFX_LOG_ERROR("[ERROR] Message");
            return nullptr;
        }
    }

    bool executeCommand(const std::string& command) override {
        auto result = executeQuery(command);
        return result && result->isValid();
    }

    std::string escapeString(const std::string& str) const override {
        // ClickHouse uses single quotes for string literals
        std::string escaped = str;
        // Escape single quotes by doubling them
        size_t pos = 0;
        while ((pos = escaped.find('\'', pos)) != std::string::npos) {
            escaped.replace(pos, 1, "''");
            pos += 2;
        }
        return "'" + escaped + "'";
    }

    bool beginTransaction() override {
        // ClickHouse doesn't support transactions in the traditional sense
        return true;
    }

    bool commitTransaction() override {
        // ClickHouse doesn't support transactions in the traditional sense
        return true;
    }

    bool rollbackTransaction() override {
        // ClickHouse doesn't support transactions in the traditional sense
        return true;
    }

    bool inTransaction() const override {
        return false;
    }

    bool prepareStatement(const std::string& name, const std::string& query) override {
        // ClickHouse doesn't support prepared statements like PostgreSQL
        prepared_statements_[name] = query;
        return true;
    }

    std::unique_ptr<DatabaseResult> executePrepared(
        const std::string& name,
        const std::vector<std::string>& params) override {

        auto it = prepared_statements_.find(name);
        if (it == prepared_statements_.end()) {
            return nullptr;
        }

        std::string query = it->second;

        // Simple parameter substitution (not as robust as real prepared statements)
        for (size_t i = 0; i < params.size(); ++i) {
            std::string placeholder = "$" + std::to_string(i + 1);
            size_t pos = query.find(placeholder);
            if (pos != std::string::npos) {
                query.replace(pos, placeholder.length(), escapeString(params[i]));
            }
        }

        return executeQuery(query);
    }

private:
    DatabaseConfig config_;
    bool connected_;
    std::unordered_map<std::string, std::string> prepared_statements_;

    std::string makeHttpRequest(const std::string& url, const std::string& query) {
        CURL* curl = curl_easy_init();
        if (!curl) return "";

        std::string response;
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, query.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);

        struct curl_slist* headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: text/plain");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        CURLcode res = curl_easy_perform(curl);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);

        if (res != CURLE_OK) {
            HFX_LOG_ERROR("[ERROR] Message");
            return "";
        }

        return response;
    }

    static size_t writeCallback(void* contents, size_t size, size_t nmemb, std::string* s) {
        size_t newLength = size * nmemb;
        s->append((char*)contents, newLength);
        return newLength;
    }
};

// Factory function
std::unique_ptr<DatabaseConnection> DatabaseFactory::createClickHouseConnection() {
    return std::make_unique<ClickHouseConnection>();
}

} // namespace hfx::db
