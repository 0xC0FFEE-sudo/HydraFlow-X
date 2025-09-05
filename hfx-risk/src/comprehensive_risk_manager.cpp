/**
 * @file comprehensive_risk_manager.cpp
 * @brief Comprehensive risk management implementation
 */

#include "../include/comprehensive_risk_manager.hpp"
#include "../../hfx-log/include/simple_logger.hpp"
#include <algorithm>
#include <numeric>
#include <cmath>
#include <random>
#include <fstream>
#include <sstream>
#include <iomanip>

namespace hfx::risk {

// Helper functions
namespace {
    double calculate_volatility(const std::vector<double>& returns) {
        if (returns.size() < 2) return 0.0;
        
        double mean = std::accumulate(returns.begin(), returns.end(), 0.0) / returns.size();
        double variance = 0.0;
        
        for (double ret : returns) {
            variance += (ret - mean) * (ret - mean);
        }
        
        variance /= (returns.size() - 1);
        return std::sqrt(variance);
    }
    
    double calculate_mean(const std::vector<double>& values) {
        if (values.empty()) return 0.0;
        return std::accumulate(values.begin(), values.end(), 0.0) / values.size();
    }
    
    std::vector<double> calculate_log_returns(const std::vector<double>& prices) {
        std::vector<double> returns;
        for (size_t i = 1; i < prices.size(); ++i) {
            if (prices[i-1] > 0 && prices[i] > 0) {
                returns.push_back(std::log(prices[i] / prices[i-1]));
            }
        }
        return returns;
    }
    
    double percentile(std::vector<double> values, double p) {
        if (values.empty()) return 0.0;
        
        std::sort(values.begin(), values.end());
        size_t index = static_cast<size_t>(p * (values.size() - 1));
        return values[index];
    }
}

// HistoricalData implementation
void HistoricalData::add_price(double price) {
    prices.push_back(price);
    
    // Maintain rolling window
    if (prices.size() > max_history_size) {
        prices.erase(prices.begin());
    }
    
    calculate_returns();
}

void HistoricalData::calculate_returns() {
    if (prices.size() < 2) return;
    
    returns.clear();
    log_returns.clear();
    
    for (size_t i = 1; i < prices.size(); ++i) {
        if (prices[i-1] > 0) {
            double ret = (prices[i] - prices[i-1]) / prices[i-1];
            returns.push_back(ret);
            
            if (prices[i] > 0) {
                log_returns.push_back(std::log(prices[i] / prices[i-1]));
            }
        }
    }
    
    // Update rolling returns
    if (!returns.empty()) {
        rolling_returns.push_back(returns.back());
        if (rolling_returns.size() > max_history_size) {
            rolling_returns.pop_front();
        }
    }
}

double HistoricalData::get_volatility(size_t periods) const {
    if (returns.size() < periods) {
        return calculate_volatility(returns);
    }
    
    std::vector<double> recent_returns(returns.end() - periods, returns.end());
    return calculate_volatility(recent_returns) * std::sqrt(252); // Annualized
}

double HistoricalData::get_var(double confidence, size_t periods) const {
    if (returns.size() < periods) return 0.0;
    
    std::vector<double> recent_returns(returns.end() - periods, returns.end());
    return RiskCalculator::calculate_historical_var(recent_returns, confidence);
}

double HistoricalData::get_cvar(double confidence, size_t periods) const {
    if (returns.size() < periods) return 0.0;
    
    std::vector<double> recent_returns(returns.end() - periods, returns.end());
    return RiskCalculator::calculate_cvar(recent_returns, confidence);
}

// PerformanceTracker implementation
void PerformanceTracker::update(double current_value, double daily_pnl) {
    portfolio_values.push_back(current_value);
    daily_pnl.push_back(daily_pnl);
    
    // Update peak and trough
    if (current_value > peak_portfolio_value) {
        peak_portfolio_value = current_value;
    }
    if (current_value < trough_portfolio_value || trough_portfolio_value == 0.0) {
        trough_portfolio_value = current_value;
    }
    
    // Maintain rolling window
    const size_t max_history = 1000;
    if (portfolio_values.size() > max_history) {
        portfolio_values.pop_front();
        daily_pnl.pop_front();
    }
}

double PerformanceTracker::calculate_sharpe_ratio(size_t periods) const {
    if (daily_pnl.size() < periods) return 0.0;
    
    std::vector<double> recent_pnl(daily_pnl.end() - periods, daily_pnl.end());
    return RiskCalculator::calculate_sharpe_ratio(recent_pnl);
}

double PerformanceTracker::calculate_maximum_drawdown() const {
    std::vector<double> values(portfolio_values.begin(), portfolio_values.end());
    return RiskCalculator::calculate_maximum_drawdown(values);
}

double PerformanceTracker::calculate_calmar_ratio() const {
    std::vector<double> pnl(daily_pnl.begin(), daily_pnl.end());
    return RiskCalculator::calculate_calmar_ratio(pnl);
}

std::vector<double> PerformanceTracker::get_rolling_returns(size_t window) const {
    std::vector<double> rolling_returns;
    
    if (portfolio_values.size() < window + 1) return rolling_returns;
    
    for (size_t i = window; i < portfolio_values.size(); ++i) {
        double start_value = portfolio_values[i - window];
        double end_value = portfolio_values[i];
        
        if (start_value > 0) {
            double return_pct = (end_value - start_value) / start_value;
            rolling_returns.push_back(return_pct);
        }
    }
    
    return rolling_returns;
}

// ComprehensiveRiskManager implementation
class ComprehensiveRiskManager::RiskManagerImpl {
public:
    RiskLimits risk_limits_;
    std::atomic<bool> running_{false};
    std::atomic<bool> real_time_monitoring_{true};
    std::chrono::milliseconds monitoring_frequency_{1000}; // 1 second
    
