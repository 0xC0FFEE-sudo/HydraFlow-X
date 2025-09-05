/**
 * @file repositories.hpp
 * @brief Repository interfaces for database operations
 */

#pragma once

#include "data_models.hpp"
#include <memory>
#include <vector>
#include <optional>

// Forward declaration
namespace hfx::db {
class DatabaseConnection;
}

namespace hfx::db {

/**
 * @brief Trade repository interface
 */
class TradeRepository {
public:
    virtual ~TradeRepository() = default;

    virtual bool saveTrade(const Trade& trade) = 0;
    virtual bool updateTradeStatus(const std::string& tradeId, OrderStatus status) = 0;
    virtual std::optional<Trade> getTradeById(const std::string& tradeId) = 0;
    virtual std::vector<Trade> getTradesByWallet(const std::string& walletAddress,
                                               size_t limit = 100,
                                               size_t offset = 0) = 0;
    virtual std::vector<Trade> getTradesByToken(const std::string& tokenAddress,
                                              size_t limit = 100,
                                              size_t offset = 0) = 0;
    virtual std::vector<Trade> getTradesInTimeRange(
        const std::chrono::system_clock::time_point& start,
        const std::chrono::system_clock::time_point& end) = 0;

    // Analytics queries
    virtual double getTotalVolume24h() = 0;
    virtual double getTotalPnL24h() = 0;
    virtual uint64_t getTradeCount24h() = 0;
    virtual std::vector<std::pair<std::string, double>> getTopTokensByVolume(size_t limit = 10) = 0;
};

/**
 * @brief Position repository interface
 */
class PositionRepository {
public:
    virtual ~PositionRepository() = default;

    virtual bool savePosition(const Position& position) = 0;
    virtual bool updatePosition(const Position& position) = 0;
    virtual std::optional<Position> getPositionById(const std::string& positionId) = 0;
    virtual std::vector<Position> getPositionsByWallet(const std::string& walletAddress) = 0;
    virtual std::vector<Position> getPositionsByToken(const std::string& tokenAddress) = 0;
    virtual bool deletePosition(const std::string& positionId) = 0;

    // Analytics queries
    virtual double getTotalPortfolioValue(const std::string& walletAddress) = 0;
    virtual double getTotalPnL(const std::string& walletAddress) = 0;
    virtual std::vector<Position> getTopPositionsByValue(size_t limit = 10) = 0;
};

/**
 * @brief Market data repository interface
 */
class MarketDataRepository {
public:
    virtual ~MarketDataRepository() = default;

    virtual bool saveMarketData(const MarketData& data) = 0;
    virtual std::optional<MarketData> getLatestMarketData(const std::string& tokenAddress) = 0;
    virtual std::vector<MarketData> getMarketDataHistory(
        const std::string& tokenAddress,
        const std::chrono::system_clock::time_point& start,
        const std::chrono::system_clock::time_point& end) = 0;
    virtual std::vector<MarketData> getTopTokensByMarketCap(size_t limit = 100) = 0;

    // Price queries
    virtual std::optional<double> getTokenPrice(const std::string& tokenAddress) = 0;
    virtual std::vector<std::pair<std::string, double>> getAllTokenPrices() = 0;
};

/**
 * @brief Liquidity pool repository interface
 */
class LiquidityPoolRepository {
public:
    virtual ~LiquidityPoolRepository() = default;

    virtual bool savePool(const LiquidityPool& pool) = 0;
    virtual bool updatePool(const LiquidityPool& pool) = 0;
    virtual std::optional<LiquidityPool> getPoolByAddress(const std::string& poolAddress) = 0;
    virtual std::vector<LiquidityPool> getPoolsByPlatform(TradingPlatform platform) = 0;
    virtual std::vector<LiquidityPool> getTopPoolsByLiquidity(size_t limit = 100) = 0;
    virtual std::vector<LiquidityPool> getPoolsByTokenPair(
        const std::string& token0,
        const std::string& token1) = 0;
};

/**
 * @brief Analytics repository interface for time-series data
 */
class AnalyticsRepository {
public:
    virtual ~AnalyticsRepository() = default;

