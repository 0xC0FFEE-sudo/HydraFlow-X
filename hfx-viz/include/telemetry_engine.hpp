/**
 * @file telemetry_engine.hpp
 * @brief Ultra-low latency telemetry system for HFT performance monitoring
 * @version 1.0.0
 * 
 * Captures microsecond-precision metrics from all HFT subsystems with minimal overhead.
 * Designed for real-time visualization and post-trade analysis.
 */

#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <functional>
#include <thread>
#include <mutex>

#ifdef __APPLE__
#include <mach/mach_time.h>
#endif

namespace hfx::viz {

using Timestamp = std::chrono::time_point<std::chrono::high_resolution_clock>;
using Duration = std::chrono::nanoseconds;

/**
 * @brief High-frequency metrics captured from the HFT engine
 */
struct HFTMetrics {
    // Default constructor
    HFTMetrics() = default;
    
    // Copy constructor - loads values from atomics
    HFTMetrics(const HFTMetrics& other) {
        // Copy timing metrics
        timing.market_data_latency_ns.store(other.timing.market_data_latency_ns.load());
        timing.order_execution_latency_ns.store(other.timing.order_execution_latency_ns.load());
        timing.arbitrage_detection_latency_ns.store(other.timing.arbitrage_detection_latency_ns.load());
        timing.risk_check_latency_ns.store(other.timing.risk_check_latency_ns.load());
        timing.network_round_trip_ns.store(other.timing.network_round_trip_ns.load());
        timing.memory_allocation_time_ns.store(other.timing.memory_allocation_time_ns.load());
        timing.p50_execution_latency_ns.store(other.timing.p50_execution_latency_ns.load());
        timing.p95_execution_latency_ns.store(other.timing.p95_execution_latency_ns.load());
        timing.p99_execution_latency_ns.store(other.timing.p99_execution_latency_ns.load());
        timing.p99_9_execution_latency_ns.store(other.timing.p99_9_execution_latency_ns.load());
        
        // Copy trading metrics
        trading.total_trades.store(other.trading.total_trades.load());
        trading.successful_arbitrages.store(other.trading.successful_arbitrages.load());
        trading.failed_arbitrages.store(other.trading.failed_arbitrages.load());
        trading.oracle_opportunities_detected.store(other.trading.oracle_opportunities_detected.load());
        trading.sequencer_alpha_opportunities.store(other.trading.sequencer_alpha_opportunities.load());
        trading.liquidity_breathing_cycles.store(other.trading.liquidity_breathing_cycles.load());
        trading.total_pnl_usd.store(other.trading.total_pnl_usd.load());
        trading.realized_pnl_usd.store(other.trading.realized_pnl_usd.load());
        trading.unrealized_pnl_usd.store(other.trading.unrealized_pnl_usd.load());
        trading.current_drawdown_percent.store(other.trading.current_drawdown_percent.load());
        trading.max_drawdown_percent.store(other.trading.max_drawdown_percent.load());
        
        // Copy risk metrics
        risk.current_var_usd.store(other.risk.current_var_usd.load());
        risk.position_exposure_usd.store(other.risk.position_exposure_usd.load());
        risk.circuit_breaker_triggers.store(other.risk.circuit_breaker_triggers.load());
        risk.anomalies_detected.store(other.risk.anomalies_detected.load());
        risk.sharpe_ratio.store(other.risk.sharpe_ratio.load());
        risk.sortino_ratio.store(other.risk.sortino_ratio.load());
        
        // Copy network metrics
        network.total_messages_sent.store(other.network.total_messages_sent.load());
        network.total_messages_received.store(other.network.total_messages_received.load());
        network.total_bytes_sent.store(other.network.total_bytes_sent.load());
        network.total_bytes_received.store(other.network.total_bytes_received.load());
        network.connection_drops.store(other.network.connection_drops.load());
        network.reconnections.store(other.network.reconnections.load());
        
        // Copy system metrics
        system.cpu_usage_percent.store(other.system.cpu_usage_percent.load());
        system.memory_usage_mb.store(other.system.memory_usage_mb.load());
        system.cache_hits.store(other.system.cache_hits.load());
        system.cache_misses.store(other.system.cache_misses.load());
        system.cache_hit_ratio.store(other.system.cache_hit_ratio.load());
        
        // Copy strategy metrics
        strategy.oracle_skew_signals.store(other.strategy.oracle_skew_signals.load());
        strategy.builder_inclusion_predictions.store(other.strategy.builder_inclusion_predictions.load());
        strategy.ml_model_accuracy.store(other.strategy.ml_model_accuracy.load());
        strategy.quantum_optimizations.store(other.strategy.quantum_optimizations.load());
    }
    
