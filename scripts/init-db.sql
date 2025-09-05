-- HydraFlow-X Database Schema
-- PostgreSQL initialization script

-- Enable necessary extensions
CREATE EXTENSION IF NOT EXISTS "uuid-ossp";
CREATE EXTENSION IF NOT EXISTS "pg_trgm";
CREATE EXTENSION IF NOT EXISTS "btree_gin";

-- Create database (if not exists)
-- Note: This should be run by a superuser or database owner

-- Create enum types
CREATE TYPE trading_platform AS ENUM (
    'UNISWAP_V3',
    'RAYDIUM_AMM',
    'ORCA_WHIRLPOOL',
    'METEORA_DLMM',
    'PUMP_FUN',
    'MOONSHOT',
    'JUPITER',
    'SERUM'
);

CREATE TYPE order_side AS ENUM ('BUY', 'SELL');
CREATE TYPE order_type AS ENUM ('MARKET', 'LIMIT', 'STOP_LOSS', 'TAKE_PROFIT', 'TRAILING_STOP');
CREATE TYPE order_status AS ENUM ('PENDING', 'OPEN', 'PARTIALLY_FILLED', 'FILLED', 'CANCELLED', 'EXPIRED', 'REJECTED');

-- Create trades table
CREATE TABLE IF NOT EXISTS trades (
    id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    order_id VARCHAR(255) NOT NULL,
    platform trading_platform NOT NULL,
    token_in VARCHAR(255) NOT NULL,
    token_out VARCHAR(255) NOT NULL,
    side order_side NOT NULL,
    amount_in BIGINT NOT NULL,
    amount_out BIGINT NOT NULL,
    amount_in_min BIGINT,
    amount_out_min BIGINT,
    price DECIMAL(36, 18),
    slippage_percent DECIMAL(10, 4),
    gas_used BIGINT,
    gas_price BIGINT,
    transaction_hash VARCHAR(255) UNIQUE,
    block_number VARCHAR(255),
    status order_status NOT NULL DEFAULT 'PENDING',
    error_message TEXT,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
    executed_at TIMESTAMP WITH TIME ZONE,
    wallet_address VARCHAR(255) NOT NULL,
    dex_address VARCHAR(255),
    pool_address VARCHAR(255),
    fee_percent DECIMAL(10, 4),
    fee_amount BIGINT,
    chain_id VARCHAR(50) NOT NULL
);

-- Create positions table
CREATE TABLE IF NOT EXISTS positions (
    id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    wallet_address VARCHAR(255) NOT NULL,
    token_address VARCHAR(255) NOT NULL,
    token_symbol VARCHAR(20) NOT NULL,
    balance BIGINT NOT NULL DEFAULT 0,
    usd_value DECIMAL(36, 18),
    avg_entry_price DECIMAL(36, 18),
    current_price DECIMAL(36, 18),
    pnl_percent DECIMAL(10, 4),
    pnl_usd DECIMAL(36, 18),
    last_updated TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
    created_at TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
    volatility_24h DECIMAL(10, 4),
    liquidity_score DECIMAL(10, 4),
    is_whitelisted BOOLEAN DEFAULT false,
    tags TEXT[],
    UNIQUE(wallet_address, token_address)
);

-- Create market_data table
CREATE TABLE IF NOT EXISTS market_data (
    id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    platform trading_platform NOT NULL,
    token_address VARCHAR(255) NOT NULL,
    token_symbol VARCHAR(20) NOT NULL,
    pair_address VARCHAR(255),
    base_token VARCHAR(255),
    quote_token VARCHAR(255),
    price_usd DECIMAL(36, 18),
    price_native DECIMAL(36, 18),
    volume_24h DECIMAL(36, 18),
    market_cap DECIMAL(36, 18),
    liquidity_usd DECIMAL(36, 18),
    total_supply BIGINT,
    price_change_24h DECIMAL(10, 4),
    price_change_7d DECIMAL(10, 4),
    price_change_30d DECIMAL(10, 4),
    timestamp TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
    pool_liquidity BIGINT,
    fee_tier DECIMAL(10, 4),
    tvl BIGINT,
    apy DECIMAL(10, 4),
    UNIQUE(platform, token_address, timestamp)
);

