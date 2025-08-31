/**
 * @file state_snapshot.cpp
 * @brief State snapshot implementation
 */

#include "state_snapshot.hpp"
#include <iostream>

namespace hfx::core {

bool StateSnapshot::save_snapshot() {
    timestamp_ = std::chrono::high_resolution_clock::now();
    snapshot_id_++;
    std::cout << "[StateSnapshot] Saved snapshot " << snapshot_id_ << "\n";
    return true;
}

bool StateSnapshot::load_snapshot() {
    std::cout << "[StateSnapshot] Loaded snapshot " << snapshot_id_ << "\n";
    return true;
}

} // namespace hfx::core
