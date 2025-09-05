'use client';

import { useState, useEffect, useCallback } from 'react';
import { apiClient } from '@/lib/api-client';

interface MEVStatus {
  protection_active: boolean;
  private_transactions_enabled: boolean;
  sandwich_protection_enabled: boolean;
  frontrun_protection_enabled: boolean;
  gas_optimization_enabled: boolean;
  jito_bundles_enabled: boolean;
  available_relays: string[];
  preferred_relays: string[];
}

interface MEVProtectionStats {
  transactions_protected: number;
  attacks_prevented: number;
  gas_saved: number;
  profit_loss_avoided: number;
  success_rate: number;
  last_updated: string;
}

interface RiskMetrics {
  portfolio_var: number;
  portfolio_cvar: number;
  sharpe_ratio: number;
  sortino_ratio: number;
  calmar_ratio: number;
  max_drawdown: number;
  volatility: number;
  concentration_risk: number;
  liquidity_risk: number;
  last_updated: string;
}

interface CircuitBreaker {
  type: string;
  threshold: number;
  current_value: number;
  is_triggered: boolean;
  last_triggered?: string;
}

interface SystemStatus {
  status: string;
  version: string;
  uptime: string;
  connections: {
    evm: string;
    solana: string;
    frontend: string;
  };
}

interface UseSystemDataOptions {
  enabled?: boolean;
  interval?: number; // in milliseconds
  onError?: (error: Error) => void;
}

// Hook for MEV status
export function useMEVStatus(options: UseSystemDataOptions = {}) {
  const { enabled = true, interval = 10000, onError } = options;

  const [mevStatus, setMevStatus] = useState<MEVStatus | null>(null);
  const [isLoading, setIsLoading] = useState(true);
  const [error, setError] = useState<Error | null>(null);
  const [lastUpdated, setLastUpdated] = useState<Date | null>(null);

  const fetchMEVStatus = useCallback(async () => {
    try {
      const data = await apiClient.getMEVStatus();
      setMevStatus(data);
      setError(null);
      setLastUpdated(new Date());
      setIsLoading(false);
    } catch (err) {
      const error = err instanceof Error ? err : new Error('Failed to fetch MEV status');
      setError(error);
      setIsLoading(false);
      onError?.(error);
    }
  }, [onError]);

  useEffect(() => {
    if (!enabled) return;
    fetchMEVStatus();
    const intervalId = setInterval(fetchMEVStatus, interval);
    return () => clearInterval(intervalId);
  }, [enabled, interval, fetchMEVStatus]);

  const refetch = useCallback(() => fetchMEVStatus(), [fetchMEVStatus]);

  return {
    mevStatus,
    isLoading,
    error,
    lastUpdated,
    refetch,
  };
}

// Hook for MEV protection stats
export function useMEVProtectionStats(options: UseSystemDataOptions = {}) {
  const { enabled = true, interval = 15000, onError } = options;

  const [stats, setStats] = useState<MEVProtectionStats | null>(null);
  const [isLoading, setIsLoading] = useState(true);
  const [error, setError] = useState<Error | null>(null);
  const [lastUpdated, setLastUpdated] = useState<Date | null>(null);

  const fetchStats = useCallback(async () => {
    try {
      const data = await apiClient.getMEVProtectionStats();
      setStats(data);
      setError(null);
      setLastUpdated(new Date());
      setIsLoading(false);
    } catch (err) {
      const error = err instanceof Error ? err : new Error('Failed to fetch MEV protection stats');
      setError(error);
      setIsLoading(false);
      onError?.(error);
    }
  }, [onError]);

  useEffect(() => {
    if (!enabled) return;
    fetchStats();
    const intervalId = setInterval(fetchStats, interval);
    return () => clearInterval(intervalId);
  }, [enabled, interval, fetchStats]);

  const refetch = useCallback(() => fetchStats(), [fetchStats]);

  return {
    stats,
    isLoading,
    error,
    lastUpdated,
    refetch,
  };
}

// Hook for risk metrics
export function useRiskMetrics(options: UseSystemDataOptions = {}) {
  const { enabled = true, interval = 20000, onError } = options;

  const [riskMetrics, setRiskMetrics] = useState<RiskMetrics | null>(null);
  const [isLoading, setIsLoading] = useState(true);
  const [error, setError] = useState<Error | null>(null);
  const [lastUpdated, setLastUpdated] = useState<Date | null>(null);

  const fetchRiskMetrics = useCallback(async () => {
    try {
      const data = await apiClient.getRiskMetrics();
      setRiskMetrics(data);
      setError(null);
      setLastUpdated(new Date());
      setIsLoading(false);
    } catch (err) {
      const error = err instanceof Error ? err : new Error('Failed to fetch risk metrics');
      setError(error);
      setIsLoading(false);
      onError?.(error);
    }
  }, [onError]);

  useEffect(() => {
    if (!enabled) return;
    fetchRiskMetrics();
    const intervalId = setInterval(fetchRiskMetrics, interval);
    return () => clearInterval(intervalId);
  }, [enabled, interval, fetchRiskMetrics]);

  const refetch = useCallback(() => fetchRiskMetrics(), [fetchRiskMetrics]);

  return {
    riskMetrics,
    isLoading,
    error,
    lastUpdated,
    refetch,
  };
}

