/**
 * @file signal_compressor.hpp
 * @brief Ultra-fast signal compression for LLM-to-execution pipeline
 * @version 1.0.0
 * 
 * Converts rich LLM outputs into compact, microsecond-consumable signals
 * With TTL, decay functions, and deterministic replay capabilities
 */

#pragma once

#include <memory>
#include <string>
#include <atomic>
#include <chrono>
#include <vector>
#include <array>
#include <cstdint>
#include <functional>
#include <unordered_map>
#include <thread>
#include <mutex>
#include <algorithm>
#include <cmath>

namespace hfx::hft {

/**
 * @brief Compact signal format (64 bytes, cache-line aligned)
 */
struct alignas(64) CompactSignal {
    // Signal identification (8 bytes)
    uint32_t signal_id;
    uint8_t signal_type;      // BUY=1, SELL=2, HOLD=0
    uint8_t confidence;       // 0-255 (calibrated probability * 255)
    uint8_t priority;         // 0-255, higher = more urgent
    uint8_t platform_mask;    // Bitfield for target platforms
    
    // Timing information (16 bytes) - store only essential timestamps
    uint64_t publish_timestamp_ns;    // Ready for consumption
    uint16_t ttl_ms;                  // Time-to-live in milliseconds
    uint16_t age_ms;                  // Age when published (source to publish)
    uint32_t reserved_timing;
    
    // Trading parameters (8 bytes) - scaled and quantized
    int16_t direction;        // -1000 to +1000 (scaled signal strength)
    int16_t magnitude;        // 0-1000 (absolute signal strength)
    uint16_t risk_score;      // 0-1000 (higher = riskier)
    uint16_t volatility;      // Expected volatility (scaled)
    
    // Asset identification (16 bytes) - space-optimized
    char token_symbol[8];     // Null-terminated symbol
    uint64_t token_hash;      // Fast hash of full contract address
    
    // Metadata and validation (16 bytes)
    uint32_t source_mask;     // Bitfield indicating data sources
    uint16_t model_version;   // LLM model version for audit
    uint8_t decay_function;   // Exponential=0, Linear=1, Step=2
    uint8_t reserved1;
    uint32_t checksum;        // CRC32 for integrity checking
    uint32_t reserved2;
    
    // Helper methods
    bool is_expired(uint64_t current_timestamp_ns) const noexcept {
        uint64_t age_ms = (current_timestamp_ns - publish_timestamp_ns) / 1000000;
        return age_ms > ttl_ms;
    }
    
    double get_decayed_confidence(uint64_t current_timestamp_ns, double lambda = 0.001) const noexcept {
        if (is_expired(current_timestamp_ns)) return 0.0;
        
        double age_ms = (current_timestamp_ns - publish_timestamp_ns) / 1000000.0;
        double base_conf = confidence / 255.0;
        
        switch (decay_function) {
            case 0: // Exponential decay
                return base_conf * std::exp(-lambda * age_ms);
            case 1: // Linear decay
                return base_conf * std::max(0.0, 1.0 - (age_ms / ttl_ms));
            case 2: // Step function
                return base_conf;
            default:
                return 0.0;
        }
    }
    
    bool is_fresh(uint64_t current_timestamp_ns, uint64_t max_age_ns) const noexcept {
        uint64_t age_ns = static_cast<uint64_t>(age_ms) * 1000000;
        return age_ns <= max_age_ns && publish_timestamp_ns <= current_timestamp_ns;
    }
    
    uint32_t calculate_checksum() const noexcept;
    bool verify_integrity() const noexcept;
} __attribute__((packed));

static_assert(sizeof(CompactSignal) == 64, "CompactSignal must be exactly 64 bytes");

/**
 * @brief Rich LLM output before compression
 */
struct LLMSignalInput {
    std::string signal_id;
    std::string token_address;
    std::string token_symbol;
    
    // LLM analysis results
    std::string sentiment_text;
    double sentiment_score;      // -1.0 to +1.0
    double confidence_score;     // 0.0 to 1.0
    std::string reasoning;       // LLM explanation
    
    // Technical indicators
    std::vector<std::pair<std::string, double>> technical_indicators;
    
    // Risk assessment
    double risk_score;           // 0.0 to 1.0
    std::vector<std::string> risk_factors;
    
    // Market context
    double volatility_estimate;
    double liquidity_score;
    double momentum_score;
    
