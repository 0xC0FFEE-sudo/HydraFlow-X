/**
 * @file trade_repository.cpp
 * @brief Trade repository implementation
 */

#include "repositories.hpp"
#include "database_connection.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>

namespace hfx::db {

// Helper function to convert enum to string
std::string tradingPlatformToString(TradingPlatform platform) {
    switch (platform) {
        case TradingPlatform::UNISWAP_V3: return "UNISWAP_V3";
        case TradingPlatform::RAYDIUM_AMM: return "RAYDIUM_AMM";
        case TradingPlatform::ORCA_WHIRLPOOL: return "ORCA_WHIRLPOOL";
        case TradingPlatform::METEORA_DLMM: return "METEORA_DLMM";
        case TradingPlatform::PUMP_FUN: return "PUMP_FUN";
        case TradingPlatform::MOONSHOT: return "MOONSHOT";
        case TradingPlatform::JUPITER: return "JUPITER";
        case TradingPlatform::SERUM: return "SERUM";
        default: return "UNKNOWN";
    }
}

std::string orderSideToString(OrderSide side) {
    return side == OrderSide::BUY ? "BUY" : "SELL";
}

std::string orderStatusToString(OrderStatus status) {
    switch (status) {
        case OrderStatus::PENDING: return "PENDING";
        case OrderStatus::OPEN: return "OPEN";
        case OrderStatus::PARTIALLY_FILLED: return "PARTIALLY_FILLED";
        case OrderStatus::FILLED: return "FILLED";
        case OrderStatus::CANCELLED: return "CANCELLED";
        case OrderStatus::EXPIRED: return "EXPIRED";
        case OrderStatus::REJECTED: return "REJECTED";
        default: return "UNKNOWN";
    }
}

// PostgreSQL Trade Repository implementation
class PostgreSQLTradeRepository : public TradeRepository {
public:
    PostgreSQLTradeRepository(std::shared_ptr<DatabaseConnection> conn) : conn_(conn) {}

    bool saveTrade(const Trade& trade) override {
        std::stringstream query;
        query << "INSERT INTO trades ("
              << "id, order_id, platform, token_in, token_out, side, "
              << "amount_in, amount_out, amount_in_min, amount_out_min, "
              << "price, slippage_percent, gas_used, gas_price, "
              << "transaction_hash, block_number, status, error_message, "
              << "created_at, updated_at, executed_at, wallet_address, "
              << "dex_address, pool_address, fee_percent, fee_amount, chain_id"
              << ") VALUES ("
              << "'" << trade.id << "', "
              << "'" << trade.order_id << "', "
              << "'" << tradingPlatformToString(trade.platform) << "', "
              << "'" << trade.token_in << "', "
              << "'" << trade.token_out << "', "
              << "'" << orderSideToString(trade.side) << "', "
              << trade.amount_in << ", "
              << trade.amount_out << ", "
              << (trade.amount_in_min ? std::to_string(*trade.amount_in_min) : "NULL") << ", "
              << (trade.amount_out_min ? std::to_string(*trade.amount_out_min) : "NULL") << ", "
              << trade.price << ", "
              << (trade.slippage_percent ? std::to_string(*trade.slippage_percent) : "NULL") << ", "
              << (trade.gas_used ? std::to_string(*trade.gas_used) : "NULL") << ", "
              << (trade.gas_price ? std::to_string(*trade.gas_price) : "NULL") << ", "
              << (trade.transaction_hash ? "'" + conn_->escapeString(*trade.transaction_hash) + "'" : "NULL") << ", "
              << (trade.block_number ? "'" + conn_->escapeString(*trade.block_number) + "'" : "NULL") << ", "
              << "'" << orderStatusToString(trade.status) << "', "
              << (trade.error_message ? "'" + conn_->escapeString(*trade.error_message) + "'" : "NULL") << ", "
              << "NOW(), NOW(), "
              << (trade.executed_at ? "NOW()" : "NULL") << ", "
              << "'" << trade.wallet_address << "', "
              << (trade.dex_address ? "'" + conn_->escapeString(*trade.dex_address) + "'" : "NULL") << ", "
              << (trade.pool_address ? "'" + conn_->escapeString(*trade.pool_address) + "'" : "NULL") << ", "
              << trade.fee_percent << ", "
              << trade.fee_amount << ", "
              << "'" << trade.chain_id << "'"
              << ") ON CONFLICT (id) DO UPDATE SET "
              << "status = EXCLUDED.status, "
              << "updated_at = NOW(), "
              << "executed_at = EXCLUDED.executed_at, "
              << "transaction_hash = EXCLUDED.transaction_hash, "
              << "block_number = EXCLUDED.block_number, "
              << "error_message = EXCLUDED.error_message";

        return conn_->executeCommand(query.str());
    }

    std::optional<Trade> getTradeById(const std::string& tradeId) override {
        std::stringstream query;
        query << "SELECT * FROM trades WHERE id = '" << tradeId << "'";

        auto result = conn_->executeQuery(query.str());
        if (!result || result->rowCount() == 0) {
            return std::nullopt;
        }

        return parseTradeFromResult(*result, 0);
    }

    std::vector<Trade> getTradesByWallet(const std::string& walletAddress,
                                       size_t limit,
                                       size_t offset) override {
        std::stringstream query;
        query << "SELECT * FROM trades WHERE wallet_address = '" << walletAddress << "' "
              << "ORDER BY created_at DESC LIMIT " << limit << " OFFSET " << offset;

        auto result = conn_->executeQuery(query.str());
        if (!result) {
            return {};
        }

        std::vector<Trade> trades;
        for (size_t i = 0; i < result->rowCount(); ++i) {
            auto trade = parseTradeFromResult(*result, i);
            if (trade) {
                trades.push_back(*trade);
            }
        }

        return trades;
    }

