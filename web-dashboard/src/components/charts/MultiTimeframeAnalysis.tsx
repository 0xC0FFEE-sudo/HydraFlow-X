'use client';

import React, { useState, useEffect } from 'react';
import { Card } from '../ui/Card';
import { Badge } from '../ui/Badge';
import { Button } from '../ui/Button';
import { Select } from '../ui/Select';

interface TimeframeAnalysis {
  timeframe: string;
  trend: 'BULLISH' | 'BEARISH' | 'NEUTRAL';
  strength: number; // 0-100
  rsi: number;
  macd: number;
  volume: number;
  support: number;
  resistance: number;
  recommendation: 'BUY' | 'SELL' | 'HOLD';
  confidence: number;
}

interface TechnicalIndicator {
  name: string;
  value: number;
  signal: 'BUY' | 'SELL' | 'NEUTRAL';
  description: string;
}

interface MarketStructure {
  higherHighs: boolean;
  higherLows: boolean;
  lowerHighs: boolean;
  lowerLows: boolean;
  breakoutLevel: number | null;
  keyLevels: number[];
}

const TIMEFRAMES = ['1m', '5m', '15m', '1h', '4h', '1d', '1w'];

const SYMBOLS = [
  { value: 'BTC', label: 'Bitcoin' },
  { value: 'ETH', label: 'Ethereum' },
  { value: 'SOL', label: 'Solana' },
  { value: 'MATIC', label: 'Polygon' },
  { value: 'AVAX', label: 'Avalanche' },
];

