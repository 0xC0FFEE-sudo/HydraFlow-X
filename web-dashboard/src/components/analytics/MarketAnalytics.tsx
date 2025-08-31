'use client';

import React, { useState, useEffect } from 'react';
import { Card } from '../ui/Card';
import { Select } from '../ui/Select';
import { Button } from '../ui/Button';
import { Badge } from '../ui/Badge';

interface MarketOverview {
  totalMarketCap: number;
  totalVolume24h: number;
  btcDominance: number;
  fearGreedIndex: number;
  activeCrypto: number;
  exchanges: number;
}

interface AssetMetrics {
  symbol: string;
  price: number;
  change24h: number;
  volume24h: number;
  marketCap: number;
  rsi: number;
  volatility: number;
  correlation: number;
  momentum: number;
  sentiment: number;
  rank: number;
}

interface SectorPerformance {
  sector: string;
  performance24h: number;
  performance7d: number;
  performance30d: number;
  marketCap: number;
  dominance: number;
  topAssets: string[];
}

interface CorrelationData {
  asset1: string;
  asset2: string;
  correlation: number;
  pValue: number;
  relationship: 'Strong' | 'Moderate' | 'Weak' | 'None';
}

interface OpportunityAlert {
  type: 'arbitrage' | 'breakout' | 'divergence' | 'unusual_volume';
  symbol: string;
  message: string;
  confidence: number;
  timeframe: string;
  potentialReturn: number;
  risk: 'Low' | 'Medium' | 'High';
}

const MARKET_SECTORS = [
  'DeFi', 'Layer 1', 'Layer 2', 'Memecoins', 'AI', 'Gaming', 'NFT', 'Privacy'
];

const CORRELATION_TIMEFRAMES = [
  { value: '1d', label: '24 Hours' },
  { value: '7d', label: '7 Days' },
  { value: '30d', label: '30 Days' },
  { value: '90d', label: '90 Days' },
];

