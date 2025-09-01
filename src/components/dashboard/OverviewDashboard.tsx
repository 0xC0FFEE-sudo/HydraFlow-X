'use client';

import { useState, useEffect } from 'react';
import { motion, AnimatePresence } from 'framer-motion';
import { Card, CardHeader, CardTitle, CardContent } from '@/components/ui/Card';
import { Badge, StatusBadge } from '@/components/ui/Badge';
import { Button } from '@/components/ui/Button';
import { formatters, colors, chartColors } from '@/lib/utils';
import {
  TrendingUp,
  TrendingDown,
  Activity,
  DollarSign,
  Zap,
  Shield,
  Clock,
  BarChart3,
  ArrowUpRight,
  ArrowDownRight,
  Play,
  Pause,
  RefreshCw,
  ChevronRight,
  Wifi,
  WifiOff,
  AlertTriangle,
  CheckCircle,
  XCircle,
} from 'lucide-react';

export function OverviewDashboard() {
  const [metrics, setMetrics] = useState({
    totalPnL: 25847.32,
    totalVolume: 2847592.50,
    winRate: 98.7,
    avgLatency: 15.2,
    activePositions: 7,
    totalTrades: 15847,
    systemUptime: 99.98,
    mevProtection: 97.3,
  });

  const [isLive, setIsLive] = useState(true);
  const [connectionStatus, setConnectionStatus] = useState<'connected' | 'connecting' | 'disconnected'>('connecting');
  const [lastUpdate, setLastUpdate] = useState(Date.now());
  const [pnlHistory, setPnlHistory] = useState<number[]>([25847.32]);
  const [latencyHistory, setLatencyHistory] = useState<number[]>([15.2]);

  // Real-time data simulation (will be replaced with actual backend data)
  useEffect(() => {
    if (!isLive) return;

    const interval = setInterval(() => {
      setMetrics(prev => {
        const newPnL = prev.totalPnL + (Math.random() - 0.4) * 100;
        const newLatency = Math.max(8, Math.min(30, prev.avgLatency + (Math.random() - 0.5) * 2));

        // Update history arrays
        setPnlHistory(history => [...history.slice(-19), newPnL]);
        setLatencyHistory(history => [...history.slice(-19), newLatency]);
        setLastUpdate(Date.now());

        return {
          ...prev,
          totalPnL: newPnL,
          avgLatency: newLatency,
          winRate: Math.max(85, Math.min(99.5, prev.winRate + (Math.random() - 0.5) * 0.5)),
        };
      });
    }, 2000);

    return () => clearInterval(interval);
  }, [isLive]);

  // Simulate connection status changes
  useEffect(() => {
    const connectionInterval = setInterval(() => {
      setConnectionStatus(Math.random() > 0.9 ? 'disconnected' : 'connected');
    }, 5000);

    return () => clearInterval(connectionInterval);
  }, []);

  const container = {
    hidden: { opacity: 0 },
    show: {
      opacity: 1,
      transition: {
        staggerChildren: 0.1,
        delayChildren: 0.1,
      },
    },
  };

  const item = {
    hidden: { y: 20, opacity: 0 },
    show: { y: 0, opacity: 1 },
  };

  return (
    <motion.div
      variants={container}
      initial="hidden"
      animate="show"
      className="space-y-8 h-full overflow-auto custom-scrollbar"
    >
      {/* Header Section */}
      <motion.div variants={item} className="relative">
        {/* Background Gradient */}
        <div className="absolute inset-0 bg-gradient-to-r from-primary-500/10 via-accent-500/5 to-secondary-500/10 rounded-3xl blur-3xl" />

        <div className="relative bg-gradient-to-r from-secondary-900/80 to-secondary-800/80 backdrop-blur-xl border border-white/10 rounded-3xl p-8">
          {/* Header Content */}
          <div className="flex flex-col lg:flex-row lg:items-center justify-between gap-6">
            <div className="space-y-3">
              <div className="flex items-center space-x-3">
                <div className="w-12 h-12 bg-gradient-to-r from-primary-500 to-accent-500 rounded-2xl flex items-center justify-center">
                  <BarChart3 className="w-6 h-6 text-white" />
                </div>
                <div>
                  <h1 className="text-4xl font-bold bg-gradient-to-r from-white via-primary-200 to-accent-200 bg-clip-text text-transparent">
                    HydraFlow-X Dashboard
                  </h1>
                  <p className="text-secondary-400 text-lg mt-1">
                    Ultra-Low Latency Trading Engine
                  </p>
                </div>
              </div>

              {/* Connection Status */}
              <div className="flex items-center space-x-4">
                <div className="flex items-center space-x-2">
                  {connectionStatus === 'connected' ? (
                    <CheckCircle className="w-4 h-4 text-success-400" />
                  ) : connectionStatus === 'connecting' ? (
                    <RefreshCw className="w-4 h-4 text-warning-400 animate-spin" />
                  ) : (
                    <XCircle className="w-4 h-4 text-danger-400" />
                  )}
                  <span className="text-sm font-medium capitalize text-white">
                    {connectionStatus}
                  </span>
                </div>
                <div className="text-xs text-secondary-500">
                  Last update: {formatters.timeAgo(lastUpdate)}
                </div>
              </div>
            </div>

            {/* Control Panel */}
            <div className="flex items-center space-x-3">
              <motion.div
                whileHover={{ scale: 1.02 }}
                whileTap={{ scale: 0.98 }}
              >
                <Button
                  variant={isLive ? 'success' : 'outline'}
                  size="lg"
                  onClick={() => setIsLive(!isLive)}
                  className="min-w-[120px] h-12"
                  leftIcon={isLive ? <Pause className="w-5 h-5" /> : <Play className="w-5 h-5" />}
                >
                  <span className="font-semibold">
                    {isLive ? 'Live Trading' : 'Start Trading'}
                  </span>
                </Button>
              </motion.div>

              <motion.div
                whileHover={{ scale: 1.05 }}
                whileTap={{ scale: 0.95 }}
              >
                <Button
                  variant="ghost"
                  size="lg"
                  className="h-12 px-4"
                >
                  <RefreshCw className="w-5 h-5" />
                </Button>
              </motion.div>
            </div>
          </div>
        </div>
      </motion.div>

      {/* Key Metrics Grid */}
      <motion.div
        variants={item}
        className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-4 gap-6"
      >
        <MetricCard
          title="Total P&L"
          value={formatters.currency(metrics.totalPnL)}
          change={+2.34}
          icon={<DollarSign className="w-6 h-6" />}
          variant={metrics.totalPnL > 0 ? 'success' : 'danger'}
          history={pnlHistory}
        />
        <MetricCard
          title="24h Volume"
          value={formatters.compact(metrics.totalVolume)}
          change={+15.7}
          icon={<BarChart3 className="w-6 h-6" />}
          variant="primary"
        />
        <MetricCard
          title="Win Rate"
          value={`${metrics.winRate.toFixed(1)}%`}
          change={+0.3}
          icon={<TrendingUp className="w-6 h-6" />}
          variant="success"
        />
        <MetricCard
          title="Avg Latency"
          value={formatters.latency(metrics.avgLatency * 1000)}
          change={-1.2}
          icon={<Zap className="w-6 h-6" />}
          variant={metrics.avgLatency < 20 ? 'success' : 'warning'}
          history={latencyHistory}
        />
      </motion.div>

      {/* Main Content Grid */}
      <motion.div
        variants={item}
        className="grid grid-cols-1 xl:grid-cols-3 gap-8 h-[calc(100vh-400px)]"
      >
        {/* Trading Overview - Left Column */}
        <div className="xl:col-span-2 space-y-8">
          {/* Real-time Performance Chart */}
          <Card variant="glass" className="h-96 overflow-hidden">
            <CardHeader className="pb-4">
              <div className="flex items-center justify-between">
                <CardTitle className="flex items-center space-x-3">
                  <div className="p-2 bg-gradient-to-r from-success-500/20 to-primary-500/20 rounded-xl">
                    <TrendingUp className="w-6 h-6 text-success-400" />
                  </div>
                  <div>
                    <h3 className="text-xl font-bold text-white">Live Performance</h3>
                    <p className="text-sm text-secondary-400">Real-time P&L tracking</p>
                  </div>
                </CardTitle>
                <div className="flex items-center space-x-2">
                  <div className="w-2 h-2 bg-success-400 rounded-full animate-pulse" />
                  <span className="text-sm text-success-400 font-medium">Live</span>
                </div>
              </div>
            </CardHeader>
            <CardContent className="h-72 pt-0">
              <LivePerformanceChart data={pnlHistory} />
            </CardContent>
          </Card>

          {/* Active Positions & Trading Activity */}
          <div className="grid grid-cols-1 lg:grid-cols-2 gap-6">
            {/* Active Positions */}
            <Card variant="glass" className="h-80">
              <CardHeader>
                <CardTitle className="flex items-center justify-between">
                  <div className="flex items-center space-x-3">
                    <div className="p-2 bg-gradient-to-r from-primary-500/20 to-accent-500/20 rounded-xl">
                      <Activity className="w-5 h-5 text-primary-400" />
                    </div>
                    <div>
                      <h3 className="text-lg font-bold text-white">Active Positions</h3>
                      <p className="text-sm text-secondary-400">Current trades</p>
                    </div>
                  </div>
                  <Badge variant="success" className="text-sm">
                    {metrics.activePositions} Active
                  </Badge>
                </CardTitle>
              </CardHeader>
              <CardContent className="h-52">
                <PositionsOverview />
              </CardContent>
            </Card>

            {/* Trading Activity */}
            <Card variant="glass" className="h-80">
              <CardHeader>
                <CardTitle className="flex items-center justify-between">
                  <div className="flex items-center space-x-3">
                    <div className="p-2 bg-gradient-to-r from-accent-500/20 to-warning-500/20 rounded-xl">
                      <BarChart3 className="w-5 h-5 text-accent-400" />
                    </div>
                    <div>
                      <h3 className="text-lg font-bold text-white">Trading Activity</h3>
                      <p className="text-sm text-secondary-400">Recent transactions</p>
                    </div>
                  </div>
                  <Badge variant="info" className="text-sm">
                    {metrics.totalTrades} Total
                  </Badge>
                </CardTitle>
              </CardHeader>
              <CardContent className="h-52">
                <TradingActivityChart />
              </CardContent>
            </Card>
          </div>
        </div>

        {/* Sidebar Widgets - Right Column */}
        <div className="space-y-6">
          {/* System Status */}
          <Card variant="glass">
            <CardHeader>
              <CardTitle className="flex items-center space-x-3">
                <div className="p-2 bg-gradient-to-r from-success-500/20 to-primary-500/20 rounded-xl">
                  <Shield className="w-5 h-5 text-success-400" />
                </div>
                <div>
                  <h3 className="text-lg font-bold text-white">System Health</h3>
                  <p className="text-sm text-secondary-400">Real-time status</p>
                </div>
              </CardTitle>
            </CardHeader>
            <CardContent className="space-y-4">
              <StatusItem
                label="Trading Engine"
                status="healthy"
                value="Online"
                icon={<CheckCircle className="w-4 h-4" />}
              />
              <StatusItem
                label="MEV Protection"
                status="healthy"
                value={`${metrics.mevProtection}%`}
                icon={<Shield className="w-4 h-4" />}
              />
              <StatusItem
                label="System Uptime"
                status="healthy"
                value={`${metrics.systemUptime}%`}
                icon={<Activity className="w-4 h-4" />}
              />
              <StatusItem
                label="Network Latency"
                status={metrics.avgLatency < 20 ? "healthy" : "warning"}
                value={formatters.latency(metrics.avgLatency * 1000)}
                icon={<Zap className="w-4 h-4" />}
              />
            </CardContent>
          </Card>

          {/* Quick Actions */}
          <Card variant="glass">
            <CardHeader>
              <CardTitle className="flex items-center space-x-3">
                <div className="p-2 bg-gradient-to-r from-primary-500/20 to-accent-500/20 rounded-xl">
                  <ChevronRight className="w-5 h-5 text-primary-400" />
                </div>
                <div>
                  <h3 className="text-lg font-bold text-white">Quick Actions</h3>
                  <p className="text-sm text-secondary-400">Fast operations</p>
                </div>
              </CardTitle>
            </CardHeader>
            <CardContent className="space-y-3">
              <motion.div whileHover={{ scale: 1.02 }} whileTap={{ scale: 0.98 }}>
                <Button variant="gradient" size="sm" className="w-full h-11">
                  Start New Strategy
                </Button>
              </motion.div>
              <motion.div whileHover={{ scale: 1.02 }} whileTap={{ scale: 0.98 }}>
                <Button variant="outline" size="sm" className="w-full h-11">
                  View All Positions
                </Button>
              </motion.div>
              <motion.div whileHover={{ scale: 1.02 }} whileTap={{ scale: 0.98 }}>
                <Button variant="ghost" size="sm" className="w-full h-11">
                  Download Report
                </Button>
              </motion.div>
            </CardContent>
          </Card>

          {/* Recent Alerts */}
          <Card variant="glass">
            <CardHeader>
              <CardTitle className="flex items-center space-x-3">
                <div className="p-2 bg-gradient-to-r from-warning-500/20 to-danger-500/20 rounded-xl">
                  <AlertTriangle className="w-5 h-5 text-warning-400" />
                </div>
                <div>
                  <h3 className="text-lg font-bold text-white">Recent Alerts</h3>
                  <p className="text-sm text-secondary-400">System notifications</p>
                </div>
              </CardTitle>
            </CardHeader>
            <CardContent className="space-y-3">
              <AlertItem
                type="success"
                message="High-profit opportunity detected"
                time="2m ago"
                icon={<TrendingUp className="w-4 h-4" />}
              />
              <AlertItem
                type="warning"
                message="Network latency increased"
                time="5m ago"
                icon={<AlertTriangle className="w-4 h-4" />}
              />
              <AlertItem
                type="info"
                message="New MEV protection activated"
                time="12m ago"
                icon={<Shield className="w-4 h-4" />}
              />
            </CardContent>
          </Card>
        </div>
      </motion.div>
    </motion.div>
  );
}

