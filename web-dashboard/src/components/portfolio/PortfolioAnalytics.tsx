'use client';

import React, { useState, useEffect } from 'react';
import { Card } from '../ui/Card';
import { Button } from '../ui/Button';
import { Badge } from '../ui/Badge';
import { Select } from '../ui/Select';
import { Tabs } from '../ui/Tabs';

interface PortfolioHolding {
  symbol: string;
  quantity: number;
  avgPrice: number;
  currentPrice: number;
  marketValue: number;
  unrealizedPnL: number;
  unrealizedPnLPercent: number;
  allocation: number;
  dayChange: number;
  dayChangePercent: number;
  cost: number;
  lastTrade: string;
}

interface PortfolioDiversification {
  byAsset: { symbol: string; allocation: number; value: number }[];
  bySector: { sector: string; allocation: number; value: number }[];
  byMarketCap: { category: string; allocation: number; value: number }[];
  byRisk: { level: string; allocation: number; value: number }[];
}

interface PortfolioPerformance {
  totalValue: number;
  totalCost: number;
  totalPnL: number;
  totalPnLPercent: number;
  dayChange: number;
  dayChangePercent: number;
  weekChange: number;
  monthChange: number;
  yearChange: number;
  allTimeHigh: number;
  allTimeLow: number;
  maxDrawdown: number;
  sharpeRatio: number;
  volatility: number;
  beta: number;
  alpha: number;
}

interface AssetAllocation {
  target: number;
  current: number;
  difference: number;
  rebalanceAmount: number;
  symbol: string;
}

interface RiskMetrics {
  portfolioVar: number;
  portfolioVarPercent: number;
  concentrationRisk: number;
  liquidityRisk: number;
  correlationRisk: number;
  volatilityRisk: number;
  leverageRatio: number;
  cashPosition: number;
}

const TIME_PERIODS = [
  { value: '1d', label: '1 Day' },
  { value: '1w', label: '1 Week' },
  { value: '1m', label: '1 Month' },
  { value: '3m', label: '3 Months' },
  { value: '1y', label: '1 Year' },
  { value: 'all', label: 'All Time' },
];

const REBALANCE_THRESHOLDS = [
  { value: '5', label: '5%' },
  { value: '10', label: '10%' },
  { value: '15', label: '15%' },
  { value: '20', label: '20%' },
];

