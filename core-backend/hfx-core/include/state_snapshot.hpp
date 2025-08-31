/**
 * @file state_snapshot.hpp
 * @brief System state snapshot for recovery and debugging
 * @version 1.0.0
 */

#pragma once

#include <chrono>
#include <cstdint>

namespace hfx::core {

class StateSnapshot {
public:
    StateSnapshot() = default;
    ~StateSnapshot() = default;

    bool save_snapshot();
    bool load_snapshot();

private:
    std::uint64_t snapshot_id_{0};
    std::chrono::time_point<std::chrono::high_resolution_clock> timestamp_;
};

} // namespace hfx::core
