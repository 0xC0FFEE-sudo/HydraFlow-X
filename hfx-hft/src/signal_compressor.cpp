/**
 * @file signal_compressor.cpp
 * @brief Signal compression implementation
 */

#include "signal_compressor.hpp"
#include <cstring>
#include <thread>
#include <mutex>
#include <iostream>
#include <algorithm>
#include <cmath>

// Simple CRC32 implementation to avoid external dependency
namespace {
    uint32_t crc32_simple(const void* data, size_t length) {
        const uint8_t* bytes = static_cast<const uint8_t*>(data);
        uint32_t crc = 0xFFFFFFFF;
        
        for (size_t i = 0; i < length; ++i) {
            crc ^= bytes[i];
            for (int j = 0; j < 8; ++j) {
                crc = (crc >> 1) ^ (0xEDB88320 & (-(crc & 1)));
            }
        }
        
        return ~crc;
    }
}

namespace hfx::hft {

uint32_t CompactSignal::calculate_checksum() const noexcept {
    // Calculate CRC32 checksum excluding the checksum field itself
    const char* data = reinterpret_cast<const char*>(this);
    size_t size = sizeof(CompactSignal) - sizeof(checksum);
    return crc32_simple(data, size);
}

bool CompactSignal::verify_integrity() const noexcept {
    return checksum == calculate_checksum();
}

// ============================================================================
// Confidence Calibrator Implementation
// ============================================================================

class ConfidenceCalibrator::CalibratorImpl {
public:
    std::vector<CalibrationPoint> calibration_curve_;
    std::vector<std::pair<double, bool>> samples_;
    
    void add_sample(double predicted_confidence, bool actual_outcome) {
        samples_.emplace_back(predicted_confidence, actual_outcome);
    }
    
    void fit_calibration_curve() {
        // Simple isotonic regression for calibration
        // In a real implementation, this would use proper statistical methods
        calibration_curve_.clear();
        
        // Create bins and calculate actual accuracies
        constexpr int num_bins = 10;
        for (int i = 0; i < num_bins; ++i) {
            double bin_start = i / 10.0;
            double bin_end = (i + 1) / 10.0;
            
            std::vector<bool> bin_outcomes;
            for (const auto& [conf, outcome] : samples_) {
                if (conf >= bin_start && conf < bin_end) {
                    bin_outcomes.push_back(outcome);
                }
            }
            
            if (!bin_outcomes.empty()) {
                double actual_accuracy = 0.0;
                for (bool outcome : bin_outcomes) {
                    if (outcome) actual_accuracy += 1.0;
                }
                actual_accuracy /= bin_outcomes.size();
                
                CalibrationPoint point;
                point.raw_confidence = (bin_start + bin_end) / 2.0;
                point.actual_accuracy = actual_accuracy;
                point.sample_count = bin_outcomes.size();
                
                calibration_curve_.push_back(point);
            }
        }
    }
    
    double calibrate_confidence(double raw_confidence) const {
        if (calibration_curve_.empty()) {
            return raw_confidence; // No calibration available
        }
        
        // Simple linear interpolation
        for (size_t i = 0; i < calibration_curve_.size() - 1; ++i) {
            const auto& p1 = calibration_curve_[i];
            const auto& p2 = calibration_curve_[i + 1];
            
            if (raw_confidence >= p1.raw_confidence && raw_confidence <= p2.raw_confidence) {
                double t = (raw_confidence - p1.raw_confidence) / 
                          (p2.raw_confidence - p1.raw_confidence);
                return p1.actual_accuracy + t * (p2.actual_accuracy - p1.actual_accuracy);
            }
        }
        
        // Outside range, use nearest point
        if (raw_confidence < calibration_curve_.front().raw_confidence) {
            return calibration_curve_.front().actual_accuracy;
        } else {
            return calibration_curve_.back().actual_accuracy;
        }
    }
};

ConfidenceCalibrator::ConfidenceCalibrator() 
    : pimpl_(std::make_unique<CalibratorImpl>()) {}

ConfidenceCalibrator::~ConfidenceCalibrator() = default;

void ConfidenceCalibrator::add_sample(double predicted_confidence, bool actual_outcome) {
    pimpl_->add_sample(predicted_confidence, actual_outcome);
}

void ConfidenceCalibrator::fit_calibration_curve() {
    pimpl_->fit_calibration_curve();
}

double ConfidenceCalibrator::calibrate_confidence(double raw_confidence) const {
    return pimpl_->calibrate_confidence(raw_confidence);
}

uint8_t ConfidenceCalibrator::quantize_confidence(double calibrated_confidence) const {
    return static_cast<uint8_t>(std::clamp(calibrated_confidence * 255.0, 0.0, 255.0));
}

double ConfidenceCalibrator::get_calibration_error() const {
    // Simplified calibration error calculation
    return 0.05; // Placeholder
}

std::vector<ConfidenceCalibrator::CalibrationPoint> ConfidenceCalibrator::get_calibration_curve() const {
    return pimpl_->calibration_curve_;
}

// ============================================================================
// Signal Compressor Implementation
// ============================================================================

class SignalCompressor::CompressorImpl {
public:
    CompressionConfig config_;
    ConfidenceCalibrator calibrator_;
    mutable CompressionMetrics metrics_;
    
    CompressorImpl(const CompressionConfig& config) : config_(config) {}
    
