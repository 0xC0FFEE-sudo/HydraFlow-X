/**
 * @file sentiment_to_execution_pipeline.cpp
 * @brief Implementation of complete sentiment-to-execution pipeline
 */

#include "sentiment_to_execution_pipeline.hpp"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <random>
#include <cmath>
#include <thread>
#include <chrono>
#include <iomanip>

namespace hfx::ai {

/**
 * @brief Implementation class for the sentiment-to-execution pipeline
 */
class SentimentToExecutionPipeline::PipelineImpl {
public:
    // Configuration and state
    PipelineConfig config_;
    std::atomic<bool> running_{false};
    std::atomic<bool> trading_paused_{false};
    mutable std::mutex state_mutex_;
    
    // Component references
    std::shared_ptr<SentimentEngine> sentiment_engine_;
    std::shared_ptr<hfx::ultra::SmartTradingEngine> trading_engine_;
    std::shared_ptr<hfx::ultra::MEVShield> mev_shield_;
    std::shared_ptr<hfx::ultra::V3TickEngine> v3_engine_;
    
    // Pipeline state
    std::queue<TradingSignal> signal_queue_;
    std::unordered_map<std::string, ExecutionResult> open_positions_;
    std::vector<ExecutionResult> trade_history_;
    mutable std::mutex queue_mutex_;
    mutable std::mutex positions_mutex_;
    mutable std::mutex history_mutex_;
    
    // Performance tracking
    PipelineMetrics metrics_;
    std::atomic<double> daily_pnl_{0.0};
    std::atomic<double> total_exposure_{0.0};
    std::chrono::steady_clock::time_point last_reset_time_;
    
    // Worker threads
    std::vector<std::thread> worker_threads_;
    std::thread signal_processor_thread_;
    std::thread position_monitor_thread_;
    std::thread risk_monitor_thread_;
    
    // Event callbacks
    std::vector<SignalCallback> signal_callbacks_;
    std::vector<ExecutionCallback> execution_callbacks_;
    std::vector<AlertCallback> alert_callbacks_;
    mutable std::mutex callback_mutex_;
    
    // Random generator for simulation
    mutable std::mt19937 random_generator_;
    
    explicit PipelineImpl(const PipelineConfig& config) 
        : config_(config), last_reset_time_(std::chrono::steady_clock::now()) {
        random_generator_.seed(std::chrono::steady_clock::now().time_since_epoch().count());
    }
    
    ~PipelineImpl() {
        stop();
    }
    
    bool initialize() {
        HFX_LOG_INFO("🚀 Initializing Sentiment-to-Execution Pipeline");
        
        // Validate configuration
        if (!validate_config()) {
            HFX_LOG_ERROR("❌ Invalid pipeline configuration");
            return false;
        }
        
        // Initialize metrics
        reset_metrics();
        
        HFX_LOG_INFO("✅ Pipeline initialized successfully");
        return true;
    }
    
