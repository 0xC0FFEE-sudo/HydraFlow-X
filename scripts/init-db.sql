-- HydraFlow-X Database Initialization Script
-- Creates the necessary tables and indexes for the trading platform

-- Create extensions
CREATE EXTENSION IF NOT EXISTS "uuid-ossp";
CREATE EXTENSION IF NOT EXISTS "pg_stat_statements";
CREATE EXTENSION IF NOT EXISTS "btree_gist";

-- Create schemas
CREATE SCHEMA IF NOT EXISTS trading;
CREATE SCHEMA IF NOT EXISTS analytics;
CREATE SCHEMA IF NOT EXISTS monitoring;
CREATE SCHEMA IF NOT EXISTS config;

-- Set search path
SET search_path TO trading, analytics, monitoring, config, public;

-- Trading tables
CREATE TABLE IF NOT EXISTS trading.positions (
    id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    symbol VARCHAR(50) NOT NULL,
    side VARCHAR(4) NOT NULL CHECK (side IN ('buy', 'sell')),
    amount DECIMAL(24,8) NOT NULL,
    price DECIMAL(24,8) NOT NULL,
    value DECIMAL(24,8) NOT NULL,
    pnl DECIMAL(24,8) DEFAULT 0,
    status VARCHAR(20) NOT NULL DEFAULT 'active' CHECK (status IN ('active', 'closed', 'liquidated')),
    wallet_address VARCHAR(42) NOT NULL,
    chain VARCHAR(20) NOT NULL,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
    closed_at TIMESTAMP WITH TIME ZONE
);

CREATE TABLE IF NOT EXISTS trading.trades (
    id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    position_id UUID REFERENCES trading.positions(id),
    symbol VARCHAR(50) NOT NULL,
    type VARCHAR(4) NOT NULL CHECK (type IN ('buy', 'sell')),
    amount DECIMAL(24,8) NOT NULL,
    price DECIMAL(24,8) NOT NULL,
    fee DECIMAL(24,8) DEFAULT 0,
    gas_used BIGINT DEFAULT 0,
    gas_price DECIMAL(24,8) DEFAULT 0,
    transaction_hash VARCHAR(66),
    block_number BIGINT,
    status VARCHAR(20) NOT NULL DEFAULT 'pending' CHECK (status IN ('pending', 'completed', 'failed', 'cancelled')),
    execution_time_ms INTEGER,
    slippage_bps INTEGER,
    mev_protection_level VARCHAR(20),
    wallet_address VARCHAR(42) NOT NULL,
    chain VARCHAR(20) NOT NULL,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
    completed_at TIMESTAMP WITH TIME ZONE
);

CREATE TABLE IF NOT EXISTS trading.wallets (
    id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    address VARCHAR(42) UNIQUE NOT NULL,
    name VARCHAR(100),
    encrypted_private_key TEXT,
    is_primary BOOLEAN DEFAULT FALSE,
    is_enabled BOOLEAN DEFAULT TRUE,
    balance_eth DECIMAL(24,8) DEFAULT 0,
    balance_sol DECIMAL(24,8) DEFAULT 0,
    active_trades INTEGER DEFAULT 0,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT NOW()
);

CREATE TABLE IF NOT EXISTS trading.strategies (
    id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    name VARCHAR(100) NOT NULL,
    type VARCHAR(50) NOT NULL,
    config JSONB NOT NULL,
    is_enabled BOOLEAN DEFAULT TRUE,
    performance_metrics JSONB,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT NOW()
);

-- Analytics tables
CREATE TABLE IF NOT EXISTS analytics.performance_metrics (
    id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    metric_name VARCHAR(100) NOT NULL,
    metric_value DECIMAL(24,8) NOT NULL,
    metric_unit VARCHAR(20),
    timestamp TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
    labels JSONB
);

CREATE TABLE IF NOT EXISTS analytics.latency_measurements (
    id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    component VARCHAR(50) NOT NULL,
    operation VARCHAR(50) NOT NULL,
    latency_ms DECIMAL(10,3) NOT NULL,
    timestamp TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
    metadata JSONB
);

CREATE TABLE IF NOT EXISTS analytics.market_data (
    id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    symbol VARCHAR(50) NOT NULL,
    price DECIMAL(24,8) NOT NULL,
    volume_24h DECIMAL(24,8),
    market_cap DECIMAL(24,8),
    change_24h DECIMAL(8,4),
    liquidity DECIMAL(24,8),
    source VARCHAR(50) NOT NULL,
    timestamp TIMESTAMP WITH TIME ZONE DEFAULT NOW()
);

-- Monitoring tables
CREATE TABLE IF NOT EXISTS monitoring.system_health (
    id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    component VARCHAR(100) NOT NULL,
    status VARCHAR(20) NOT NULL CHECK (status IN ('healthy', 'warning', 'error', 'unknown')),
    message TEXT,
    uptime_seconds BIGINT,
    last_check TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
    metadata JSONB
);

