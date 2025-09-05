/**
 * @file order_routing_engine.cpp
 * @brief Implementation of ultra-fast order routing engine for memecoin trading
 */

#include "memecoin_integrations.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <chrono>
#include <random>
#include <unordered_map>
#include <vector>
#include <queue>
#include <thread>
#include <atomic>

namespace hfx::hft {

// Helper functions for order routing
static std::string generate_order_id() {
    static std::atomic<uint64_t> counter{0};
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    return "order_" + std::to_string(timestamp) + "_" + std::to_string(++counter);
}

static double calculate_price_impact(uint64_t amount_in, uint64_t pool_reserve_in, uint64_t pool_reserve_out) {
    if (pool_reserve_in == 0 || pool_reserve_out == 0) return 100.0;

    // Simplified price impact calculation using constant product formula
    double k = static_cast<double>(pool_reserve_in) * static_cast<double>(pool_reserve_out);
    double new_reserve_in = static_cast<double>(pool_reserve_in) + static_cast<double>(amount_in);
    double new_reserve_out = k / new_reserve_in;

    double expected_out = static_cast<double>(pool_reserve_out) - new_reserve_out;
    double price_impact = ((static_cast<double>(amount_in) / expected_out) - 1.0) * 100.0;

    return std::max(0.0, price_impact);
}

// OrderRoutingEngine Implementation
class OrderRoutingEngine::RoutingEngineImpl {
public:
    RoutingEngineImpl(DEXManager* dex_manager, WalletManager* wallet_manager)
        : dex_manager_(dex_manager), wallet_manager_(wallet_manager) {

        // Initialize routing metrics
        metrics_.total_orders_processed = 0;
        metrics_.successful_executions = 0;
        metrics_.failed_executions = 0;
        metrics_.avg_execution_time_ms = 0.0;
        metrics_.avg_price_impact_percent = 0.0;

        // Start background processing thread
        processing_thread_ = std::thread([this]() {
            background_processing_loop();
        });
    }

    ~RoutingEngineImpl() {
        running_ = false;
        if (processing_thread_.joinable()) {
            processing_thread_.join();
        }
    }

