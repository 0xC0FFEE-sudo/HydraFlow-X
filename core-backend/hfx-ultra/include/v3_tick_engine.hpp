#pragma once

#include <memory>
#include <vector>
#include <unordered_map>
#include <string>
#include <cmath>
#include <atomic>
#include <mutex>
#include <chrono>
#include <thread>
#include <random>

namespace hfx::ultra {

// Using 128-bit integers for precise calculations (simplified representation)
using uint256_t = __uint128_t;
using int256_t = __int128_t;
using uint128_t = __uint128_t;
using int128_t = __int128_t;
using uint24_t = uint32_t;  // Use 32-bit for 24-bit values
using int24_t = int32_t;    // Use 32-bit for 24-bit values  
using uint160_t = uint64_t; // Simplified for 160-bit values
using int56_t = int64_t;    // Simplified for 56-bit values

// Uniswap V3 math constants
constexpr uint256_t Q96 = uint256_t(1) << 96;
constexpr uint256_t Q128 = uint256_t(1) << 64; // Simplified for 128-bit compatibility

// Tick range constants
constexpr int32_t MIN_TICK = -887272;
constexpr int32_t MAX_TICK = 887272;

// V3 fee tier configuration
struct V3FeeTier {
    uint24_t fee;
    int32_t tick_spacing;
    uint32_t observation_cardinality;
    
    V3FeeTier() = default;
    V3FeeTier(uint24_t f, int32_t ts, uint32_t oc) : fee(f), tick_spacing(ts), observation_cardinality(oc) {}
};

// V3 pool state structure (cache-aligned for performance)
struct alignas(64) V3PoolState {
    uint256_t sqrt_price_x96;     // Current sqrt price in Q64.96 format
    int24_t tick;                 // Current tick
    uint16_t observation_index;   // Current observation index
    uint16_t observation_cardinality; // Number of observations
    uint16_t observation_cardinality_next; // Next observation cardinality
    uint8_t fee_protocol;         // Fee protocol
    bool unlocked;                // Pool lock status
    uint128_t liquidity;          // Current liquidity
    uint256_t fee_growth_global_0_x128; // Fee growth for token0
    uint256_t fee_growth_global_1_x128; // Fee growth for token1
    uint128_t protocol_fees_token_0;
    uint128_t protocol_fees_token_1;
    
    // Pool configuration
    std::string pool_address;     // Pool contract address
    std::string token0;           // Token0 address (for compatibility)
    std::string token1;           // Token1 address (for compatibility)  
    std::string token0_address;
    std::string token1_address;
    uint24_t fee;                 // Fee tier (500, 3000, 10000)
    int24_t tick_spacing;         // Tick spacing
    
    // Performance metadata
    uint64_t last_updated_ns;
    mutable std::atomic<uint32_t> update_sequence{0};
    
    // Copy constructor and assignment operator for atomic member
    V3PoolState() = default;
    V3PoolState(const V3PoolState& other) 
        : sqrt_price_x96(other.sqrt_price_x96), tick(other.tick),
          observation_index(other.observation_index), observation_cardinality(other.observation_cardinality),
          observation_cardinality_next(other.observation_cardinality_next), fee_protocol(other.fee_protocol),
          unlocked(other.unlocked), liquidity(other.liquidity),
          fee_growth_global_0_x128(other.fee_growth_global_0_x128), fee_growth_global_1_x128(other.fee_growth_global_1_x128),
          protocol_fees_token_0(other.protocol_fees_token_0), protocol_fees_token_1(other.protocol_fees_token_1),
          pool_address(other.pool_address), token0(other.token0), token1(other.token1),
          token0_address(other.token0_address), token1_address(other.token1_address),
          fee(other.fee), tick_spacing(other.tick_spacing), last_updated_ns(other.last_updated_ns),
          update_sequence(other.update_sequence.load()) {}
    
    V3PoolState& operator=(const V3PoolState& other) {
        if (this != &other) {
            sqrt_price_x96 = other.sqrt_price_x96;
            tick = other.tick;
            observation_index = other.observation_index;
            observation_cardinality = other.observation_cardinality;
            observation_cardinality_next = other.observation_cardinality_next;
            fee_protocol = other.fee_protocol;
            unlocked = other.unlocked;
            liquidity = other.liquidity;
            fee_growth_global_0_x128 = other.fee_growth_global_0_x128;
            fee_growth_global_1_x128 = other.fee_growth_global_1_x128;
            protocol_fees_token_0 = other.protocol_fees_token_0;
            protocol_fees_token_1 = other.protocol_fees_token_1;
            token0_address = other.token0_address;
            token1_address = other.token1_address;
            fee = other.fee;
            tick_spacing = other.tick_spacing;
            last_updated_ns = other.last_updated_ns;
            update_sequence.store(other.update_sequence.load());
        }
        return *this;
    }
};

// Individual tick data
struct TickData {
    uint128_t liquidity_gross;    // Total liquidity at this tick
    int128_t liquidity_net;       // Net liquidity change at this tick
    uint256_t fee_growth_outside_0_x128;
    uint256_t fee_growth_outside_1_x128;
    int56_t tick_cumulative_outside;
    uint160_t seconds_per_liquidity_outside_x128;
    uint32_t seconds_outside;
    bool initialized;
    