-- Create liquidity_pools table
CREATE TABLE IF NOT EXISTS liquidity_pools (
    id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    platform trading_platform NOT NULL,
    pool_address VARCHAR(255) UNIQUE NOT NULL,
    token0_address VARCHAR(255) NOT NULL,
    token1_address VARCHAR(255) NOT NULL,
    token0_symbol VARCHAR(20) NOT NULL,
    token1_symbol VARCHAR(20) NOT NULL,
    token0_reserve BIGINT NOT NULL DEFAULT 0,
    token1_reserve BIGINT NOT NULL DEFAULT 0,
    total_liquidity BIGINT NOT NULL DEFAULT 0,
    fee_tier DECIMAL(10, 4),
    apy DECIMAL(10, 4),
    impermanent_loss_24h DECIMAL(10, 4),
    last_updated TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
    created_at TIMESTAMP WITH TIME ZONE DEFAULT NOW()
);

-- Create analytics_data table (time-series data for ClickHouse-style analytics)
CREATE TABLE IF NOT EXISTS analytics_data (
    id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    metric_name VARCHAR(255) NOT NULL,
    metric_type VARCHAR(100) NOT NULL,
    value DECIMAL(36, 18) NOT NULL,
    timestamp TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
    time_bucket VARCHAR(10) NOT NULL, -- '1m', '5m', '1h', '1d', etc.
    platform trading_platform,
    token_symbol VARCHAR(20),
    wallet_address VARCHAR(255),
    strategy_name VARCHAR(255),
    tags JSONB,
    INDEX idx_analytics_timestamp (timestamp),
    INDEX idx_analytics_metric (metric_name, timestamp),
    INDEX idx_analytics_platform (platform, timestamp)
);

-- Create risk_metrics table
CREATE TABLE IF NOT EXISTS risk_metrics (
    id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    wallet_address VARCHAR(255) NOT NULL,
    total_portfolio_value DECIMAL(36, 18),
    total_pnl_usd DECIMAL(36, 18),
    total_pnl_percent DECIMAL(10, 4),
    max_drawdown_percent DECIMAL(10, 4),
    sharpe_ratio DECIMAL(10, 4),
    volatility_annualized DECIMAL(10, 4),
    total_trades BIGINT DEFAULT 0,
    winning_trades BIGINT DEFAULT 0,
    losing_trades BIGINT DEFAULT 0,
    win_rate_percent DECIMAL(10, 4),
    avg_trade_size_usd DECIMAL(36, 18),
    largest_win_usd DECIMAL(36, 18),
    largest_loss_usd DECIMAL(36, 18),
    calculated_at TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
    max_position_size_percent DECIMAL(10, 4) DEFAULT 10.0,
    max_daily_loss_percent DECIMAL(10, 4) DEFAULT 5.0,
    max_drawdown_limit_percent DECIMAL(10, 4) DEFAULT 20.0,
    risk_limits_breached BOOLEAN DEFAULT false,
    UNIQUE(wallet_address, calculated_at)
);

-- Create performance_metrics table
CREATE TABLE IF NOT EXISTS performance_metrics (
    id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    timestamp TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
    cpu_usage_percent DECIMAL(10, 4),
    memory_usage_mb BIGINT,
    network_latency_ms DECIMAL(10, 4),
    active_connections BIGINT,
    total_requests BIGINT,
    error_count BIGINT,
    avg_response_time_ms DECIMAL(10, 4),
    trades_per_second BIGINT,
    orders_per_second BIGINT,
    db_query_time_ms DECIMAL(10, 4),
    db_connections_active BIGINT,
    db_connections_idle BIGINT,
    avg_slippage_percent DECIMAL(10, 4),
    success_rate_percent DECIMAL(10, 4),
    mev_attacks_detected BIGINT,
    mev_attacks_prevented BIGINT
);

-- Create indexes for performance
CREATE INDEX IF NOT EXISTS idx_trades_wallet ON trades(wallet_address, created_at DESC);
CREATE INDEX IF NOT EXISTS idx_trades_token_in ON trades(token_in, created_at DESC);
CREATE INDEX IF NOT EXISTS idx_trades_token_out ON trades(token_out, created_at DESC);
CREATE INDEX IF NOT EXISTS idx_trades_status ON trades(status, created_at DESC);
CREATE INDEX IF NOT EXISTS idx_trades_transaction_hash ON trades(transaction_hash);
CREATE INDEX IF NOT EXISTS idx_trades_created_at ON trades(created_at DESC);

