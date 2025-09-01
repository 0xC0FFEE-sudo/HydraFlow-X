// API configuration for HydraFlow-X C++ backend integration

export const API_CONFIG = {
  // C++ REST API endpoints
  REST_BASE_URL: process.env.NEXT_PUBLIC_API_URL || 'http://localhost:8080',

  // WebSocket endpoints for real-time data
  WS_BASE_URL: process.env.NEXT_PUBLIC_WS_URL || 'ws://localhost:8081',
  
  // API endpoints mapping to C++ backend routes
  ENDPOINTS: {
    // System & Health
    HEALTH: '/health',
    SYSTEM_METRICS: '/api/system/metrics',
    PERFORMANCE: '/api/performance',
    ALERTS: '/api/alerts',
    
    // Trading
    POSITIONS: '/api/trading/positions',
    TRADES: '/api/trading/trades',
    ORDER_BOOK: '/api/trading/orderbook',
    PLACE_ORDER: '/api/trading/order',
    CANCEL_ORDER: '/api/trading/order/cancel',
    STRATEGIES: '/api/trading/strategies',
    
    // MEV Protection
    MEV_STATUS: '/api/mev/status',
    MEV_BUNDLES: '/api/mev/bundles',
    MEV_CONFIG: '/api/mev/config',
    
    // Wallets & Networks
    WALLETS: '/api/wallets',
    NETWORKS: '/api/networks',
    BALANCES: '/api/balances',
    
    // Analytics
    ANALYTICS: '/api/analytics',
    ARBITRAGE: '/api/arbitrage/opportunities',
    
    // Configuration
    CONFIG: '/api/config',
    UPDATE_CONFIG: '/api/config/update',
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
