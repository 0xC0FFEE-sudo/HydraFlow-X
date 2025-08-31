#include "ultra_fast_mempool.hpp"
#include <algorithm>
#include <chrono>
#include <thread>
#include <pthread.h>

#ifdef __linux__
#include <sched.h>
#endif

#ifdef __APPLE__
#include <mach/thread_policy.h>
#include <mach/thread_act.h>
#include <mach/mach.h>
#endif

namespace hfx::ultra {

UltraFastMempoolMonitor::UltraFastMempoolMonitor(const MempoolConfig& config)
    : config_(config)
    , dex_filter_(DEXSelectors::create_dex_filter())
{
    // Pre-allocate work queues for each worker thread
    work_queues_.reserve(config_.worker_threads);
    for (size_t i = 0; i < config_.worker_threads; ++i) {
        work_queues_.emplace_back(std::make_unique<LockFreeQueue<FastTransaction>>(config_.max_queue_size));
    }
}

UltraFastMempoolMonitor::~UltraFastMempoolMonitor() {
    stop();
}

bool UltraFastMempoolMonitor::start() {
    if (running_.load()) {
        return false; // Already running
    }
    
    running_.store(true);
    
    // Start worker threads
    worker_threads_.reserve(config_.worker_threads);
    for (size_t i = 0; i < config_.worker_threads; ++i) {
        worker_threads_.emplace_back([this, i]() {
            worker_thread(i);
        });
    }
    
    return true;
}

void UltraFastMempoolMonitor::stop() {
    if (!running_.load()) {
        return;
    }
    
    running_.store(false);
    
    // Wait for all worker threads to finish
    for (auto& thread : worker_threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    
    worker_threads_.clear();
}

bool UltraFastMempoolMonitor::add_transaction(const FastTransaction& tx) {
    if (!running_.load()) {
        return false;
    }
    
    // Quick pre-filtering
    if (!should_process_transaction(tx)) {
        metrics_.filtered_transactions.fetch_add(1);
        return false;
    }
    
    // Rate limiting check
    if (is_rate_limited(tx)) {
        return false;
    }
    
    // Distribute to worker threads using round-robin
    static std::atomic<size_t> round_robin_counter{0};
    size_t worker_id = round_robin_counter.fetch_add(1) % config_.worker_threads;
    
    if (!work_queues_[worker_id]->push(tx)) {
        metrics_.queue_overflows.fetch_add(1);
        return false;
    }
    
    metrics_.total_transactions.fetch_add(1);
    return true;
}

void UltraFastMempoolMonitor::worker_thread(size_t thread_id) {
    // Pin thread to specific core for consistent performance
    if (config_.pin_threads_to_cores) {
        pin_thread_to_core(thread_id);
    }
    
    // Set high priority for real-time performance
    setup_thread_priority();
    
    FastTransaction tx;
    auto& queue = *work_queues_[thread_id];
    
    while (running_.load()) {
        if (queue.pop(tx)) {
            auto start_time = get_timestamp_ns();
            
            process_transaction(tx);
            
            auto end_time = get_timestamp_ns();
            update_metrics(end_time - start_time);
        } else {
            // Use a very short sleep to avoid busy-waiting while maintaining low latency
            std::this_thread::sleep_for(config_.processing_interval);
        }
    }
}

bool UltraFastMempoolMonitor::should_process_transaction(const FastTransaction& tx) const {
    // Gas price filtering
    if (config_.enable_gas_filtering) {
        if (tx.max_fee_per_gas < config_.min_gas_price ||
            tx.max_priority_fee_per_gas < config_.min_priority_fee) {
            return false;
        }
    }
    
    // Value filtering
    if (config_.enable_value_filtering && tx.value < config_.min_value) {
        // Allow DEX transactions even with low value (could be swaps)
        if (!tx.is_dex_transaction()) {
            return false;
        }
    }
    
    // Bloom filter for DEX transactions (ultra-fast filtering)
    if (config_.enable_bloom_filtering && !tx.is_dex_transaction()) {
        return false;
    }
    
    // Router allowlist/denylist check
    uint64_t router_hash = address_to_uint64(tx.to_address);
    if (!config_.allowed_routers.empty()) {
        if (config_.allowed_routers.find(router_hash) == config_.allowed_routers.end()) {
            return false;
        }
    }
    
    if (config_.denied_routers.find(router_hash) != config_.denied_routers.end()) {
        return false;
    }
    
    return true;
}

void UltraFastMempoolMonitor::process_transaction(const FastTransaction& tx) {
    // Mark as processed atomically (mutable atomic allows modification of const object)
    bool expected = false;
    if (!tx.processed.compare_exchange_strong(expected, true)) {
        return; // Already processed
    }
    
    metrics_.processed_transactions.fetch_add(1);
    
    // Execute callbacks based on transaction type
    {
        std::lock_guard<std::mutex> lock(callback_mutex_);
        
        // Check if this is a liquidity event (ADD_LIQUIDITY functions)
        if (tx.function_selector == DEXSelectors::ADD_LIQUIDITY ||
            tx.function_selector == DEXSelectors::ADD_LIQUIDITY_ETH ||
            tx.function_selector == DEXSelectors::PANCAKE_ADD_LIQUIDITY) {
            
            for (const auto& callback : liquidity_callbacks_) {
                callback(tx);
            }
        }
        
        // General transaction callbacks
        for (const auto& callback : tx_callbacks_) {
            callback(tx);
        }
    }
}

bool UltraFastMempoolMonitor::is_rate_limited(const FastTransaction& tx) {
    auto now = std::chrono::steady_clock::now().time_since_epoch().count();
    uint64_t sender_hash = address_to_uint64(tx.from_address);
    
    std::lock_guard<std::mutex> lock(rate_limit_mutex_);
    
    auto it = sender_rate_limits_.find(sender_hash);
    if (it != sender_rate_limits_.end()) {
        // Simple rate limiting: if we've seen this sender recently, potentially rate limit
        // This is a simplified implementation - production would use token bucket or sliding window
        uint64_t last_seen = it->second.load();
        if (now - last_seen < 1000000000ULL) { // Less than 1 second
            return true;
        }
    }
    
    // Update last seen time
    sender_rate_limits_[sender_hash].store(now);
    return false;
}

void UltraFastMempoolMonitor::update_metrics(uint64_t processing_time_ns) {
    // Update average processing time using exponential moving average
    uint64_t current_avg = metrics_.avg_processing_time_ns.load();
    uint64_t new_avg = (current_avg * 7 + processing_time_ns) / 8; // 7/8 weight to historical
    metrics_.avg_processing_time_ns.store(new_avg);
    
    // Update processing rate (simplified)
    static auto last_update = std::chrono::steady_clock::now();
    static std::atomic<uint64_t> processed_since_last{0};
    
    processed_since_last.fetch_add(1);
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_update).count();
    
    if (elapsed >= 1000) { // Update every second
        double rate = processed_since_last.load() * 1000.0 / elapsed;
        metrics_.processing_rate.store(rate);
        processed_since_last.store(0);
        last_update = now;
    }
}

uint64_t UltraFastMempoolMonitor::address_to_uint64(const std::array<uint8_t, 20>& address) const {
    // Convert first 8 bytes of address to uint64 for fast lookups
    uint64_t result = 0;
    for (size_t i = 0; i < 8 && i < address.size(); ++i) {
        result = (result << 8) | address[i];
    }
    return result;
}

void UltraFastMempoolMonitor::pin_thread_to_core(size_t core_id) {
#ifdef __linux__
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id % std::thread::hardware_concurrency(), &cpuset);
    pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
#elif defined(__APPLE__)
    // macOS thread affinity
    thread_affinity_policy_data_t policy = { static_cast<integer_t>(core_id) };
    thread_policy_set(mach_thread_self(), THREAD_AFFINITY_POLICY,
                     (thread_policy_t)&policy, THREAD_AFFINITY_POLICY_COUNT);
#endif
}

