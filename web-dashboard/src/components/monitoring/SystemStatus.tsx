'use client';

import { Card } from '@/components/ui/Card';
import { Badge } from '@/components/ui/Badge';
import { 
  CheckCircleIcon, 
  ExclamationTriangleIcon, 
  XCircleIcon,
  CpuChipIcon,
  ServerIcon,
  DatabaseIcon,
  WifiIcon
} from '@heroicons/react/24/outline';

interface SystemStatusProps {
  metrics?: {
    cpu: number;
    memory: number;
    activeConnections: number;
  };
  expanded?: boolean;
}

export function SystemStatus({ metrics, expanded = false }: SystemStatusProps) {
  const defaultMetrics = {
    cpu: 45,
    memory: 32,
    activeConnections: 847,
  };

  const data = metrics || defaultMetrics;

  const systemComponents = [
    {
      name: 'Trading Engine',
      status: 'healthy',
      uptime: '99.98%',
      lastCheck: '2 sec ago',
    },
    {
      name: 'MEV Shield',
      status: 'healthy',
      uptime: '99.95%',
      lastCheck: '1 sec ago',
    },
    {
      name: 'API Gateway',
      status: 'healthy',
      uptime: '99.99%',
      lastCheck: '3 sec ago',
    },
    {
      name: 'Database',
      status: 'warning',
      uptime: '99.85%',
      lastCheck: '5 sec ago',
    },
    {
      name: 'WebSocket Server',
      status: 'healthy',
      uptime: '99.97%',
      lastCheck: '1 sec ago',
    },
  ];

  const getStatusIcon = (status: string) => {
    switch (status) {
      case 'healthy':
        return <CheckCircleIcon className="w-5 h-5 text-green-500" />;
      case 'warning':
        return <ExclamationTriangleIcon className="w-5 h-5 text-yellow-500" />;
      case 'error':
        return <XCircleIcon className="w-5 h-5 text-red-500" />;
      default:
        return <CheckCircleIcon className="w-5 h-5 text-gray-500" />;
    }
  };

  const getStatusBadge = (status: string) => {
    switch (status) {
      case 'healthy':
        return <Badge variant="success">Healthy</Badge>;
      case 'warning':
        return <Badge variant="warning">Warning</Badge>;
      case 'error':
        return <Badge variant="destructive">Error</Badge>;
      default:
        return <Badge variant="secondary">Unknown</Badge>;
    }
  };

  return (
    <Card className="p-6">
      <div className="flex items-center justify-between mb-6">
        <h3 className="text-lg font-semibold text-gray-100">System Status</h3>
        <Badge variant="success">All Systems Operational</Badge>
      </div>

      {/* Resource Usage */}
      <div className="grid grid-cols-3 gap-4 mb-6">
        <div className="text-center">
          <CpuChipIcon className="w-8 h-8 text-blue-500 mx-auto mb-2" />
          <p className="text-sm text-gray-400">CPU Usage</p>
          <p className="text-xl font-bold text-gray-100">{data.cpu}%</p>
          <div className="w-full bg-gray-700 rounded-full h-2 mt-1">
            <div 
              className="bg-blue-500 h-2 rounded-full transition-all duration-300" 
              style={{ width: `${data.cpu}%` }}
            />
          </div>
        </div>

        <div className="text-center">
          <ServerIcon className="w-8 h-8 text-green-500 mx-auto mb-2" />
          <p className="text-sm text-gray-400">Memory</p>
          <p className="text-xl font-bold text-gray-100">{data.memory}%</p>
          <div className="w-full bg-gray-700 rounded-full h-2 mt-1">
            <div 
              className="bg-green-500 h-2 rounded-full transition-all duration-300" 
              style={{ width: `${data.memory}%` }}
            />
          </div>
        </div>

        <div className="text-center">
          <WifiIcon className="w-8 h-8 text-purple-500 mx-auto mb-2" />
          <p className="text-sm text-gray-400">Connections</p>
          <p className="text-xl font-bold text-gray-100">{data.activeConnections}</p>
          <p className="text-xs text-gray-500 mt-1">Active</p>
        </div>
      </div>

      {/* Component Status */}
      {expanded && (
        <div className="space-y-3">
          <h4 className="text-sm font-medium text-gray-300 mb-3">Component Health</h4>
          {systemComponents.map((component, index) => (
            <div key={index} className="flex items-center justify-between p-3 bg-gray-700/30 rounded-lg">
              <div className="flex items-center">
                {getStatusIcon(component.status)}
                <div className="ml-3">
                  <p className="text-sm font-medium text-gray-100">{component.name}</p>
                  <p className="text-xs text-gray-400">Uptime: {component.uptime}</p>
                </div>
              </div>
              <div className="text-right">
                {getStatusBadge(component.status)}
                <p className="text-xs text-gray-400 mt-1">{component.lastCheck}</p>
              </div>
            </div>
          ))}
        </div>
      )}

      {!expanded && (
        <div className="space-y-2">
          {systemComponents.slice(0, 3).map((component, index) => (
            <div key={index} className="flex items-center justify-between">
              <div className="flex items-center">
                {getStatusIcon(component.status)}
                <span className="ml-2 text-sm text-gray-300">{component.name}</span>
              </div>
              {getStatusBadge(component.status)}
            </div>
          ))}
          <p className="text-xs text-gray-400 text-center mt-3">
            +{systemComponents.length - 3} more components
          </p>
        </div>
      )}
    </Card>
  );
}
