// API service for communicating with HydraFlow C++ backend

import { API_CONFIG } from '@/config/api';
import type { Position, Trade, TradingStrategy } from '@/types/trading';
import type { SystemMetrics, PerformanceMetrics, SystemHealth } from '@/types/system';

class ApiService {
  private baseUrl: string;

  constructor() {
    this.baseUrl = API_CONFIG.REST_BASE_URL;
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

      const data = await response.json();

      if (!data.success) {
        throw new Error(data.error || 'API request failed');
      }

      return data.data;
    } catch (error) {
      console.error(`API request failed for ${endpoint}:`, error);
      throw error;
    }
  }

  // System APIs
  async getSystemStatus(): Promise<SystemHealth> {
    return this.request<SystemHealth>(API_CONFIG.ENDPOINTS.SYSTEM_METRICS);
  }

  async getPerformanceMetrics(): Promise<PerformanceMetrics> {
    return this.request<PerformanceMetrics>(API_CONFIG.ENDPOINTS.PERFORMANCE);
  }

  async getSystemMetrics(): Promise<SystemMetrics> {
    return this.request<SystemMetrics>(API_CONFIG.ENDPOINTS.SYSTEM_METRICS);
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
    order_type: 'market' | 'limit';
    slippage_percent: number;
    gas_price_gwei: number;
  }): Promise<{ order_id: string; status: string; estimated_execution_time_ms: number }> {
    return this.request(API_CONFIG.ENDPOINTS.PLACE_ORDER, {
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
