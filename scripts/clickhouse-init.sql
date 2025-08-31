-- HydraFlow-X ClickHouse Analytics Database Initialization
-- Optimized for high-performance time-series data and analytics

-- Create database
CREATE DATABASE IF NOT EXISTS hydraflow_analytics;
USE hydraflow_analytics;

-- Trading performance analytics table
CREATE TABLE IF NOT EXISTS trading_performance
(
    timestamp DateTime64(3) CODEC(DoubleDelta, ZSTD(1)),
    trade_id String CODEC(ZSTD(1)),
    symbol LowCardinality(String) CODEC(ZSTD(1)),
    side Enum8('buy' = 1, 'sell' = 2) CODEC(ZSTD(1)),
    amount Decimal64(8) CODEC(ZSTD(1)),
    price Decimal64(8) CODEC(ZSTD(1)),
    execution_time_ms UInt32 CODEC(ZSTD(1)),
    slippage_bps Int16 CODEC(ZSTD(1)),
    gas_used UInt64 CODEC(ZSTD(1)),
    gas_price Decimal64(8) CODEC(ZSTD(1)),
    mev_protection_level LowCardinality(String) CODEC(ZSTD(1)),
    wallet_address String CODEC(ZSTD(1)),
    chain LowCardinality(String) CODEC(ZSTD(1)),
    transaction_hash String CODEC(ZSTD(1)),
    block_number UInt64 CODEC(ZSTD(1)),
    status Enum8('completed' = 1, 'failed' = 2, 'cancelled' = 3) CODEC(ZSTD(1))
)
ENGINE = MergeTree()
PARTITION BY toYYYYMM(timestamp)
ORDER BY (timestamp, symbol, chain)
TTL timestamp + INTERVAL 1 YEAR
SETTINGS index_granularity = 8192;

-- Latency measurements table
CREATE TABLE IF NOT EXISTS latency_measurements
(
    timestamp DateTime64(3) CODEC(DoubleDelta, ZSTD(1)),
    component LowCardinality(String) CODEC(ZSTD(1)),
    operation LowCardinality(String) CODEC(ZSTD(1)),
    latency_ms Decimal64(3) CODEC(ZSTD(1)),
    p50_ms Decimal64(3) CODEC(ZSTD(1)),
    p95_ms Decimal64(3) CODEC(ZSTD(1)),
    p99_ms Decimal64(3) CODEC(ZSTD(1)),
    request_count UInt32 CODEC(ZSTD(1)),
    error_count UInt32 CODEC(ZSTD(1)),
    metadata String CODEC(ZSTD(1))
)
ENGINE = MergeTree()
PARTITION BY toYYYYMM(timestamp)
ORDER BY (timestamp, component, operation)
TTL timestamp + INTERVAL 6 MONTH
SETTINGS index_granularity = 8192;

-- Market data table
CREATE TABLE IF NOT EXISTS market_data
(
    timestamp DateTime64(3) CODEC(DoubleDelta, ZSTD(1)),
    symbol LowCardinality(String) CODEC(ZSTD(1)),
    price Decimal64(8) CODEC(ZSTD(1)),
    volume_1m Decimal64(8) CODEC(ZSTD(1)),
    volume_5m Decimal64(8) CODEC(ZSTD(1)),
    volume_1h Decimal64(8) CODEC(ZSTD(1)),
    volume_24h Decimal64(8) CODEC(ZSTD(1)),
    market_cap Decimal64(8) CODEC(ZSTD(1)),
    liquidity Decimal64(8) CODEC(ZSTD(1)),
    change_1m Decimal64(4) CODEC(ZSTD(1)),
    change_5m Decimal64(4) CODEC(ZSTD(1)),
    change_1h Decimal64(4) CODEC(ZSTD(1)),
    change_24h Decimal64(4) CODEC(ZSTD(1)),
    volatility Decimal64(4) CODEC(ZSTD(1)),
    source LowCardinality(String) CODEC(ZSTD(1)),
    chain LowCardinality(String) CODEC(ZSTD(1)),
    dex LowCardinality(String) CODEC(ZSTD(1))
)
ENGINE = ReplacingMergeTree()
PARTITION BY toYYYYMM(timestamp)
ORDER BY (timestamp, symbol, source, chain)
TTL timestamp + INTERVAL 3 MONTH
SETTINGS index_granularity = 8192;

