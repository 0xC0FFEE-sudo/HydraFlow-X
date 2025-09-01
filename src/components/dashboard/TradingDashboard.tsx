'use client';

import { useState, useEffect } from 'react';
import { motion } from 'framer-motion';
import { Card, CardHeader, CardTitle, CardContent } from '@/components/ui/Card';
import { Badge, StatusBadge } from '@/components/ui/Badge';
import { Button } from '@/components/ui/Button';
import { formatters, colors } from '@/lib/utils';
import { useWebSocket } from '@/providers/WebSocketProvider';
import { apiService } from '@/lib/api-service';
import {
  Play,
  Pause,
  TrendingUp,
  TrendingDown,
  DollarSign,
  Target,
  Zap,
  Clock,
  BarChart3,
  ArrowUpRight,
  ArrowDownRight,
  Plus,
  Minus,
  RefreshCw,
  Settings,
  Activity,
  AlertTriangle,
} from 'lucide-react';
import type { Position, Trade } from '@/types/trading';

export function TradingDashboard() {
  const { isConnected, connectionStatus, positions, trades, systemMetrics } = useWebSocket();
  const [isTrading, setIsTrading] = useState(false);
  const [currentStrategy, setCurrentStrategy] = useState('STANDARD_BUY');
  const [tradingStats, setTradingStats] = useState({
    totalPnL: 25847.32,
    winRate: 98.7,
    activePositions: 7,
    totalTrades: 15847,
    avgLatency: 15.2,
  });

  // Fetch trading data on mount
  useEffect(() => {
    const fetchTradingData = async () => {
      try {
        const status = await apiService.getTradingStatus();
        setIsTrading(status.active);
        setCurrentStrategy(status.strategy || 'STANDARD_BUY');
        setTradingStats({
          totalPnL: status.available_balance || 25847.32,
          winRate: status.success_rate || 98.7,
          activePositions: status.current_positions || 7,
          totalTrades: status.total_trades || 15847,
          avgLatency: status.avg_latency_ms || 15.2,
        });
      } catch (error) {
        console.warn('Failed to fetch trading status:', error);
      }
    };

    fetchTradingData();
  }, []);

  const handleStartTrading = async () => {
    try {
      await apiService.startTrading({
        mode: currentStrategy,
        settings: {
          maxPositionSize: 1000,
          maxSlippage: 2.0,
          enableMEVProtection: true,
          enableSniperMode: false,
        },
      });
      setIsTrading(true);
    } catch (error) {
      console.error('Failed to start trading:', error);
    }
  };

  const handleStopTrading = async () => {
    try {
      await apiService.stopTrading();
      setIsTrading(false);
    } catch (error) {
      console.error('Failed to stop trading:', error);
    }
  };

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
      {/* Header */}
      <motion.div variants={item} className="flex items-center justify-between">
        <div>
          <h1 className="text-3xl font-bold gradient-text">Trading Dashboard</h1>
          <p className="text-secondary-400 mt-1">
            Real-time trading execution and position management
          </p>
        </div>
        <div className="flex items-center space-x-4">
          {/* Connection Status */}
          <div className="flex items-center space-x-2">
            <div className={`w-2 h-2 rounded-full ${
              connectionStatus === 'connected' ? 'bg-success-400' :
              connectionStatus === 'connecting' ? 'bg-warning-400 animate-pulse' :
              'bg-danger-400'
            }`} />
            <span className="text-sm text-secondary-300 capitalize">
              {connectionStatus}
            </span>
          </div>

          {/* Trading Controls */}
          <Button
            variant={isTrading ? 'success' : 'outline'}
            size="lg"
            onClick={isTrading ? handleStopTrading : handleStartTrading}
            leftIcon={isTrading ? <Pause className="w-5 h-5" /> : <Play className="w-5 h-5" />}
            className="min-w-[140px]"
          >
            {isTrading ? 'Stop Trading' : 'Start Trading'}
          </Button>
        </div>
      </motion.div>

      {/* Trading Stats */}
      <motion.div
        variants={item}
        className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-4 gap-6"
      >
        <TradingMetricCard
          title="Total P&L"
          value={formatters.currency(tradingStats.totalPnL)}
          change={+2.34}
          icon={<DollarSign className="w-6 h-6" />}
          variant={tradingStats.totalPnL > 0 ? 'success' : 'danger'}
        />
        <TradingMetricCard
          title="Win Rate"
          value={`${tradingStats.winRate.toFixed(1)}%`}
          change={+0.3}
          icon={<Target className="w-6 h-6" />}
          variant="success"
        />
        <TradingMetricCard
          title="Active Positions"
          value={tradingStats.activePositions.toString()}
          change={0}
          icon={<Activity className="w-6 h-6" />}
          variant="primary"
        />
        <TradingMetricCard
          title="Avg Latency"
          value={formatters.latency(tradingStats.avgLatency * 1000)}
          change={-1.2}
          icon={<Zap className="w-6 h-6" />}
          variant={tradingStats.avgLatency < 20 ? 'success' : 'warning'}
        />
      </motion.div>

      {/* Main Trading Interface */}
      <motion.div variants={item} className="grid grid-cols-1 xl:grid-cols-3 gap-8 h-[calc(100vh-400px)]">
        {/* Left Column - Positions */}
        <div className="xl:col-span-2 space-y-6">
          {/* Active Positions */}
          <Card variant="glass" className="h-96">
            <CardHeader>
              <div className="flex items-center justify-between">
                <CardTitle className="flex items-center space-x-3">
                  <div className="p-2 bg-gradient-to-r from-primary-500/20 to-accent-500/20 rounded-xl">
                    <BarChart3 className="w-6 h-6 text-primary-400" />
                  </div>
                  <div>
                    <h3 className="text-xl font-bold text-white">Active Positions</h3>
                    <p className="text-sm text-secondary-400">Current open trades</p>
                  </div>
                </CardTitle>
                <Badge variant="success" className="text-sm">
                  {tradingStats.activePositions} Active
                </Badge>
              </div>
            </CardHeader>
            <CardContent className="h-72 overflow-auto">
              <PositionsList positions={positions.slice(0, 5)} />
            </CardContent>
          </Card>

          {/* Recent Trades */}
          <Card variant="glass" className="h-96">
            <CardHeader>
              <div className="flex items-center justify-between">
                <CardTitle className="flex items-center space-x-3">
                  <div className="p-2 bg-gradient-to-r from-accent-500/20 to-warning-500/20 rounded-xl">
                    <Clock className="w-6 h-6 text-accent-400" />
                  </div>
                  <div>
                    <h3 className="text-xl font-bold text-white">Recent Trades</h3>
                    <p className="text-sm text-secondary-400">Latest transaction history</p>
                  </div>
                </CardTitle>
                <Badge variant="info" className="text-sm">
                  {tradingStats.totalTrades} Total
                </Badge>
              </div>
            </CardHeader>
            <CardContent className="h-72 overflow-auto">
              <TradesList trades={trades.slice(0, 8)} />
            </CardContent>
          </Card>
        </div>

        {/* Right Column - Controls & Info */}
        <div className="space-y-6">
          {/* Quick Trading Actions */}
          <Card variant="glass">
            <CardHeader>
              <CardTitle className="flex items-center space-x-3">
                <div className="p-2 bg-gradient-to-r from-success-500/20 to-primary-500/20 rounded-xl">
                  <Plus className="w-5 h-5 text-success-400" />
                </div>
                <div>
                  <h3 className="text-lg font-bold text-white">Quick Actions</h3>
                  <p className="text-sm text-secondary-400">Fast trading operations</p>
                </div>
              </CardTitle>
            </CardHeader>
            <CardContent className="space-y-3">
              <motion.div whileHover={{ scale: 1.02 }} whileTap={{ scale: 0.98 }}>
                <Button variant="gradient" size="sm" className="w-full h-11">
                  <Plus className="w-4 h-4 mr-2" />
                  New Position
                </Button>
              </motion.div>
              <motion.div whileHover={{ scale: 1.02 }} whileTap={{ scale: 0.98 }}>
                <Button variant="outline" size="sm" className="w-full h-11">
                  <Minus className="w-4 h-4 mr-2" />
                  Close All
                </Button>
              </motion.div>
              <motion.div whileHover={{ scale: 1.02 }} whileTap={{ scale: 0.98 }}>
                <Button variant="ghost" size="sm" className="w-full h-11">
                  <Settings className="w-4 h-4 mr-2" />
                  Strategy Settings
                </Button>
              </motion.div>
            </CardContent>
          </Card>

          {/* Strategy Info */}
          <Card variant="glass">
            <CardHeader>
              <CardTitle className="flex items-center space-x-3">
                <div className="p-2 bg-gradient-to-r from-warning-500/20 to-accent-500/20 rounded-xl">
                  <Target className="w-5 h-5 text-warning-400" />
                </div>
                <div>
                  <h3 className="text-lg font-bold text-white">Active Strategy</h3>
                  <p className="text-sm text-secondary-400">Current trading mode</p>
                </div>
              </CardTitle>
            </CardHeader>
            <CardContent className="space-y-4">
              <div className="text-center p-4 bg-secondary-800/50 rounded-xl border border-secondary-700/50">
                <div className="text-xl font-bold text-white mb-1">
                  {currentStrategy.replace('_', ' ')}
                </div>
                <div className="text-sm text-secondary-400">
                  {isTrading ? 'Active' : 'Inactive'}
                </div>
              </div>

              {isTrading && (
                <div className="space-y-2">
                  <div className="flex justify-between text-sm">
                    <span className="text-secondary-400">Status</span>
                    <span className="text-success-400 font-medium">Running</span>
                  </div>
                  <div className="flex justify-between text-sm">
                    <span className="text-secondary-400">Uptime</span>
                    <span className="text-white font-medium">2h 34m</span>
                  </div>
                  <div className="flex justify-between text-sm">
                    <span className="text-secondary-400">Trades Today</span>
                    <span className="text-white font-medium">247</span>
                  </div>
                </div>
              )}
            </CardContent>
          </Card>

          {/* Alerts */}
          <Card variant="glass">
            <CardHeader>
              <CardTitle className="flex items-center space-x-3">
                <div className="p-2 bg-gradient-to-r from-danger-500/20 to-warning-500/20 rounded-xl">
                  <AlertTriangle className="w-5 h-5 text-danger-400" />
                </div>
                <div>
                  <h3 className="text-lg font-bold text-white">Alerts</h3>
                  <p className="text-sm text-secondary-400">System notifications</p>
                </div>
              </CardTitle>
            </CardHeader>
            <CardContent className="space-y-3">
              <div className="flex items-start space-x-3 p-3 rounded-xl bg-warning-500/10 border border-warning-500/20">
                <AlertTriangle className="w-4 h-4 text-warning-400 mt-0.5" />
                <div className="flex-1">
                  <p className="text-sm font-medium text-white">High slippage detected</p>
                  <p className="text-xs text-secondary-400">2 minutes ago</p>
                </div>
              </div>

              <div className="flex items-start space-x-3 p-3 rounded-xl bg-success-500/10 border border-success-500/20">
                <TrendingUp className="w-4 h-4 text-success-400 mt-0.5" />
                <div className="flex-1">
                  <p className="text-sm font-medium text-white">New profitable opportunity</p>
                  <p className="text-xs text-secondary-400">5 minutes ago</p>
                </div>
              </div>
            </CardContent>
          </Card>
        </div>
      </motion.div>
    </motion.div>
  );
}

