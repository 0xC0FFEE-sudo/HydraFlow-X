-- HydraFlow-X ClickHouse Analytics Schema
-- ClickHouse initialization script for high-performance analytics

-- Create database
CREATE DATABASE IF NOT EXISTS hydraflow_analytics;

-- Use the database
USE hydraflow_analytics;

-- Create enum types (ClickHouse supports enums)
CREATE TYPE trading_platform AS Enum8(
    'UNISWAP_V3' = 1,
    'RAYDIUM_AMM' = 2,
    'ORCA_WHIRLPOOL' = 3,
    'METEORA_DLMM' = 4,
    'PUMP_FUN' = 5,
    'MOONSHOT' = 6,
    'JUPITER' = 7,
    'SERUM' = 8
);

-- Create analytics_events table (main time-series table)
CREATE TABLE IF NOT EXISTS analytics_events (
    timestamp DateTime64(3, 'UTC'),
    event_type String,
    metric_name String,
    metric_value Decimal64(18),
    time_bucket String, -- '1m', '5m', '1h', '1d'

    -- Dimensions
    platform trading_platform,
    token_symbol String,
    token_address String,
    wallet_address String,
    strategy_name String,
    chain_id String,
    dex_address String,
    pool_address String,

    -- Additional context
    tags Map(String, String),
    metadata String, -- JSON string for additional data

    -- Technical fields
    event_id UUID,
    ingestion_timestamp DateTime64(3, 'UTC') DEFAULT now64(3)
)
ENGINE = MergeTree()
PARTITION BY toYYYYMMDD(timestamp)
ORDER BY (timestamp, event_type, metric_name, platform, token_symbol)
TTL toDateTime(timestamp) + INTERVAL 1 YEAR
SETTINGS index_granularity = 8192;

-- Create trades_analytics table (specialized for trade data)
CREATE TABLE IF NOT EXISTS trades_analytics (
    timestamp DateTime64(3, 'UTC'),
    trade_id String,
    order_id String,
    platform trading_platform,
    side Enum8('BUY' = 1, 'SELL' = 2),
    status Enum8('PENDING' = 1, 'OPEN' = 2, 'PARTIALLY_FILLED' = 3, 'FILLED' = 4, 'CANCELLED' = 5, 'EXPIRED' = 6, 'REJECTED' = 7),

    -- Token information
    token_in String,
    token_out String,
    token_in_symbol String,
    token_out_symbol String,

    -- Amounts and prices
    amount_in UInt256,
    amount_out UInt256,
    amount_in_min UInt256,
    amount_out_min UInt256,
    price Decimal64(18),
    price_usd Decimal64(8),

    -- Performance metrics
    slippage_percent Decimal32(4),
    gas_used UInt64,
    gas_price UInt64,
    gas_cost_usd Decimal64(8),
    fee_amount UInt256,
    fee_percent Decimal32(4),

    -- Timing
    created_at DateTime64(3, 'UTC'),
    executed_at DateTime64(3, 'UTC'),
    execution_time_ms UInt32,

    -- Addresses
    wallet_address String,
    dex_address String,
    pool_address String,
    transaction_hash String,
    block_number String,

    -- Additional context
    chain_id String,
    strategy_name String,
    error_message String,

    -- Technical fields
    ingestion_timestamp DateTime64(3, 'UTC') DEFAULT now64(3)
)
ENGINE = MergeTree()
PARTITION BY toYYYYMMDD(timestamp)
ORDER BY (timestamp, platform, wallet_address, token_in, token_out)
TTL toDateTime(timestamp) + INTERVAL 2 YEARS
SETTINGS index_granularity = 8192;