    std::string submit_order(const OrderRoutingEngine::Order& order) {
        OrderRoutingEngine::Order processed_order = order;
        if (processed_order.order_id.empty()) {
            processed_order.order_id = generate_order_id();
        }
        processed_order.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();

        // Store order
        std::lock_guard<std::mutex> lock(orders_mutex_);
        active_orders_[processed_order.order_id] = processed_order;

        // Add to execution queue
        {
            std::lock_guard<std::mutex> queue_lock(queue_mutex_);
            execution_queue_.push(processed_order.order_id);
        }

        metrics_.total_orders_processed++;

        HFX_LOG_INFO("[OrderRouter] Submitted order: " << processed_order.order_id
                  << " (" << processed_order.amount_in << " " << processed_order.token_in
                  << " -> " << processed_order.token_out << ")" << std::endl;

        return processed_order.order_id;
    }

    OrderRoutingEngine::RoutingDecision analyze_routing_options(const OrderRoutingEngine::Order& order) {
        OrderRoutingEngine::RoutingDecision decision;
        decision.order_id = order.order_id;

        // Get quotes from all available venues
        auto quotes = get_venue_quotes(order.token_in, order.token_out, order.amount_in, order.chain);

        if (quotes.empty()) {
            decision.risk_assessment = "No liquidity available";
            return decision;
        }

        // Analyze routing options based on strategy
        switch (order.strategy) {
            case OrderRoutingEngine::ExecutionStrategy::BEST_PRICE:
                decision = route_best_price(order, quotes);
                break;
            case OrderRoutingEngine::ExecutionStrategy::FASTEST_EXECUTION:
                decision = route_fastest_execution(order, quotes);
                break;
            case OrderRoutingEngine::ExecutionStrategy::MINIMUM_SLIPPAGE:
                decision = route_minimum_slippage(order, quotes);
                break;
            case OrderRoutingEngine::ExecutionStrategy::SPLIT_ORDER:
                decision = split_order_across_venues(order, quotes, 3);
                break;
            case OrderRoutingEngine::ExecutionStrategy::SMART_ROUTING:
                decision = smart_route_order(order, quotes);
                break;
            default:
                decision = route_best_price(order, quotes);
                break;
        }

        // Assess MEV protection needs
        decision.requires_mev_protection = assess_mev_risk(order, decision);

        return decision;
    }

    OrderRoutingEngine::ExecutionResult execute_order(const std::string& order_id) {
        OrderRoutingEngine::ExecutionResult result;
        result.order_id = order_id;

        // Get order details
        OrderRoutingEngine::Order order;
        {
            std::lock_guard<std::mutex> lock(orders_mutex_);
            auto it = active_orders_.find(order_id);
            if (it == active_orders_.end()) {
                result.error_message = "Order not found";
                return result;
            }
            order = it->second;
        }

        auto start_time = std::chrono::high_resolution_clock::now();

        // Analyze routing options
        auto decision = analyze_routing_options(order);

        if (decision.venue_allocations.empty()) {
            result.error_message = "No viable routing options";
            metrics_.failed_executions++;
            return result;
        }

        // Execute order across selected venues
        uint64_t total_filled = 0;
        uint64_t total_gas = 0;
        std::vector<std::string> tx_hashes;

        for (const auto& [venue, amount] : decision.venue_allocations) {
            if (!wallet_manager_) {
                result.error_message = "Wallet manager not available";
                break;
            }

            // Simulate execution
            std::string tx_hash = execute_on_venue(order, venue, amount);
            if (!tx_hash.empty()) {
                tx_hashes.push_back(tx_hash);
                total_filled += amount; // Simplified - in reality this would be actual filled amount
                total_gas += decision.estimated_gas / decision.venue_allocations.size();
            }
        }

        auto end_time = std::chrono::high_resolution_clock::now();
        auto execution_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

        // Update results
        result.success = !tx_hashes.empty();
        result.total_filled = total_filled;
        result.gas_used = total_gas;
        result.transaction_hashes = tx_hashes;
        result.execution_time_ms = execution_time.count();
        result.avg_price_impact = decision.total_price_impact;

        if (result.success) {
            metrics_.successful_executions++;
            metrics_.avg_execution_time_ms = (metrics_.avg_execution_time_ms + result.execution_time_ms) / 2.0;
            metrics_.avg_price_impact_percent = (metrics_.avg_price_impact_percent + result.avg_price_impact) / 2.0;

            // Remove from active orders
            {
                std::lock_guard<std::mutex> lock(orders_mutex_);
                active_orders_.erase(order_id);
            }
        } else {
            metrics_.failed_executions++;
        }

        return result;
    }

    bool cancel_order(const std::string& order_id) {
        std::lock_guard<std::mutex> lock(orders_mutex_);
        auto it = active_orders_.find(order_id);
        if (it != active_orders_.end()) {
            active_orders_.erase(it);
            HFX_LOG_INFO("[OrderRouter] Cancelled order: " << order_id << std::endl;
            return true;
        }
        return false;
    }

    bool modify_order(const std::string& order_id, const OrderRoutingEngine::Order& updated_order) {
        std::lock_guard<std::mutex> lock(orders_mutex_);
        auto it = active_orders_.find(order_id);
        if (it != active_orders_.end()) {
            it->second = updated_order;
            it->second.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
            HFX_LOG_INFO("[OrderRouter] Modified order: " << order_id << std::endl;
            return true;
        }
        return false;
    }

    std::vector<OrderRoutingEngine::Order> get_active_orders() const {
        std::lock_guard<std::mutex> lock(orders_mutex_);
        std::vector<OrderRoutingEngine::Order> orders;
        orders.reserve(active_orders_.size());
        for (const auto& [id, order] : active_orders_) {
            orders.push_back(order);
        }
        return orders;
    }

    OrderRoutingEngine::Order get_order_status(const std::string& order_id) const {
        std::lock_guard<std::mutex> lock(orders_mutex_);
        auto it = active_orders_.find(order_id);
        if (it != active_orders_.end()) {
            return it->second;
        }
        return OrderRoutingEngine::Order{}; // Return empty order
    }

    std::vector<OrderRoutingEngine::VenueQuote> get_venue_quotes(const std::string& token_in, const std::string& token_out, uint64_t amount_in, const std::string& chain) {
        std::vector<OrderRoutingEngine::VenueQuote> quotes;

        if (!dex_manager_) {
            return quotes;
        }

        // Get best prices from DEX manager
        auto prices = dex_manager_->get_best_price(token_in, token_out, amount_in, chain);

        for (const auto& [protocol, price] : prices) {
            OrderRoutingEngine::VenueQuote quote;
            quote.venue_name = protocol_to_venue(protocol);
            quote.dex_protocol = DEXProtocol_to_string(protocol);
            quote.expected_out = static_cast<uint64_t>(amount_in * price);
            quote.price_impact_percent = calculate_price_impact(amount_in, 1000000000000ULL, 1000000000000ULL); // Mock reserves
            quote.gas_estimate = estimate_gas_cost(chain, protocol);
            quote.execution_time_ms = estimate_execution_time(protocol);
            quote.fee_percent = get_fee_percent(protocol);
            quote.is_liquid = true;
            quotes.push_back(quote);
        }

        return quotes;
    }

    std::vector<std::string> find_optimal_route(const std::string& token_in, const std::string& token_out, uint64_t amount_in, const std::string& chain) {
        std::vector<std::string> route;
        route.push_back(token_in);

        if (dex_manager_) {
            auto optimal_route = dex_manager_->find_optimal_route(token_in, token_out, chain);
            if (!optimal_route.empty()) {
                route = optimal_route;
            }
        }

        route.push_back(token_out);
        return route;
    }

    OrderRoutingEngine::RoutingMetrics get_routing_metrics() const {
        return metrics_;
    }

    OrderRoutingEngine::RoutingDecision smart_route_order(const OrderRoutingEngine::Order& order, const std::vector<OrderRoutingEngine::VenueQuote>& quotes) {
        // AI-powered routing decision
        // For now, use a simple scoring system
        OrderRoutingEngine::RoutingDecision decision;
        decision.order_id = order.order_id;

        if (quotes.empty()) {
            return decision;
        }

        // Score each venue based on multiple factors
        struct VenueScore {
            std::string venue;
            double score;
            OrderRoutingEngine::VenueQuote quote;
        };

        std::vector<VenueScore> scores;
        for (const auto& quote : quotes) {
            double score = 0.0;

            // Price impact factor (lower is better)
            score += (100.0 - quote.price_impact_percent) * 0.4;

            // Execution time factor (lower is better)
            score += (1000.0 - quote.execution_time_ms) / 10.0 * 0.3;

            // Gas cost factor (lower is better)
            score += (50000.0 - static_cast<double>(quote.gas_estimate)) / 50000.0 * 100.0 * 0.2;

            // Fee factor (lower is better)
            score += (1.0 - quote.fee_percent) * 20.0 * 0.1;

            scores.push_back({quote.venue_name, score, quote});
        }

        // Sort by score (highest first)
        std::sort(scores.begin(), scores.end(), [](const VenueScore& a, const VenueScore& b) {
            return a.score > b.score;
        });

        // Use top venue
        if (!scores.empty()) {
            const auto& best = scores[0];
            decision.venue_allocations.emplace_back(best.venue, order.amount_in);
            decision.total_expected_out = best.quote.expected_out;
            decision.total_price_impact = best.quote.price_impact_percent;
            decision.estimated_gas = best.quote.gas_estimate;
            decision.estimated_execution_time = best.quote.execution_time_ms;
            decision.execution_plan = {best.venue};
            decision.risk_assessment = "Smart routing: " + best.venue;
        }

        return decision;
    }

    OrderRoutingEngine::RoutingDecision split_order_across_venues(const OrderRoutingEngine::Order& order, const std::vector<OrderRoutingEngine::VenueQuote>& quotes, size_t max_splits) {
        OrderRoutingEngine::RoutingDecision decision;
        decision.order_id = order.order_id;

        if (quotes.size() < 2) {
            return smart_route_order(order, quotes);
        }

        // Sort quotes by price impact (best first)
        auto sorted_quotes = quotes;
        std::sort(sorted_quotes.begin(), sorted_quotes.end(), [](const auto& a, const auto& b) {
            return a.price_impact_percent < b.price_impact_percent;
        });

        // Split order across top venues
        size_t venues_to_use = std::min(max_splits, sorted_quotes.size());
        uint64_t remaining_amount = order.amount_in;

        for (size_t i = 0; i < venues_to_use && remaining_amount > 0; ++i) {
            uint64_t allocation = remaining_amount / (venues_to_use - i);
            const auto& quote = sorted_quotes[i];

            decision.venue_allocations.emplace_back(quote.venue_name, allocation);
            decision.total_expected_out += static_cast<uint64_t>(allocation * (100.0 - quote.price_impact_percent) / 100.0);
            decision.total_price_impact += quote.price_impact_percent / venues_to_use;
            decision.estimated_gas += quote.gas_estimate;
            decision.execution_plan.push_back(quote.venue_name);

            remaining_amount -= allocation;
        }

        decision.estimated_execution_time = 100.0; // Parallel execution
        decision.risk_assessment = "Split order across " + std::to_string(venues_to_use) + " venues";

        return decision;
    }

    OrderRoutingEngine::RoutingDecision route_with_mev_protection(const OrderRoutingEngine::Order& order) {
        // Implement MEV protection routing
        auto quotes = get_venue_quotes(order.token_in, order.token_out, order.amount_in, order.chain);
        auto decision = smart_route_order(order, quotes);

        decision.requires_mev_protection = true;
        decision.risk_assessment += " + MEV protection enabled";

        return decision;
    }

    void update_market_data(const std::string& token_pair, const std::string& chain, double price, double liquidity) {
        std::lock_guard<std::mutex> lock(market_data_mutex_);
        market_data_[chain + "_" + token_pair] = {price, liquidity};
    }

    void update_gas_prices(const std::string& chain, uint64_t fast_gas_price, uint64_t standard_gas_price) {
        std::lock_guard<std::mutex> lock(gas_price_mutex_);
        gas_prices_[chain] = {fast_gas_price, standard_gas_price};
    }

private:
    DEXManager* dex_manager_;
    WalletManager* wallet_manager_;

    mutable std::mutex orders_mutex_;
    std::unordered_map<std::string, OrderRoutingEngine::Order> active_orders_;

    mutable std::mutex queue_mutex_;
    std::queue<std::string> execution_queue_;

    mutable std::mutex market_data_mutex_;
    std::unordered_map<std::string, std::pair<double, double>> market_data_; // token_pair -> (price, liquidity)

    mutable std::mutex gas_price_mutex_;
    std::unordered_map<std::string, std::pair<uint64_t, uint64_t>> gas_prices_; // chain -> (fast, standard)

    RoutingMetrics metrics_;

    std::thread processing_thread_;
    std::atomic<bool> running_{true};

    void background_processing_loop() {
        while (running_) {
            std::string order_id;
            {
                std::lock_guard<std::mutex> lock(queue_mutex_);
                if (!execution_queue_.empty()) {
                    order_id = execution_queue_.front();
                    execution_queue_.pop();
                }
            }

            if (!order_id.empty()) {
                // Process order in background
                execute_order(order_id);
            }

            // Sleep to prevent busy waiting
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    OrderRoutingEngine::RoutingDecision route_best_price(const OrderRoutingEngine::Order& order, const std::vector<OrderRoutingEngine::VenueQuote>& quotes) {
        OrderRoutingEngine::RoutingDecision decision;
        decision.order_id = order.order_id;

        if (quotes.empty()) {
            return decision;
        }

        // Find quote with best expected output
        auto best_quote = std::max_element(quotes.begin(), quotes.end(),
            [](const auto& a, const auto& b) { return a.expected_out < b.expected_out; });

        decision.venue_allocations.emplace_back(best_quote->venue_name, order.amount_in);
        decision.total_expected_out = best_quote->expected_out;
        decision.total_price_impact = best_quote->price_impact_percent;
        decision.estimated_gas = best_quote->gas_estimate;
        decision.estimated_execution_time = best_quote->execution_time_ms;
        decision.execution_plan = {best_quote->venue_name};
        decision.risk_assessment = "Best price routing";

        return decision;
    }

    OrderRoutingEngine::RoutingDecision route_fastest_execution(const OrderRoutingEngine::Order& order, const std::vector<OrderRoutingEngine::VenueQuote>& quotes) {
        OrderRoutingEngine::RoutingDecision decision;
        decision.order_id = order.order_id;

        if (quotes.empty()) {
            return decision;
        }

        // Find quote with fastest execution time
        auto fastest_quote = std::min_element(quotes.begin(), quotes.end(),
            [](const auto& a, const auto& b) { return a.execution_time_ms < b.execution_time_ms; });

        decision.venue_allocations.emplace_back(fastest_quote->venue_name, order.amount_in);
        decision.total_expected_out = fastest_quote->expected_out;
        decision.total_price_impact = fastest_quote->price_impact_percent;
        decision.estimated_gas = fastest_quote->gas_estimate;
        decision.estimated_execution_time = fastest_quote->execution_time_ms;
        decision.execution_plan = {fastest_quote->venue_name};
        decision.risk_assessment = "Fastest execution routing";

        return decision;
    }

    OrderRoutingEngine::RoutingDecision route_minimum_slippage(const OrderRoutingEngine::Order& order, const std::vector<OrderRoutingEngine::VenueQuote>& quotes) {
        OrderRoutingEngine::RoutingDecision decision;
        decision.order_id = order.order_id;

        if (quotes.empty()) {
            return decision;
        }

        // Find quote with minimum price impact
        auto best_quote = std::min_element(quotes.begin(), quotes.end(),
            [](const auto& a, const auto& b) { return a.price_impact_percent < b.price_impact_percent; });

        decision.venue_allocations.emplace_back(best_quote->venue_name, order.amount_in);
        decision.total_expected_out = best_quote->expected_out;
        decision.total_price_impact = best_quote->price_impact_percent;
        decision.estimated_gas = best_quote->gas_estimate;
        decision.estimated_execution_time = best_quote->execution_time_ms;
        decision.execution_plan = {best_quote->venue_name};
        decision.risk_assessment = "Minimum slippage routing";

        return decision;
    }

    std::string execute_on_venue(const OrderRoutingEngine::Order& order, const std::string& venue, uint64_t amount) {
        if (!dex_manager_ || !wallet_manager_) {
            return "";
        }

        // Determine DEX protocol from venue
        DEXProtocol protocol = venue_to_protocol(venue);

        // Execute swap through DEX manager
        std::string tx_hash = dex_manager_->execute_swap(protocol, order.token_in, order.token_out,
                                                        amount, 0.5, order.user_address);

        if (!tx_hash.empty()) {
            HFX_LOG_INFO("[OrderRouter] Executed " << amount << " on " << venue
                      << " (tx: " << tx_hash << ")" << std::endl;
        }

        return tx_hash;
    }

    DEXProtocol venue_to_protocol(const std::string& venue) {
        if (venue == "Uniswap V3") return DEXProtocol::UNISWAP_V3;
        if (venue == "Raydium AMM") return DEXProtocol::RAYDIUM_AMM;
        return DEXProtocol::UNISWAP_V3; // Default
    }

    std::string protocol_to_venue(DEXProtocol protocol) {
        switch (protocol) {
            case DEXProtocol::UNISWAP_V3: return "Uniswap V3";
            case DEXProtocol::RAYDIUM_AMM: return "Raydium AMM";
            case DEXProtocol::ORCA_WHIRLPOOL: return "Orca Whirlpool";
            case DEXProtocol::METEORA_DLMM: return "Meteora DLMM";
            default: return "Unknown";
        }
    }

    std::string DEXProtocol_to_string(DEXProtocol protocol) {
        switch (protocol) {
            case DEXProtocol::UNISWAP_V3: return "Uniswap V3";
            case DEXProtocol::RAYDIUM_AMM: return "Raydium AMM";
            case DEXProtocol::ORCA_WHIRLPOOL: return "Orca Whirlpool";
            case DEXProtocol::METEORA_DLMM: return "Meteora DLMM";
            case DEXProtocol::PUMP_FUN: return "Pump.fun";
            case DEXProtocol::MOONSHOT: return "Moonshot";
            default: return "Unknown";
        }
    }

    uint64_t estimate_gas_cost(const std::string& chain, DEXProtocol protocol) {
        if (chain == "ethereum") {
            switch (protocol) {
                case DEXProtocol::UNISWAP_V3: return 150000; // ~150k gas for Uniswap V3 swap
                default: return 100000;
            }
        } else if (chain == "solana") {
            return 5000; // Much lower gas costs on Solana
        }
        return 100000;
    }

    double estimate_execution_time(DEXProtocol protocol) {
        switch (protocol) {
            case DEXProtocol::UNISWAP_V3: return 15.0; // ~15 seconds on Ethereum
            case DEXProtocol::RAYDIUM_AMM: return 0.5; // ~0.5 seconds on Solana
            default: return 10.0;
        }
    }

    double get_fee_percent(DEXProtocol protocol) {
        switch (protocol) {
            case DEXProtocol::UNISWAP_V3: return 0.3; // 0.3% fee
            case DEXProtocol::RAYDIUM_AMM: return 0.25; // 0.25% fee
            default: return 0.3;
        }
    }

    bool assess_mev_risk(const OrderRoutingEngine::Order& order, const OrderRoutingEngine::RoutingDecision& decision) {
        // Assess if order is at risk of MEV attacks
        if (order.amount_in > 1000000000000000000ULL) { // > 1 ETH equivalent
            return true; // Large orders are MEV targets
        }

        if (decision.total_price_impact > 5.0) { // > 5% price impact
            return true; // High impact orders attract MEV
        }

        return false;
    }
};

OrderRoutingEngine::OrderRoutingEngine(DEXManager* dex_manager, WalletManager* wallet_manager)
    : pimpl_(std::make_unique<RoutingEngineImpl>(dex_manager, wallet_manager)) {}

OrderRoutingEngine::~OrderRoutingEngine() = default;

std::string OrderRoutingEngine::submit_order(const Order& order) {
    return pimpl_->submit_order(order);
}

OrderRoutingEngine::RoutingDecision OrderRoutingEngine::analyze_routing_options(const Order& order) {
    return pimpl_->analyze_routing_options(order);
}

OrderRoutingEngine::ExecutionResult OrderRoutingEngine::execute_order(const std::string& order_id) {
    return pimpl_->execute_order(order_id);
}

bool OrderRoutingEngine::cancel_order(const std::string& order_id) {
    return pimpl_->cancel_order(order_id);
}

bool OrderRoutingEngine::modify_order(const std::string& order_id, const Order& updated_order) {
    return pimpl_->modify_order(order_id, updated_order);
}

std::vector<OrderRoutingEngine::Order> OrderRoutingEngine::get_active_orders() const {
    return pimpl_->get_active_orders();
}

OrderRoutingEngine::Order OrderRoutingEngine::get_order_status(const std::string& order_id) const {
    return pimpl_->get_order_status(order_id);
}

std::vector<OrderRoutingEngine::VenueQuote> OrderRoutingEngine::get_venue_quotes(const std::string& token_in, const std::string& token_out, uint64_t amount_in, const std::string& chain) {
    return pimpl_->get_venue_quotes(token_in, token_out, amount_in, chain);
}

std::vector<std::string> OrderRoutingEngine::find_optimal_route(const std::string& token_in, const std::string& token_out, uint64_t amount_in, const std::string& chain) {
    return pimpl_->find_optimal_route(token_in, token_out, amount_in, chain);
}

OrderRoutingEngine::RoutingMetrics OrderRoutingEngine::get_routing_metrics() const {
    return pimpl_->get_routing_metrics();
}

OrderRoutingEngine::RoutingDecision OrderRoutingEngine::smart_route_order(const Order& order) {
    auto quotes = pimpl_->get_venue_quotes(order.token_in, order.token_out, order.amount_in, order.chain);
    return pimpl_->smart_route_order(order, quotes);
}

OrderRoutingEngine::RoutingDecision OrderRoutingEngine::split_order_across_venues(const Order& order, size_t max_splits) {
    auto quotes = pimpl_->get_venue_quotes(order.token_in, order.token_out, order.amount_in, order.chain);
    return pimpl_->split_order_across_venues(order, quotes, max_splits);
}

OrderRoutingEngine::RoutingDecision OrderRoutingEngine::route_with_mev_protection(const Order& order) {
    return pimpl_->route_with_mev_protection(order);
}

void OrderRoutingEngine::update_market_data(const std::string& token_pair, const std::string& chain, double price, double liquidity) {
    pimpl_->update_market_data(token_pair, chain, price, liquidity);
}

void OrderRoutingEngine::update_gas_prices(const std::string& chain, uint64_t fast_gas_price, uint64_t standard_gas_price) {
    pimpl_->update_gas_prices(chain, fast_gas_price, standard_gas_price);
}

} // namespace hfx::hft
