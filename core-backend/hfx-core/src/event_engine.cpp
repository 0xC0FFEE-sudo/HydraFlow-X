/**
 * @file event_engine.cpp
 * @brief Implementation of ultra-low latency event processing engine
 */

#include "event_engine.hpp"
#include "lockfree_queue.hpp"
#include <iostream>
#include <thread>
#include <chrono>

#ifdef __APPLE__
#include <pthread.h>
#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>
#include <unistd.h>
#include <mach/mach_time.h>
#endif

namespace hfx::core {

// High-precision timestamp function for Apple Silicon
[[maybe_unused]] static inline std::uint64_t get_timestamp_ns() noexcept {
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

EventEngine::EventEngine()
    : event_queue_(std::make_unique<LockFreeQueue<Event>>(EVENT_QUEUE_SIZE)),
      last_process_time_(std::chrono::high_resolution_clock::now()) {
    
    // Initialize handlers array
    handlers_.fill(nullptr);
}

EventEngine::~EventEngine() {
    shutdown();
}

bool EventEngine::initialize() {
    if (running_.load(std::memory_order_acquire)) {
        return false;  // Already running
    }

    // Initialize platform-specific resources
    if (!initialize_platform()) {
        return false;
    }

    // Warm up the event queue
    event_queue_->warmup();

    // Set running flag
    running_.store(true, std::memory_order_release);

    std::cout << "[EventEngine] Initialized successfully\n";
    return true;
}

void EventEngine::shutdown() {
    if (!running_.load(std::memory_order_acquire)) {
        return;  // Already shutdown
    }

    shutdown_requested_.store(true, std::memory_order_release);
    
    // Process remaining events
    std::size_t remaining = 0;
    do {
        remaining = process_events();
        if (remaining > 0) {
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
    } while (remaining > 0);

    running_.store(false, std::memory_order_release);

#ifdef __APPLE__
    if (kqueue_fd_ >= 0) {
        close(kqueue_fd_);
        kqueue_fd_ = -1;
    }
#endif

    std::cout << "[EventEngine] Shutdown complete. Processed " 
              << event_count_.load() << " events total.\n";
}

std::size_t EventEngine::process_events() {
    if (!running_.load(std::memory_order_acquire)) {
        return 0;
    }

    const auto batch_start_time = get_timestamp_ns();
    std::size_t processed = 0;

    // Process events in batches for better cache efficiency
    Event events[MAX_EVENTS_PER_BATCH];
    const auto dequeued = event_queue_->dequeue_batch(events, MAX_EVENTS_PER_BATCH);

    for (std::size_t i = 0; i < dequeued; ++i) {
        process_single_event(events[i]);
        ++processed;
    }

    // Update metrics
    if (processed > 0) {
        const auto batch_end_time = get_timestamp_ns();
        const auto batch_latency = batch_end_time - batch_start_time;
        update_metrics(batch_latency / processed);  // Average per event
    }

    return processed;
}

void EventEngine::register_handler(EventType type, EventHandler handler) {
    const auto index = static_cast<std::size_t>(type);
    if (index < handlers_.size()) {
        handlers_[index] = std::move(handler);
    }
}

bool EventEngine::post_event(const Event& event) {
    if (!running_.load(std::memory_order_acquire)) {
        return false;
    }

    return event_queue_->enqueue(event);
}

std::uint64_t EventEngine::get_timestamp_ns() {
#ifdef __APPLE__
    return mach_absolute_time();
#else
    const auto now = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
        now.time_since_epoch()).count();
#endif
}

std::uint64_t EventEngine::get_mach_timestamp() {
#ifdef __APPLE__
    return mach_absolute_time();
#else
    return get_timestamp_ns();
#endif
}

double EventEngine::get_avg_latency_ns() const {
    const auto total_events = event_count_.load(std::memory_order_relaxed);
    const auto total_latency = total_latency_ns_.load(std::memory_order_relaxed);
    
    return (total_events > 0) ? static_cast<double>(total_latency) / total_events : 0.0;
}

bool EventEngine::initialize_platform() {
#ifdef __APPLE__
    // Initialize kqueue for high-performance event handling
    kqueue_fd_ = kqueue();
    if (kqueue_fd_ < 0) {
        std::cerr << "[EventEngine] Failed to create kqueue\n";
        return false;
    }

    // Get timebase info for timestamp conversion
    if (mach_timebase_info(&timebase_info_) != KERN_SUCCESS) {
        std::cerr << "[EventEngine] Failed to get mach timebase info\n";
        return false;
    }

    // Set thread priority for low latency
    struct sched_param param;
    param.sched_priority = sched_get_priority_max(SCHED_FIFO);
    if (pthread_setschedparam(pthread_self(), SCHED_FIFO, &param) != 0) {
        std::cout << "[EventEngine] Warning: Could not set high priority\n";
    }

    // Set thread QoS for Apple platforms
    if (pthread_set_qos_class_self_np(QOS_CLASS_USER_INTERACTIVE, 0) != 0) {
        std::cout << "[EventEngine] Warning: Could not set QoS class\n";
    }

    std::cout << "[EventEngine] Apple-specific optimizations enabled\n";
#endif

    return true;
}

void EventEngine::process_single_event(const Event& event) {
    const auto start_time = get_timestamp_ns();
    
    const auto handler_index = static_cast<std::size_t>(event.type);
    if (handler_index < handlers_.size() && handlers_[handler_index]) {
        // Call the registered handler
        handlers_[handler_index](event);
    } else {
        // Handle built-in event types
        switch (event.type) {
            case EventType::TIMER_EXPIRED:
                handle_timer_event(event);
                break;
            
            case EventType::SYSTEM_SHUTDOWN:
                shutdown_requested_.store(true, std::memory_order_release);
                break;
            
            default:
                // Unknown event type - log but continue
                std::cerr << "[EventEngine] Unknown event type: " 
                          << static_cast<int>(event.type) << "\n";
                break;
        }
    }

    event_count_.fetch_add(1, std::memory_order_relaxed);

    // Update latency metrics
    const auto end_time = get_timestamp_ns();
    const auto processing_latency = end_time - start_time;
    update_metrics(processing_latency);
}

void EventEngine::handle_timer_event(const Event& event) {
    // Timer events are handled by the time wheel system
    // This is just a placeholder for timer-specific logic
    if (event.payload) {
        auto* callback = static_cast<std::function<void()>*>(event.payload);
        // Execute timer callback (no exception handling in HFT code)
        (*callback)();
    }
}

void EventEngine::update_metrics(std::uint64_t processing_time_ns) {
    total_latency_ns_.fetch_add(processing_time_ns, std::memory_order_relaxed);
    
    // Update maximum latency (using compare-exchange loop)
    std::uint64_t current_max = 0;
    do {
        current_max = total_latency_ns_.load(std::memory_order_relaxed);
        if (processing_time_ns <= current_max) {
            break;
        }
    } while (!total_latency_ns_.compare_exchange_weak(
        current_max, processing_time_ns, std::memory_order_relaxed));
}

} // namespace hfx::core
