/**
 * @file database_manager.cpp
 * @brief Database manager implementation
 */

#include "database_manager.hpp"
#include "database_connection.hpp"
#include <iostream>
#include <sstream>
#include <thread>
#include <chrono>
#include <memory>

namespace hfx::db {

class DatabaseManager::DatabaseManagerImpl {
public:
    DatabaseManagerImpl(const DatabaseManagerConfig& config)
        : config_(config), callback_(nullptr), running_(false) {}

    ~DatabaseManagerImpl() {
        shutdown();
    }

    bool initialize() {
        std::lock_guard<std::mutex> lock(mutex_);

        try {
            // Create PostgreSQL connection
            pg_conn_ = DatabaseFactory::createPostgreSQLConnection();
            if (!pg_conn_->connect(config_.postgresql_config)) {
                HFX_LOG_ERROR("[DatabaseManager] Failed to connect to PostgreSQL");
                return false;
            }

            // Create ClickHouse connection if enabled
            if (config_.enable_clickhouse) {
                ch_conn_ = DatabaseFactory::createClickHouseConnection();
                DatabaseConfig ch_config;
                ch_config.host = config_.clickhouse_config.host;
                ch_config.port = config_.clickhouse_config.port;
                ch_config.database = config_.clickhouse_config.database;
                ch_config.username = config_.clickhouse_config.username;
                ch_config.password = config_.clickhouse_config.password;

                if (!ch_conn_->connect(ch_config)) {
                    HFX_LOG_ERROR("[DatabaseManager] Failed to connect to ClickHouse");
                    // Don't fail if ClickHouse is optional
                }
            }

            // Initialize repositories
            initRepositories();

            // Start health check thread
            running_ = true;
            health_check_thread_ = std::thread(&DatabaseManagerImpl::healthCheckLoop, this);

            return true;
        } catch (const std::exception& e) {
            HFX_LOG_ERROR("[DatabaseManager] Initialization failed: " << e.what() << std::endl;
            return false;
        }
    }

    void shutdown() {
        std::lock_guard<std::mutex> lock(mutex_);

        running_ = false;
        if (health_check_thread_.joinable()) {
            health_check_thread_.join();
        }

        // Shutdown repositories
        trade_repo_.reset();
        position_repo_.reset();
        market_data_repo_.reset();
        liquidity_pool_repo_.reset();
        analytics_repo_.reset();
        risk_metrics_repo_.reset();
        performance_metrics_repo_.reset();

        // Close connections
        if (pg_conn_) pg_conn_->disconnect();
        if (ch_conn_) ch_conn_->disconnect();
    }

    bool isHealthy() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return pg_conn_ && pg_conn_->isConnected();
    }

    HealthCheckResult getHealthStatus() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return last_health_check_;
    }

    // Repository access
    std::shared_ptr<TradeRepository> getTradeRepository() { return trade_repo_; }
    std::shared_ptr<PositionRepository> getPositionRepository() { return position_repo_; }
    std::shared_ptr<MarketDataRepository> getMarketDataRepository() { return market_data_repo_; }
    std::shared_ptr<LiquidityPoolRepository> getLiquidityPoolRepository() { return liquidity_pool_repo_; }
    std::shared_ptr<AnalyticsRepository> getAnalyticsRepository() { return analytics_repo_; }
    std::shared_ptr<RiskMetricsRepository> getRiskMetricsRepository() { return risk_metrics_repo_; }
    std::shared_ptr<PerformanceMetricsRepository> getPerformanceMetricsRepository() { return performance_metrics_repo_; }

    // Convenience methods
    bool saveTrade(const Trade& trade) {
        if (!trade_repo_) return false;
        return trade_repo_->saveTrade(trade);
    }

    bool savePosition(const Position& position) {
        if (!position_repo_) return false;
        return position_repo_->savePosition(position);
    }

    bool saveMarketData(const MarketData& data) {
        if (!market_data_repo_) return false;
        return market_data_repo_->saveMarketData(data);
    }

