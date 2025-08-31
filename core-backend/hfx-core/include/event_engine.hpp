/**
 * @file event_engine.hpp
 * @brief Ultra-low latency event processing engine with deterministic timing
 * @version 1.0.0
 * 
 * Single-threaded event loop with nanosecond precision timing for HFT applications.
 * Optimized for Apple Silicon with kqueue integration and lock-free data structures.
 */

#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

#include "lockfree_queue.hpp"

#ifdef __APPLE__
#include <sys/event.h>
#include <sys/time.h>
#include <mach/mach_time.h>
#endif

namespace hfx::core {

using TimeStamp = std::chrono::time_point<std::chrono::high_resolution_clock>;
using EventId = std::uint64_t;

/**
 * @enum EventType
 * @brief Types of events processed by the engine
 */
enum class EventType : std::uint8_t {
    TIMER_EXPIRED,
    NETWORK_DATA,
    MARKET_DATA,
    TRADE_SIGNAL,
    RISK_ALERT,
    SYSTEM_SHUTDOWN
};

/**
 * @struct Event
 * @brief Minimal event structure optimized for cache efficiency
 */
struct alignas(64) Event {  // 64-byte cache line alignment for Apple Silicon
    EventType type;
    EventId id;
    TimeStamp timestamp;
    std::uint64_t data;
    void* payload;
    
    Event() = default;
    Event(EventType t, EventId i, std::uint64_t d = 0, void* p = nullptr)
        : type(t), id(i), timestamp(std::chrono::high_resolution_clock::now()), 
          data(d), payload(p) {}
} __attribute__((packed));

/**
 * @class EventEngine
 * @brief High-performance event processing engine
 * 
 * Features:
 * - Single-threaded deterministic execution
 * - kqueue integration for macOS
 * - Sub-nanosecond timing precision
 * - Lock-free event queues
 * - Zero dynamic allocation on hot path
 */
class EventEngine {
public:
    using EventHandler = std::function<void(const Event&)>;
    
    EventEngine();
    ~EventEngine();

    // Non-copyable, non-movable
    EventEngine(const EventEngine&) = delete;
    EventEngine& operator=(const EventEngine&) = delete;
    EventEngine(EventEngine&&) = delete;
    EventEngine& operator=(EventEngine&&) = delete;

    /**
     * @brief Initialize the event engine
     * @return true on success, false on failure
     */
    bool initialize();

    /**
     * @brief Shutdown the event engine gracefully
     */
    void shutdown();

    /**
     * @brief Process all pending events (main loop iteration)
     * @return Number of events processed
     */
    std::size_t process_events();

    /**
     * @brief Register an event handler for a specific event type
     * @param type Event type to handle
     * @param handler Function to call when event occurs
     */
    void register_handler(EventType type, EventHandler handler);

    /**
     * @brief Post an event to the queue (lock-free)
     * @param event Event to post
     * @return true if posted successfully, false if queue full
     */
    bool post_event(const Event& event);

    /**
     * @brief Get current high-precision timestamp
     * @return Nanosecond precision timestamp
     */
    static std::uint64_t get_timestamp_ns();

    /**
     * @brief Get monotonic clock timestamp (Apple-specific)
     * @return Mach timebase timestamp
     */
    static std::uint64_t get_mach_timestamp();

    /**
     * @brief Check if engine is running
     * @return true if running, false otherwise
     */
    bool is_running() const noexcept { return running_.load(std::memory_order_acquire); }

    /**
     * @brief Get total events processed since start
     * @return Event count
     */
    std::uint64_t get_event_count() const noexcept { return event_count_.load(std::memory_order_relaxed); }

    /**
     * @brief Get average processing latency in nanoseconds
     * @return Average latency
     */
    double get_avg_latency_ns() const;

private:
    static constexpr std::size_t MAX_EVENTS_PER_BATCH = 1024;
    static constexpr std::size_t EVENT_QUEUE_SIZE = 65536;  // Power of 2 for fast modulo

    std::atomic<bool> running_{false};
    std::atomic<bool> shutdown_requested_{false};
    std::atomic<std::uint64_t> event_count_{0};

    // Event handlers array indexed by EventType
    std::array<EventHandler, 8> handlers_{};

    // High-performance event queue (lock-free SPSC)
    std::unique_ptr<LockFreeQueue<Event>> event_queue_;

    // macOS-specific kqueue descriptor
#ifdef __APPLE__
    int kqueue_fd_{-1};
    mach_timebase_info_data_t timebase_info_{};
#endif

    // Performance metrics
    mutable std::atomic<std::uint64_t> total_latency_ns_{0};
    TimeStamp last_process_time_;

    /**
     * @brief Initialize platform-specific resources
     */
    bool initialize_platform();

    /**
     * @brief Process a single event with timing measurement
     * @param event Event to process
     */
    void process_single_event(const Event& event);

    /**
     * @brief Handle timer events
     * @param event Timer event
     */
    void handle_timer_event(const Event& event);

    /**
     * @brief Update performance metrics
     * @param processing_time_ns Time taken to process event
     */
    void update_metrics(std::uint64_t processing_time_ns);
};

/**
 * @brief RAII timer for measuring event processing latency
 */
class [[nodiscard]] LatencyTimer {
public:
    explicit LatencyTimer(std::uint64_t& latency_accumulator)
        : start_time_(EventEngine::get_timestamp_ns()),
          latency_accumulator_(latency_accumulator) {}

    ~LatencyTimer() {
        const auto end_time = EventEngine::get_timestamp_ns();
        latency_accumulator_ += (end_time - start_time_);
    }

private:
    const std::uint64_t start_time_;
    std::uint64_t& latency_accumulator_;
};

} // namespace hfx::core
