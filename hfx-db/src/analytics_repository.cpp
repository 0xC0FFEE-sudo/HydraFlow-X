/**
 * @file analytics_repository.cpp
 * @brief Analytics repository stub implementation
 */

#include "repositories.hpp"

namespace hfx::db {

// Stub implementation - to be replaced with full implementation
class StubAnalyticsRepository : public AnalyticsRepository {
public:
    bool saveAnalyticsData(const AnalyticsData& data) override { return true; }
    std::vector<AnalyticsData> getAnalyticsData(const std::string& metricName,
                                              const std::chrono::system_clock::time_point& start,
                                              const std::chrono::system_clock::time_point& end,
                                              const std::string& timeBucket) override { return {}; }
    std::vector<std::pair<std::string, double>> getMetricSums(const std::string& metricName,
                                                            const std::chrono::system_clock::time_point& start,
                                                            const std::chrono::system_clock::time_point& end) override { return {}; }
    std::vector<std::pair<std::string, double>> getMetricAverages(const std::string& metricName,
                                                                const std::chrono::system_clock::time_point& start,
                                                                const std::chrono::system_clock::time_point& end) override { return {}; }
    double getMetricTotal(const std::string& metricName,
                        const std::chrono::system_clock::time_point& start,
                        const std::chrono::system_clock::time_point& end) override { return 0.0; }
};

} // namespace hfx::db

std::unique_ptr<hfx::db::AnalyticsRepository> hfx::db::RepositoryFactory::createAnalyticsRepository(
    std::shared_ptr<DatabaseConnection> conn) {
    return std::make_unique<StubAnalyticsRepository>();
}
