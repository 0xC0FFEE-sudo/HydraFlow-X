/**
 * @file consensus_engine.cpp
 * @brief Consensus engine stub implementation
 */

#include "signal_compressor.hpp"
#include <iostream>
#include <algorithm>
#include <random>
#include <thread>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <cmath>

namespace hfx::hft {

// ============================================================================
// Signal Aggregator Implementation
// ============================================================================

class SignalAggregator::AggregatorImpl {
public:
    AggregationConfig config_;
    std::vector<CompactSignal> pending_signals_;
    std::mutex signals_mutex_;
    std::atomic<bool> running_{false};
    std::unique_ptr<std::thread> aggregation_thread_;
    
    ConsensusCallback consensus_callback_;
    DisagreementCallback disagreement_callback_;
    std::mutex callback_mutex_;
    
    AggregatorImpl(const AggregationConfig& config) : config_(config) {}
    
    ~AggregatorImpl() {
        stop_aggregation_loop();
    }
    
    void add_signal(const CompactSignal& signal) {
        std::lock_guard<std::mutex> lock(signals_mutex_);
        pending_signals_.push_back(signal);
        
        // Remove old signals outside aggregation window
        auto current_time = std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch()).count();
        
        pending_signals_.erase(
            std::remove_if(pending_signals_.begin(), pending_signals_.end(),
                [this, current_time](const CompactSignal& s) {
                    return (current_time - s.publish_timestamp_ns) > config_.aggregation_window_ns;
                }),
            pending_signals_.end()
        );
    }
    
    bool get_consensus_signal(CompactSignal& output) {
        std::lock_guard<std::mutex> lock(signals_mutex_);
        
        if (pending_signals_.size() < config_.min_sources) {
            return false;
        }
        
        // Group signals by token
        std::unordered_map<std::string, std::vector<CompactSignal>> token_signals;
        for (const auto& signal : pending_signals_) {
            std::string token(signal.token_symbol);
            token_signals[token].push_back(signal);
        }
        
        // Find token with most signals
        std::string best_token;
        size_t max_signals = 0;
        for (const auto& [token, signals] : token_signals) {
            if (signals.size() >= config_.min_sources && signals.size() > max_signals) {
                max_signals = signals.size();
                best_token = token;
            }
        }
        
        if (best_token.empty()) {
            return false;
        }
        
        const auto& signals = token_signals[best_token];
        
        // Check consensus
        if (!validate_consensus(signals)) {
            return false;
        }
        
        // Create consensus signal
        output = create_consensus_signal(signals);
        return true;
    }
    
    bool validate_consensus(const std::vector<CompactSignal>& signals) {
        double agreement = calculate_agreement_score(signals);
        return agreement >= config_.consensus_threshold;
    }
    
    double calculate_agreement_score(const std::vector<CompactSignal>& signals) {
        if (signals.empty()) return 0.0;
        
        // Calculate agreement based on signal direction
        int positive_count = 0;
        int negative_count = 0;
        int neutral_count = 0;
        
        for (const auto& signal : signals) {
            if (signal.direction > 100) {
                positive_count++;
            } else if (signal.direction < -100) {
                negative_count++;
            } else {
                neutral_count++;
            }
        }
        
        int max_count = std::max({positive_count, negative_count, neutral_count});
        return static_cast<double>(max_count) / signals.size();
    }
    
    std::vector<size_t> detect_outliers(const std::vector<CompactSignal>& signals) {
        std::vector<size_t> outliers;
        
        if (signals.size() < 3) {
            return outliers; // Need at least 3 signals for outlier detection
        }
        
        // Simple outlier detection based on signal direction
        std::vector<double> directions;
        for (const auto& signal : signals) {
            directions.push_back(signal.direction);
        }
        
        // Calculate mean and standard deviation
        double mean = 0.0;
        for (double dir : directions) {
            mean += dir;
        }
        mean /= directions.size();
        
        double variance = 0.0;
        for (double dir : directions) {
            variance += (dir - mean) * (dir - mean);
        }
        variance /= directions.size();
        double std_dev = std::sqrt(variance);
        
        // Find outliers (z-score > threshold)
        for (size_t i = 0; i < signals.size(); ++i) {
            double z_score = std::abs(directions[i] - mean) / std_dev;
            if (z_score > config_.outlier_z_threshold) {
                outliers.push_back(i);
            }
        }
        
        return outliers;
    }
    
    CompactSignal create_consensus_signal(const std::vector<CompactSignal>& signals) {
        CompactSignal consensus = signals[0]; // Start with first signal
        
        // Average the numeric values
        int64_t total_direction = 0;
        int64_t total_magnitude = 0;
        int64_t total_confidence = 0;
        
        for (const auto& signal : signals) {
            total_direction += signal.direction;
            total_magnitude += signal.magnitude;
            total_confidence += signal.confidence;
        }
        
        consensus.direction = static_cast<int16_t>(total_direction / signals.size());
        consensus.magnitude = static_cast<int16_t>(total_magnitude / signals.size());
        consensus.confidence = static_cast<uint8_t>(total_confidence / signals.size());
        
        // Update timestamps
        consensus.publish_timestamp_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch()).count();
        
        // Update source mask to indicate consensus
        consensus.source_mask = 0xFFFFFFFF; // All sources
        
