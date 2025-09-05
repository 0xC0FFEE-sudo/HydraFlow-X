/**
 * @file position_repository.cpp
 * @brief Position repository stub implementation
 */

#include "repositories.hpp"

namespace hfx::db {

// Stub implementation - to be replaced with full implementation
class StubPositionRepository : public PositionRepository {
public:
    bool savePosition(const Position& position) override { return true; }
    bool updatePosition(const Position& position) override { return true; }
    std::optional<Position> getPositionById(const std::string& positionId) override { return std::nullopt; }
    std::vector<Position> getPositionsByWallet(const std::string& walletAddress) override { return {}; }
    std::vector<Position> getPositionsByToken(const std::string& tokenAddress) override { return {}; }
    bool deletePosition(const std::string& positionId) override { return true; }
    double getTotalPortfolioValue(const std::string& walletAddress) override { return 0.0; }
    double getTotalPnL(const std::string& walletAddress) override { return 0.0; }
    std::vector<Position> getTopPositionsByValue(size_t limit) override { return {}; }
};

} // namespace hfx::db

std::unique_ptr<hfx::db::PositionRepository> hfx::db::RepositoryFactory::createPositionRepository(
    std::shared_ptr<DatabaseConnection> conn) {
    return std::make_unique<StubPositionRepository>();
}