    TickData() : liquidity_gross(0), liquidity_net(0), 
                fee_growth_outside_0_x128(0), fee_growth_outside_1_x128(0),
                tick_cumulative_outside(0), seconds_per_liquidity_outside_x128(0),
                seconds_outside(0), initialized(false) {}
};

// Swap step result for internal calculations
struct SwapStepResult {
    uint256_t sqrt_price_start_x96;
    uint256_t sqrt_price_next_x96;
    uint256_t amount_in;
    uint256_t amount_out;
    uint256_t fee_amount;
    bool sqrt_price_next_initialized;
};

// Complete route through multiple V3 pools
struct V3Route {
    struct Hop {
        std::string pool_address;
        std::string token_in;
        std::string token_out;
        uint24_t fee;
        bool zero_for_one; // Direction of swap
    };
    
    std::vector<Hop> hops;
    uint256_t expected_amount_out;
    uint256_t minimum_amount_out;
    uint256_t estimated_gas;
    uint256_t price_impact_bps; // Basis points
    std::chrono::nanoseconds computation_time;
};

// Configuration for V3 calculations
struct V3EngineConfig {
    // Calculation precision
    uint32_t max_tick_iterations = 1000;
    uint256_t min_sqrt_ratio = 4295128739ULL;     // sqrt(2^-64) approximation
    uint256_t max_sqrt_ratio = (uint256_t(1) << 96) - 1; // Practical maximum for 128-bit
    
    // Route finding
    uint32_t max_hops = 3;
    uint32_t max_routes_per_pair = 5;
    uint256_t min_liquidity_threshold = 1000000; // Minimum liquidity to consider
    
    // Performance settings
    bool enable_parallel_computation = true;
    bool cache_tick_data = true;
    std::chrono::milliseconds cache_ttl{500};
    uint32_t worker_threads = 4;
    
    // Slippage protection
    uint256_t max_price_impact_bps = 500; // 5%
    bool enable_sandwich_detection = true;
};

// Ultra-fast Uniswap V3 tick-walk engine
class V3TickEngine {
public:
    explicit V3TickEngine(const V3EngineConfig& config);
    ~V3TickEngine();
    
    // Core V3 calculations
    uint256_t get_amount_out(uint256_t amount_in,
                            uint256_t reserve_in,
                            uint256_t reserve_out,
                            uint24_t fee) const;
    
    uint256_t get_amount_in(uint256_t amount_out,
                           uint256_t reserve_in,
                           uint256_t reserve_out,
                           uint24_t fee) const;
    
    // Advanced V3 tick walking
    struct SwapResult {
        uint256_t amount_out;
        uint256_t amount_in_used;
        uint256_t fee_paid;
        int24_t final_tick;
        uint256_t final_sqrt_price;
        uint256_t price_impact_bps;
        std::vector<int24_t> ticks_crossed;
        bool successful;
    };
    
    SwapResult simulate_v3_swap(const std::string& pool_address,
                               uint256_t amount_in,
                               bool zero_for_one,
                               uint256_t sqrt_price_limit_x96 = 0);
    
    // Multi-hop routing
    std::vector<V3Route> find_optimal_routes(const std::string& token_in,
                                            const std::string& token_out,
                                            uint256_t amount_in,
                                            uint32_t max_routes = 3);
    
    V3Route find_best_single_route(const std::string& token_in,
                                  const std::string& token_out,
                                  uint256_t amount_in);
    
    // Pool state management
    bool update_pool_state(const std::string& pool_address, const V3PoolState& state);
    V3PoolState get_pool_state(const std::string& pool_address) const;
    
    // Tick data management
    bool update_tick_data(const std::string& pool_address, int24_t tick, const TickData& data);
    TickData get_tick_data(const std::string& pool_address, int24_t tick) const;
    
    // Utility methods
    static int24_t get_tick_at_sqrt_ratio(uint256_t sqrt_price_x96);
    static uint256_t get_sqrt_ratio_at_tick(int24_t tick);
    static uint256_t mul_div(uint256_t a, uint256_t b, uint256_t denominator);
    
    // Performance metrics
    struct Metrics {
        std::atomic<uint64_t> total_calculations{0};
        std::atomic<uint64_t> cache_hits{0};
        std::atomic<uint64_t> cache_misses{0};
        std::atomic<double> avg_calculation_time_us{0.0};
        std::atomic<uint64_t> routes_computed{0};
        std::atomic<uint64_t> ticks_processed{0};
    };
    
    const Metrics& get_metrics() const { return metrics_; }
    void reset_metrics();
    
private:
    V3EngineConfig config_;
    mutable Metrics metrics_;
    
    // State storage
    std::unordered_map<std::string, V3PoolState> pool_states_;
    std::unordered_map<std::string, std::unordered_map<int24_t, TickData>> tick_data_;
    mutable std::mutex state_mutex_;
    
