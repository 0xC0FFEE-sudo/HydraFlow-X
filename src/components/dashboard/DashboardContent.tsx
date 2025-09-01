'use client';

import { OverviewDashboard } from './OverviewDashboard';
import { TradingDashboard } from './TradingDashboard';
import { AnalyticsDashboard } from './AnalyticsDashboard';
import { MEVDashboard } from './MEVDashboard';
import { WalletsDashboard } from './WalletsDashboard';
import { MonitoringDashboard } from './MonitoringDashboard';
import { SettingsDashboard } from './SettingsDashboard';

interface DashboardContentProps {
  activeView: string;
}

export function DashboardRouter({ activeView }: DashboardContentProps) {
  switch (activeView) {
    case 'overview':
      return <OverviewDashboard />;
    case 'trading':
      return <TradingDashboard />;
    case 'analytics':
      return <AnalyticsDashboard />;
    case 'mev-protection':
      return <MEVDashboard />;
    case 'wallets':
      return <WalletsDashboard />;
    case 'monitoring':
      return <MonitoringDashboard />;
    case 'settings':
      return <SettingsDashboard />;
    default:
      return <OverviewDashboard />;
  }
}
