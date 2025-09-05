// API service for communicating with HydraFlow C++ backend

import { API_CONFIG } from '@/config/api';
import type { Position, Trade, TradingStrategy } from '@/types/trading';
import type { SystemMetrics, PerformanceMetrics, SystemHealth } from '@/types/system';

class ApiService {
  private baseUrl: string;

  constructor() {
    this.baseUrl = API_CONFIG.REST_BASE_URL;
  }

  async get<T = any>(endpoint: string): Promise<T> {
    return this.request<T>(endpoint, { method: 'GET' });
  }

  async post<T = any>(endpoint: string, data?: any): Promise<T> {
    return this.request<T>(endpoint, {
      method: 'POST',
      body: data ? JSON.stringify(data) : undefined,
    });
  }

  private async request<T>(
    endpoint: string,
    options: RequestInit = {}
  ): Promise<T> {
    const url = `${this.baseUrl}${endpoint}`;

    const config: RequestInit = {
      headers: {
        'Content-Type': 'application/json',
        ...options.headers,
      },
      ...options,
    };

    try {
      const response = await fetch(url, config);

      if (!response.ok) {
        throw new Error(`HTTP ${response.status}: ${response.statusText}`);
      }

      return await response.json();
    } catch (error) {
      console.error(`API request failed for ${endpoint}:`, error);
      throw error;
    }
  }

  // System APIs
  async getSystemStatus(): Promise<any> {
    return this.request(API_CONFIG.ENDPOINTS.STATUS);
  }

  async getSystemInfo(): Promise<any> {
    return this.request(API_CONFIG.ENDPOINTS.SYSTEM_INFO);
  }

  async getMarketPrices(): Promise<{
    ethereum: {
      gas_price: string;
      block_number: string;
      pending_txs: string;
    };
    solana: {
      slot: string;
      tps: string;
      jito_tips: string;
    };
    tokens: Array<{
      symbol: string;
      price: string;
      change_24h: string;
    }>;
  }> {
    return this.request('/api/v1/market/prices');
  }

  async getWebSocketInfo(): Promise<any> {
    return this.request(API_CONFIG.ENDPOINTS.WEBSOCKET_INFO);
  }

  // Trading APIs
  async getOrders(): Promise<any> {
    return this.request(API_CONFIG.ENDPOINTS.ORDERS);
  }

  async createOrder(order: {
    symbol: string;
    side: 'buy' | 'sell';
    type: 'market' | 'limit';
    quantity: number;
    price?: number;
  }): Promise<any> {
    return this.request(API_CONFIG.ENDPOINTS.CREATE_ORDER, {
      method: 'POST',
      body: JSON.stringify(order),
    });
  }


  async getBalances(): Promise<any> {
    return this.request(API_CONFIG.ENDPOINTS.BALANCES);
  }



  // MEV Protection APIs

  async getMEVProtectionStats(): Promise<any> {
    return this.request(API_CONFIG.ENDPOINTS.MEV_PROTECTION_STATS);
  }

  async getMEVRelays(): Promise<any> {
    return this.request(API_CONFIG.ENDPOINTS.MEV_RELAYS);
  }

  // Risk Management APIs
  async getRiskMetrics(): Promise<any> {
    return this.request(API_CONFIG.ENDPOINTS.RISK_METRICS);
  }

  async getCircuitBreakers(): Promise<any> {
    return this.request(API_CONFIG.ENDPOINTS.CIRCUIT_BREAKERS);
  }

  async getTradingAllowed(): Promise<any> {
    return this.request(API_CONFIG.ENDPOINTS.TRADING_ALLOWED);
  }

  // Trading APIs
  async getTradingStatus(): Promise<{
    active: boolean;
    strategy: string;
    uptime_seconds: number;
    total_trades: number;
    success_rate: number;
    avg_latency_ms: number;
    current_positions: number;
    available_balance: number;
  }> {
    return this.request(API_CONFIG.ENDPOINTS.HEALTH);
  }

  async startTrading(config: {
    mode: string;
    settings: {
      maxPositionSize: number;
      maxSlippage: number;
      enableMEVProtection: boolean;
      enableSniperMode: boolean;
    };
  }): Promise<{ status: string; strategy: string }> {
    return this.request(API_CONFIG.ENDPOINTS.UPDATE_CONFIG, {
      method: 'POST',
      body: JSON.stringify(config),
    });
  }

  async stopTrading(): Promise<{ status: string; positions_closed: number }> {
    return this.request('/api/trading/stop', {
      method: 'POST',
    });
  }

