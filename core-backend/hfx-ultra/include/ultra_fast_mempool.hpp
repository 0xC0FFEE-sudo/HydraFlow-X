#pragma once

#include <memory>
#include <atomic>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <shared_mutex>
#include <thread>
#include <chrono>
#include <functional>
#include <queue>
#include <bitset>

// macOS compatibility
#ifdef __APPLE__
#ifndef __cpp_lib_shared_mutex
#define shared_mutex mutex
#define shared_lock lock_guard
#define unique_lock lock_guard
#endif
#endif

namespace hfx::ultra {

// Ultra-fast bloom filter for function selector filtering
class BloomFilter {
public:
    static constexpr size_t FILTER_SIZE = 4096;
    static constexpr size_t NUM_HASHES = 3;
    
    BloomFilter() : bits_(FILTER_SIZE) {}
    
    void add(uint32_t selector) {
        for (size_t i = 0; i < NUM_HASHES; ++i) {
            size_t hash = hash_func(selector, i) % FILTER_SIZE;
            bits_[hash] = true;
        }
    }
    
    bool might_contain(uint32_t selector) const {
        for (size_t i = 0; i < NUM_HASHES; ++i) {
            size_t hash = hash_func(selector, i) % FILTER_SIZE;
            if (!bits_[hash]) return false;
        }
        return true;
    }
    
private:
    std::bitset<FILTER_SIZE> bits_;
    
    size_t hash_func(uint32_t value, size_t seed) const {
        // Simple hash function optimized for speed
        return (value * 2654435761UL + seed) >> 16;
    }
};

// Critical Uniswap/DEX function selectors for ultra-fast filtering
struct DEXSelectors {
    // Uniswap V2 Router
    static constexpr uint32_t SWAP_EXACT_TOKENS_FOR_TOKENS = 0x38ed1739;
    static constexpr uint32_t SWAP_EXACT_TOKENS_FOR_ETH = 0x18cbafe5;
    static constexpr uint32_t SWAP_EXACT_ETH_FOR_TOKENS = 0x7ff36ab5;
    static constexpr uint32_t ADD_LIQUIDITY = 0xe8e33700;
    static constexpr uint32_t ADD_LIQUIDITY_ETH = 0xf305d719;
    
    // Uniswap V3 Router
    static constexpr uint32_t EXACT_INPUT_SINGLE = 0x04e45aaf;
    static constexpr uint32_t EXACT_OUTPUT_SINGLE = 0x5023b4df;
    static constexpr uint32_t EXACT_INPUT = 0x0b24c7e0;
    static constexpr uint32_t EXACT_OUTPUT = 0x09b81346;
    
    // Universal Router
    static constexpr uint32_t EXECUTE = 0x3593564c;
    
    // PancakeSwap selectors
    static constexpr uint32_t PANCAKE_SWAP_EXACT_TOKENS = 0x38ed1739;
    static constexpr uint32_t PANCAKE_ADD_LIQUIDITY = 0xe8e33700;
    
    // Jupiter (Solana) - represented as uint32 for consistency
    static constexpr uint32_t JUPITER_SWAP = 0x4e6d5bfd; // Pseudo selector
    
    static BloomFilter create_dex_filter() {
        BloomFilter filter;
        filter.add(SWAP_EXACT_TOKENS_FOR_TOKENS);
        filter.add(SWAP_EXACT_TOKENS_FOR_ETH);
        filter.add(SWAP_EXACT_ETH_FOR_TOKENS);
        filter.add(ADD_LIQUIDITY);
        filter.add(ADD_LIQUIDITY_ETH);
        filter.add(EXACT_INPUT_SINGLE);
        filter.add(EXACT_OUTPUT_SINGLE);
        filter.add(EXACT_INPUT);
        filter.add(EXACT_OUTPUT);
        filter.add(EXECUTE);
        filter.add(PANCAKE_SWAP_EXACT_TOKENS);
        filter.add(PANCAKE_ADD_LIQUIDITY);
        filter.add(JUPITER_SWAP);
        return filter;
    }
};

// Ultra-fast transaction structure (cache-aligned)
struct alignas(64) FastTransaction {
    uint64_t hash;
    uint32_t nonce;
    uint64_t gas_price;
    uint64_t max_fee_per_gas;
    uint64_t max_priority_fee_per_gas;
    uint32_t gas_limit;
    uint32_t function_selector;
    uint64_t value;
    uint64_t timestamp_ns;
    uint64_t seen_at_ns;
    mutable std::atomic<bool> processed{false};
    std::array<uint8_t, 20> from_address;
    std::array<uint8_t, 20> to_address;
    std::array<uint8_t, 4> calldata_prefix; // First 4 bytes for selector
    