// Enhanced Metric Card Component with Mini Chart
function MetricCard({
  title,
  value,
  change,
  icon,
  variant = 'default',
  history
}: {
  title: string;
  value: string;
  change: number;
  icon: React.ReactNode;
  variant?: 'default' | 'primary' | 'success' | 'warning' | 'danger';
  history?: number[];
}) {
  const isPositive = change > 0;

  return (
    <motion.div
      whileHover={{ scale: 1.02 }}
      transition={{ type: "spring", stiffness: 300, damping: 20 }}
    >
      <Card variant="glass" className="metric-card overflow-hidden">
        <CardContent className="space-y-4 p-6">
          <div className="flex items-center justify-between">
            <div className={`p-3 rounded-xl bg-gradient-to-r from-${variant}-500/20 to-${variant}-500/10 border border-${variant}-500/20`}>
              {icon}
            </div>
            <div className="flex items-center space-x-2 text-sm">
              <motion.div
                animate={{ rotate: isPositive ? 0 : 180 }}
                transition={{ type: "spring", stiffness: 300 }}
              >
                {isPositive ? (
                  <ArrowUpRight className="w-4 h-4 text-success-400" />
                ) : (
                  <ArrowDownRight className="w-4 h-4 text-danger-400" />
                )}
              </motion.div>
              <span className={`font-semibold ${isPositive ? 'text-success-400' : 'text-danger-400'}`}>
                {Math.abs(change)}%
              </span>
            </div>
          </div>

          <div className="space-y-2">
            <p className="text-sm font-medium text-secondary-400 uppercase tracking-wide">
              {title}
            </p>
            <p className="text-2xl font-bold text-white">{value}</p>
          </div>

          {/* Mini Chart */}
          {history && history.length > 1 && (
            <div className="h-8 w-full">
              <MiniChart data={history} variant={variant} />
            </div>
          )}
        </CardContent>
      </Card>
    </motion.div>
  );
}