    void start() {
        if (running_.load()) {
            HFX_LOG_INFO("⚠️  Pipeline already running");
            return;
        }
        
        running_.store(true);
        metrics_.pipeline_active.store(true);
        
        // Start worker threads
        signal_processor_thread_ = std::thread(&PipelineImpl::signal_processor_worker, this);
        position_monitor_thread_ = std::thread(&PipelineImpl::position_monitor_worker, this);
        risk_monitor_thread_ = std::thread(&PipelineImpl::risk_monitor_worker, this);
        
        // Start additional worker threads for parallel processing
        for (size_t i = 0; i < 3; ++i) {
            worker_threads_.emplace_back(&PipelineImpl::execution_worker, this, i);
        }
        
        HFX_LOG_INFO("🎯 Sentiment-to-Execution Pipeline started with " 
                  << (worker_threads_.size() + 3) << " threads" << std::endl;
        
        // Send startup alert
        send_alert("pipeline_start", "Sentiment-to-execution pipeline started successfully");
    }
    
    void stop() {
        if (!running_.load()) return;
        
        running_.store(false);
        metrics_.pipeline_active.store(false);
        
        // Wait for all threads to finish
        if (signal_processor_thread_.joinable()) {
            signal_processor_thread_.join();
        }
        if (position_monitor_thread_.joinable()) {
            position_monitor_thread_.join();
        }
        if (risk_monitor_thread_.joinable()) {
            risk_monitor_thread_.join();
        }
        
        for (auto& thread : worker_threads_) {
            if (thread.joinable()) {
                thread.join();
            }
        }
        worker_threads_.clear();
        
        HFX_LOG_INFO("🛑 Sentiment-to-execution pipeline stopped");
        send_alert("pipeline_stop", "Pipeline stopped gracefully");
    }
    
    void process_sentiment_signal(const SentimentSignal& sentiment) {
        if (!running_.load() || trading_paused_.load()) {
            return;
        }
        
        auto start_time = std::chrono::high_resolution_clock::now();
        
        // Convert sentiment to trading signal
        TradingSignal trading_signal = convert_sentiment_to_trading_signal(sentiment);
        
        // Apply filters and validation
        if (!validate_trading_signal(trading_signal)) {
            metrics_.signals_filtered.fetch_add(1);
            return;
        }
        
        // Add to processing queue
        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            signal_queue_.push(trading_signal);
        }
        
        metrics_.total_signals_generated.fetch_add(1);
        
        // Update latency metrics
        auto end_time = std::chrono::high_resolution_clock::now();
        auto latency_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time).count();
        metrics_.avg_signal_latency_ns.store(latency_ns);
        metrics_.last_signal_timestamp.store(
            std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::system_clock::now().time_since_epoch()).count());
        
        // Notify callbacks
        notify_signal_callbacks(trading_signal);
    }
    
    void reset_metrics() {
        metrics_ = PipelineMetrics{};
        daily_pnl_.store(0.0);
        total_exposure_.store(0.0);
        last_reset_time_ = std::chrono::steady_clock::now();
    }
    
    void send_alert(const std::string& alert_type, const std::string& message) {
        std::lock_guard<std::mutex> lock(callback_mutex_);
        for (const auto& callback : alert_callbacks_) {
            try {
                callback(alert_type, message);
            } catch (const std::exception& e) {
                HFX_LOG_ERROR("[ERROR] Message");
            }
        }
        
        // Also log to console
        HFX_LOG_INFO("[LOG] Message");
    }
    
    void emergency_stop_all_trading() {
        trading_paused_.store(true);
        
        // Close all open positions immediately
        std::lock_guard<std::mutex> lock(positions_mutex_);
        for (auto& [signal_id, position] : open_positions_) {
            position.realized_pnl_usd = position.unrealized_pnl_usd;
            daily_pnl_.store(daily_pnl_.load() + position.unrealized_pnl_usd);
        }
        open_positions_.clear();
        total_exposure_.store(0.0);
        
        send_alert("emergency_stop", "Emergency stop activated - all trading halted");
        HFX_LOG_INFO("🛑 EMERGENCY STOP: All trading halted");
    }
    