// Trading Metric Card Component
function TradingMetricCard({
  title,
  value,
  change,
  icon,
  variant = 'default'
}: {
  title: string;
  value: string;
  change: number;
  icon: React.ReactNode;
  variant?: 'default' | 'primary' | 'success' | 'warning' | 'danger';
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
        </CardContent>
      </Card>
    </motion.div>
  );
}

// Positions List Component
function PositionsList({ positions }: { positions: Position[] }) {
  // Mock positions if empty
  const mockPositions = positions.length > 0 ? positions : [
    {
      id: 'pos_1',
      symbol: 'PEPE/USDC',
      side: 'long' as const,
      size: 1000000,
      entryPrice: 0.000012,
      currentPrice: 0.000014,
      pnl: 245.32,
      pnlPercent: 16.7,
      unrealizedPnl: 245.32,
      realizedPnl: 0,
      openTime: Date.now() - 3600000,
      status: 'open' as const,
      leverage: 1,
      marginUsed: 120,
    },
    {
      id: 'pos_2',
      symbol: 'SHIB/ETH',
      side: 'short' as const,
      size: 50000,
      entryPrice: 0.00000002,
      currentPrice: 0.000000019,
      pnl: -12.5,
      pnlPercent: -2.5,
      unrealizedPnl: -12.5,
      realizedPnl: 0,
      openTime: Date.now() - 1800000,
      status: 'open' as const,
      leverage: 2,
      marginUsed: 100,
    },
  ];

  return (
    <div className="space-y-3">
      {mockPositions.map((pos, index) => (
        <motion.div
          key={pos.id}
          initial={{ opacity: 0, x: -20 }}
          animate={{ opacity: 1, x: 0 }}
          transition={{ delay: index * 0.1 }}
          className="flex items-center justify-between p-4 rounded-xl bg-secondary-800/30 border border-secondary-700/50 hover:bg-secondary-800/50 transition-colors"
        >
          <div className="flex items-center space-x-3">
            <div className={`w-3 h-3 rounded-full ${pos.side === 'long' ? 'bg-success-400' : 'bg-danger-400'}`} />
            <div>
              <p className="text-sm font-medium text-white">{pos.symbol}</p>
              <p className="text-xs text-secondary-400">
                {formatters.currency(pos.size)} @ {formatters.currency(pos.entryPrice)}
              </p>
            </div>
          </div>
          <div className="text-right">
            <p className={`text-sm font-medium ${pos.pnl > 0 ? 'text-success-400' : 'text-danger-400'}`}>
              {pos.pnl > 0 ? '+' : ''}{formatters.currency(pos.pnl)}
            </p>
            <p className={`text-xs ${pos.pnlPercent > 0 ? 'text-success-400' : 'text-danger-400'}`}>
              {pos.pnlPercent > 0 ? '+' : ''}{pos.pnlPercent.toFixed(1)}%
            </p>
          </div>
        </motion.div>
      ))}
    </div>
  );
}

