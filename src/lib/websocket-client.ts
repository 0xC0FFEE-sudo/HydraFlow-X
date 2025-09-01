// WebSocket client for real-time data streaming from C++ backend

import { API_CONFIG } from '@/config/api';
import type { SystemMetrics, PerformanceMetrics } from '@/types/system';
import type { Position, Trade } from '@/types/trading';
import type { MEVProtectionStatus } from '@/types/mev';

export type WebSocketEventMap = {
  'system:metrics': SystemMetrics;
  'market:prices': { symbol: string; price: number; timestamp: number }[];
  'trading:updates': { positions: Position[]; trades: Trade[] };
  'mev:alerts': { type: 'bundle_submitted' | 'protection_triggered'; data: any };
  'orderbook:updates': { symbol: string; bids: [number, number][]; asks: [number, number][] };
  'positions:updates': Position[];
  'performance:metrics': PerformanceMetrics;
};

export type WebSocketEvent = keyof WebSocketEventMap;

class WebSocketClient {
  private ws: WebSocket | null = null;
  private url: string;
  private reconnectInterval: number;
  private reconnectAttempts: number = 0;
  private maxReconnectAttempts: number = 10;
  private listeners: Map<WebSocketEvent, Set<Function>> = new Map();
  private isReconnecting: boolean = false;

  constructor() {
    this.url = API_CONFIG.WS_BASE_URL;
    this.reconnectInterval = API_CONFIG.WS_RECONNECT_INTERVAL;
  }

  connect(): Promise<void> {
    return new Promise((resolve, reject) => {
      try {
        this.ws = new WebSocket(this.url);

        this.ws.onopen = () => {
          console.log('WebSocket connected');
          this.reconnectAttempts = 0;
          this.isReconnecting = false;
          resolve();
        };

        this.ws.onmessage = (event) => {
          try {
            const data = JSON.parse(event.data);
            this.handleMessage(data);
          } catch (error) {
            console.error('Error parsing WebSocket message:', error);
          }
        };

        this.ws.onclose = (event) => {
          console.log('WebSocket disconnected:', event.code, event.reason);
          this.handleDisconnection();
        };

        this.ws.onerror = (error) => {
          console.error('WebSocket error:', error);
          reject(error);
        };

      } catch (error) {
        reject(error);
      }
    });
  }

  private handleMessage(data: { event: WebSocketEvent; payload: any }) {
    const { event, payload } = data;
    const eventListeners = this.listeners.get(event);
    
    if (eventListeners) {
      eventListeners.forEach(listener => {
        try {
          listener(payload);
        } catch (error) {
          console.error(`Error in WebSocket event listener for ${event}:`, error);
        }
      });
    }
  }

  private handleDisconnection() {
    if (!this.isReconnecting && this.reconnectAttempts < this.maxReconnectAttempts) {
      this.isReconnecting = true;
      console.log(`Attempting to reconnect... (${this.reconnectAttempts + 1}/${this.maxReconnectAttempts})`);
      
      setTimeout(() => {
        this.reconnectAttempts++;
        this.connect().catch(() => {
          console.error('Reconnection failed');
          this.isReconnecting = false;
        });
      }, this.reconnectInterval);
    }
  }

  subscribe<T extends WebSocketEvent>(
    event: T,
    callback: (data: WebSocketEventMap[T]) => void
  ): () => void {
    if (!this.listeners.has(event)) {
      this.listeners.set(event, new Set());
    }
    
    this.listeners.get(event)!.add(callback);

    // Send subscription message to backend
    if (this.ws && this.ws.readyState === WebSocket.OPEN) {
      this.ws.send(JSON.stringify({ action: 'subscribe', event }));
    }

    // Return unsubscribe function
    return () => {
      const eventListeners = this.listeners.get(event);
      if (eventListeners) {
        eventListeners.delete(callback);
        if (eventListeners.size === 0) {
          // Send unsubscribe message to backend
          if (this.ws && this.ws.readyState === WebSocket.OPEN) {
            this.ws.send(JSON.stringify({ action: 'unsubscribe', event }));
          }
        }
      }
    };
  }

  unsubscribe(event: WebSocketEvent) {
    this.listeners.delete(event);
    if (this.ws && this.ws.readyState === WebSocket.OPEN) {
      this.ws.send(JSON.stringify({ action: 'unsubscribe', event }));
    }
  }

  disconnect() {
    if (this.ws) {
      this.ws.close();
      this.ws = null;
    }
    this.listeners.clear();
    this.isReconnecting = false;
  }

  isConnected(): boolean {
    return this.ws !== null && this.ws.readyState === WebSocket.OPEN;
  }

  getConnectionStatus(): 'connected' | 'connecting' | 'disconnected' | 'reconnecting' {
    if (!this.ws) return 'disconnected';
    
    switch (this.ws.readyState) {
      case WebSocket.CONNECTING:
        return 'connecting';
      case WebSocket.OPEN:
        return 'connected';
      case WebSocket.CLOSING:
      case WebSocket.CLOSED:
        return this.isReconnecting ? 'reconnecting' : 'disconnected';
      default:
        return 'disconnected';
    }
  }
}

// Create and export singleton instance
export const wsClient = new WebSocketClient();

// Auto-connect with fallback to mock data
export const initializeWebSocket = async () => {
  try {
    await wsClient.connect();
    console.log('WebSocket client initialized');
  } catch (error) {
    console.warn('WebSocket connection failed, using mock data:', error);
    // You can implement mock data streaming here if needed
  }
};
