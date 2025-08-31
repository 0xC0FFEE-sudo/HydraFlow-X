'use client';

import { 
  WifiIcon, 
  SignalIcon, 
  ClockIcon,
  ExclamationTriangleIcon
} from '@heroicons/react/24/outline';
import { Badge } from '@/components/ui/Badge';
import { clsx } from 'clsx';

interface HeaderProps {
  connectionStatus: 'connected' | 'disconnected' | 'connecting';
}

export function Header({ connectionStatus }: HeaderProps) {
  const currentTime = new Date().toLocaleTimeString();

  const getConnectionStatusInfo = () => {
    switch (connectionStatus) {
      case 'connected':
        return {
          icon: WifiIcon,
          text: 'Connected',
          variant: 'success' as const,
          className: 'text-green-500'
        };
      case 'connecting':
        return {
          icon: SignalIcon,
          text: 'Connecting',
          variant: 'warning' as const,
          className: 'text-yellow-500 animate-pulse'
        };
      case 'disconnected':
      default:
        return {
          icon: ExclamationTriangleIcon,
          text: 'Disconnected',
          variant: 'destructive' as const,
          className: 'text-red-500'
        };
    }
  };

  const statusInfo = getConnectionStatusInfo();
  const StatusIcon = statusInfo.icon;

  return (
    <header className="flex items-center justify-between px-6 py-4 bg-gray-800 border-b border-gray-700">
      <div className="flex items-center space-x-6">
        <h2 className="text-lg font-semibold text-gray-100">
          Trading Dashboard
        </h2>
        
        {/* Real-time metrics */}
        <div className="flex items-center space-x-4 text-sm text-gray-400">
          <div className="flex items-center">
            <ClockIcon className="w-4 h-4 mr-1" />
            <span>{currentTime}</span>
          </div>
          
          <div className="flex items-center">
            <div className="w-2 h-2 bg-green-500 rounded-full mr-2 animate-pulse" />
            <span>Latency: ~15ms</span>
          </div>
        </div>
      </div>

      <div className="flex items-center space-x-4">
        {/* Connection Status */}
        <div className="flex items-center space-x-2">
          <StatusIcon className={clsx('w-5 h-5', statusInfo.className)} />
          <Badge variant={statusInfo.variant}>
            {statusInfo.text}
          </Badge>
        </div>

        {/* System Performance Indicator */}
        <div className="flex items-center space-x-2 text-sm">
          <span className="text-gray-400">CPU:</span>
          <div className="w-16 bg-gray-700 rounded-full h-2">
            <div className="bg-blue-500 h-2 rounded-full" style={{ width: '45%' }} />
          </div>
          <span className="text-gray-300">45%</span>
        </div>

        <div className="flex items-center space-x-2 text-sm">
          <span className="text-gray-400">Memory:</span>
          <div className="w-16 bg-gray-700 rounded-full h-2">
            <div className="bg-green-500 h-2 rounded-full" style={{ width: '32%' }} />
          </div>
          <span className="text-gray-300">32%</span>
        </div>
      </div>
    </header>
  );
}
