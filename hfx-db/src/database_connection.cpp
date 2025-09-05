/**
 * @file database_connection.cpp
 * @brief Database connection factory and base implementations
 */

#include "database_connection.hpp"
#include "repositories.hpp"
#include <iostream>

namespace hfx::db {

// Simple result implementation for basic functionality
class SimpleDatabaseResult : public DatabaseResult {
public:
    SimpleDatabaseResult() = default;
    ~SimpleDatabaseResult() = default;

    bool isValid() const override { return true; }
    size_t rowCount() const override { return 0; }
    size_t columnCount() const override { return 0; }
    std::vector<std::string> columnNames() const override { return {}; }

    std::optional<std::string> getString(size_t row, size_t col) const override { return std::nullopt; }
    std::optional<int64_t> getInt64(size_t row, size_t col) const override { return std::nullopt; }
    std::optional<double> getDouble(size_t row, size_t col) const override { return std::nullopt; }
    std::optional<bool> getBool(size_t row, size_t col) const override { return std::nullopt; }
};

// Factory function implementations are in individual repository files

} // namespace hfx::db
