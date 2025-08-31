/**
 * @file strategy_engine.hpp
 * @brief Quantum-inspired multi-agent strategy engine for DeFi HFT
 * @version 1.0.0
 * 
 * Advanced strategy engine incorporating 2024-2025 breakthrough techniques:
 * - Quantum-inspired AI algorithms with superposition and entanglement
 * - Multi-agent collaborative decision making (AlphaAgents framework)
 * - Memory-augmented context-aware reinforcement learning (MacroHFT)
 * - Hybrid VAR+Neural network models for Order Flow Imbalance prediction
 * - Real-time anomaly detection with sliding window Transformers
 */

#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

#include "event_engine.hpp"
#include "network_manager.hpp"

#ifdef __APPLE__
// Note: CoreML, Metal frameworks conflict with -fno-rtti flag
// For HFT, we use optimized C++ implementations instead
#include <Accelerate/Accelerate.h>  // BLAS/LAPACK still usable
#endif

namespace hfx::strat {

using TimeStamp = std::chrono::time_point<std::chrono::high_resolution_clock>;
using Price = double;
using Volume = double;
using SignalId = std::uint64_t;

/**
 * @enum StrategyType
 * @brief Types of trading strategies
 */
enum class StrategyType : std::uint8_t {
    ORACLE_ARBITRAGE,
    SEQUENCER_QUEUE_ALPHA,
    LIQUIDITY_EPOCH_BREATHING,
    MEV_FRONTRUN_PROTECTION,
    CROSS_CHAIN_ARBITRAGE,
    FLASH_LOAN_ARBITRAGE
};

/**
 * @enum SignalStrength
 * @brief Signal confidence levels
 */
enum class SignalStrength : std::uint8_t {
    WEAK = 1,
    MEDIUM = 2,
    STRONG = 3,
    VERY_STRONG = 4,
    CRITICAL = 5
};

/**
 * @struct MarketData
 * @brief Real-time market data structure
 */
struct alignas(64) MarketData {
    std::uint64_t timestamp_ns;
    std::string symbol;
    Price bid;
    Price ask;
    Price mid;
    Volume bid_size;
    Volume ask_size;
    Price last_trade_price;
    Volume last_trade_size;
    
    // DeFi-specific fields
    Price oracle_price;
    std::uint64_t block_number;
    std::uint64_t gas_price;
    double inclusion_probability;
    
    MarketData() = default;
} __attribute__((packed));

/**
 * @struct TradingSignal
 * @brief Generated trading signal with quantum-enhanced confidence
 */
struct alignas(64) TradingSignal {
    SignalId id;
    StrategyType strategy;
    TimeStamp generated_time;
    std::string asset_pair;
    Price entry_price;
    Price target_price;
    Price stop_loss;
    Volume size;
    SignalStrength strength;
    double confidence;  // Quantum-enhanced probability [0,1]
    double expected_pnl;
    std::chrono::milliseconds expected_duration;
    std::string rationale;  // AI-generated explanation
    
    // Multi-agent consensus
    std::uint8_t agent_votes;  // Bitmap of agent approvals
    double consensus_score;    // Multi-agent agreement level
    
    TradingSignal() = default;
    TradingSignal(StrategyType strat, const std::string& pair, 
                  Price entry, Volume vol, SignalStrength str)
        : id(0), strategy(strat), 
          generated_time(std::chrono::high_resolution_clock::now()),
          asset_pair(pair), entry_price(entry), size(vol), strength(str),
          confidence(0.0), expected_pnl(0.0), agent_votes(0), consensus_score(0.0) {}
} __attribute__((packed));

/**
 * @struct QuantumState
 * @brief Quantum-inspired state representation for parallel strategy evaluation
 */
struct QuantumState {
    std::vector<double> amplitudes;     // Superposition amplitudes
    std::vector<double> phases;         // Quantum phases
    std::vector<std::vector<double>> entanglement_matrix;  // Agent interactions
    double coherence_time;              // State stability duration
    