    // Data sources
    std::vector<std::string> data_sources;
    std::vector<std::string> news_headlines;
    std::vector<std::string> social_mentions;
    
    // Timing
    uint64_t source_timestamp_ns;
    uint64_t processing_start_ns;
    uint64_t processing_end_ns;
    
    // Model metadata
    std::string model_name;
    std::string model_version;
    std::unordered_map<std::string, std::string> model_params;
};

/**
 * @brief Calibration mapping for confidence scores
 */
class ConfidenceCalibrator {
public:
    struct CalibrationPoint {
        double raw_confidence;
        double actual_accuracy;
        uint32_t sample_count;
    };
    
    explicit ConfidenceCalibrator();
    ~ConfidenceCalibrator();
    
    // Calibration training
    void add_sample(double predicted_confidence, bool actual_outcome);
    void fit_calibration_curve();
    
    // Calibration application
    double calibrate_confidence(double raw_confidence) const;
    uint8_t quantize_confidence(double calibrated_confidence) const;
    
    // Evaluation
    double get_calibration_error() const;
    std::vector<CalibrationPoint> get_calibration_curve() const;
    
private:
    class CalibratorImpl;
    std::unique_ptr<CalibratorImpl> pimpl_;
};

/**
 * @brief Signal compression engine
 */
class SignalCompressor {
public:
    struct CompressionConfig {
        uint16_t default_ttl_ms = 500;        // Default signal lifetime
        double default_decay_lambda = 0.001;   // Exponential decay rate
        bool enable_checksum = true;           // Enable integrity checking
        bool enable_compression_stats = true;  // Track compression metrics
        size_t max_batch_size = 1000;         // Maximum batch processing size
    };
    
    explicit SignalCompressor(const CompressionConfig& config);
    ~SignalCompressor();
    
    // Single signal compression
    CompactSignal compress_signal(const LLMSignalInput& input);
    
    // Batch compression for throughput
    size_t compress_batch(const LLMSignalInput* inputs, size_t count, 
                         CompactSignal* outputs);
    
    // Decompression for audit/replay
    bool decompress_signal(const CompactSignal& compact, LLMSignalInput& output);
    
    // Signal validation
    bool validate_signal(const CompactSignal& signal) const;
    bool is_signal_stale(const CompactSignal& signal, uint64_t max_age_ns) const;
    
    // Calibration management
    void update_calibrator(const ConfidenceCalibrator& calibrator);
    const ConfidenceCalibrator& get_calibrator() const;
    
    // Performance metrics
    struct CompressionMetrics {
        std::atomic<uint64_t> signals_compressed{0};
        std::atomic<uint64_t> signals_validated{0};
        std::atomic<uint64_t> compression_errors{0};
        std::atomic<uint64_t> avg_compression_time_ns{0};
        std::atomic<uint64_t> checksum_failures{0};
    };
    
    const CompressionMetrics& get_metrics() const;
    void reset_metrics();
    
private:
    class CompressorImpl;
    std::unique_ptr<CompressorImpl> pimpl_;
};

/**
 * @brief Signal aggregation and consensus
 */
class SignalAggregator {
public:
    struct AggregationConfig {
        size_t min_sources = 2;              // Minimum sources for consensus
        double consensus_threshold = 0.7;     // Agreement threshold
        bool enable_outlier_detection = true; // Remove outlying signals
        double outlier_z_threshold = 2.0;     // Z-score threshold for outliers
        uint64_t aggregation_window_ns = 100000000; // 100ms window
    };
    
    explicit SignalAggregator(const AggregationConfig& config);
    ~SignalAggregator();
    
    // Signal aggregation
    void add_signal(const CompactSignal& signal);
    bool get_consensus_signal(CompactSignal& output);
    
    // Multi-source validation
    bool validate_consensus(const std::vector<CompactSignal>& signals);
    double calculate_agreement_score(const std::vector<CompactSignal>& signals);
    
    // Outlier detection
    std::vector<size_t> detect_outliers(const std::vector<CompactSignal>& signals);
    
    // Real-time processing
    void start_aggregation_loop();
    void stop_aggregation_loop();
    
    // Callbacks
    using ConsensusCallback = std::function<void(const CompactSignal&)>;
    using DisagreementCallback = std::function<void(const std::vector<CompactSignal>&)>;
    