// Enhanced Status Item Component
function StatusItem({
  label,
  status,
  value,
  icon
}: {
  label: string;
  status: 'healthy' | 'warning' | 'error';
  value: string;
  icon?: React.ReactNode;
}) {
  const statusColors = {
    healthy: 'text-success-400 bg-success-500/20',
    warning: 'text-warning-400 bg-warning-500/20',
    error: 'text-danger-400 bg-danger-500/20',
  };

  return (
    <div className="flex items-center justify-between p-3 rounded-xl bg-secondary-800/50 border border-secondary-700/50">
      <div className="flex items-center space-x-3">
        <div className={`p-1.5 rounded-lg ${statusColors[status]}`}>
          {icon}
        </div>
        <span className="text-sm font-medium text-white">{label}</span>
      </div>
      <span className="text-sm font-bold text-white">{value}</span>
    </div>
  );
}

// Enhanced Alert Item Component
function AlertItem({
  type,
  message,
  time,
  icon
}: {
  type: 'success' | 'warning' | 'info' | 'error';
  message: string;
  time: string;
  icon?: React.ReactNode;
}) {
  const colors = {
    success: 'text-success-400 bg-success-500/10 border-success-500/20',
    warning: 'text-warning-400 bg-warning-500/10 border-warning-500/20',
    info: 'text-primary-400 bg-primary-500/10 border-primary-500/20',
    error: 'text-danger-400 bg-danger-500/10 border-danger-500/20',
  };

  return (
    <motion.div
      initial={{ opacity: 0, x: -20 }}
      animate={{ opacity: 1, x: 0 }}
      className="flex items-start space-x-3 p-3 rounded-xl bg-secondary-800/30 border border-secondary-700/50 hover:bg-secondary-800/50 transition-colors"
    >
      <div className={`p-1.5 rounded-lg ${colors[type]}`}>
        {icon}
      </div>
      <div className="flex-1 min-w-0">
        <p className="text-sm font-medium text-white">{message}</p>
        <p className="text-xs text-secondary-400 mt-1">{time}</p>
      </div>
    </motion.div>
  );
}

