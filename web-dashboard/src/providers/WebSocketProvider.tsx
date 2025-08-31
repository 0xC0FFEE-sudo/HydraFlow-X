'use client';

import { createContext, useContext, ReactNode } from 'react';
import { useWebSocket } from '@/hooks/useWebSocket';

interface WebSocketContextType {
  socket: any;
  systemMetrics: any;
  tradingData: any;
  connectionStatus: 'connected' | 'disconnected' | 'connecting';
  sendMessage: (event: string, data: any) => void;
}

const WebSocketContext = createContext<WebSocketContextType | undefined>(undefined);

interface WebSocketProviderProps {
  children: ReactNode;
}

export function WebSocketProvider({ children }: WebSocketProviderProps) {
  const websocketData = useWebSocket();

  return (
    <WebSocketContext.Provider value={websocketData}>
      {children}
    </WebSocketContext.Provider>
  );
}

export function useWebSocketContext() {
  const context = useContext(WebSocketContext);
  if (context === undefined) {
    throw new Error('useWebSocketContext must be used within a WebSocketProvider');
  }
  return context;
}