    bool saveAnalyticsData(const AnalyticsData& data) {
        if (!analytics_repo_) return false;
        return analytics_repo_->saveAnalyticsData(data);
    }

    bool savePerformanceMetrics(const PerformanceMetrics& metrics) {
        if (!performance_metrics_repo_) return false;
        return performance_metrics_repo_->savePerformanceMetrics(metrics);
    }

    std::optional<Trade> getTradeById(const std::string& trade_id) {
        if (!trade_repo_) return std::nullopt;
        return trade_repo_->getTradeById(trade_id);
    }

    std::vector<Trade> getRecentTrades(size_t limit) {
        if (!trade_repo_) return {};
        return trade_repo_->getTradesByWallet("", limit, 0); // Empty wallet gets all
    }

    std::optional<double> getTokenPrice(const std::string& token_address) {
        if (!market_data_repo_) return std::nullopt;
        return market_data_repo_->getTokenPrice(token_address);
    }

    std::vector<std::pair<std::string, double>> getAllTokenPrices() {
        if (!market_data_repo_) return {};
        return market_data_repo_->getAllTokenPrices();
    }

    double getTotalPortfolioValue(const std::string& wallet_address) {
        if (!position_repo_) return 0.0;
        return position_repo_->getTotalPortfolioValue(wallet_address);
    }

    double getTotalPnL24h() {
        if (!trade_repo_) return 0.0;
        return trade_repo_->getTotalPnL24h();
    }

    void setOperationCallback(DatabaseCallback callback) {
        std::lock_guard<std::mutex> lock(mutex_);
        callback_ = callback;
    }

    DatabaseStats getStatistics() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return stats_;
    }

private:
    DatabaseManagerConfig config_;
    std::shared_ptr<DatabaseConnection> pg_conn_;
    std::shared_ptr<DatabaseConnection> ch_conn_;

    // Repositories
    std::shared_ptr<TradeRepository> trade_repo_;
    std::shared_ptr<PositionRepository> position_repo_;
    std::shared_ptr<MarketDataRepository> market_data_repo_;
    std::shared_ptr<LiquidityPoolRepository> liquidity_pool_repo_;
    std::shared_ptr<AnalyticsRepository> analytics_repo_;
    std::shared_ptr<RiskMetricsRepository> risk_metrics_repo_;
    std::shared_ptr<PerformanceMetricsRepository> performance_metrics_repo_;

    // Health monitoring
    std::thread health_check_thread_;
    mutable std::mutex mutex_;
    DatabaseCallback callback_;
    HealthCheckResult last_health_check_;
    DatabaseStats stats_;
    bool running_;

    void initRepositories() {
        // Create repository implementations with database connections
        trade_repo_ = RepositoryFactory::createTradeRepository(pg_conn_);
        position_repo_ = RepositoryFactory::createPositionRepository(pg_conn_);
        market_data_repo_ = RepositoryFactory::createMarketDataRepository(pg_conn_);
        liquidity_pool_repo_ = RepositoryFactory::createLiquidityPoolRepository(pg_conn_);

        // Use ClickHouse for analytics if available, otherwise PostgreSQL
        auto analytics_conn = ch_conn_ ? ch_conn_ : pg_conn_;
        analytics_repo_ = RepositoryFactory::createAnalyticsRepository(analytics_conn);

        risk_metrics_repo_ = RepositoryFactory::createRiskMetricsRepository(pg_conn_);
        performance_metrics_repo_ = RepositoryFactory::createPerformanceMetricsRepository(pg_conn_);
    }

    void healthCheckLoop() {
        while (running_) {
            auto start_time = std::chrono::steady_clock::now();

            HealthCheckResult result;
            result.isHealthy = true;
            result.lastCheck = std::chrono::system_clock::now();

            // Check PostgreSQL
            if (pg_conn_ && pg_conn_->isConnected()) {
                result.isHealthy &= true;
                stats_.postgresql_healthy = true;
            } else {
                result.isHealthy = false;
                result.errorMessage = "PostgreSQL connection failed";
                stats_.postgresql_healthy = false;
            }

            // Check ClickHouse (if enabled)
            if (config_.enable_clickhouse && ch_conn_) {
                if (ch_conn_->isConnected()) {
                    stats_.clickhouse_healthy = true;
                } else {
                    stats_.clickhouse_healthy = false;
                    if (result.isHealthy) {
                        result.errorMessage += "; ";
                    }
                    result.errorMessage += "ClickHouse connection failed";
                }
            }

            auto end_time = std::chrono::steady_clock::now();
            result.responseTime = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

            {
                std::lock_guard<std::mutex> lock(mutex_);
                last_health_check_ = result;
            }

            std::this_thread::sleep_for(config_.health_check_interval);
        }
    }
};