    // Custom copy constructor to handle atomic member
    FastTransaction(const FastTransaction& other) 
        : hash(other.hash), nonce(other.nonce), gas_price(other.gas_price),
          max_fee_per_gas(other.max_fee_per_gas), max_priority_fee_per_gas(other.max_priority_fee_per_gas),
          gas_limit(other.gas_limit), function_selector(other.function_selector),
          value(other.value), timestamp_ns(other.timestamp_ns), seen_at_ns(other.seen_at_ns),
          processed(other.processed.load()), from_address(other.from_address),
          to_address(other.to_address), calldata_prefix(other.calldata_prefix) {}
    
    // Custom assignment operator to handle atomic member
    FastTransaction& operator=(const FastTransaction& other) {
        if (this != &other) {
            hash = other.hash;
            nonce = other.nonce;
            gas_price = other.gas_price;
            max_fee_per_gas = other.max_fee_per_gas;
            max_priority_fee_per_gas = other.max_priority_fee_per_gas;
            gas_limit = other.gas_limit;
            function_selector = other.function_selector;
            value = other.value;
            timestamp_ns = other.timestamp_ns;
            seen_at_ns = other.seen_at_ns;
            processed.store(other.processed.load());
            from_address = other.from_address;
            to_address = other.to_address;
            calldata_prefix = other.calldata_prefix;
        }
        return *this;
    }
    
    // Default constructor
    FastTransaction() = default;
    
    // Fast utility methods
    bool is_dex_transaction() const {
        static const BloomFilter dex_filter = DEXSelectors::create_dex_filter();
        return dex_filter.might_contain(function_selector);
    }
    
    bool is_high_value() const {
        return value > 1000000000000000000ULL; // > 1 ETH
    }
    
    bool is_high_gas() const {
        return max_priority_fee_per_gas > 50000000000ULL; // > 50 gwei
    }
};

// Lock-free queue for ultra-fast transaction processing
template<typename T, size_t Size = 65536>
class LockFreeQueue {
private:
    alignas(64) std::atomic<size_t> head_{0};
    alignas(64) std::atomic<size_t> tail_{0};
    alignas(64) std::array<T, Size> buffer_;
    size_t actual_size_;
    
public:
    LockFreeQueue() : actual_size_(Size) {}
    explicit LockFreeQueue(size_t size) : actual_size_(std::min(size, Size)) {}
    bool push(const T& item) {
        const size_t current_tail = tail_.load(std::memory_order_relaxed);
        const size_t next_tail = (current_tail + 1) % actual_size_;
        
        if (next_tail == head_.load(std::memory_order_acquire)) {
            return false; // Queue full
        }
        
        buffer_[current_tail] = item;
        tail_.store(next_tail, std::memory_order_release);
        return true;
    }
    
    bool pop(T& item) {
        const size_t current_head = head_.load(std::memory_order_relaxed);
        
        if (current_head == tail_.load(std::memory_order_acquire)) {
            return false; // Queue empty
        }
        
        item = buffer_[current_head];
        head_.store((current_head + 1) % actual_size_, std::memory_order_release);
        return true;
    }
    
    bool empty() const {
        return head_.load(std::memory_order_acquire) == tail_.load(std::memory_order_acquire);
    }
    
    size_t size() const {
        const size_t h = head_.load(std::memory_order_acquire);
        const size_t t = tail_.load(std::memory_order_acquire);
        return (t >= h) ? (t - h) : (actual_size_ - h + t);
    }
};

// Ultra-fast mempool configuration
struct MempoolConfig {
    // Performance settings
    bool enable_bloom_filtering = true;
    bool enable_gas_filtering = true;
    bool enable_value_filtering = true;
    size_t max_queue_size = 65536;
    
    // Filtering thresholds
    uint64_t min_gas_price = 1000000000ULL;     // 1 gwei
    uint64_t min_priority_fee = 1000000000ULL;  // 1 gwei
    uint64_t min_value = 10000000000000000ULL;  // 0.01 ETH
    
