'use client';

import { useState, useEffect, useCallback } from 'react';
import { apiClient } from '@/lib/api-client';

interface MarketData {
  ethereum?: {
    gas_price: string;
    block_number: string;
    pending_txs: string;
  };
  solana?: {
    slot: string;
    tps: string;
    jito_tips: string;
  };
  tokens: Array<{
    symbol: string;
    price: string;
    change_24h: string;
  }>;
}

interface UseMarketDataOptions {
  enabled?: boolean;
  interval?: number; // in milliseconds
  onError?: (error: Error) => void;
}

export function useMarketData(options: UseMarketDataOptions = {}) {
  const { enabled = true, interval = 5000, onError } = options;

  const [data, setData] = useState<MarketData | null>(null);
  const [isLoading, setIsLoading] = useState(true);
  const [error, setError] = useState<Error | null>(null);
  const [lastUpdated, setLastUpdated] = useState<Date | null>(null);

  const fetchMarketData = useCallback(async () => {
    try {
      const marketData = await apiClient.getMarketPrices();
      setData(marketData);
      setError(null);
      setLastUpdated(new Date());
      setIsLoading(false);
    } catch (err) {
      const error = err instanceof Error ? err : new Error('Failed to fetch market data');
      setError(error);
      setIsLoading(false);
      onError?.(error);
    }
  }, [onError]);

  useEffect(() => {
    if (!enabled) return;

    // Initial fetch
    fetchMarketData();

    // Set up interval for periodic updates
    const intervalId = setInterval(fetchMarketData, interval);

    return () => clearInterval(intervalId);
  }, [enabled, interval, fetchMarketData]);

  const refetch = useCallback(() => {
    fetchMarketData();
  }, [fetchMarketData]);

  return {
    data,
    isLoading,
    error,
    lastUpdated,
    refetch,
  };
}

// Hook for specific token prices
export function useTokenPrice(symbol: string, options: UseMarketDataOptions = {}) {
  const { data, ...rest } = useMarketData(options);

  const tokenData = data?.tokens?.find(token => token.symbol === symbol);

  return {
    price: tokenData?.price || null,
    change24h: tokenData?.change_24h || null,
    ...rest,
  };
}

// Hook for gas prices
export function useGasPrice(options: UseMarketDataOptions = {}) {
  const { data, ...rest } = useMarketData(options);

  return {
    gasPrice: data?.ethereum?.gas_price || null,
    blockNumber: data?.ethereum?.block_number || null,
    pendingTxs: data?.ethereum?.pending_txs || null,
    ...rest,
  };
}

// Hook for Solana data
export function useSolanaData(options: UseMarketDataOptions = {}) {
  const { data, ...rest } = useMarketData(options);

  return {
    slot: data?.solana?.slot || null,
    tps: data?.solana?.tps || null,
    jitoTips: data?.solana?.jito_tips || null,
    ...rest,
  };
}
