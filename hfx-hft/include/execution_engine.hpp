/**
 * @file execution_engine.hpp
 * @brief Ultra-low latency execution engine for memecoin trading
 * @version 1.0.0
 * 
 * Sub-microsecond order execution with hardware optimization
 * Lock-free data structures and kernel bypass for maximum speed
 */

#pragma once

#include <memory>
#include <string>
#include <atomic>
#include <chrono>
#include <functional>
#include <array>
#include <vector>
#include <unordered_map>
#include <thread>
#include <mutex>
#include <cstring>

#ifdef __APPLE__
#include <mach/mach_time.h>
#include <pthread.h>
#include <mach/thread_policy.h>
#include <mach/thread_act.h>
#endif

#ifdef HFX_DPDK_ENABLED
#include <rte_config.h>
#include <rte_ring.h>
#include <rte_mempool.h>
#endif

namespace hfx::hft {

/**
 * @brief Ultra-high precision timing utilities
 */
class PrecisionTimer {
public:
    static inline uint64_t get_nanoseconds() noexcept {
#ifdef __APPLE__
        return mach_absolute_time();
#else
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
        return ts.tv_sec * 1000000000ULL + ts.tv_nsec;
#endif
    }
    
    static inline uint64_t get_cpu_cycles() noexcept {
#ifdef __x86_64__
        unsigned int lo, hi;
        __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
        return ((uint64_t)hi << 32) | lo;
#else
        return get_nanoseconds(); // Fallback
#endif
    }
};

/**
 * @brief Lock-free ring buffer for ultra-fast message passing
 */
template<typename T, size_t Size>
class alignas(64) LockFreeRingBuffer {
    static_assert((Size & (Size - 1)) == 0, "Size must be power of 2");
    
public:
    LockFreeRingBuffer() : head_(0), tail_(0) {}
    
    bool try_push(const T& item) noexcept {
        const size_t current_tail = tail_.load(std::memory_order_relaxed);
        const size_t next_tail = (current_tail + 1) & (Size - 1);
        
        if (next_tail == head_.load(std::memory_order_acquire)) {
            return false; // Buffer full
        }
        
        buffer_[current_tail] = item;
        tail_.store(next_tail, std::memory_order_release);
        return true;
    }
    
    bool try_pop(T& item) noexcept {
        const size_t current_head = head_.load(std::memory_order_relaxed);
        
        if (current_head == tail_.load(std::memory_order_acquire)) {
            return false; // Buffer empty
        }
        
        item = buffer_[current_head];
        head_.store((current_head + 1) & (Size - 1), std::memory_order_release);
        return true;
    }
    
    bool empty() const noexcept {
        return head_.load(std::memory_order_acquire) == 
               tail_.load(std::memory_order_acquire);
    }
    
private:
    alignas(64) std::atomic<size_t> head_;
    alignas(64) std::atomic<size_t> tail_;
    alignas(64) std::array<T, Size> buffer_;
};

/**
 * @brief Ultra-fast order execution command
 */
struct alignas(64) ExecutionCommand {
    enum class Type : uint8_t {
        BUY_MARKET,
        SELL_MARKET,
        BUY_LIMIT,
        SELL_LIMIT,
        CANCEL_ORDER,
        EMERGENCY_STOP
    };
    
    Type type;
    uint8_t platform_id;
    uint16_t priority; // Higher = more urgent
    uint32_t order_id;
    
    char token_address[64];
    double amount;
    double price;
    double max_slippage;
    uint64_t timestamp_ns;
    uint64_t max_execution_time_ns;
    
    // Execution flags
    bool use_mev_protection : 1;
    bool use_priority_fees : 1;
    bool emergency_mode : 1;
    bool reserved : 5;
} __attribute__((packed));

/**
 * @brief Execution result with timing metrics
 */
struct alignas(64) ExecutionResult {
    enum class Status : uint8_t {
        SUCCESS,
        FAILED,
        PARTIAL_FILL,
        CANCELLED,
        TIMEOUT,
        INSUFFICIENT_BALANCE,
        SLIPPAGE_EXCEEDED,
        MEV_DETECTED
    };
    
