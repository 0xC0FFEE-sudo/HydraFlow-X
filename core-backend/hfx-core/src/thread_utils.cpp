/**
 * @file thread_utils.cpp
 * @brief Thread utilities implementation
 */

#include "thread_utils.hpp"
#include "../../hfx-log/include/simple_logger.hpp"
#include <thread>
#include <iostream>

#ifdef __APPLE__
#include <pthread.h>
#include <sys/sysctl.h>
#endif

namespace hfx::core {

bool ThreadUtils::pin_to_cpu(std::uint32_t cpu_id) {
#ifdef __APPLE__
    // macOS doesn't support CPU affinity, but we can set thread QoS
    pthread_set_qos_class_self_np(QOS_CLASS_USER_INTERACTIVE, 0);
    HFX_LOG_INFO("[ThreadUtils] Set high QoS class (CPU affinity not supported on macOS)\n");
    return true;
#else
    // Linux implementation would go here
    return false;
#endif
}

bool ThreadUtils::set_high_priority() {
#ifdef __APPLE__
    pthread_set_qos_class_self_np(QOS_CLASS_USER_INTERACTIVE, 0);
    return true;
#else
    return false;
#endif
}

std::uint32_t ThreadUtils::get_cpu_count() {
    return std::thread::hardware_concurrency();
}

void ThreadUtils::yield_cpu() {
    std::this_thread::yield();
}

} // namespace hfx::core