    QuantumState(std::size_t num_agents = 4) 
        : amplitudes(num_agents, 1.0/std::sqrt(num_agents)),
          phases(num_agents, 0.0),
          entanglement_matrix(num_agents, std::vector<double>(num_agents, 0.0)),
          coherence_time(1.0) {}
};

/**
 * @class StrategyEngine
 * @brief Advanced multi-agent strategy engine with quantum-inspired optimization
 * 
 * Features:
 * - Multi-agent collaborative decision making (Fundamental, Technical, Risk agents)
 * - Quantum-inspired parallel strategy evaluation
 * - Memory-augmented reinforcement learning with market context awareness
 * - Real-time anomaly detection using staged sliding window Transformers
 * - Apple Neural Engine acceleration for ML inference
 * - Integration with quantitative finance libraries (Zipline, empyrical, etc.)
 */
class StrategyEngine {
public:
    using SignalCallback = std::function<bool(const TradingSignal&)>;  // Returns true if risk approved
    using MarketDataCallback = std::function<void(const MarketData&)>;

    StrategyEngine();
    ~StrategyEngine();

    // Non-copyable, non-movable
    StrategyEngine(const StrategyEngine&) = delete;
    StrategyEngine& operator=(const StrategyEngine&) = delete;
    StrategyEngine(StrategyEngine&&) = delete;
    StrategyEngine& operator=(StrategyEngine&&) = delete;

    /**
     * @brief Initialize the strategy engine with AI models
     * @return true on success
     */
    bool initialize();

    /**
     * @brief Shutdown and cleanup resources
     */
    void shutdown();

    /**
     * @brief Process incoming market data through all strategies
     * @param market_data Real-time market update
     */
    void process_market_data(const MarketData& market_data);

    /**
     * @brief Set callback for generated trading signals
     * @param callback Function to call for risk validation
     */
    void set_risk_callback(SignalCallback callback) {
        signal_callback_ = std::move(callback);
    }

    /**
     * @brief Enable/disable a specific strategy
     * @param strategy Strategy type
     * @param enabled Whether to enable
     */
    void set_strategy_enabled(StrategyType strategy, bool enabled);

    /**
     * @brief Update strategy parameters dynamically
     * @param strategy Strategy type
     * @param parameters Key-value parameter map
     */
    void update_strategy_parameters(StrategyType strategy, 
                                  const std::unordered_map<std::string, double>& parameters);

    /**
     * @brief Get strategy performance metrics
     */
    struct StrategyMetrics {
        std::uint64_t signals_generated{0};
        std::uint64_t signals_executed{0};
        double total_pnl{0.0};
        double win_rate{0.0};
        double avg_holding_time_ms{0.0};
        double sharpe_ratio{0.0};
        double max_drawdown{0.0};
        double quantum_coherence{0.0};      // Average quantum state coherence
        double multi_agent_consensus{0.0};  // Average agent agreement
    };

    StrategyMetrics get_strategy_metrics(StrategyType strategy) const;

    /**
     * @brief Get overall engine statistics
     */
    struct EngineStatistics {
        std::uint64_t total_market_updates{0};
        std::uint64_t total_signals_generated{0};
        std::uint64_t anomalies_detected{0};
        double avg_processing_latency_ns{0.0};
        double ml_inference_time_ns{0.0};
        double quantum_optimization_time_ns{0.0};
    };

    EngineStatistics get_engine_statistics() const;

    /**
     * @brief Check if engine is running
     */
    bool is_running() const noexcept {
        return running_.load(std::memory_order_acquire);
    }

private:
    class MultiAgentSystem;      // Forward declaration
    class QuantumOptimizer;      // Forward declaration
    class AnomalyDetector;       // Forward declaration
    class MemoryAugmentedRL;     // Forward declaration
    
    std::atomic<bool> running_{false};
    
    // Core AI systems
    std::unique_ptr<MultiAgentSystem> multi_agent_system_;
    std::unique_ptr<QuantumOptimizer> quantum_optimizer_;
    std::unique_ptr<AnomalyDetector> anomaly_detector_;
    std::unique_ptr<MemoryAugmentedRL> reinforcement_learner_;

    // Strategy implementations
    std::unordered_map<StrategyType, bool> strategy_enabled_;
    
    // Signal callback
    SignalCallback signal_callback_;