    Status status;
    uint8_t platform_id;
    uint16_t reserved;
    uint32_t order_id;
    
    char transaction_hash[128];
    double executed_amount;
    double executed_price;
    double actual_slippage;
    double total_fees;
    
    // Timing metrics (nanoseconds)
    uint64_t command_received_ns;
    uint64_t validation_complete_ns;
    uint64_t order_sent_ns;
    uint64_t order_confirmed_ns;
    uint64_t total_latency_ns;
    
    // Performance metrics
    bool frontran_detected : 1;
    bool sandwich_detected : 1;
    bool mev_protection_used : 1;
    bool priority_fees_used : 1;
    uint8_t reserved_flags : 4;
} __attribute__((packed));

/**
 * @brief Memory pool for zero-allocation execution
 */
class ExecutionMemoryPool {
public:
    static constexpr size_t POOL_SIZE = 1024 * 1024; // 1MB
    static constexpr size_t COMMAND_POOL_SIZE = 10000;
    static constexpr size_t RESULT_POOL_SIZE = 10000;
    
    ExecutionMemoryPool();
    ~ExecutionMemoryPool();
    
    ExecutionCommand* allocate_command() noexcept;
    void deallocate_command(ExecutionCommand* cmd) noexcept;
    
    ExecutionResult* allocate_result() noexcept;
    void deallocate_result(ExecutionResult* result) noexcept;
    
private:
    alignas(64) std::atomic<ExecutionCommand*> command_freelist_;
    alignas(64) std::atomic<ExecutionResult*> result_freelist_;
    
    std::unique_ptr<char[]> memory_pool_;
    ExecutionCommand* command_pool_;
    ExecutionResult* result_pool_;
};

/**
 * @brief Platform abstraction for execution
 */
class IExecutionPlatform {
public:
    virtual ~IExecutionPlatform() = default;
    
    virtual bool execute_command(const ExecutionCommand& cmd, ExecutionResult& result) = 0;
    virtual bool cancel_order(uint32_t order_id) = 0;
    virtual double get_balance() = 0;
    virtual bool is_healthy() = 0;
    
    // Platform-specific optimizations
    virtual void warm_up_connection() = 0;
    virtual void set_priority_mode(bool enabled) = 0;
};

/**
 * @brief Ultra-low latency execution engine
 */
class UltraFastExecutionEngine {
public:
    struct Config {
        size_t worker_threads = 4;
        size_t command_queue_size = 65536;
        size_t result_queue_size = 65536;
        uint64_t max_execution_latency_ns = 1000000; // 1ms default
        bool enable_cpu_affinity = true;
        bool enable_memory_locking = true;
        bool enable_real_time_priority = true;
        std::vector<int> cpu_cores; // Specific cores to use
    };
    
    explicit UltraFastExecutionEngine(const Config& config);
    ~UltraFastExecutionEngine();
    
    // Lifecycle
    bool initialize();
    void shutdown();
    bool is_running() const;
    
    // Platform management
    bool add_platform(uint8_t platform_id, std::unique_ptr<IExecutionPlatform> platform);
    void remove_platform(uint8_t platform_id);
    
    // Command execution (ultra-fast path)
    bool submit_command(const ExecutionCommand& cmd) noexcept;
    bool get_result(ExecutionResult& result) noexcept;
    
    // Batch operations
    size_t submit_commands(const ExecutionCommand* commands, size_t count) noexcept;
    size_t get_results(ExecutionResult* results, size_t max_count) noexcept;
    
    // Performance monitoring
    struct PerformanceMetrics {
        std::atomic<uint64_t> commands_processed{0};
        std::atomic<uint64_t> commands_failed{0};
        std::atomic<uint64_t> avg_latency_ns{0};
        std::atomic<uint64_t> max_latency_ns{0};
        std::atomic<uint64_t> queue_overflows{0};
        std::atomic<uint64_t> mev_attacks_detected{0};
        std::atomic<uint64_t> emergency_stops{0};
    };
    
