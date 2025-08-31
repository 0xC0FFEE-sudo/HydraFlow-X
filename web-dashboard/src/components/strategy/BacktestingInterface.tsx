'use client';

import React, { useState, useEffect } from 'react';
import { Card } from '../ui/Card';
import { Button } from '../ui/Button';
import { Badge } from '../ui/Badge';
import { Select } from '../ui/Select';
import { Input } from '../ui/Input';
import { Tabs } from '../ui/Tabs';

interface BacktestConfig {
  strategy: string;
  startDate: string;
  endDate: string;
  initialCapital: number;
  symbols: string[];
  timeframe: string;
  commissionRate: number;
  slippageBps: number;
  enableSentiment: boolean;
  enableAI: boolean;
}

interface BacktestResults {
  summary: {
    totalReturn: number;
    annualizedReturn: number;
    volatility: number;
    sharpeRatio: number;
    sortinoRatio: number;
    calmarRatio: number;
    maxDrawdown: number;
    winRate: number;
    profitFactor: number;
    totalTrades: number;
    avgTrade: number;
    avgWinner: number;
    avgLoser: number;
    largestWin: number;
    largestLoss: number;
    avgHoldingTime: number;
  };
  equityCurve: { date: string; value: number; benchmark: number }[];
  trades: BacktestTrade[];
  monthlyReturns: { month: string; return: number; benchmark: number }[];
  drawdownHistory: { date: string; drawdown: number }[];
  riskMetrics: {
    var95: number;
    var99: number;
    expectedShortfall: number;
    beta: number;
    alpha: number;
    trackingError: number;
  };
}

interface BacktestTrade {
  id: string;
  symbol: string;
  side: 'BUY' | 'SELL';
  entryTime: string;
  exitTime: string;
  entryPrice: number;
  exitPrice: number;
  quantity: number;
  pnl: number;
  pnlPercent: number;
  holdingTime: number;
  fees: number;
  strategy: string;
  confidence: number;
}

interface StrategyComparison {
  strategy: string;
  return: number;
  sharpe: number;
  maxDrawdown: number;
  winRate: number;
  trades: number;
  correlation: number;
}

const AVAILABLE_STRATEGIES = [
  { value: 'momentum_scalping', label: 'Momentum Scalping' },
  { value: 'mean_reversion', label: 'Mean Reversion' },
  { value: 'sentiment_trading', label: 'AI Sentiment Trading' },
  { value: 'grid_trading', label: 'Dynamic Grid Trading' },
  { value: 'arbitrage', label: 'Cross-Exchange Arbitrage' },
];

const TIMEFRAMES = [
  { value: '1m', label: '1 Minute' },
  { value: '5m', label: '5 Minutes' },
  { value: '15m', label: '15 Minutes' },
  { value: '1h', label: '1 Hour' },
  { value: '4h', label: '4 Hours' },
  { value: '1d', label: '1 Day' },
];

const AVAILABLE_SYMBOLS = ['BTC', 'ETH', 'SOL', 'MATIC', 'AVAX', 'ADA', 'DOT', 'ATOM', 'LINK', 'UNI'];

