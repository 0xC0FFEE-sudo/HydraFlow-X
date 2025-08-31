/**
 * @file chain_manager.hpp
 * @brief Multi-chain connection manager for Ethereum L1/L2 networks
 * @version 1.0.0
 */

#pragma once

#include <atomic>
#include <memory>
#include <string>
#include <unordered_map>

namespace hfx::chain {

class ChainManager {
public:
    ChainManager() = default;
    ~ChainManager() = default;

    bool initialize();
    void shutdown();
    bool is_running() const { return running_.load(); }

private:
    std::atomic<bool> running_{false};
};

} // namespace hfx::chain
