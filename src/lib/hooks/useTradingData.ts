'use client';

import { useState, useEffect, useCallback } from 'react';
import { apiClient } from '@/lib/api-client';

interface Position {
  symbol: string;
  quantity: number;
  average_price: number;
  current_price: number;
  unrealized_pnl: number;
  pnl_percentage: number;
  market_value: number;
}

interface Order {
  order_id: string;
  status: string;
  order_type: string;
  symbol: string;
  side: string;
  quantity: number;
  price: number;
  filled_quantity: number;
  timestamp: string;
}

interface Balance {
  asset: string;
  free: number;
  locked: number;
  total: number;
}

interface Trade {
  trade_id: string;
  order_id: string;
  symbol: string;
  side: string;
  quantity: number;
  price: number;
  total: number;
  fee: number;
  fee_asset: string;
  timestamp: string;
}

interface UseTradingDataOptions {
  enabled?: boolean;
  interval?: number; // in milliseconds
  onError?: (error: Error) => void;
}

// Hook for positions
export function usePositions(options: UseTradingDataOptions = {}) {
  const { enabled = true, interval = 10000, onError } = options;

  const [positions, setPositions] = useState<Position[]>([]);
  const [isLoading, setIsLoading] = useState(true);
  const [error, setError] = useState<Error | null>(null);
  const [lastUpdated, setLastUpdated] = useState<Date | null>(null);

  const fetchPositions = useCallback(async () => {
    try {
      const data = await apiClient.getPositions();
      setPositions(data.positions || []);
      setError(null);
      setLastUpdated(new Date());
      setIsLoading(false);
    } catch (err) {
      const error = err instanceof Error ? err : new Error('Failed to fetch positions');
      setError(error);
      setIsLoading(false);
      onError?.(error);
    }
  }, [onError]);

  useEffect(() => {
    if (!enabled) return;
    fetchPositions();
    const intervalId = setInterval(fetchPositions, interval);
    return () => clearInterval(intervalId);
  }, [enabled, interval, fetchPositions]);

  const refetch = useCallback(() => fetchPositions(), [fetchPositions]);

  return {
    positions,
    totalValue: positions.reduce((sum, pos) => sum + pos.market_value, 0),
    totalPnL: positions.reduce((sum, pos) => sum + pos.unrealized_pnl, 0),
    isLoading,
    error,
    lastUpdated,
    refetch,
  };
}

// Hook for orders
export function useOrders(options: UseTradingDataOptions = {}) {
  const { enabled = true, interval = 5000, onError } = options;

  const [orders, setOrders] = useState<Order[]>([]);
  const [isLoading, setIsLoading] = useState(true);
  const [error, setError] = useState<Error | null>(null);
  const [lastUpdated, setLastUpdated] = useState<Date | null>(null);

  const fetchOrders = useCallback(async () => {
    try {
      const data = await apiClient.getOrders();
      setOrders(data.orders || []);
      setError(null);
      setLastUpdated(new Date());
      setIsLoading(false);
    } catch (err) {
      const error = err instanceof Error ? err : new Error('Failed to fetch orders');
      setError(error);
      setIsLoading(false);
      onError?.(error);
    }
  }, [onError]);

  useEffect(() => {
    if (!enabled) return;
    fetchOrders();
    const intervalId = setInterval(fetchOrders, interval);
    return () => clearInterval(intervalId);
  }, [enabled, interval, fetchOrders]);

  const refetch = useCallback(() => fetchOrders(), [fetchOrders]);

  return {
    orders,
    activeOrders: orders.filter(order => order.status === 'pending' || order.status === 'partial'),
    filledOrders: orders.filter(order => order.status === 'filled'),
    cancelledOrders: orders.filter(order => order.status === 'cancelled'),
    isLoading,
    error,
    lastUpdated,
    refetch,
  };
}

