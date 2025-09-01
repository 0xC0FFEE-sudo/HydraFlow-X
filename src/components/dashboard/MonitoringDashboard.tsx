'use client';

import { motion } from 'framer-motion';
import { Card, CardHeader, CardTitle, CardContent } from '@/components/ui/Card';
import { StatusBadge } from '@/components/ui/Badge';
import { formatters } from '@/lib/utils';
import { Activity, Cpu, HardDrive, Wifi, AlertTriangle } from 'lucide-react';

export function MonitoringDashboard() {
  const systemHealth = [
    { name: 'Trading Engine', status: 'healthy', uptime: 99.98, latency: 12.5 },
    { name: 'MEV Protection', status: 'healthy', uptime: 99.95, latency: 8.2 },
    { name: 'Database', status: 'warning', uptime: 99.85, latency: 25.1 },
    { name: 'WebSocket Server', status: 'healthy', uptime: 99.97, latency: 15.3 },
  ];

  return (
    <motion.div
      initial={{ opacity: 0, y: 20 }}
      animate={{ opacity: 1, y: 0 }}
      className="space-y-6"
    >
      <div>
        <h1 className="text-3xl font-bold gradient-text">System Monitoring</h1>
        <p className="text-secondary-400 mt-1">Real-time system health and performance</p>
      </div>

      <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-4 gap-6">
        <Card variant="glass">
          <CardContent className="space-y-3">
            <div className="flex items-center justify-between">
              <Cpu className="w-8 h-8 text-primary-400" />
            </div>
            <div>
              <p className="metric-label">CPU Usage</p>
              <p className="metric-value">24.5%</p>
            </div>
          </CardContent>
        </Card>

        <Card variant="glass">
          <CardContent className="space-y-3">
            <div className="flex items-center justify-between">
              <HardDrive className="w-8 h-8 text-success-400" />
            </div>
            <div>
              <p className="metric-label">Memory</p>
              <p className="metric-value">67.2%</p>
            </div>
          </CardContent>
        </Card>

        <Card variant="glass">
          <CardContent className="space-y-3">
            <div className="flex items-center justify-between">
              <Wifi className="w-8 h-8 text-warning-400" />
            </div>
            <div>
              <p className="metric-label">Network</p>
              <p className="metric-value">12.5ms</p>
            </div>
          </CardContent>
        </Card>

        <Card variant="glass">
          <CardContent className="space-y-3">
            <div className="flex items-center justify-between">
              <Activity className="w-8 h-8 text-success-400" />
            </div>
            <div>
              <p className="metric-label">Uptime</p>
              <p className="metric-value">99.98%</p>
            </div>
          </CardContent>
        </Card>
      </div>

      <Card variant="glass">
        <CardHeader>
          <CardTitle className="flex items-center space-x-2">
            <AlertTriangle className="w-5 h-5 text-warning-400" />
            <span>Service Health</span>
          </CardTitle>
        </CardHeader>
        <CardContent>
          <div className="space-y-4">
            {systemHealth.map((service) => (
              <div key={service.name} className="flex items-center justify-between p-3 rounded-lg bg-white/5">
                <div className="flex items-center space-x-3">
                  <StatusBadge 
                    status={service.status === 'healthy' ? 'healthy' : 'warning'} 
                  />
                  <span className="font-medium text-white">{service.name}</span>
                </div>
                <div className="text-right space-y-1">
                  <div className="text-sm text-secondary-400">
                    Uptime: {service.uptime}%
                  </div>
                  <div className="text-sm text-secondary-400">
                    Latency: {formatters.latency(service.latency * 1000)}
                  </div>
                </div>
              </div>
            ))}
          </div>
        </CardContent>
      </Card>
    </motion.div>
  );
}