    // Data structures
    std::unordered_map<std::string, Position> positions_;
    std::unordered_map<std::string, MarketData> market_data_;
    std::unordered_map<std::string, HistoricalData> historical_data_;
    std::unordered_map<CircuitBreakerType, CircuitBreakerConfig> circuit_breakers_;
    std::unordered_map<CircuitBreakerType, bool> circuit_breaker_states_;
    std::unordered_set<std::string> paused_symbols_;
    
    RiskMetrics current_metrics_;
    PerformanceTracker performance_tracker_;
    std::vector<RiskAlert> active_alerts_;
    
    // Threading
    std::thread monitoring_thread_;
    std::mutex positions_mutex_;
    std::mutex market_data_mutex_;
    std::mutex alerts_mutex_;
    std::mutex metrics_mutex_;
    
    // Callbacks
    RiskAlertCallback alert_callback_;
    CircuitBreakerCallback circuit_breaker_callback_;
    PositionUpdateCallback position_update_callback_;
    MetricsUpdateCallback metrics_update_callback_;
    
    // Emergency controls
    std::atomic<bool> emergency_stop_{false};
    std::atomic<bool> auto_hedging_enabled_{false};
    std::atomic<bool> dynamic_position_sizing_enabled_{false};
    double hedge_ratio_ = 0.5;
    
    RiskManagerImpl(const RiskLimits& limits) : risk_limits_(limits) {
        initialize_default_circuit_breakers();
        performance_tracker_.inception_time = std::chrono::system_clock::now();
    }
    
    ~RiskManagerImpl() {
        stop();
    }
    
    bool start() {
        if (running_.load()) return true;
        
        running_ = true;
        
        if (real_time_monitoring_) {
            monitoring_thread_ = std::thread(&RiskManagerImpl::monitoring_worker, this);
        }
        
        HFX_LOG_INFO("[RiskManager] Started with real-time monitoring");
        return true;
    }
    
    void stop() {
        if (!running_.load()) return;
        
        running_ = false;
        
        if (monitoring_thread_.joinable()) {
            monitoring_thread_.join();
        }
        
        HFX_LOG_INFO("[RiskManager] Stopped");
    }
    