// Modern Chart Components with Real-time Data Visualization

// Mini Chart for Metric Cards
function MiniChart({ data, variant }: { data: number[]; variant: string }) {
  const min = Math.min(...data);
  const max = Math.max(...data);
  const range = max - min || 1;

  const pathData = data.map((value, index) => {
    const x = (index / (data.length - 1)) * 100;
    const y = 100 - ((value - min) / range) * 100;
    return `${index === 0 ? 'M' : 'L'} ${x} ${y}`;
  }).join(' ');

  const isPositive = data[data.length - 1] > data[0];
  const strokeColor = isPositive ? '#10b981' : '#ef4444';

  return (
    <div className="w-full h-full">
      <svg viewBox="0 0 100 100" className="w-full h-full">
        <defs>
          <linearGradient id={`gradient-${variant}`} x1="0%" y1="0%" x2="100%" y2="0%">
            <stop offset="0%" stopColor={strokeColor} stopOpacity="0.3" />
            <stop offset="100%" stopColor={strokeColor} stopOpacity="0.1" />
          </linearGradient>
        </defs>
        <path
          d={pathData}
          fill="none"
          stroke={strokeColor}
          strokeWidth="2"
          strokeLinecap="round"
          strokeLinejoin="round"
          className="drop-shadow-sm"
        />
        <path
          d={`${pathData} L 100 100 L 0 100 Z`}
          fill={`url(#gradient-${variant})`}
          opacity="0.3"
        />
      </svg>
    </div>
  );
}