    // Copy assignment operator
    HFTMetrics& operator=(const HFTMetrics& other) {
        if (this != &other) {
            // Manually copy all atomic values (can't use copy constructor here)
            // Copy timing metrics
            timing.market_data_latency_ns.store(other.timing.market_data_latency_ns.load());
            timing.order_execution_latency_ns.store(other.timing.order_execution_latency_ns.load());
            timing.arbitrage_detection_latency_ns.store(other.timing.arbitrage_detection_latency_ns.load());
            timing.risk_check_latency_ns.store(other.timing.risk_check_latency_ns.load());
            timing.network_round_trip_ns.store(other.timing.network_round_trip_ns.load());
            timing.memory_allocation_time_ns.store(other.timing.memory_allocation_time_ns.load());
            timing.p50_execution_latency_ns.store(other.timing.p50_execution_latency_ns.load());
            timing.p95_execution_latency_ns.store(other.timing.p95_execution_latency_ns.load());
            timing.p99_execution_latency_ns.store(other.timing.p99_execution_latency_ns.load());
            timing.p99_9_execution_latency_ns.store(other.timing.p99_9_execution_latency_ns.load());
            
            // Copy trading metrics
            trading.total_trades.store(other.trading.total_trades.load());
            trading.successful_arbitrages.store(other.trading.successful_arbitrages.load());
            trading.failed_arbitrages.store(other.trading.failed_arbitrages.load());
            trading.oracle_opportunities_detected.store(other.trading.oracle_opportunities_detected.load());
            trading.sequencer_alpha_opportunities.store(other.trading.sequencer_alpha_opportunities.load());
            trading.liquidity_breathing_cycles.store(other.trading.liquidity_breathing_cycles.load());
            trading.total_pnl_usd.store(other.trading.total_pnl_usd.load());
            trading.realized_pnl_usd.store(other.trading.realized_pnl_usd.load());
            trading.unrealized_pnl_usd.store(other.trading.unrealized_pnl_usd.load());
            trading.current_drawdown_percent.store(other.trading.current_drawdown_percent.load());
            trading.max_drawdown_percent.store(other.trading.max_drawdown_percent.load());
            
            // Copy risk metrics
            risk.current_var_usd.store(other.risk.current_var_usd.load());
            risk.position_exposure_usd.store(other.risk.position_exposure_usd.load());
            risk.circuit_breaker_triggers.store(other.risk.circuit_breaker_triggers.load());
            risk.anomalies_detected.store(other.risk.anomalies_detected.load());
            risk.sharpe_ratio.store(other.risk.sharpe_ratio.load());
            risk.sortino_ratio.store(other.risk.sortino_ratio.load());
            
            // Copy network metrics
            network.total_messages_sent.store(other.network.total_messages_sent.load());
            network.total_messages_received.store(other.network.total_messages_received.load());
            network.total_bytes_sent.store(other.network.total_bytes_sent.load());
            network.total_bytes_received.store(other.network.total_bytes_received.load());
            network.connection_drops.store(other.network.connection_drops.load());
            network.reconnections.store(other.network.reconnections.load());
            
            // Copy system metrics
            system.cpu_usage_percent.store(other.system.cpu_usage_percent.load());
            system.memory_usage_mb.store(other.system.memory_usage_mb.load());
            system.cache_hits.store(other.system.cache_hits.load());
            system.cache_misses.store(other.system.cache_misses.load());
            system.cache_hit_ratio.store(other.system.cache_hit_ratio.load());
            
            // Copy strategy metrics
            strategy.oracle_skew_signals.store(other.strategy.oracle_skew_signals.load());
            strategy.builder_inclusion_predictions.store(other.strategy.builder_inclusion_predictions.load());
            strategy.ml_model_accuracy.store(other.strategy.ml_model_accuracy.load());
            strategy.quantum_optimizations.store(other.strategy.quantum_optimizations.load());
        }
        return *this;
    }
    
