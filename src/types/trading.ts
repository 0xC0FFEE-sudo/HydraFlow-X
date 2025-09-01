// Core trading types for HydraFlow-X backend integration

export interface Position {
  id: string;
  symbol: string;
  side: 'long' | 'short';
  size: number;
  entryPrice: number;
  currentPrice: number;
  pnl: number;
  pnlPercent: number;
  unrealizedPnl: number;
  realizedPnl: number;
  openTime: number;
  status: 'open' | 'closed' | 'pending';
  leverage: number;
  marginUsed: number;
}

export interface Trade {
  id: string;
  symbol: string;
  side: 'buy' | 'sell';
  type: 'market' | 'limit' | 'stop';
  quantity: number;
  price: number;
  executedPrice?: number;
  executedQuantity?: number;
  fee: number;
  timestamp: number;
  status: 'pending' | 'filled' | 'cancelled' | 'rejected';
  latency: number; // microseconds
  orderId: string;
  source: 'manual' | 'bot' | 'mev' | 'arbitrage';
}

export interface OrderBook {
  symbol: string;
  bids: [number, number][]; // [price, quantity]
  asks: [number, number][]; // [price, quantity]
  timestamp: number;
  spread: number;
  midPrice: number;
}

export interface TradingStrategy {
  id: string;
  name: string;
  type: 'sniper' | 'arbitrage' | 'mev' | 'dca' | 'grid';
  status: 'active' | 'paused' | 'stopped';
  profitLoss: number;
  winRate: number;
  totalTrades: number;
  avgLatency: number;
  riskLevel: 'low' | 'medium' | 'high';
  parameters: Record<string, any>;
}
