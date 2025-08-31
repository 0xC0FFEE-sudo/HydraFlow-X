/**
 * @file time_wheel.hpp
 * @brief High-precision timing wheel for sub-microsecond scheduling
 * @version 1.0.0
 * 
 * Hierarchical timing wheels optimized for HFT with nanosecond precision.
 * Designed for deterministic execution on Apple Silicon.
 */

#pragma once

#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

#ifdef __APPLE__
#include <mach/mach_time.h>
#endif

namespace hfx::core {

using TimerId = std::uint64_t;
using TimePoint = std::chrono::time_point<std::chrono::high_resolution_clock>;
using Duration = std::chrono::nanoseconds;

/**
 * @struct TimerEvent
 * @brief Lightweight timer event structure
 */
struct alignas(64) TimerEvent {
    TimerId id;
    TimePoint expiry_time;
    Duration interval;  // 0 for one-shot timers
    std::function<void()> callback;
    bool recurring;
    
    TimerEvent() = default;
    TimerEvent(TimerId timer_id, TimePoint expiry, Duration inter, 
               std::function<void()> cb, bool recur = false)
        : id(timer_id), expiry_time(expiry), interval(inter), 
          callback(std::move(cb)), recurring(recur) {}
} __attribute__((packed));

/**
 * @class TimeWheel
 * @brief Multi-level timing wheel for efficient timer management
 * 
 * Features:
 * - Hierarchical timing wheels (nanosecond to second granularity)
 * - O(1) timer insertion and deletion
 * - Sub-microsecond precision on Apple Silicon
 * - Zero heap allocation on timer firing
 * - Optimized for high-frequency timer operations
 */
class TimeWheel {
public:
    static constexpr std::size_t WHEEL_SIZE = 256;  // 8-bit addressing
    static constexpr std::size_t NUM_LEVELS = 4;    // ns, μs, ms, s
    
    /**
     * @brief Constructor
     * @param tick_duration Base tick duration (default: 1μs)
     */
    explicit TimeWheel(Duration tick_duration = std::chrono::microseconds(1));
    
    /**
     * @brief Destructor
     */
    ~TimeWheel();

    // Non-copyable, non-movable
    TimeWheel(const TimeWheel&) = delete;
    TimeWheel& operator=(const TimeWheel&) = delete;
    TimeWheel(TimeWheel&&) = delete;
    TimeWheel& operator=(TimeWheel&&) = delete;

    /**
     * @brief Initialize the timing wheel
     * @return true on success
     */
    bool initialize();

    /**
     * @brief Schedule a one-shot timer
     * @param delay Delay before firing
     * @param callback Function to call when timer expires
     * @return Timer ID for cancellation
     */
    TimerId schedule_once(Duration delay, std::function<void()> callback);

    /**
     * @brief Schedule a recurring timer
     * @param interval Interval between firings
     * @param callback Function to call on each expiry
     * @return Timer ID for cancellation
     */
    TimerId schedule_recurring(Duration interval, std::function<void()> callback);

    /**
     * @brief Cancel a timer
     * @param timer_id ID returned from schedule_*
     * @return true if cancelled, false if not found
     */
    bool cancel_timer(TimerId timer_id);

    /**
     * @brief Advance time and execute expired timers
     * @param current_time Current timestamp
     * @return Number of timers executed
     */
    std::size_t tick(TimePoint current_time);

    /**
     * @brief Get current high-precision timestamp
     * @return Current time with nanosecond precision
     */
    static TimePoint now() {
        return std::chrono::high_resolution_clock::now();
    }

    /**
     * @brief Get Apple-specific mach timestamp
     * @return Mach absolute time
     */
#ifdef __APPLE__
    static std::uint64_t mach_now() {
        return mach_absolute_time();
    }
#endif

    /**
     * @brief Get number of active timers
     * @return Timer count
     */
    std::size_t active_timer_count() const {
        return active_timers_.load(std::memory_order_relaxed);
    }

    /**
     * @brief Get timing statistics
     */
    struct Statistics {
        std::uint64_t total_scheduled{0};
        std::uint64_t total_executed{0};
        std::uint64_t total_cancelled{0};
        double avg_execution_time_ns{0.0};
        std::uint64_t max_execution_time_ns{0};
    };

    Statistics get_statistics() const;

private:
    /**
     * @brief Timer bucket for each wheel slot
     */
    struct TimerBucket {
        std::vector<std::unique_ptr<TimerEvent>> timers;
        
        TimerBucket() {
            timers.reserve(16);  // Avoid small allocations
        }
    };

    /**
     * @brief Single timing wheel level
     */
    struct WheelLevel {
        std::array<TimerBucket, WHEEL_SIZE> buckets;
        std::size_t current_slot{0};
        Duration tick_size;
        
        explicit WheelLevel(Duration tick) : tick_size(tick) {}
    };

    const Duration base_tick_duration_;
    TimePoint last_tick_time_;
    
    // Multi-level timing wheels
    std::array<std::unique_ptr<WheelLevel>, NUM_LEVELS> wheels_;
    
    // Timer management
    std::atomic<TimerId> next_timer_id_{1};
    std::atomic<std::size_t> active_timers_{0};
    
    // Statistics
    mutable std::atomic<std::uint64_t> total_scheduled_{0};
    mutable std::atomic<std::uint64_t> total_executed_{0};
    mutable std::atomic<std::uint64_t> total_cancelled_{0};
    mutable std::atomic<std::uint64_t> total_execution_time_ns_{0};
    mutable std::atomic<std::uint64_t> max_execution_time_ns_{0};

    /**
     * @brief Initialize wheel levels with appropriate tick sizes
     */
    void initialize_wheels();

    /**
     * @brief Insert timer into appropriate wheel level
     * @param timer Timer to insert
     * @param current_time Current timestamp
     */
    void insert_timer(std::unique_ptr<TimerEvent> timer, TimePoint current_time);

    /**
     * @brief Calculate wheel level and slot for a timer
     * @param expiry_time Timer expiry time
     * @param current_time Current time
     * @return Pair of (level, slot)
     */
    std::pair<std::size_t, std::size_t> calculate_position(
        TimePoint expiry_time, TimePoint current_time) const;

    /**
     * @brief Advance a single wheel level
     * @param level Wheel level index
     * @param ticks Number of ticks to advance
     * @param current_time Current timestamp
     * @return Number of timers promoted to lower levels
     */
    std::size_t advance_wheel(std::size_t level, std::size_t ticks, TimePoint current_time);

    /**
     * @brief Execute timers in a bucket
     * @param bucket Bucket containing expired timers
     * @param current_time Current timestamp
     * @return Number of timers executed
     */
    std::size_t execute_bucket(TimerBucket& bucket, TimePoint current_time);

    /**
     * @brief Update performance statistics
     * @param execution_time_ns Time taken to execute timer
     */
    void update_statistics(std::uint64_t execution_time_ns) const;
};

/**
 * @brief High-precision timer for measuring execution times
 */
class [[nodiscard]] PrecisionTimer {
public:
    PrecisionTimer() : start_time_(TimeWheel::now()) {}

    /**
     * @brief Get elapsed time in nanoseconds
     * @return Elapsed nanoseconds
     */
    std::uint64_t elapsed_ns() const {
        const auto end_time = TimeWheel::now();
        return std::chrono::duration_cast<std::chrono::nanoseconds>(
            end_time - start_time_).count();
    }

private:
    const TimePoint start_time_;
};

} // namespace hfx::core