    virtual bool saveAnalyticsData(const AnalyticsData& data) = 0;
    virtual std::vector<AnalyticsData> getAnalyticsData(
        const std::string& metricName,
        const std::chrono::system_clock::time_point& start,
        const std::chrono::system_clock::time_point& end,
        const std::string& timeBucket = "1h") = 0;

    // Aggregation queries
    virtual std::vector<std::pair<std::string, double>> getMetricSums(
        const std::string& metricName,
        const std::chrono::system_clock::time_point& start,
        const std::chrono::system_clock::time_point& end) = 0;

    virtual std::vector<std::pair<std::string, double>> getMetricAverages(
        const std::string& metricName,
        const std::chrono::system_clock::time_point& start,
        const std::chrono::system_clock::time_point& end) = 0;

    virtual double getMetricTotal(const std::string& metricName,
                                const std::chrono::system_clock::time_point& start,
                                const std::chrono::system_clock::time_point& end) = 0;
};

/**
 * @brief Risk metrics repository interface
 */
class RiskMetricsRepository {
public:
    virtual ~RiskMetricsRepository() = default;

    virtual bool saveRiskMetrics(const RiskMetrics& metrics) = 0;
    virtual std::optional<RiskMetrics> getLatestRiskMetrics(const std::string& walletAddress) = 0;
    virtual std::vector<RiskMetrics> getRiskMetricsHistory(
        const std::string& walletAddress,
        const std::chrono::system_clock::time_point& start,
        const std::chrono::system_clock::time_point& end) = 0;

    // Risk alerts
    virtual std::vector<RiskMetrics> getWalletsWithBreachedLimits() = 0;
    virtual bool updateRiskLimits(const std::string& walletAddress,
                                double maxPositionSizePercent,
                                double maxDailyLossPercent,
                                double maxDrawdownLimitPercent) = 0;
};

/**
 * @brief Performance metrics repository interface
 */
class PerformanceMetricsRepository {
public:
    virtual ~PerformanceMetricsRepository() = default;

    virtual bool savePerformanceMetrics(const PerformanceMetrics& metrics) = 0;
    virtual std::optional<PerformanceMetrics> getLatestMetrics() = 0;
    virtual std::vector<PerformanceMetrics> getMetricsHistory(
        const std::chrono::system_clock::time_point& start,
        const std::chrono::system_clock::time_point& end) = 0;

    // System health queries
    virtual double getAverageCpuUsage(const std::chrono::system_clock::time_point& start,
                                    const std::chrono::system_clock::time_point& end) = 0;
    virtual double getAverageResponseTime(const std::chrono::system_clock::time_point& start,
                                        const std::chrono::system_clock::time_point& end) = 0;
    virtual uint64_t getErrorCount(const std::chrono::system_clock::time_point& start,
                                 const std::chrono::system_clock::time_point& end) = 0;
};

/**
 * @brief Repository factory
 */
class RepositoryFactory {
public:
    static std::unique_ptr<TradeRepository> createTradeRepository(
        std::shared_ptr<DatabaseConnection> conn);
    static std::unique_ptr<PositionRepository> createPositionRepository(
        std::shared_ptr<DatabaseConnection> conn);
    static std::unique_ptr<MarketDataRepository> createMarketDataRepository(
        std::shared_ptr<DatabaseConnection> conn);
    static std::unique_ptr<LiquidityPoolRepository> createLiquidityPoolRepository(
        std::shared_ptr<DatabaseConnection> conn);
    static std::unique_ptr<AnalyticsRepository> createAnalyticsRepository(
        std::shared_ptr<DatabaseConnection> conn);
    static std::unique_ptr<RiskMetricsRepository> createRiskMetricsRepository(
        std::shared_ptr<DatabaseConnection> conn);
    static std::unique_ptr<PerformanceMetricsRepository> createPerformanceMetricsRepository(
        std::shared_ptr<DatabaseConnection> conn);
};

} // namespace hfx::db
