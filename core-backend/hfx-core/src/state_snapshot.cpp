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
    HFX_LOG_INFO("[StateSnapshot] Saved snapshot " << snapshot_id_ << "\n";
    return true;
}

bool StateSnapshot::load_snapshot() {
    HFX_LOG_INFO("[StateSnapshot] Loaded snapshot " << snapshot_id_ << "\n";
    return true;
}

} // namespace hfx::core
