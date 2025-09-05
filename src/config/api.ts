// API configuration for HydraFlow-X C++ backend integration

export const API_CONFIG = {
  // C++ REST API endpoints
  REST_BASE_URL: process.env.NEXT_PUBLIC_API_URL || 'http://localhost:8083',

  // WebSocket endpoints for real-time data
  WS_BASE_URL: process.env.NEXT_PUBLIC_WS_URL || 'ws://localhost:8083',
  
  // API endpoints mapping to C++ backend routes
  ENDPOINTS: {
    // System & Health
    STATUS: '/api/v1/status',
    SYSTEM_INFO: '/api/v1/system/info',
    HEALTH: '/api/v1/health',
    METRICS: '/api/v1/metrics',
    PERFORMANCE: '/api/v1/performance/stats',

    // Trading
    ORDERS: '/api/v1/trading/orders',
    CREATE_ORDER: '/api/v1/trading/orders',
    POSITIONS: '/api/v1/trading/positions',
    BALANCES: '/api/v1/trading/balances',
    TRADE_HISTORY: '/api/v1/trading/history',
    TRADES: '/api/v1/trading/trades',
    MARKET_PRICES: '/api/v1/market/prices',
    START_TRADING: '/api/v1/trading/start',
    STOP_TRADING: '/api/v1/trading/stop',
    UPDATE_CONFIG: '/api/v1/config/update',
    CONFIG: '/api/v1/config',
    WALLETS: '/api/v1/wallets',
    ANALYTICS: '/api/v1/analytics',

    // Risk Management
    RISK_METRICS: '/api/v1/risk/metrics',
    CIRCUIT_BREAKERS: '/api/v1/risk/circuit-breakers',
    TRADING_ALLOWED: '/api/v1/risk/trading-allowed',

    // MEV Protection
    MEV_STATUS: '/api/v1/mev/status',
    MEV_PROTECTION_STATS: '/api/v1/mev/protection-stats',
    MEV_RELAYS: '/api/v1/mev/relays',
    MEV_BUNDLES: '/api/v1/mev/bundles',
    MEV_CONFIG: '/api/v1/mev/config',

    // Authentication
    AUTH_LOGIN: '/api/v1/auth/login',
    AUTH_LOGOUT: '/api/v1/auth/logout',
    AUTH_VERIFY: '/api/v1/auth/verify',
    AUTH_REFRESH: '/api/v1/auth/refresh',
    AUTH_REGISTER: '/api/v1/auth/register',

    // WebSocket Info
    WEBSOCKET_INFO: '/api/v1/websocket/info',
  },
  
  // WebSocket channels for real-time updates
  WS_CHANNELS: {
    SYSTEM_METRICS: 'system:metrics',
    PRICE_FEEDS: 'market:prices',
    TRADE_UPDATES: 'trading:updates',
    MEV_ALERTS: 'mev:alerts',
    ORDER_BOOK: 'orderbook:updates',
    POSITIONS: 'positions:updates',
    PERFORMANCE: 'performance:metrics',
  },
  
  // Request configuration
  REQUEST_TIMEOUT: 10000, // 10 seconds
  WS_RECONNECT_INTERVAL: 5000, // 5 seconds
  RETRY_ATTEMPTS: 3,
  
  // Real-time update intervals
  METRICS_UPDATE_INTERVAL: 1000, // 1 second
  PRICE_UPDATE_INTERVAL: 100, // 100ms for ultra-low latency
  POSITION_UPDATE_INTERVAL: 500, // 500ms
} as const;

export type APIEndpoint = keyof typeof API_CONFIG.ENDPOINTS;
export type WSChannel = keyof typeof API_CONFIG.WS_CHANNELS;