  async placeOrder(order: {
    symbol: string;
    side: 'buy' | 'sell';
    amount: number;
    slippage?: number;
  }): Promise<{
    order_id: string;
    status: string;
    platform: string;
    symbol: string;
    side: string;
    amount: string;
    price: string;
    timestamp: string;
    estimated_gas: string;
  }> {
    return this.request('/api/v1/orders', {
      method: 'POST',
      body: JSON.stringify(order),
    });
  }

  async getPositions(params?: {
    status?: 'active' | 'closed' | 'all';
    limit?: number;
    offset?: number;
  }): Promise<{ positions: Position[]; total_count: number; total_value_usd: number }> {
    const queryParams = params ? new URLSearchParams(params as any).toString() : '';
    const endpoint = queryParams ? `${API_CONFIG.ENDPOINTS.POSITIONS}?${queryParams}` : API_CONFIG.ENDPOINTS.POSITIONS;
    return this.request(endpoint);
  }

  async getTradeHistory(params?: {
    start_date?: string;
    end_date?: string;
    symbol?: string;
    status?: string;
    limit?: number;
  }): Promise<{
    trades: Trade[];
    total_count: number;
    summary: {
      total_volume_usd: number;
      total_fees_usd: number;
      avg_execution_time_ms: number;
    };
  }> {
    const queryParams = params ? new URLSearchParams(params as any).toString() : '';
    const endpoint = queryParams ? `${API_CONFIG.ENDPOINTS.TRADES}?${queryParams}` : API_CONFIG.ENDPOINTS.TRADES;
    return this.request(endpoint);
  }

  // Configuration APIs
  async getConfiguration(): Promise<{
    apis: Record<string, any>;
    rpcs: Record<string, any>;
  }> {
    return this.request(API_CONFIG.ENDPOINTS.CONFIG);
  }

  async updateConfiguration(config: any): Promise<{ success: boolean }> {
    return this.request(API_CONFIG.ENDPOINTS.UPDATE_CONFIG, {
      method: 'PUT',
      body: JSON.stringify(config),
    });
  }

  async testConnection(config: {
    type: string;
    name: string;
    config: any;
  }): Promise<{
    connection_status: string;
    latency_ms: number;
    details: string;
  }> {
    return this.request(API_CONFIG.ENDPOINTS.UPDATE_CONFIG, {
      method: 'POST',
      body: JSON.stringify(config),
    });
  }

  // Wallet APIs
  async getWallets(): Promise<{
    wallets: Array<{
      id: string;
      address: string;
      name: string;
      balance_eth: number;
      balance_usd: number;
      active_trades: number;
      is_primary: boolean;
      enabled: boolean;
    }>;
    total_count: number;
    total_balance_usd: number;
  }> {
    return this.request(API_CONFIG.ENDPOINTS.WALLETS);
  }

  async addWallet(wallet: {
    name: string;
    private_key: string;
    enabled: boolean;
  }): Promise<{ success: boolean; wallet_id: string }> {
    return this.request(API_CONFIG.ENDPOINTS.WALLETS, {
      method: 'POST',
      body: JSON.stringify(wallet),
    });
  }

  async removeWallet(walletId: string): Promise<{ success: boolean }> {
    return this.request(`${API_CONFIG.ENDPOINTS.WALLETS}/${walletId}`, {
      method: 'DELETE',
    });
  }

  // Analytics APIs
  async getAnalytics(params?: {
    timeframe?: string;
    metrics?: string[];
  }): Promise<any> {
    const queryParams = params ? new URLSearchParams(params as any).toString() : '';
    const endpoint = queryParams ? `${API_CONFIG.ENDPOINTS.ANALYTICS}?${queryParams}` : API_CONFIG.ENDPOINTS.ANALYTICS;
    return this.request(endpoint);
  }

  // MEV Protection APIs
  async getMEVStatus(): Promise<{
    enabled: boolean;
    active_bundles: number;
    protection_score: number;
    recent_blocks: number;
    mev_extracted: number;
  }> {
    return this.request(API_CONFIG.ENDPOINTS.MEV_STATUS);
  }

  async getMEVBundles(): Promise<any[]> {
    return this.request(API_CONFIG.ENDPOINTS.MEV_BUNDLES);
  }

  async updateMEVConfig(config: any): Promise<{ success: boolean }> {
    return this.request(API_CONFIG.ENDPOINTS.MEV_CONFIG, {
      method: 'PUT',
      body: JSON.stringify(config),
    });
  }
}

// Export singleton instance
export const apiService = new ApiService();

// Export default for convenience
export default apiService;