CREATE TABLE IF NOT EXISTS monitoring.alerts (
    id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    alert_type VARCHAR(50) NOT NULL,
    severity VARCHAR(20) NOT NULL CHECK (severity IN ('info', 'warning', 'error', 'critical')),
    title VARCHAR(200) NOT NULL,
    description TEXT,
    component VARCHAR(100),
    is_resolved BOOLEAN DEFAULT FALSE,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
    resolved_at TIMESTAMP WITH TIME ZONE,
    metadata JSONB
);

CREATE TABLE IF NOT EXISTS monitoring.mev_protection_logs (
    id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    trade_id UUID REFERENCES trading.trades(id),
    protection_type VARCHAR(50) NOT NULL,
    threat_type VARCHAR(50),
    action_taken VARCHAR(100),
    effectiveness_score DECIMAL(5,2),
    timestamp TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
    details JSONB
);

-- Configuration tables
CREATE TABLE IF NOT EXISTS config.api_configs (
    id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    provider VARCHAR(50) UNIQUE NOT NULL,
    encrypted_api_key TEXT,
    encrypted_secret_key TEXT,
    base_url VARCHAR(255),
    rate_limit_per_second INTEGER DEFAULT 100,
    is_enabled BOOLEAN DEFAULT TRUE,
    custom_headers JSONB,
    status VARCHAR(20) DEFAULT 'disconnected',
    last_test TIMESTAMP WITH TIME ZONE,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT NOW()
);

CREATE TABLE IF NOT EXISTS config.rpc_configs (
    id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    chain VARCHAR(50) NOT NULL,
    provider VARCHAR(50) NOT NULL,
    endpoint VARCHAR(255) NOT NULL,
    encrypted_api_key TEXT,
    websocket_endpoint VARCHAR(255),
    timeout_ms INTEGER DEFAULT 5000,
    max_connections INTEGER DEFAULT 10,
    is_enabled BOOLEAN DEFAULT TRUE,
    status VARCHAR(20) DEFAULT 'disconnected',
    last_test TIMESTAMP WITH TIME ZONE,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
    UNIQUE(chain, provider)
);

-- Create indexes for performance
CREATE INDEX IF NOT EXISTS idx_positions_symbol ON trading.positions(symbol);
CREATE INDEX IF NOT EXISTS idx_positions_status ON trading.positions(status);
CREATE INDEX IF NOT EXISTS idx_positions_wallet ON trading.positions(wallet_address);
CREATE INDEX IF NOT EXISTS idx_positions_created_at ON trading.positions(created_at);

CREATE INDEX IF NOT EXISTS idx_trades_symbol ON trading.trades(symbol);
CREATE INDEX IF NOT EXISTS idx_trades_status ON trading.trades(status);
CREATE INDEX IF NOT EXISTS idx_trades_wallet ON trading.trades(wallet_address);
CREATE INDEX IF NOT EXISTS idx_trades_created_at ON trading.trades(created_at);
CREATE INDEX IF NOT EXISTS idx_trades_tx_hash ON trading.trades(transaction_hash);

CREATE INDEX IF NOT EXISTS idx_wallets_address ON trading.wallets(address);
CREATE INDEX IF NOT EXISTS idx_wallets_primary ON trading.wallets(is_primary) WHERE is_primary = TRUE;
CREATE INDEX IF NOT EXISTS idx_wallets_enabled ON trading.wallets(is_enabled) WHERE is_enabled = TRUE;

CREATE INDEX IF NOT EXISTS idx_performance_metrics_name ON analytics.performance_metrics(metric_name);
CREATE INDEX IF NOT EXISTS idx_performance_metrics_timestamp ON analytics.performance_metrics(timestamp);

CREATE INDEX IF NOT EXISTS idx_latency_component ON analytics.latency_measurements(component);
CREATE INDEX IF NOT EXISTS idx_latency_timestamp ON analytics.latency_measurements(timestamp);

CREATE INDEX IF NOT EXISTS idx_market_data_symbol ON analytics.market_data(symbol);
CREATE INDEX IF NOT EXISTS idx_market_data_timestamp ON analytics.market_data(timestamp);

CREATE INDEX IF NOT EXISTS idx_system_health_component ON monitoring.system_health(component);
CREATE INDEX IF NOT EXISTS idx_system_health_status ON monitoring.system_health(status);

CREATE INDEX IF NOT EXISTS idx_alerts_severity ON monitoring.alerts(severity);
CREATE INDEX IF NOT EXISTS idx_alerts_resolved ON monitoring.alerts(is_resolved);
CREATE INDEX IF NOT EXISTS idx_alerts_created ON monitoring.alerts(created_at);

-- Create triggers for updated_at columns
CREATE OR REPLACE FUNCTION update_updated_at_column()
RETURNS TRIGGER AS $$
BEGIN
    NEW.updated_at = NOW();
    RETURN NEW;