-- Sentiment data table
CREATE TABLE IF NOT EXISTS sentiment_data
(
    timestamp DateTime64(3) CODEC(DoubleDelta, ZSTD(1)),
    symbol LowCardinality(String) CODEC(ZSTD(1)),
    sentiment_score Decimal64(4) CODEC(ZSTD(1)),
    confidence Decimal64(4) CODEC(ZSTD(1)),
    mention_count UInt32 CODEC(ZSTD(1)),
    positive_mentions UInt32 CODEC(ZSTD(1)),
    negative_mentions UInt32 CODEC(ZSTD(1)),
    neutral_mentions UInt32 CODEC(ZSTD(1)),
    source LowCardinality(String) CODEC(ZSTD(1)),
    source_url String CODEC(ZSTD(1)),
    keywords Array(String) CODEC(ZSTD(1)),
    influencer_score Decimal64(4) CODEC(ZSTD(1)),
    virality_score Decimal64(4) CODEC(ZSTD(1))
)
ENGINE = MergeTree()
PARTITION BY toYYYYMM(timestamp)
ORDER BY (timestamp, symbol, source)
TTL timestamp + INTERVAL 1 MONTH
SETTINGS index_granularity = 8192;

-- MEV protection events table
CREATE TABLE IF NOT EXISTS mev_protection_events
(
    timestamp DateTime64(3) CODEC(DoubleDelta, ZSTD(1)),
    trade_id String CODEC(ZSTD(1)),
    protection_type LowCardinality(String) CODEC(ZSTD(1)),
    threat_type LowCardinality(String) CODEC(ZSTD(1)),
    action_taken LowCardinality(String) CODEC(ZSTD(1)),
    effectiveness_score Decimal64(2) CODEC(ZSTD(1)),
    potential_loss_prevented Decimal64(8) CODEC(ZSTD(1)),
    gas_cost_increase Decimal64(8) CODEC(ZSTD(1)),
    relay_used LowCardinality(String) CODEC(ZSTD(1)),
    bundle_included Bool CODEC(ZSTD(1)),
    chain LowCardinality(String) CODEC(ZSTD(1)),
    details String CODEC(ZSTD(1))
)
ENGINE = MergeTree()
PARTITION BY toYYYYMM(timestamp)
ORDER BY (timestamp, protection_type, chain)
TTL timestamp + INTERVAL 6 MONTH
SETTINGS index_granularity = 8192;

-- System metrics table
CREATE TABLE IF NOT EXISTS system_metrics
(
    timestamp DateTime64(3) CODEC(DoubleDelta, ZSTD(1)),
    metric_name LowCardinality(String) CODEC(ZSTD(1)),
    metric_value Decimal64(8) CODEC(ZSTD(1)),
    metric_unit LowCardinality(String) CODEC(ZSTD(1)),
    component LowCardinality(String) CODEC(ZSTD(1)),
    instance_id String CODEC(ZSTD(1)),
    hostname LowCardinality(String) CODEC(ZSTD(1)),
    tags Map(String, String) CODEC(ZSTD(1))
)
ENGINE = MergeTree()
PARTITION BY toYYYYMM(timestamp)
ORDER BY (timestamp, metric_name, component)
TTL timestamp + INTERVAL 3 MONTH
SETTINGS index_granularity = 8192;