    bool validate_trade(const std::string& symbol, double quantity, double price) {
        if (emergency_stop_.load()) {
            create_alert(RiskLevel::CRITICAL, "TRADE_BLOCKED", "Trading stopped due to emergency", symbol);
            return false;
        }
        
        if (paused_symbols_.count(symbol)) {
            create_alert(RiskLevel::HIGH, "SYMBOL_PAUSED", "Symbol is paused for trading", symbol);
            return false;
        }
        
        if (risk_limits_.blacklisted_symbols.count(symbol)) {
            create_alert(RiskLevel::HIGH, "BLACKLISTED", "Symbol is blacklisted", symbol);
            return false;
        }
        
        // Check trade size limits
        double trade_value = std::abs(quantity * price);
        if (trade_value > risk_limits_.max_single_trade_usd) {
            create_alert(RiskLevel::HIGH, "TRADE_SIZE_EXCEEDED", 
                        "Trade size exceeds limit: $" + std::to_string(trade_value), symbol);
            return false;
        }
        
        // Check position size limits
        std::lock_guard<std::mutex> lock(positions_mutex_);
        auto it = positions_.find(symbol);
        if (it != positions_.end()) {
            double new_quantity = it->second.quantity + quantity;
            double new_value = std::abs(new_quantity * price);
            
            if (new_value > risk_limits_.max_position_size_usd) {
                create_alert(RiskLevel::HIGH, "POSITION_SIZE_EXCEEDED", 
                           "Position size would exceed limit: $" + std::to_string(new_value), symbol);
                return false;
            }
        }
        
        // Check portfolio limits
        if (!check_portfolio_limits(trade_value)) {
            return false;
        }
        
        // Check circuit breakers
        if (check_circuit_breakers()) {
            create_alert(RiskLevel::CRITICAL, "CIRCUIT_BREAKER", "Circuit breakers active", symbol);
            return false;
        }
        
        return true;
    }
    
    void add_position(const std::string& symbol, double quantity, double price) {
        std::lock_guard<std::mutex> lock(positions_mutex_);
        
        auto it = positions_.find(symbol);
        if (it != positions_.end()) {
            // Update existing position
            Position& pos = it->second;
            double total_cost = pos.quantity * pos.average_price + quantity * price;
            pos.quantity += quantity;
            
            if (pos.quantity != 0) {
                pos.average_price = total_cost / pos.quantity;
            }
        } else {
            // Create new position
            Position pos;
            pos.symbol = symbol;
            pos.quantity = quantity;
            pos.average_price = price;
            pos.current_price = price;
            pos.entry_time = std::chrono::system_clock::now();
            positions_[symbol] = pos;
        }
        
        update_position_metrics(symbol);
        
        if (position_update_callback_) {
            position_update_callback_(positions_[symbol]);
        }
    }
    
    void update_position(const std::string& symbol, double new_price) {
        std::lock_guard<std::mutex> lock(positions_mutex_);
        
        auto it = positions_.find(symbol);
        if (it != positions_.end()) {
            Position& pos = it->second;
            pos.current_price = new_price;
            pos.market_value = pos.quantity * new_price;
            pos.unrealized_pnl = pos.market_value - (pos.quantity * pos.average_price);
            pos.last_update = std::chrono::system_clock::now();
            
            update_position_metrics(symbol);
            
            if (position_update_callback_) {
                position_update_callback_(pos);
            }
        }
    }
    
    void close_position(const std::string& symbol, double close_price) {
        std::lock_guard<std::mutex> lock(positions_mutex_);
        
        auto it = positions_.find(symbol);
        if (it != positions_.end()) {
            Position& pos = it->second;
            pos.realized_pnl += pos.quantity * (close_price - pos.average_price);
            pos.quantity = 0;
            pos.market_value = 0;
            pos.unrealized_pnl = 0;
            
            // Remove from active positions
            positions_.erase(it);
            
            HFX_LOG_INFO("[RiskManager] Position closed: " + symbol + 
                        " P&L: $" + std::to_string(pos.realized_pnl));
        }
    }
    