private:
    bool validate_config() const {
        return config_.min_sentiment_threshold >= -1.0 && 
               config_.min_sentiment_threshold <= 1.0 &&
               config_.min_confidence_threshold >= 0.0 && 
               config_.min_confidence_threshold <= 1.0 &&
               config_.max_position_size_usd > 0.0;
    }
    
    TradingSignal convert_sentiment_to_trading_signal(const SentimentSignal& sentiment) {
        TradingSignal signal;
        
        signal.symbol = sentiment.symbol;
        signal.sentiment_score = sentiment.weighted_sentiment;
        signal.confidence = std::min(1.0, std::abs(sentiment.weighted_sentiment) + sentiment.momentum * 0.3);
        signal.urgency = sentiment.momentum;
        signal.timestamp_ns = sentiment.timestamp_ns;
        
        // Determine action based on sentiment
        if (sentiment.weighted_sentiment > 0.6) {
            signal.action = TradingSignal::STRONG_BUY;
            signal.suggested_amount_usd = config_.max_position_size_usd * 0.8;
        } else if (sentiment.weighted_sentiment > 0.3) {
            signal.action = TradingSignal::BUY;
            signal.suggested_amount_usd = config_.max_position_size_usd * 0.5;
        } else if (sentiment.weighted_sentiment < -0.6) {
            signal.action = TradingSignal::STRONG_SELL;
            signal.suggested_amount_usd = config_.max_position_size_usd * 0.8;
        } else if (sentiment.weighted_sentiment < -0.3) {
            signal.action = TradingSignal::SELL;
            signal.suggested_amount_usd = config_.max_position_size_usd * 0.5;
        } else {
            signal.action = TradingSignal::HOLD;
            signal.suggested_amount_usd = 0.0;
        }
        
        // Set risk parameters based on confidence and urgency
        signal.max_slippage_bps = config_.default_slippage_tolerance_bps * (2.0 - signal.confidence);
        signal.execution_timeout_ms = config_.signal_execution_timeout_ms;
        signal.stop_loss_pct = 5.0 / signal.confidence; // Higher stop loss for lower confidence
        signal.take_profit_pct = 10.0 * signal.confidence; // Higher take profit for higher confidence
        signal.position_size_pct = std::min(20.0, signal.confidence * 30.0); // Max 20% position
        
        // Generate reasoning
        signal.reasoning = generate_signal_reasoning(sentiment, signal);
        
        // Calculate technical scores (simplified for demo)
        signal.momentum_score = std::min(1.0, sentiment.momentum);
        signal.volume_score = std::min(1.0, sentiment.volume_factor / 2.0);
        signal.liquidity_score = 0.8; // Default high liquidity assumption
        signal.mev_risk_score = 0.3; // Default moderate MEV risk
        
        return signal;
    }
    
    std::string generate_signal_reasoning(const SentimentSignal& sentiment, const TradingSignal& signal) {
        std::stringstream ss;
        ss << "Sentiment: " << std::fixed << std::setprecision(2) << sentiment.weighted_sentiment 
           << " (confidence: " << signal.confidence << "), "
           << "Momentum: " << sentiment.momentum << ", "
           << "Volume factor: " << sentiment.volume_factor << ". ";
        
        if (signal.action == TradingSignal::STRONG_BUY || signal.action == TradingSignal::BUY) {
            ss << "Strong bullish sentiment detected across sources. ";
        } else if (signal.action == TradingSignal::STRONG_SELL || signal.action == TradingSignal::SELL) {
            ss << "Strong bearish sentiment detected across sources. ";
        } else {
            ss << "Neutral sentiment, holding position. ";
        }
        
        ss << "Contributing sources: " << sentiment.contributing_scores.size();
        
        return ss.str();
    }
    
    bool validate_trading_signal(const TradingSignal& signal) {
        // Check sentiment threshold
        if (std::abs(signal.sentiment_score) < config_.min_sentiment_threshold) {
            return false;
        }
        
        // Check confidence threshold
        if (signal.confidence < config_.min_confidence_threshold) {
            return false;
        }
        
        // Check urgency threshold
        if (signal.urgency < config_.min_urgency_threshold) {
            return false;
        }
        
        // Check risk limits
        if (signal.suggested_amount_usd > config_.max_position_size_usd) {
            return false;
        }
        
        // Check total exposure
        if (total_exposure_.load() + signal.suggested_amount_usd > config_.max_total_exposure_usd) {
            return false;
        }
        
        // Check daily loss limit
        if (daily_pnl_.load() < -config_.max_daily_loss_usd) {
            return false;
        }
        
        // Check concurrent trades limit
        std::lock_guard<std::mutex> lock(positions_mutex_);
        if (open_positions_.size() >= config_.max_concurrent_trades) {
            return false;
        }
        
        return true;
    }
    
    void signal_processor_worker() {
        while (running_.load()) {
            TradingSignal signal;
            bool has_signal = false;
            
            {
                std::lock_guard<std::mutex> lock(queue_mutex_);
                if (!signal_queue_.empty()) {
                    signal = signal_queue_.front();
                    signal_queue_.pop();
                    has_signal = true;
                }
            }
            
            if (has_signal) {
                execute_trading_signal(signal);
            } else {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }
    }
    
    void execute_trading_signal(const TradingSignal& signal) {
        if (!trading_engine_) {
            HFX_LOG_ERROR("❌ Trading engine not available");
            return;
        }
        
        auto start_time = std::chrono::high_resolution_clock::now();
        
        ExecutionResult result;
        result.signal_id = generate_signal_id(signal);
        result.execution_timestamp_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
            start_time.time_since_epoch()).count();
        
        try {
            if (config_.enable_paper_trading) {
                result = execute_paper_trade(signal);
            } else {
                result = execute_real_trade(signal);
            }
            
            // Update metrics
            auto end_time = std::chrono::high_resolution_clock::now();
            auto execution_latency = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
            result.execution_latency_ms = execution_latency;
            metrics_.avg_execution_latency_ms.store(execution_latency);
            
            if (result.success) {
                metrics_.successful_trades.fetch_add(1);
                metrics_.total_volume_usd.store(metrics_.total_volume_usd.load() + (result.actual_amount * result.actual_price));
                
                // Add to open positions if it's a buy
                if (signal.action == TradingSignal::BUY || signal.action == TradingSignal::STRONG_BUY) {
                    std::lock_guard<std::mutex> lock(positions_mutex_);
                    open_positions_[result.signal_id] = result;
                    total_exposure_.store(total_exposure_.load() + (result.actual_amount * result.actual_price));
                }
            } else {
                metrics_.failed_trades.fetch_add(1);
            }
            
            metrics_.signals_executed.fetch_add(1);
            
            // Add to trade history
            {
                std::lock_guard<std::mutex> lock(history_mutex_);
                trade_history_.push_back(result);
                
                // Keep only last 1000 trades in memory
                if (trade_history_.size() > 1000) {
                    trade_history_.erase(trade_history_.begin());
                }
            }
            
            // Notify callbacks
            notify_execution_callbacks(result);
            
            // Update timestamp
            metrics_.last_execution_timestamp.store(
                std::chrono::duration_cast<std::chrono::seconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count());
            
        } catch (const std::exception& e) {
            HFX_LOG_ERROR("[ERROR] Message");
            result.success = false;
            result.error_message = e.what();
            metrics_.failed_trades.fetch_add(1);
        }
    }
    
    ExecutionResult execute_paper_trade(const TradingSignal& signal) {
        ExecutionResult result;
        
        // Simulate trade execution with realistic parameters
        result.success = (random_generator_() % 100) < 95; // 95% success rate
        result.actual_price = 0.00123 + (random_generator_() % 100 - 50) * 0.000001; // Mock price
        result.actual_amount = signal.suggested_amount_usd / result.actual_price;
        result.actual_slippage_bps = (random_generator_() % 50) + 10; // 0.1-0.5% slippage
        result.gas_cost_usd = 0.05 + (random_generator_() % 20) * 0.001; // $0.05-0.07
        result.total_cost_usd = result.actual_amount * result.actual_price + result.gas_cost_usd;
        result.transaction_hash = "0x" + std::to_string(random_generator_()) + std::to_string(result.execution_timestamp_ns);
        
        // MEV protection simulation
        result.mev_protection_used = config_.enable_mev_protection;
        if (result.mev_protection_used) {
            result.protection_method = "jito_bundle";
            result.protection_cost_usd = 0.02;
            result.total_cost_usd += result.protection_cost_usd;
        }
        
        return result;
    }
    
    ExecutionResult execute_real_trade(const TradingSignal& signal) {
        ExecutionResult result;
        
        // In a real implementation, this would:
        // 1. Use the smart trading engine to execute the trade
        // 2. Apply MEV protection via Jito bundles or Flashbots
        // 3. Monitor for confirmation
        // 4. Handle errors and retries
        
        // For now, simulate real trading
        auto trade_result = trading_engine_->buy_token(
            signal.symbol, 
            static_cast<uint64_t>(signal.suggested_amount_usd * 1000000), // Convert to lamports
            hfx::ultra::TradingMode::STANDARD_BUY
        );
        
        result.success = trade_result.successful;
        result.transaction_hash = trade_result.transaction_hash;
        result.actual_price = 0.00123; // Would get from trade result
        result.actual_amount = signal.suggested_amount_usd / result.actual_price;
        result.gas_cost_usd = trade_result.gas_used * 0.000000020; // Convert gas to USD
        result.total_cost_usd = result.actual_amount * result.actual_price + result.gas_cost_usd;
        
        if (!result.success) {
            result.error_message = "Trade execution failed";
        }
        
        return result;
    }
    
    void position_monitor_worker() {
        while (running_.load()) {
            monitor_open_positions();
            std::this_thread::sleep_for(std::chrono::milliseconds(config_.position_check_interval_ms));
        }
    }
    
    void monitor_open_positions() {
        std::lock_guard<std::mutex> lock(positions_mutex_);
        
        for (auto it = open_positions_.begin(); it != open_positions_.end();) {
            auto& [signal_id, position] = *it;
            
            // Update position P&L (simplified)
            double current_price = get_current_token_price(position.signal_id);
            double pnl = (current_price - position.actual_price) * position.actual_amount;
            position.unrealized_pnl_usd = pnl;
            
            // Check for auto take profit/stop loss
            bool should_close = false;
            if (config_.auto_take_profit && pnl > 0) {
                double profit_pct = (pnl / (position.actual_price * position.actual_amount)) * 100.0;
                if (profit_pct >= 10.0) { // 10% take profit
                    should_close = true;
                    send_alert("take_profit", "Taking profit on " + signal_id + " at " + std::to_string(profit_pct) + "%");
                }
            }
            
            if (config_.auto_stop_loss && pnl < 0) {
                double loss_pct = std::abs(pnl / (position.actual_price * position.actual_amount)) * 100.0;
                if (loss_pct >= 5.0) { // 5% stop loss
                    should_close = true;
                    send_alert("stop_loss", "Stop loss triggered on " + signal_id + " at -" + std::to_string(loss_pct) + "%");
                }
            }
            
            if (should_close) {
                // Close position (simplified)
                position.realized_pnl_usd = pnl;
                daily_pnl_.store(daily_pnl_.load() + pnl);
                total_exposure_.store(total_exposure_.load() - (position.actual_price * position.actual_amount));
                
                // Move to trade history
                {
                    std::lock_guard<std::mutex> history_lock(history_mutex_);
                    trade_history_.push_back(position);
                }
                
                it = open_positions_.erase(it);
            } else {
                ++it;
            }
        }
        
        // Update current open positions metric
        metrics_.current_open_positions.store(open_positions_.size());
    }
    
    void risk_monitor_worker() {
        while (running_.load()) {
            monitor_risk_limits();
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    }
    
    void monitor_risk_limits() {
        double current_daily_pnl = daily_pnl_.load();
        double current_exposure = total_exposure_.load();
        
        // Check daily loss limit
        if (current_daily_pnl < -config_.max_daily_loss_usd) {
            emergency_stop_all_trading();
            send_alert("risk_limit", "Daily loss limit reached: $" + std::to_string(-current_daily_pnl));
        }
        
        // Check total exposure limit
        if (current_exposure > config_.max_total_exposure_usd) {
            trading_paused_.store(true);
            send_alert("risk_limit", "Total exposure limit reached: $" + std::to_string(current_exposure));
        }
        
        // Update risk metrics
        metrics_.total_pnl_usd.store(current_daily_pnl);
        
        // Calculate win rate
        double total_trades = metrics_.successful_trades.load() + metrics_.failed_trades.load();
        if (total_trades > 0) {
            metrics_.win_rate_pct.store((metrics_.successful_trades.load() / total_trades) * 100.0);
        }
    }
    
    void execution_worker(size_t worker_id) {
        while (running_.load()) {
            // Additional processing can be done here
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
    
    std::string generate_signal_id(const TradingSignal& signal) {
        return signal.symbol + "_" + std::to_string(signal.timestamp_ns);
    }
    
    double get_current_token_price(const std::string& signal_id) const {
        // Mock price movement for demo
        return 0.00123 + (random_generator_() % 200 - 100) * 0.000001;
    }
    

    
    void notify_signal_callbacks(const TradingSignal& signal) {
        std::lock_guard<std::mutex> lock(callback_mutex_);
        for (const auto& callback : signal_callbacks_) {
            try {
                callback(signal);
            } catch (const std::exception& e) {
                HFX_LOG_ERROR("[ERROR] Message");
            }
        }
    }
    
    void notify_execution_callbacks(const ExecutionResult& result) {
        std::lock_guard<std::mutex> lock(callback_mutex_);
        for (const auto& callback : execution_callbacks_) {
            try {
                callback(result);
            } catch (const std::exception& e) {
                HFX_LOG_ERROR("[ERROR] Message");
            }
        }
    }
};

// Main class implementation
SentimentToExecutionPipeline::SentimentToExecutionPipeline(const PipelineConfig& config)
    : pimpl_(std::make_unique<PipelineImpl>(config)) {}

SentimentToExecutionPipeline::~SentimentToExecutionPipeline() {
    if (pimpl_) {
        pimpl_->stop();
    }
}

bool SentimentToExecutionPipeline::initialize() {
    return pimpl_->initialize();
}

void SentimentToExecutionPipeline::start() {
    pimpl_->start();
}

void SentimentToExecutionPipeline::stop() {
    pimpl_->stop();
}

void SentimentToExecutionPipeline::shutdown() {
    pimpl_->stop();
}

void SentimentToExecutionPipeline::set_sentiment_engine(std::shared_ptr<SentimentEngine> engine) {
    pimpl_->sentiment_engine_ = engine;
}

void SentimentToExecutionPipeline::set_trading_engine(std::shared_ptr<hfx::ultra::SmartTradingEngine> engine) {
    pimpl_->trading_engine_ = engine;
}

void SentimentToExecutionPipeline::set_mev_shield(std::shared_ptr<hfx::ultra::MEVShield> shield) {
    pimpl_->mev_shield_ = shield;
}

void SentimentToExecutionPipeline::set_v3_engine(std::shared_ptr<hfx::ultra::V3TickEngine> engine) {
    pimpl_->v3_engine_ = engine;
}

void SentimentToExecutionPipeline::process_sentiment_signal(const SentimentSignal& sentiment) {
    pimpl_->process_sentiment_signal(sentiment);
}

void SentimentToExecutionPipeline::manual_trading_signal(const TradingSignal& signal) {
    std::lock_guard<std::mutex> lock(pimpl_->queue_mutex_);
    pimpl_->signal_queue_.push(signal);
}

void SentimentToExecutionPipeline::update_config(const PipelineConfig& config) {
    std::lock_guard<std::mutex> lock(pimpl_->state_mutex_);
    pimpl_->config_ = config;
}

PipelineConfig SentimentToExecutionPipeline::get_config() const {
    std::lock_guard<std::mutex> lock(pimpl_->state_mutex_);
    return pimpl_->config_;
}

void SentimentToExecutionPipeline::emergency_stop_all_trading() {
    pimpl_->emergency_stop_all_trading();
}

void SentimentToExecutionPipeline::pause_trading(const std::string& reason) {
    pimpl_->trading_paused_.store(true);
    pimpl_->send_alert("trading_paused", "Trading paused: " + reason);
}

void SentimentToExecutionPipeline::resume_trading() {
    pimpl_->trading_paused_.store(false);
    pimpl_->send_alert("trading_resumed", "Trading resumed");
}

std::vector<ExecutionResult> SentimentToExecutionPipeline::get_open_positions() const {
    std::lock_guard<std::mutex> lock(pimpl_->positions_mutex_);
    std::vector<ExecutionResult> positions;
    for (const auto& [signal_id, position] : pimpl_->open_positions_) {
        positions.push_back(position);
    }
    return positions;
}

std::vector<ExecutionResult> SentimentToExecutionPipeline::get_trade_history(uint32_t lookback_hours) const {
    std::lock_guard<std::mutex> lock(pimpl_->history_mutex_);
    
    auto cutoff_time = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count() - 
        (lookback_hours * 3600ULL * 1000000000ULL);
    
    std::vector<ExecutionResult> filtered_history;
    for (const auto& trade : pimpl_->trade_history_) {
        if (trade.execution_timestamp_ns >= cutoff_time) {
            filtered_history.push_back(trade);
        }
    }
    
    return filtered_history;
}

double SentimentToExecutionPipeline::get_portfolio_value() const {
    return pimpl_->total_exposure_.load();
}

double SentimentToExecutionPipeline::get_unrealized_pnl() const {
    std::lock_guard<std::mutex> lock(pimpl_->positions_mutex_);
    double total_unrealized = 0.0;
    for (const auto& [signal_id, position] : pimpl_->open_positions_) {
        total_unrealized += position.unrealized_pnl_usd;
    }
    return total_unrealized;
}

double SentimentToExecutionPipeline::get_realized_pnl() const {
    return pimpl_->daily_pnl_.load();
}

void SentimentToExecutionPipeline::get_metrics(PipelineMetrics& metrics_out) const {
    metrics_out = pimpl_->metrics_;
}

void SentimentToExecutionPipeline::reset_metrics() {
    pimpl_->reset_metrics();
}

void SentimentToExecutionPipeline::register_signal_callback(SignalCallback callback) {
    std::lock_guard<std::mutex> lock(pimpl_->callback_mutex_);
    pimpl_->signal_callbacks_.push_back(callback);
}

void SentimentToExecutionPipeline::register_execution_callback(ExecutionCallback callback) {
    std::lock_guard<std::mutex> lock(pimpl_->callback_mutex_);
    pimpl_->execution_callbacks_.push_back(callback);
}

void SentimentToExecutionPipeline::register_alert_callback(AlertCallback callback) {
    std::lock_guard<std::mutex> lock(pimpl_->callback_mutex_);
    pimpl_->alert_callbacks_.push_back(callback);
}

std::string SentimentToExecutionPipeline::generate_performance_report() const {
    std::stringstream ss;
    PipelineMetrics metrics;
    get_metrics(metrics);
    
    ss << "=== SENTIMENT-TO-EXECUTION PIPELINE PERFORMANCE REPORT ===\n";
    ss << "Total Signals Generated: " << metrics.total_signals_generated.load() << "\n";
    ss << "Signals Executed: " << metrics.signals_executed.load() << "\n";
    ss << "Signals Filtered: " << metrics.signals_filtered.load() << "\n";
    ss << "Successful Trades: " << metrics.successful_trades.load() << "\n";
    ss << "Failed Trades: " << metrics.failed_trades.load() << "\n";
    ss << "Win Rate: " << std::fixed << std::setprecision(1) << metrics.win_rate_pct.load() << "%\n";
    ss << "Total PnL: $" << std::fixed << std::setprecision(2) << metrics.total_pnl_usd.load() << "\n";
    ss << "Total Volume: $" << std::fixed << std::setprecision(2) << metrics.total_volume_usd.load() << "\n";
    ss << "Current Open Positions: " << metrics.current_open_positions.load() << "\n";
    ss << "Average Execution Latency: " << metrics.avg_execution_latency_ms.load() << "ms\n";
    ss << "Average Signal Latency: " << metrics.avg_signal_latency_ns.load() / 1000000.0 << "ms\n";
    ss << "Pipeline Active: " << (metrics.pipeline_active.load() ? "Yes" : "No") << "\n";
    
    return ss.str();
}

// Factory implementations
PipelineConfig PipelineConfigFactory::create_conservative_config() {
    PipelineConfig config;
    config.min_sentiment_threshold = 0.6;
    config.min_confidence_threshold = 0.8;
    config.max_position_size_usd = 100.0;
    config.max_total_exposure_usd = 1000.0;
    config.max_daily_loss_usd = 50.0;
    config.max_concurrent_trades = 2;
    config.enable_paper_trading = true;
    return config;
}

PipelineConfig PipelineConfigFactory::create_aggressive_config() {
    PipelineConfig config;
    config.min_sentiment_threshold = 0.2;
    config.min_confidence_threshold = 0.4;
    config.max_position_size_usd = 5000.0;
    config.max_total_exposure_usd = 50000.0;
    config.max_daily_loss_usd = 2000.0;
    config.max_concurrent_trades = 10;
    config.enable_paper_trading = false;
    return config;
}

PipelineConfig PipelineConfigFactory::create_memecoin_config() {
    PipelineConfig config;
    config.min_sentiment_threshold = 0.4;
    config.min_confidence_threshold = 0.5;
    config.min_urgency_threshold = 0.6;
    config.max_position_size_usd = 500.0;
    config.max_total_exposure_usd = 5000.0;
    config.max_daily_loss_usd = 300.0;
    config.max_concurrent_trades = 8;
    config.default_slippage_tolerance_bps = 200; // 2% for memecoins
    config.enable_mev_protection = true;
    config.enabled_chains = {"solana"};
    config.enabled_dexes = {"raydium", "jupiter"};
    return config;
}

PipelineConfig PipelineConfigFactory::create_paper_trading_config() {
    PipelineConfig config;
    config.enable_paper_trading = true;
    config.max_position_size_usd = 1000.0;
    config.max_total_exposure_usd = 10000.0;
    config.max_daily_loss_usd = 500.0;
    return config;
}

} // namespace hfx::ai
