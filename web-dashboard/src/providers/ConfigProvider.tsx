'use client';

import { createContext, useContext, ReactNode } from 'react';
import { useConfig } from '@/hooks/useConfig';

interface ConfigContextType {
  config: any;
  updateConfig: (updates: any) => Promise<void>;
  loading: boolean;
  error: string | null;
}

const ConfigContext = createContext<ConfigContextType | undefined>(undefined);

interface ConfigProviderProps {
  children: ReactNode;
}

export function ConfigProvider({ children }: ConfigProviderProps) {
  const configData = useConfig();

  return (
    <ConfigContext.Provider value={configData}>
      {children}
    </ConfigContext.Provider>
  );
}

export function useConfigContext() {
  const context = useContext(ConfigContext);
  if (context === undefined) {
    throw new Error('useConfigContext must be used within a ConfigProvider');
  }
  return context;
}