    RiskMetrics calculate_risk_metrics() {
        std::lock_guard<std::mutex> positions_lock(positions_mutex_);
        std::lock_guard<std::mutex> metrics_lock(metrics_mutex_);
        
        RiskMetrics metrics;
        metrics.calculation_time = std::chrono::system_clock::now();
        
        // Calculate portfolio totals
        double total_market_value = 0.0;
        double total_unrealized_pnl = 0.0;
        double total_realized_pnl = 0.0;
        double gross_exposure = 0.0;
        double net_exposure = 0.0;
        
        std::vector<double> position_values;
        std::vector<double> position_returns;
        
        for (const auto& pair : positions_) {
            const Position& pos = pair.second;
            
            total_market_value += std::abs(pos.market_value);
            total_unrealized_pnl += pos.unrealized_pnl;
            total_realized_pnl += pos.realized_pnl;
            gross_exposure += std::abs(pos.market_value);
            net_exposure += pos.market_value;
            
            position_values.push_back(std::abs(pos.market_value));
            
            // Calculate position return
            if (pos.average_price > 0) {
                double position_return = (pos.current_price - pos.average_price) / pos.average_price;
                position_returns.push_back(position_return);
            }
        }
        
        metrics.total_value = total_market_value;
        metrics.unrealized_pnl = total_unrealized_pnl;
        metrics.realized_pnl = total_realized_pnl;
        metrics.total_pnl = total_unrealized_pnl + total_realized_pnl;
        metrics.gross_exposure = gross_exposure;
        metrics.net_exposure = net_exposure;
        
        // Calculate concentration metrics
        if (!position_values.empty()) {
            std::sort(position_values.begin(), position_values.end(), std::greater<double>());
            
            metrics.largest_position_pct = (position_values[0] / total_market_value) * 100.0;
            
            if (position_values.size() >= 5) {
                double top_5_value = std::accumulate(position_values.begin(), position_values.begin() + 5, 0.0);
                metrics.top_5_positions_pct = (top_5_value / total_market_value) * 100.0;
            }
        }
        
        // Calculate VaR and CVaR if we have enough data
        if (position_returns.size() > 10) {
            metrics.portfolio_var_95 = RiskCalculator::calculate_historical_var(position_returns, 0.95) * total_market_value;
            metrics.portfolio_var_99 = RiskCalculator::calculate_historical_var(position_returns, 0.99) * total_market_value;
            metrics.portfolio_cvar_95 = RiskCalculator::calculate_cvar(position_returns, 0.95) * total_market_value;
            metrics.portfolio_cvar_99 = RiskCalculator::calculate_cvar(position_returns, 0.99) * total_market_value;
        }
        
        // Calculate performance metrics
        if (performance_tracker_.daily_pnl.size() > 1) {
            std::vector<double> daily_pnl(performance_tracker_.daily_pnl.begin(), 
                                        performance_tracker_.daily_pnl.end());
            
            metrics.sharpe_ratio = RiskCalculator::calculate_sharpe_ratio(daily_pnl);
            metrics.sortino_ratio = RiskCalculator::calculate_sortino_ratio(daily_pnl);
            metrics.annualized_volatility = calculate_volatility(daily_pnl) * std::sqrt(252);
        }
        
        // Calculate drawdown
        metrics.maximum_drawdown = performance_tracker_.calculate_maximum_drawdown();
        if (performance_tracker_.peak_portfolio_value > 0) {
            metrics.current_drawdown = (performance_tracker_.peak_portfolio_value - total_market_value) / 
                                     performance_tracker_.peak_portfolio_value * 100.0;
        }
        
        // Calculate leverage
        if (total_market_value > 0) {
            metrics.leverage_ratio = gross_exposure / total_market_value;
        }
        
        current_metrics_ = metrics;
        
        if (metrics_update_callback_) {
            metrics_update_callback_(metrics);
        }
        
        return metrics;
    }
    
    bool check_risk_limits(const RiskMetrics& metrics) {
        bool within_limits = true;
        
        // Check portfolio value limit
        if (metrics.total_value > risk_limits_.max_portfolio_value) {
            create_alert(RiskLevel::HIGH, "PORTFOLIO_LIMIT", 
                        "Portfolio value exceeds limit: $" + std::to_string(metrics.total_value), "");
            within_limits = false;
        }
        
        // Check daily loss limit
        if (metrics.daily_pnl < -risk_limits_.max_daily_loss) {
            create_alert(RiskLevel::CRITICAL, "DAILY_LOSS_LIMIT", 
                        "Daily loss limit exceeded: $" + std::to_string(std::abs(metrics.daily_pnl)), "");
            within_limits = false;
        }
        
        // Check maximum drawdown
        if (metrics.current_drawdown > risk_limits_.max_drawdown_pct) {
            create_alert(RiskLevel::CRITICAL, "DRAWDOWN_LIMIT", 
                        "Maximum drawdown exceeded: " + std::to_string(metrics.current_drawdown) + "%", "");
            within_limits = false;
        }
        
        // Check VaR limit
        if (metrics.portfolio_var_95 > risk_limits_.max_portfolio_var) {
            create_alert(RiskLevel::HIGH, "VAR_LIMIT", 
                        "Portfolio VaR exceeds limit: $" + std::to_string(metrics.portfolio_var_95), "");
            within_limits = false;
        }
        
        // Check leverage limit
        if (metrics.leverage_ratio > risk_limits_.max_leverage_ratio) {
            create_alert(RiskLevel::HIGH, "LEVERAGE_LIMIT", 
                        "Leverage ratio exceeds limit: " + std::to_string(metrics.leverage_ratio), "");
            within_limits = false;
        }
        
        // Check concentration limit
        if (metrics.largest_position_pct > risk_limits_.max_concentration_pct) {
            create_alert(RiskLevel::HIGH, "CONCENTRATION_LIMIT", 
                        "Position concentration exceeds limit: " + std::to_string(metrics.largest_position_pct) + "%", "");
            within_limits = false;
        }
        
        return within_limits;
    }
    