void UltraFastMempoolMonitor::setup_thread_priority() {
#ifdef __linux__
    // Set real-time priority for ultra-low latency
    struct sched_param param;
    param.sched_priority = 80; // High priority (1-99 range)
    pthread_setschedparam(pthread_self(), SCHED_FIFO, &param);
#elif defined(__APPLE__)
    // macOS thread priority
    struct sched_param param;
    param.sched_priority = 47; // High priority
    pthread_setschedparam(pthread_self(), SCHED_RR, &param);
#endif
}

uint64_t UltraFastMempoolMonitor::get_timestamp_ns() const {
    return std::chrono::high_resolution_clock::now().time_since_epoch().count();
}

void UltraFastMempoolMonitor::register_transaction_callback(TransactionCallback callback) {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    tx_callbacks_.emplace_back(std::move(callback));
}

void UltraFastMempoolMonitor::register_liquidity_callback(LiquidityCallback callback) {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    liquidity_callbacks_.emplace_back(std::move(callback));
}

void UltraFastMempoolMonitor::add_priority_address(const std::array<uint8_t, 20>& address) {
    std::lock_guard<std::mutex> lock(priority_mutex_);
    priority_addresses_.insert(address_to_uint64(address));
}

void UltraFastMempoolMonitor::remove_priority_address(const std::array<uint8_t, 20>& address) {
    std::lock_guard<std::mutex> lock(priority_mutex_);
    priority_addresses_.erase(address_to_uint64(address));
}

