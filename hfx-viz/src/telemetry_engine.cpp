/**
 * @file telemetry_engine.cpp
 * @brief Implementation of ultra-low latency telemetry system
 */

#include "telemetry_engine.hpp"
#include <algorithm>
#include <thread>
#include <iostream>
#include <cstring>

#ifdef __APPLE__
#include <sys/sysctl.h>
#include <mach/mach.h>
#include <mach/vm_statistics.h>
#endif

namespace hfx::viz {

TelemetryEngine::TelemetryEngine() {
    // Pre-allocate history buffer for lock-free operation
    history_buffer_.resize(HISTORY_SIZE);
    
    // Initialize all metrics to zero
    std::memset(&metrics_, 0, sizeof(metrics_));
    
    std::cout << "[TelemetryEngine] Initialized with " << HISTORY_SIZE 
              << " sample history buffer\n";
}

TelemetryEngine::~TelemetryEngine() {
    stop();
}

void TelemetryEngine::start() {
    if (running_.load(std::memory_order_acquire)) {
        return;
    }
    
    running_.store(true, std::memory_order_release);
    
    // Start worker thread for callbacks and system monitoring
    worker_thread_ = std::make_unique<std::thread>(&TelemetryEngine::worker_loop, this);
    
    std::cout << "[TelemetryEngine] Started with update frequency: " 
              << std::chrono::duration_cast<std::chrono::milliseconds>(
                     update_frequency_.load()).count() << "ms\n";
}

void TelemetryEngine::stop() {
    if (!running_.load(std::memory_order_acquire)) {
        return;
    }
    
    running_.store(false, std::memory_order_release);
    
    if (worker_thread_ && worker_thread_->joinable()) {
        worker_thread_->join();
    }
    
    std::cout << "[TelemetryEngine] Stopped\n";
}

void TelemetryEngine::reset_metrics() {
    // Atomic reset of all metrics
    metrics_.timing.market_data_latency_ns.store(0);
    metrics_.timing.order_execution_latency_ns.store(0);
    metrics_.timing.arbitrage_detection_latency_ns.store(0);
    metrics_.timing.risk_check_latency_ns.store(0);
    metrics_.timing.network_round_trip_ns.store(0);
    
    metrics_.trading.total_trades.store(0);
    metrics_.trading.successful_arbitrages.store(0);
    metrics_.trading.failed_arbitrages.store(0);
    metrics_.trading.total_pnl_usd.store(0.0);
    
    metrics_.risk.current_var_usd.store(0.0);
    metrics_.risk.position_exposure_usd.store(0.0);
    metrics_.risk.circuit_breaker_triggers.store(0);
    
    metrics_.network.total_messages_sent.store(0);
    metrics_.network.total_messages_received.store(0);
    metrics_.network.total_bytes_sent.store(0);
    metrics_.network.total_bytes_received.store(0);
    
    std::cout << "[TelemetryEngine] All metrics reset\n";
}

void TelemetryEngine::record_latency(const std::string& metric_name, Duration latency) {
    const auto latency_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(latency).count();
    
    // Route to appropriate metric (lock-free)
    if (metric_name == "market_data") {
        metrics_.timing.market_data_latency_ns.store(latency_ns, std::memory_order_relaxed);
    } else if (metric_name == "order_execution") {
        metrics_.timing.order_execution_latency_ns.store(latency_ns, std::memory_order_relaxed);
        // Update percentiles (simplified - would use proper histogram in production)
        const auto current_p99 = metrics_.timing.p99_execution_latency_ns.load();
        if (latency_ns > current_p99) {
            metrics_.timing.p99_execution_latency_ns.store(latency_ns, std::memory_order_relaxed);
        }
    } else if (metric_name == "arbitrage_detection") {
        metrics_.timing.arbitrage_detection_latency_ns.store(latency_ns, std::memory_order_relaxed);
    } else if (metric_name == "risk_check") {
        metrics_.timing.risk_check_latency_ns.store(latency_ns, std::memory_order_relaxed);
    } else if (metric_name == "network_rtt") {
        metrics_.timing.network_round_trip_ns.store(latency_ns, std::memory_order_relaxed);
    }
}

void TelemetryEngine::record_trade(double pnl_usd, bool successful) {
    metrics_.trading.total_trades.fetch_add(1, std::memory_order_relaxed);
    
    if (successful) {
        metrics_.trading.successful_arbitrages.fetch_add(1, std::memory_order_relaxed);
    } else {
        metrics_.trading.failed_arbitrages.fetch_add(1, std::memory_order_relaxed);
    }
    
    // Update P&L (atomic double operations)
    auto current_pnl = metrics_.trading.total_pnl_usd.load(std::memory_order_relaxed);
    while (!metrics_.trading.total_pnl_usd.compare_exchange_weak(
        current_pnl, current_pnl + pnl_usd, std::memory_order_relaxed)) {
        // Retry if another thread modified the value
    }
    
    // Update drawdown tracking
    if (pnl_usd < 0) {
        auto current_dd = metrics_.trading.current_drawdown_percent.load();
        const double new_dd = std::abs(pnl_usd) / 1000000.0 * 100; // Assume $1M base
        if (new_dd > current_dd) {
            metrics_.trading.current_drawdown_percent.store(new_dd, std::memory_order_relaxed);
            
            auto max_dd = metrics_.trading.max_drawdown_percent.load();
            if (new_dd > max_dd) {
                metrics_.trading.max_drawdown_percent.store(new_dd, std::memory_order_relaxed);
            }
        }
    }
}

void TelemetryEngine::record_arbitrage_opportunity(const std::string& pair, double profit_usd) {
    metrics_.trading.oracle_opportunities_detected.fetch_add(1, std::memory_order_relaxed);
    
    // Add to market state (thread-safe via atomic operations on the count)
    // In production, this would use a lock-free queue
    MarketState::ArbitrageOpportunity opportunity;
    opportunity.pair = pair;
    opportunity.potential_profit_usd = profit_usd;
    opportunity.detected_at = std::chrono::high_resolution_clock::now();
    opportunity.window_remaining = std::chrono::milliseconds(500); // 500ms window
    
    // For now, just update the counter - full implementation would maintain opportunity list
}

void TelemetryEngine::record_risk_metric(const std::string& name, double value) {
    if (name == "var") {
        metrics_.risk.current_var_usd.store(value, std::memory_order_relaxed);
    } else if (name == "exposure") {
        metrics_.risk.position_exposure_usd.store(value, std::memory_order_relaxed);
    } else if (name == "sharpe") {
        metrics_.risk.sharpe_ratio.store(value, std::memory_order_relaxed);
    } else if (name == "sortino") {
        metrics_.risk.sortino_ratio.store(value, std::memory_order_relaxed);
    }
}

void TelemetryEngine::record_network_activity(std::uint64_t bytes_sent, std::uint64_t bytes_received) {
    metrics_.network.total_bytes_sent.fetch_add(bytes_sent, std::memory_order_relaxed);
    metrics_.network.total_bytes_received.fetch_add(bytes_received, std::memory_order_relaxed);
    
    if (bytes_sent > 0) {
        metrics_.network.total_messages_sent.fetch_add(1, std::memory_order_relaxed);
    }
    if (bytes_received > 0) {
        metrics_.network.total_messages_received.fetch_add(1, std::memory_order_relaxed);
    }
}

void TelemetryEngine::record_system_resource(double cpu_percent, std::uint64_t memory_mb) {
    metrics_.system.cpu_usage_percent.store(cpu_percent, std::memory_order_relaxed);
    metrics_.system.memory_usage_mb.store(memory_mb, std::memory_order_relaxed);
}

void TelemetryEngine::update_token_price(const std::string& symbol, double price_usd, double change_percent) {
    // Thread-safe update of market state
    // In production, this would use a lock-free data structure
    MarketState::TokenPrice price_update;
    price_update.symbol = symbol;
    price_update.price_usd = price_usd;
    price_update.change_24h_percent = change_percent;
    price_update.last_updated = std::chrono::high_resolution_clock::now();
    
    // For now, just demonstrate the interface - full implementation would maintain price list
}

void TelemetryEngine::update_gas_market(double standard, double fast, double instant) {
    market_state_.gas_market.standard_gwei = standard;
    market_state_.gas_market.fast_gwei = fast;
    market_state_.gas_market.instant_gwei = instant;
    market_state_.gas_market.next_base_fee = fast * 1.125; // EIP-1559 estimation
}

void TelemetryEngine::register_callback(TelemetryCallback callback) {
    callbacks_.push_back(callback);
    std::cout << "[TelemetryEngine] Registered callback, total: " << callbacks_.size() << "\n";
}

void TelemetryEngine::set_update_frequency(Duration frequency) {
    update_frequency_.store(frequency, std::memory_order_relaxed);
    std::cout << "[TelemetryEngine] Update frequency set to " 
              << std::chrono::duration_cast<std::chrono::milliseconds>(frequency).count() << "ms\n";
}

TelemetryEngine::PerformanceSnapshot TelemetryEngine::get_snapshot() const {
    PerformanceSnapshot snapshot;
    snapshot.timestamp = std::chrono::high_resolution_clock::now();
    snapshot.metrics = metrics_;
    snapshot.market_state = market_state_;
    
    // Calculate derived analytics
    const auto total_trades = metrics_.trading.total_trades.load();
    const auto successful_trades = metrics_.trading.successful_arbitrages.load();
    
    if (total_trades > 0) {
        snapshot.win_rate_percent = (static_cast<double>(successful_trades) / total_trades) * 100.0;
        snapshot.profit_per_trade_usd = metrics_.trading.total_pnl_usd.load() / total_trades;
    }
    
    snapshot.avg_latency_ms = metrics_.timing.order_execution_latency_ns.load() / 1e6;
    
    // Trades per second (simplified calculation)
    snapshot.trades_per_second = total_trades; // Would calculate rate over time window
    
    return snapshot;
}

std::vector<TelemetryEngine::PerformanceSnapshot> TelemetryEngine::get_history(Duration lookback_period) const {
    std::vector<PerformanceSnapshot> history;
    
    const auto current_index = history_index_.load(std::memory_order_acquire);
    const auto lookback_samples = std::min(HISTORY_SIZE, 
        static_cast<std::size_t>(lookback_period / update_frequency_.load()));
    
    // Copy historical data (thread-safe read)
    for (std::size_t i = 0; i < lookback_samples; ++i) {
        const auto idx = (current_index - i) % HISTORY_SIZE;
        if (!history_buffer_[idx].timestamp.time_since_epoch().count()) {
            break; // Empty slot
        }
        history.push_back(history_buffer_[idx]);
    }
    
    std::reverse(history.begin(), history.end());
    return history;
}

void TelemetryEngine::worker_loop() {
    std::cout << "[TelemetryEngine] Worker thread started\n";
    
    while (running_.load(std::memory_order_acquire)) {
        const auto start_time = std::chrono::high_resolution_clock::now();
        
        // Get current system metrics (platform-specific)
#ifdef __APPLE__
        // Get CPU usage
        host_cpu_load_info_data_t cpuinfo;
        mach_msg_type_number_t count = HOST_CPU_LOAD_INFO_COUNT;
        if (host_statistics(mach_host_self(), HOST_CPU_LOAD_INFO, 
                          (host_info_t)&cpuinfo, &count) == KERN_SUCCESS) {
            const auto total = cpuinfo.cpu_ticks[CPU_STATE_USER] + 
                              cpuinfo.cpu_ticks[CPU_STATE_SYSTEM] +
                              cpuinfo.cpu_ticks[CPU_STATE_IDLE] + 
                              cpuinfo.cpu_ticks[CPU_STATE_NICE];
            const auto used = total - cpuinfo.cpu_ticks[CPU_STATE_IDLE];
            if (total > 0) {
                const double cpu_percent = (static_cast<double>(used) / total) * 100.0;
                metrics_.system.cpu_usage_percent.store(cpu_percent, std::memory_order_relaxed);
            }
        }
        
        // Get memory usage
        vm_statistics64_data_t vm_stat;
        count = HOST_VM_INFO64_COUNT;
        if (host_statistics64(mach_host_self(), HOST_VM_INFO64, 
                            (host_info64_t)&vm_stat, &count) == KERN_SUCCESS) {
            const auto page_size = vm_page_size;
            const auto used_memory = (vm_stat.active_count + vm_stat.wire_count) * page_size;
            metrics_.system.memory_usage_mb.store(used_memory / (1024 * 1024), std::memory_order_relaxed);
        }
#endif
        
        // Create performance snapshot and store in history
        auto snapshot = get_snapshot();
        const auto current_index = history_index_.fetch_add(1, std::memory_order_relaxed) % HISTORY_SIZE;
        history_buffer_[current_index] = snapshot;
        
        // Call all registered callbacks
        for (const auto& callback : callbacks_) {
            callback(metrics_, market_state_);
        }
        
        // Sleep until next update
        const auto elapsed = std::chrono::high_resolution_clock::now() - start_time;
        const auto sleep_duration = update_frequency_.load() - elapsed;
        if (sleep_duration > Duration::zero()) {
            std::this_thread::sleep_for(sleep_duration);
        }
    }
    
    std::cout << "[TelemetryEngine] Worker thread stopped\n";
}

} // namespace hfx::viz