    void emergency_liquidate_all(const std::string& reason) {
        emergency_stop_ = true;
        
        create_alert(RiskLevel::EMERGENCY, "EMERGENCY_LIQUIDATION", 
                    "Emergency liquidation triggered: " + reason, "");
        
        std::lock_guard<std::mutex> lock(positions_mutex_);
        
        for (auto& pair : positions_) {
            Position& pos = pair.second;
            if (pos.quantity != 0) {
                // In real implementation, would execute liquidation orders
                HFX_LOG_CRITICAL("[RiskManager] EMERGENCY: Liquidating position " + pos.symbol + 
                                " qty: " + std::to_string(pos.quantity));
                
                // Simulate liquidation
                pos.realized_pnl += pos.unrealized_pnl;
                pos.quantity = 0;
                pos.market_value = 0;
                pos.unrealized_pnl = 0;
            }
        }
        
        positions_.clear();
        
        HFX_LOG_CRITICAL("[RiskManager] Emergency liquidation completed: " + reason);
    }

private:
    void initialize_default_circuit_breakers() {
        // Portfolio drawdown circuit breaker
        CircuitBreakerConfig drawdown_breaker;
        drawdown_breaker.type = CircuitBreakerType::PORTFOLIO_DRAWDOWN;
        drawdown_breaker.trigger_threshold = 15.0; // 15% drawdown
        drawdown_breaker.reset_threshold = 10.0;   // 10% drawdown
        drawdown_breaker.emergency_liquidation = true;
        circuit_breakers_[CircuitBreakerType::PORTFOLIO_DRAWDOWN] = drawdown_breaker;
        circuit_breaker_states_[CircuitBreakerType::PORTFOLIO_DRAWDOWN] = false;
        
        // Daily loss circuit breaker
        CircuitBreakerConfig daily_loss_breaker;
        daily_loss_breaker.type = CircuitBreakerType::DAILY_LOSS;
        daily_loss_breaker.trigger_threshold = risk_limits_.max_daily_loss;
        daily_loss_breaker.reset_threshold = risk_limits_.max_daily_loss * 0.8;
        circuit_breakers_[CircuitBreakerType::DAILY_LOSS] = daily_loss_breaker;
        circuit_breaker_states_[CircuitBreakerType::DAILY_LOSS] = false;
        
        // Volatility circuit breaker
        CircuitBreakerConfig volatility_breaker;
        volatility_breaker.type = CircuitBreakerType::VOLATILITY;
        volatility_breaker.trigger_threshold = 0.5; // 50% volatility spike
        volatility_breaker.reset_threshold = 0.3;   // 30% volatility
        circuit_breakers_[CircuitBreakerType::VOLATILITY] = volatility_breaker;
        circuit_breaker_states_[CircuitBreakerType::VOLATILITY] = false;
    }
    
    void monitoring_worker() {
        while (running_.load()) {
            try {
                // Calculate current risk metrics
                auto metrics = calculate_risk_metrics();
                
                // Update performance tracker
                performance_tracker_.update(metrics.total_value, metrics.daily_pnl);
                
                // Check risk limits
                check_risk_limits(metrics);
                
                // Check circuit breakers
                check_and_update_circuit_breakers(metrics);
                
                // Sleep until next monitoring cycle
                std::this_thread::sleep_for(monitoring_frequency_);
                
            } catch (const std::exception& e) {
                HFX_LOG_ERROR("[RiskManager] Monitoring error: " + std::string(e.what()));
                std::this_thread::sleep_for(std::chrono::seconds(5));
            }
        }
    }
    