export const PortfolioAnalytics: React.FC = () => {
  const [selectedPeriod, setSelectedPeriod] = useState('1m');
  const [rebalanceThreshold, setRebalanceThreshold] = useState('10');
  const [activeTab, setActiveTab] = useState('overview');
  const [holdings, setHoldings] = useState<PortfolioHolding[]>([]);
  const [performance, setPerformance] = useState<PortfolioPerformance | null>(null);
  const [diversification, setDiversification] = useState<PortfolioDiversification | null>(null);
  const [allocations, setAllocations] = useState<AssetAllocation[]>([]);
  const [riskMetrics, setRiskMetrics] = useState<RiskMetrics | null>(null);

  // Generate mock portfolio data
  const generatePortfolioData = () => {
    // Generate holdings
    const symbols = ['BTC', 'ETH', 'SOL', 'MATIC', 'AVAX', 'ADA', 'DOT', 'ATOM'];
    const mockHoldings: PortfolioHolding[] = symbols.map(symbol => {
      const quantity = 0.1 + Math.random() * 10;
      const avgPrice = symbol === 'BTC' ? 40000 + Math.random() * 5000 :
                      symbol === 'ETH' ? 2000 + Math.random() * 1000 :
                      10 + Math.random() * 100;
      const currentPrice = avgPrice * (0.9 + Math.random() * 0.4); // -10% to +30%
      const marketValue = quantity * currentPrice;
      const cost = quantity * avgPrice;
      const unrealizedPnL = marketValue - cost;
      const dayChange = (Math.random() - 0.5) * marketValue * 0.1; // ±5% day change
      
      return {
        symbol,
        quantity,
        avgPrice,
        currentPrice,
        marketValue,
        unrealizedPnL,
        unrealizedPnLPercent: (unrealizedPnL / cost) * 100,
        allocation: 0, // Will be calculated
        dayChange,
        dayChangePercent: (dayChange / marketValue) * 100,
        cost,
        lastTrade: new Date(Date.now() - Math.random() * 7 * 24 * 3600 * 1000).toISOString(),
      };
    });

    // Calculate allocations
    const totalValue = mockHoldings.reduce((sum, h) => sum + h.marketValue, 0);
    mockHoldings.forEach(holding => {
      holding.allocation = (holding.marketValue / totalValue) * 100;
    });

    setHoldings(mockHoldings);

    // Generate performance metrics
    const totalCost = mockHoldings.reduce((sum, h) => sum + h.cost, 0);
    const totalPnL = totalValue - totalCost;
    
    setPerformance({
      totalValue,
      totalCost,
      totalPnL,
      totalPnLPercent: (totalPnL / totalCost) * 100,
      dayChange: mockHoldings.reduce((sum, h) => sum + h.dayChange, 0),
      dayChangePercent: (mockHoldings.reduce((sum, h) => sum + h.dayChange, 0) / totalValue) * 100,
      weekChange: (Math.random() - 0.3) * 15, // -4.5% to 10.5%
      monthChange: (Math.random() - 0.2) * 25, // -5% to 20%
      yearChange: (Math.random() - 0.1) * 80, // -8% to 72%
      allTimeHigh: totalValue * (1.2 + Math.random() * 0.5), // 20-70% higher
      allTimeLow: totalValue * (0.3 + Math.random() * 0.4), // 30-70% lower
      maxDrawdown: -(5 + Math.random() * 25), // -5% to -30%
      sharpeRatio: 0.8 + Math.random() * 2.2, // 0.8 to 3.0
      volatility: 15 + Math.random() * 35, // 15% to 50%
      beta: 0.7 + Math.random() * 0.8, // 0.7 to 1.5
      alpha: (Math.random() - 0.3) * 10, // -3% to 7%
    });

    // Generate diversification data
    setDiversification({
      byAsset: mockHoldings.map(h => ({
        symbol: h.symbol,
        allocation: h.allocation,
        value: h.marketValue,
      })),
      bySector: [
        { sector: 'Layer 1', allocation: 60, value: totalValue * 0.6 },
        { sector: 'DeFi', allocation: 25, value: totalValue * 0.25 },
        { sector: 'Layer 2', allocation: 10, value: totalValue * 0.1 },
        { sector: 'Others', allocation: 5, value: totalValue * 0.05 },
      ],
      byMarketCap: [
        { category: 'Large Cap', allocation: 70, value: totalValue * 0.7 },
        { category: 'Mid Cap', allocation: 20, value: totalValue * 0.2 },
        { category: 'Small Cap', allocation: 10, value: totalValue * 0.1 },
      ],
      byRisk: [
        { level: 'Low Risk', allocation: 30, value: totalValue * 0.3 },
        { level: 'Medium Risk', allocation: 50, value: totalValue * 0.5 },
        { level: 'High Risk', allocation: 20, value: totalValue * 0.2 },
      ],
    });

    // Generate allocation recommendations
    const targetAllocations = [35, 25, 15, 10, 8, 4, 2, 1]; // Target percentages
    const mockAllocations: AssetAllocation[] = mockHoldings.map((holding, index) => {
      const target = targetAllocations[index] || 1;
      const difference = holding.allocation - target;
      const rebalanceAmount = (difference / 100) * totalValue;
      
      return {
        symbol: holding.symbol,
        target,
        current: holding.allocation,
        difference,
        rebalanceAmount,
      };
    });

    setAllocations(mockAllocations);

    // Generate risk metrics
    setRiskMetrics({
      portfolioVar: -(totalValue * 0.05), // 5% VaR
      portfolioVarPercent: -5,
      concentrationRisk: Math.max(...mockHoldings.map(h => h.allocation)), // Largest allocation
      liquidityRisk: 15 + Math.random() * 25, // 15-40%
      correlationRisk: 0.6 + Math.random() * 0.3, // 0.6-0.9
      volatilityRisk: 20 + Math.random() * 30, // 20-50%
      leverageRatio: Math.random() * 2, // 0-2x
      cashPosition: Math.random() * 20, // 0-20%
    });
  };

  useEffect(() => {
    generatePortfolioData();
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

  const renderOverview = () => (
    <div className="space-y-6">
      {/* Portfolio Summary */}
      <div className="grid grid-cols-2 md:grid-cols-4 gap-4">
        <Card className="p-4">
          <div className="text-sm text-gray-400">Total Value</div>
          <div className="text-2xl font-bold text-white">
            {formatCurrency(performance?.totalValue || 0)}
          </div>
          <div className={`text-sm ${getPerformanceColor(performance?.dayChangePercent || 0)}`}>
            {formatPercent(performance?.dayChangePercent || 0)} today
          </div>
        </Card>

        <Card className="p-4">
          <div className="text-sm text-gray-400">Total P&L</div>
          <div className={`text-2xl font-bold ${getPerformanceColor(performance?.totalPnL || 0)}`}>
            {formatCurrency(performance?.totalPnL || 0)}
          </div>
          <div className={`text-sm ${getPerformanceColor(performance?.totalPnLPercent || 0)}`}>
            {formatPercent(performance?.totalPnLPercent || 0)}
          </div>
        </Card>

        <Card className="p-4">
          <div className="text-sm text-gray-400">Largest Holding</div>
          <div className="text-2xl font-bold text-white">
            {holdings.length > 0 ? holdings[0].symbol : 'N/A'}
          </div>
          <div className="text-sm text-gray-400">
            {holdings.length > 0 ? `${holdings[0].allocation.toFixed(1)}%` : '0%'}
          </div>
        </Card>

        <Card className="p-4">
          <div className="text-sm text-gray-400">Sharpe Ratio</div>
          <div className="text-2xl font-bold text-white">
            {performance?.sharpeRatio.toFixed(2) || '0.00'}
          </div>
          <div className="text-sm text-gray-400">Risk-adjusted return</div>
        </Card>
      </div>

      {/* Holdings Table */}
      <Card className="p-6">
        <h3 className="text-lg font-semibold text-white mb-4">Holdings</h3>
        <div className="overflow-x-auto">
          <table className="w-full text-sm">
            <thead>
              <tr className="border-b border-gray-700">
                <th className="text-left py-2 text-gray-400">Asset</th>
                <th className="text-right py-2 text-gray-400">Quantity</th>
                <th className="text-right py-2 text-gray-400">Avg Price</th>
                <th className="text-right py-2 text-gray-400">Current Price</th>
                <th className="text-right py-2 text-gray-400">Market Value</th>
                <th className="text-right py-2 text-gray-400">P&L</th>
                <th className="text-right py-2 text-gray-400">Allocation</th>
                <th className="text-right py-2 text-gray-400">Day Change</th>
              </tr>
            </thead>
            <tbody>
              {holdings.map(holding => (
                <tr key={holding.symbol} className="border-b border-gray-800">
                  <td className="py-2">
                    <div className="font-semibold text-white">{holding.symbol}</div>
                  </td>
                  <td className="py-2 text-right text-white">
                    {holding.quantity.toFixed(4)}
                  </td>
                  <td className="py-2 text-right text-white">
                    {formatCurrency(holding.avgPrice)}
                  </td>
                  <td className="py-2 text-right text-white">
                    {formatCurrency(holding.currentPrice)}
                  </td>
                  <td className="py-2 text-right text-white">
                    {formatCurrency(holding.marketValue)}
                  </td>
                  <td className={`py-2 text-right ${getPerformanceColor(holding.unrealizedPnL)}`}>
                    <div>{formatCurrency(holding.unrealizedPnL)}</div>
                    <div className="text-xs">
                      {formatPercent(holding.unrealizedPnLPercent)}
                    </div>
                  </td>
                  <td className="py-2 text-right text-white">
                    {holding.allocation.toFixed(1)}%
                  </td>
                  <td className={`py-2 text-right ${getPerformanceColor(holding.dayChange)}`}>
                    <div>{formatCurrency(holding.dayChange)}</div>
                    <div className="text-xs">
                      {formatPercent(holding.dayChangePercent)}
                    </div>
                  </td>
                </tr>
              ))}
            </tbody>
          </table>
        </div>
      </Card>

      {/* Performance Summary */}
      <div className="grid grid-cols-1 lg:grid-cols-2 gap-6">
        <Card className="p-6">
          <h3 className="text-lg font-semibold text-white mb-4">Performance Summary</h3>
          <div className="space-y-3">
            <div className="flex justify-between">
              <span className="text-gray-400">1 Week:</span>
              <span className={getPerformanceColor(performance?.weekChange || 0)}>
                {formatPercent(performance?.weekChange || 0)}
              </span>
            </div>
            <div className="flex justify-between">
              <span className="text-gray-400">1 Month:</span>
              <span className={getPerformanceColor(performance?.monthChange || 0)}>
                {formatPercent(performance?.monthChange || 0)}
              </span>
            </div>
            <div className="flex justify-between">
              <span className="text-gray-400">1 Year:</span>
              <span className={getPerformanceColor(performance?.yearChange || 0)}>
                {formatPercent(performance?.yearChange || 0)}
              </span>
            </div>
            <div className="flex justify-between">
              <span className="text-gray-400">All Time High:</span>
              <span className="text-white">{formatCurrency(performance?.allTimeHigh || 0)}</span>
            </div>
            <div className="flex justify-between">
              <span className="text-gray-400">Max Drawdown:</span>
              <span className="text-red-400">{formatPercent(performance?.maxDrawdown || 0)}</span>
            </div>
          </div>
        </Card>

        <Card className="p-6">
          <h3 className="text-lg font-semibold text-white mb-4">Risk Metrics</h3>
          <div className="space-y-3">
            <div className="flex justify-between">
              <span className="text-gray-400">Volatility:</span>
              <span className="text-white">{formatPercent(performance?.volatility || 0)}</span>
            </div>
            <div className="flex justify-between">
              <span className="text-gray-400">Beta:</span>
              <span className="text-white">{performance?.beta.toFixed(2) || '0.00'}</span>
            </div>
            <div className="flex justify-between">
              <span className="text-gray-400">Alpha:</span>
              <span className={getPerformanceColor(performance?.alpha || 0)}>
                {formatPercent(performance?.alpha || 0)}
              </span>
            </div>
            <div className="flex justify-between">
              <span className="text-gray-400">Portfolio VaR:</span>
              <span className="text-red-400">{formatCurrency(riskMetrics?.portfolioVar || 0)}</span>
            </div>
            <div className="flex justify-between">
              <span className="text-gray-400">Concentration Risk:</span>
              <span className="text-yellow-400">{riskMetrics?.concentrationRisk.toFixed(1)}%</span>
            </div>
          </div>
        </Card>
      </div>
    </div>
  );

  const renderDiversification = () => (
    <div className="space-y-6">
      <div className="grid grid-cols-1 lg:grid-cols-2 gap-6">
        {/* By Sector */}
        <Card className="p-6">
          <h3 className="text-lg font-semibold text-white mb-4">Diversification by Sector</h3>
          <div className="space-y-3">
            {diversification?.bySector.map(sector => (
              <div key={sector.sector} className="flex items-center justify-between">
                <div className="flex items-center space-x-3">
                  <div className="text-white">{sector.sector}</div>
                </div>
                <div className="text-right">
                  <div className="text-white">{sector.allocation.toFixed(1)}%</div>
                  <div className="text-sm text-gray-400">{formatCurrency(sector.value)}</div>
                </div>
              </div>
            ))}
          </div>
        </Card>

        {/* By Market Cap */}
        <Card className="p-6">
          <h3 className="text-lg font-semibold text-white mb-4">Diversification by Market Cap</h3>
          <div className="space-y-3">
            {diversification?.byMarketCap.map(cap => (
              <div key={cap.category} className="flex items-center justify-between">
                <div className="flex items-center space-x-3">
                  <div className="text-white">{cap.category}</div>
                </div>
                <div className="text-right">
                  <div className="text-white">{cap.allocation.toFixed(1)}%</div>
                  <div className="text-sm text-gray-400">{formatCurrency(cap.value)}</div>
                </div>
              </div>
            ))}
          </div>
        </Card>
      </div>

      {/* Asset Allocation Chart Placeholder */}
      <Card className="p-6">
        <h3 className="text-lg font-semibold text-white mb-4">Asset Allocation</h3>
        <div className="space-y-4">
          {diversification?.byAsset.map(asset => (
            <div key={asset.symbol} className="space-y-2">
              <div className="flex justify-between text-sm">
                <span className="text-white">{asset.symbol}</span>
                <span className="text-gray-400">{asset.allocation.toFixed(1)}%</span>
              </div>
              <div className="w-full bg-gray-700 rounded-full h-2">
                <div
                  className="bg-blue-500 h-2 rounded-full"
                  style={{ width: `${asset.allocation}%` }}
                />
              </div>
            </div>
          ))}
        </div>
      </Card>
    </div>
  );

  const renderRebalancing = () => (
    <div className="space-y-6">
      <Card className="p-6">
        <div className="flex justify-between items-center mb-4">
          <h3 className="text-lg font-semibold text-white">Rebalancing Recommendations</h3>
          <Select
            value={rebalanceThreshold}
            onValueChange={setRebalanceThreshold}
            options={REBALANCE_THRESHOLDS}
          />
        </div>

        <div className="space-y-4">
          {allocations.filter(alloc => Math.abs(alloc.difference) > Number(rebalanceThreshold)).map(allocation => (
            <div key={allocation.symbol} className="p-4 bg-gray-800 rounded-lg">
              <div className="flex items-center justify-between mb-2">
                <div className="font-semibold text-white">{allocation.symbol}</div>
                <Badge variant={allocation.difference > 0 ? 'destructive' : 'success'}>
                  {allocation.difference > 0 ? 'Overweight' : 'Underweight'}
                </Badge>
              </div>
              
              <div className="grid grid-cols-3 gap-4 text-sm">
                <div>
                  <div className="text-gray-400">Target:</div>
                  <div className="text-white">{allocation.target.toFixed(1)}%</div>
                </div>
                <div>
                  <div className="text-gray-400">Current:</div>
                  <div className="text-white">{allocation.current.toFixed(1)}%</div>
                </div>
                <div>
                  <div className="text-gray-400">Action:</div>
                  <div className={allocation.rebalanceAmount > 0 ? 'text-red-400' : 'text-green-400'}>
                    {allocation.rebalanceAmount > 0 ? 'Sell' : 'Buy'} {formatCurrency(Math.abs(allocation.rebalanceAmount))}
                  </div>
                </div>
              </div>
            </div>
          ))}

          {allocations.filter(alloc => Math.abs(alloc.difference) > Number(rebalanceThreshold)).length === 0 && (
            <div className="text-center py-8 text-gray-400">
              No rebalancing needed at {rebalanceThreshold}% threshold
            </div>
          )}
        </div>

        <div className="flex justify-center mt-6">
          <Button>Execute Rebalancing</Button>
        </div>
      </Card>
    </div>
  );

  const tabs = [
    { id: 'overview', label: 'Overview' },
    { id: 'diversification', label: 'Diversification' },
    { id: 'rebalancing', label: 'Rebalancing' },
  ];

  return (
    <div className="space-y-6">
      {/* Header */}
      <div className="flex justify-between items-center">
        <h2 className="text-2xl font-bold text-white">Portfolio Analytics</h2>
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
          {activeTab === 'diversification' && renderDiversification()}
          {activeTab === 'rebalancing' && renderRebalancing()}
        </div>
      </Tabs>
    </div>
  );
};