void UltraFastMempoolMonitor::update_router_allowlist(const std::unordered_set<uint64_t>& routers) {
    // This would require making config_ mutable or having a separate mutable configuration
    // For now, this is a placeholder for the interface
}

// Factory implementations for different networks
std::unique_ptr<UltraFastMempoolMonitor> MempoolMonitorFactory::create_ethereum_monitor() {
    MempoolConfig config;
    config.min_gas_price = 1000000000ULL;      // 1 gwei
    config.min_priority_fee = 1000000000ULL;   // 1 gwei
    config.min_value = 10000000000000000ULL;   // 0.01 ETH
    config.worker_threads = 4;
    config.enable_bloom_filtering = true;
    config.enable_gas_filtering = true;
    config.enable_value_filtering = true;
    
    // Add known Ethereum routers to allowlist (using first 8 bytes for hash)
    config.allowed_routers = {
        0x7a250d5630B4cF53ULL, // Uniswap V2 Router (truncated)
        0xE592427A0AEce92DULL, // Uniswap V3 Router (truncated)
        0x10ED43C718714eb6ULL, // PancakeSwap Router (truncated)
    };
    
    return std::make_unique<UltraFastMempoolMonitor>(config);
}

std::unique_ptr<UltraFastMempoolMonitor> MempoolMonitorFactory::create_bsc_monitor() {
    MempoolConfig config;
    config.min_gas_price = 5000000000ULL;      // 5 gwei
    config.min_priority_fee = 1000000000ULL;   // 1 gwei
    config.min_value = 1000000000000000ULL;    // 0.001 BNB
    config.worker_threads = 6;
    config.processing_interval = std::chrono::microseconds(50); // Faster for BSC
    
    // BSC-specific routers (using first 8 bytes for hash)
    config.allowed_routers = {
        0x10ED43C718714eb6ULL, // PancakeSwap V2 (truncated)
        0x1b81D678ffb9C026ULL, // PancakeSwap V3 (truncated)
    };
    
    return std::make_unique<UltraFastMempoolMonitor>(config);
}

std::unique_ptr<UltraFastMempoolMonitor> MempoolMonitorFactory::create_solana_monitor() {
    MempoolConfig config;
    config.min_gas_price = 5000ULL;            // 0.005 SOL
    config.min_priority_fee = 1000ULL;         // 0.001 SOL  
    config.min_value = 1000000ULL;             // 0.001 SOL
    config.worker_threads = 8;                 // More threads for Solana's higher TPS
    config.processing_interval = std::chrono::microseconds(25); // Very fast for Solana
    
    // Solana program IDs (represented as uint64 hashes)
    config.allowed_routers = {
        0x2c2eb13c4bb46dc8, // Jupiter
        0x675d3c43d6e161b9, // Raydium
        0x9c2c2eb13c4bb46d, // Orca
    };
    
    return std::make_unique<UltraFastMempoolMonitor>(config);
}

std::unique_ptr<UltraFastMempoolMonitor> MempoolMonitorFactory::create_custom_monitor(const MempoolConfig& config) {
    return std::make_unique<UltraFastMempoolMonitor>(config);
}

} // namespace hfx::ultra
