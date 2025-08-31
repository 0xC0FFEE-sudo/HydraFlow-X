/**
 * @file lockfree_queue.hpp
 * @brief Ultra-high performance lock-free SPSC queue for Apple Silicon
 * @version 1.0.0
 * 
 * Single Producer Single Consumer queue optimized for sub-nanosecond latency.
 * Uses cache-aligned memory and ARM64-specific optimizations.
 */

#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <new>

#ifdef __APPLE__
#include <libkern/OSCacheControl.h>
#endif

namespace hfx::core {

/**
 * @class LockFreeQueue
 * @brief High-performance SPSC queue with cache optimization
 * 
 * Features:
 * - Single Producer, Single Consumer
 * - Cache line aligned for Apple Silicon (64 bytes)
 * - Memory barriers optimized for ARM64
 * - Zero allocation after initialization
 * - Batch operations for improved throughput
 */
template<typename T>
class alignas(64) LockFreeQueue {
public:
    static_assert(std::is_trivially_copyable_v<T>, "T must be trivially copyable");
    
    /**
     * @brief Constructor with power-of-2 capacity for fast modulo
     * @param capacity Queue capacity (must be power of 2)
     */
    explicit LockFreeQueue(std::size_t capacity = 65536)
        : capacity_(next_power_of_2(capacity)),
          mask_(capacity_ - 1),
          buffer_(static_cast<T*>(std::aligned_alloc(64, sizeof(T) * capacity_))) {
        
        if (!buffer_) {
            std::abort(); // HFT: No exceptions, abort on allocation failure
        }
        
        // Initialize atomic indices on separate cache lines
        head_.store(0, std::memory_order_relaxed);
        tail_.store(0, std::memory_order_relaxed);
    }

    /**
     * @brief Destructor
     */
    ~LockFreeQueue() {
        std::free(buffer_);
    }

    // Non-copyable, non-movable
    LockFreeQueue(const LockFreeQueue&) = delete;
    LockFreeQueue& operator=(const LockFreeQueue&) = delete;
    LockFreeQueue(LockFreeQueue&&) = delete;
    LockFreeQueue& operator=(LockFreeQueue&&) = delete;

    /**
     * @brief Enqueue an element (producer side)
     * @param item Item to enqueue
     * @return true if enqueued, false if queue is full
     */
    [[gnu::hot]] bool enqueue(const T& item) noexcept {
        const auto current_tail = tail_.load(std::memory_order_relaxed);
        const auto next_tail = (current_tail + 1) & mask_;
        
        // Check if queue is full (leave one slot empty to distinguish full from empty)
        if (next_tail == head_.load(std::memory_order_acquire)) {
            return false;
        }
        
        // Store the item
        buffer_[current_tail] = item;
        
        // Memory fence for ARM64 - ensure store completes before updating tail
#ifdef __aarch64__
        __asm__ volatile("dmb ishst" ::: "memory");
#else
        std::atomic_thread_fence(std::memory_order_release);
#endif
        
        // Update tail with release semantics
        tail_.store(next_tail, std::memory_order_release);
        return true;
    }

    /**
     * @brief Dequeue an element (consumer side)
     * @param item Reference to store dequeued item
     * @return true if dequeued, false if queue is empty
     */
    [[gnu::hot]] bool dequeue(T& item) noexcept {
        const auto current_head = head_.load(std::memory_order_relaxed);
        
        // Check if queue is empty
        if (current_head == tail_.load(std::memory_order_acquire)) {
            return false;
        }
        
        // Load the item
        item = buffer_[current_head];
        
        // Memory fence for ARM64
#ifdef __aarch64__
        __asm__ volatile("dmb ishld" ::: "memory");
#else
        std::atomic_thread_fence(std::memory_order_release);
#endif
        
        // Update head with release semantics
        head_.store((current_head + 1) & mask_, std::memory_order_release);
        return true;
    }

    /**
     * @brief Batch enqueue for improved throughput
     * @param items Array of items to enqueue
     * @param count Number of items
     * @return Number of items actually enqueued
     */
    std::size_t enqueue_batch(const T* items, std::size_t count) noexcept {
        std::size_t enqueued = 0;
        for (std::size_t i = 0; i < count; ++i) {
            if (!enqueue(items[i])) {
                break;
            }
            ++enqueued;
        }
        return enqueued;
    }

    /**
     * @brief Batch dequeue for improved throughput
     * @param items Array to store dequeued items
     * @param max_count Maximum number of items to dequeue
     * @return Number of items actually dequeued
     */
    std::size_t dequeue_batch(T* items, std::size_t max_count) noexcept {
        std::size_t dequeued = 0;
        for (std::size_t i = 0; i < max_count; ++i) {
            if (!dequeue(items[i])) {
                break;
            }
            ++dequeued;
        }
        return dequeued;
    }

    /**
     * @brief Check if queue is empty (approximate)
     * @return true if likely empty, false otherwise
     */
    bool empty() const noexcept {
        return head_.load(std::memory_order_relaxed) == 
               tail_.load(std::memory_order_relaxed);
    }

    /**
     * @brief Get approximate queue size
     * @return Approximate number of items in queue
     */
    std::size_t size() const noexcept {
        const auto h = head_.load(std::memory_order_relaxed);
        const auto t = tail_.load(std::memory_order_relaxed);
        return (t - h) & mask_;
    }

    /**
     * @brief Get queue capacity
     * @return Maximum number of items that can be stored
     */
    std::size_t capacity() const noexcept {
        return capacity_ - 1;  // One slot reserved to distinguish full from empty
    }

    /**
     * @brief Warm up the queue by touching all cache lines
     */
    void warmup() noexcept {
        // Touch all cache lines to ensure they're in cache
        const std::size_t cache_line_size = 64;
        const std::size_t lines = (capacity_ * sizeof(T) + cache_line_size - 1) / cache_line_size;
        
        volatile char* ptr = reinterpret_cast<volatile char*>(buffer_);
        for (std::size_t i = 0; i < lines; ++i) {
            ptr[i * cache_line_size] = ptr[i * cache_line_size];
        }
    }

private:
    const std::size_t capacity_;
    const std::size_t mask_;
    T* const buffer_;

    // Align atomic variables to separate cache lines to prevent false sharing
    alignas(64) std::atomic<std::size_t> head_{0};
    alignas(64) std::atomic<std::size_t> tail_{0};

    /**
     * @brief Calculate next power of 2 greater than or equal to n
     */
    static constexpr std::size_t next_power_of_2(std::size_t n) {
        if (n <= 1) return 2;
        
        --n;
        n |= n >> 1;
        n |= n >> 2;
        n |= n >> 4;
        n |= n >> 8;
        n |= n >> 16;
        if constexpr (sizeof(std::size_t) > 4) {
            n |= n >> 32;
        }
        return n + 1;
    }
};

// Type alias for Event queue
struct Event;  // Forward declaration (matches event_engine.hpp)
using EventQueue = LockFreeQueue<Event>;

} // namespace hfx::core