    // Caching for performance
    struct CachedResult {
        uint256_t amount_out;
        std::chrono::steady_clock::time_point cached_at;
        uint32_t state_sequence;
    };
    
    std::unordered_map<std::string, CachedResult> calculation_cache_;
    mutable std::mutex cache_mutex_;
    
    // Worker threads for parallel computation
    std::vector<std::thread> worker_threads_;
    std::atomic<bool> running_{false};
    
    // Internal calculation methods
    SwapStepResult compute_swap_step(uint256_t sqrt_ratio_current_x96,
                                   uint256_t sqrt_ratio_target_x96,
                                   uint128_t liquidity,
                                   uint256_t amount_remaining,
                                   uint24_t fee) const;
    
    std::pair<int24_t, bool> next_initialized_tick_within_one_word(
        const std::string& pool_address,
        int24_t tick,
        int24_t tick_spacing,
        bool lte) const;
    
    uint256_t get_amount_0_delta(uint256_t sqrt_ratio_a_x96,
                                uint256_t sqrt_ratio_b_x96,
                                uint128_t liquidity) const;
    
    uint256_t get_amount_1_delta(uint256_t sqrt_ratio_a_x96,
                                uint256_t sqrt_ratio_b_x96,
                                uint128_t liquidity) const;
    
    // Route optimization
    double calculate_route_score(const V3Route& route,
                               uint256_t amount_in,
                               uint256_t gas_price) const;
    
    bool is_route_profitable(const V3Route& route,
                           uint256_t amount_in,
                           uint256_t gas_price,
                           uint256_t min_profit_bps) const;
    
    // Cache management
    bool is_cache_valid(const CachedResult& result, uint32_t current_sequence) const;
    void cleanup_cache();
    std::string make_cache_key(const std::string& pool, uint256_t amount, bool direction) const;
    
    // Mathematical helpers
    static uint256_t sqrt(uint256_t x);
    static uint256_t most_significant_bit(uint256_t x);
    static int256_t to_int256(uint256_t x);
    static uint256_t to_uint256(int256_t x);
    
    // V3 specific helper methods
    void initializeCommonPools();
    uint160_t calculateSqrtPriceX96(double price) const;
    int32_t priceToTick(double price) const;
    double tickToPrice(int32_t tick) const;
    int32_t getNextInitializedTick(const std::string& pool_address, int32_t current_tick, bool zero_for_one) const;
    uint128_t getLiquidityForTick(const std::string& pool_address, int32_t tick) const;
    SwapResult simulateSwap(const std::string& pool_address, uint256_t amount_in, bool zero_for_one) const;
    uint256_t calculateAmountOut(uint256_t amount_in, uint128_t liquidity, bool zero_for_one) const;
    
    // Additional member variables
    std::unordered_map<uint24_t, int32_t> tick_spacings_;
    std::unordered_map<uint24_t, V3FeeTier> fee_tiers_;
    uint160_t sqrt_price_x96_min_;
    uint160_t sqrt_price_x96_max_;
    mutable std::mt19937 random_generator_;
    
    // Threading helpers
    void start_workers();
    void stop_workers();
    void worker_thread(size_t thread_id);
};

// Advanced V3 price oracle using tick data
class V3PriceOracle {
public:
    struct PriceData {
        uint256_t price;           // Current price
        uint256_t price_1h_ago;    // Price 1 hour ago
        uint256_t price_24h_ago;   // Price 24 hours ago
        uint256_t volatility;      // Historical volatility
        uint256_t volume_24h;      // 24h volume
        int24_t current_tick;
        uint256_t liquidity;
        std::chrono::system_clock::time_point last_update;
    };
    
    explicit V3PriceOracle(std::shared_ptr<V3TickEngine> engine);
    
    PriceData get_price_data(const std::string& token0,
                           const std::string& token1,
                           uint24_t fee) const;
    
    uint256_t get_twap_price(const std::string& token0,
                           const std::string& token1,
                           uint24_t fee,
                           uint32_t period_seconds) const;
    
    bool is_price_manipulation_detected(const std::string& pool_address,
                                      uint256_t current_price,
                                      double threshold_pct = 5.0) const;
    
private:
    std::shared_ptr<V3TickEngine> engine_;
    std::unordered_map<std::string, PriceData> price_cache_;
    mutable std::mutex cache_mutex_;
};

// Factory for creating optimized V3 engines for different chains
class V3EngineFactory {
public:
    static std::unique_ptr<V3TickEngine> create_ethereum_engine();
    static std::unique_ptr<V3TickEngine> create_arbitrum_engine();
    static std::unique_ptr<V3TickEngine> create_optimism_engine();
    static std::unique_ptr<V3TickEngine> create_polygon_engine();
    static std::unique_ptr<V3TickEngine> create_custom_engine(const V3EngineConfig& config);
    
    // Pool discovery utilities
    static std::vector<std::string> discover_v3_pools(const std::string& token0,
                                                     const std::string& token1);
    
    static uint24_t get_optimal_fee_tier(const std::string& token0,
                                       const std::string& token1,
                                       uint256_t amount);
};

} // namespace hfx::ultra