-- Error logs table
CREATE TABLE IF NOT EXISTS error_logs
(
    timestamp DateTime64(3) CODEC(DoubleDelta, ZSTD(1)),
    level Enum8('DEBUG' = 1, 'INFO' = 2, 'WARN' = 3, 'ERROR' = 4, 'FATAL' = 5) CODEC(ZSTD(1)),
    component LowCardinality(String) CODEC(ZSTD(1)),
    message String CODEC(ZSTD(1)),
    error_code String CODEC(ZSTD(1)),
    stack_trace String CODEC(ZSTD(1)),
    user_id String CODEC(ZSTD(1)),
    session_id String CODEC(ZSTD(1)),
    request_id String CODEC(ZSTD(1)),
    hostname LowCardinality(String) CODEC(ZSTD(1)),
    metadata String CODEC(ZSTD(1))
)
ENGINE = MergeTree()
PARTITION BY toYYYYMM(timestamp)
ORDER BY (timestamp, level, component)
TTL timestamp + INTERVAL 1 MONTH
SETTINGS index_granularity = 8192;

-- Create materialized views for real-time analytics

-- Trading performance summary (1-minute intervals)
CREATE MATERIALIZED VIEW IF NOT EXISTS trading_performance_1m
ENGINE = SummingMergeTree()
PARTITION BY toYYYYMM(minute)
ORDER BY (minute, symbol, chain)
AS SELECT
    toStartOfMinute(timestamp) AS minute,
    symbol,
    chain,
    count() AS trade_count,
    sum(amount) AS total_volume,
    avg(execution_time_ms) AS avg_execution_time,
    quantile(0.5)(execution_time_ms) AS p50_execution_time,
    quantile(0.95)(execution_time_ms) AS p95_execution_time,
    quantile(0.99)(execution_time_ms) AS p99_execution_time,
    avg(slippage_bps) AS avg_slippage,
    sum(gas_used) AS total_gas_used,
    countIf(status = 'completed') AS successful_trades,
    countIf(status = 'failed') AS failed_trades
FROM trading_performance
GROUP BY minute, symbol, chain;

-- Latency monitoring (1-minute intervals)
CREATE MATERIALIZED VIEW IF NOT EXISTS latency_monitoring_1m
ENGINE = SummingMergeTree()
PARTITION BY toYYYYMM(minute)
ORDER BY (minute, component, operation)
AS SELECT
    toStartOfMinute(timestamp) AS minute,
    component,
    operation,
    avg(latency_ms) AS avg_latency,
    quantile(0.5)(latency_ms) AS p50_latency,
    quantile(0.95)(latency_ms) AS p95_latency,
    quantile(0.99)(latency_ms) AS p99_latency,
    max(latency_ms) AS max_latency,
    sum(request_count) AS total_requests,
    sum(error_count) AS total_errors
FROM latency_measurements
GROUP BY minute, component, operation;

-- MEV protection effectiveness (1-hour intervals)
CREATE MATERIALIZED VIEW IF NOT EXISTS mev_protection_1h
ENGINE = SummingMergeTree()
PARTITION BY toYYYYMM(hour)
ORDER BY (hour, protection_type, chain)
AS SELECT
    toStartOfHour(timestamp) AS hour,
    protection_type,
    chain,
    count() AS protection_events,
    avg(effectiveness_score) AS avg_effectiveness,
    sum(potential_loss_prevented) AS total_loss_prevented,
    sum(gas_cost_increase) AS total_gas_increase,
    countIf(bundle_included) AS successful_bundles,
    count() - countIf(bundle_included) AS failed_bundles
FROM mev_protection_events
GROUP BY hour, protection_type, chain;

-- Create functions for analytics queries

-- Function to calculate trading success rate
CREATE FUNCTION IF NOT EXISTS calculateSuccessRate AS (completed, total) -> 
    if(total > 0, completed * 100.0 / total, 0);

-- Function to calculate profit/loss
CREATE FUNCTION IF NOT EXISTS calculatePnL AS (side, amount, entry_price, exit_price) ->
    if(side = 'buy', (exit_price - entry_price) * amount, (entry_price - exit_price) * amount);

-- Function to calculate volatility
CREATE FUNCTION IF NOT EXISTS calculateVolatility AS (prices) ->
    if(length(prices) > 1, sqrt(varPop(prices)), 0);

-- Create dictionaries for fast lookups