END;
$$ language 'plpgsql';

CREATE TRIGGER update_positions_updated_at BEFORE UPDATE ON trading.positions
    FOR EACH ROW EXECUTE FUNCTION update_updated_at_column();

CREATE TRIGGER update_wallets_updated_at BEFORE UPDATE ON trading.wallets
    FOR EACH ROW EXECUTE FUNCTION update_updated_at_column();

CREATE TRIGGER update_strategies_updated_at BEFORE UPDATE ON trading.strategies
    FOR EACH ROW EXECUTE FUNCTION update_updated_at_column();

CREATE TRIGGER update_api_configs_updated_at BEFORE UPDATE ON config.api_configs
    FOR EACH ROW EXECUTE FUNCTION update_updated_at_column();

CREATE TRIGGER update_rpc_configs_updated_at BEFORE UPDATE ON config.rpc_configs
    FOR EACH ROW EXECUTE FUNCTION update_updated_at_column();

-- Insert default data
INSERT INTO config.api_configs (provider, base_url, is_enabled, status) VALUES 
    ('twitter', 'https://api.twitter.com/2', FALSE, 'disconnected'),
    ('reddit', 'https://oauth.reddit.com', FALSE, 'disconnected'),
    ('dexscreener', 'https://api.dexscreener.com/latest', FALSE, 'disconnected'),
    ('gmgn', 'https://gmgn.ai/api', FALSE, 'disconnected')
ON CONFLICT (provider) DO NOTHING;

INSERT INTO config.rpc_configs (chain, provider, endpoint, is_enabled, status) VALUES 
    ('ethereum', 'alchemy', 'https://eth-mainnet.g.alchemy.com/v2/YOUR_API_KEY', FALSE, 'disconnected'),
    ('solana', 'helius', 'https://rpc.helius.xyz/?api-key=YOUR_API_KEY', FALSE, 'disconnected'),
    ('base', 'base', 'https://mainnet.base.org', FALSE, 'disconnected'),
    ('arbitrum', 'arbitrum', 'https://arb1.arbitrum.io/rpc', FALSE, 'disconnected')
ON CONFLICT (chain, provider) DO NOTHING;

-- Create views for easier querying
CREATE OR REPLACE VIEW trading.active_positions AS
SELECT * FROM trading.positions WHERE status = 'active';

CREATE OR REPLACE VIEW trading.recent_trades AS
SELECT * FROM trading.trades 
WHERE created_at >= NOW() - INTERVAL '24 hours'
ORDER BY created_at DESC;

CREATE OR REPLACE VIEW analytics.latest_metrics AS
SELECT DISTINCT ON (metric_name) 
    metric_name, metric_value, metric_unit, timestamp
FROM analytics.performance_metrics
ORDER BY metric_name, timestamp DESC;

-- Grant permissions
GRANT ALL PRIVILEGES ON ALL TABLES IN SCHEMA trading TO hydraflow;
GRANT ALL PRIVILEGES ON ALL TABLES IN SCHEMA analytics TO hydraflow;
GRANT ALL PRIVILEGES ON ALL TABLES IN SCHEMA monitoring TO hydraflow;
GRANT ALL PRIVILEGES ON ALL TABLES IN SCHEMA config TO hydraflow;
GRANT ALL PRIVILEGES ON ALL SEQUENCES IN SCHEMA trading TO hydraflow;
GRANT ALL PRIVILEGES ON ALL SEQUENCES IN SCHEMA analytics TO hydraflow;
GRANT ALL PRIVILEGES ON ALL SEQUENCES IN SCHEMA monitoring TO hydraflow;
GRANT ALL PRIVILEGES ON ALL SEQUENCES IN SCHEMA config TO hydraflow;

-- Create materialized views for performance
CREATE MATERIALIZED VIEW IF NOT EXISTS analytics.trading_summary AS
SELECT 
    DATE_TRUNC('hour', created_at) as hour,
    COUNT(*) as trade_count,
    SUM(CASE WHEN type = 'buy' THEN amount ELSE 0 END) as total_buy_volume,
    SUM(CASE WHEN type = 'sell' THEN amount ELSE 0 END) as total_sell_volume,
    AVG(execution_time_ms) as avg_execution_time,
    COUNT(CASE WHEN status = 'completed' THEN 1 END) * 100.0 / COUNT(*) as success_rate
FROM trading.trades
WHERE created_at >= NOW() - INTERVAL '7 days'
GROUP BY DATE_TRUNC('hour', created_at)
ORDER BY hour DESC;

CREATE UNIQUE INDEX IF NOT EXISTS idx_trading_summary_hour ON analytics.trading_summary(hour);

-- Set up refresh for materialized view
CREATE OR REPLACE FUNCTION refresh_trading_summary()
RETURNS void AS $$
BEGIN
    REFRESH MATERIALIZED VIEW CONCURRENTLY analytics.trading_summary;
END;
$$ LANGUAGE plpgsql;