    // Move constructor and assignment are deleted because atomic types can't be moved
    HFTMetrics(HFTMetrics&& other) = delete;
    HFTMetrics& operator=(HFTMetrics&& other) = delete;
    // Timing metrics (nanosecond precision)
    struct {
        std::atomic<std::uint64_t> market_data_latency_ns{0};
        std::atomic<std::uint64_t> order_execution_latency_ns{0};
        std::atomic<std::uint64_t> arbitrage_detection_latency_ns{0};
        std::atomic<std::uint64_t> risk_check_latency_ns{0};
        std::atomic<std::uint64_t> network_round_trip_ns{0};
        std::atomic<std::uint64_t> memory_allocation_time_ns{0};
        
        // Percentile tracking (lock-free circular buffers)
        std::atomic<std::uint64_t> p50_execution_latency_ns{0};
        std::atomic<std::uint64_t> p95_execution_latency_ns{0};
        std::atomic<std::uint64_t> p99_execution_latency_ns{0};
        std::atomic<std::uint64_t> p99_9_execution_latency_ns{0};
    } timing;
    
    // Trading metrics
    struct {
        std::atomic<std::uint64_t> total_trades{0};
        std::atomic<std::uint64_t> successful_arbitrages{0};
        std::atomic<std::uint64_t> failed_arbitrages{0};
        std::atomic<std::uint64_t> oracle_opportunities_detected{0};
        std::atomic<std::uint64_t> sequencer_alpha_opportunities{0};
        std::atomic<std::uint64_t> liquidity_breathing_cycles{0};
        
        std::atomic<double> total_pnl_usd{0.0};
        std::atomic<double> realized_pnl_usd{0.0};
        std::atomic<double> unrealized_pnl_usd{0.0};
        std::atomic<double> current_drawdown_percent{0.0};
        std::atomic<double> max_drawdown_percent{0.0};
    } trading;
    
    // Risk metrics
    struct {
        std::atomic<double> current_var_usd{0.0};
        std::atomic<double> position_exposure_usd{0.0};
        std::atomic<std::uint64_t> circuit_breaker_triggers{0};
        std::atomic<std::uint64_t> anomalies_detected{0};
        std::atomic<double> sharpe_ratio{0.0};
        std::atomic<double> sortino_ratio{0.0};
    } risk;
    
    // Network metrics
    struct {
        std::atomic<std::uint64_t> total_messages_sent{0};
        std::atomic<std::uint64_t> total_messages_received{0};
        std::atomic<std::uint64_t> total_bytes_sent{0};
        std::atomic<std::uint64_t> total_bytes_received{0};
        std::atomic<std::uint64_t> connection_drops{0};
        std::atomic<std::uint64_t> reconnections{0};
    } network;
    
    // System metrics
    struct {
        std::atomic<double> cpu_usage_percent{0.0};
        std::atomic<std::uint64_t> memory_usage_mb{0};
        std::atomic<std::uint64_t> cache_hits{0};
        std::atomic<std::uint64_t> cache_misses{0};
        std::atomic<double> cache_hit_ratio{0.0};
    } system;
    