// Trades List Component
function TradesList({ trades }: { trades: Trade[] }) {
  // Mock trades if empty
  const mockTrades = trades.length > 0 ? trades : [
    {
      id: 'trade_1',
      symbol: 'PEPE/USDC',
      side: 'buy' as const,
      type: 'market' as const,
      quantity: 100000,
      price: 0.000012,
      executedPrice: 0.00001205,
      executedQuantity: 100000,
      fee: 0.12,
      timestamp: Date.now() - 120000,
      status: 'completed' as const,
      latency: 15.2,
      orderId: 'ord_123',
      source: 'manual' as const,
    },
    {
      id: 'trade_2',
      symbol: 'SHIB/ETH',
      side: 'sell' as const,
      type: 'limit' as const,
      quantity: 50000,
      price: 0.00000002,
      executedPrice: 0.0000000198,
      executedQuantity: 50000,
      fee: 0.025,
      timestamp: Date.now() - 300000,
      status: 'completed' as const,
      latency: 12.8,
      orderId: 'ord_124',
      source: 'bot' as const,
    },
  ];

  return (
    <div className="space-y-3">
      {mockTrades.map((trade, index) => (
        <motion.div
          key={trade.id}
          initial={{ opacity: 0, y: 20 }}
          animate={{ opacity: 1, y: 0 }}
          transition={{ delay: index * 0.05 }}
          className="flex items-center justify-between p-3 rounded-xl bg-secondary-800/30 border border-secondary-700/50"
        >
          <div className="flex items-center space-x-3">
            <div className={`w-2 h-2 rounded-full ${trade.side === 'buy' ? 'bg-success-400' : 'bg-danger-400'}`} />
            <div>
              <p className="text-sm font-medium text-white">
                {trade.side.toUpperCase()} {trade.symbol}
              </p>
              <p className="text-xs text-secondary-400">
                {formatters.currency(trade.executedQuantity || 0)} @ {formatters.currency(trade.executedPrice || 0)}
              </p>
            </div>
          </div>
          <div className="text-right">
            <p className="text-sm font-medium text-white">
              {formatters.currency((trade.executedQuantity || 0) * (trade.executedPrice || 0))}
            </p>
            <p className="text-xs text-secondary-400">
              {formatters.timeAgo(trade.timestamp)}
            </p>
          </div>
        </motion.div>
      ))}
    </div>
  );
}