// Hook for balances
export function useBalances(options: UseTradingDataOptions = {}) {
  const { enabled = true, interval = 15000, onError } = options;

  const [balances, setBalances] = useState<Balance[]>([]);
  const [isLoading, setIsLoading] = useState(true);
  const [error, setError] = useState<Error | null>(null);
  const [lastUpdated, setLastUpdated] = useState<Date | null>(null);

  const fetchBalances = useCallback(async () => {
    try {
      const data = await apiClient.getBalances();
      setBalances(data.balances || []);
      setError(null);
      setLastUpdated(new Date());
      setIsLoading(false);
    } catch (err) {
      const error = err instanceof Error ? err : new Error('Failed to fetch balances');
      setError(error);
      setIsLoading(false);
      onError?.(error);
    }
  }, [onError]);

  useEffect(() => {
    if (!enabled) return;
    fetchBalances();
    const intervalId = setInterval(fetchBalances, interval);
    return () => clearInterval(intervalId);
  }, [enabled, interval, fetchBalances]);

  const refetch = useCallback(() => fetchBalances(), [fetchBalances]);

  const getBalance = useCallback((asset: string) => {
    return balances.find(balance => balance.asset === asset);
  }, [balances]);

  const totalValue = balances.reduce((sum, balance) => sum + balance.total, 0);

  return {
    balances,
    totalValue,
    getBalance,
    isLoading,
    error,
    lastUpdated,
    refetch,
  };
}

// Hook for trade history
export function useTradeHistory(options: UseTradingDataOptions = {}) {
  const { enabled = true, interval = 30000, onError } = options;

  const [trades, setTrades] = useState<Trade[]>([]);
  const [isLoading, setIsLoading] = useState(true);
  const [error, setError] = useState<Error | null>(null);
  const [lastUpdated, setLastUpdated] = useState<Date | null>(null);

  const fetchTradeHistory = useCallback(async () => {
    try {
      const data = await apiClient.getTradeHistory();
      setTrades(data.trades || []);
      setError(null);
      setLastUpdated(new Date());
      setIsLoading(false);
    } catch (err) {
      const error = err instanceof Error ? err : new Error('Failed to fetch trade history');
      setError(error);
      setIsLoading(false);
      onError?.(error);
    }
  }, [onError]);

  useEffect(() => {
    if (!enabled) return;
    fetchTradeHistory();
    const intervalId = setInterval(fetchTradeHistory, interval);
    return () => clearInterval(intervalId);
  }, [enabled, interval, fetchTradeHistory]);

  const refetch = useCallback(() => fetchTradeHistory(), [fetchTradeHistory]);

  const recentTrades = trades.slice(0, 50); // Last 50 trades
  const totalVolume = trades.reduce((sum, trade) => sum + trade.total, 0);
  const totalFees = trades.reduce((sum, trade) => sum + trade.fee, 0);

  return {
    trades,
    recentTrades,
    totalVolume,
    totalFees,
    isLoading,
    error,
    lastUpdated,
    refetch,
  };
}

// Combined trading data hook
export function useTradingData(options: UseTradingDataOptions = {}) {
  const positions = usePositions(options);
  const orders = useOrders(options);
  const balances = useBalances(options);
  const tradeHistory = useTradeHistory(options);

  return {
    positions,
    orders,
    balances,
    tradeHistory,
    // Combined loading state
    isLoading: positions.isLoading || orders.isLoading || balances.isLoading || tradeHistory.isLoading,
    // Combined error state (first error found)
    error: positions.error || orders.error || balances.error || tradeHistory.error,
    // Latest update time
    lastUpdated: [
      positions.lastUpdated,
      orders.lastUpdated,
      balances.lastUpdated,
      tradeHistory.lastUpdated
    ].filter(Boolean).sort((a, b) => (b?.getTime() || 0) - (a?.getTime() || 0))[0] || null,
    // Refetch all data
    refetch: () => {
      positions.refetch();
      orders.refetch();
      balances.refetch();
      tradeHistory.refetch();
    },
  };
}