// Hook for circuit breakers
export function useCircuitBreakers(options: UseSystemDataOptions = {}) {
  const { enabled = true, interval = 10000, onError } = options;

  const [circuitBreakers, setCircuitBreakers] = useState<CircuitBreaker[]>([]);
  const [isLoading, setIsLoading] = useState(true);
  const [error, setError] = useState<Error | null>(null);
  const [lastUpdated, setLastUpdated] = useState<Date | null>(null);

  const fetchCircuitBreakers = useCallback(async () => {
    try {
      const data = await apiClient.getCircuitBreakers();
      setCircuitBreakers(data.breakers || []);
      setError(null);
      setLastUpdated(new Date());
      setIsLoading(false);
    } catch (err) {
      const error = err instanceof Error ? err : new Error('Failed to fetch circuit breakers');
      setError(error);
      setIsLoading(false);
      onError?.(error);
    }
  }, [onError]);

  useEffect(() => {
    if (!enabled) return;
    fetchCircuitBreakers();
    const intervalId = setInterval(fetchCircuitBreakers, interval);
    return () => clearInterval(intervalId);
  }, [enabled, interval, fetchCircuitBreakers]);

  const refetch = useCallback(() => fetchCircuitBreakers(), [fetchCircuitBreakers]);

  const activeBreakers = circuitBreakers.filter(cb => cb.is_triggered);

  return {
    circuitBreakers,
    activeBreakers,
    isLoading,
    error,
    lastUpdated,
    refetch,
  };
}

// Hook for system status
export function useSystemStatus(options: UseSystemDataOptions = {}) {
  const { enabled = true, interval = 5000, onError } = options;

  const [systemStatus, setSystemStatus] = useState<SystemStatus | null>(null);
  const [isLoading, setIsLoading] = useState(true);
  const [error, setError] = useState<Error | null>(null);
  const [lastUpdated, setLastUpdated] = useState<Date | null>(null);

  const fetchSystemStatus = useCallback(async () => {
    try {
      const data = await apiClient.getStatus();
      setSystemStatus(data);
      setError(null);
      setLastUpdated(new Date());
      setIsLoading(false);
    } catch (err) {
      const error = err instanceof Error ? err : new Error('Failed to fetch system status');
      setError(error);
      setIsLoading(false);
      onError?.(error);
    }
  }, [onError]);

  useEffect(() => {
    if (!enabled) return;
    fetchSystemStatus();
    const intervalId = setInterval(fetchSystemStatus, interval);
    return () => clearInterval(intervalId);
  }, [enabled, interval, fetchSystemStatus]);

  const refetch = useCallback(() => fetchSystemStatus(), [fetchSystemStatus]);

  return {
    systemStatus,
    isOnline: systemStatus?.status === 'online',
    isLoading,
    error,
    lastUpdated,
    refetch,
  };
}

// Combined system monitoring hook
export function useSystemMonitoring(options: UseSystemDataOptions = {}) {
  const mevStatus = useMEVStatus(options);
  const mevStats = useMEVProtectionStats(options);
  const riskMetrics = useRiskMetrics(options);
  const circuitBreakers = useCircuitBreakers(options);
  const systemStatus = useSystemStatus(options);

  return {
    mevStatus,
    mevStats,
    riskMetrics,
    circuitBreakers,
    systemStatus,
    // Combined loading state
    isLoading: mevStatus.isLoading || mevStats.isLoading || riskMetrics.isLoading ||
               circuitBreakers.isLoading || systemStatus.isLoading,
    // Combined error state (first error found)
    error: mevStatus.error || mevStats.error || riskMetrics.error ||
           circuitBreakers.error || systemStatus.error,
    // Latest update time
    lastUpdated: [
      mevStatus.lastUpdated,
      mevStats.lastUpdated,
      riskMetrics.lastUpdated,
      circuitBreakers.lastUpdated,
      systemStatus.lastUpdated
    ].filter(Boolean).sort((a, b) => (b?.getTime() || 0) - (a?.getTime() || 0))[0] || null,
    // Refetch all data
    refetch: () => {
      mevStatus.refetch();
      mevStats.refetch();
      riskMetrics.refetch();
      circuitBreakers.refetch();
      systemStatus.refetch();
    },
  };
}
