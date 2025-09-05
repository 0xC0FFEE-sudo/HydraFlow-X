/**
 * @file policy_engine.cpp
 * @brief Policy engine implementation stub
 */

#include "policy_engine.hpp"
#include <iostream>

namespace hfx::hft {

// ============================================================================
// Position Size Policy Implementation
// ============================================================================

PositionSizePolicy::PositionSizePolicy(const Config& config) : config_(config) {}

bool PositionSizePolicy::evaluate(const OrderDetails& order, const MarketContext& market,
                                 const PortfolioState& portfolio, PolicyResult& result) {
    // Calculate position size as percentage of capital
    double order_value = std::abs(order.quantity * order.price);
    double position_percent = (order_value / portfolio.total_capital) * 100.0;
    
    if (position_percent > config_.max_single_order_percent) {
        result.set_violation(get_policy_id(), ViolationSeverity::ERROR, 
                           "Order size exceeds maximum allowed percentage");
        return false;
    }
    
    // Check total symbol exposure
    auto it = portfolio.exposures.find(order.symbol);
    double current_exposure = (it != portfolio.exposures.end()) ? it->second : 0.0;
    double new_exposure = (current_exposure + order_value) / portfolio.total_capital * 100.0;
    
    if (new_exposure > config_.max_symbol_exposure) {
        result.set_violation(get_policy_id(), ViolationSeverity::ERROR,
                           "Symbol exposure would exceed maximum allowed");
        return false;
    }
    
    return true;
}

void PositionSizePolicy::update_parameters(const std::unordered_map<std::string, double>& params) {
    if (params.find("max_position_percent") != params.end()) {
        config_.max_position_percent = params.at("max_position_percent");
    }
    if (params.find("max_single_order_percent") != params.end()) {
        config_.max_single_order_percent = params.at("max_single_order_percent");
    }
}

std::unordered_map<std::string, double> PositionSizePolicy::get_parameters() const {
    return {
        {"max_position_percent", config_.max_position_percent},
        {"max_single_order_percent", config_.max_single_order_percent},
        {"max_symbol_exposure", config_.max_symbol_exposure}
    };
}

std::string PositionSizePolicy::get_policy_description() const {
    return "Enforces maximum position sizes and symbol exposure limits";
}

std::string PositionSizePolicy::get_last_violation_details() const {
    return last_violation_;
}

// ============================================================================
// Price Deviation Policy Implementation
// ============================================================================

PriceDeviationPolicy::PriceDeviationPolicy(const Config& config) : config_(config) {}

bool PriceDeviationPolicy::evaluate(const OrderDetails& order, const MarketContext& market,
                                   const PortfolioState& portfolio, PolicyResult& result) {
    double reference_price = calculate_reference_price(market);
    double max_deviation = calculate_max_deviation(market);
    
    double price_deviation = std::abs(order.price - reference_price) / reference_price * 100.0;
    
    if (price_deviation > max_deviation) {
        result.set_violation(get_policy_id(), ViolationSeverity::ERROR,
                           "Order price deviates too much from reference price");
        return false;
    }
    
    return true;
}

void PriceDeviationPolicy::update_parameters(const std::unordered_map<std::string, double>& params) {
    if (params.find("max_deviation_percent") != params.end()) {
        config_.max_deviation_percent = params.at("max_deviation_percent");
    }
}

std::unordered_map<std::string, double> PriceDeviationPolicy::get_parameters() const {
    return {
        {"max_deviation_percent", config_.max_deviation_percent},
        {"volatility_multiplier", config_.volatility_multiplier}
    };
}

std::string PriceDeviationPolicy::get_policy_description() const {
    return "Prevents fat finger trades by limiting price deviation from reference";
}

std::string PriceDeviationPolicy::get_last_violation_details() const {
    return last_violation_;
}

double PriceDeviationPolicy::calculate_reference_price(const MarketContext& market) const {
    if (config_.reference_price_type == "LAST") {
        return market.current_price;
    } else if (config_.reference_price_type == "VWAP") {
        return market.reference_price;
    } else {
        // MID price
        return market.current_price; // Simplified
    }
}

double PriceDeviationPolicy::calculate_max_deviation(const MarketContext& market) const {
    double base_deviation = config_.max_deviation_percent;
    
    if (config_.use_dynamic_thresholds) {
        // Adjust for volatility
        base_deviation *= (1.0 + market.volatility_1h * config_.volatility_multiplier);
    }
    
    return base_deviation;
}

// ============================================================================
// Policy Engine Implementation
// ============================================================================

class PolicyEngine::PolicyEngineImpl {
public:
    EngineConfig config_;
    std::unordered_map<uint32_t, std::unique_ptr<IPolicy>> policies_;
    std::unordered_map<uint32_t, bool> policy_enabled_;
    std::atomic<bool> emergency_stopped_{false};
    std::atomic<bool> audit_enabled_{false};
    std::vector<AuditEntry> audit_trail_;
    mutable PolicyMetrics metrics_;
    std::mutex policies_mutex_;
    std::mutex audit_mutex_;
    