// Live Performance Chart with Real-time Updates
function LivePerformanceChart({ data }: { data: number[] }) {
  const min = Math.min(...data);
  const max = Math.max(...data);
  const range = max - min || 1;

  const pathData = data.map((value, index) => {
    const x = (index / (data.length - 1)) * 100;
    const y = 100 - ((value - min) / range) * 100;
    return `${index === 0 ? 'M' : 'L'} ${x} ${y}`;
  }).join(' ');

  const currentValue = data[data.length - 1];
  const isPositive = currentValue > 0;

  return (
    <div className="h-full w-full">
      <div className="flex items-end justify-between h-full">
        {/* Y-axis labels */}
        <div className="flex flex-col justify-between h-full text-xs text-secondary-500 pr-2">
          <span>{formatters.currency(max)}</span>
          <span>{formatters.currency((max + min) / 2)}</span>
          <span>{formatters.currency(min)}</span>
        </div>

        {/* Chart area */}
        <div className="flex-1 h-full relative">
          <svg viewBox="0 0 100 100" className="w-full h-full">
            <defs>
              <linearGradient id="performance-gradient" x1="0%" y1="0%" x2="0%" y2="100%">
                <stop offset="0%" stopColor="#10b981" stopOpacity="0.4" />
                <stop offset="100%" stopColor="#10b981" stopOpacity="0.1" />
              </linearGradient>
            </defs>

            {/* Grid lines */}
            <line x1="0" y1="25" x2="100" y2="25" stroke="#374151" strokeWidth="0.5" opacity="0.3" />
            <line x1="0" y1="50" x2="100" y2="50" stroke="#374151" strokeWidth="0.5" opacity="0.3" />
            <line x1="0" y1="75" x2="100" y2="75" stroke="#374151" strokeWidth="0.5" opacity="0.3" />

            {/* Performance line */}
            <path
              d={pathData}
              fill="none"
              stroke={isPositive ? "#10b981" : "#ef4444"}
              strokeWidth="2"
              strokeLinecap="round"
              strokeLinejoin="round"
              className="drop-shadow-sm"
            />

            {/* Fill area */}
            <path
              d={`${pathData} L 100 100 L 0 100 Z`}
              fill="url(#performance-gradient)"
              opacity="0.6"
            />

            {/* Current value indicator */}
            <circle
              cx="100"
              cy={100 - ((currentValue - min) / range) * 100}
              r="3"
              fill={isPositive ? "#10b981" : "#ef4444"}
              className="animate-pulse"
            />
          </svg>

          {/* Current value display */}
          <div className="absolute top-2 right-2 bg-secondary-900/80 backdrop-blur-sm px-3 py-1 rounded-lg border border-white/10">
            <div className="text-sm font-bold text-white">
              {formatters.currency(currentValue)}
            </div>
            <div className={`text-xs ${isPositive ? 'text-success-400' : 'text-danger-400'}`}>
              {isPositive ? '+' : ''}{(currentValue / data[0] * 100 - 100).toFixed(2)}%
            </div>
          </div>
        </div>
      </div>
    </div>
  );
}

