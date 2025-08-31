import { useEffect, useState, useCallback } from 'react';
import { io, Socket } from 'socket.io-client';

interface SystemMetrics {
  performance: {
    avgLatency: number;
    totalTrades: number;
    successRate: number;
    uptime: number;
  };
  latencyHistory: Array<{
    timestamp: number;
    latency: number;
  }>;
  cpu: number;
  memory: number;
  activeConnections: number;
}

interface TradingData {
  positions: Array<{
    id: string;
    symbol: string;
    amount: number;
    value: number;
    pnl: number;
    status: 'active' | 'closed';
  }>;
  recentTrades: Array<{
    id: string;
    symbol: string;
    type: 'buy' | 'sell';
    amount: number;
    price: number;
    timestamp: number;
    status: 'completed' | 'pending' | 'failed';
  }>;
}

interface UseWebSocketReturn {
  socket: Socket | null;
  systemMetrics: SystemMetrics | null;
  tradingData: TradingData | null;
  connectionStatus: 'connected' | 'disconnected' | 'connecting';
  sendMessage: (event: string, data: any) => void;
}

export function useWebSocket(): UseWebSocketReturn {
  const [socket, setSocket] = useState<Socket | null>(null);
  const [systemMetrics, setSystemMetrics] = useState<SystemMetrics | null>(null);
  const [tradingData, setTradingData] = useState<TradingData | null>(null);
  const [connectionStatus, setConnectionStatus] = useState<'connected' | 'disconnected' | 'connecting'>('disconnected');

  useEffect(() => {
    const socketInstance = io(process.env.NEXT_PUBLIC_WS_URL || 'ws://localhost:8080', {
      transports: ['websocket'],
      autoConnect: true,
    });

    setSocket(socketInstance);
    setConnectionStatus('connecting');

    // Connection events
    socketInstance.on('connect', () => {
      console.log('WebSocket connected');
      setConnectionStatus('connected');
    });

    socketInstance.on('disconnect', () => {
      console.log('WebSocket disconnected');
      setConnectionStatus('disconnected');
    });

    socketInstance.on('connect_error', (error) => {
      console.error('WebSocket connection error:', error);
      setConnectionStatus('disconnected');
    });

    // Data events
    socketInstance.on('system_metrics', (data: SystemMetrics) => {
      setSystemMetrics(data);
    });

    socketInstance.on('trading_data', (data: TradingData) => {
      setTradingData(data);
    });

    socketInstance.on('trade_update', (trade: any) => {
      setTradingData(prev => {
        if (!prev) return prev;
        
        return {
          ...prev,
          recentTrades: [trade, ...prev.recentTrades.slice(0, 49)], // Keep last 50 trades
        };
      });
    });

    socketInstance.on('metrics_update', (metrics: Partial<SystemMetrics>) => {
      setSystemMetrics(prev => {
        if (!prev) return prev;
        
        return {
          ...prev,
          ...metrics,
        };
      });
    });

    return () => {
      socketInstance.disconnect();
    };
  }, []);

  const sendMessage = useCallback((event: string, data: any) => {
    if (socket && connectionStatus === 'connected') {
      socket.emit(event, data);
    }
  }, [socket, connectionStatus]);

  return {
    socket,
    systemMetrics,
    tradingData,
    connectionStatus,
    sendMessage,
  };
}
