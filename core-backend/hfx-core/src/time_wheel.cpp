/**
 * @file time_wheel.cpp
 * @brief Implementation of high-precision timing wheel
 */

#include "time_wheel.hpp"
#include <iostream>
#include <chrono>

#ifdef __APPLE__
#include <mach/mach_time.h>
#endif

namespace hfx::core {

// High-precision timestamp function for Apple Silicon
static inline std::uint64_t get_timestamp_ns() noexcept {
#ifdef __APPLE__
    static mach_timebase_info_data_t timebase_info = {0, 0};
    if (timebase_info.denom == 0) {
        mach_timebase_info(&timebase_info);
    }
    return mach_absolute_time() * timebase_info.numer / timebase_info.denom;
#else
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
#endif
}

TimeWheel::TimeWheel(Duration tick_duration) 
    : base_tick_duration_(tick_duration),
      last_tick_time_(std::chrono::high_resolution_clock::now()) {
}

TimeWheel::~TimeWheel() = default;

bool TimeWheel::initialize() {
    initialize_wheels();
    HFX_LOG_INFO("[TimeWheel] Initialized with " << base_tick_duration_.count() 
              << "ns base tick\n";
    return true;
}

TimerId TimeWheel::schedule_once(Duration delay, std::function<void()> callback) {
    const auto timer_id = next_timer_id_.fetch_add(1, std::memory_order_relaxed);
    const auto current_time = now();
    const auto expiry_time = current_time + delay;
    
    auto timer = std::make_unique<TimerEvent>(timer_id, expiry_time, Duration::zero(), 
                                              std::move(callback), false);
    
    insert_timer(std::move(timer), current_time);
    active_timers_.fetch_add(1, std::memory_order_relaxed);
    total_scheduled_.fetch_add(1, std::memory_order_relaxed);
    
    return timer_id;
}

TimerId TimeWheel::schedule_recurring(Duration interval, std::function<void()> callback) {
    const auto timer_id = next_timer_id_.fetch_add(1, std::memory_order_relaxed);
    const auto current_time = now();
    const auto expiry_time = current_time + interval;
    
    auto timer = std::make_unique<TimerEvent>(timer_id, expiry_time, interval, 
                                              std::move(callback), true);
    
    insert_timer(std::move(timer), current_time);
    active_timers_.fetch_add(1, std::memory_order_relaxed);
    total_scheduled_.fetch_add(1, std::memory_order_relaxed);
    
    return timer_id;
}

bool TimeWheel::cancel_timer(TimerId timer_id) {
    // Simplified implementation - in practice would need more complex management
    total_cancelled_.fetch_add(1, std::memory_order_relaxed);
    return true;
}

std::size_t TimeWheel::tick(TimePoint current_time) {
    std::size_t executed_count = 0;
    
    // Calculate ticks elapsed since last tick
    const auto elapsed = current_time - last_tick_time_;
    const auto elapsed_ns = std::chrono::duration_cast<Duration>(elapsed);
    const auto ticks_count = elapsed_ns.count() / base_tick_duration_.count();
    
    if (ticks_count > 0) {
        // Advance all wheels
        for (std::size_t level = 0; level < wheels_.size(); ++level) {
            if (wheels_[level]) {
                executed_count += advance_wheel(level, ticks_count, current_time);
            }
        }
        
        last_tick_time_ = current_time;
    }
    
    return executed_count;
}

TimeWheel::Statistics TimeWheel::get_statistics() const {
    return Statistics{
        .total_scheduled = total_scheduled_.load(std::memory_order_relaxed),
        .total_executed = total_executed_.load(std::memory_order_relaxed),
        .total_cancelled = total_cancelled_.load(std::memory_order_relaxed),
        .avg_execution_time_ns = 0.0, // Simplified
        .max_execution_time_ns = max_execution_time_ns_.load(std::memory_order_relaxed)
    };
}

void TimeWheel::initialize_wheels() {
    // Initialize hierarchical wheels with different granularities
    wheels_[0] = std::make_unique<WheelLevel>(base_tick_duration_);  // Base level
    wheels_[1] = std::make_unique<WheelLevel>(base_tick_duration_ * WHEEL_SIZE);  // Second level
    wheels_[2] = std::make_unique<WheelLevel>(base_tick_duration_ * WHEEL_SIZE * WHEEL_SIZE);  // Third level
    wheels_[3] = std::make_unique<WheelLevel>(base_tick_duration_ * WHEEL_SIZE * WHEEL_SIZE * WHEEL_SIZE);  // Fourth level
}

void TimeWheel::insert_timer(std::unique_ptr<TimerEvent> timer, TimePoint current_time) {
    const auto [level, slot] = calculate_position(timer->expiry_time, current_time);
    
    if (level < wheels_.size() && wheels_[level]) {
        wheels_[level]->buckets[slot].timers.emplace_back(std::move(timer));
    }
}

std::pair<std::size_t, std::size_t> TimeWheel::calculate_position(
    TimePoint expiry_time, TimePoint current_time) const {
    
    const auto delay = expiry_time - current_time;
    const auto delay_ns = std::chrono::duration_cast<Duration>(delay);
    
    // Find appropriate wheel level
    std::size_t level = 0;
    std::uint64_t ticks = delay_ns.count() / base_tick_duration_.count();
    
    while (level < wheels_.size() - 1 && ticks >= WHEEL_SIZE) {
        ticks /= WHEEL_SIZE;
        ++level;
    }
    
    const std::size_t slot = ticks % WHEEL_SIZE;
    return {level, slot};
}

std::size_t TimeWheel::advance_wheel(std::size_t level, std::size_t ticks, TimePoint current_time) {
    if (level >= wheels_.size() || !wheels_[level]) {
        return 0;
    }
    
    std::size_t executed_count = 0;
    auto& wheel = *wheels_[level];
    
    for (std::size_t tick = 0; tick < ticks; ++tick) {
        auto& bucket = wheel.buckets[wheel.current_slot];
        executed_count += execute_bucket(bucket, current_time);
        
        wheel.current_slot = (wheel.current_slot + 1) % WHEEL_SIZE;
    }
    
    return executed_count;
}

std::size_t TimeWheel::execute_bucket(TimerBucket& bucket, TimePoint current_time) {
    std::size_t executed_count = 0;
    
    auto it = bucket.timers.begin();
    while (it != bucket.timers.end()) {
        auto& timer = *it;
        
        if (timer->expiry_time <= current_time) {
            // Execute timer
            const auto start_time = get_timestamp_ns();
            
            // Execute callback (no exception handling in HFT code)
            timer->callback();
            executed_count++;
            total_executed_.fetch_add(1, std::memory_order_relaxed);
            
            const auto end_time = get_timestamp_ns();
            update_statistics(end_time - start_time);
            
            // Handle recurring timers
            if (timer->recurring && timer->interval > Duration::zero()) {
                timer->expiry_time = current_time + timer->interval;
                insert_timer(std::move(timer), current_time);
            } else {
                active_timers_.fetch_sub(1, std::memory_order_relaxed);
            }
            
            it = bucket.timers.erase(it);
        } else {
            ++it;
        }
    }
    
    return executed_count;
}

void TimeWheel::update_statistics(std::uint64_t execution_time_ns) const {
    total_execution_time_ns_.fetch_add(execution_time_ns, std::memory_order_relaxed);
    
    // Update maximum execution time
    std::uint64_t current_max = max_execution_time_ns_.load(std::memory_order_relaxed);
    while (execution_time_ns > current_max &&
           !max_execution_time_ns_.compare_exchange_weak(
               current_max, execution_time_ns, std::memory_order_relaxed)) {
        // Loop until we successfully update or find a larger value
    }
}

// get_timestamp_ns is defined in the header of this file

} // namespace hfx::core