    CompactSignal compress_signal(const LLMSignalInput& input) {
        auto start_time = std::chrono::high_resolution_clock::now();
        
        CompactSignal signal;
        memset(&signal, 0, sizeof(signal));
        
        // Set signal identification
        signal.signal_id = std::hash<std::string>{}(input.signal_id);
        
        // Determine signal type
        if (input.sentiment_score > 0.1) {
            signal.signal_type = 1; // BUY
        } else if (input.sentiment_score < -0.1) {
            signal.signal_type = 2; // SELL
        } else {
            signal.signal_type = 0; // HOLD
        }
        
        // Set confidence (calibrated and quantized)
        double calibrated_conf = calibrator_.calibrate_confidence(input.confidence_score);
        signal.confidence = calibrator_.quantize_confidence(calibrated_conf);
        
        // Set priority based on confidence and urgency
        signal.priority = static_cast<uint8_t>(calibrated_conf * 255);
        
        // Set timing information
        uint64_t current_time = std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch()).count();
        signal.publish_timestamp_ns = current_time;
        signal.ttl_ms = config_.default_ttl_ms;
        signal.age_ms = static_cast<uint16_t>((current_time - input.source_timestamp_ns) / 1000000);
        
        // Set trading parameters
        signal.direction = static_cast<int16_t>(input.sentiment_score * 1000);
        signal.magnitude = static_cast<int16_t>(std::abs(input.sentiment_score) * 1000);
        signal.risk_score = static_cast<uint16_t>(input.risk_score * 1000);
        signal.volatility = static_cast<uint16_t>(input.volatility_estimate * 1000);
        
        // Set asset identification
        strncpy(signal.token_symbol, input.token_symbol.c_str(), sizeof(signal.token_symbol) - 1);
        signal.token_hash = std::hash<std::string>{}(input.token_address);
        
        // Set metadata
        signal.source_mask = 0; // Encode data sources as bitmask
        signal.model_version = std::hash<std::string>{}(input.model_version) & 0xFFFF;
        signal.decay_function = 0; // Exponential decay
        
        // Calculate and set checksum
        if (config_.enable_checksum) {
            signal.checksum = signal.calculate_checksum();
        }
        
        // Update metrics
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);
        
        metrics_.signals_compressed.fetch_add(1);
        update_avg_compression_time(duration.count());
        
        return signal;
    }
    
    void update_avg_compression_time(uint64_t time_ns) {
        uint64_t current_avg = metrics_.avg_compression_time_ns.load();
        uint64_t new_avg = (current_avg * 63 + time_ns) / 64; // Moving average
        metrics_.avg_compression_time_ns.store(new_avg);
    }
};

SignalCompressor::SignalCompressor(const CompressionConfig& config)
    : pimpl_(std::make_unique<CompressorImpl>(config)) {}

SignalCompressor::~SignalCompressor() = default;

CompactSignal SignalCompressor::compress_signal(const LLMSignalInput& input) {
    return pimpl_->compress_signal(input);
}

size_t SignalCompressor::compress_batch(const LLMSignalInput* inputs, size_t count, 
                                       CompactSignal* outputs) {
    for (size_t i = 0; i < count; ++i) {
        outputs[i] = compress_signal(inputs[i]);
    }
    return count;
}

bool SignalCompressor::decompress_signal(const CompactSignal& compact, LLMSignalInput& output) {
    // Basic decompression - in real implementation would be more complete
    output.signal_id = std::to_string(compact.signal_id);
    output.token_symbol = std::string(compact.token_symbol);
    output.sentiment_score = compact.direction / 1000.0;
    output.confidence_score = compact.confidence / 255.0;
    output.risk_score = compact.risk_score / 1000.0;
    output.volatility_estimate = compact.volatility / 1000.0;
    
    // Reconstruct timestamps from simplified structure
    output.source_timestamp_ns = compact.publish_timestamp_ns - (compact.age_ms * 1000000ULL);
    output.processing_start_ns = compact.publish_timestamp_ns - (compact.age_ms * 500000ULL); // Estimate
    output.processing_end_ns = compact.publish_timestamp_ns;
    
    return true;
}

bool SignalCompressor::validate_signal(const CompactSignal& signal) const {
    if (pimpl_->config_.enable_checksum && !signal.verify_integrity()) {
        pimpl_->metrics_.checksum_failures.fetch_add(1);
        return false;
    }
    
    pimpl_->metrics_.signals_validated.fetch_add(1);
    return true;
}

bool SignalCompressor::is_signal_stale(const CompactSignal& signal, uint64_t max_age_ns) const {
    auto current_time = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    
    return signal.is_expired(current_time) || 
           (current_time - signal.publish_timestamp_ns) > max_age_ns;
}

void SignalCompressor::update_calibrator(const ConfidenceCalibrator& calibrator) {
    // Recreate the implementation with a new calibrator
    // In a real implementation, we'd copy the calibrator data properly
    pimpl_ = std::make_unique<CompressorImpl>(pimpl_->config_);
    // The new impl will have a default calibrator - in practice we'd copy the data
}

const ConfidenceCalibrator& SignalCompressor::get_calibrator() const {
    return pimpl_->calibrator_;
}

const SignalCompressor::CompressionMetrics& SignalCompressor::get_metrics() const {
    return pimpl_->metrics_;
}

void SignalCompressor::reset_metrics() {
    pimpl_->metrics_.signals_compressed.store(0);
    pimpl_->metrics_.signals_validated.store(0);
    pimpl_->metrics_.compression_errors.store(0);
    pimpl_->metrics_.avg_compression_time_ns.store(0);
    pimpl_->metrics_.checksum_failures.store(0);
}

} // namespace hfx::hft