export const MarketAnalytics: React.FC = () => {
  const [marketOverview, setMarketOverview] = useState<MarketOverview | null>(null);
  const [topAssets, setTopAssets] = useState<AssetMetrics[]>([]);
  const [sectorPerformance, setSectorPerformance] = useState<SectorPerformance[]>([]);
  const [correlationData, setCorrelationData] = useState<CorrelationData[]>([]);
  const [opportunities, setOpportunities] = useState<OpportunityAlert[]>([]);
  const [correlationTimeframe, setCorrelationTimeframe] = useState('7d');
  const [selectedSector, setSelectedSector] = useState('All');

  // Generate mock market overview
  const generateMarketOverview = (): MarketOverview => ({
    totalMarketCap: 1200000000000 + Math.random() * 500000000000, // $1.2T-$1.7T
    totalVolume24h: 25000000000 + Math.random() * 50000000000, // $25B-$75B
    btcDominance: 45 + Math.random() * 15, // 45-60%
    fearGreedIndex: 20 + Math.random() * 60, // 20-80
    activeCrypto: 8500 + Math.floor(Math.random() * 1500),
    exchanges: 450 + Math.floor(Math.random() * 100),
  });

  // Generate top assets data
  const generateTopAssets = (): AssetMetrics[] => {
    const symbols = ['BTC', 'ETH', 'SOL', 'ADA', 'MATIC', 'AVAX', 'DOT', 'ATOM', 'LINK', 'UNI'];
    
    return symbols.map((symbol, index) => ({
      symbol,
      price: symbol === 'BTC' ? 43000 + Math.random() * 5000 :
             symbol === 'ETH' ? 2600 + Math.random() * 400 :
             10 + Math.random() * 100,
      change24h: (Math.random() - 0.5) * 20, // -10% to +10%
      volume24h: 500000000 + Math.random() * 2000000000,
      marketCap: 10000000000 + Math.random() * 100000000000,
      rsi: 30 + Math.random() * 40,
      volatility: 20 + Math.random() * 60,
      correlation: (Math.random() - 0.5) * 2, // -1 to 1 with BTC
      momentum: (Math.random() - 0.5) * 2,
      sentiment: (Math.random() - 0.5) * 2,
      rank: index + 1,
    }));
  };

  // Generate sector performance data
  const generateSectorPerformance = (): SectorPerformance[] => {
    return MARKET_SECTORS.map(sector => ({
      sector,
      performance24h: (Math.random() - 0.5) * 20,
      performance7d: (Math.random() - 0.5) * 40,
      performance30d: (Math.random() - 0.5) * 60,
      marketCap: 10000000000 + Math.random() * 100000000000,
      dominance: 2 + Math.random() * 15,
      topAssets: ['BTC', 'ETH', 'SOL'].slice(0, Math.floor(1 + Math.random() * 3)),
    }));
  };

  // Generate correlation matrix
  const generateCorrelationData = (): CorrelationData[] => {
    const assets = ['BTC', 'ETH', 'SOL', 'ADA', 'MATIC'];
    const correlations: CorrelationData[] = [];
    
    for (let i = 0; i < assets.length; i++) {
      for (let j = i + 1; j < assets.length; j++) {
        const correlation = (Math.random() - 0.5) * 2;
        const pValue = Math.random() * 0.1; // 0-0.1 for significance
        
        correlations.push({
          asset1: assets[i],
          asset2: assets[j],
          correlation,
          pValue,
          relationship: Math.abs(correlation) > 0.7 ? 'Strong' :
                       Math.abs(correlation) > 0.4 ? 'Moderate' :
                       Math.abs(correlation) > 0.2 ? 'Weak' : 'None',
        });
      }
    }
    
    return correlations;
  };

  // Generate market opportunities
  const generateOpportunities = (): OpportunityAlert[] => {
    const types: OpportunityAlert['type'][] = ['arbitrage', 'breakout', 'divergence', 'unusual_volume'];
    const symbols = ['BTC', 'ETH', 'SOL', 'MATIC', 'AVAX'];
    const risks: OpportunityAlert['risk'][] = ['Low', 'Medium', 'High'];
    
    return Array.from({ length: 5 }, () => {
      const type = types[Math.floor(Math.random() * types.length)];
      const symbol = symbols[Math.floor(Math.random() * symbols.length)];
      
      return {
        type,
        symbol,
        message: generateOpportunityMessage(type, symbol),
        confidence: 0.6 + Math.random() * 0.4,
        timeframe: ['1m', '5m', '15m', '1h'][Math.floor(Math.random() * 4)],
        potentialReturn: 0.5 + Math.random() * 4.5, // 0.5-5%
        risk: risks[Math.floor(Math.random() * risks.length)],
      };
    });
  };

  const generateOpportunityMessage = (type: string, symbol: string): string => {
    switch (type) {
      case 'arbitrage':
        return `${symbol} price difference detected between exchanges (2.3% spread)`;
      case 'breakout':
        return `${symbol} breaking above key resistance level with high volume`;
      case 'divergence':
        return `${symbol} showing bullish divergence on RSI vs price action`;
      case 'unusual_volume':
        return `${symbol} experiencing 340% above average volume`;
      default:
        return `${symbol} opportunity detected`;
    }
  };

  // Initialize data
  useEffect(() => {
    setMarketOverview(generateMarketOverview());
    setTopAssets(generateTopAssets());
    setSectorPerformance(generateSectorPerformance());
    setCorrelationData(generateCorrelationData());
    setOpportunities(generateOpportunities());
  }, [correlationTimeframe]);

  // Auto-refresh data
  useEffect(() => {
    const interval = setInterval(() => {
      setMarketOverview(generateMarketOverview());
      setTopAssets(generateTopAssets());
      setOpportunities(generateOpportunities());
    }, 30000); // 30 seconds

    return () => clearInterval(interval);
  }, []);

  const formatLargeNumber = (num: number): string => {
    if (num >= 1e12) return `$${(num / 1e12).toFixed(2)}T`;
    if (num >= 1e9) return `$${(num / 1e9).toFixed(2)}B`;
    if (num >= 1e6) return `$${(num / 1e6).toFixed(2)}M`;
    return `$${num.toFixed(2)}`;
  };

  const formatPercent = (value: number): string => {
    return `${value >= 0 ? '+' : ''}${value.toFixed(2)}%`;
  };

  const getPerformanceColor = (value: number): string => {
    return value >= 0 ? 'text-green-400' : 'text-red-400';
  };

  const getFearGreedColor = (value: number): string => {
    if (value < 25) return 'text-red-400';
    if (value < 50) return 'text-yellow-400';
    if (value < 75) return 'text-blue-400';
    return 'text-green-400';
  };

  const getCorrelationColor = (value: number): string => {
    const abs = Math.abs(value);
    if (abs > 0.7) return value > 0 ? 'text-green-400' : 'text-red-400';
    if (abs > 0.4) return value > 0 ? 'text-green-300' : 'text-red-300';
    return 'text-gray-400';
  };

  const getOpportunityTypeColor = (type: string): string => {
    switch (type) {
      case 'arbitrage': return 'text-blue-400';
      case 'breakout': return 'text-green-400';
      case 'divergence': return 'text-purple-400';
      case 'unusual_volume': return 'text-yellow-400';
      default: return 'text-gray-400';
    }
  };

  const getRiskBadge = (risk: string) => {
    switch (risk) {
      case 'Low':
        return <Badge variant="success">Low Risk</Badge>;
      case 'Medium':
        return <Badge variant="secondary">Medium Risk</Badge>;
      case 'High':
        return <Badge variant="destructive">High Risk</Badge>;
      default:
        return <Badge variant="secondary">{risk}</Badge>;
    }
  };

  return (
    <div className="space-y-6">
      {/* Header */}
      <div className="flex justify-between items-center">
        <h2 className="text-2xl font-bold text-white">Market Analytics</h2>
        <Button variant="outline">Export Analysis</Button>
      </div>

      {/* Market Overview */}
      <div className="grid grid-cols-2 md:grid-cols-3 lg:grid-cols-6 gap-4">
        <Card className="p-4">
          <div className="text-sm text-gray-400">Total Market Cap</div>
          <div className="text-xl font-bold text-white">
            {formatLargeNumber(marketOverview?.totalMarketCap || 0)}
          </div>
        </Card>

        <Card className="p-4">
          <div className="text-sm text-gray-400">24h Volume</div>
          <div className="text-xl font-bold text-white">
            {formatLargeNumber(marketOverview?.totalVolume24h || 0)}
          </div>
        </Card>

        <Card className="p-4">
          <div className="text-sm text-gray-400">BTC Dominance</div>
          <div className="text-xl font-bold text-orange-400">
            {marketOverview?.btcDominance.toFixed(1)}%
          </div>
        </Card>

        <Card className="p-4">
          <div className="text-sm text-gray-400">Fear & Greed</div>
          <div className={`text-xl font-bold ${getFearGreedColor(marketOverview?.fearGreedIndex || 0)}`}>
            {marketOverview?.fearGreedIndex.toFixed(0)}
          </div>
        </Card>

        <Card className="p-4">
          <div className="text-sm text-gray-400">Active Crypto</div>
          <div className="text-xl font-bold text-white">
            {marketOverview?.activeCrypto.toLocaleString()}
          </div>
        </Card>

        <Card className="p-4">
          <div className="text-sm text-gray-400">Exchanges</div>
          <div className="text-xl font-bold text-white">
            {marketOverview?.exchanges}
          </div>
        </Card>
      </div>

      <div className="grid grid-cols-1 lg:grid-cols-2 gap-6">
        {/* Top Assets */}
        <Card className="p-6">
          <h3 className="text-lg font-semibold text-white mb-4">Top Assets Analysis</h3>
          <div className="overflow-x-auto">
            <table className="w-full text-sm">
              <thead>
                <tr className="border-b border-gray-700">
                  <th className="text-left py-2 text-gray-400">Asset</th>
                  <th className="text-right py-2 text-gray-400">Price</th>
                  <th className="text-right py-2 text-gray-400">24h</th>
                  <th className="text-right py-2 text-gray-400">RSI</th>
                  <th className="text-right py-2 text-gray-400">Sentiment</th>
                </tr>
              </thead>
              <tbody>
                {topAssets.slice(0, 8).map(asset => (
                  <tr key={asset.symbol} className="border-b border-gray-800">
                    <td className="py-2">
                      <div className="flex items-center">
                        <span className="font-semibold text-white">{asset.symbol}</span>
                        <span className="ml-2 text-xs text-gray-400">#{asset.rank}</span>
                      </div>
                    </td>
                    <td className="py-2 text-right text-white">
                      ${asset.price.toLocaleString()}
                    </td>
                    <td className={`py-2 text-right ${getPerformanceColor(asset.change24h)}`}>
                      {formatPercent(asset.change24h)}
                    </td>
                    <td className={`py-2 text-right ${
                      asset.rsi > 70 ? 'text-red-400' : 
                      asset.rsi < 30 ? 'text-green-400' : 'text-white'
                    }`}>
                      {asset.rsi.toFixed(0)}
                    </td>
                    <td className={`py-2 text-right ${
                      asset.sentiment > 0.3 ? 'text-green-400' :
                      asset.sentiment < -0.3 ? 'text-red-400' : 'text-gray-400'
                    }`}>
                      {asset.sentiment > 0 ? '🟢' : asset.sentiment < 0 ? '🔴' : '⚪'}
                    </td>
                  </tr>
                ))}
              </tbody>
            </table>
          </div>
        </Card>

        {/* Sector Performance */}
        <Card className="p-6">
          <h3 className="text-lg font-semibold text-white mb-4">Sector Performance</h3>
          <div className="space-y-3">
            {sectorPerformance.map(sector => (
              <div key={sector.sector} className="flex items-center justify-between p-3 bg-gray-800 rounded-lg">
                <div>
                  <div className="font-semibold text-white">{sector.sector}</div>
                  <div className="text-sm text-gray-400">
                    {formatLargeNumber(sector.marketCap)} • {sector.dominance.toFixed(1)}%
                  </div>
                </div>
                <div className="text-right">
                  <div className={`font-semibold ${getPerformanceColor(sector.performance24h)}`}>
                    {formatPercent(sector.performance24h)}
                  </div>
                  <div className="text-sm text-gray-400">24h</div>
                </div>
              </div>
            ))}
          </div>
        </Card>
      </div>

      <div className="grid grid-cols-1 lg:grid-cols-2 gap-6">
        {/* Correlation Matrix */}
        <Card className="p-6">
          <div className="flex justify-between items-center mb-4">
            <h3 className="text-lg font-semibold text-white">Asset Correlations</h3>
            <Select
              value={correlationTimeframe}
              onValueChange={setCorrelationTimeframe}
              options={CORRELATION_TIMEFRAMES}
            />
          </div>
          <div className="space-y-3">
            {correlationData.map(corr => (
              <div key={`${corr.asset1}-${corr.asset2}`} className="flex items-center justify-between">
                <div className="flex items-center space-x-2">
                  <span className="text-white">{corr.asset1}</span>
                  <span className="text-gray-400">×</span>
                  <span className="text-white">{corr.asset2}</span>
                </div>
                <div className="flex items-center space-x-2">
                  <span className={getCorrelationColor(corr.correlation)}>
                    {corr.correlation.toFixed(3)}
                  </span>
                  <Badge 
                    variant={corr.relationship === 'Strong' ? 'default' : 'secondary'}
                    className="text-xs"
                  >
                    {corr.relationship}
                  </Badge>
                </div>
              </div>
            ))}
          </div>
        </Card>

        {/* Market Opportunities */}
        <Card className="p-6">
          <h3 className="text-lg font-semibold text-white mb-4">Market Opportunities</h3>
          <div className="space-y-4">
            {opportunities.map((opp, index) => (
              <div key={index} className="p-3 bg-gray-800 rounded-lg">
                <div className="flex items-start justify-between mb-2">
                  <div className="flex items-center space-x-2">
                    <span className="font-semibold text-white">{opp.symbol}</span>
                    <span className={`text-sm uppercase ${getOpportunityTypeColor(opp.type)}`}>
                      {opp.type}
                    </span>
                  </div>
                  {getRiskBadge(opp.risk)}
                </div>
                
                <div className="text-sm text-gray-300 mb-2">{opp.message}</div>
                
                <div className="flex items-center justify-between text-xs">
                  <span className="text-gray-400">{opp.timeframe} • {(opp.confidence * 100).toFixed(0)}% confidence</span>
                  <span className="text-green-400">
                    +{opp.potentialReturn.toFixed(2)}% potential
                  </span>
                </div>
              </div>
            ))}
          </div>
        </Card>
      </div>

      {/* Action Panel */}
      <Card className="p-6">
        <div className="flex items-center justify-between">
          <div>
            <h3 className="text-lg font-semibold text-white">Market Analysis Summary</h3>
            <p className="text-gray-400 mt-1">
              Market showing {marketOverview && marketOverview.fearGreedIndex > 50 ? 'bullish' : 'bearish'} sentiment with 
              {sectorPerformance.filter(s => s.performance24h > 0).length}/{sectorPerformance.length} sectors positive.
            </p>
          </div>
          <div className="flex space-x-3">
            <Button variant="outline">Set Alerts</Button>
            <Button>Execute Strategy</Button>
          </div>
        </div>
      </Card>
    </div>
  );
};
