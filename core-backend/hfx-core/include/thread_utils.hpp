/**
 * @file thread_utils.hpp
 * @brief Thread utilities for high-performance computing
 * @version 1.0.0
 */

#pragma once

#include <cstdint>

namespace hfx::core {

class ThreadUtils {
public:
    static bool pin_to_cpu(std::uint32_t cpu_id);
    static bool set_high_priority();
    static std::uint32_t get_cpu_count();
    static void yield_cpu();
};

} // namespace hfx::core