export const MultiTimeframeAnalysis: React.FC = () => {
  const [selectedSymbol, setSelectedSymbol] = useState('BTC');
  const [timeframeAnalysis, setTimeframeAnalysis] = useState<TimeframeAnalysis[]>([]);
  const [technicalIndicators, setTechnicalIndicators] = useState<TechnicalIndicator[]>([]);
  const [marketStructure, setMarketStructure] = useState<MarketStructure | null>(null);
  const [overallSignal, setOverallSignal] = useState<'BUY' | 'SELL' | 'HOLD'>('HOLD');
  const [signalStrength, setSignalStrength] = useState(0);

  // Generate mock timeframe analysis
  const generateTimeframeAnalysis = (symbol: string): TimeframeAnalysis[] => {
    return TIMEFRAMES.map(timeframe => {
      const trendRandom = Math.random();
      const trend = trendRandom > 0.6 ? 'BULLISH' : trendRandom < 0.4 ? 'BEARISH' : 'NEUTRAL';
      const strength = 20 + Math.random() * 60;
      
      return {
        timeframe,
        trend,
        strength,
        rsi: 30 + Math.random() * 40,
        macd: (Math.random() - 0.5) * 2,
        volume: 0.5 + Math.random() * 1.5,
        support: 40000 + Math.random() * 2000,
        resistance: 45000 + Math.random() * 2000,
        recommendation: trend === 'BULLISH' ? 'BUY' : trend === 'BEARISH' ? 'SELL' : 'HOLD',
        confidence: 0.6 + Math.random() * 0.4,
      };
    });
  };

  // Generate technical indicators
  const generateTechnicalIndicators = (): TechnicalIndicator[] => {
    const indicators = [
      {
        name: 'RSI (14)',
        value: 30 + Math.random() * 40,
        signal: 'NEUTRAL' as const,
        description: 'Relative Strength Index indicates momentum',
      },
      {
        name: 'MACD',
        value: (Math.random() - 0.5) * 100,
        signal: 'BUY' as const,
        description: 'Moving Average Convergence Divergence shows trend changes',
      },
      {
        name: 'Bollinger Bands',
        value: Math.random(),
        signal: 'SELL' as const,
        description: 'Price position relative to volatility bands',
      },
      {
        name: 'Stochastic',
        value: Math.random() * 100,
        signal: 'NEUTRAL' as const,
        description: 'Momentum oscillator comparing closing price to range',
      },
      {
        name: 'Williams %R',
        value: -Math.random() * 100,
        signal: 'BUY' as const,
        description: 'Momentum indicator measuring overbought/oversold levels',
      },
      {
        name: 'Volume Profile',
        value: 0.5 + Math.random() * 1.5,
        signal: 'BUY' as const,
        description: 'Volume distribution analysis across price levels',
      },
    ];

    return indicators.map(indicator => ({
      ...indicator,
      signal: indicator.value > 70 ? 'SELL' : 
              indicator.value < 30 ? 'BUY' : 'NEUTRAL',
    }));
  };

  // Generate market structure analysis
  const generateMarketStructure = (): MarketStructure => {
    const higherHighs = Math.random() > 0.5;
    const higherLows = Math.random() > 0.5;
    const lowerHighs = Math.random() > 0.5;
    const lowerLows = Math.random() > 0.5;
    
    return {
      higherHighs,
      higherLows,
      lowerHighs,
      lowerLows,
      breakoutLevel: Math.random() > 0.7 ? 44000 + Math.random() * 2000 : null,
      keyLevels: [42000, 43500, 45000, 46500, 48000].map(level => 
        level + (Math.random() - 0.5) * 1000
      ),
    };
  };

  // Calculate overall signal
  const calculateOverallSignal = (analysis: TimeframeAnalysis[]): { signal: 'BUY' | 'SELL' | 'HOLD', strength: number } => {
    const weights = {
      '1m': 0.05,
      '5m': 0.1,
      '15m': 0.15,
      '1h': 0.2,
      '4h': 0.25,
      '1d': 0.2,
      '1w': 0.05,
    };

    let buyScore = 0;
    let sellScore = 0;
    let totalWeight = 0;

    analysis.forEach(tf => {
      const weight = weights[tf.timeframe as keyof typeof weights] || 0.1;
      totalWeight += weight;

      if (tf.recommendation === 'BUY') {
        buyScore += weight * tf.confidence;
      } else if (tf.recommendation === 'SELL') {
        sellScore += weight * tf.confidence;
      }
    });

    const normalizedBuy = buyScore / totalWeight;
    const normalizedSell = sellScore / totalWeight;

    if (normalizedBuy > normalizedSell && normalizedBuy > 0.3) {
      return { signal: 'BUY', strength: normalizedBuy * 100 };
    } else if (normalizedSell > normalizedBuy && normalizedSell > 0.3) {
      return { signal: 'SELL', strength: normalizedSell * 100 };
    } else {
      return { signal: 'HOLD', strength: Math.max(normalizedBuy, normalizedSell) * 100 };
    }
  };

  // Update data when symbol changes
  useEffect(() => {
    const analysis = generateTimeframeAnalysis(selectedSymbol);
    const indicators = generateTechnicalIndicators();
    const structure = generateMarketStructure();
    const overall = calculateOverallSignal(analysis);

    setTimeframeAnalysis(analysis);
    setTechnicalIndicators(indicators);
    setMarketStructure(structure);
    setOverallSignal(overall.signal);
    setSignalStrength(overall.strength);
  }, [selectedSymbol]);

  // Auto-refresh data every 30 seconds
  useEffect(() => {
    const interval = setInterval(() => {
      const analysis = generateTimeframeAnalysis(selectedSymbol);
      const indicators = generateTechnicalIndicators();
      const structure = generateMarketStructure();
      const overall = calculateOverallSignal(analysis);

      setTimeframeAnalysis(analysis);
      setTechnicalIndicators(indicators);
      setMarketStructure(structure);
      setOverallSignal(overall.signal);
      setSignalStrength(overall.strength);
    }, 30000);

    return () => clearInterval(interval);
  }, [selectedSymbol]);

  const getTrendColor = (trend: string) => {
    switch (trend) {
      case 'BULLISH': return 'text-green-400';
      case 'BEARISH': return 'text-red-400';
      default: return 'text-gray-400';
    }
  };

  const getSignalColor = (signal: string) => {
    switch (signal) {
      case 'BUY': return 'text-green-400';
      case 'SELL': return 'text-red-400';
      default: return 'text-gray-400';
    }
  };

  const getSignalBadgeVariant = (signal: string) => {
    switch (signal) {
      case 'BUY': return 'success' as const;
      case 'SELL': return 'destructive' as const;
      default: return 'secondary' as const;
    }
  };

  return (
    <div className="space-y-6">
      {/* Header */}
      <div className="flex justify-between items-center">
        <h2 className="text-2xl font-bold text-white">Multi-Timeframe Analysis</h2>
        <Select
          value={selectedSymbol}
          onValueChange={setSelectedSymbol}
          options={SYMBOLS}
        />
      </div>

      {/* Overall Signal */}
      <Card className="p-6">
        <div className="flex items-center justify-between">
          <div>
            <h3 className="text-lg font-semibold text-white mb-2">Overall Signal</h3>
            <div className="flex items-center space-x-4">
              <Badge variant={getSignalBadgeVariant(overallSignal)} className="text-lg px-4 py-2">
                {overallSignal}
              </Badge>
              <div className="text-2xl font-bold text-white">
                {signalStrength.toFixed(1)}%
              </div>
            </div>
          </div>
          
          <div className="text-right">
            <div className="text-sm text-gray-400 mb-2">Signal Strength</div>
            <div className="w-32 bg-gray-700 rounded-full h-4">
              <div
                className={`h-4 rounded-full ${
                  overallSignal === 'BUY' ? 'bg-green-500' :
                  overallSignal === 'SELL' ? 'bg-red-500' : 'bg-gray-500'
                }`}
                style={{ width: `${signalStrength}%` }}
              />
            </div>
          </div>
        </div>
      </Card>

      {/* Timeframe Analysis Grid */}
      <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-4 gap-4">
        {timeframeAnalysis.map(tf => (
          <Card key={tf.timeframe} className="p-4">
            <div className="flex justify-between items-center mb-3">
              <h4 className="font-semibold text-white">{tf.timeframe}</h4>
              <Badge variant={getSignalBadgeVariant(tf.recommendation)}>
                {tf.recommendation}
              </Badge>
            </div>
            
            <div className="space-y-2">
              <div className="flex justify-between">
                <span className="text-gray-400">Trend:</span>
                <span className={getTrendColor(tf.trend)}>{tf.trend}</span>
              </div>
              
              <div className="flex justify-between">
                <span className="text-gray-400">Strength:</span>
                <span className="text-white">{tf.strength.toFixed(1)}%</span>
              </div>
              
              <div className="flex justify-between">
                <span className="text-gray-400">RSI:</span>
                <span className={`${tf.rsi > 70 ? 'text-red-400' : tf.rsi < 30 ? 'text-green-400' : 'text-white'}`}>
                  {tf.rsi.toFixed(1)}
                </span>
              </div>
              
              <div className="flex justify-between">
                <span className="text-gray-400">MACD:</span>
                <span className={tf.macd > 0 ? 'text-green-400' : 'text-red-400'}>
                  {tf.macd.toFixed(3)}
                </span>
              </div>
              
              <div className="flex justify-between">
                <span className="text-gray-400">Confidence:</span>
                <span className="text-white">{(tf.confidence * 100).toFixed(1)}%</span>
              </div>
            </div>
            
            <div className="mt-3 pt-3 border-t border-gray-700">
              <div className="text-xs text-gray-400">Support/Resistance</div>
              <div className="text-xs text-white">
                ${tf.support.toFixed(0)} / ${tf.resistance.toFixed(0)}
              </div>
            </div>
          </Card>
        ))}
      </div>

      <div className="grid grid-cols-1 lg:grid-cols-2 gap-6">
        {/* Technical Indicators */}
        <Card className="p-6">
          <h3 className="text-lg font-semibold text-white mb-4">Technical Indicators</h3>
          <div className="space-y-4">
            {technicalIndicators.map(indicator => (
              <div key={indicator.name} className="flex items-center justify-between">
                <div className="flex-1">
                  <div className="flex items-center justify-between">
                    <span className="font-medium text-white">{indicator.name}</span>
                    <Badge variant={getSignalBadgeVariant(indicator.signal)} className="ml-2">
                      {indicator.signal}
                    </Badge>
                  </div>
                  <div className="text-sm text-gray-400 mt-1">{indicator.description}</div>
                </div>
                <div className="text-right ml-4">
                  <div className="font-semibold text-white">
                    {indicator.value.toFixed(2)}
                  </div>
                </div>
              </div>
            ))}
          </div>
        </Card>

        {/* Market Structure */}
        <Card className="p-6">
          <h3 className="text-lg font-semibold text-white mb-4">Market Structure</h3>
          
          {marketStructure && (
            <div className="space-y-4">
              <div className="grid grid-cols-2 gap-4">
                <div className="flex items-center justify-between">
                  <span className="text-gray-400">Higher Highs:</span>
                  <span className={marketStructure.higherHighs ? 'text-green-400' : 'text-red-400'}>
                    {marketStructure.higherHighs ? '✓' : '✗'}
                  </span>
                </div>
                
                <div className="flex items-center justify-between">
                  <span className="text-gray-400">Higher Lows:</span>
                  <span className={marketStructure.higherLows ? 'text-green-400' : 'text-red-400'}>
                    {marketStructure.higherLows ? '✓' : '✗'}
                  </span>
                </div>
                
                <div className="flex items-center justify-between">
                  <span className="text-gray-400">Lower Highs:</span>
                  <span className={marketStructure.lowerHighs ? 'text-red-400' : 'text-green-400'}>
                    {marketStructure.lowerHighs ? '✓' : '✗'}
                  </span>
                </div>
                
                <div className="flex items-center justify-between">
                  <span className="text-gray-400">Lower Lows:</span>
                  <span className={marketStructure.lowerLows ? 'text-red-400' : 'text-green-400'}>
                    {marketStructure.lowerLows ? '✓' : '✗'}
                  </span>
                </div>
              </div>
              
              {marketStructure.breakoutLevel && (
                <div className="pt-4 border-t border-gray-700">
                  <div className="flex justify-between items-center">
                    <span className="text-gray-400">Breakout Level:</span>
                    <span className="text-yellow-400 font-semibold">
                      ${marketStructure.breakoutLevel.toFixed(0)}
                    </span>
                  </div>
                </div>
              )}
              
              <div className="pt-4 border-t border-gray-700">
                <div className="text-gray-400 mb-2">Key Levels:</div>
                <div className="space-y-1">
                  {marketStructure.keyLevels.map((level, index) => (
                    <div key={index} className="flex justify-between text-sm">
                      <span className="text-gray-500">Level {index + 1}:</span>
                      <span className="text-white">${level.toFixed(0)}</span>
                    </div>
                  ))}
                </div>
              </div>
            </div>
          )}
        </Card>
      </div>

      {/* Action Buttons */}
      <div className="flex justify-center space-x-4">
        <Button 
          className="bg-green-600 hover:bg-green-700"
          disabled={overallSignal !== 'BUY'}
        >
          Execute Buy Order
        </Button>
        <Button 
          className="bg-red-600 hover:bg-red-700"
          disabled={overallSignal !== 'SELL'}
        >
          Execute Sell Order
        </Button>
        <Button variant="outline">
          Set Price Alert
        </Button>
        <Button variant="outline">
          Save Analysis
        </Button>
      </div>
    </div>
  );
};