// DatabaseManager implementation
DatabaseManager::DatabaseManager(const DatabaseManagerConfig& config)
    : pimpl_(std::make_unique<DatabaseManagerImpl>(config)) {}

DatabaseManager::~DatabaseManager() = default;

bool DatabaseManager::initialize() {
    return pimpl_->initialize();
}

void DatabaseManager::shutdown() {
    pimpl_->shutdown();
}

bool DatabaseManager::isHealthy() const {
    return pimpl_->isHealthy();
}

HealthCheckResult DatabaseManager::getHealthStatus() const {
    return pimpl_->getHealthStatus();
}

std::shared_ptr<TradeRepository> DatabaseManager::getTradeRepository() {
    return pimpl_->getTradeRepository();
}

std::shared_ptr<PositionRepository> DatabaseManager::getPositionRepository() {
    return pimpl_->getPositionRepository();
}

std::shared_ptr<MarketDataRepository> DatabaseManager::getMarketDataRepository() {
    return pimpl_->getMarketDataRepository();
}

std::shared_ptr<LiquidityPoolRepository> DatabaseManager::getLiquidityPoolRepository() {
    return pimpl_->getLiquidityPoolRepository();
}

std::shared_ptr<AnalyticsRepository> DatabaseManager::getAnalyticsRepository() {
    return pimpl_->getAnalyticsRepository();
}

std::shared_ptr<RiskMetricsRepository> DatabaseManager::getRiskMetricsRepository() {
    return pimpl_->getRiskMetricsRepository();
}

std::shared_ptr<PerformanceMetricsRepository> DatabaseManager::getPerformanceMetricsRepository() {
    return pimpl_->getPerformanceMetricsRepository();
}

bool DatabaseManager::saveTrade(const Trade& trade) {
    return pimpl_->saveTrade(trade);
}

bool DatabaseManager::savePosition(const Position& position) {
    return pimpl_->savePosition(position);
}

bool DatabaseManager::saveMarketData(const MarketData& data) {
    return pimpl_->saveMarketData(data);
}

bool DatabaseManager::saveAnalyticsData(const AnalyticsData& data) {
    return pimpl_->saveAnalyticsData(data);
}

bool DatabaseManager::savePerformanceMetrics(const PerformanceMetrics& metrics) {
    return pimpl_->savePerformanceMetrics(metrics);
}

std::optional<Trade> DatabaseManager::getTradeById(const std::string& trade_id) {
    return pimpl_->getTradeById(trade_id);
}

std::vector<Trade> DatabaseManager::getRecentTrades(size_t limit) {
    return pimpl_->getRecentTrades(limit);
}

std::optional<double> DatabaseManager::getTokenPrice(const std::string& token_address) {
    return pimpl_->getTokenPrice(token_address);
}

std::vector<std::pair<std::string, double>> DatabaseManager::getAllTokenPrices() {
    return pimpl_->getAllTokenPrices();
}

double DatabaseManager::getTotalPortfolioValue(const std::string& wallet_address) {
    return pimpl_->getTotalPortfolioValue(wallet_address);
}

double DatabaseManager::getTotalPnL24h() {
    return pimpl_->getTotalPnL24h();
}

void DatabaseManager::setOperationCallback(DatabaseCallback callback) {
    pimpl_->setOperationCallback(callback);
}

DatabaseManager::DatabaseStats DatabaseManager::getStatistics() const {
    return pimpl_->getStatistics();
}

} // namespace hfx::db
