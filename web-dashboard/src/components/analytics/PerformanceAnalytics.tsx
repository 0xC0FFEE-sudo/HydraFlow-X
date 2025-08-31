'use client';

import React, { useState, useEffect } from 'react';
import { Card } from '../ui/Card';
import { Select } from '../ui/Select';
import { Button } from '../ui/Button';
import { Badge } from '../ui/Badge';
import { Tabs } from '../ui/Tabs';

interface PerformanceMetrics {
  totalReturn: number;
  totalReturnPercent: number;
  annualizedReturn: number;
  sharpeRatio: number;
  maxDrawdown: number;
  volatility: number;
  calmarRatio: number;
  sortinoRatio: number;
  winRate: number;
  profitFactor: number;
  avgWinningTrade: number;
  avgLosingTrade: number;
  largestWin: number;
  largestLoss: number;
  avgHoldingTime: number;
  totalTrades: number;
  winningTrades: number;
  losingTrades: number;
}

interface PeriodPerformance {
  period: string;
  return: number;
  benchmark: number;
  alpha: number;
  beta: number;
  trades: number;
}

interface RiskMetrics {
  var95: number; // Value at Risk 95%
  var99: number; // Value at Risk 99%
  expectedShortfall: number;
  maxDailyDrawdown: number;
  consecutiveLosses: number;
  downside: number;
  upsideCapture: number;
  downsideCapture: number;
}

interface StrategyPerformance {
  strategy: string;
  allocation: number;
  return: number;
  sharpe: number;
  maxDrawdown: number;
  trades: number;
  winRate: number;
  status: 'active' | 'paused' | 'disabled';
}

const TIME_PERIODS = [
  { value: '1d', label: 'Last 24 Hours' },
  { value: '7d', label: 'Last 7 Days' },
  { value: '30d', label: 'Last 30 Days' },
  { value: '90d', label: 'Last 3 Months' },
  { value: '1y', label: 'Last Year' },
  { value: 'all', label: 'All Time' },
];

