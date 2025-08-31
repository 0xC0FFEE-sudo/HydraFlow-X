'use client';

import { useState, useEffect } from 'react';
import { Header } from '@/components/layout/Header';
import { Sidebar } from '@/components/layout/Sidebar';
import { TradingControls } from '@/components/trading/TradingControls';
import { PerformanceMetrics } from '@/components/metrics/PerformanceMetrics';
import { LatencyChart } from '@/components/charts/LatencyChart';
import { PositionsTable } from '@/components/trading/PositionsTable';
import { APIConfigPanel } from '@/components/config/APIConfigPanel';
import { SystemStatus } from '@/components/monitoring/SystemStatus';
import { RecentTrades } from '@/components/trading/RecentTrades';
import { MEVProtectionStatus } from '@/components/monitoring/MEVProtectionStatus';
import { WalletManager } from '@/components/wallet/WalletManager';
import { useWebSocket } from '@/hooks/useWebSocket';
import { useConfig } from '@/hooks/useConfig';

export default function Dashboard() {
  const [activeView, setActiveView] = useState('overview');
  const { systemMetrics, tradingData, connectionStatus } = useWebSocket();
  const { config, updateConfig } = useConfig();

  return (
    <div className="flex h-screen bg-gray-900">
      <Sidebar activeView={activeView} onViewChange={setActiveView} />
      
      <div className="flex-1 flex flex-col overflow-hidden">
        <Header connectionStatus={connectionStatus} />
        
        <main className="flex-1 overflow-auto p-6">
          {activeView === 'overview' && (
            <div className="space-y-6">
              {/* System Status Row */}
              <div className="grid grid-cols-1 lg:grid-cols-3 gap-6">
                <SystemStatus metrics={systemMetrics} />
                <MEVProtectionStatus />
                <TradingControls />
              </div>

              {/* Performance Metrics Row */}
              <div className="grid grid-cols-1 lg:grid-cols-4 gap-6">
                <PerformanceMetrics metrics={systemMetrics?.performance} />
              </div>

              {/* Charts Row */}
              <div className="grid grid-cols-1 lg:grid-cols-2 gap-6">
                <LatencyChart data={systemMetrics?.latencyHistory} />
                <div className="bg-gray-800 rounded-lg p-6">
                  <h3 className="text-lg font-semibold text-gray-100 mb-4">
                    Trading Volume (24h)
                  </h3>
                  <div className="h-64 flex items-center justify-center text-gray-400">
                    Volume Chart Component
                  </div>
                </div>
              </div>

              {/* Positions and Trades */}
              <div className="grid grid-cols-1 lg:grid-cols-2 gap-6">
                <PositionsTable positions={tradingData?.positions} />
                <RecentTrades trades={tradingData?.recentTrades} />
              </div>
            </div>
          )}

          {activeView === 'trading' && (
            <div className="space-y-6">
              <h2 className="text-2xl font-bold text-gray-100">Trading Dashboard</h2>
              <div className="grid grid-cols-1 lg:grid-cols-2 gap-6">
                <TradingControls expanded={true} />
                <PositionsTable positions={tradingData?.positions} />
              </div>
              <RecentTrades trades={tradingData?.recentTrades} />
            </div>
          )}

          {activeView === 'wallets' && (
            <div className="space-y-6">
              <h2 className="text-2xl font-bold text-gray-100">Wallet Management</h2>
              <WalletManager />
            </div>
          )}

          {activeView === 'config' && (
            <div className="space-y-6">
              <h2 className="text-2xl font-bold text-gray-100">API Configuration</h2>
              <APIConfigPanel config={config} onConfigUpdate={updateConfig} />
            </div>
          )}

          {activeView === 'monitoring' && (
            <div className="space-y-6">
              <h2 className="text-2xl font-bold text-gray-100">System Monitoring</h2>
              <div className="grid grid-cols-1 lg:grid-cols-2 gap-6">
                <SystemStatus metrics={systemMetrics} expanded={true} />
                <MEVProtectionStatus expanded={true} />
              </div>
              <LatencyChart data={systemMetrics?.latencyHistory} />
            </div>
          )}
        </main>
      </div>
    </div>
  );
}