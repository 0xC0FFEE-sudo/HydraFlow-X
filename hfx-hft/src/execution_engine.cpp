/**
 * @file execution_engine.cpp
 * @brief Ultra-low latency execution engine implementation
 */

#include "execution_engine.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <algorithm>
#include <random>
#include <cstring>
#include <memory>
#include <mutex>

#ifdef __APPLE__
#include <mach/mach_time.h>
#include <sys/types.h>
#include <sys/sysctl.h>
#include <pthread.h>
#include <mach/thread_policy.h>
#include <mach/thread_act.h>
#endif

#ifdef __linux__
#include <sched.h>
#include <sys/mman.h>
#include <numa.h>
#endif

namespace hfx::hft {

// ============================================================================
// Memory Pool Implementation
// ============================================================================

ExecutionMemoryPool::ExecutionMemoryPool() {
    // Allocate aligned memory pool
    memory_pool_ = std::unique_ptr<char[]>(new(std::align_val_t(64)) char[POOL_SIZE]);
    
    // Initialize command pool
    command_pool_ = reinterpret_cast<ExecutionCommand*>(memory_pool_.get());
    for (size_t i = 0; i < COMMAND_POOL_SIZE - 1; ++i) {
        *reinterpret_cast<ExecutionCommand**>(&command_pool_[i]) = &command_pool_[i + 1];
    }
    *reinterpret_cast<ExecutionCommand**>(&command_pool_[COMMAND_POOL_SIZE - 1]) = nullptr;
    command_freelist_.store(&command_pool_[0]);
    
    // Initialize result pool
    result_pool_ = reinterpret_cast<ExecutionResult*>(
        memory_pool_.get() + sizeof(ExecutionCommand) * COMMAND_POOL_SIZE);
    for (size_t i = 0; i < RESULT_POOL_SIZE - 1; ++i) {
        *reinterpret_cast<ExecutionResult**>(&result_pool_[i]) = &result_pool_[i + 1];
    }
    *reinterpret_cast<ExecutionResult**>(&result_pool_[RESULT_POOL_SIZE - 1]) = nullptr;
    result_freelist_.store(&result_pool_[0]);
}

ExecutionMemoryPool::~ExecutionMemoryPool() = default;

ExecutionCommand* ExecutionMemoryPool::allocate_command() noexcept {
    ExecutionCommand* head = command_freelist_.load();
    while (head != nullptr) {
        ExecutionCommand* next = *reinterpret_cast<ExecutionCommand**>(head);
        if (command_freelist_.compare_exchange_weak(head, next)) {
            return head;
        }
    }
    return nullptr; // Pool exhausted
}

void ExecutionMemoryPool::deallocate_command(ExecutionCommand* cmd) noexcept {
    if (!cmd) return;
    
    ExecutionCommand* head = command_freelist_.load();
    do {
        *reinterpret_cast<ExecutionCommand**>(cmd) = head;
    } while (!command_freelist_.compare_exchange_weak(head, cmd));
}

ExecutionResult* ExecutionMemoryPool::allocate_result() noexcept {
    ExecutionResult* head = result_freelist_.load();
    while (head != nullptr) {
        ExecutionResult* next = *reinterpret_cast<ExecutionResult**>(head);
        if (result_freelist_.compare_exchange_weak(head, next)) {
            return head;
        }
    }
    return nullptr; // Pool exhausted
}

void ExecutionMemoryPool::deallocate_result(ExecutionResult* result) noexcept {
    if (!result) return;
    
    ExecutionResult* head = result_freelist_.load();
    do {
        *reinterpret_cast<ExecutionResult**>(result) = head;
    } while (!result_freelist_.compare_exchange_weak(head, result));
}

// ============================================================================
// Ultra-Fast Execution Engine Implementation
// ============================================================================

class UltraFastExecutionEngine::ExecutionImpl {
public:
    Config config_;
    std::atomic<bool> running_{false};
    std::atomic<bool> emergency_stopped_{false};
    std::atomic<bool> paused_{false};
    
    // Memory management
    std::unique_ptr<ExecutionMemoryPool> memory_pool_;
    
    // Lock-free queues
    std::unique_ptr<LockFreeRingBuffer<ExecutionCommand, 65536>> command_queue_;
    std::unique_ptr<LockFreeRingBuffer<ExecutionResult, 65536>> result_queue_;
    
    // Platform management
    std::unordered_map<uint8_t, std::unique_ptr<IExecutionPlatform>> platforms_;
    std::mutex platforms_mutex_;
    