    void set_consensus_callback(ConsensusCallback callback);
    void set_disagreement_callback(DisagreementCallback callback);
    
private:
    class AggregatorImpl;
    std::unique_ptr<AggregatorImpl> pimpl_;
};

/**
 * @brief Fast signal lookup and caching
 */
class SignalCache {
public:
    static constexpr size_t CACHE_SIZE = 65536; // Must be power of 2
    static constexpr size_t CACHE_LINE_SIZE = 64;
    
    explicit SignalCache();
    ~SignalCache();
    
    // Cache operations
    bool insert(const std::string& token_symbol, const CompactSignal& signal);
    bool lookup(const std::string& token_symbol, CompactSignal& signal);
    void evict_expired(uint64_t current_timestamp_ns);
    void clear();
    
    // Cache statistics
    struct CacheStats {
        std::atomic<uint64_t> hits{0};
        std::atomic<uint64_t> misses{0};
        std::atomic<uint64_t> insertions{0};
        std::atomic<uint64_t> evictions{0};
        std::atomic<uint64_t> size{0};
    };
    
    const CacheStats& get_stats() const;
    double get_hit_ratio() const;
    
private:
    class CacheImpl;
    std::unique_ptr<CacheImpl> pimpl_;
};

/**
 * @brief Signal replay system for backtesting and compliance
 */
class SignalReplayEngine {
public:
    struct ReplayConfig {
        std::string replay_data_path;
        bool strict_timing = true;          // Enforce original timing
        bool validate_checksums = true;     // Verify signal integrity
        double time_scale = 1.0;            // Speed up/slow down replay
        bool enable_deterministic_mode = true; // Ensure reproducible results
    };
    
    explicit SignalReplayEngine(const ReplayConfig& config);
    ~SignalReplayEngine();
    
    // Replay control
    bool load_replay_data(const std::string& file_path);
    void start_replay();
    void pause_replay();
    void stop_replay();
    void reset_replay();
    
    // Signal access during replay
    bool get_next_signal(CompactSignal& signal);
    bool peek_next_signal(CompactSignal& signal);
    void seek_to_timestamp(uint64_t timestamp_ns);
    
    // Replay validation
    bool validate_replay_integrity();
    std::vector<std::string> get_validation_errors();
    
    // Callbacks
    using SignalReplayCallback = std::function<void(const CompactSignal&)>;
    void set_signal_callback(SignalReplayCallback callback);
    
    // Replay statistics
    struct ReplayStats {
        uint64_t total_signals;
        uint64_t current_position;
        uint64_t start_timestamp_ns;
        uint64_t end_timestamp_ns;
        uint64_t current_timestamp_ns;
        double progress_percent;
    };
    
    ReplayStats get_replay_stats() const;
    
private:
    class ReplayImpl;
    std::unique_ptr<ReplayImpl> pimpl_;
};

/**
 * @brief Signal distribution system
 */
class SignalDistributor {
public:
    enum class DistributionMode {
        BROADCAST,      // Send to all subscribers
        ROUND_ROBIN,    // Distribute evenly
        PRIORITY_BASED, // Send to highest priority subscriber
        LOAD_BALANCED   // Send to least busy subscriber
    };
    
    struct DistributorConfig {
        DistributionMode mode = DistributionMode::BROADCAST;
        size_t max_subscribers = 100;
        size_t buffer_size_per_subscriber = 1000;
        bool enable_backpressure = true;
    };
    
    explicit SignalDistributor(const DistributorConfig& config);
    ~SignalDistributor();
    
    // Subscriber management
    uint32_t subscribe(const std::string& subscriber_id, uint8_t priority = 128);
    void unsubscribe(uint32_t subscriber_handle);
    
    // Signal distribution
    bool distribute_signal(const CompactSignal& signal);
    size_t distribute_batch(const CompactSignal* signals, size_t count);
    
    // Subscriber access
    bool get_signal(uint32_t subscriber_handle, CompactSignal& signal);
    size_t get_signals(uint32_t subscriber_handle, CompactSignal* signals, size_t max_count);
    
    // Statistics
    struct DistributionStats {
        std::atomic<uint64_t> signals_distributed{0};
        std::atomic<uint64_t> total_subscribers{0};
        std::atomic<uint64_t> backpressure_events{0};
        std::atomic<uint64_t> dropped_signals{0};
    };
    
    const DistributionStats& get_stats() const;
    
private:
    class DistributorImpl;
    std::unique_ptr<DistributorImpl> pimpl_;
};

} // namespace hfx::hft
