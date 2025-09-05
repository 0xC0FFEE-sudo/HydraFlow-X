'use client';

import { useState, useEffect } from 'react';
import { motion } from 'framer-motion';
import { Card, CardHeader, CardTitle, CardContent } from '@/components/ui/Card';
import { Badge } from '@/components/ui/Badge';
import { Button } from '@/components/ui/Button';
import { Tabs, TabsContent, TabsList, TabsTrigger } from '@/components/ui/Tabs';
import { RealTimeChart } from '@/components/charts/RealTimeChart';
import { apiService } from '@/lib/api-service';
import {
  TrendingUp,
  TrendingDown,
  Activity,
  BarChart3,
  PieChart,
  LineChart,
  Zap,
  Shield,
  Clock,
  Target,
  DollarSign,
  Users,
  AlertTriangle,
  CheckCircle,
  RefreshCw,
} from 'lucide-react';

interface AnalyticsData {
  performance: {
    tradesExecuted: number;
    volume24h: number;
    pnl24h: number;
    winRate: number;
    avgTradeSize: number;
    bestTrade: number;
    worstTrade: number;
  };
  risk: {
    currentExposure: number;
    maxDrawdown: number;
    sharpeRatio: number;
    var95: number;
    positionCount: number;
    liquidPositions: number;
  };
  system: {
    cpuUsage: number;
    memoryUsage: number;
    activeConnections: number;
    queueDepth: number;
    latency: number;
    uptime: number;
  };
  health: {
    overallStatus: string;
    healthyComponents: number;
    totalComponents: number;
    lastHealthCheck: string;
  };
}