    // Worker threads
    std::vector<std::unique_ptr<std::thread>> worker_threads_;
    
    // Performance tracking
    mutable PerformanceMetrics metrics_;
    std::atomic<uint64_t> last_latency_ns_{0};
    
    // Callbacks
    LatencyCallback latency_callback_;
    ErrorCallback error_callback_;
    std::mutex callback_mutex_;
    
    ExecutionImpl(const Config& config) : config_(config) {
        memory_pool_ = std::make_unique<ExecutionMemoryPool>();
        command_queue_ = std::make_unique<LockFreeRingBuffer<ExecutionCommand, 65536>>();
        result_queue_ = std::make_unique<LockFreeRingBuffer<ExecutionResult, 65536>>();
    }
    
    ~ExecutionImpl() {
        shutdown();
    }
    
    bool initialize() {
        if (running_.load()) {
            return false;
        }
        
        std::cout << "[ExecutionEngine] Initializing ultra-low latency execution engine..." << std::endl;
        
        // Set up real-time scheduling if enabled
        if (config_.enable_real_time_priority) {
            setup_real_time_scheduling();
        }
        
        // Lock memory pages if enabled
        if (config_.enable_memory_locking) {
            lock_memory_pages();
        }
        
        // Start worker threads
        running_.store(true);
        for (size_t i = 0; i < config_.worker_threads; ++i) {
            auto thread = std::make_unique<std::thread>(&ExecutionImpl::worker_loop, this, i);
            
            // Set CPU affinity if specified
            if (config_.enable_cpu_affinity && i < config_.cpu_cores.size()) {
                set_thread_affinity(*thread, config_.cpu_cores[i]);
            }
            
            worker_threads_.push_back(std::move(thread));
        }
        
        std::cout << "[ExecutionEngine] Initialized with " << config_.worker_threads 
                  << " worker threads" << std::endl;
        return true;
    }
    
    void shutdown() {
        if (!running_.load()) {
            return;
        }
        
        running_.store(false);
        
        // Wait for worker threads to finish
        for (auto& thread : worker_threads_) {
            if (thread && thread->joinable()) {
                thread->join();
            }
        }
        worker_threads_.clear();
        
        std::cout << "[ExecutionEngine] Shutdown complete" << std::endl;
    }
    
    bool submit_command(const ExecutionCommand& cmd) noexcept {
        if (emergency_stopped_.load() || paused_.load()) {
            return false;
        }
        
        bool success = command_queue_->try_push(cmd);
        if (!success) {
            metrics_.queue_overflows.fetch_add(1);
        }
        return success;
    }
    
    bool get_result(ExecutionResult& result) noexcept {
        return result_queue_->try_pop(result);
    }
    
    size_t submit_commands(const ExecutionCommand* commands, size_t count) noexcept {
        if (emergency_stopped_.load() || paused_.load()) {
            return 0;
        }
        
        size_t submitted = 0;
        for (size_t i = 0; i < count; ++i) {
            if (command_queue_->try_push(commands[i])) {
                submitted++;
            } else {
                metrics_.queue_overflows.fetch_add(1);
                break; // Queue full
            }
        }
        return submitted;
    }
    
    size_t get_results(ExecutionResult* results, size_t max_count) noexcept {
        size_t retrieved = 0;
        for (size_t i = 0; i < max_count; ++i) {
            if (!result_queue_->try_pop(results[i])) {
                break;
            }
            retrieved++;
        }
        return retrieved;
    }
    
    void worker_loop(size_t worker_id) {
        std::cout << "[ExecutionEngine] Worker " << worker_id << " started" << std::endl;
        
        ExecutionCommand cmd;
        ExecutionResult result;
        
        while (running_.load()) {
            if (emergency_stopped_.load() || paused_.load()) {
                std::this_thread::sleep_for(std::chrono::microseconds(100));
                continue;
            }
            
            // Try to get a command
            if (!command_queue_->try_pop(cmd)) {
                // No work available, yield briefly
                std::this_thread::yield();
                continue;
            }
            
            // Execute the command
            auto start_time = PrecisionTimer::get_nanoseconds();
            
            bool executed = execute_command(cmd, result);
            
            auto end_time = PrecisionTimer::get_nanoseconds();
            uint64_t latency_ns = end_time - start_time;
            
            // Update metrics
            metrics_.commands_processed.fetch_add(1);
            if (!executed) {
                metrics_.commands_failed.fetch_add(1);
            }
            
            update_latency_metrics(latency_ns);
            
            // Store result
            result.total_latency_ns = latency_ns;
            result.command_received_ns = start_time;
            result.order_confirmed_ns = end_time;
            
            if (!result_queue_->try_push(result)) {
                // Result queue full - this is a serious problem
                call_error_callback("Result queue overflow");
            }
            
            // Call latency callback if set
            call_latency_callback(latency_ns);
        }
        
        std::cout << "[ExecutionEngine] Worker " << worker_id << " stopped" << std::endl;
    }
    