-- Create market_data_analytics table
CREATE TABLE IF NOT EXISTS market_data_analytics (
    timestamp DateTime64(3, 'UTC'),
    platform trading_platform,
    token_address String,
    token_symbol String,
    pair_address String,
    base_token String,
    quote_token String,

    -- Price data
    price_usd Decimal64(8),
    price_native Decimal64(18),
    price_change_1m Decimal32(4),
    price_change_5m Decimal32(4),
    price_change_1h Decimal32(4),
    price_change_24h Decimal32(4),

    -- Volume and liquidity
    volume_usd_1m Decimal64(8),
    volume_usd_5m Decimal64(8),
    volume_usd_1h Decimal64(8),
    volume_usd_24h Decimal64(8),
    liquidity_usd Decimal64(8),
    market_cap_usd Decimal64(8),

    -- Technical indicators
    volatility_1h Decimal32(4),
    volatility_24h Decimal32(4),
    rsi_14 Decimal32(2),
    macd_value Decimal64(8),
    macd_signal Decimal64(8),
    macd_histogram Decimal64(8),

    -- Pool data
    pool_liquidity UInt256,
    fee_tier Decimal32(4),
    tvl UInt256,
    apy Decimal32(4),

    -- Social and on-chain metrics
    holder_count UInt32,
    transaction_count_24h UInt32,
    unique_wallets_24h UInt32,

    -- Technical fields
    ingestion_timestamp DateTime64(3, 'UTC') DEFAULT now64(3)
)
ENGINE = MergeTree()
PARTITION BY toYYYYMMDD(timestamp)
ORDER BY (timestamp, platform, token_symbol, token_address)
TTL toDateTime(timestamp) + INTERVAL 1 YEAR
SETTINGS index_granularity = 8192;

-- Create risk_analytics table
CREATE TABLE IF NOT EXISTS risk_analytics (
    timestamp DateTime64(3, 'UTC'),
    wallet_address String,
    strategy_name String,

    -- Portfolio metrics
    total_portfolio_value_usd Decimal64(8),
    total_positions UInt32,
    total_pnl_usd Decimal64(8),
    total_pnl_percent Decimal32(4),

    -- Risk metrics
    volatility_24h Decimal32(4),
    sharpe_ratio Decimal32(4),
    max_drawdown_percent Decimal32(4),
    value_at_risk_95 Decimal32(4), -- 95% VaR
    expected_shortfall_95 Decimal32(4), -- 95% ES

    -- Trading metrics
    total_trades UInt32,
    winning_trades UInt32,
    losing_trades UInt32,
    win_rate_percent Decimal32(4),
    avg_trade_size_usd Decimal64(8),
    avg_holding_time_minutes UInt32,

    -- Risk limits and breaches
    max_position_size_percent Decimal32(4),
    max_daily_loss_percent Decimal32(4),
    max_drawdown_limit_percent Decimal32(4),
    risk_limits_breached UInt8,

    -- Liquidity and concentration
    largest_position_percent Decimal32(4),
    top_10_positions_percent Decimal32(4),
    illiquid_positions_percent Decimal32(4),

    -- Technical fields
    ingestion_timestamp DateTime64(3, 'UTC') DEFAULT now64(3)
)
ENGINE = MergeTree()
PARTITION BY toYYYYMMDD(timestamp)
ORDER BY (timestamp, wallet_address, strategy_name)
TTL toDateTime(timestamp) + INTERVAL 2 YEARS
SETTINGS index_granularity = 8192;

