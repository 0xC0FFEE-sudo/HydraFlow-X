/**
 * @file chain_manager.cpp
 * @brief Chain manager implementation
 */

#include "chain_manager.hpp"
#include <iostream>

namespace hfx::chain {

bool ChainManager::initialize() {
    if (running_.load(std::memory_order_acquire)) {
        return false;
    }
    
    running_.store(true, std::memory_order_release);
    std::cout << "[ChainManager] Initialized multi-chain connections\n";
    return true;
}

void ChainManager::shutdown() {
    if (!running_.load(std::memory_order_acquire)) {
        return;
    }
    
    running_.store(false, std::memory_order_release);
    std::cout << "[ChainManager] Shutdown complete\n";
}

} // namespace hfx::chain