export const BacktestingInterface: React.FC = () => {
  const [activeTab, setActiveTab] = useState('configure');
  const [config, setConfig] = useState<BacktestConfig>({
    strategy: 'momentum_scalping',
    startDate: '2024-01-01',
    endDate: '2024-01-31',
    initialCapital: 100000,
    symbols: ['BTC', 'ETH', 'SOL'],
    timeframe: '15m',
    commissionRate: 0.001,
    slippageBps: 10,
    enableSentiment: true,
    enableAI: true,
  });
  const [isRunning, setIsRunning] = useState(false);
  const [progress, setProgress] = useState(0);
  const [results, setResults] = useState<BacktestResults | null>(null);
  const [comparisons, setComparisons] = useState<StrategyComparison[]>([]);

  // Generate mock backtest results
  const generateBacktestResults = (): BacktestResults => {
    const totalReturn = 15 + (Math.random() - 0.3) * 40; // -10% to 45%
    const volatility = 20 + Math.random() * 30; // 20-50%
    const maxDrawdown = -(2 + Math.random() * 18); // -2% to -20%
    const winRate = 45 + Math.random() * 35; // 45-80%
    const totalTrades = 100 + Math.floor(Math.random() * 400); // 100-500
    
    // Generate equity curve
    const equityCurve = [];
    const days = 30;
    let portfolioValue = config.initialCapital;
    let benchmarkValue = config.initialCapital;
    
    for (let i = 0; i < days; i++) {
      const date = new Date(config.startDate);
      date.setDate(date.getDate() + i);
      
      portfolioValue *= (1 + (Math.random() - 0.45) * 0.03);
      benchmarkValue *= (1 + (Math.random() - 0.48) * 0.02);
      
      equityCurve.push({
        date: date.toISOString().split('T')[0],
        value: portfolioValue,
        benchmark: benchmarkValue,
      });
    }

    // Generate trades
    const trades: BacktestTrade[] = [];
    for (let i = 0; i < totalTrades; i++) {
      const symbol = config.symbols[Math.floor(Math.random() * config.symbols.length)];
      const side = Math.random() > 0.5 ? 'BUY' : 'SELL';
      const entryPrice = 100 + Math.random() * 900;
      const pnlPercent = (Math.random() - 0.4) * 10; // Slightly positive bias
      const exitPrice = entryPrice * (1 + pnlPercent / 100);
      const quantity = (500 + Math.random() * 1500) / entryPrice;
      const pnl = (exitPrice - entryPrice) * quantity;
      
      trades.push({
        id: `trade_${i}`,
        symbol,
        side,
        entryTime: new Date(Date.now() - Math.random() * 30 * 24 * 3600 * 1000).toISOString(),
        exitTime: new Date(Date.now() - Math.random() * 29 * 24 * 3600 * 1000).toISOString(),
        entryPrice,
        exitPrice,
        quantity,
        pnl,
        pnlPercent,
        holdingTime: 5 + Math.random() * 300, // 5-305 minutes
        fees: quantity * entryPrice * config.commissionRate,
        strategy: config.strategy,
        confidence: 0.6 + Math.random() * 0.4,
      });
    }

    // Calculate summary metrics
    const winningTrades = trades.filter(t => t.pnl > 0);
    const losingTrades = trades.filter(t => t.pnl <= 0);
    
    return {
      summary: {
        totalReturn,
        annualizedReturn: totalReturn * (365 / 30), // Rough annualization
        volatility,
        sharpeRatio: totalReturn / volatility,
        sortinoRatio: (totalReturn / volatility) * 1.4, // Simplified
        calmarRatio: totalReturn / Math.abs(maxDrawdown),
        maxDrawdown,
        winRate,
        profitFactor: winningTrades.reduce((sum, t) => sum + t.pnl, 0) / 
                     Math.abs(losingTrades.reduce((sum, t) => sum + t.pnl, 0)),
        totalTrades,
        avgTrade: trades.reduce((sum, t) => sum + t.pnl, 0) / totalTrades,
        avgWinner: winningTrades.length > 0 ? 
                  winningTrades.reduce((sum, t) => sum + t.pnl, 0) / winningTrades.length : 0,
        avgLoser: losingTrades.length > 0 ? 
                 losingTrades.reduce((sum, t) => sum + t.pnl, 0) / losingTrades.length : 0,
        largestWin: Math.max(...trades.map(t => t.pnl)),
        largestLoss: Math.min(...trades.map(t => t.pnl)),
        avgHoldingTime: trades.reduce((sum, t) => sum + t.holdingTime, 0) / totalTrades,
      },
      equityCurve,
      trades,
      monthlyReturns: [], // Simplified for now
      drawdownHistory: [], // Simplified for now
      riskMetrics: {
        var95: -(config.initialCapital * 0.02), // 2% VaR
        var99: -(config.initialCapital * 0.05), // 5% VaR
        expectedShortfall: -(config.initialCapital * 0.07),
        beta: 0.8 + Math.random() * 0.6, // 0.8-1.4
        alpha: (Math.random() - 0.3) * 5, // -1.5% to 3.5%
        trackingError: 3 + Math.random() * 7, // 3-10%
      },
    };
  };

  const runBacktest = async () => {
    setIsRunning(true);
    setProgress(0);
    setActiveTab('results');

    // Simulate progress
    const progressInterval = setInterval(() => {
      setProgress(prev => {
        const newProgress = prev + Math.random() * 15;
        if (newProgress >= 100) {
          clearInterval(progressInterval);
          setIsRunning(false);
          setResults(generateBacktestResults());
          return 100;
        }
        return newProgress;
      });
    }, 200);
  };

  const runComparison = () => {
    const strategies = ['momentum_scalping', 'mean_reversion', 'sentiment_trading', 'grid_trading'];
    const comparisonData: StrategyComparison[] = strategies.map(strategy => ({
      strategy,
      return: (Math.random() - 0.3) * 40,
      sharpe: 0.5 + Math.random() * 2.5,
      maxDrawdown: -(2 + Math.random() * 18),
      winRate: 45 + Math.random() * 35,
      trades: 100 + Math.floor(Math.random() * 300),
      correlation: Math.random() * 0.8,
    }));

    setComparisons(comparisonData);
    setActiveTab('comparison');
  };

  const formatPercent = (value: number): string => {
    return `${value >= 0 ? '+' : ''}${value.toFixed(2)}%`;
  };

  const formatCurrency = (value: number): string => {
    return new Intl.NumberFormat('en-US', {
      style: 'currency',
      currency: 'USD',
      minimumFractionDigits: 0,
      maximumFractionDigits: 0,
    }).format(value);
  };

  const getPerformanceColor = (value: number): string => {
    return value >= 0 ? 'text-green-400' : 'text-red-400';
  };

  const renderConfiguration = () => (
    <div className="space-y-6">
      {/* Strategy Selection */}
      <Card className="p-6">
        <h3 className="text-lg font-semibold text-white mb-4">Strategy Configuration</h3>
        <div className="grid grid-cols-1 md:grid-cols-2 gap-4">
          <div>
            <label className="block text-sm text-gray-400 mb-2">Strategy</label>
            <Select
              value={config.strategy}
              onValueChange={(value) => setConfig(prev => ({ ...prev, strategy: value }))}
              options={AVAILABLE_STRATEGIES}
            />
          </div>
          
          <div>
            <label className="block text-sm text-gray-400 mb-2">Timeframe</label>
            <Select
              value={config.timeframe}
              onValueChange={(value) => setConfig(prev => ({ ...prev, timeframe: value }))}
              options={TIMEFRAMES}
            />
          </div>
        </div>
      </Card>

      {/* Date Range & Capital */}
      <Card className="p-6">
        <h3 className="text-lg font-semibold text-white mb-4">Backtest Parameters</h3>
        <div className="grid grid-cols-1 md:grid-cols-3 gap-4">
          <div>
            <label className="block text-sm text-gray-400 mb-2">Start Date</label>
            <Input
              type="date"
              value={config.startDate}
              onChange={(e) => setConfig(prev => ({ ...prev, startDate: e.target.value }))}
            />
          </div>
          
          <div>
            <label className="block text-sm text-gray-400 mb-2">End Date</label>
            <Input
              type="date"
              value={config.endDate}
              onChange={(e) => setConfig(prev => ({ ...prev, endDate: e.target.value }))}
            />
          </div>
          
          <div>
            <label className="block text-sm text-gray-400 mb-2">Initial Capital</label>
            <Input
              type="number"
              value={config.initialCapital}
              onChange={(e) => setConfig(prev => ({ ...prev, initialCapital: Number(e.target.value) }))}
            />
          </div>
        </div>
      </Card>

      {/* Symbol Selection */}
      <Card className="p-6">
        <h3 className="text-lg font-semibold text-white mb-4">Trading Symbols</h3>
        <div className="grid grid-cols-5 gap-2">
          {AVAILABLE_SYMBOLS.map(symbol => (
            <label key={symbol} className="flex items-center space-x-2">
              <input
                type="checkbox"
                checked={config.symbols.includes(symbol)}
                onChange={(e) => {
                  if (e.target.checked) {
                    setConfig(prev => ({ ...prev, symbols: [...prev.symbols, symbol] }));
                  } else {
                    setConfig(prev => ({ ...prev, symbols: prev.symbols.filter(s => s !== symbol) }));
                  }
                }}
                className="rounded border-gray-600 bg-gray-700 text-blue-600"
              />
              <span className="text-white">{symbol}</span>
            </label>
          ))}
        </div>
      </Card>

      {/* Advanced Settings */}
      <Card className="p-6">
        <h3 className="text-lg font-semibold text-white mb-4">Advanced Settings</h3>
        <div className="grid grid-cols-1 md:grid-cols-2 gap-4">
          <div>
            <label className="block text-sm text-gray-400 mb-2">Commission Rate (%)</label>
            <Input
              type="number"
              step="0.001"
              value={config.commissionRate}
              onChange={(e) => setConfig(prev => ({ ...prev, commissionRate: Number(e.target.value) }))}
            />
          </div>
          
          <div>
            <label className="block text-sm text-gray-400 mb-2">Slippage (bps)</label>
            <Input
              type="number"
              value={config.slippageBps}
              onChange={(e) => setConfig(prev => ({ ...prev, slippageBps: Number(e.target.value) }))}
            />
          </div>
        </div>
        
        <div className="flex items-center space-x-6 mt-4">
          <label className="flex items-center space-x-2">
            <input
              type="checkbox"
              checked={config.enableSentiment}
              onChange={(e) => setConfig(prev => ({ ...prev, enableSentiment: e.target.checked }))}
              className="rounded border-gray-600 bg-gray-700 text-blue-600"
            />
            <span className="text-white">Enable Sentiment Analysis</span>
          </label>
          
          <label className="flex items-center space-x-2">
            <input
              type="checkbox"
              checked={config.enableAI}
              onChange={(e) => setConfig(prev => ({ ...prev, enableAI: e.target.checked }))}
              className="rounded border-gray-600 bg-gray-700 text-blue-600"
            />
            <span className="text-white">Enable AI Decisions</span>
          </label>
        </div>
      </Card>

      {/* Action Buttons */}
      <div className="flex justify-center space-x-4">
        <Button onClick={runBacktest} disabled={isRunning}>
          {isRunning ? 'Running...' : 'Run Backtest'}
        </Button>
        <Button variant="outline" onClick={runComparison}>
          Compare Strategies
        </Button>
      </div>
    </div>
  );

  const renderResults = () => {
    if (isRunning) {
      return (
        <Card className="p-8 text-center">
          <div className="text-white text-lg mb-4">Running Backtest...</div>
          <div className="w-full bg-gray-700 rounded-full h-4 mb-4">
            <div 
              className="bg-blue-500 h-4 rounded-full transition-all duration-300"
              style={{ width: `${progress}%` }}
            />
          </div>
          <div className="text-gray-400">{progress.toFixed(1)}% complete</div>
        </Card>
      );
    }

    if (!results) {
      return (
        <Card className="p-8 text-center">
          <div className="text-gray-400">No backtest results available.</div>
          <Button className="mt-4" onClick={() => setActiveTab('configure')}>
            Configure Backtest
          </Button>
        </Card>
      );
    }

    return (
      <div className="space-y-6">
        {/* Summary Metrics */}
        <div className="grid grid-cols-2 md:grid-cols-4 gap-4">
          <Card className="p-4">
            <div className="text-sm text-gray-400">Total Return</div>
            <div className={`text-2xl font-bold ${getPerformanceColor(results.summary.totalReturn)}`}>
              {formatPercent(results.summary.totalReturn)}
            </div>
          </Card>

          <Card className="p-4">
            <div className="text-sm text-gray-400">Sharpe Ratio</div>
            <div className="text-2xl font-bold text-white">
              {results.summary.sharpeRatio.toFixed(2)}
            </div>
          </Card>

          <Card className="p-4">
            <div className="text-sm text-gray-400">Max Drawdown</div>
            <div className="text-2xl font-bold text-red-400">
              {formatPercent(results.summary.maxDrawdown)}
            </div>
          </Card>

          <Card className="p-4">
            <div className="text-sm text-gray-400">Win Rate</div>
            <div className="text-2xl font-bold text-green-400">
              {results.summary.winRate.toFixed(1)}%
            </div>
          </Card>
        </div>

        {/* Detailed Metrics */}
        <div className="grid grid-cols-1 lg:grid-cols-2 gap-6">
          <Card className="p-6">
            <h3 className="text-lg font-semibold text-white mb-4">Performance Metrics</h3>
            <div className="space-y-3">
              <div className="flex justify-between">
                <span className="text-gray-400">Annualized Return:</span>
                <span className={getPerformanceColor(results.summary.annualizedReturn)}>
                  {formatPercent(results.summary.annualizedReturn)}
                </span>
              </div>
              <div className="flex justify-between">
                <span className="text-gray-400">Volatility:</span>
                <span className="text-white">{formatPercent(results.summary.volatility)}</span>
              </div>
              <div className="flex justify-between">
                <span className="text-gray-400">Sortino Ratio:</span>
                <span className="text-white">{results.summary.sortinoRatio.toFixed(2)}</span>
              </div>
              <div className="flex justify-between">
                <span className="text-gray-400">Calmar Ratio:</span>
                <span className="text-white">{results.summary.calmarRatio.toFixed(2)}</span>
              </div>
              <div className="flex justify-between">
                <span className="text-gray-400">Profit Factor:</span>
                <span className="text-white">{results.summary.profitFactor.toFixed(2)}</span>
              </div>
            </div>
          </Card>

          <Card className="p-6">
            <h3 className="text-lg font-semibold text-white mb-4">Trade Analysis</h3>
            <div className="space-y-3">
              <div className="flex justify-between">
                <span className="text-gray-400">Total Trades:</span>
                <span className="text-white">{results.summary.totalTrades}</span>
              </div>
              <div className="flex justify-between">
                <span className="text-gray-400">Avg Trade:</span>
                <span className={getPerformanceColor(results.summary.avgTrade)}>
                  {formatCurrency(results.summary.avgTrade)}
                </span>
              </div>
              <div className="flex justify-between">
                <span className="text-gray-400">Avg Winner:</span>
                <span className="text-green-400">{formatCurrency(results.summary.avgWinner)}</span>
              </div>
              <div className="flex justify-between">
                <span className="text-gray-400">Avg Loser:</span>
                <span className="text-red-400">{formatCurrency(results.summary.avgLoser)}</span>
              </div>
              <div className="flex justify-between">
                <span className="text-gray-400">Avg Holding Time:</span>
                <span className="text-white">{results.summary.avgHoldingTime.toFixed(1)} min</span>
              </div>
            </div>
          </Card>
        </div>

        {/* Trades Table */}
        <Card className="p-6">
          <h3 className="text-lg font-semibold text-white mb-4">Trade History (Last 10)</h3>
          <div className="overflow-x-auto">
            <table className="w-full text-sm">
              <thead>
                <tr className="border-b border-gray-700">
                  <th className="text-left py-2 text-gray-400">Symbol</th>
                  <th className="text-left py-2 text-gray-400">Side</th>
                  <th className="text-right py-2 text-gray-400">Entry</th>
                  <th className="text-right py-2 text-gray-400">Exit</th>
                  <th className="text-right py-2 text-gray-400">P&L</th>
                  <th className="text-right py-2 text-gray-400">Return</th>
                  <th className="text-right py-2 text-gray-400">Time</th>
                </tr>
              </thead>
              <tbody>
                {results.trades.slice(-10).map(trade => (
                  <tr key={trade.id} className="border-b border-gray-800">
                    <td className="py-2 text-white">{trade.symbol}</td>
                    <td className="py-2">
                      <Badge variant={trade.side === 'BUY' ? 'success' : 'destructive'}>
                        {trade.side}
                      </Badge>
                    </td>
                    <td className="py-2 text-right text-white">${trade.entryPrice.toFixed(2)}</td>
                    <td className="py-2 text-right text-white">${trade.exitPrice.toFixed(2)}</td>
                    <td className={`py-2 text-right ${getPerformanceColor(trade.pnl)}`}>
                      {formatCurrency(trade.pnl)}
                    </td>
                    <td className={`py-2 text-right ${getPerformanceColor(trade.pnlPercent)}`}>
                      {formatPercent(trade.pnlPercent)}
                    </td>
                    <td className="py-2 text-right text-gray-400">
                      {trade.holdingTime.toFixed(0)}m
                    </td>
                  </tr>
                ))}
              </tbody>
            </table>
          </div>
        </Card>
      </div>
    );
  };

  const renderComparison = () => (
    <div className="space-y-6">
      <Card className="p-6">
        <h3 className="text-lg font-semibold text-white mb-4">Strategy Comparison</h3>
        <div className="overflow-x-auto">
          <table className="w-full text-sm">
            <thead>
              <tr className="border-b border-gray-700">
                <th className="text-left py-2 text-gray-400">Strategy</th>
                <th className="text-right py-2 text-gray-400">Return</th>
                <th className="text-right py-2 text-gray-400">Sharpe</th>
                <th className="text-right py-2 text-gray-400">Max DD</th>
                <th className="text-right py-2 text-gray-400">Win Rate</th>
                <th className="text-right py-2 text-gray-400">Trades</th>
                <th className="text-right py-2 text-gray-400">Correlation</th>
              </tr>
            </thead>
            <tbody>
              {comparisons.map(comp => (
                <tr key={comp.strategy} className="border-b border-gray-800">
                  <td className="py-2 text-white">
                    {AVAILABLE_STRATEGIES.find(s => s.value === comp.strategy)?.label || comp.strategy}
                  </td>
                  <td className={`py-2 text-right ${getPerformanceColor(comp.return)}`}>
                    {formatPercent(comp.return)}
                  </td>
                  <td className="py-2 text-right text-white">{comp.sharpe.toFixed(2)}</td>
                  <td className="py-2 text-right text-red-400">{formatPercent(comp.maxDrawdown)}</td>
                  <td className="py-2 text-right text-white">{comp.winRate.toFixed(1)}%</td>
                  <td className="py-2 text-right text-white">{comp.trades}</td>
                  <td className="py-2 text-right text-white">{comp.correlation.toFixed(3)}</td>
                </tr>
              ))}
            </tbody>
          </table>
        </div>
      </Card>
    </div>
  );

  const tabs = [
    { id: 'configure', label: 'Configure' },
    { id: 'results', label: 'Results' },
    { id: 'comparison', label: 'Comparison' },
  ];

  return (
    <div className="space-y-6">
      {/* Header */}
      <div className="flex justify-between items-center">
        <h2 className="text-2xl font-bold text-white">Strategy Backtesting</h2>
        <div className="flex space-x-2">
          <Button variant="outline">Load Template</Button>
          <Button variant="outline">Save Config</Button>
        </div>
      </div>

      {/* Tabs */}
      <Tabs value={activeTab} onValueChange={setActiveTab} tabs={tabs}>
        <div className="mt-6">
          {activeTab === 'configure' && renderConfiguration()}
          {activeTab === 'results' && renderResults()}
          {activeTab === 'comparison' && renderComparison()}
        </div>
      </Tabs>
    </div>
  );
};