    // Strategy-specific metrics
    struct {
        std::atomic<std::uint64_t> oracle_skew_signals{0};
        std::atomic<std::uint64_t> builder_inclusion_predictions{0};
        std::atomic<double> ml_model_accuracy{0.0};
        std::atomic<std::uint64_t> quantum_optimizations{0};
    } strategy;
};

/**
 * @brief Real-time market state visualization data
 */
struct MarketState {
    struct TokenPrice {
        std::string symbol;
        double price_usd{0.0};
        double change_24h_percent{0.0};
        double volume_24h_usd{0.0};
        Timestamp last_updated;
    };
    
    std::vector<TokenPrice> token_prices;
    
    struct ArbitrageOpportunity {
        std::string pair;
        double price_diff_percent{0.0};
        double potential_profit_usd{0.0};
        std::string source_exchange;
        std::string dest_exchange;
        Timestamp detected_at;
        Duration window_remaining;
    };
    
    std::vector<ArbitrageOpportunity> active_opportunities;
    
    struct GasMarket {
        double standard_gwei{0.0};
        double fast_gwei{0.0};
        double instant_gwei{0.0};
        double next_base_fee{0.0};
    } gas_market;
};

/**
 * @brief Event-driven telemetry callback system
 */
using TelemetryCallback = std::function<void(const HFTMetrics&, const MarketState&)>;

/**
 * @class TelemetryEngine
 * @brief Lock-free, ultra-low latency telemetry collection system
 */
class TelemetryEngine {
public:
    TelemetryEngine();
    ~TelemetryEngine();
    
    // Core telemetry operations
    void start();
    void stop();
    void reset_metrics();
    
    // Metric recording (lock-free, < 10ns overhead)
    void record_latency(const std::string& metric_name, Duration latency);
    void record_trade(double pnl_usd, bool successful);
    void record_arbitrage_opportunity(const std::string& pair, double profit_usd);
    void record_risk_metric(const std::string& name, double value);
    void record_network_activity(std::uint64_t bytes_sent, std::uint64_t bytes_received);
    void record_system_resource(double cpu_percent, std::uint64_t memory_mb);
    
    // Market state updates
    void update_token_price(const std::string& symbol, double price_usd, double change_percent);
    void update_gas_market(double standard, double fast, double instant);
    
    // Data access (thread-safe)
    const HFTMetrics& get_metrics() const { return metrics_; }
    const MarketState& get_market_state() const { return market_state_; }
    
    // Real-time streaming
    void register_callback(TelemetryCallback callback);
    void set_update_frequency(Duration frequency);
    
    // Performance snapshot for visualization
    struct PerformanceSnapshot {
        Timestamp timestamp;
        HFTMetrics metrics;
        MarketState market_state;
        
        // Derived analytics
        double trades_per_second{0.0};
        double avg_latency_ms{0.0};
        double profit_per_trade_usd{0.0};
        double win_rate_percent{0.0};
    };
    
    PerformanceSnapshot get_snapshot() const;
    
    // Historical data for charts (circular buffer)
    std::vector<PerformanceSnapshot> get_history(Duration lookback_period) const;
    
private:
    mutable HFTMetrics metrics_;
    mutable MarketState market_state_;
    
    std::vector<TelemetryCallback> callbacks_;
    std::atomic<bool> running_{false};
    std::atomic<Duration> update_frequency_{std::chrono::milliseconds(100)};
    
    // High-frequency circular buffer for historical data
    static constexpr std::size_t HISTORY_SIZE = 10000;
    mutable std::vector<PerformanceSnapshot> history_buffer_;
    mutable std::atomic<std::size_t> history_index_{0};
    
    // Worker thread for callbacks
    std::unique_ptr<std::thread> worker_thread_;
    
    void worker_loop();
    
    // Platform-specific high-precision timing
    static inline std::uint64_t get_timestamp_ns() noexcept {
#ifdef __APPLE__
        static mach_timebase_info_data_t timebase_info = {0, 0};
        if (timebase_info.denom == 0) {
            mach_timebase_info(&timebase_info);
        }
        return mach_absolute_time() * timebase_info.numer / timebase_info.denom;
#else
        return std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch()).count();
#endif
    }
};

} // namespace hfx::viz
