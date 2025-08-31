import { useState, useEffect } from 'react';

interface APIConfig {
  apis: Record<string, {
    provider: string;
    api_key: string;
    secret_key?: string;
    enabled: boolean;
    status: 'connected' | 'disconnected' | 'testing';
  }>;
  rpcs: Record<string, {
    chain: string;
    endpoint: string;
    api_key: string;
    enabled: boolean;
    status: 'connected' | 'disconnected' | 'testing';
  }>;
}

interface UseConfigReturn {
  config: APIConfig;
  updateConfig: (updates: Partial<APIConfig>) => Promise<void>;
  loading: boolean;
  error: string | null;
}

export function useConfig(): UseConfigReturn {
  const [config, setConfig] = useState<APIConfig>({
    apis: {
      twitter: {
        provider: 'twitter',
        api_key: '',
        secret_key: '',
        enabled: false,
        status: 'disconnected',
      },
      reddit: {
        provider: 'reddit',
        api_key: '',
        secret_key: '',
        enabled: false,
        status: 'disconnected',
      },
      dexscreener: {
        provider: 'dexscreener',
        api_key: '',
        enabled: false,
        status: 'disconnected',
      },
      gmgn: {
        provider: 'gmgn',
        api_key: '',
        enabled: false,
        status: 'disconnected',
      },
    },
    rpcs: {
      ethereum: {
        chain: 'ethereum',
        endpoint: '',
        api_key: '',
        enabled: false,
        status: 'disconnected',
      },
      solana: {
        chain: 'solana',
        endpoint: '',
        api_key: '',
        enabled: false,
        status: 'disconnected',
      },
      base: {
        chain: 'base',
        endpoint: 'https://mainnet.base.org',
        api_key: '',
        enabled: false,
        status: 'disconnected',
      },
      arbitrum: {
        chain: 'arbitrum',
        endpoint: 'https://arb1.arbitrum.io/rpc',
        api_key: '',
        enabled: false,
        status: 'disconnected',
      },
    },
  });
  
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState<string | null>(null);

  useEffect(() => {
    loadConfig();
  }, []);

  const loadConfig = async () => {
    try {
      setLoading(true);
      const response = await fetch('/api/config');
      
      if (response.ok) {
        const data = await response.json();
        setConfig(data);
      } else {
        // Use default config if API fails
        console.warn('Failed to load config from API, using defaults');
      }
    } catch (err) {
      console.error('Error loading config:', err);
      setError('Failed to load configuration');
    } finally {
      setLoading(false);
    }
  };

  const updateConfig = async (updates: Partial<APIConfig>) => {
    try {
      setError(null);
      const newConfig = { ...config, ...updates };
      
      const response = await fetch('/api/config', {
        method: 'PUT',
        headers: {
          'Content-Type': 'application/json',
        },
        body: JSON.stringify(newConfig),
      });

      if (response.ok) {
        setConfig(newConfig);
      } else {
        throw new Error('Failed to update configuration');
      }
    } catch (err) {
      console.error('Error updating config:', err);
      setError('Failed to update configuration');
      throw err;
    }
  };

  return {
    config,
    updateConfig,
    loading,
    error,
  };
}