    void get_metrics(PerformanceMetrics& metrics_out) const;
    void reset_metrics();
    
    // Emergency controls
    void emergency_stop_all();
    void pause_execution();
    void resume_execution();
    
    // Callbacks for monitoring
    using LatencyCallback = std::function<void(uint64_t latency_ns)>;
    using ErrorCallback = std::function<void(const std::string& error)>;
    
    void set_latency_callback(LatencyCallback callback);
    void set_error_callback(ErrorCallback callback);
    
private:
    class ExecutionImpl;
    std::unique_ptr<ExecutionImpl> pimpl_;
};

/**
 * @brief Hardware-optimized networking for direct exchange access
 */
class HighSpeedNetworking {
public:
    struct NetworkConfig {
        std::string interface_name = "eth0";
        bool enable_kernel_bypass = true;
        bool enable_zero_copy = true;
        bool enable_batching = true;
        size_t rx_buffer_size = 2048;
        size_t tx_buffer_size = 2048;
        int cpu_core = -1; // -1 = auto
    };
    
    explicit HighSpeedNetworking(const NetworkConfig& config);
    ~HighSpeedNetworking();
    
    bool initialize();
    void shutdown();
    
    // Ultra-fast packet send/receive
    bool send_packet(const void* data, size_t len, const std::string& destination);
    bool receive_packet(void* buffer, size_t& len, std::string& source);
    
    // Batch operations
    size_t send_batch(const void** packets, const size_t* lengths, 
                     const std::string* destinations, size_t count);
    size_t receive_batch(void** buffers, size_t* lengths, 
                        std::string* sources, size_t max_count);
    
    // Statistics
    struct NetworkStats {
        std::atomic<uint64_t> packets_sent{0};
        std::atomic<uint64_t> packets_received{0};
        std::atomic<uint64_t> bytes_sent{0};
        std::atomic<uint64_t> bytes_received{0};
        std::atomic<uint64_t> send_errors{0};
        std::atomic<uint64_t> receive_errors{0};
        std::atomic<uint64_t> avg_send_latency_ns{0};
    };
    
    const NetworkStats& get_stats() const;
    
private:
    class NetworkImpl;
    std::unique_ptr<NetworkImpl> pimpl_;
};

/**
 * @brief CPU cache-optimized data structures
 */
template<typename T, size_t CacheLineSize = 64>
class alignas(CacheLineSize) CacheOptimizedArray {
public:
    explicit CacheOptimizedArray(size_t size) 
        : size_(size), data_(static_cast<T*>(std::aligned_alloc(CacheLineSize, size * sizeof(T)))) {
        // In HFT systems, we don't use exceptions - return nullptr will be checked by caller
    }
    
    ~CacheOptimizedArray() {
        std::free(data_);
    }
    
    T& operator[](size_t index) noexcept { return data_[index]; }
    const T& operator[](size_t index) const noexcept { return data_[index]; }
    
    size_t size() const noexcept { return size_; }
    T* data() noexcept { return data_; }
    const T* data() const noexcept { return data_; }
    
private:
    size_t size_;
    T* data_;
};

/**
 * @brief NUMA-aware memory allocator
 */
class NUMAAllocator {
public:
    static void* allocate(size_t size, int numa_node = -1);
    static void deallocate(void* ptr, size_t size);
    static int get_current_numa_node();
    static int get_optimal_numa_node_for_cpu(int cpu_core);
};

/**
 * @brief Real-time thread utilities
 */
class RealTimeThread {
public:
    enum class Priority {
        NORMAL = 0,
        HIGH = 1,
        REAL_TIME = 99
    };
    
    static bool set_thread_priority(Priority priority);
    static bool set_thread_affinity(const std::vector<int>& cpu_cores);
    static bool lock_memory_pages();
    static bool disable_page_faults();
};

} // namespace hfx::hft
