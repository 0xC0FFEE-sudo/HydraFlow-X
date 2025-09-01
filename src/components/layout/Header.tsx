'use client';

import { useState, useEffect } from 'react';
import { motion } from 'framer-motion';
import { cn, formatters } from '@/lib/utils';
import { Badge, StatusBadge } from '@/components/ui/Badge';
import { Button } from '@/components/ui/Button';
import { SearchInput } from '@/components/ui/Input';
import {
  Menu,
  Search,
  Bell,
  Settings,
  Maximize2,
  Wifi,
  WifiOff,
  Clock,
  Activity,
  TrendingUp,
  TrendingDown,
  Minus,
} from 'lucide-react';

interface HeaderProps {
  onMenuClick: () => void;
  sidebarOpen: boolean;
}

export function Header({ onMenuClick, sidebarOpen }: HeaderProps) {
  const [currentTime, setCurrentTime] = useState<string>('');
  const [connectionStatus, setConnectionStatus] = useState<'connected' | 'disconnected' | 'connecting'>('disconnected');
  const [systemMetrics, setSystemMetrics] = useState({
    cpu: 24.5,
    memory: 67.2,
    latency: 12.5,
    throughput: 1250,
  });
  const [notifications, setNotifications] = useState(3);

  // Update time every second
  useEffect(() => {
    const updateTime = () => {
      setCurrentTime(new Date().toLocaleTimeString('en-US', {
        hour12: false,
        hour: '2-digit',
        minute: '2-digit',
        second: '2-digit',
      }));
    };

    updateTime();
    const interval = setInterval(updateTime, 1000);
    return () => clearInterval(interval);
  }, []);

  // Simulate connection status and metrics updates
  useEffect(() => {
    setConnectionStatus('connected');
    
    const metricsInterval = setInterval(() => {
      setSystemMetrics(prev => ({
        cpu: Math.max(10, Math.min(90, prev.cpu + (Math.random() - 0.5) * 5)),
        memory: Math.max(20, Math.min(95, prev.memory + (Math.random() - 0.5) * 3)),
        latency: Math.max(8, Math.min(50, prev.latency + (Math.random() - 0.5) * 2)),
        throughput: Math.max(800, Math.min(2000, prev.throughput + (Math.random() - 0.5) * 100)),
      }));
    }, 3000);

    return () => clearInterval(metricsInterval);
  }, []);

  const getConnectionIcon = () => {
    switch (connectionStatus) {
      case 'connected':
        return <Wifi className="w-4 h-4 text-success-400" />;
      case 'connecting':
        return <Wifi className="w-4 h-4 text-warning-400 animate-pulse" />;
      default:
        return <WifiOff className="w-4 h-4 text-danger-400" />;
    }
  };

  const getStatusVariant = () => {
    switch (connectionStatus) {
      case 'connected': return 'healthy';
      case 'connecting': return 'warning';
      default: return 'error';
    }
  };

  return (
    <motion.header 
      className="flex items-center justify-between h-16 px-6 bg-secondary-900/80 backdrop-blur-xl border-b border-white/10"
      initial={{ y: -20, opacity: 0 }}
      animate={{ y: 0, opacity: 1 }}
      transition={{ duration: 0.3 }}
    >
      {/* Left section */}
      <div className="flex items-center space-x-4">
        {/* Menu toggle */}
        <Button
          variant="ghost"
          size="icon-sm"
          onClick={onMenuClick}
          className="text-secondary-400 hover:text-white"
        >
          <Menu className="w-5 h-5" />
        </Button>

        {/* Page title and breadcrumb */}
        <div className="hidden sm:flex items-center space-x-2">
          <h1 className="text-xl font-semibold text-white">Trading Dashboard</h1>
          <Badge variant="outline" size="sm">Live</Badge>
        </div>
      </div>

      {/* Center section - Quick metrics */}
      <div className="hidden lg:flex items-center space-x-6">
        {/* System metrics */}
        <div className="flex items-center space-x-4 text-sm">
          {/* Latency */}
          <div className="flex items-center space-x-2">
            <Activity className="w-4 h-4 text-primary-400" />
            <span className="text-secondary-400">Latency:</span>
            <span className={cn(
              'font-medium',
              systemMetrics.latency < 20 ? 'text-success-400' :
              systemMetrics.latency < 35 ? 'text-warning-400' : 'text-danger-400'
            )}>
              {formatters.latency(systemMetrics.latency * 1000)}
            </span>
          </div>

          {/* Throughput */}
          <div className="flex items-center space-x-2">
            <TrendingUp className="w-4 h-4 text-success-400" />
            <span className="text-secondary-400">TPS:</span>
            <span className="font-medium text-white">
              {formatters.compact(systemMetrics.throughput)}
            </span>
          </div>

          {/* CPU Usage */}
          <div className="flex items-center space-x-2">
            <div className="w-12 bg-secondary-700 rounded-full h-1.5 overflow-hidden">
              <motion.div
                className={cn(
                  'h-full rounded-full transition-colors duration-300',
                  systemMetrics.cpu < 70 ? 'bg-success-500' :
                  systemMetrics.cpu < 85 ? 'bg-warning-500' : 'bg-danger-500'
                )}
                initial={{ width: 0 }}
                animate={{ width: `${systemMetrics.cpu}%` }}
                transition={{ duration: 0.5 }}
              />
            </div>
            <span className="text-secondary-400 text-xs">
              {systemMetrics.cpu.toFixed(1)}%
            </span>
          </div>
        </div>
      </div>

      {/* Right section */}
      <div className="flex items-center space-x-3">
        {/* Search */}
        <div className="hidden md:block w-64">
          <SearchInput
            placeholder="Search positions, trades..."
            variant="glass"
          />
        </div>

        {/* Time */}
        <div className="hidden sm:flex items-center space-x-2 text-sm">
          <Clock className="w-4 h-4 text-secondary-400" />
          <span className="font-mono text-white">
            {currentTime || '--:--:--'}
          </span>
        </div>

        {/* Connection Status */}
        <div className="flex items-center space-x-2">
          {getConnectionIcon()}
          <StatusBadge status={getStatusVariant()} />
        </div>

        {/* Notifications */}
        <Button variant="ghost" size="icon-sm" className="relative">
          <Bell className="w-5 h-5 text-secondary-400" />
          {notifications > 0 && (
            <motion.div
              className="absolute -top-1 -right-1 w-5 h-5 bg-danger-500 rounded-full flex items-center justify-center text-xs font-bold text-white"
              initial={{ scale: 0 }}
              animate={{ scale: 1 }}
              transition={{ type: "spring", stiffness: 300, damping: 20 }}
            >
              {notifications > 9 ? '9+' : notifications}
            </motion.div>
          )}
        </Button>

        {/* Search (mobile) */}
        <Button variant="ghost" size="icon-sm" className="md:hidden">
          <Search className="w-5 h-5 text-secondary-400" />
        </Button>

        {/* Settings */}
        <Button variant="ghost" size="icon-sm">
          <Settings className="w-5 h-5 text-secondary-400" />
        </Button>

        {/* Fullscreen */}
        <Button variant="ghost" size="icon-sm" className="hidden lg:flex">
          <Maximize2 className="w-5 h-5 text-secondary-400" />
        </Button>
      </div>
    </motion.header>
  );
}