export function AnalyticsDashboard() {
  const [analyticsData, setAnalyticsData] = useState<AnalyticsData | null>(null);
  const [isLoading, setIsLoading] = useState(true);
  const [lastUpdate, setLastUpdate] = useState<Date | null>(null);

  const fetchAnalyticsData = async () => {
    try {
      const [metrics, health, performance] = await Promise.all([
        apiService.get('/api/v1/metrics'),
        apiService.get('/api/v1/health'),
        apiService.get('/api/v1/performance/stats'),
      ]);

      // Transform data for the dashboard
      const transformedData: AnalyticsData = {
        performance: {
          tradesExecuted: metrics.data?.trades_executed_total || 0,
          volume24h: metrics.data?.trade_volume_total || 0,
          pnl24h: metrics.data?.pnl_24h || 0,
          winRate: metrics.data?.win_rate_percent || 0,
          avgTradeSize: metrics.data?.avg_trade_size || 0,
          bestTrade: metrics.data?.best_trade || 0,
          worstTrade: metrics.data?.worst_trade || 0,
        },
        risk: {
          currentExposure: metrics.data?.risk_exposure_amount || 0,
          maxDrawdown: metrics.data?.max_drawdown_percent || 0,
          sharpeRatio: metrics.data?.sharpe_ratio || 0,
          var95: metrics.data?.portfolio_var_amount || 0,
          positionCount: metrics.data?.active_positions || 0,
          liquidPositions: metrics.data?.liquid_positions || 0,
        },
        system: {
          cpuUsage: metrics.data?.cpu_usage_percent || 0,
          memoryUsage: metrics.data?.memory_usage_bytes || 0,
          activeConnections: metrics.data?.active_connections || 0,
          queueDepth: metrics.data?.queue_depth || 0,
          latency: metrics.data?.request_latency_ms || 0,
          uptime: metrics.data?.system_uptime_seconds || 0,
        },
        health: {
          overallStatus: health.data?.status || 'UNKNOWN',
          healthyComponents: health.data?.healthy_components || 0,
          totalComponents: health.data?.total_components || 0,
          lastHealthCheck: health.data?.timestamp || new Date().toISOString(),
        },
      };

      setAnalyticsData(transformedData);
      setLastUpdate(new Date());
    } catch (error) {
      console.error('Failed to fetch analytics data:', error);
    } finally {
      setIsLoading(false);
    }
  };

  useEffect(() => {
    fetchAnalyticsData();
    const interval = setInterval(fetchAnalyticsData, 10000); // Update every 10 seconds
    return () => clearInterval(interval);
  }, []);

  const formatCurrency = (value: number) => {
    return new Intl.NumberFormat('en-US', {
      style: 'currency',
      currency: 'USD',
      minimumFractionDigits: 2,
      maximumFractionDigits: 2,
    }).format(value);
  };

  const formatPercentage = (value: number) => {
    return `${value.toFixed(2)}%`;
  };

  const formatNumber = (value: number) => {
    return new Intl.NumberFormat('en-US').format(value);
  };

  const formatBytes = (bytes: number) => {
    const units = ['B', 'KB', 'MB', 'GB', 'TB'];
    let value = bytes;
    let unitIndex = 0;

    while (value >= 1024 && unitIndex < units.length - 1) {
      value /= 1024;
      unitIndex++;
    }

    return `${value.toFixed(2)} ${units[unitIndex]}`;
  };

  const getHealthStatusColor = (status: string) => {
    switch (status.toLowerCase()) {
      case 'healthy': return 'text-green-600';
      case 'degraded': return 'text-yellow-600';
      case 'unhealthy': return 'text-orange-600';
      case 'critical': return 'text-red-600';
      default: return 'text-gray-600';
    }
  };

  const getHealthStatusIcon = (status: string) => {
    switch (status.toLowerCase()) {
      case 'healthy': return <CheckCircle className="w-5 h-5 text-green-600" />;
      case 'degraded': return <AlertTriangle className="w-5 h-5 text-yellow-600" />;
      case 'unhealthy': return <AlertTriangle className="w-5 h-5 text-orange-600" />;
      case 'critical': return <AlertTriangle className="w-5 h-5 text-red-600" />;
      default: return <Activity className="w-5 h-5 text-gray-600" />;
    }
  };

  if (isLoading || !analyticsData) {
    return (
      <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-4 gap-6">
        {[...Array(8)].map((_, i) => (
          <Card key={i} className="animate-pulse">
            <CardHeader>
              <div className="h-4 bg-gray-200 rounded w-3/4"></div>
            </CardHeader>
            <CardContent>
              <div className="h-8 bg-gray-200 rounded w-1/2 mb-2"></div>
              <div className="h-4 bg-gray-200 rounded w-full"></div>
            </CardContent>
          </Card>
        ))}
      </div>
    );
  }

  return (
    <div className="space-y-6">
      {/* Header */}
      <div className="flex items-center justify-between">
        <div>
          <h2 className="text-3xl font-bold tracking-tight">Analytics Dashboard</h2>
          <p className="text-muted-foreground">
            Real-time trading performance and system analytics
          </p>
        </div>
        <div className="flex items-center gap-4">
          <div className="flex items-center gap-2">
            {getHealthStatusIcon(analyticsData.health.overallStatus)}
            <span className={`font-medium ${getHealthStatusColor(analyticsData.health.overallStatus)}`}>
              {analyticsData.health.overallStatus}
            </span>
          </div>
          <Button
            variant="outline"
            size="sm"
            onClick={fetchAnalyticsData}
          >
            <RefreshCw className="w-4 h-4 mr-2" />
            Refresh
          </Button>
        </div>
      </div>

      {/* Key Metrics Cards */}
      <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-4 gap-6">
        <Card>
          <CardHeader className="flex flex-row items-center justify-between space-y-0 pb-2">
            <CardTitle className="text-sm font-medium">Trades Executed</CardTitle>
            <BarChart3 className="h-4 w-4 text-muted-foreground" />
          </CardHeader>
          <CardContent>
            <div className="text-2xl font-bold">{formatNumber(analyticsData.performance.tradesExecuted)}</div>
            <p className="text-xs text-muted-foreground">
              +12% from last hour
            </p>
          </CardContent>
        </Card>

        <Card>
          <CardHeader className="flex flex-row items-center justify-between space-y-0 pb-2">
            <CardTitle className="text-sm font-medium">24h Volume</CardTitle>
            <DollarSign className="h-4 w-4 text-muted-foreground" />
          </CardHeader>
          <CardContent>
            <div className="text-2xl font-bold">{formatCurrency(analyticsData.performance.volume24h)}</div>
            <p className="text-xs text-muted-foreground">
              +8% from yesterday
            </p>
          </CardContent>
        </Card>

        <Card>
          <CardHeader className="flex flex-row items-center justify-between space-y-0 pb-2">
            <CardTitle className="text-sm font-medium">Win Rate</CardTitle>
            <Target className="h-4 w-4 text-muted-foreground" />
          </CardHeader>
          <CardContent>
            <div className="text-2xl font-bold">{formatPercentage(analyticsData.performance.winRate)}</div>
            <p className="text-xs text-muted-foreground">
              +2.1% from last week
            </p>
          </CardContent>
        </Card>

        <Card>
          <CardHeader className="flex flex-row items-center justify-between space-y-0 pb-2">
            <CardTitle className="text-sm font-medium">System Health</CardTitle>
            {getHealthStatusIcon(analyticsData.health.overallStatus)}
          </CardHeader>
          <CardContent>
            <div className="text-2xl font-bold">
              {analyticsData.health.healthyComponents}/{analyticsData.health.totalComponents}
            </div>
            <p className="text-xs text-muted-foreground">
              Components healthy
            </p>
          </CardContent>
        </Card>
      </div>

      {/* Charts Section */}
      <Tabs defaultValue="performance" className="space-y-6">
        <TabsList className="grid w-full grid-cols-4">
          <TabsTrigger value="performance">Performance</TabsTrigger>
          <TabsTrigger value="risk">Risk Analytics</TabsTrigger>
          <TabsTrigger value="system">System Metrics</TabsTrigger>
          <TabsTrigger value="health">Health Status</TabsTrigger>
        </TabsList>

        <TabsContent value="performance" className="space-y-6">
          <div className="grid grid-cols-1 lg:grid-cols-2 gap-6">
            <RealTimeChart
              title="Trade Volume"
              subtitle="Real-time trading volume"
              endpoint="/api/v1/metrics"
              type="area"
              yAxisLabel="Volume (USD)"
              color="#3b82f6"
              height={300}
            />

            <RealTimeChart
              title="PnL Over Time"
              subtitle="Profit and loss tracking"
              endpoint="/api/v1/performance/stats"
              type="line"
              yAxisLabel="PnL (USD)"
              color="#10b981"
              height={300}
            />
          </div>

          <div className="grid grid-cols-1 lg:grid-cols-2 gap-6">
            <RealTimeChart
              title="Trade Frequency"
              subtitle="Trades per minute"
              endpoint="/api/v1/metrics"
              type="bar"
              yAxisLabel="Trades/min"
              color="#f59e0b"
              height={300}
            />

            <RealTimeChart
              title="Win Rate Trend"
              subtitle="Trading success rate over time"
              endpoint="/api/v1/performance/stats"
              type="line"
              yAxisLabel="Win Rate (%)"
              color="#ef4444"
              height={300}
            />
          </div>
        </TabsContent>

        <TabsContent value="risk" className="space-y-6">
          <div className="grid grid-cols-1 lg:grid-cols-2 gap-6">
            <RealTimeChart
              title="Risk Exposure"
              subtitle="Current portfolio risk exposure"
              endpoint="/api/v1/risk/metrics"
              type="area"
              yAxisLabel="Exposure (USD)"
              color="#dc2626"
              height={300}
            />

            <RealTimeChart
              title="VaR 95%"
              subtitle="Value at Risk (95% confidence)"
              endpoint="/api/v1/risk/metrics"
              type="line"
              yAxisLabel="VaR (USD)"
              color="#7c3aed"
              height={300}
            />
          </div>

          <div className="grid grid-cols-1 lg:grid-cols-2 gap-6">
            <RealTimeChart
              title="Drawdown"
              subtitle="Maximum drawdown tracking"
              endpoint="/api/v1/risk/metrics"
              type="area"
              yAxisLabel="Drawdown (%)"
              color="#f97316"
              height={300}
            />

            <RealTimeChart
              title="Sharpe Ratio"
              subtitle="Risk-adjusted return metric"
              endpoint="/api/v1/risk/metrics"
              type="line"
              yAxisLabel="Sharpe Ratio"
              color="#06b6d4"
              height={300}
            />
          </div>
        </TabsContent>

        <TabsContent value="system" className="space-y-6">
          <div className="grid grid-cols-1 lg:grid-cols-2 gap-6">
            <RealTimeChart
              title="CPU Usage"
              subtitle="System CPU utilization"
              endpoint="/api/v1/metrics"
              type="line"
              yAxisLabel="CPU (%)"
              color="#3b82f6"
              height={300}
            />

            <RealTimeChart
              title="Memory Usage"
              subtitle="System memory consumption"
              endpoint="/api/v1/metrics"
              type="area"
              yAxisLabel="Memory (MB)"
              color="#10b981"
              height={300}
            />
          </div>

          <div className="grid grid-cols-1 lg:grid-cols-2 gap-6">
            <RealTimeChart
              title="Active Connections"
              subtitle="Current active connections"
              endpoint="/api/v1/metrics"
              type="line"
              yAxisLabel="Connections"
              color="#f59e0b"
              height={300}
            />

            <RealTimeChart
              title="Request Latency"
              subtitle="API request response time"
              endpoint="/api/v1/performance/stats"
              type="scatter"
              yAxisLabel="Latency (ms)"
              color="#ef4444"
              height={300}
            />
          </div>
        </TabsContent>

        <TabsContent value="health" className="space-y-6">
          <Card>
            <CardHeader>
              <CardTitle className="flex items-center gap-2">
                <Shield className="w-5 h-5" />
                Component Health Status
              </CardTitle>
            </CardHeader>
            <CardContent>
              <div className="space-y-4">
                {/* Health status will be shown here */}
                <div className="text-center py-8 text-muted-foreground">
                  Health monitoring data will be displayed here
                </div>
              </div>
            </CardContent>
          </Card>
        </TabsContent>
      </Tabs>

      {/* Last Update Info */}
      {lastUpdate && (
        <div className="text-center text-sm text-muted-foreground">
          Last updated: {lastUpdate.toLocaleString()}
        </div>
      )}
    </div>
  );
}