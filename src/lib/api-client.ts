// API client for HydraFlow-X C++ backend integration

import { API_CONFIG } from '@/config/api';
import type {
  SystemMetrics,
  PerformanceMetrics
} from '@/types/system';
import type { 
  Position, 
  Trade, 
  OrderBook, 
  TradingStrategy 
} from '@/types/trading';
import type { 
  MEVProtectionStatus, 
  MEVBundle, 
  BlockchainNetwork, 
  Wallet, 
  ArbitrageOpportunity 
} from '@/types/mev';

class APIClient {
  private baseURL: string;
  private timeout: number;

  constructor() {
    this.baseURL = API_CONFIG.REST_BASE_URL;
    this.timeout = API_CONFIG.REQUEST_TIMEOUT;
  }

  private async request<T>(
    endpoint: string,
    options: RequestInit = {}
  ): Promise<T> {
    const url = `${this.baseURL}${endpoint}`;
    const config: RequestInit = {
      headers: {
        'Content-Type': 'application/json',
        ...options.headers,
      },
      ...options,
    };

    const controller = new AbortController();
    const timeoutId = setTimeout(() => controller.abort(), this.timeout);

    try {
      const response = await fetch(url, {
        ...config,
        signal: controller.signal,
      });

      clearTimeout(timeoutId);

      if (!response.ok) {
        throw new Error(`API Error: ${response.status} ${response.statusText}`);
      }

      return await response.json();
    } catch (error) {
      clearTimeout(timeoutId);
      if (error instanceof Error && error.name === 'AbortError') {
        throw new Error('Request timeout');
      }
      throw error;
    }
  }

  // System & Health APIs
  async getSystemMetrics(): Promise<SystemMetrics> {
    return this.request<SystemMetrics>(API_CONFIG.ENDPOINTS.SYSTEM_METRICS);
  }

  async getPerformanceMetrics(): Promise<PerformanceMetrics> {
    return this.request<PerformanceMetrics>(API_CONFIG.ENDPOINTS.PERFORMANCE);
  }

  async getAlerts(): Promise<any[]> {
    return this.request<any[]>(API_CONFIG.ENDPOINTS.ALERTS);
  }

  async getHealthStatus(): Promise<any[]> {
    return this.request<any[]>(API_CONFIG.ENDPOINTS.HEALTH);
  }

  // Trading APIs
  async getPositions(): Promise<Position[]> {
    return this.request<Position[]>(API_CONFIG.ENDPOINTS.POSITIONS);
  }

  async getTrades(limit?: number): Promise<Trade[]> {
    const params = limit ? `?limit=${limit}` : '';
    return this.request<Trade[]>(`${API_CONFIG.ENDPOINTS.TRADES}${params}`);
  }

  async getOrderBook(symbol: string): Promise<OrderBook> {
    return this.request<OrderBook>(`${API_CONFIG.ENDPOINTS.ORDER_BOOK}/${symbol}`);
  }

  async placeOrder(order: {
    symbol: string;
    side: 'buy' | 'sell';
    type: 'market' | 'limit';
    quantity: number;
    price?: number;
  }): Promise<{ orderId: string; status: string }> {
    return this.request(API_CONFIG.ENDPOINTS.PLACE_ORDER, {
      method: 'POST',
      body: JSON.stringify(order),
    });
  }

  async cancelOrder(orderId: string): Promise<{ success: boolean }> {
    return this.request(API_CONFIG.ENDPOINTS.CANCEL_ORDER, {
      method: 'DELETE',
      body: JSON.stringify({ orderId }),
    });
  }

  async getStrategies(): Promise<TradingStrategy[]> {
    return this.request<TradingStrategy[]>(API_CONFIG.ENDPOINTS.STRATEGIES);
  }

  // MEV Protection APIs
  async getMEVStatus(): Promise<MEVProtectionStatus> {
    return this.request<MEVProtectionStatus>(API_CONFIG.ENDPOINTS.MEV_STATUS);
  }

  async getMEVBundles(): Promise<MEVBundle[]> {
    return this.request<MEVBundle[]>(API_CONFIG.ENDPOINTS.MEV_BUNDLES);
  }

  async updateMEVConfig(config: {
    enabled: boolean;
    mode: 'aggressive' | 'balanced' | 'conservative';
    priorityFee: number;
  }): Promise<{ success: boolean }> {
    return this.request(API_CONFIG.ENDPOINTS.MEV_CONFIG, {
      method: 'PUT',
      body: JSON.stringify(config),
    });
  }

  // Wallet & Network APIs
  async getWallets(): Promise<Wallet[]> {
    return this.request<Wallet[]>(API_CONFIG.ENDPOINTS.WALLETS);
  }

  async getNetworks(): Promise<BlockchainNetwork[]> {
    return this.request<BlockchainNetwork[]>(API_CONFIG.ENDPOINTS.NETWORKS);
  }

  async getArbitrageOpportunities(): Promise<ArbitrageOpportunity[]> {
    return this.request<ArbitrageOpportunity[]>(API_CONFIG.ENDPOINTS.ARBITRAGE);
  }

  // Configuration APIs
  async getConfig(): Promise<Record<string, any>> {
    return this.request<Record<string, any>>(API_CONFIG.ENDPOINTS.CONFIG);
  }

  async updateConfig(config: Record<string, any>): Promise<{ success: boolean }> {
    return this.request(API_CONFIG.ENDPOINTS.UPDATE_CONFIG, {
      method: 'PUT',
      body: JSON.stringify(config),
    });
  }
}

// Create and export singleton instance
export const apiClient = new APIClient();

// Mock data for development (when backend is not available)
export const mockData = {
  systemMetrics: {
    timestamp: Date.now(),
    cpu: 45.2,
    memory: 25.0,
    network_rx: 1250.0,
    network_tx: 980.0,
    latency: 12.5,
    uptime: 86400,
  } as SystemMetrics,

  performanceMetrics: {
    trades_per_second: 12.5,
    avg_latency_ms: 15.2,
    success_rate: 98.7,
    total_trades: 15847,
    active_positions: 7,
    system_load: 45.2,
  } as PerformanceMetrics,
};