    bool execute_command(const ExecutionCommand& cmd, ExecutionResult& result) {
        // Initialize result
        memset(&result, 0, sizeof(result));
        result.order_id = cmd.order_id;
        result.platform_id = cmd.platform_id;
        result.status = ExecutionResult::Status::FAILED;
        
        // Check for emergency stop
        if (emergency_stopped_.load()) {
            result.status = ExecutionResult::Status::CANCELLED;
            strcpy(result.transaction_hash, "EMERGENCY_STOP");
            return false;
        }
        
        // Find platform
        std::lock_guard<std::mutex> lock(platforms_mutex_);
        auto it = platforms_.find(cmd.platform_id);
        if (it == platforms_.end()) {
            strcpy(result.transaction_hash, "PLATFORM_NOT_FOUND");
            return false;
        }
        
        // Execute on platform
        bool success = it->second->execute_command(cmd, result);
        
        if (success) {
            result.status = ExecutionResult::Status::SUCCESS;
            result.executed_amount = cmd.amount;
            result.executed_price = cmd.price;
            result.actual_slippage = 0.1; // Simulate small slippage
            result.total_fees = result.executed_amount * 0.003; // 0.3% fee
            
            // Generate transaction hash
            std::hash<std::string> hasher;
            auto hash_val = hasher(std::string(cmd.token_address) + std::to_string(cmd.timestamp_ns));
            snprintf(result.transaction_hash, sizeof(result.transaction_hash), "0x%016lx", hash_val);
        }
        
        return success;
    }
    
    void update_latency_metrics(uint64_t latency_ns) {
        // Update average latency (simple moving average)
        uint64_t current_avg = metrics_.avg_latency_ns.load();
        uint64_t new_avg = (current_avg * 63 + latency_ns) / 64; // 64-sample moving average
        metrics_.avg_latency_ns.store(new_avg);
        
        // Update max latency
        uint64_t current_max = metrics_.max_latency_ns.load();
        while (latency_ns > current_max) {
            if (metrics_.max_latency_ns.compare_exchange_weak(current_max, latency_ns)) {
                break;
            }
        }
        
        last_latency_ns_.store(latency_ns);
    }
    
    void call_latency_callback(uint64_t latency_ns) {
        std::lock_guard<std::mutex> lock(callback_mutex_);
        if (latency_callback_) {
            latency_callback_(latency_ns);
        }
    }
    
    void call_error_callback(const std::string& error) {
        std::lock_guard<std::mutex> lock(callback_mutex_);
        if (error_callback_) {
            error_callback_(error);
        }
    }
    
    void setup_real_time_scheduling() {
#ifdef __APPLE__
        // macOS real-time scheduling
        mach_port_t thread_port = pthread_mach_thread_np(pthread_self());
        thread_time_constraint_policy_data_t policy;
        
        policy.period = 1000000; // 1ms in nanoseconds
        policy.computation = 500000; // 0.5ms
        policy.constraint = 1000000; // 1ms
        policy.preemptible = true;
        
        kern_return_t result = thread_policy_set(
            thread_port,
            THREAD_TIME_CONSTRAINT_POLICY,
            (thread_policy_t)&policy,
            THREAD_TIME_CONSTRAINT_POLICY_COUNT
        );
        
        if (result == KERN_SUCCESS) {
            std::cout << "[ExecutionEngine] Real-time scheduling enabled" << std::endl;
        }
#elif defined(__linux__)
        // Linux real-time scheduling
        struct sched_param param;
        param.sched_priority = 99; // Highest RT priority
        
        if (sched_setscheduler(0, SCHED_FIFO, &param) == 0) {
            std::cout << "[ExecutionEngine] Real-time scheduling enabled" << std::endl;
        }
#endif
    }
    
    void lock_memory_pages() {
#ifdef __linux__
        if (mlockall(MCL_CURRENT | MCL_FUTURE) == 0) {
            std::cout << "[ExecutionEngine] Memory pages locked" << std::endl;
        }
#elif defined(__APPLE__)
        // macOS doesn't have mlockall, but we can lock specific pages
        std::cout << "[ExecutionEngine] Memory locking attempted (limited on macOS)" << std::endl;
#endif
    }
    