        return consensus;
    }
    
    void start_aggregation_loop() {
        if (running_.load()) return;
        
        running_.store(true);
        aggregation_thread_ = std::make_unique<std::thread>(&AggregatorImpl::aggregation_loop, this);
        HFX_LOG_INFO("[SignalAggregator] Started aggregation loop");
    }
    
    void stop_aggregation_loop() {
        running_.store(false);
        if (aggregation_thread_ && aggregation_thread_->joinable()) {
            aggregation_thread_->join();
        }
        HFX_LOG_INFO("[SignalAggregator] Stopped aggregation loop");
    }
    
    void aggregation_loop() {
        while (running_.load()) {
            CompactSignal consensus;
            if (get_consensus_signal(consensus)) {
                call_consensus_callback(consensus);
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
    
    void call_consensus_callback(const CompactSignal& signal) {
        std::lock_guard<std::mutex> lock(callback_mutex_);
        if (consensus_callback_) {
            consensus_callback_(signal);
        }
    }
    
    void set_consensus_callback(ConsensusCallback callback) {
        std::lock_guard<std::mutex> lock(callback_mutex_);
        consensus_callback_ = std::move(callback);
    }
    
    void set_disagreement_callback(DisagreementCallback callback) {
        std::lock_guard<std::mutex> lock(callback_mutex_);
        disagreement_callback_ = std::move(callback);
    }
};

SignalAggregator::SignalAggregator(const AggregationConfig& config)
    : pimpl_(std::make_unique<AggregatorImpl>(config)) {}

SignalAggregator::~SignalAggregator() = default;

void SignalAggregator::add_signal(const CompactSignal& signal) {
    pimpl_->add_signal(signal);
}

bool SignalAggregator::get_consensus_signal(CompactSignal& output) {
    return pimpl_->get_consensus_signal(output);
}

bool SignalAggregator::validate_consensus(const std::vector<CompactSignal>& signals) {
    return pimpl_->validate_consensus(signals);
}

double SignalAggregator::calculate_agreement_score(const std::vector<CompactSignal>& signals) {
    return pimpl_->calculate_agreement_score(signals);
}

std::vector<size_t> SignalAggregator::detect_outliers(const std::vector<CompactSignal>& signals) {
    return pimpl_->detect_outliers(signals);
}

void SignalAggregator::start_aggregation_loop() {
    pimpl_->start_aggregation_loop();
}

void SignalAggregator::stop_aggregation_loop() {
    pimpl_->stop_aggregation_loop();
}

void SignalAggregator::set_consensus_callback(ConsensusCallback callback) {
    pimpl_->set_consensus_callback(std::move(callback));
}

void SignalAggregator::set_disagreement_callback(DisagreementCallback callback) {
    pimpl_->set_disagreement_callback(std::move(callback));
}

// ============================================================================
// Signal Cache Implementation
// ============================================================================

class SignalCache::CacheImpl {
public:
    struct CacheEntry {
        CompactSignal signal;
        uint64_t insertion_time_ns;
        bool valid;
    };
    
    std::array<CacheEntry, CACHE_SIZE> cache_;
    mutable CacheStats stats_;
    
    CacheImpl() {
        for (auto& entry : cache_) {
            entry.valid = false;
        }
    }
    
    bool insert(const std::string& token_symbol, const CompactSignal& signal) {
        uint64_t hash = std::hash<std::string>{}(token_symbol);
        size_t index = hash & (CACHE_SIZE - 1); // Fast modulo for power of 2
        
        cache_[index].signal = signal;
        cache_[index].insertion_time_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch()).count();
        cache_[index].valid = true;
        
        stats_.insertions.fetch_add(1);
        stats_.size.store(count_valid_entries());
        
        return true;
    }
    
    bool lookup(const std::string& token_symbol, CompactSignal& signal) {
        uint64_t hash = std::hash<std::string>{}(token_symbol);
        size_t index = hash & (CACHE_SIZE - 1);
        
        if (cache_[index].valid && 
            std::string(cache_[index].signal.token_symbol) == token_symbol) {
            signal = cache_[index].signal;
            stats_.hits.fetch_add(1);
            return true;
        }
        
        stats_.misses.fetch_add(1);
        return false;
    }
    
    void evict_expired(uint64_t current_timestamp_ns) {
        size_t evicted = 0;
        
        for (auto& entry : cache_) {
            if (entry.valid && entry.signal.is_expired(current_timestamp_ns)) {
                entry.valid = false;
                evicted++;
            }
        }
        
        stats_.evictions.fetch_add(evicted);
        stats_.size.store(count_valid_entries());
    }
    
    void clear() {
        for (auto& entry : cache_) {
            entry.valid = false;
        }
        stats_.size.store(0);
    }
    
    size_t count_valid_entries() const {
        size_t count = 0;
        for (const auto& entry : cache_) {
            if (entry.valid) count++;
        }
        return count;
    }
};

SignalCache::SignalCache() : pimpl_(std::make_unique<CacheImpl>()) {}

SignalCache::~SignalCache() = default;

bool SignalCache::insert(const std::string& token_symbol, const CompactSignal& signal) {
    return pimpl_->insert(token_symbol, signal);
}

bool SignalCache::lookup(const std::string& token_symbol, CompactSignal& signal) {
    return pimpl_->lookup(token_symbol, signal);
}

void SignalCache::evict_expired(uint64_t current_timestamp_ns) {
    pimpl_->evict_expired(current_timestamp_ns);
}

void SignalCache::clear() {
    pimpl_->clear();
}

const SignalCache::CacheStats& SignalCache::get_stats() const {
    return pimpl_->stats_;
}

double SignalCache::get_hit_ratio() const {
    uint64_t hits = pimpl_->stats_.hits.load();
    uint64_t misses = pimpl_->stats_.misses.load();
    uint64_t total = hits + misses;
    
    return (total > 0) ? (static_cast<double>(hits) / total) : 0.0;
}

} // namespace hfx::hft
