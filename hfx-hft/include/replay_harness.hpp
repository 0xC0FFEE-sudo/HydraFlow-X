/**
 * @file replay_harness.hpp
 * @brief Market replay system with TTL gates and compliance validation
 * @version 1.0.0
 * 
 * Deterministic replay for backtesting, compliance, and signal validation
 * Enforces TTL constraints and audit trail generation
 */

#pragma once

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <queue>
#include <atomic>
#include <chrono>
#include <functional>

#include "signal_compressor.hpp"
#include "execution_engine.hpp"
#include "policy_engine.hpp"

namespace hfx::hft {

/**
 * @brief Market data event for replay
 */
struct MarketDataEvent {
    uint64_t timestamp_ns;           // Event timestamp
    uint64_t sequence_number;        // Event ordering
    std::string symbol;
    std::string event_type;          // "TRADE", "QUOTE", "BOOK_UPDATE"
    
    // Price/volume data
    double price;
    double quantity;
    double bid_price;
    double ask_price;
    double bid_size;
    double ask_size;
    
    // Additional metadata
    std::string exchange;
    std::string trade_condition;
    uint32_t book_level;            // For depth updates
    bool is_bid;                    // For order book events
    
    // Replay metadata
    uint64_t original_latency_ns;   // Original processing latency
    std::string data_source;
    uint32_t checksum;              // Data integrity
} __attribute__((packed));

/**
 * @brief Signal injection event for replay
 */
struct SignalEvent {
    uint64_t timestamp_ns;
    CompactSignal signal;
    std::string signal_source;
    std::string strategy_name;
    bool was_executed;              // Did this signal result in a trade?
    ExecutionResult execution_result; // If executed, the result
} __attribute__((packed));

/**
 * @brief Trade execution event for replay
 */
struct TradeEvent {
    uint64_t timestamp_ns;
    ExecutionCommand command;
    ExecutionResult result;
    PolicyResult policy_result;
    std::string strategy_name;
    double portfolio_value_before;
    double portfolio_value_after;
    std::unordered_map<std::string, double> positions_after;
} __attribute__((packed));

/**
 * @brief Replay session configuration
 */
struct ReplayConfig {
    std::string session_name;
    uint64_t start_timestamp_ns;
    uint64_t end_timestamp_ns;
    double time_scale = 1.0;         // 1.0 = real-time, 2.0 = 2x speed
    
    // Data sources
    std::vector<std::string> market_data_files;
    std::vector<std::string> signal_files;
    std::vector<std::string> trade_files;
    
    // Replay behavior
    bool strict_timing = true;       // Enforce original timing constraints
    bool validate_signals = true;    // Check signal TTL and integrity
    bool validate_policies = true;   // Re-run policy validation
    bool deterministic_mode = true;  // Ensure reproducible results
    
    // TTL enforcement
    bool enforce_signal_ttl = true;
    uint64_t max_signal_age_ns = 1000000000; // 1 second default
    bool drop_expired_signals = true;
    
    // Output configuration
    std::string output_directory;
    bool generate_audit_trail = true;
    bool export_performance_metrics = true;
    bool save_reproduced_results = true;
    
    // Memory and performance
    size_t max_events_in_memory = 1000000;
    size_t read_ahead_buffer_size = 100000;
    size_t worker_thread_count = 4;
};

/**
 * @brief Event stream for chronological replay
 */
class EventStream {
public:
    enum class EventType : uint8_t {
        MARKET_DATA,
        SIGNAL,
        TRADE,
        POLICY_UPDATE,
        SYSTEM_EVENT
    };
    
    struct Event {
        uint64_t timestamp_ns;
        EventType type;
        size_t data_size;
        std::unique_ptr<char[]> data;
        
        template<typename T>
        T* get_data() { return reinterpret_cast<T*>(data.get()); }
        
        template<typename T>
        const T* get_data() const { return reinterpret_cast<const T*>(data.get()); }
    };
    
    explicit EventStream(const std::string& file_path);
    ~EventStream();
    
    bool open();
    void close();
    bool is_open() const;
    
    bool read_next_event(Event& event);
    bool peek_next_event(Event& event);
    void seek_to_timestamp(uint64_t timestamp_ns);
    void reset_to_beginning();
    
    uint64_t get_start_timestamp() const;
    uint64_t get_end_timestamp() const;
    size_t get_total_events() const;
    size_t get_current_position() const;
    
private:
    class StreamImpl;
    std::unique_ptr<StreamImpl> pimpl_;
};

/**
 * @brief TTL validation engine
 */
class TTLValidator {
public:
    struct ValidationConfig {
        uint64_t default_max_age_ns = 500000000;  // 500ms default
        bool strict_mode = true;                   // Fail on any TTL violation
        bool log_violations = true;                // Log TTL violations
        double decay_lambda = 0.001;              // Exponential decay rate
    };
    