    PolicyEngineImpl(const EngineConfig& config) : config_(config) {}
    
    void add_policy(std::unique_ptr<IPolicy> policy) {
        std::lock_guard<std::mutex> lock(policies_mutex_);
        uint32_t policy_id = policy->get_policy_id();
        policies_[policy_id] = std::move(policy);
        policy_enabled_[policy_id] = true;
        
        HFX_LOG_INFO("[LOG] Message");
    }
    
    void remove_policy(uint32_t policy_id) {
        std::lock_guard<std::mutex> lock(policies_mutex_);
        policies_.erase(policy_id);
        policy_enabled_.erase(policy_id);
        
        HFX_LOG_INFO("[LOG] Message");
    }
    
    void enable_policy(uint32_t policy_id, bool enabled) {
        std::lock_guard<std::mutex> lock(policies_mutex_);
        auto it = policy_enabled_.find(policy_id);
        if (it != policy_enabled_.end()) {
            it->second = enabled;
        }
    }
    
    PolicyResult evaluate_order(const OrderDetails& order,
                               const MarketContext& market,
                               const PortfolioState& portfolio) {
        auto start_time = std::chrono::high_resolution_clock::now();
        
        PolicyResult result;
        result.allowed = true;
        result.severity = ViolationSeverity::INFO;
        result.violated_policy_count = 0;
        result.primary_violation_id = 0;
        result.evaluated_policy_count = 0;
        strcpy(result.violation_reason, "");
        
        if (emergency_stopped_.load()) {
            result.set_violation(0, ViolationSeverity::CRITICAL, "Emergency stop active");
            auto end_time = std::chrono::high_resolution_clock::now();
            result.evaluation_time_ns = std::chrono::duration_cast<std::chrono::nanoseconds>
                (end_time - start_time).count();
            return result;
        }
        
        std::lock_guard<std::mutex> lock(policies_mutex_);
        
        for (const auto& [policy_id, policy] : policies_) {
            if (!policy_enabled_[policy_id]) {
                continue;
            }
            
            result.evaluated_policy_count++;
            
            if (!policy->evaluate(order, market, portfolio, result)) {
                if (config_.enable_early_termination && 
                    result.severity >= ViolationSeverity::CRITICAL) {
                    break; // Stop on critical violation
                }
            }
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        result.evaluation_time_ns = std::chrono::duration_cast<std::chrono::nanoseconds>
            (end_time - start_time).count();
        
        // Update metrics
        metrics_.evaluations_total.fetch_add(1);
        if (result.allowed) {
            metrics_.evaluations_passed.fetch_add(1);
        } else {
            metrics_.evaluations_failed.fetch_add(1);
        }
        
        update_avg_evaluation_time(result.evaluation_time_ns);
        
        // Add to audit trail if enabled
        if (audit_enabled_.load()) {
            add_audit_entry(order, result);
        }
        
        return result;
    }
    
    void update_avg_evaluation_time(uint64_t time_ns) {
        uint64_t current_avg = metrics_.avg_evaluation_time_ns.load();
        uint64_t new_avg = (current_avg * 63 + time_ns) / 64; // Moving average
        metrics_.avg_evaluation_time_ns.store(new_avg);
        
        // Update max time
        uint64_t current_max = metrics_.max_evaluation_time_ns.load();
        while (time_ns > current_max) {
            if (metrics_.max_evaluation_time_ns.compare_exchange_weak(current_max, time_ns)) {
                break;
            }
        }
    }
    
    void add_audit_entry(const OrderDetails& order, const PolicyResult& result) {
        std::lock_guard<std::mutex> lock(audit_mutex_);
        
        AuditEntry entry;
        entry.timestamp_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch()).count();
        entry.order_id = order.client_order_id;
        entry.symbol = order.symbol;
        entry.result = result;
        
        audit_trail_.push_back(entry);
        
        // Keep audit trail size manageable
        if (audit_trail_.size() > 10000) {
            audit_trail_.erase(audit_trail_.begin(), audit_trail_.begin() + 1000);
        }
    }
};

PolicyEngine::PolicyEngine(const EngineConfig& config)
    : pimpl_(std::make_unique<PolicyEngineImpl>(config)) {}

PolicyEngine::~PolicyEngine() = default;

void PolicyEngine::add_policy(std::unique_ptr<IPolicy> policy) {
    pimpl_->add_policy(std::move(policy));
}

void PolicyEngine::remove_policy(uint32_t policy_id) {
    pimpl_->remove_policy(policy_id);
}

void PolicyEngine::enable_policy(uint32_t policy_id, bool enabled) {
    pimpl_->enable_policy(policy_id, enabled);
}

PolicyResult PolicyEngine::evaluate_order(const OrderDetails& order,
                                         const MarketContext& market,
                                         const PortfolioState& portfolio) {
    return pimpl_->evaluate_order(order, market, portfolio);
}

size_t PolicyEngine::evaluate_orders(const OrderDetails* orders, size_t count,
                                    const MarketContext& market,
                                    const PortfolioState& portfolio,
                                    PolicyResult* results) {
    for (size_t i = 0; i < count; ++i) {
        results[i] = evaluate_order(orders[i], market, portfolio);
    }
    return count;
}

void PolicyEngine::update_policy_parameters(uint32_t policy_id, 
                                           const std::unordered_map<std::string, double>& params) {
    std::lock_guard<std::mutex> lock(pimpl_->policies_mutex_);
    auto it = pimpl_->policies_.find(policy_id);
    if (it != pimpl_->policies_.end()) {
        it->second->update_parameters(params);
    }
}

void PolicyEngine::emergency_stop_all() {
    pimpl_->emergency_stopped_.store(true);
    pimpl_->metrics_.emergency_stops.fetch_add(1);
    HFX_LOG_INFO("[PolicyEngine] 🚨 EMERGENCY STOP ACTIVATED 🚨");
}

void PolicyEngine::reset_emergency_stop() {
    pimpl_->emergency_stopped_.store(false);
    HFX_LOG_INFO("[PolicyEngine] Emergency stop reset");
}

bool PolicyEngine::is_emergency_stopped() const {
    return pimpl_->emergency_stopped_.load();
}

void PolicyEngine::enable_audit_logging(bool enabled) {
    pimpl_->audit_enabled_.store(enabled);
}

std::vector<PolicyEngine::AuditEntry> PolicyEngine::get_audit_trail(uint64_t since_timestamp_ns) const {
    std::lock_guard<std::mutex> lock(pimpl_->audit_mutex_);
    
    std::vector<AuditEntry> filtered;
    for (const auto& entry : pimpl_->audit_trail_) {
        if (entry.timestamp_ns >= since_timestamp_ns) {
            filtered.push_back(entry);
        }
    }
    
    return filtered;
}

void PolicyEngine::clear_audit_trail() {
    std::lock_guard<std::mutex> lock(pimpl_->audit_mutex_);
    pimpl_->audit_trail_.clear();
}

const PolicyEngine::PolicyMetrics& PolicyEngine::get_metrics() const {
    return pimpl_->metrics_;
}

void PolicyEngine::reset_metrics() {
    pimpl_->metrics_.evaluations_total.store(0);
    pimpl_->metrics_.evaluations_passed.store(0);
    pimpl_->metrics_.evaluations_failed.store(0);
    pimpl_->metrics_.avg_evaluation_time_ns.store(0);
    pimpl_->metrics_.max_evaluation_time_ns.store(0);
    pimpl_->metrics_.timeout_count.store(0);
    pimpl_->metrics_.emergency_stops.store(0);
}

std::unordered_map<uint32_t, PolicyEngine::PolicyStats> PolicyEngine::get_policy_statistics() const {
    std::unordered_map<uint32_t, PolicyStats> stats;
    
    // Simplified stats calculation
    std::lock_guard<std::mutex> lock(pimpl_->policies_mutex_);
    for (const auto& [policy_id, policy] : pimpl_->policies_) {
        PolicyStats stat;
        stat.evaluations = 100; // Placeholder
        stat.violations = 5;    // Placeholder
        stat.avg_time_ns = 1000; // Placeholder
        stat.max_severity = ViolationSeverity::WARNING;
        
        stats[policy_id] = stat;
    }
    
    return stats;
}

} // namespace hfx::hft
