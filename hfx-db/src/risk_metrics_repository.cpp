/**
 * @file risk_metrics_repository.cpp
 * @brief Risk metrics repository stub implementation
 */

#include "repositories.hpp"

namespace hfx::db {

// Stub implementation - to be replaced with full implementation
class StubRiskMetricsRepository : public RiskMetricsRepository {
public:
    bool saveRiskMetrics(const RiskMetrics& metrics) override { return true; }
    std::optional<RiskMetrics> getLatestRiskMetrics(const std::string& walletAddress) override { return std::nullopt; }
    std::vector<RiskMetrics> getRiskMetricsHistory(const std::string& walletAddress,
                                                 const std::chrono::system_clock::time_point& start,
                                                 const std::chrono::system_clock::time_point& end) override { return {}; }
    std::vector<RiskMetrics> getWalletsWithBreachedLimits() override { return {}; }
    bool updateRiskLimits(const std::string& walletAddress,
                        double maxPositionSizePercent,
                        double maxDailyLossPercent,
                        double maxDrawdownLimitPercent) override { return true; }
};

} // namespace hfx::db

std::unique_ptr<hfx::db::RiskMetricsRepository> hfx::db::RepositoryFactory::createRiskMetricsRepository(
    std::shared_ptr<DatabaseConnection> conn) {
    return std::make_unique<StubRiskMetricsRepository>();
}
