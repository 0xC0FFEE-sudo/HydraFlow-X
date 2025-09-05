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
  private authToken: string | null = null;

  constructor() {
    this.baseURL = API_CONFIG.REST_BASE_URL;
    this.timeout = API_CONFIG.REQUEST_TIMEOUT;

    // Initialize token from localStorage if available
    if (typeof window !== 'undefined') {
      const storedToken = localStorage.getItem('authToken');
      if (storedToken) {
        this.authToken = storedToken;
      }
    }
  }

  private async request<T>(
    endpoint: string,
    options: RequestInit = {},
    retryCount = 0
  ): Promise<T> {
    const url = `${this.baseURL}${endpoint}`;
    const headers: Record<string, string> = {
      'Content-Type': 'application/json',
      ...(options.headers as Record<string, string> || {}),
    };

    // Add JWT token if available
    if (this.authToken) {
      headers['Authorization'] = `Bearer ${this.authToken}`;
    }

    const config: RequestInit = {
      headers,
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
        const errorText = await response.text().catch(() => 'Unknown error');
        throw new Error(`API Error ${response.status}: ${response.statusText} - ${errorText}`);
      }

      const data = await response.json();

      // Log successful requests in development
      if (process.env.NODE_ENV === 'development') {
        console.log(`✅ API ${options.method || 'GET'} ${endpoint} -> ${response.status}`);
      }

      return data;
    } catch (error) {
      clearTimeout(timeoutId);

      if (error instanceof Error) {
        if (error.name === 'AbortError') {
          throw new Error(`Request timeout after ${this.timeout}ms for ${endpoint}`);
        }

        // Retry logic for network errors
        if (retryCount < 2 && (error.message.includes('Failed to fetch') || error.message.includes('NetworkError'))) {
          console.warn(`🔄 Retrying ${endpoint} (${retryCount + 1}/3)...`);
          await new Promise(resolve => setTimeout(resolve, 1000 * (retryCount + 1)));
          return this.request<T>(endpoint, options, retryCount + 1);
        }

        // Enhanced error logging
        console.error(`❌ API Error for ${endpoint}:`, {
          error: error.message,
          method: options.method || 'GET',
          url,
          retryCount,
          timestamp: new Date().toISOString()
        });
      }

      throw error;
    }
  }

  // System & Health APIs
  async getStatus(): Promise<any> {
    return this.request(API_CONFIG.ENDPOINTS.STATUS);
  }

  async getSystemInfo(): Promise<any> {
    return this.request(API_CONFIG.ENDPOINTS.SYSTEM_INFO);
  }

  async getPerformanceMetrics(): Promise<any> {
    return this.request(API_CONFIG.ENDPOINTS.PERFORMANCE);
  }

  async getHealthStatus(): Promise<any> {
    return this.request(API_CONFIG.ENDPOINTS.HEALTH);
  }

  async getMetrics(): Promise<any> {
    return this.request(API_CONFIG.ENDPOINTS.METRICS);
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

  async getPositions(): Promise<any> {
    return this.request(API_CONFIG.ENDPOINTS.POSITIONS);
  }

  async getBalances(): Promise<any> {
    return this.request(API_CONFIG.ENDPOINTS.BALANCES);
  }

  async getTradeHistory(): Promise<any> {
    return this.request(API_CONFIG.ENDPOINTS.TRADE_HISTORY);
  }

  async getMarketPrices(): Promise<any> {
    return this.request(API_CONFIG.ENDPOINTS.MARKET_PRICES);
  }

  // MEV Protection APIs
  async getMEVStatus(): Promise<any> {
    return this.request(API_CONFIG.ENDPOINTS.MEV_STATUS);
  }

  async getMEVProtectionStats(): Promise<any> {
    return this.request(API_CONFIG.ENDPOINTS.MEV_PROTECTION_STATS);
  }

  async getMEVRelays(): Promise<any> {
    return this.request(API_CONFIG.ENDPOINTS.MEV_RELAYS);
  }

  // Authentication APIs
  async login(credentials: { username: string; password: string }): Promise<any> {
    const response = await this.request<{ token?: string; [key: string]: any }>(API_CONFIG.ENDPOINTS.AUTH_LOGIN, {
      method: 'POST',
      body: JSON.stringify(credentials),
    });

    // Store JWT token if login successful
    if (response.token) {
      this.setAuthToken(response.token);
    }

    return response;
  }

  async logout(): Promise<any> {
    const response = await this.request(API_CONFIG.ENDPOINTS.AUTH_LOGOUT, {
      method: 'POST',
    });

    // Clear JWT token on logout
    this.clearAuthToken();

    return response;
  }

  async verifyAuth(): Promise<any> {
    return this.request(API_CONFIG.ENDPOINTS.AUTH_VERIFY);
  }

  async refreshToken(): Promise<any> {
    return this.request(API_CONFIG.ENDPOINTS.AUTH_REFRESH, {
      method: 'POST',
    });
  }

  async register(user: { username: string; email: string; password: string }): Promise<any> {
    return this.request(API_CONFIG.ENDPOINTS.AUTH_REGISTER, {
      method: 'POST',
      body: JSON.stringify(user),
    });
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

  // WebSocket Info
  async getWebSocketInfo(): Promise<any> {
    return this.request(API_CONFIG.ENDPOINTS.WEBSOCKET_INFO);
  }

  // JWT Token Management
  setAuthToken(token: string) {
    this.authToken = token;
    // Store in localStorage for persistence
    if (typeof window !== 'undefined') {
      localStorage.setItem('authToken', token);
    }
  }

  getAuthToken(): string | null {
    if (this.authToken) return this.authToken;
    // Try to get from localStorage
    if (typeof window !== 'undefined') {
      return localStorage.getItem('authToken');
    }
    return null;
  }

  clearAuthToken() {
    this.authToken = null;
    if (typeof window !== 'undefined') {
      localStorage.removeItem('authToken');
    }
  }

  isAuthenticated(): boolean {
    return this.getAuthToken() !== null;
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