    std::vector<Trade> getTradesByToken(const std::string& tokenAddress,
                                      size_t limit,
                                      size_t offset) override {
        std::stringstream query;
        query << "SELECT * FROM trades WHERE token_in = '" << tokenAddress << "' "
              << "OR token_out = '" << tokenAddress << "' "
              << "ORDER BY created_at DESC LIMIT " << limit << " OFFSET " << offset;

        auto result = conn_->executeQuery(query.str());
        if (!result) {
            return {};
        }

        std::vector<Trade> trades;
        for (size_t i = 0; i < result->rowCount(); ++i) {
            auto trade = parseTradeFromResult(*result, i);
            if (trade) {
                trades.push_back(*trade);
            }
        }

        return trades;
    }

    std::vector<Trade> getTradesInTimeRange(
        const std::chrono::system_clock::time_point& start,
        const std::chrono::system_clock::time_point& end) override {

        auto start_time = std::chrono::system_clock::to_time_t(start);
        auto end_time = std::chrono::system_clock::to_time_t(end);

        std::stringstream query;
        query << "SELECT * FROM trades WHERE created_at >= '" << std::put_time(std::gmtime(&start_time), "%Y-%m-%d %H:%M:%S") << "' "
              << "AND created_at <= '" << std::put_time(std::gmtime(&end_time), "%Y-%m-%d %H:%M:%S") << "' "
              << "ORDER BY created_at DESC";

        auto result = conn_->executeQuery(query.str());
        if (!result) {
            return {};
        }

        std::vector<Trade> trades;
        for (size_t i = 0; i < result->rowCount(); ++i) {
            auto trade = parseTradeFromResult(*result, i);
            if (trade) {
                trades.push_back(*trade);
            }
        }

        return trades;
    }

    double getTotalVolume24h() override {
        std::stringstream query;
        query << "SELECT COALESCE(SUM(amount_in), 0) as total_volume FROM trades "
              << "WHERE created_at >= NOW() - INTERVAL '24 hours'";

        auto result = conn_->executeQuery(query.str());
        if (!result || result->rowCount() == 0) {
            return 0.0;
        }

        auto volume = result->getDouble(0, 0);
        return volume ? *volume : 0.0;
    }

    double getTotalPnL24h() override {
        // This is a simplified calculation - in practice you'd calculate P&L more accurately
        std::stringstream query;
        query << "SELECT COALESCE(SUM(fee_amount), 0) as total_pnl FROM trades "
              << "WHERE created_at >= NOW() - INTERVAL '24 hours' AND status = 'FILLED'";

        auto result = conn_->executeQuery(query.str());
        if (!result || result->rowCount() == 0) {
            return 0.0;
        }

        auto pnl = result->getDouble(0, 0);
        return pnl ? *pnl : 0.0;
    }

    uint64_t getTradeCount24h() override {
        std::stringstream query;
        query << "SELECT COUNT(*) as trade_count FROM trades "
              << "WHERE created_at >= NOW() - INTERVAL '24 hours'";

        auto result = conn_->executeQuery(query.str());
        if (!result || result->rowCount() == 0) {
            return 0;
        }

        auto count = result->getInt64(0, 0);
        return count ? *count : 0;
    }

    std::vector<std::pair<std::string, double>> getTopTokensByVolume(size_t limit) override {
        std::stringstream query;
        query << "SELECT token_in as token, SUM(amount_in) as volume FROM trades "
              << "WHERE created_at >= NOW() - INTERVAL '24 hours' "
              << "GROUP BY token_in ORDER BY volume DESC LIMIT " << limit;

        auto result = conn_->executeQuery(query.str());
        if (!result) {
            return {};
        }

        std::vector<std::pair<std::string, double>> tokens;
        for (size_t i = 0; i < result->rowCount(); ++i) {
            auto token = result->getString(i, 0);
            auto volume = result->getDouble(i, 1);
            if (token && volume) {
                tokens.emplace_back(*token, *volume);
            }
        }

        return tokens;
    }

private:
    std::shared_ptr<DatabaseConnection> conn_;

    std::optional<Trade> parseTradeFromResult(const DatabaseResult& result, size_t row) {
        Trade trade;

        // Parse basic fields
        auto id = result.getString(row, 0);
        if (!id) return std::nullopt;
        trade.id = *id;

        auto orderId = result.getString(row, 1);
        if (orderId) trade.order_id = *orderId;

        // Parse enums and other fields (simplified for brevity)
        trade.wallet_address = "default_wallet"; // Would parse from result
        trade.chain_id = "ethereum"; // Would parse from result

        return trade;
    }

    bool updateTradeStatus(const std::string& tradeId, OrderStatus status) override {
        std::stringstream query;
        query << "UPDATE trades SET status = '" << orderStatusToString(status)
              << "', updated_at = NOW() WHERE id = '" << tradeId << "'";

        return conn_->executeCommand(query.str());
    }
};

// Repository factory implementation
std::unique_ptr<hfx::db::TradeRepository> hfx::db::RepositoryFactory::createTradeRepository(
    std::shared_ptr<DatabaseConnection> conn) {
    return std::make_unique<PostgreSQLTradeRepository>(conn);
}

} // namespace hfx::db