    // Processing settings
    size_t worker_threads = 4;
    bool pin_threads_to_cores = true;
    std::chrono::microseconds processing_interval{100}; // 100μs
    
    // Router allowlists (for security)
    std::unordered_set<uint64_t> allowed_routers;
    std::unordered_set<uint64_t> denied_routers;
    
    // Rate limiting
    size_t max_tx_per_sender_per_second = 100;
    size_t max_total_tx_per_second = 10000;
};

// Ultra-fast mempool monitor with sub-microsecond processing
class UltraFastMempoolMonitor {
public:
    using TransactionCallback = std::function<void(const FastTransaction&)>;
    using LiquidityCallback = std::function<void(const FastTransaction&)>;
    
    explicit UltraFastMempoolMonitor(const MempoolConfig& config);
    ~UltraFastMempoolMonitor();
    
    // Core functionality
    bool start();
    void stop();
    bool is_running() const { return running_.load(); }
    
    // Transaction processing
    bool add_transaction(const FastTransaction& tx);
    void register_transaction_callback(TransactionCallback callback);
    void register_liquidity_callback(LiquidityCallback callback);
    
    // Filtering and prioritization
    void add_priority_address(const std::array<uint8_t, 20>& address);
    void remove_priority_address(const std::array<uint8_t, 20>& address);
    void update_router_allowlist(const std::unordered_set<uint64_t>& routers);
    
    // Performance metrics
    struct Metrics {
        std::atomic<uint64_t> total_transactions{0};
        std::atomic<uint64_t> filtered_transactions{0};
        std::atomic<uint64_t> processed_transactions{0};
        std::atomic<uint64_t> avg_processing_time_ns{0};
        std::atomic<uint64_t> queue_overflows{0};
        std::atomic<double> processing_rate{0.0}; // tx/second
    };
    
    const Metrics& get_metrics() const { return metrics_; }
    
    // Advanced features for HFT
    void enable_sandwich_detection(bool enable) { sandwich_detection_ = enable; }
    void enable_frontrun_protection(bool enable) { frontrun_protection_ = enable; }
    void set_mev_threshold(uint64_t threshold) { mev_threshold_ = threshold; }
    
private:
    MempoolConfig config_;
    std::atomic<bool> running_{false};
    mutable Metrics metrics_;
    
    // Threading
    std::vector<std::thread> worker_threads_;
    std::vector<std::unique_ptr<LockFreeQueue<FastTransaction>>> work_queues_;
    
    // Filtering components
    BloomFilter dex_filter_;
    std::unordered_set<uint64_t> priority_addresses_;
    mutable std::mutex priority_mutex_;
    
    // Callbacks
    std::vector<TransactionCallback> tx_callbacks_;
    std::vector<LiquidityCallback> liquidity_callbacks_;
    mutable std::mutex callback_mutex_;
    
    // MEV protection
    std::atomic<bool> sandwich_detection_{true};
    std::atomic<bool> frontrun_protection_{true};
    std::atomic<uint64_t> mev_threshold_{1000000000000000000ULL}; // 1 ETH
    
    // Rate limiting
    std::unordered_map<uint64_t, std::atomic<uint64_t>> sender_rate_limits_;
    mutable std::mutex rate_limit_mutex_;
    
    // Internal methods
    void worker_thread(size_t thread_id);
    bool should_process_transaction(const FastTransaction& tx) const;
    void process_transaction(const FastTransaction& tx);
    bool is_rate_limited(const FastTransaction& tx);
    void update_metrics(uint64_t processing_time_ns);
    uint64_t address_to_uint64(const std::array<uint8_t, 20>& address) const;
    
    // Performance optimization helpers
    void pin_thread_to_core(size_t core_id);
    void setup_thread_priority();
    uint64_t get_timestamp_ns() const;
};

// Factory for creating optimized mempool monitors
class MempoolMonitorFactory {
public:
    static std::unique_ptr<UltraFastMempoolMonitor> create_ethereum_monitor();
    static std::unique_ptr<UltraFastMempoolMonitor> create_bsc_monitor();
    static std::unique_ptr<UltraFastMempoolMonitor> create_solana_monitor();
    static std::unique_ptr<UltraFastMempoolMonitor> create_custom_monitor(const MempoolConfig& config);
};

} // namespace hfx::ultra