    explicit TTLValidator(const ValidationConfig& config);
    ~TTLValidator();
    
    // Signal validation
    bool validate_signal_freshness(const CompactSignal& signal, 
                                  uint64_t current_timestamp_ns) const;
    
    double calculate_signal_weight(const CompactSignal& signal,
                                 uint64_t current_timestamp_ns) const;
    
    // Batch validation
    size_t validate_signal_batch(const CompactSignal* signals, size_t count,
                                uint64_t current_timestamp_ns,
                                bool* validation_results) const;
    
    // Violation tracking
    struct TTLViolation {
        uint64_t timestamp_ns;
        uint32_t signal_id;
        uint64_t signal_age_ns;
        uint64_t max_allowed_age_ns;
        std::string violation_reason;
    };
    
    std::vector<TTLViolation> get_violations_since(uint64_t since_timestamp_ns) const;
    void clear_violations();
    
    // Statistics
    struct TTLStats {
        std::atomic<uint64_t> signals_validated{0};
        std::atomic<uint64_t> signals_passed{0};
        std::atomic<uint64_t> signals_failed{0};
        std::atomic<uint64_t> total_violations{0};
        std::atomic<uint64_t> avg_signal_age_ns{0};
    };
    
    const TTLStats& get_statistics() const;
    void reset_statistics();
    
private:
    class ValidatorImpl;
    std::unique_ptr<ValidatorImpl> pimpl_;
};

/**
 * @brief Market replay engine
 */
class ReplayEngine {
public:
    explicit ReplayEngine(const ReplayConfig& config);
    ~ReplayEngine();
    
    // Session management
    bool initialize();
    void shutdown();
    bool is_initialized() const;
    
    // Data loading
    bool load_market_data(const std::vector<std::string>& file_paths);
    bool load_signal_data(const std::vector<std::string>& file_paths);
    bool load_trade_data(const std::vector<std::string>& file_paths);
    
    // Replay control
    void start_replay();
    void pause_replay();
    void resume_replay();
    void stop_replay();
    void step_forward(size_t events = 1);
    
    // Navigation
    void seek_to_timestamp(uint64_t timestamp_ns);
    void seek_to_percentage(double percentage);
    void reset_to_beginning();
    
    // Real-time event access
    bool get_next_event(EventStream::Event& event);
    bool peek_next_event(EventStream::Event& event);
    uint64_t get_current_timestamp() const;
    
    // Signal processing with TTL validation
    bool process_signal_event(const SignalEvent& signal_event);
    std::vector<CompactSignal> get_active_signals(uint64_t current_timestamp_ns) const;
    
    // Policy validation replay
    bool replay_policy_decisions();
    std::vector<PolicyResult> compare_policy_results() const;
    
    // Callbacks for event processing
    using MarketDataCallback = std::function<void(const MarketDataEvent&)>;
    using SignalCallback = std::function<void(const SignalEvent&)>;
    using TradeCallback = std::function<void(const TradeEvent&)>;
    using TTLViolationCallback = std::function<void(const TTLValidator::TTLViolation&)>;
    
    void set_market_data_callback(MarketDataCallback callback);
    void set_signal_callback(SignalCallback callback);
    void set_trade_callback(TradeCallback callback);
    void set_ttl_violation_callback(TTLViolationCallback callback);
    
    // Replay validation
    bool validate_replay_integrity();
    std::vector<std::string> get_validation_errors() const;
    
    // Performance metrics
    struct ReplayMetrics {
        uint64_t total_events_processed;
        uint64_t market_data_events;
        uint64_t signal_events;
        uint64_t trade_events;
        uint64_t ttl_violations;
        uint64_t policy_violations;
        uint64_t start_timestamp_ns;
        uint64_t end_timestamp_ns;
        uint64_t actual_runtime_ns;
        double time_compression_ratio;
    };
    
    ReplayMetrics get_replay_metrics() const;
    
    // Audit trail generation
    bool generate_audit_trail(const std::string& output_path);
    bool export_compliance_report(const std::string& output_path);
    
private:
    class ReplayEngineImpl;
    std::unique_ptr<ReplayEngineImpl> pimpl_;
};

/**
 * @brief Compliance validation engine
 */
class ComplianceValidator {
public:
    struct ComplianceConfig {
        bool validate_order_timing = true;
        bool validate_signal_sources = true;
        bool validate_policy_adherence = true;
        bool validate_risk_limits = true;
        bool check_market_manipulation = true;
        std::vector<std::string> required_audit_fields;
    };
    
    explicit ComplianceValidator(const ComplianceConfig& config);
    ~ComplianceValidator();
    
    // Validation methods
    bool validate_trading_session(const std::vector<TradeEvent>& trades);
    bool validate_signal_usage(const std::vector<SignalEvent>& signals);
    bool validate_policy_compliance(const std::vector<TradeEvent>& trades);
    
