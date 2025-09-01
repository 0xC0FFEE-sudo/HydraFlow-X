'use client';

import React, { createContext, useContext, useEffect, useState, useCallback, ReactNode } from 'react';
import { API_CONFIG } from '@/config/api';
import type { SystemMetrics, PerformanceMetrics } from '@/types/system';
import type { Position, Trade } from '@/types/trading';

interface WebSocketContextType {
  isConnected: boolean;
  connectionStatus: 'connected' | 'connecting' | 'disconnected' | 'reconnecting';
  systemMetrics: SystemMetrics | null;
  performanceMetrics: PerformanceMetrics | null;
  positions: Position[];
  trades: Trade[];
  subscribe: (event: string, callback: (data: any) => void) => () => void;
  unsubscribe: (event: string) => void;
  send: (event: string, data: any) => void;
}

const WebSocketContext = createContext<WebSocketContextType | undefined>(undefined);

interface WebSocketProviderProps {
  children: ReactNode;
}

export function WebSocketProvider({ children }: WebSocketProviderProps) {
  const [ws, setWs] = useState<WebSocket | null>(null);
  const [isConnected, setIsConnected] = useState(false);
  const [connectionStatus, setConnectionStatus] = useState<'connected' | 'connecting' | 'disconnected' | 'reconnecting'>('connecting');
  const [reconnectAttempts, setReconnectAttempts] = useState(0);
  const [systemMetrics, setSystemMetrics] = useState<SystemMetrics | null>(null);
  const [performanceMetrics, setPerformanceMetrics] = useState<PerformanceMetrics | null>(null);
  const [positions, setPositions] = useState<Position[]>([]);
  const [trades, setTrades] = useState<Trade[]>([]);
  const [listeners, setListeners] = useState<Map<string, Set<(data: any) => void>>>(new Map());

  const maxReconnectAttempts = 10;
  const reconnectInterval = 5000;

  // WebSocket connection management
  const connect = useCallback(() => {
    if (ws?.readyState === WebSocket.CONNECTING || ws?.readyState === WebSocket.OPEN) {
      return;
    }

    setConnectionStatus('connecting');

    try {
      const websocket = new WebSocket(API_CONFIG.WS_BASE_URL);

      websocket.onopen = () => {
        console.log('WebSocket connected to HydraFlow backend');
        setIsConnected(true);
        setConnectionStatus('connected');
        setReconnectAttempts(0);
        setWs(websocket);

        // Subscribe to default channels
        websocket.send(JSON.stringify({
          action: 'subscribe',
          channels: [
            API_CONFIG.WS_CHANNELS.SYSTEM_METRICS,
            API_CONFIG.WS_CHANNELS.PRICE_FEEDS,
            API_CONFIG.WS_CHANNELS.TRADE_UPDATES,
            API_CONFIG.WS_CHANNELS.PERFORMANCE
          ]
        }));
      };

      websocket.onmessage = (event) => {
        try {
          const data = JSON.parse(event.data);

          if (data.event && data.payload) {
            // Handle different event types
            switch (data.event) {
              case 'system:metrics':
                setSystemMetrics(data.payload);
                break;
              case 'performance:metrics':
                setPerformanceMetrics(data.payload);
                break;
              case 'trading:updates':
                if (data.payload.positions) setPositions(data.payload.positions);
                if (data.payload.trades) setTrades(data.payload.trades);
                break;
              default:
                // Notify listeners
                const eventListeners = listeners.get(data.event);
                if (eventListeners) {
                  eventListeners.forEach(callback => {
                    try {
                      callback(data.payload);
                    } catch (error) {
                      console.error(`Error in WebSocket listener for ${data.event}:`, error);
                    }
                  });
                }
            }
          }
        } catch (error) {
          console.error('Error parsing WebSocket message:', error);
        }
      };

      websocket.onclose = (event) => {
        console.log('WebSocket disconnected:', event.code, event.reason);
        setIsConnected(false);
        setWs(null);

        // Attempt reconnection
        if (reconnectAttempts < maxReconnectAttempts) {
          setConnectionStatus('reconnecting');
          setTimeout(() => {
            setReconnectAttempts(prev => prev + 1);
            connect();
          }, reconnectInterval);
        } else {
          setConnectionStatus('disconnected');
        }
      };

      websocket.onerror = (error) => {
        console.error('WebSocket error:', error);
        setConnectionStatus('disconnected');
      };

    } catch (error) {
      console.error('Failed to create WebSocket connection:', error);
      setConnectionStatus('disconnected');
    }
  }, [ws, reconnectAttempts, listeners]);

  // Connect on mount
  useEffect(() => {
    connect();

    return () => {
      if (ws) {
        ws.close();
      }
    };
  }, []);

  // Subscription management
  const subscribe = useCallback((event: string, callback: (data: any) => void) => {
    setListeners(prev => {
      const newListeners = new Map(prev);
      if (!newListeners.has(event)) {
        newListeners.set(event, new Set());
      }
      newListeners.get(event)!.add(callback);
      return newListeners;
    });

    // Send subscription to server
    if (ws && ws.readyState === WebSocket.OPEN) {
      ws.send(JSON.stringify({ action: 'subscribe', event }));
    }

    // Return unsubscribe function
    return () => {
      setListeners(prev => {
        const newListeners = new Map(prev);
        const eventListeners = newListeners.get(event);
        if (eventListeners) {
          eventListeners.delete(callback);
          if (eventListeners.size === 0) {
            newListeners.delete(event);
            // Send unsubscribe to server
            if (ws && ws.readyState === WebSocket.OPEN) {
              ws.send(JSON.stringify({ action: 'unsubscribe', event }));
            }
          }
        }
        return newListeners;
      });
    };
  }, [ws]);

  const unsubscribe = useCallback((event: string) => {
    setListeners(prev => {
      const newListeners = new Map(prev);
      newListeners.delete(event);
      return newListeners;
    });

    if (ws && ws.readyState === WebSocket.OPEN) {
      ws.send(JSON.stringify({ action: 'unsubscribe', event }));
    }
  }, [ws]);

  const send = useCallback((event: string, data: any) => {
    if (ws && ws.readyState === WebSocket.OPEN) {
      ws.send(JSON.stringify({ event, payload: data }));
    } else {
      console.warn('WebSocket not connected, message not sent:', event, data);
    }
  }, [ws]);

  const contextValue: WebSocketContextType = {
    isConnected,
    connectionStatus,
    systemMetrics,
    performanceMetrics,
    positions,
    trades,
    subscribe,
    unsubscribe,
    send,
  };

  return (
    <WebSocketContext.Provider value={contextValue}>
      {children}
    </WebSocketContext.Provider>
  );
}

export function useWebSocket() {
  const context = useContext(WebSocketContext);
  if (context === undefined) {
    throw new Error('useWebSocket must be used within a WebSocketProvider');
  }
  return context;
}