export const PerformanceAnalytics: React.FC = () => {
  const [selectedPeriod, setSelectedPeriod] = useState('30d');
  const [activeTab, setActiveTab] = useState('overview');
  const [performanceMetrics, setPerformanceMetrics] = useState<PerformanceMetrics | null>(null);
  const [periodPerformance, setPeriodPerformance] = useState<PeriodPerformance[]>([]);
  const [riskMetrics, setRiskMetrics] = useState<RiskMetrics | null>(null);
  const [strategyPerformance, setStrategyPerformance] = useState<StrategyPerformance[]>([]);
  const [equityCurve, setEquityCurve] = useState<{ date: string; value: number; benchmark: number }[]>([]);

  // Generate mock performance data
  const generatePerformanceMetrics = (): PerformanceMetrics => {
    const baseReturn = 15000; // Starting with $15k profit
    const totalReturnPercent = 15 + (Math.random() - 0.5) * 10; // 10-20% range
    
    return {
      totalReturn: baseReturn * (1 + (Math.random() - 0.5) * 0.3),
      totalReturnPercent,
      annualizedReturn: totalReturnPercent * (365 / 30), // Approximate annualized
      sharpeRatio: 1.2 + Math.random() * 1.5, // 1.2-2.7 range
      maxDrawdown: -(2 + Math.random() * 8), // -2% to -10%
      volatility: 12 + Math.random() * 15, // 12-27%
      calmarRatio: 0.8 + Math.random() * 1.2,
      sortinoRatio: 1.5 + Math.random() * 1.5,
      winRate: 55 + Math.random() * 25, // 55-80%
      profitFactor: 1.2 + Math.random() * 1.8, // 1.2-3.0
      avgWinningTrade: 150 + Math.random() * 300,
      avgLosingTrade: -(50 + Math.random() * 150),
      largestWin: 800 + Math.random() * 1200,
      largestLoss: -(200 + Math.random() * 600),
      avgHoldingTime: 2 + Math.random() * 8, // 2-10 hours
      totalTrades: 450 + Math.floor(Math.random() * 300),
      winningTrades: 0,
      losingTrades: 0,
    };
  };

  const generatePeriodPerformance = (): PeriodPerformance[] => {
    const periods = ['1D', '1W', '1M', '3M', '6M', '1Y'];
    return periods.map(period => ({
      period,
      return: (Math.random() - 0.3) * 20, // -6% to 14%
      benchmark: (Math.random() - 0.4) * 15, // -6% to 9%
      alpha: (Math.random() - 0.5) * 10, // -5% to 5%
      beta: 0.7 + Math.random() * 0.6, // 0.7-1.3
      trades: Math.floor(10 + Math.random() * 100),
    }));
  };

  const generateRiskMetrics = (): RiskMetrics => ({
    var95: -(1000 + Math.random() * 2000), // -$1k to -$3k
    var99: -(2000 + Math.random() * 3000), // -$2k to -$5k
    expectedShortfall: -(2500 + Math.random() * 2500), // -$2.5k to -$5k
    maxDailyDrawdown: -(500 + Math.random() * 1500), // -$500 to -$2k
    consecutiveLosses: Math.floor(2 + Math.random() * 6), // 2-8 trades
    downside: 8 + Math.random() * 12, // 8-20%
    upsideCapture: 85 + Math.random() * 25, // 85-110%
    downsideCapture: 70 + Math.random() * 40, // 70-110%
  });

  const generateStrategyPerformance = (): StrategyPerformance[] => {
    const strategies = [
      'Momentum Scalping',
      'Mean Reversion',
      'Arbitrage Hunter',
      'Sentiment Trading',
      'MEV Frontrunning',
      'Grid Trading',
      'DCA Strategy',
    ];

    return strategies.map(strategy => ({
      strategy,
      allocation: 5 + Math.random() * 25, // 5-30%
      return: (Math.random() - 0.3) * 30, // -9% to 21%
      sharpe: 0.5 + Math.random() * 2.5, // 0.5-3.0
      maxDrawdown: -(1 + Math.random() * 15), // -1% to -16%
      trades: Math.floor(20 + Math.random() * 200),
      winRate: 40 + Math.random() * 45, // 40-85%
      status: Math.random() > 0.8 ? 'paused' : Math.random() > 0.9 ? 'disabled' : 'active',
    }));
  };

  const generateEquityCurve = (): { date: string; value: number; benchmark: number }[] => {
    const data = [];
    const days = 30;
    let portfolioValue = 100000;
    let benchmarkValue = 100000;
    
    for (let i = 0; i < days; i++) {
      const date = new Date();
      date.setDate(date.getDate() - (days - i));
      
      portfolioValue *= (1 + (Math.random() - 0.45) * 0.03); // Slightly positive bias
      benchmarkValue *= (1 + (Math.random() - 0.48) * 0.02); // Market performance
      
      data.push({
        date: date.toISOString().split('T')[0],
        value: portfolioValue,
        benchmark: benchmarkValue,
      });
    }
    
    return data;
  };

  // Initialize data
  useEffect(() => {
    const metrics = generatePerformanceMetrics();
    metrics.winningTrades = Math.floor(metrics.totalTrades * (metrics.winRate / 100));
    metrics.losingTrades = metrics.totalTrades - metrics.winningTrades;

    setPerformanceMetrics(metrics);
    setPeriodPerformance(generatePeriodPerformance());
    setRiskMetrics(generateRiskMetrics());
    setStrategyPerformance(generateStrategyPerformance());
    setEquityCurve(generateEquityCurve());
  }, [selectedPeriod]);

  const formatCurrency = (value: number): string => {
    return new Intl.NumberFormat('en-US', {
      style: 'currency',
      currency: 'USD',
      minimumFractionDigits: 0,
      maximumFractionDigits: 0,
    }).format(value);
  };

  const formatPercent = (value: number): string => {
    return `${value >= 0 ? '+' : ''}${value.toFixed(2)}%`;
  };

  const getPerformanceColor = (value: number): string => {
    return value >= 0 ? 'text-green-400' : 'text-red-400';
  };

  const getStatusBadge = (status: string) => {
    switch (status) {
      case 'active':
        return <Badge variant="success">Active</Badge>;
      case 'paused':
        return <Badge variant="secondary">Paused</Badge>;
      case 'disabled':
        return <Badge variant="destructive">Disabled</Badge>;
      default:
        return <Badge variant="secondary">{status}</Badge>;
    }
  };

  const renderOverview = () => (
    <div className="space-y-6">
      {/* Key Metrics Grid */}
      <div className="grid grid-cols-2 md:grid-cols-4 gap-4">
        <Card className="p-4">
          <div className="text-sm text-gray-400">Total Return</div>
          <div className={`text-2xl font-bold ${getPerformanceColor(performanceMetrics?.totalReturn || 0)}`}>
            {formatCurrency(performanceMetrics?.totalReturn || 0)}
          </div>
          <div className={`text-sm ${getPerformanceColor(performanceMetrics?.totalReturnPercent || 0)}`}>
            {formatPercent(performanceMetrics?.totalReturnPercent || 0)}
          </div>
        </Card>

        <Card className="p-4">
          <div className="text-sm text-gray-400">Sharpe Ratio</div>
          <div className="text-2xl font-bold text-white">
            {performanceMetrics?.sharpeRatio.toFixed(2)}
          </div>
          <div className="text-sm text-gray-400">Risk-adjusted return</div>
        </Card>

        <Card className="p-4">
          <div className="text-sm text-gray-400">Max Drawdown</div>
          <div className="text-2xl font-bold text-red-400">
            {formatPercent(performanceMetrics?.maxDrawdown || 0)}
          </div>
          <div className="text-sm text-gray-400">Peak to trough</div>
        </Card>

        <Card className="p-4">
          <div className="text-sm text-gray-400">Win Rate</div>
          <div className="text-2xl font-bold text-green-400">
            {performanceMetrics?.winRate.toFixed(1)}%
          </div>
          <div className="text-sm text-gray-400">
            {performanceMetrics?.winningTrades}/{performanceMetrics?.totalTrades} trades
          </div>
        </Card>
      </div>

      {/* Period Performance */}
      <Card className="p-6">
        <h3 className="text-lg font-semibold text-white mb-4">Period Performance</h3>
        <div className="overflow-x-auto">
          <table className="w-full text-sm">
            <thead>
              <tr className="border-b border-gray-700">
                <th className="text-left py-2 text-gray-400">Period</th>
                <th className="text-right py-2 text-gray-400">Return</th>
                <th className="text-right py-2 text-gray-400">Benchmark</th>
                <th className="text-right py-2 text-gray-400">Alpha</th>
                <th className="text-right py-2 text-gray-400">Beta</th>
                <th className="text-right py-2 text-gray-400">Trades</th>
              </tr>
            </thead>
            <tbody>
              {periodPerformance.map(period => (
                <tr key={period.period} className="border-b border-gray-800">
                  <td className="py-2 text-white">{period.period}</td>
                  <td className={`py-2 text-right ${getPerformanceColor(period.return)}`}>
                    {formatPercent(period.return)}
                  </td>
                  <td className={`py-2 text-right ${getPerformanceColor(period.benchmark)}`}>
                    {formatPercent(period.benchmark)}
                  </td>
                  <td className={`py-2 text-right ${getPerformanceColor(period.alpha)}`}>
                    {formatPercent(period.alpha)}
                  </td>
                  <td className="py-2 text-right text-white">
                    {period.beta.toFixed(2)}
                  </td>
                  <td className="py-2 text-right text-gray-400">
                    {period.trades}
                  </td>
                </tr>
              ))}
            </tbody>
          </table>
        </div>
      </Card>

      {/* Additional Metrics */}
      <div className="grid grid-cols-1 md:grid-cols-2 gap-6">
        <Card className="p-6">
          <h3 className="text-lg font-semibold text-white mb-4">Risk-Return Metrics</h3>
          <div className="space-y-3">
            <div className="flex justify-between">
              <span className="text-gray-400">Annualized Return:</span>
              <span className={getPerformanceColor(performanceMetrics?.annualizedReturn || 0)}>
                {formatPercent(performanceMetrics?.annualizedReturn || 0)}
              </span>
            </div>
            <div className="flex justify-between">
              <span className="text-gray-400">Volatility:</span>
              <span className="text-white">{formatPercent(performanceMetrics?.volatility || 0)}</span>
            </div>
            <div className="flex justify-between">
              <span className="text-gray-400">Calmar Ratio:</span>
              <span className="text-white">{performanceMetrics?.calmarRatio.toFixed(2)}</span>
            </div>
            <div className="flex justify-between">
              <span className="text-gray-400">Sortino Ratio:</span>
              <span className="text-white">{performanceMetrics?.sortinoRatio.toFixed(2)}</span>
            </div>
            <div className="flex justify-between">
              <span className="text-gray-400">Profit Factor:</span>
              <span className="text-white">{performanceMetrics?.profitFactor.toFixed(2)}</span>
            </div>
          </div>
        </Card>

        <Card className="p-6">
          <h3 className="text-lg font-semibold text-white mb-4">Trade Analysis</h3>
          <div className="space-y-3">
            <div className="flex justify-between">
              <span className="text-gray-400">Avg Winning Trade:</span>
              <span className="text-green-400">
                {formatCurrency(performanceMetrics?.avgWinningTrade || 0)}
              </span>
            </div>
            <div className="flex justify-between">
              <span className="text-gray-400">Avg Losing Trade:</span>
              <span className="text-red-400">
                {formatCurrency(performanceMetrics?.avgLosingTrade || 0)}
              </span>
            </div>
            <div className="flex justify-between">
              <span className="text-gray-400">Largest Win:</span>
              <span className="text-green-400">
                {formatCurrency(performanceMetrics?.largestWin || 0)}
              </span>
            </div>
            <div className="flex justify-between">
              <span className="text-gray-400">Largest Loss:</span>
              <span className="text-red-400">
                {formatCurrency(performanceMetrics?.largestLoss || 0)}
              </span>
            </div>
            <div className="flex justify-between">
              <span className="text-gray-400">Avg Holding Time:</span>
              <span className="text-white">
                {performanceMetrics?.avgHoldingTime.toFixed(1)} hours
              </span>
            </div>
          </div>
        </Card>
      </div>
    </div>
  );

  const renderRiskAnalysis = () => (
    <div className="space-y-6">
      <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-4">
        <Card className="p-4">
          <div className="text-sm text-gray-400">Value at Risk (95%)</div>
          <div className="text-2xl font-bold text-red-400">
            {formatCurrency(riskMetrics?.var95 || 0)}
          </div>
          <div className="text-sm text-gray-400">Daily 95% confidence</div>
        </Card>

        <Card className="p-4">
          <div className="text-sm text-gray-400">Value at Risk (99%)</div>
          <div className="text-2xl font-bold text-red-400">
            {formatCurrency(riskMetrics?.var99 || 0)}
          </div>
          <div className="text-sm text-gray-400">Daily 99% confidence</div>
        </Card>

        <Card className="p-4">
          <div className="text-sm text-gray-400">Expected Shortfall</div>
          <div className="text-2xl font-bold text-red-400">
            {formatCurrency(riskMetrics?.expectedShortfall || 0)}
          </div>
          <div className="text-sm text-gray-400">Conditional VaR</div>
        </Card>
      </div>

      <Card className="p-6">
        <h3 className="text-lg font-semibold text-white mb-4">Risk Metrics</h3>
        <div className="grid grid-cols-1 md:grid-cols-2 gap-6">
          <div className="space-y-3">
            <div className="flex justify-between">
              <span className="text-gray-400">Max Daily Drawdown:</span>
              <span className="text-red-400">{formatCurrency(riskMetrics?.maxDailyDrawdown || 0)}</span>
            </div>
            <div className="flex justify-between">
              <span className="text-gray-400">Consecutive Losses:</span>
              <span className="text-white">{riskMetrics?.consecutiveLosses} trades</span>
            </div>
            <div className="flex justify-between">
              <span className="text-gray-400">Downside Deviation:</span>
              <span className="text-white">{formatPercent(riskMetrics?.downside || 0)}</span>
            </div>
          </div>
          <div className="space-y-3">
            <div className="flex justify-between">
              <span className="text-gray-400">Upside Capture:</span>
              <span className="text-green-400">{riskMetrics?.upsideCapture.toFixed(1)}%</span>
            </div>
            <div className="flex justify-between">
              <span className="text-gray-400">Downside Capture:</span>
              <span className="text-red-400">{riskMetrics?.downsideCapture.toFixed(1)}%</span>
            </div>
          </div>
        </div>
      </Card>
    </div>
  );

  const renderStrategyBreakdown = () => (
    <Card className="p-6">
      <h3 className="text-lg font-semibold text-white mb-4">Strategy Performance</h3>
      <div className="overflow-x-auto">
        <table className="w-full text-sm">
          <thead>
            <tr className="border-b border-gray-700">
              <th className="text-left py-2 text-gray-400">Strategy</th>
              <th className="text-right py-2 text-gray-400">Allocation</th>
              <th className="text-right py-2 text-gray-400">Return</th>
              <th className="text-right py-2 text-gray-400">Sharpe</th>
              <th className="text-right py-2 text-gray-400">Max DD</th>
              <th className="text-right py-2 text-gray-400">Trades</th>
              <th className="text-right py-2 text-gray-400">Win Rate</th>
              <th className="text-center py-2 text-gray-400">Status</th>
            </tr>
          </thead>
          <tbody>
            {strategyPerformance.map(strategy => (
              <tr key={strategy.strategy} className="border-b border-gray-800">
                <td className="py-2 text-white">{strategy.strategy}</td>
                <td className="py-2 text-right text-white">{strategy.allocation.toFixed(1)}%</td>
                <td className={`py-2 text-right ${getPerformanceColor(strategy.return)}`}>
                  {formatPercent(strategy.return)}
                </td>
                <td className="py-2 text-right text-white">{strategy.sharpe.toFixed(2)}</td>
                <td className="py-2 text-right text-red-400">{formatPercent(strategy.maxDrawdown)}</td>
                <td className="py-2 text-right text-gray-400">{strategy.trades}</td>
                <td className="py-2 text-right text-white">{strategy.winRate.toFixed(1)}%</td>
                <td className="py-2 text-center">{getStatusBadge(strategy.status)}</td>
              </tr>
            ))}
          </tbody>
        </table>
      </div>
    </Card>
  );

  const tabs = [
    { id: 'overview', label: 'Overview' },
    { id: 'risk', label: 'Risk Analysis' },
    { id: 'strategies', label: 'Strategy Breakdown' },
  ];

  return (
    <div className="space-y-6">
      {/* Header */}
      <div className="flex justify-between items-center">
        <h2 className="text-2xl font-bold text-white">Performance Analytics</h2>
        <div className="flex items-center space-x-4">
          <Select
            value={selectedPeriod}
            onValueChange={setSelectedPeriod}
            options={TIME_PERIODS}
          />
          <Button variant="outline">Export Report</Button>
        </div>
      </div>

      {/* Tabs */}
      <Tabs value={activeTab} onValueChange={setActiveTab} tabs={tabs}>
        <div className="mt-6">
          {activeTab === 'overview' && renderOverview()}
          {activeTab === 'risk' && renderRiskAnalysis()}
          {activeTab === 'strategies' && renderStrategyBreakdown()}
        </div>
      </Tabs>
    </div>
  );
};