    void set_thread_affinity(std::thread& thread, int cpu_core) {
#ifdef __linux__
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(cpu_core, &cpuset);
        
        pthread_t native_thread = thread.native_handle();
        if (pthread_setaffinity_np(native_thread, sizeof(cpu_set_t), &cpuset) == 0) {
            std::cout << "[ExecutionEngine] Thread affinity set to CPU " << cpu_core << std::endl;
        }
#elif defined(__APPLE__)
        // macOS thread affinity is more limited
        thread_affinity_policy_data_t policy = { cpu_core };
        mach_port_t thread_port = pthread_mach_thread_np(thread.native_handle());
        
        thread_policy_set(
            thread_port,
            THREAD_AFFINITY_POLICY,
            (thread_policy_t)&policy,
            THREAD_AFFINITY_POLICY_COUNT
        );
        
        std::cout << "[ExecutionEngine] Thread affinity set to CPU " << cpu_core << " (macOS)" << std::endl;
#endif
    }
};

UltraFastExecutionEngine::UltraFastExecutionEngine(const Config& config)
    : pimpl_(std::make_unique<ExecutionImpl>(config)) {}

UltraFastExecutionEngine::~UltraFastExecutionEngine() = default;

bool UltraFastExecutionEngine::initialize() {
    return pimpl_->initialize();
}

void UltraFastExecutionEngine::shutdown() {
    pimpl_->shutdown();
}

bool UltraFastExecutionEngine::is_running() const {
    return pimpl_->running_.load();
}

bool UltraFastExecutionEngine::add_platform(uint8_t platform_id, 
                                           std::unique_ptr<IExecutionPlatform> platform) {
    std::lock_guard<std::mutex> lock(pimpl_->platforms_mutex_);
    auto result = pimpl_->platforms_.emplace(platform_id, std::move(platform));
    return result.second;
}

void UltraFastExecutionEngine::remove_platform(uint8_t platform_id) {
    std::lock_guard<std::mutex> lock(pimpl_->platforms_mutex_);
    pimpl_->platforms_.erase(platform_id);
}

bool UltraFastExecutionEngine::submit_command(const ExecutionCommand& cmd) noexcept {
    return pimpl_->submit_command(cmd);
}

bool UltraFastExecutionEngine::get_result(ExecutionResult& result) noexcept {
    return pimpl_->get_result(result);
}

size_t UltraFastExecutionEngine::submit_commands(const ExecutionCommand* commands, size_t count) noexcept {
    return pimpl_->submit_commands(commands, count);
}

size_t UltraFastExecutionEngine::get_results(ExecutionResult* results, size_t max_count) noexcept {
    return pimpl_->get_results(results, max_count);
}

void UltraFastExecutionEngine::get_metrics(PerformanceMetrics& metrics_out) const {
    // Manually load atomic values
    metrics_out.commands_processed.store(pimpl_->metrics_.commands_processed.load());
    metrics_out.commands_failed.store(pimpl_->metrics_.commands_failed.load());
    metrics_out.avg_latency_ns.store(pimpl_->metrics_.avg_latency_ns.load());
    metrics_out.max_latency_ns.store(pimpl_->metrics_.max_latency_ns.load());
    metrics_out.queue_overflows.store(pimpl_->metrics_.queue_overflows.load());
    metrics_out.mev_attacks_detected.store(pimpl_->metrics_.mev_attacks_detected.load());
    metrics_out.emergency_stops.store(pimpl_->metrics_.emergency_stops.load());
}

void UltraFastExecutionEngine::reset_metrics() {
    pimpl_->metrics_.commands_processed.store(0);
    pimpl_->metrics_.commands_failed.store(0);
    pimpl_->metrics_.avg_latency_ns.store(0);
    pimpl_->metrics_.max_latency_ns.store(0);
    pimpl_->metrics_.queue_overflows.store(0);
    pimpl_->metrics_.mev_attacks_detected.store(0);
}

void UltraFastExecutionEngine::emergency_stop_all() {
    pimpl_->emergency_stopped_.store(true);
    pimpl_->metrics_.emergency_stops.fetch_add(1);
    std::cout << "[ExecutionEngine] 🚨 EMERGENCY STOP ACTIVATED 🚨" << std::endl;
}

void UltraFastExecutionEngine::pause_execution() {
    pimpl_->paused_.store(true);
    std::cout << "[ExecutionEngine] Execution paused" << std::endl;
}

void UltraFastExecutionEngine::resume_execution() {
    pimpl_->paused_.store(false);
    std::cout << "[ExecutionEngine] Execution resumed" << std::endl;
}

void UltraFastExecutionEngine::set_latency_callback(LatencyCallback callback) {
    std::lock_guard<std::mutex> lock(pimpl_->callback_mutex_);
    pimpl_->latency_callback_ = std::move(callback);
}

void UltraFastExecutionEngine::set_error_callback(ErrorCallback callback) {
    std::lock_guard<std::mutex> lock(pimpl_->callback_mutex_);
    pimpl_->error_callback_ = std::move(callback);
}

// ============================================================================
// Real-Time Thread Utilities Implementation
// ============================================================================

bool RealTimeThread::set_thread_priority(Priority priority) {
#ifdef __APPLE__
    mach_port_t thread_port = pthread_mach_thread_np(pthread_self());
    
    if (priority == Priority::REAL_TIME) {
        thread_time_constraint_policy_data_t policy;
        policy.period = 1000000; // 1ms
        policy.computation = 500000; // 0.5ms
        policy.constraint = 1000000; // 1ms
        policy.preemptible = true;
        
        return thread_policy_set(
            thread_port,
            THREAD_TIME_CONSTRAINT_POLICY,
            (thread_policy_t)&policy,
            THREAD_TIME_CONSTRAINT_POLICY_COUNT
        ) == KERN_SUCCESS;
    } else {
        thread_precedence_policy_data_t policy;
        policy.importance = static_cast<integer_t>(priority);
        
        return thread_policy_set(
            thread_port,
            THREAD_PRECEDENCE_POLICY,
            (thread_policy_t)&policy,
            THREAD_PRECEDENCE_POLICY_COUNT
        ) == KERN_SUCCESS;
    }
#elif defined(__linux__)
    struct sched_param param;
    int policy_type = (priority == Priority::REAL_TIME) ? SCHED_FIFO : SCHED_OTHER;
    param.sched_priority = (priority == Priority::REAL_TIME) ? 99 : static_cast<int>(priority);
    
    return sched_setscheduler(0, policy_type, &param) == 0;
#else
    return false;
#endif
}

bool RealTimeThread::set_thread_affinity(const std::vector<int>& cpu_cores) {
    if (cpu_cores.empty()) return false;
    
#ifdef __linux__
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    for (int core : cpu_cores) {
        CPU_SET(core, &cpuset);
    }
    
    return pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset) == 0;
#elif defined(__APPLE__)
    // macOS only supports setting affinity to one core
    thread_affinity_policy_data_t policy = { cpu_cores[0] };
    mach_port_t thread_port = pthread_mach_thread_np(pthread_self());
    
