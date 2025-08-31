/**
 * @file hedge_engine.hpp
 * @brief Cross-venue hedging and atomic execution engine
 * @version 1.0.0
 */

#pragma once

#include <atomic>
#include <functional>
#include "strategy_engine.hpp"

namespace hfx::hedge {

class HedgeEngine {
public:
    HedgeEngine() = default;
    ~HedgeEngine() = default;

    bool initialize();
    void shutdown();
    bool execute_hedge(const strat::TradingSignal& signal);
    bool is_running() const { return running_.load(); }

private:
    std::atomic<bool> running_{false};
};

} // namespace hfx::hedge
