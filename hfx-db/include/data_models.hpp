/**
 * @file data_models.hpp
 * @brief Database data models for HydraFlow-X
 */

#pragma once

#include <string>
#include <vector>
#include <chrono>
#include <optional>
#include <unordered_map>

namespace hfx::db {

/**
 * @brief Trading platform enumeration
 */
enum class TradingPlatform {
    UNISWAP_V3,
    RAYDIUM_AMM,
    ORCA_WHIRLPOOL,
    METEORA_DLMM,
    PUMP_FUN,
    MOONSHOT,
    JUPITER,
    SERUM
};

/**
 * @brief Order side enumeration
 */
enum class OrderSide {
    BUY,
    SELL
};

/**
 * @brief Order type enumeration
 */
enum class OrderType {
    MARKET,
    LIMIT,
    STOP_LOSS,
    TAKE_PROFIT,
    TRAILING_STOP
};

/**
 * @brief Order status enumeration
 */
enum class OrderStatus {
    PENDING,
    OPEN,
    PARTIALLY_FILLED,
    FILLED,
    CANCELLED,
    EXPIRED,
    REJECTED
};

/**
 * @brief Trade execution details
 */
struct Trade {
    std::string id;
    std::string order_id;
    TradingPlatform platform;
    std::string token_in;
    std::string token_out;
    OrderSide side;
    uint64_t amount_in;        // Raw amount (wei/lamports)
    uint64_t amount_out;       // Raw amount (wei/lamports)
    std::optional<uint64_t> amount_in_min;    // Minimum amount in for slippage protection
    std::optional<uint64_t> amount_out_min;   // Minimum amount out for slippage protection
    double price;              // Execution price
    std::optional<double> slippage_percent;   // Actual slippage percentage
    std::optional<uint64_t> gas_used;         // Gas used for transaction
    std::optional<uint64_t> gas_price;        // Gas price paid (wei/gwei)
    std::optional<std::string> transaction_hash;
    std::optional<std::string> block_number;
    OrderStatus status;
    std::optional<std::string> error_message;
    std::chrono::system_clock::time_point created_at;
    std::chrono::system_clock::time_point updated_at;
    std::optional<std::chrono::system_clock::time_point> executed_at;

    // Additional metadata
    std::string wallet_address;
    std::optional<std::string> dex_address;
    std::optional<std::string> pool_address;
    double fee_percent;
    uint64_t fee_amount;
    std::string chain_id;
};

/**
 * @brief Position information
 */
struct Position {
    std::string id;
    std::string wallet_address;
    std::string token_address;
    std::string token_symbol;
    uint64_t balance;          // Raw balance (wei/lamports)
    double usd_value;          // USD value of position
    double avg_entry_price;    // Average entry price in USD
    double current_price;      // Current market price in USD
    double pnl_percent;        // P&L percentage
    double pnl_usd;            // P&L in USD
    std::chrono::system_clock::time_point last_updated;
    std::chrono::system_clock::time_point created_at;

    // Risk metrics
    double volatility_24h;
    double liquidity_score;
    bool is_whitelisted;
    std::vector<std::string> tags;
};

/**
 * @brief Market data snapshot
 */
struct MarketData {
    std::string id;
    TradingPlatform platform;
    std::string token_address;
    std::string token_symbol;
    std::string pair_address;
    std::string base_token;
    std::string quote_token;
    double price_usd;
    double price_native;       // Price in native token (ETH/SOL)
    double volume_24h;
    double market_cap;
    double liquidity_usd;
    uint64_t total_supply;
    double price_change_24h;
    double price_change_7d;
    double price_change_30d;
    std::chrono::system_clock::time_point timestamp;

    // DEX-specific data
    uint64_t pool_liquidity;
    double fee_tier;           // Fee tier percentage
    uint64_t tvl;             // Total value locked
    double apy;               // Annual percentage yield
};

/**
 * @brief Liquidity pool information
 */
struct LiquidityPool {
    std::string id;
    TradingPlatform platform;
    std::string pool_address;
    std::string token0_address;
    std::string token1_address;
    std::string token0_symbol;
    std::string token1_symbol;
    uint64_t token0_reserve;
    uint64_t token1_reserve;
    uint64_t total_liquidity;
    double fee_tier;
    double apy;
    double impermanent_loss_24h;
    std::chrono::system_clock::time_point last_updated;
    std::chrono::system_clock::time_point created_at;
};

/**
 * @brief Analytics data for time-series analysis
 */
struct AnalyticsData {
    std::string id;
    std::string metric_name;
    std::string metric_type;   // "trade_volume", "pnl", "gas_usage", etc.
    double value;
    std::chrono::system_clock::time_point timestamp;
    std::string time_bucket;   // "1m", "5m", "1h", "1d", etc.

    // Dimensions for filtering
    std::optional<std::string> platform;
    std::optional<std::string> token_symbol;
    std::optional<std::string> wallet_address;
    std::optional<std::string> strategy_name;

    // Additional metadata
    std::unordered_map<std::string, std::string> tags;
};

/**
 * @brief Risk metrics for monitoring
 */
struct RiskMetrics {
    std::string id;
    std::string wallet_address;
    double total_portfolio_value;
    double total_pnl_usd;
    double total_pnl_percent;
    double max_drawdown_percent;
    double sharpe_ratio;
    double volatility_annualized;
    uint64_t total_trades;
    uint64_t winning_trades;
    uint64_t losing_trades;
    double win_rate_percent;
    double avg_trade_size_usd;
    double largest_win_usd;
    double largest_loss_usd;
    std::chrono::system_clock::time_point calculated_at;

    // Risk limits
    double max_position_size_percent;
    double max_daily_loss_percent;
    double max_drawdown_limit_percent;
    bool risk_limits_breached;
};

/**
 * @brief System performance metrics
 */
struct PerformanceMetrics {
    std::string id;
    std::chrono::system_clock::time_point timestamp;
    double cpu_usage_percent;
    uint64_t memory_usage_mb;
    double network_latency_ms;
    uint64_t active_connections;
    uint64_t total_requests;
    uint64_t error_count;
    double avg_response_time_ms;
    uint64_t trades_per_second;
    uint64_t orders_per_second;

    // Database performance
    double db_query_time_ms;
    uint64_t db_connections_active;
    uint64_t db_connections_idle;

    // Trading performance
    double avg_slippage_percent;
    double success_rate_percent;
    uint64_t mev_attacks_detected;
    uint64_t mev_attacks_prevented;
};

} // namespace hfx::db
