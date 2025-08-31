'use client';

import React, { useState, useEffect } from 'react';
import { Card } from '../ui/Card';
import { Button } from '../ui/Button';
import { Badge } from '../ui/Badge';
import { Switch } from '../ui/Switch';
import { Select } from '../ui/Select';
import { Tabs } from '../ui/Tabs';
import { Input } from '../ui/Input';

interface TradingStrategy {
  id: string;
  name: string;
  description: string;
  status: 'active' | 'paused' | 'disabled' | 'testing';
  category: 'arbitrage' | 'momentum' | 'mean_reversion' | 'sentiment' | 'mev' | 'grid' | 'custom';
  performance: {
    totalReturn: number;
    winRate: number;
    sharpeRatio: number;
    maxDrawdown: number;
    totalTrades: number;
    avgHoldingTime: number;
  };
  risk: {
    maxPositionSize: number;
    stopLoss: number;
    takeProfit: number;
    maxDailyLoss: number;
    correlation: number;
  };
  allocation: number;
  lastUpdate: number;
  backtestResults?: BacktestSummary;
  parameters: StrategyParameters;
}

interface StrategyParameters {
  [key: string]: {
    value: number | string | boolean;
    type: 'number' | 'string' | 'boolean' | 'select';
    min?: number;
    max?: number;
    options?: string[];
    description: string;
  };
}

interface BacktestSummary {
  period: string;
  totalReturn: number;
  sharpeRatio: number;
  maxDrawdown: number;
  winRate: number;
  totalTrades: number;
  confidence: number;
}

interface StrategyTemplate {
  id: string;
  name: string;
  description: string;
  category: string;
  complexity: 'Beginner' | 'Intermediate' | 'Advanced';
  defaultParameters: StrategyParameters;
  estimatedPerformance: {
    returnRange: [number, number];
    riskLevel: 'Low' | 'Medium' | 'High';
    timeframe: string;
  };
}

const STRATEGY_CATEGORIES = [
  { value: 'all', label: 'All Categories' },
  { value: 'arbitrage', label: 'Arbitrage' },
  { value: 'momentum', label: 'Momentum' },
  { value: 'mean_reversion', label: 'Mean Reversion' },
  { value: 'sentiment', label: 'Sentiment' },
  { value: 'mev', label: 'MEV' },
  { value: 'grid', label: 'Grid Trading' },
  { value: 'custom', label: 'Custom' },
];

const STRATEGY_TEMPLATES: StrategyTemplate[] = [
  {
    id: 'momentum_scalping',
    name: 'Momentum Scalping',
    description: 'High-frequency momentum-based scalping strategy',
    category: 'momentum',
    complexity: 'Advanced',
    defaultParameters: {
      timeframe: { value: '1m', type: 'select', options: ['1m', '5m', '15m'], description: 'Trading timeframe' },
      momentum_threshold: { value: 0.02, type: 'number', min: 0.01, max: 0.1, description: 'Momentum threshold %' },
      stop_loss: { value: 0.5, type: 'number', min: 0.1, max: 2.0, description: 'Stop loss %' },
      take_profit: { value: 1.0, type: 'number', min: 0.2, max: 5.0, description: 'Take profit %' },
      max_positions: { value: 5, type: 'number', min: 1, max: 10, description: 'Max concurrent positions' },
    },
    estimatedPerformance: {
      returnRange: [15, 35],
      riskLevel: 'High',
      timeframe: '1-5 minutes',
    },
  },
  {
    id: 'mean_reversion',
    name: 'Mean Reversion',
    description: 'Statistical arbitrage using mean reversion principles',
    category: 'mean_reversion',
    complexity: 'Intermediate',
    defaultParameters: {
      lookback_period: { value: 20, type: 'number', min: 5, max: 100, description: 'Lookback period' },
      deviation_threshold: { value: 2.0, type: 'number', min: 1.0, max: 4.0, description: 'Standard deviation threshold' },
      holding_period: { value: 60, type: 'number', min: 10, max: 300, description: 'Max holding period (minutes)' },
      risk_per_trade: { value: 1.0, type: 'number', min: 0.1, max: 5.0, description: 'Risk per trade %' },
    },
    estimatedPerformance: {
      returnRange: [8, 18],
      riskLevel: 'Medium',
      timeframe: '15-60 minutes',
    },
  },
  {
    id: 'sentiment_trading',
    name: 'AI Sentiment Trading',
    description: 'Trade based on real-time sentiment analysis',
    category: 'sentiment',
    complexity: 'Intermediate',
    defaultParameters: {
      sentiment_threshold: { value: 0.7, type: 'number', min: 0.5, max: 0.9, description: 'Sentiment confidence threshold' },
      sources: { value: 'twitter,reddit,news', type: 'string', description: 'Sentiment sources (comma-separated)' },
      min_volume: { value: 1000000, type: 'number', min: 100000, max: 10000000, description: 'Minimum volume filter' },
      position_size: { value: 2.0, type: 'number', min: 0.5, max: 10.0, description: 'Position size %' },
    },
    estimatedPerformance: {
      returnRange: [12, 25],
      riskLevel: 'Medium',
      timeframe: '5-30 minutes',
    },
  },
  {
    id: 'grid_trading',
    name: 'Dynamic Grid Trading',
    description: 'Automated grid trading with dynamic adjustments',
    category: 'grid',
    complexity: 'Beginner',
    defaultParameters: {
      grid_spacing: { value: 1.0, type: 'number', min: 0.1, max: 5.0, description: 'Grid spacing %' },
      num_grids: { value: 10, type: 'number', min: 3, max: 20, description: 'Number of grids' },
      base_order_size: { value: 100, type: 'number', min: 10, max: 1000, description: 'Base order size $' },
      profit_target: { value: 1.5, type: 'number', min: 0.5, max: 5.0, description: 'Profit target %' },
    },
    estimatedPerformance: {
      returnRange: [5, 15],
      riskLevel: 'Low',
      timeframe: 'Hours to days',
    },
  },
];