    // Apple-specific ML resources (placeholder for future implementation)
#ifdef __APPLE__
    // Metal resources would go here (requires Objective-C++ compilation)
    // For HFT, we use optimized C++ implementations instead to avoid RTTI overhead
    std::unique_ptr<class AppleMLAccelerator> ml_accelerator_;
#endif

    // Performance tracking
    mutable std::atomic<std::uint64_t> total_market_updates_{0};
    mutable std::atomic<std::uint64_t> total_signals_generated_{0};
    mutable std::atomic<std::uint64_t> anomalies_detected_{0};
    mutable std::atomic<std::uint64_t> total_processing_time_ns_{0};

    // Market state tracking
    std::unordered_map<std::string, MarketData> latest_market_data_;
    QuantumState current_quantum_state_;

    /**
     * @brief Initialize Apple-specific ML acceleration
     */
    bool initialize_apple_ml();

    /**
     * @brief Initialize Python quantitative libraries integration
     */
    bool initialize_quantlib_integration();

    /**
     * @brief Run quantum-inspired optimization on market state
     * @param market_data Current market conditions
     * @return Optimized quantum state
     */
    QuantumState run_quantum_optimization(const MarketData& market_data);

    /**
     * @brief Execute multi-agent decision making process
     * @param market_data Market input
     * @param quantum_state Current quantum state  
     * @return Generated signals with consensus scores
     */
    std::vector<TradingSignal> run_multi_agent_analysis(
        const MarketData& market_data, const QuantumState& quantum_state);

    /**
     * @brief Check for market anomalies using Transformer models
     * @param market_data Current market data
     * @return true if anomaly detected
     */
    bool detect_market_anomaly(const MarketData& market_data);

    /**
     * @brief Update reinforcement learning model with market feedback
     * @param signal Executed signal
     * @param outcome Trade outcome (PnL, duration, etc.)
     */
    void update_rl_model(const TradingSignal& signal, double outcome);

    /**
     * @brief Execute specific strategy
     * @param strategy Strategy type to run
     * @param market_data Market input
     * @return Generated signals (if any)
     */
    std::vector<TradingSignal> execute_strategy(StrategyType strategy, 
                                              const MarketData& market_data);

    /**
     * @brief Oracle arbitrage strategy implementation
     */
    std::vector<TradingSignal> execute_oracle_arbitrage(const MarketData& market_data);

    /**
     * @brief Sequencer queue alpha strategy implementation  
     */
    std::vector<TradingSignal> execute_sequencer_queue_alpha(const MarketData& market_data);

    /**
     * @brief Liquidity epoch breathing strategy implementation
     */
    std::vector<TradingSignal> execute_liquidity_epoch_breathing(const MarketData& market_data);

    /**
     * @brief Validate and enhance signal with quantum confidence
     * @param signal Raw signal to enhance
     * @param quantum_state Current quantum state
     * @return Enhanced signal with quantum confidence
     */
    TradingSignal enhance_signal_with_quantum_confidence(
        const TradingSignal& signal, const QuantumState& quantum_state);

    /**
     * @brief Update performance metrics
     * @param processing_time_ns Time taken to process market data
     */
    void update_performance_metrics(std::uint64_t processing_time_ns);
};

/**
 * @brief Strategy configuration for different market conditions
 */
struct StrategyConfig {
    // Oracle arbitrage parameters
    struct {
        double min_skew_threshold{0.001};      // 0.1% price skew minimum
        double max_position_size{100000.0};    // $100k max position
        std::chrono::milliseconds max_hold_time{5000};  // 5 second max hold
        double gas_cost_tolerance{50.0};       // Max $50 gas cost
    } oracle_arb;

    // Sequencer queue parameters  
    struct {
        double min_inclusion_prob{0.95};       // 95% inclusion probability
        double tip_optimization_factor{1.2};   // 20% tip buffer
        std::size_t max_queue_depth{100};      // Monitor top 100 txs
    } sequencer_queue;

    // Liquidity epoch parameters
    struct {
        std::chrono::seconds epoch_window{3600};  // 1 hour epochs
        double spread_threshold{0.005};           // 0.5% spread widening
        double inventory_limit{0.1};              // 10% max inventory
    } liquidity_epoch;
};

} // namespace hfx::strat