    // Audit trail validation
    bool validate_audit_completeness(const std::string& audit_file_path);
    bool validate_data_lineage(const std::vector<SignalEvent>& signals);
    
    // Compliance reporting
    struct ComplianceReport {
        std::string session_id;
        uint64_t validation_timestamp_ns;
        bool overall_compliance;
        
        struct Violation {
            std::string violation_type;
            std::string description;
            uint64_t timestamp_ns;
            std::string severity;
            std::string remediation_action;
        };
        
        std::vector<Violation> violations;
        std::unordered_map<std::string, size_t> violation_counts;
        std::unordered_map<std::string, std::string> metadata;
    };
    
    ComplianceReport generate_compliance_report();
    bool export_compliance_report(const ComplianceReport& report, 
                                 const std::string& output_path);
    
    // Regulatory checks
    bool check_best_execution(const std::vector<TradeEvent>& trades);
    bool check_market_making_obligations(const std::vector<TradeEvent>& trades);
    bool check_position_limits(const std::vector<TradeEvent>& trades);
    bool check_risk_management_controls(const std::vector<TradeEvent>& trades);
    
private:
    class ComplianceImpl;
    std::unique_ptr<ComplianceImpl> pimpl_;
};

/**
 * @brief Replay session manager
 */
class ReplaySessionManager {
public:
    explicit ReplaySessionManager();
    ~ReplaySessionManager();
    
    // Session management
    std::string create_session(const ReplayConfig& config);
    bool load_session(const std::string& session_id);
    void delete_session(const std::string& session_id);
    std::vector<std::string> list_sessions() const;
    
    // Session execution
    bool run_session(const std::string& session_id);
    bool run_session_async(const std::string& session_id);
    void cancel_session(const std::string& session_id);
    
    // Session status
    enum class SessionStatus {
        CREATED,
        LOADING,
        READY,
        RUNNING,
        PAUSED,
        COMPLETED,
        FAILED,
        CANCELLED
    };
    
    SessionStatus get_session_status(const std::string& session_id) const;
    double get_session_progress(const std::string& session_id) const;
    
    // Results access
    ReplayEngine::ReplayMetrics get_session_metrics(const std::string& session_id) const;
    ComplianceValidator::ComplianceReport get_compliance_report(const std::string& session_id) const;
    
    // Batch processing
    std::vector<std::string> run_batch_sessions(const std::vector<ReplayConfig>& configs);
    bool compare_session_results(const std::string& session_id1, 
                                const std::string& session_id2,
                                const std::string& comparison_report_path);
    
    // Event notifications
    using SessionEventCallback = std::function<void(const std::string& session_id, 
                                                   SessionStatus status,
                                                   const std::string& message)>;
    void set_session_event_callback(SessionEventCallback callback);
    
private:
    class SessionManagerImpl;
    std::unique_ptr<SessionManagerImpl> pimpl_;
};

/**
 * @brief Deterministic random number generator for replay
 */
class ReplayRandomGenerator {
public:
    explicit ReplayRandomGenerator(uint64_t seed);
    
    uint32_t next_uint32();
    uint64_t next_uint64();
    double next_double();           // [0.0, 1.0)
    double next_gaussian();         // Normal distribution
    
    void set_seed(uint64_t seed);
    uint64_t get_seed() const;
    
    // Save/restore state for exact replay
    std::vector<uint8_t> save_state() const;
    void restore_state(const std::vector<uint8_t>& state);
    
private:
    uint64_t seed_;
    uint64_t state_;
};

/**
 * @brief Data integrity checker
 */
class DataIntegrityChecker {
public:
    explicit DataIntegrityChecker();
    ~DataIntegrityChecker();
    
    // Checksum validation
    bool validate_file_integrity(const std::string& file_path);
    bool validate_event_checksums(const std::vector<EventStream::Event>& events);
    
    // Data consistency checks
    bool check_timestamp_ordering(const std::vector<EventStream::Event>& events);
    bool check_sequence_continuity(const std::vector<EventStream::Event>& events);
    bool check_data_completeness(const std::vector<std::string>& required_fields,
                                const std::vector<EventStream::Event>& events);
    
    // Corruption detection
    struct CorruptionReport {
        bool has_corruption;
        std::vector<std::string> corruption_details;
        std::vector<size_t> corrupted_event_indices;
        double corruption_percentage;
    };
    
    CorruptionReport detect_corruption(const std::vector<EventStream::Event>& events);
    
    // Repair utilities
    bool attempt_data_repair(std::vector<EventStream::Event>& events);
    bool interpolate_missing_data(std::vector<EventStream::Event>& events);
    
private:
    class IntegrityImpl;
    std::unique_ptr<IntegrityImpl> pimpl_;
};

} // namespace hfx::hft
