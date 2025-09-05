/**
 * @file market_data_repository.cpp
 * @brief Market data repository stub implementation
 */

#include "repositories.hpp"

namespace hfx::db {

// Stub implementation - to be replaced with full implementation
class StubMarketDataRepository : public MarketDataRepository {
public:
    bool saveMarketData(const MarketData& data) override { return true; }
    std::optional<MarketData> getLatestMarketData(const std::string& tokenAddress) override { return std::nullopt; }
    std::vector<MarketData> getMarketDataHistory(const std::string& tokenAddress,
                                               const std::chrono::system_clock::time_point& start,
                                               const std::chrono::system_clock::time_point& end) override { return {}; }
    std::vector<MarketData> getTopTokensByMarketCap(size_t limit) override { return {}; }
    std::optional<double> getTokenPrice(const std::string& tokenAddress) override { return std::nullopt; }
    std::vector<std::pair<std::string, double>> getAllTokenPrices() override { return {}; }
};

} // namespace hfx::db

std::unique_ptr<hfx::db::MarketDataRepository> hfx::db::RepositoryFactory::createMarketDataRepository(
    std::shared_ptr<DatabaseConnection> conn) {
    return std::make_unique<StubMarketDataRepository>();
}