    void update_position_metrics(const std::string& symbol) {
        auto it = positions_.find(symbol);
        if (it == positions_.end()) return;
        
        Position& pos = it->second;
        
        // Update historical data if we have market data
        auto market_it = market_data_.find(symbol);
        if (market_it != market_data_.end()) {
            historical_data_[symbol].add_price(market_it->second.price);
            
            // Calculate position-specific metrics
            pos.volatility = historical_data_[symbol].get_volatility(30);
            pos.var_contribution = historical_data_[symbol].get_var(0.95, 30) * std::abs(pos.market_value);
        }
    }
    
    bool check_portfolio_limits(double additional_trade_value) {
        double current_portfolio_value = 0.0;
        
        {
            std::lock_guard<std::mutex> lock(positions_mutex_);
            for (const auto& pair : positions_) {
                current_portfolio_value += std::abs(pair.second.market_value);
            }
        }
        
        if (current_portfolio_value + additional_trade_value > risk_limits_.max_portfolio_value) {
            create_alert(RiskLevel::HIGH, "PORTFOLIO_LIMIT", 
                        "Portfolio limit would be exceeded", "");
            return false;
        }
        
        return true;
    }
    
    bool check_circuit_breakers() {
        for (const auto& pair : circuit_breaker_states_) {
            if (pair.second) { // Circuit breaker is triggered
                return true;
            }
        }
        return false;
    }
    
    void check_and_update_circuit_breakers(const RiskMetrics& metrics) {
        // Check portfolio drawdown circuit breaker
        auto& drawdown_state = circuit_breaker_states_[CircuitBreakerType::PORTFOLIO_DRAWDOWN];
        auto& drawdown_config = circuit_breakers_[CircuitBreakerType::PORTFOLIO_DRAWDOWN];
        
        if (!drawdown_state && metrics.current_drawdown > drawdown_config.trigger_threshold) {
            trigger_circuit_breaker(CircuitBreakerType::PORTFOLIO_DRAWDOWN, 
                                   "Portfolio drawdown exceeded: " + std::to_string(metrics.current_drawdown) + "%");
        } else if (drawdown_state && metrics.current_drawdown < drawdown_config.reset_threshold) {
            reset_circuit_breaker(CircuitBreakerType::PORTFOLIO_DRAWDOWN);
        }
        
        // Check daily loss circuit breaker
        auto& daily_loss_state = circuit_breaker_states_[CircuitBreakerType::DAILY_LOSS];
        auto& daily_loss_config = circuit_breakers_[CircuitBreakerType::DAILY_LOSS];
        
        if (!daily_loss_state && metrics.daily_pnl < -daily_loss_config.trigger_threshold) {
            trigger_circuit_breaker(CircuitBreakerType::DAILY_LOSS, 
                                   "Daily loss limit exceeded: $" + std::to_string(std::abs(metrics.daily_pnl)));
        }
        
        // Additional circuit breaker checks would go here...
    }
    
    void trigger_circuit_breaker(CircuitBreakerType type, const std::string& reason) {
        circuit_breaker_states_[type] = true;
        
        create_alert(RiskLevel::CRITICAL, "CIRCUIT_BREAKER_TRIGGERED", 
                    "Circuit breaker triggered: " + reason, "");
        
        auto& config = circuit_breakers_[type];
        if (config.emergency_liquidation) {
            emergency_liquidate_all("Circuit breaker: " + reason);
        }
        
        if (circuit_breaker_callback_) {
            circuit_breaker_callback_(type, true);
        }
        
        HFX_LOG_CRITICAL("[RiskManager] Circuit breaker triggered: " + reason);
    }
    
    void reset_circuit_breaker(CircuitBreakerType type) {
        circuit_breaker_states_[type] = false;
        
        if (circuit_breaker_callback_) {
            circuit_breaker_callback_(type, false);
        }
        
        HFX_LOG_INFO("[RiskManager] Circuit breaker reset for type: " + std::to_string(static_cast<int>(type)));
    }
    