-- Symbol metadata dictionary
CREATE DICTIONARY IF NOT EXISTS symbol_metadata
(
    symbol String,
    name String,
    decimals UInt8,
    contract_address String,
    chain String,
    is_stable Bool,
    created_at DateTime
)
PRIMARY KEY symbol
SOURCE(CLICKHOUSE(HOST 'localhost' PORT 9000 USER 'default' TABLE 'symbols' DB 'hydraflow_analytics'))
LIFETIME(MIN 300 MAX 3600)
LAYOUT(HASHED());

-- Exchange metadata dictionary  
CREATE DICTIONARY IF NOT EXISTS exchange_metadata
(
    exchange_id String,
    name String,
    chain String,
    dex_type String,
    fee_bps UInt16,
    min_liquidity Decimal64(8)
)
PRIMARY KEY exchange_id
SOURCE(CLICKHOUSE(HOST 'localhost' PORT 9000 USER 'default' TABLE 'exchanges' DB 'hydraflow_analytics'))
LIFETIME(MIN 600 MAX 3600)
LAYOUT(HASHED());

-- Create indexes for faster queries
ALTER TABLE trading_performance ADD INDEX idx_execution_time execution_time_ms TYPE minmax GRANULARITY 3;
ALTER TABLE trading_performance ADD INDEX idx_slippage slippage_bps TYPE minmax GRANULARITY 3;
ALTER TABLE latency_measurements ADD INDEX idx_latency latency_ms TYPE minmax GRANULARITY 3;
ALTER TABLE market_data ADD INDEX idx_price price TYPE minmax GRANULARITY 3;
ALTER TABLE sentiment_data ADD INDEX idx_sentiment sentiment_score TYPE minmax GRANULARITY 3;

-- Create users and permissions
CREATE USER IF NOT EXISTS hydraflow_readonly IDENTIFIED BY 'readonly_password';
CREATE USER IF NOT EXISTS hydraflow_analytics IDENTIFIED BY 'analytics_password';

-- Grant permissions
GRANT SELECT ON hydraflow_analytics.* TO hydraflow_readonly;
GRANT SELECT, INSERT ON hydraflow_analytics.* TO hydraflow_analytics;

-- Create sample data insertion queries (for testing)
/*
-- Insert sample trading performance data
INSERT INTO trading_performance VALUES
    (now(), 'trade_001', 'PEPE/USDC', 'buy', 1000000, 0.000012, 15, 50, 21000, 20000000000, 'HIGH', '0x123...', 'ethereum', '0xabc...', 18500000, 'completed'),
    (now() - 60, 'trade_002', 'BONK/SOL', 'sell', 500000, 0.000023, 12, 30, 5000, 1000000, 'MEDIUM', '0x456...', 'solana', '0xdef...', 245000000, 'completed');

-- Insert sample latency measurements
INSERT INTO latency_measurements VALUES
    (now(), 'trading_engine', 'place_order', 15.5, 12.0, 18.0, 25.0, 100, 0, '{"chain": "ethereum"}'),
    (now(), 'mev_shield', 'bundle_creation', 8.2, 7.0, 12.0, 15.0, 50, 1, '{"protection_level": "high"}');

-- Insert sample market data
INSERT INTO market_data VALUES
    (now(), 'PEPE/USDC', 0.000012, 50000, 250000, 1000000, 50000000, 5000000000, 2000000, 2.5, 5.1, 8.7, 15.3, 45.2, 'dexscreener', 'ethereum', 'uniswap_v3');
*/

-- Optimize tables
OPTIMIZE TABLE trading_performance FINAL;
OPTIMIZE TABLE latency_measurements FINAL;
OPTIMIZE TABLE market_data FINAL;
OPTIMIZE TABLE sentiment_data FINAL;
OPTIMIZE TABLE mev_protection_events FINAL;
OPTIMIZE TABLE system_metrics FINAL;
OPTIMIZE TABLE error_logs FINAL;

-- Show table information
SELECT 
    database,
    table,
    formatReadableSize(total_bytes) AS size,
    total_rows,
    engine
FROM system.tables 
WHERE database = 'hydraflow_analytics' 
ORDER BY total_bytes DESC;
