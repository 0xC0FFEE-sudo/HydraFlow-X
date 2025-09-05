/**
 * @file performance_metrics_repository.cpp
 * @brief Performance metrics repository stub implementation
 */

#include "repositories.hpp"

namespace hfx::db {

// Stub implementation - to be replaced with full implementation
class StubPerformanceMetricsRepository : public PerformanceMetricsRepository {
public:
    bool savePerformanceMetrics(const PerformanceMetrics& metrics) override { return true; }
    std::optional<PerformanceMetrics> getLatestMetrics() override { return std::nullopt; }
    std::vector<PerformanceMetrics> getMetricsHistory(const std::chrono::system_clock::time_point& start,
                                                    const std::chrono::system_clock::time_point& end) override { return {}; }
    double getAverageCpuUsage(const std::chrono::system_clock::time_point& start,
                            const std::chrono::system_clock::time_point& end) override { return 0.0; }
    double getAverageResponseTime(const std::chrono::system_clock::time_point& start,
                                const std::chrono::system_clock::time_point& end) override { return 0.0; }
    uint64_t getErrorCount(const std::chrono::system_clock::time_point& start,
                         const std::chrono::system_clock::time_point& end) override { return 0; }
};

} // namespace hfx::db

std::unique_ptr<hfx::db::PerformanceMetricsRepository> hfx::db::RepositoryFactory::createPerformanceMetricsRepository(
    std::shared_ptr<DatabaseConnection> conn) {
    return std::make_unique<StubPerformanceMetricsRepository>();
}