    void create_alert(RiskLevel level, const std::string& type, const std::string& description, const std::string& symbol) {
        RiskAlert alert;
        alert.level = level;
        alert.alert_type = type;
        alert.description = description;
        alert.affected_symbol = symbol;
        alert.timestamp = std::chrono::system_clock::now();
        
        {
            std::lock_guard<std::mutex> lock(alerts_mutex_);
            active_alerts_.push_back(alert);
            
            // Keep only recent alerts
            if (active_alerts_.size() > 1000) {
                active_alerts_.erase(active_alerts_.begin());
            }
        }
        
        if (alert_callback_) {
            alert_callback_(alert);
        }
        
        // Log based on severity
        std::string log_msg = "[" + type + "] " + description + (symbol.empty() ? "" : " (" + symbol + ")");
        switch (level) {
            case RiskLevel::LOW:
            case RiskLevel::MODERATE:
                HFX_LOG_INFO("[RiskManager] " + log_msg);
                break;
            case RiskLevel::HIGH:
                HFX_LOG_WARN("[RiskManager] " + log_msg);
                break;
            case RiskLevel::CRITICAL:
            case RiskLevel::EMERGENCY:
                HFX_LOG_ERROR("[RiskManager] " + log_msg);
                break;
        }
    }
};

// ComprehensiveRiskManager public implementation
ComprehensiveRiskManager::ComprehensiveRiskManager(const RiskLimits& limits)
    : pimpl_(std::make_unique<RiskManagerImpl>(limits)) {
    HFX_LOG_INFO("[ComprehensiveRiskManager] Initialized with comprehensive risk controls");
}

ComprehensiveRiskManager::~ComprehensiveRiskManager() = default;

bool ComprehensiveRiskManager::start() {
    return pimpl_->start();
}

void ComprehensiveRiskManager::stop() {
    pimpl_->stop();
}

bool ComprehensiveRiskManager::is_running() const {
    return pimpl_->running_.load();
}

bool ComprehensiveRiskManager::validate_trade(const std::string& symbol, double quantity, double price) {
    return pimpl_->validate_trade(symbol, quantity, price);
}

void ComprehensiveRiskManager::add_position(const std::string& symbol, double quantity, double price) {
    pimpl_->add_position(symbol, quantity, price);
}

void ComprehensiveRiskManager::update_position(const std::string& symbol, double new_price) {
    pimpl_->update_position(symbol, new_price);
}

void ComprehensiveRiskManager::close_position(const std::string& symbol, double close_price) {
    pimpl_->close_position(symbol, close_price);
}

RiskMetrics ComprehensiveRiskManager::calculate_risk_metrics() {
    return pimpl_->calculate_risk_metrics();
}

RiskMetrics ComprehensiveRiskManager::get_current_risk_metrics() const {
    std::lock_guard<std::mutex> lock(pimpl_->metrics_mutex_);
    return pimpl_->current_metrics_;
}

std::vector<Position> ComprehensiveRiskManager::get_all_positions() const {
    std::lock_guard<std::mutex> lock(pimpl_->positions_mutex_);
    std::vector<Position> positions;
    
    for (const auto& pair : pimpl_->positions_) {
        positions.push_back(pair.second);
    }
    
    return positions;
}

Position ComprehensiveRiskManager::get_position(const std::string& symbol) const {
    std::lock_guard<std::mutex> lock(pimpl_->positions_mutex_);
    
    auto it = pimpl_->positions_.find(symbol);
    return (it != pimpl_->positions_.end()) ? it->second : Position{};
}

void ComprehensiveRiskManager::emergency_liquidate_all(const std::string& reason) {
    pimpl_->emergency_liquidate_all(reason);
}

void ComprehensiveRiskManager::register_alert_callback(RiskAlertCallback callback) {
    pimpl_->alert_callback_ = callback;
}

void ComprehensiveRiskManager::register_circuit_breaker_callback(CircuitBreakerCallback callback) {
    pimpl_->circuit_breaker_callback_ = callback;
}

void ComprehensiveRiskManager::register_position_update_callback(PositionUpdateCallback callback) {
    pimpl_->position_update_callback_ = callback;
}

void ComprehensiveRiskManager::register_metrics_update_callback(MetricsUpdateCallback callback) {
    pimpl_->metrics_update_callback_ = callback;
}

// RiskCalculator implementation
double RiskCalculator::calculate_historical_var(const std::vector<double>& returns, double confidence) {
    if (returns.empty()) return 0.0;
    
    std::vector<double> sorted_returns = returns;
    std::sort(sorted_returns.begin(), sorted_returns.end());
    
    size_t index = static_cast<size_t>((1.0 - confidence) * sorted_returns.size());
    return -sorted_returns[index]; // Negative because VaR is typically reported as positive loss
}

double RiskCalculator::calculate_cvar(const std::vector<double>& returns, double confidence) {
    if (returns.empty()) return 0.0;
    
    std::vector<double> sorted_returns = returns;
    std::sort(sorted_returns.begin(), sorted_returns.end());
    
    size_t var_index = static_cast<size_t>((1.0 - confidence) * sorted_returns.size());
    
    double cvar_sum = 0.0;
    for (size_t i = 0; i < var_index; ++i) {
        cvar_sum += sorted_returns[i];
    }
    
    return var_index > 0 ? -cvar_sum / var_index : 0.0;
}

double RiskCalculator::calculate_sharpe_ratio(const std::vector<double>& returns, double risk_free_rate) {
    if (returns.empty()) return 0.0;
    
    double mean_return = calculate_mean(returns);
    double volatility = calculate_volatility(returns);
    
    if (volatility == 0.0) return 0.0;
    
    return (mean_return * 252 - risk_free_rate) / (volatility * std::sqrt(252)); // Annualized
}

double RiskCalculator::calculate_sortino_ratio(const std::vector<double>& returns, double target_return) {
    if (returns.empty()) return 0.0;
    
    double mean_return = calculate_mean(returns);
    
    // Calculate downside deviation
    double downside_variance = 0.0;
    size_t downside_count = 0;
    
    for (double ret : returns) {
        if (ret < target_return) {
            downside_variance += (ret - target_return) * (ret - target_return);
            downside_count++;
        }
    }
    
    if (downside_count == 0) return std::numeric_limits<double>::infinity();
    
    double downside_deviation = std::sqrt(downside_variance / downside_count);
    return downside_deviation > 0 ? (mean_return - target_return) / downside_deviation : 0.0;
}

double RiskCalculator::calculate_maximum_drawdown(const std::vector<double>& values) {
    if (values.empty()) return 0.0;
    
    double max_drawdown = 0.0;
    double peak = values[0];
    
    for (double value : values) {
        if (value > peak) {
            peak = value;
        } else {
            double drawdown = (peak - value) / peak;
            max_drawdown = std::max(max_drawdown, drawdown);
        }
    }
    
    return max_drawdown * 100.0; // Return as percentage
}

// RiskModelFactory implementation
RiskLimits RiskModelFactory::create_conservative_limits() {
    RiskLimits limits;
    limits.max_daily_loss = 25000.0;          // $25K daily loss
    limits.max_drawdown_pct = 10.0;           // 10% max drawdown
    limits.max_position_size_pct = 5.0;       // 5% max position size
    limits.max_leverage_ratio = 1.5;          // 1.5:1 leverage
    limits.max_concentration_pct = 15.0;      // 15% max sector concentration
    limits.max_portfolio_var = 20000.0;       // $20K VaR limit
    return limits;
}

RiskLimits RiskModelFactory::create_moderate_limits() {
    RiskLimits limits;
    limits.max_daily_loss = 50000.0;          // $50K daily loss
    limits.max_drawdown_pct = 15.0;           // 15% max drawdown  
    limits.max_position_size_pct = 8.0;       // 8% max position size
    limits.max_leverage_ratio = 2.5;          // 2.5:1 leverage
    limits.max_concentration_pct = 20.0;      // 20% max sector concentration
    limits.max_portfolio_var = 40000.0;       // $40K VaR limit
    return limits;
}

RiskLimits RiskModelFactory::create_aggressive_limits() {
    RiskLimits limits;
    limits.max_daily_loss = 100000.0;         // $100K daily loss
    limits.max_drawdown_pct = 25.0;           // 25% max drawdown
    limits.max_position_size_pct = 15.0;      // 15% max position size
    limits.max_leverage_ratio = 4.0;          // 4:1 leverage
    limits.max_concentration_pct = 30.0;      // 30% max sector concentration
    limits.max_portfolio_var = 75000.0;       // $75K VaR limit
    return limits;
}

} // namespace hfx::risk