CREATE INDEX IF NOT EXISTS idx_positions_wallet ON positions(wallet_address, last_updated DESC);
CREATE INDEX IF NOT EXISTS idx_positions_token ON positions(token_address, last_updated DESC);
CREATE INDEX IF NOT EXISTS idx_positions_pnl ON positions(pnl_percent DESC);

CREATE INDEX IF NOT EXISTS idx_market_data_token ON market_data(token_address, timestamp DESC);
CREATE INDEX IF NOT EXISTS idx_market_data_platform ON market_data(platform, timestamp DESC);
CREATE INDEX IF NOT EXISTS idx_market_data_symbol ON market_data(token_symbol, timestamp DESC);
CREATE INDEX IF NOT EXISTS idx_market_data_timestamp ON market_data(timestamp DESC);

CREATE INDEX IF NOT EXISTS idx_pools_platform ON liquidity_pools(platform, last_updated DESC);
CREATE INDEX IF NOT EXISTS idx_pools_tokens ON liquidity_pools(token0_address, token1_address);
CREATE INDEX IF NOT EXISTS idx_pools_liquidity ON liquidity_pools(total_liquidity DESC);

CREATE INDEX IF NOT EXISTS idx_risk_wallet ON risk_metrics(wallet_address, calculated_at DESC);
CREATE INDEX IF NOT EXISTS idx_risk_breached ON risk_metrics(risk_limits_breached, calculated_at DESC);

CREATE INDEX IF NOT EXISTS idx_performance_timestamp ON performance_metrics(timestamp DESC);

-- Create triggers for updated_at timestamps
CREATE OR REPLACE FUNCTION update_updated_at_column()
RETURNS TRIGGER AS $$
BEGIN
    NEW.updated_at = NOW();
    RETURN NEW;
END;
$$ language 'plpgsql';

CREATE TRIGGER update_trades_updated_at BEFORE UPDATE ON trades
    FOR EACH ROW EXECUTE FUNCTION update_updated_at_column();

CREATE TRIGGER update_positions_updated_at BEFORE UPDATE ON positions
    FOR EACH ROW EXECUTE FUNCTION update_updated_at_column();

CREATE TRIGGER update_pools_updated_at BEFORE UPDATE ON liquidity_pools
    FOR EACH ROW EXECUTE FUNCTION update_updated_at_column();

-- Create views for common queries
CREATE OR REPLACE VIEW recent_trades AS
SELECT * FROM trades
WHERE created_at >= NOW() - INTERVAL '24 hours'
ORDER BY created_at DESC;

CREATE OR REPLACE VIEW portfolio_summary AS
SELECT
    wallet_address,
    COUNT(*) as total_positions,
    SUM(usd_value) as total_value,
    SUM(pnl_usd) as total_pnl,
    AVG(pnl_percent) as avg_pnl_percent,
    MAX(last_updated) as last_updated
FROM positions
WHERE balance > 0
GROUP BY wallet_address;

CREATE OR REPLACE VIEW trading_stats_24h AS
SELECT
    COUNT(*) as total_trades,
    COUNT(CASE WHEN status = 'FILLED' THEN 1 END) as successful_trades,
    SUM(amount_in) as total_volume_in,
    SUM(amount_out) as total_volume_out,
    AVG(price) as avg_price,
    AVG(slippage_percent) as avg_slippage,
    SUM(gas_used * gas_price) as total_gas_cost
FROM trades
WHERE created_at >= NOW() - INTERVAL '24 hours';

-- Insert some sample data (optional)
-- This would typically be done by the application

-- Create database user (if needed)
-- GRANT ALL PRIVILEGES ON DATABASE hydraflow TO hydraflow;
-- GRANT ALL PRIVILEGES ON ALL TABLES IN SCHEMA public TO hydraflow;
-- GRANT ALL PRIVILEGES ON ALL SEQUENCES IN SCHEMA public TO hydraflow;