-- Create performance_metrics table
CREATE TABLE IF NOT EXISTS performance_metrics (
    timestamp DateTime64(3, 'UTC'),
    component String, -- 'trading_engine', 'websocket_server', 'database', etc.

    -- System metrics
    cpu_usage_percent Decimal32(4),
    memory_usage_mb UInt32,
    disk_usage_percent Decimal32(4),
    network_rx_mb UInt32,
    network_tx_mb UInt32,

    -- Application metrics
    active_connections UInt32,
    total_requests UInt64,
    error_count UInt32,
    avg_response_time_ms Decimal32(4),
    p95_response_time_ms Decimal32(4),
    p99_response_time_ms Decimal32(4),

    -- Trading metrics
    trades_per_second Decimal32(4),
    orders_per_second Decimal32(4),
    avg_slippage_percent Decimal32(4),
    success_rate_percent Decimal32(4),

    -- MEV metrics
    mev_attacks_detected UInt32,
    mev_attacks_prevented UInt32,
    mev_profit_usd Decimal64(8),
    mev_loss_usd Decimal64(8),

    -- Database metrics
    db_query_time_ms Decimal32(4),
    db_connections_active UInt32,
    db_connections_idle UInt32,
    db_cache_hit_rate Decimal32(4),

    -- Technical fields
    ingestion_timestamp DateTime64(3, 'UTC') DEFAULT now64(3)
)
ENGINE = MergeTree()
PARTITION BY toYYYYMMDD(timestamp)
ORDER BY (timestamp, component)
TTL toDateTime(timestamp) + INTERVAL 90 DAY
SETTINGS index_granularity = 8192;

-- Create materialized views for common aggregations
CREATE MATERIALIZED VIEW IF NOT EXISTS trades_summary_mv
ENGINE = SummingMergeTree()
PARTITION BY toYYYYMMDD(timestamp)
ORDER BY (timestamp, platform, wallet_address, time_bucket)
AS SELECT
    timestamp,
    platform,
    wallet_address,
    time_bucket,
    count() as trade_count,
    sum(amount_in) as total_volume_in,
    sum(amount_out) as total_volume_out,
    avg(price) as avg_price,
    avg(slippage_percent) as avg_slippage,
    sum(gas_cost_usd) as total_gas_cost,
    sum(fee_amount) as total_fees,
    countIf(status = 'FILLED') as successful_trades,
    countIf(side = 'BUY') as buy_trades,
    countIf(side = 'SELL') as sell_trades
FROM trades_analytics
GROUP BY timestamp, platform, wallet_address, time_bucket;

-- Create materialized view for portfolio analytics
CREATE MATERIALIZED VIEW IF NOT EXISTS portfolio_analytics_mv
ENGINE = AggregatingMergeTree()
PARTITION BY toYYYYMMDD(timestamp)
ORDER BY (timestamp, wallet_address, time_bucket)
AS SELECT
    timestamp,
    wallet_address,
    time_bucket,
    count() as position_count,
    sum(total_portfolio_value_usd) as total_value,
    sum(total_pnl_usd) as total_pnl,
    avg(total_pnl_percent) as avg_pnl_percent,
    max(max_drawdown_percent) as max_drawdown,
    avg(volatility_24h) as avg_volatility
FROM risk_analytics
GROUP BY timestamp, wallet_address, time_bucket;

-- Create dictionary for token metadata (if needed)
CREATE DICTIONARY IF NOT EXISTS token_metadata_dict (
    token_address String,
    token_symbol String,
    token_name String,
    decimals UInt8,
    chain_id String,
    coingecko_id String,
    is_verified UInt8
)
PRIMARY KEY token_address
SOURCE(HTTP(URL 'http://localhost:8080/api/v1/token-metadata' FORMAT 'JSONEachRow'))
LIFETIME(MIN 300 MAX 3600)
LAYOUT(FLAT());

-- Create user and permissions
CREATE USER IF NOT EXISTS hydraflow IDENTIFIED BY 'hydraflow_password';
GRANT SELECT, INSERT, ALTER, CREATE, DROP ON hydraflow_analytics.* TO hydraflow;

-- Optimize settings for high-throughput analytics
SET max_threads = 8;
SET max_memory_usage = '32GB';
SET max_bytes_before_external_group_by = '10GB';
SET max_bytes_before_external_sort = '10GB';

-- Create retention policies (optional)
-- ALTER TABLE analytics_events MODIFY TTL toDateTime(timestamp) + INTERVAL 1 YEAR;
-- ALTER TABLE trades_analytics MODIFY TTL toDateTime(timestamp) + INTERVAL 2 YEARS;