// Positions Overview Component
function PositionsOverview() {
  const positions = [
    { symbol: 'PEPE/USDC', pnl: 245.32, change: 12.5 },
    { symbol: 'SHIB/ETH', pnl: -89.45, change: -3.2 },
    { symbol: 'DOGE/USDT', pnl: 567.89, change: 8.7 },
  ];

  return (
    <div className="space-y-3 h-full overflow-hidden">
      {positions.map((pos, index) => (
        <motion.div
          key={pos.symbol}
          initial={{ opacity: 0, x: -20 }}
          animate={{ opacity: 1, x: 0 }}
          transition={{ delay: index * 0.1 }}
          className="flex items-center justify-between p-3 rounded-xl bg-secondary-800/30 border border-secondary-700/50 hover:bg-secondary-800/50 transition-colors"
        >
          <div>
            <p className="text-sm font-medium text-white">{pos.symbol}</p>
            <p className={`text-xs ${pos.pnl > 0 ? 'text-success-400' : 'text-danger-400'}`}>
              {pos.pnl > 0 ? '+' : ''}{pos.pnl.toFixed(2)} ({pos.change > 0 ? '+' : ''}{pos.change}%)
            </p>
          </div>
          <div className={`w-2 h-2 rounded-full ${pos.pnl > 0 ? 'bg-success-400' : 'bg-danger-400'}`} />
        </motion.div>
      ))}
    </div>
  );
}

// Trading Activity Chart Component
function TradingActivityChart() {
  const activities = [
    { time: '2m ago', action: 'BUY', symbol: 'PEPE', amount: '$1,250' },
    { time: '5m ago', action: 'SELL', symbol: 'SHIB', amount: '$890' },
    { time: '8m ago', action: 'BUY', symbol: 'DOGE', amount: '$2,100' },
    { time: '12m ago', action: 'SELL', symbol: 'PEPE', amount: '$675' },
  ];

  return (
    <div className="space-y-3 h-full overflow-hidden">
      {activities.map((activity, index) => (
        <motion.div
          key={index}
          initial={{ opacity: 0, y: 20 }}
          animate={{ opacity: 1, y: 0 }}
          transition={{ delay: index * 0.1 }}
          className="flex items-center justify-between p-3 rounded-xl bg-secondary-800/30 border border-secondary-700/50"
        >
          <div className="flex items-center space-x-3">
            <div className={`w-2 h-2 rounded-full ${activity.action === 'BUY' ? 'bg-success-400' : 'bg-danger-400'}`} />
            <div>
              <p className="text-sm font-medium text-white">
                {activity.action} {activity.symbol}
              </p>
              <p className="text-xs text-secondary-400">{activity.time}</p>
            </div>
          </div>
          <span className="text-sm font-bold text-white">{activity.amount}</span>
        </motion.div>
      ))}
    </div>
  );
}
