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
#include <cstdint>

namespace hfx::chain {

class ChainManager {
public:
    ChainManager();
    ~ChainManager();

    bool initialize();
    void shutdown();
    bool is_running() const { return running_.load(); }

    // Connection status
    bool is_ethereum_connected() const;
    bool is_solana_connected() const;

    // Real-time data access
    uint64_t get_ethereum_block_number() const;
    uint64_t get_solana_slot_number() const;
    double get_ethereum_gas_price() const;
    uint32_t get_ethereum_pending_transactions() const;

    // Status JSON
    std::string get_chain_status() const;

private:
    class ChainManagerImpl;
    std::unique_ptr<ChainManagerImpl> pimpl_;
    std::atomic<bool> running_{false};
};

} // namespace hfx::chain