    return thread_policy_set(
        thread_port,
        THREAD_AFFINITY_POLICY,
        (thread_policy_t)&policy,
        THREAD_AFFINITY_POLICY_COUNT
    ) == KERN_SUCCESS;
#else
    return false;
#endif
}

bool RealTimeThread::lock_memory_pages() {
#ifdef __linux__
    return mlockall(MCL_CURRENT | MCL_FUTURE) == 0;
#else
    return false; // Not fully supported on other platforms
#endif
}

bool RealTimeThread::disable_page_faults() {
    // This is typically handled by memory locking
    return lock_memory_pages();
}

// ============================================================================
// NUMA Allocator Implementation
// ============================================================================

void* NUMAAllocator::allocate(size_t size, int numa_node) {
#ifdef __linux__
    if (numa_node >= 0 && numa_available() != -1) {
        return numa_alloc_onnode(size, numa_node);
    }
#endif
    return std::aligned_alloc(64, size); // Fallback to cache-aligned allocation
}

void NUMAAllocator::deallocate(void* ptr, size_t size) {
#ifdef __linux__
    if (numa_available() != -1) {
        numa_free(ptr, size);
        return;
    }
#endif
    std::free(ptr);
}

int NUMAAllocator::get_current_numa_node() {
#ifdef __linux__
    if (numa_available() != -1) {
        return numa_node_of_cpu(sched_getcpu());
    }
#endif
    return 0; // Default to node 0
}

int NUMAAllocator::get_optimal_numa_node_for_cpu(int cpu_core) {
#ifdef __linux__
    if (numa_available() != -1) {
        return numa_node_of_cpu(cpu_core);
    }
#endif
    return 0; // Default to node 0
}

} // namespace hfx::hft