export const StrategyManager: React.FC = () => {
  const [activeTab, setActiveTab] = useState('active');
  const [selectedCategory, setSelectedCategory] = useState('all');
  const [strategies, setStrategies] = useState<TradingStrategy[]>([]);
  const [selectedStrategy, setSelectedStrategy] = useState<TradingStrategy | null>(null);
  const [isCreating, setIsCreating] = useState(false);
  const [newStrategyTemplate, setNewStrategyTemplate] = useState<StrategyTemplate | null>(null);

  // Generate mock strategies
  const generateMockStrategies = (): TradingStrategy[] => {
    const statuses: TradingStrategy['status'][] = ['active', 'paused', 'disabled', 'testing'];
    
    return STRATEGY_TEMPLATES.map((template, index) => ({
      id: `strategy_${index}`,
      name: template.name,
      description: template.description,
      status: statuses[Math.floor(Math.random() * statuses.length)],
      category: template.category as TradingStrategy['category'],
      performance: {
        totalReturn: (Math.random() - 0.3) * 50, // -15% to 35%
        winRate: 45 + Math.random() * 35, // 45-80%
        sharpeRatio: 0.5 + Math.random() * 2.5, // 0.5-3.0
        maxDrawdown: -(2 + Math.random() * 18), // -2% to -20%
        totalTrades: 100 + Math.floor(Math.random() * 500),
        avgHoldingTime: 5 + Math.random() * 120, // 5-125 minutes
      },
      risk: {
        maxPositionSize: 5 + Math.random() * 15, // 5-20%
        stopLoss: 0.5 + Math.random() * 3.5, // 0.5-4%
        takeProfit: 1 + Math.random() * 4, // 1-5%
        maxDailyLoss: 2 + Math.random() * 8, // 2-10%
        correlation: Math.random() * 0.6, // 0-0.6
      },
      allocation: 5 + Math.random() * 20, // 5-25%
      lastUpdate: Date.now() - Math.random() * 7 * 24 * 3600 * 1000, // Last week
      parameters: template.defaultParameters,
      backtestResults: {
        period: '30 days',
        totalReturn: (Math.random() - 0.2) * 40,
        sharpeRatio: 0.8 + Math.random() * 2.2,
        maxDrawdown: -(3 + Math.random() * 15),
        winRate: 50 + Math.random() * 30,
        totalTrades: 50 + Math.floor(Math.random() * 200),
        confidence: 0.7 + Math.random() * 0.3,
      },
    }));
  };

  useEffect(() => {
    setStrategies(generateMockStrategies());
  }, []);

  const filteredStrategies = strategies.filter(strategy => {
    const categoryMatch = selectedCategory === 'all' || strategy.category === selectedCategory;
    const statusMatch = activeTab === 'active' ? strategy.status === 'active' :
                       activeTab === 'paused' ? strategy.status === 'paused' :
                       activeTab === 'testing' ? strategy.status === 'testing' :
                       activeTab === 'templates' ? false : true;
    return categoryMatch && statusMatch;
  });

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

  const getStatusBadge = (status: string) => {
    switch (status) {
      case 'active':
        return <Badge variant="success">Active</Badge>;
      case 'paused':
        return <Badge variant="secondary">Paused</Badge>;
      case 'disabled':
        return <Badge variant="destructive">Disabled</Badge>;
      case 'testing':
        return <Badge className="bg-blue-600">Testing</Badge>;
      default:
        return <Badge variant="secondary">{status}</Badge>;
    }
  };

  const getPerformanceColor = (value: number): string => {
    return value >= 0 ? 'text-green-400' : 'text-red-400';
  };

  const getRiskLevelColor = (level: string): string => {
    switch (level) {
      case 'Low': return 'text-green-400';
      case 'Medium': return 'text-yellow-400';
      case 'High': return 'text-red-400';
      default: return 'text-gray-400';
    }
  };

  const toggleStrategyStatus = (strategyId: string) => {
    setStrategies(prev => prev.map(strategy => 
      strategy.id === strategyId 
        ? { ...strategy, status: strategy.status === 'active' ? 'paused' : 'active' }
        : strategy
    ));
  };

  const createStrategyFromTemplate = (template: StrategyTemplate) => {
    const newStrategy: TradingStrategy = {
      id: `strategy_${Date.now()}`,
      name: `${template.name} - Custom`,
      description: template.description,
      status: 'testing',
      category: template.category as TradingStrategy['category'],
      performance: {
        totalReturn: 0,
        winRate: 0,
        sharpeRatio: 0,
        maxDrawdown: 0,
        totalTrades: 0,
        avgHoldingTime: 0,
      },
      risk: {
        maxPositionSize: 10,
        stopLoss: 2,
        takeProfit: 3,
        maxDailyLoss: 5,
        correlation: 0,
      },
      allocation: 5,
      lastUpdate: Date.now(),
      parameters: { ...template.defaultParameters },
    };

    setStrategies(prev => [...prev, newStrategy]);
    setSelectedStrategy(newStrategy);
    setIsCreating(false);
    setNewStrategyTemplate(null);
  };

  const renderStrategyCard = (strategy: TradingStrategy) => (
    <Card key={strategy.id} className="p-4 cursor-pointer hover:bg-gray-800 transition-colors">
      <div className="flex justify-between items-start mb-3">
        <div>
          <h4 className="font-semibold text-white">{strategy.name}</h4>
          <p className="text-sm text-gray-400 mt-1">{strategy.description}</p>
        </div>
        <div className="flex items-center space-x-2">
          {getStatusBadge(strategy.status)}
          <Switch
            checked={strategy.status === 'active'}
            onCheckedChange={() => toggleStrategyStatus(strategy.id)}
          />
        </div>
      </div>

      <div className="grid grid-cols-2 md:grid-cols-4 gap-3 mb-3">
        <div>
          <div className="text-xs text-gray-400">Total Return</div>
          <div className={`font-semibold ${getPerformanceColor(strategy.performance.totalReturn)}`}>
            {formatPercent(strategy.performance.totalReturn)}
          </div>
        </div>
        <div>
          <div className="text-xs text-gray-400">Win Rate</div>
          <div className="text-white font-semibold">
            {strategy.performance.winRate.toFixed(1)}%
          </div>
        </div>
        <div>
          <div className="text-xs text-gray-400">Sharpe Ratio</div>
          <div className="text-white font-semibold">
            {strategy.performance.sharpeRatio.toFixed(2)}
          </div>
        </div>
        <div>
          <div className="text-xs text-gray-400">Max Drawdown</div>
          <div className="text-red-400 font-semibold">
            {formatPercent(strategy.performance.maxDrawdown)}
          </div>
        </div>
      </div>

      <div className="flex justify-between items-center">
        <div className="text-sm text-gray-400">
          Allocation: {strategy.allocation.toFixed(1)}% • {strategy.performance.totalTrades} trades
        </div>
        <Button
          size="sm"
          variant="outline"
          onClick={() => setSelectedStrategy(strategy)}
        >
          Configure
        </Button>
      </div>
    </Card>
  );

  const renderTemplateCard = (template: StrategyTemplate) => (
    <Card key={template.id} className="p-4">
      <div className="flex justify-between items-start mb-3">
        <div>
          <h4 className="font-semibold text-white">{template.name}</h4>
          <p className="text-sm text-gray-400 mt-1">{template.description}</p>
        </div>
        <Badge 
          variant={template.complexity === 'Beginner' ? 'success' : 
                  template.complexity === 'Intermediate' ? 'secondary' : 'destructive'}
        >
          {template.complexity}
        </Badge>
      </div>

      <div className="grid grid-cols-3 gap-3 mb-3">
        <div>
          <div className="text-xs text-gray-400">Est. Return</div>
          <div className="text-green-400 font-semibold">
            {template.estimatedPerformance.returnRange[0]}-{template.estimatedPerformance.returnRange[1]}%
          </div>
        </div>
        <div>
          <div className="text-xs text-gray-400">Risk Level</div>
          <div className={`font-semibold ${getRiskLevelColor(template.estimatedPerformance.riskLevel)}`}>
            {template.estimatedPerformance.riskLevel}
          </div>
        </div>
        <div>
          <div className="text-xs text-gray-400">Timeframe</div>
          <div className="text-white font-semibold">
            {template.estimatedPerformance.timeframe}
          </div>
        </div>
      </div>

      <div className="flex justify-between items-center">
        <div className="text-sm text-gray-400">
          Category: {template.category}
        </div>
        <div className="flex space-x-2">
          <Button size="sm" variant="outline">Preview</Button>
          <Button 
            size="sm"
            onClick={() => createStrategyFromTemplate(template)}
          >
            Create
          </Button>
        </div>
      </div>
    </Card>
  );

  const tabs = [
    { id: 'active', label: `Active (${strategies.filter(s => s.status === 'active').length})` },
    { id: 'paused', label: `Paused (${strategies.filter(s => s.status === 'paused').length})` },
    { id: 'testing', label: `Testing (${strategies.filter(s => s.status === 'testing').length})` },
    { id: 'templates', label: 'Templates' },
  ];

  return (
    <div className="space-y-6">
      {/* Header */}
      <div className="flex justify-between items-center">
        <h2 className="text-2xl font-bold text-white">Strategy Manager</h2>
        <div className="flex items-center space-x-4">
          <Select
            value={selectedCategory}
            onValueChange={setSelectedCategory}
            options={STRATEGY_CATEGORIES}
          />
          <Button onClick={() => setIsCreating(true)}>+ New Strategy</Button>
        </div>
      </div>

      {/* Portfolio Overview */}
      <div className="grid grid-cols-2 md:grid-cols-4 gap-4">
        <Card className="p-4">
          <div className="text-sm text-gray-400">Total Strategies</div>
          <div className="text-2xl font-bold text-white">{strategies.length}</div>
        </Card>

        <Card className="p-4">
          <div className="text-sm text-gray-400">Active Strategies</div>
          <div className="text-2xl font-bold text-green-400">
            {strategies.filter(s => s.status === 'active').length}
          </div>
        </Card>

        <Card className="p-4">
          <div className="text-sm text-gray-400">Total Allocation</div>
          <div className="text-2xl font-bold text-white">
            {strategies.reduce((sum, s) => sum + s.allocation, 0).toFixed(1)}%
          </div>
        </Card>

        <Card className="p-4">
          <div className="text-sm text-gray-400">Avg Performance</div>
          <div className={`text-2xl font-bold ${getPerformanceColor(
            strategies.reduce((sum, s) => sum + s.performance.totalReturn, 0) / strategies.length
          )}`}>
            {formatPercent(strategies.reduce((sum, s) => sum + s.performance.totalReturn, 0) / strategies.length)}
          </div>
        </Card>
      </div>

      {/* Strategy Lists */}
      <Tabs value={activeTab} onValueChange={setActiveTab} tabs={tabs}>
        <div className="mt-6">
          {activeTab === 'templates' ? (
            <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-4">
              {STRATEGY_TEMPLATES.map(renderTemplateCard)}
            </div>
          ) : (
            <div className="space-y-4">
              {filteredStrategies.length > 0 ? (
                filteredStrategies.map(renderStrategyCard)
              ) : (
                <Card className="p-8 text-center">
                  <div className="text-gray-400">
                    No strategies found in this category.
                  </div>
                  <Button 
                    className="mt-4"
                    onClick={() => setActiveTab('templates')}
                  >
                    Browse Templates
                  </Button>
                </Card>
              )}
            </div>
          )}
        </div>
      </Tabs>

      {/* Strategy Configuration Modal */}
      {selectedStrategy && (
        <div className="fixed inset-0 bg-black bg-opacity-50 flex items-center justify-center p-4 z-50">
          <Card className="w-full max-w-4xl max-h-[90vh] overflow-y-auto p-6">
            <div className="flex justify-between items-center mb-6">
              <h3 className="text-xl font-bold text-white">{selectedStrategy.name}</h3>
              <Button variant="outline" onClick={() => setSelectedStrategy(null)}>
                Close
              </Button>
            </div>

            <div className="grid grid-cols-1 lg:grid-cols-2 gap-6">
              {/* Performance Metrics */}
              <div>
                <h4 className="text-lg font-semibold text-white mb-4">Performance</h4>
                <div className="space-y-3">
                  <div className="flex justify-between">
                    <span className="text-gray-400">Total Return:</span>
                    <span className={getPerformanceColor(selectedStrategy.performance.totalReturn)}>
                      {formatPercent(selectedStrategy.performance.totalReturn)}
                    </span>
                  </div>
                  <div className="flex justify-between">
                    <span className="text-gray-400">Win Rate:</span>
                    <span className="text-white">{selectedStrategy.performance.winRate.toFixed(1)}%</span>
                  </div>
                  <div className="flex justify-between">
                    <span className="text-gray-400">Sharpe Ratio:</span>
                    <span className="text-white">{selectedStrategy.performance.sharpeRatio.toFixed(2)}</span>
                  </div>
                  <div className="flex justify-between">
                    <span className="text-gray-400">Max Drawdown:</span>
                    <span className="text-red-400">{formatPercent(selectedStrategy.performance.maxDrawdown)}</span>
                  </div>
                </div>
              </div>

              {/* Risk Parameters */}
              <div>
                <h4 className="text-lg font-semibold text-white mb-4">Risk Management</h4>
                <div className="space-y-3">
                  <div className="flex justify-between">
                    <span className="text-gray-400">Max Position Size:</span>
                    <span className="text-white">{selectedStrategy.risk.maxPositionSize.toFixed(1)}%</span>
                  </div>
                  <div className="flex justify-between">
                    <span className="text-gray-400">Stop Loss:</span>
                    <span className="text-white">{selectedStrategy.risk.stopLoss.toFixed(1)}%</span>
                  </div>
                  <div className="flex justify-between">
                    <span className="text-gray-400">Take Profit:</span>
                    <span className="text-white">{selectedStrategy.risk.takeProfit.toFixed(1)}%</span>
                  </div>
                  <div className="flex justify-between">
                    <span className="text-gray-400">Max Daily Loss:</span>
                    <span className="text-white">{selectedStrategy.risk.maxDailyLoss.toFixed(1)}%</span>
                  </div>
                </div>
              </div>
            </div>

            {/* Strategy Parameters */}
            <div className="mt-6">
              <h4 className="text-lg font-semibold text-white mb-4">Parameters</h4>
              <div className="grid grid-cols-1 md:grid-cols-2 gap-4">
                {Object.entries(selectedStrategy.parameters).map(([key, param]) => (
                  <div key={key}>
                    <label className="block text-sm text-gray-400 mb-1">
                      {key.replace(/_/g, ' ').replace(/\b\w/g, l => l.toUpperCase())}
                    </label>
                    {param.type === 'number' ? (
                      <Input
                        type="number"
                        value={param.value as number}
                        min={param.min}
                        max={param.max}
                        className="w-full"
                      />
                    ) : param.type === 'select' ? (
                      <Select
                        value={param.value as string}
                        onValueChange={() => {}}
                        options={param.options?.map(opt => ({ value: opt, label: opt })) || []}
                      />
                    ) : (
                      <Input
                        type="text"
                        value={param.value as string}
                        className="w-full"
                      />
                    )}
                    <div className="text-xs text-gray-500 mt-1">{param.description}</div>
                  </div>
                ))}
              </div>
            </div>

            {/* Action Buttons */}
            <div className="flex justify-end space-x-4 mt-6">
              <Button variant="outline">Run Backtest</Button>
              <Button variant="outline">Clone Strategy</Button>
              <Button>Save Changes</Button>
            </div>
          </Card>
        </div>
      )}
    </div>
  );
};
