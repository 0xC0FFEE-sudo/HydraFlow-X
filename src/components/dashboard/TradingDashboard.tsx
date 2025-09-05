'use client';

import { useState, useEffect } from 'react';
import { motion, AnimatePresence } from 'framer-motion';
import { Card, CardHeader, CardTitle, CardContent } from '@/components/ui/Card';
import { Badge } from '@/components/ui/Badge';
import { Button } from '@/components/ui/Button';
import { Table, TableBody, TableCell, TableHead, TableHeader, TableRow } from '@/components/ui/Table';
import { RealTimeChart } from '@/components/charts/RealTimeChart';
import { apiService } from '@/lib/api-service';
import {
  TrendingUp,
  TrendingDown,
  DollarSign,
  Activity,
  Zap,
  Target,
  Clock,
  BarChart3,
  PieChart,
  ArrowUpRight,
  ArrowDownRight,
  RefreshCw,
  Play,
  Pause,
  AlertTriangle,
  CheckCircle,
  XCircle,
  Eye,
  EyeOff,
} from 'lucide-react';

interface Position {
  id: string;
  symbol: string;
  side: 'buy' | 'sell';
  size: number;
  price: number;
  currentPrice: number;
  pnl: number;
  pnlPercent: number;
  timestamp: number;
  status: 'open' | 'closing' | 'closed';
}

interface Order {
  id: string;
  symbol: string;
  type: 'market' | 'limit' | 'stop';
  side: 'buy' | 'sell';
  size: number;
  price: number;
  status: 'pending' | 'filled' | 'cancelled' | 'rejected';
  timestamp: number;
}

interface MarketData {
  symbol: string;
  price: number;
  change24h: number;
  changePercent24h: number;
  volume24h: number;
  high24h: number;
  low24h: number;
  lastUpdate: number;
}

