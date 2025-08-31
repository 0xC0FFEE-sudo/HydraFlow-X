'use client';

import { 
  HomeIcon, 
  ChartBarIcon, 
  WalletIcon, 
  CogIcon, 
  ShieldCheckIcon,
  BoltIcon
} from '@heroicons/react/24/outline';
import { clsx } from 'clsx';

interface SidebarProps {
  activeView: string;
  onViewChange: (view: string) => void;
}

const navigationItems = [
  { id: 'overview', name: 'Overview', icon: HomeIcon },
  { id: 'trading', name: 'Trading', icon: ChartBarIcon },
  { id: 'wallets', name: 'Wallets', icon: WalletIcon },
  { id: 'monitoring', name: 'Monitoring', icon: ShieldCheckIcon },
  { id: 'config', name: 'Configuration', icon: CogIcon },
];

export function Sidebar({ activeView, onViewChange }: SidebarProps) {
  return (
    <div className="flex flex-col w-64 bg-gray-800 border-r border-gray-700">
      {/* Logo */}
      <div className="flex items-center px-6 py-4 border-b border-gray-700">
        <BoltIcon className="w-8 h-8 text-blue-500 mr-3" />
        <div>
          <h1 className="text-xl font-bold text-gray-100">HydraFlow-X</h1>
          <p className="text-xs text-gray-400">Ultra-Low Latency Trading</p>
        </div>
      </div>

      {/* Navigation */}
      <nav className="flex-1 px-4 py-6 space-y-2">
        {navigationItems.map((item) => {
          const Icon = item.icon;
          const isActive = activeView === item.id;
          
          return (
            <button
              key={item.id}
              onClick={() => onViewChange(item.id)}
              className={clsx(
                'w-full flex items-center px-3 py-2 text-sm font-medium rounded-md transition-colors',
                isActive
                  ? 'bg-blue-600 text-white'
                  : 'text-gray-300 hover:bg-gray-700 hover:text-white'
              )}
            >
              <Icon className="w-5 h-5 mr-3" />
              {item.name}
            </button>
          );
        })}
      </nav>

      {/* Status Footer */}
      <div className="p-4 border-t border-gray-700">
        <div className="flex items-center text-sm text-gray-400">
          <div className="w-2 h-2 bg-green-500 rounded-full mr-2 animate-pulse" />
          System Online
        </div>
      </div>
    </div>
  );
}