export function TradingDashboard() {
  const [positions, setPositions] = useState<Position[]>([]);
  const [orders, setOrders] = useState<Order[]>([]);
  const [marketData, setMarketData] = useState<MarketData[]>([]);
  const [isLoading, setIsLoading] = useState(true);
  const [isPaused, setIsPaused] = useState(false);
  const [lastUpdate, setLastUpdate] = useState<Date | null>(null);
  const [selectedSymbol, setSelectedSymbol] = useState<string | null>(null);

  const fetchTradingData = async () => {
    try {
      const [positionsRes, ordersRes, marketRes] = await Promise.all([
        apiService.get('/api/v1/positions'),
        apiService.get('/api/v1/orders'),
        apiService.get('/api/v1/market/prices'),
      ]);

      // Transform positions data
      const transformedPositions: Position[] = (positionsRes.data || []).map((pos: any) => ({
        id: pos.id || Math.random().toString(),
        symbol: pos.symbol || 'UNKNOWN',
        side: pos.side || 'buy',
        size: pos.size || 0,
        price: pos.price || 0,
        currentPrice: pos.currentPrice || pos.price || 0,
        pnl: pos.pnl || 0,
        pnlPercent: pos.pnlPercent || 0,
        timestamp: pos.timestamp || Date.now(),
        status: pos.status || 'open',
      }));

      // Transform orders data
      const transformedOrders: Order[] = (ordersRes.data || []).map((order: any) => ({
        id: order.id || Math.random().toString(),
        symbol: order.symbol || 'UNKNOWN',
        type: order.type || 'market',
        side: order.side || 'buy',
        size: order.size || 0,
        price: order.price || 0,
        status: order.status || 'pending',
        timestamp: order.timestamp || Date.now(),
      }));

      // Transform market data
      const transformedMarketData: MarketData[] = (marketRes.data || []).map((market: any) => ({
        symbol: market.symbol || 'UNKNOWN',
        price: market.price || 0,
        change24h: market.change24h || 0,
        changePercent24h: market.changePercent24h || 0,
        volume24h: market.volume24h || 0,
        high24h: market.high24h || 0,
        low24h: market.low24h || 0,
        lastUpdate: market.lastUpdate || Date.now(),
      }));

      setPositions(transformedPositions);
      setOrders(transformedOrders);
      setMarketData(transformedMarketData);
      setLastUpdate(new Date());
    } catch (error) {
      console.error('Failed to fetch trading data:', error);
    } finally {
      setIsLoading(false);
    }
  };

  useEffect(() => {
    fetchTradingData();

    if (!isPaused) {
      const interval = setInterval(fetchTradingData, 2000); // Update every 2 seconds
      return () => clearInterval(interval);
    }
  }, [isPaused]);

  const togglePause = () => {
    setIsPaused(!isPaused);
  };

  const formatCurrency = (value: number) => {
    return new Intl.NumberFormat('en-US', {
      style: 'currency',
      currency: 'USD',
      minimumFractionDigits: 2,
      maximumFractionDigits: 2,
    }).format(value);
  };

  const formatPercentage = (value: number) => {
    const sign = value >= 0 ? '+' : '';
    return `${sign}${value.toFixed(2)}%`;
  };

  const getStatusColor = (status: string) => {
    switch (status.toLowerCase()) {
      case 'open':
      case 'filled':
        return 'text-green-600';
      case 'pending':
        return 'text-yellow-600';
      case 'closing':
      case 'cancelled':
      case 'rejected':
        return 'text-red-600';
      default:
        return 'text-gray-600';
    }
  };

  const getStatusIcon = (status: string) => {
    switch (status.toLowerCase()) {
      case 'open':
      case 'filled':
        return <CheckCircle className="w-4 h-4 text-green-600" />;
      case 'pending':
        return <Clock className="w-4 h-4 text-yellow-600" />;
      case 'closing':
      case 'cancelled':
      case 'rejected':
        return <XCircle className="w-4 h-4 text-red-600" />;
      default:
        return <Activity className="w-4 h-4 text-gray-600" />;
    }
  };

  const totalPnl = positions.reduce((sum, pos) => sum + pos.pnl, 0);
  const totalValue = positions.reduce((sum, pos) => sum + (pos.size * pos.currentPrice), 0);
  const activeOrders = orders.filter(order => order.status === 'pending').length;

  return (
    <div className="space-y-6">
      {/* Header */}
      <div className="flex items-center justify-between">
        <div>
          <h2 className="text-3xl font-bold tracking-tight">Trading Dashboard</h2>
          <p className="text-muted-foreground">
            Real-time positions, orders, and market data
          </p>
        </div>
        <div className="flex items-center gap-4">
          <Badge variant={isPaused ? 'secondary' : 'default'}>
            {isPaused ? 'Paused' : 'Live'}
          </Badge>
          <Button
            variant="outline"
            size="sm"
            onClick={togglePause}
          >
            {isPaused ? <Play className="w-4 h-4" /> : <Pause className="w-4 h-4" />}
          </Button>
          <Button
            variant="outline"
            size="sm"
            onClick={fetchTradingData}
          >
            <RefreshCw className="w-4 h-4" />
          </Button>
        </div>
      </div>

      {/* Key Metrics */}
      <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-4 gap-6">
        <Card>
          <CardHeader className="flex flex-row items-center justify-between space-y-0 pb-2">
            <CardTitle className="text-sm font-medium">Portfolio Value</CardTitle>
            <DollarSign className="h-4 w-4 text-muted-foreground" />
          </CardHeader>
          <CardContent>
            <div className="text-2xl font-bold">{formatCurrency(totalValue)}</div>
            <p className="text-xs text-muted-foreground">
              {positions.length} positions
            </p>
          </CardContent>
        </Card>

        <Card>
          <CardHeader className="flex flex-row items-center justify-between space-y-0 pb-2">
            <CardTitle className="text-sm font-medium">Total P&L</CardTitle>
            {totalPnl >= 0 ? (
              <TrendingUp className="h-4 w-4 text-green-600" />
            ) : (
              <TrendingDown className="h-4 w-4 text-red-600" />
            )}
          </CardHeader>
          <CardContent>
            <div className={`text-2xl font-bold ${totalPnl >= 0 ? 'text-green-600' : 'text-red-600'}`}>
              {formatCurrency(totalPnl)}
            </div>
            <p className="text-xs text-muted-foreground">
              {formatPercentage(totalValue > 0 ? (totalPnl / totalValue) * 100 : 0)} of portfolio
            </p>
          </CardContent>
        </Card>

        <Card>
          <CardHeader className="flex flex-row items-center justify-between space-y-0 pb-2">
            <CardTitle className="text-sm font-medium">Active Orders</CardTitle>
            <Target className="h-4 w-4 text-muted-foreground" />
          </CardHeader>
          <CardContent>
            <div className="text-2xl font-bold">{activeOrders}</div>
            <p className="text-xs text-muted-foreground">
              {orders.length} total orders
            </p>
          </CardContent>
        </Card>

        <Card>
          <CardHeader className="flex flex-row items-center justify-between space-y-0 pb-2">
            <CardTitle className="text-sm font-medium">Market Status</CardTitle>
            <Activity className="h-4 w-4 text-muted-foreground" />
          </CardHeader>
          <CardContent>
            <div className="text-2xl font-bold">{marketData.length}</div>
            <p className="text-xs text-muted-foreground">
              Symbols tracked
            </p>
          </CardContent>
        </Card>
      </div>

      {/* Charts and Tables */}
      <div className="grid grid-cols-1 lg:grid-cols-2 gap-6">
        {/* Price Chart */}
        <Card className="lg:col-span-2">
          <CardHeader>
            <CardTitle>Market Overview</CardTitle>
          </CardHeader>
          <CardContent>
            <RealTimeChart
              title="Price Movements"
              subtitle="Real-time price data across all tracked symbols"
              endpoint="/api/v1/market/prices"
              type="line"
              yAxisLabel="Price (USD)"
              color="#3b82f6"
              height={400}
            />
          </CardContent>
        </Card>

        {/* Positions Table */}
        <Card>
          <CardHeader>
            <CardTitle>Open Positions</CardTitle>
          </CardHeader>
          <CardContent>
            <div className="space-y-4">
              {positions.length === 0 ? (
                <div className="text-center py-8 text-muted-foreground">
                  No open positions
                </div>
              ) : (
                <div className="space-y-3">
                  {positions.slice(0, 5).map((position) => (
                    <div key={position.id} className="flex items-center justify-between p-3 border rounded-lg">
                      <div className="flex items-center gap-3">
                        {getStatusIcon(position.status)}
                        <div>
                          <div className="font-medium">{position.symbol}</div>
                          <div className="text-sm text-muted-foreground">
                            {position.side.toUpperCase()} {position.size.toFixed(4)}
                          </div>
                        </div>
                      </div>
                      <div className="text-right">
                        <div className={`font-medium ${position.pnl >= 0 ? 'text-green-600' : 'text-red-600'}`}>
                          {formatCurrency(position.pnl)}
                        </div>
                        <div className={`text-sm ${position.pnl >= 0 ? 'text-green-600' : 'text-red-600'}`}>
                          {formatPercentage(position.pnlPercent)}
                        </div>
                      </div>
                    </div>
                  ))}
                </div>
              )}
            </div>
          </CardContent>
        </Card>

        {/* Orders Table */}
        <Card>
          <CardHeader>
            <CardTitle>Recent Orders</CardTitle>
          </CardHeader>
          <CardContent>
            <div className="space-y-4">
              {orders.length === 0 ? (
                <div className="text-center py-8 text-muted-foreground">
                  No orders
                </div>
              ) : (
                <div className="space-y-3">
                  {orders.slice(0, 5).map((order) => (
                    <div key={order.id} className="flex items-center justify-between p-3 border rounded-lg">
                      <div className="flex items-center gap-3">
                        {getStatusIcon(order.status)}
                        <div>
                          <div className="font-medium">{order.symbol}</div>
                          <div className="text-sm text-muted-foreground">
                            {order.side.toUpperCase()} {order.size.toFixed(4)}
                          </div>
                        </div>
                      </div>
                      <div className="text-right">
                        <div className="font-medium">{formatCurrency(order.price)}</div>
                        <div className={`text-sm ${getStatusColor(order.status)}`}>
                          {order.status.toUpperCase()}
                        </div>
                      </div>
                    </div>
                  ))}
                </div>
              )}
            </div>
          </CardContent>
        </Card>

        {/* Market Data Table */}
        <Card className="lg:col-span-2">
          <CardHeader>
            <CardTitle>Market Data</CardTitle>
          </CardHeader>
          <CardContent>
            <Table>
              <TableHeader>
                <TableRow>
                  <TableHead>Symbol</TableHead>
                  <TableHead>Price</TableHead>
                  <TableHead>24h Change</TableHead>
                  <TableHead>Volume</TableHead>
                  <TableHead>High/Low</TableHead>
                </TableRow>
              </TableHeader>
              <TableBody>
                {marketData.slice(0, 10).map((market) => (
                  <TableRow key={market.symbol}>
                    <TableCell className="font-medium">{market.symbol}</TableCell>
                    <TableCell>{formatCurrency(market.price)}</TableCell>
                    <TableCell>
                      <div className={`flex items-center gap-1 ${market.changePercent24h >= 0 ? 'text-green-600' : 'text-red-600'}`}>
                        {market.changePercent24h >= 0 ? (
                          <ArrowUpRight className="w-4 h-4" />
                        ) : (
                          <ArrowDownRight className="w-4 h-4" />
                        )}
                        {formatPercentage(market.changePercent24h)}
                      </div>
                    </TableCell>
                    <TableCell>{formatCurrency(market.volume24h)}</TableCell>
                    <TableCell>
                      <div className="text-sm">
                        <div>H: {formatCurrency(market.high24h)}</div>
                        <div>L: {formatCurrency(market.low24h)}</div>
                      </div>
                    </TableCell>
                  </TableRow>
                ))}
              </TableBody>
            </Table>
          </CardContent>
        </Card>
      </div>

      {/* Last Update Info */}
      {lastUpdate && (
        <div className="text-center text-sm text-muted-foreground">
          Last updated: {lastUpdate.toLocaleString()}
        </div>
      )}
    </div>
  );
}