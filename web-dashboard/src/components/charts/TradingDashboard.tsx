'use client';

import React, { useState, useEffect } from 'react';
import { TradingViewChart } from './TradingViewChart';
import { LatencyChart } from './LatencyChart';
import { Card } from '../ui/Card';
import { Button } from '../ui/Button';
import { Select } from '../ui/Select';
import { Tabs } from '../ui/Tabs';
import { Badge } from '../ui/Badge';
import { useWebSocket } from '../../hooks/useWebSocket';

interface MarketData {
  symbol: string;
  price: number;
  change24h: number;
  volume24h: number;
  marketCap: number;
  sentiment: number;
}

interface TradingSignal {
  symbol: string;
  signal: 'BUY' | 'SELL' | 'HOLD';
  confidence: number;
  timestamp: number;
  source: string;
  reasoning: string;
}

interface PortfolioPosition {
  symbol: string;
  amount: number;
  entryPrice: number;
  currentPrice: number;
  pnl: number;
  pnlPercent: number;
}

const WATCHLIST_SYMBOLS = ['BTC', 'ETH', 'SOL', 'MATIC', 'AVAX', 'ADA', 'DOT', 'ATOM'];

const CHART_LAYOUTS = [
  { value: 'single', label: 'Single Chart' },
  { value: 'dual', label: 'Dual Charts' },
  { value: 'quad', label: 'Quad Charts' },
  { value: 'grid', label: 'Grid View' },
];

export const TradingDashboard: React.FC = () => {
  const [selectedSymbol, setSelectedSymbol] = useState('BTC');
  const [chartLayout, setChartLayout] = useState('single');
  const [marketData, setMarketData] = useState<MarketData[]>([]);
  const [tradingSignals, setTradingSignals] = useState<TradingSignal[]>([]);
  const [portfolio, setPortfolio] = useState<PortfolioPosition[]>([]);
  const [activeTab, setActiveTab] = useState('charts');
  const [autoTradingEnabled, setAutoTradingEnabled] = useState(false);

  const { isConnected, subscribe, send } = useWebSocket('ws://localhost:8080/ws');

  // Generate mock market data
  useEffect(() => {
    const generateMarketData = (): MarketData[] => {
      return WATCHLIST_SYMBOLS.map(symbol => ({
        symbol,
        price: symbol === 'BTC' ? 43000 + Math.random() * 2000 :
               symbol === 'ETH' ? 2600 + Math.random() * 200 :
               symbol === 'SOL' ? 105 + Math.random() * 20 :
               0.5 + Math.random() * 2,
        change24h: (Math.random() - 0.5) * 20,
        volume24h: 100000000 + Math.random() * 500000000,
        marketCap: Math.random() * 100000000000,
        sentiment: (Math.random() - 0.5) * 2, // -1 to 1
      }));
    };

    setMarketData(generateMarketData());
    
    // Update market data every 5 seconds
    const interval = setInterval(() => {
      setMarketData(generateMarketData());
    }, 5000);

    return () => clearInterval(interval);
  }, []);

  // Generate mock trading signals
  useEffect(() => {
    const generateSignal = (): TradingSignal => {
      const symbol = WATCHLIST_SYMBOLS[Math.floor(Math.random() * WATCHLIST_SYMBOLS.length)];
      const signals: ('BUY' | 'SELL' | 'HOLD')[] = ['BUY', 'SELL', 'HOLD'];
      const sources = ['AI Engine', 'Sentiment Analysis', 'Technical Analysis', 'MEV Detection'];
      
      return {
        symbol,
        signal: signals[Math.floor(Math.random() * signals.length)],
        confidence: 0.6 + Math.random() * 0.4,
        timestamp: Date.now(),
        source: sources[Math.floor(Math.random() * sources.length)],
        reasoning: 'Strong momentum detected with bullish sentiment confluence',
      };
    };

    const interval = setInterval(() => {
      setTradingSignals(prev => [generateSignal(), ...prev.slice(0, 9)]);
    }, 10000);

    return () => clearInterval(interval);
  }, []);

  // Generate mock portfolio
  useEffect(() => {
    const generatePortfolio = (): PortfolioPosition[] => {
      return ['BTC', 'ETH', 'SOL'].map(symbol => {
        const entryPrice = symbol === 'BTC' ? 42000 : symbol === 'ETH' ? 2500 : 100;
        const currentPrice = entryPrice * (1 + (Math.random() - 0.5) * 0.2);
        const amount = Math.random() * 10;
        const pnl = (currentPrice - entryPrice) * amount;
        
        return {
          symbol,
          amount,
          entryPrice,
          currentPrice,
          pnl,
          pnlPercent: ((currentPrice - entryPrice) / entryPrice) * 100,
        };
      });
    };

    setPortfolio(generatePortfolio());
  }, []);

  const handleOrderPlace = async (side: 'buy' | 'sell', price: number, amount: number) => {
    if (!isConnected) {
      console.error('Not connected to trading backend');
      return;
    }

    try {
      const order = {
        type: 'place_order',
        data: {
          symbol: selectedSymbol,
          side,
          price,
          amount,
          timestamp: Date.now(),
        },
      };

      send(JSON.stringify(order));
      
      // Show success feedback
      console.log(`Order placed: ${side.toUpperCase()} ${amount} ${selectedSymbol} at $${price}`);
    } catch (error) {
      console.error('Failed to place order:', error);
    }
  };

  const toggleAutoTrading = () => {
    setAutoTradingEnabled(!autoTradingEnabled);
    
    if (isConnected) {
      send(JSON.stringify({
        type: 'toggle_auto_trading',
        data: { enabled: !autoTradingEnabled },
      }));
    }
  };

  const renderChartLayout = () => {
    switch (chartLayout) {
      case 'dual':
        return (
          <div className="grid grid-cols-1 lg:grid-cols-2 gap-4">
            <TradingViewChart
              symbol={selectedSymbol}
              onOrderPlace={handleOrderPlace}
            />
            <TradingViewChart
              symbol={selectedSymbol === 'BTC' ? 'ETH' : 'BTC'}
              onOrderPlace={handleOrderPlace}
            />
          </div>
        );
      
      case 'quad':
        return (
          <div className="grid grid-cols-2 gap-4">
            {['BTC', 'ETH', 'SOL', 'MATIC'].map(symbol => (
              <TradingViewChart
                key={symbol}
                symbol={symbol}
                onOrderPlace={handleOrderPlace}
                className="h-[300px]"
              />
            ))}
          </div>
        );
      
      case 'grid':
        return (
          <div className="grid grid-cols-2 md:grid-cols-3 lg:grid-cols-4 gap-4">
            {WATCHLIST_SYMBOLS.map(symbol => (
              <TradingViewChart
                key={symbol}
                symbol={symbol}
                onOrderPlace={handleOrderPlace}
                className="h-[250px]"
              />
            ))}
          </div>
        );
      
      default:
        return (
          <TradingViewChart
            symbol={selectedSymbol}
            onOrderPlace={handleOrderPlace}
            className="h-[600px]"
          />
        );
    }
  };

  const renderMarketOverview = () => (
    <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-4 gap-4">
      {marketData.map(data => (
        <Card
          key={data.symbol}
          className={`p-4 cursor-pointer transition-all ${
            selectedSymbol === data.symbol ? 'ring-2 ring-blue-500' : ''
          }`}
          onClick={() => setSelectedSymbol(data.symbol)}
        >
          <div className="flex justify-between items-start mb-2">
            <h3 className="font-semibold text-white">{data.symbol}</h3>
            <Badge
              variant={data.sentiment > 0 ? 'success' : data.sentiment < 0 ? 'destructive' : 'secondary'}
            >
              {data.sentiment > 0 ? 'Bullish' : data.sentiment < 0 ? 'Bearish' : 'Neutral'}
            </Badge>
          </div>
          
          <div className="space-y-1">
            <div className="text-xl font-bold text-white">
              ${data.price.toLocaleString()}
            </div>
            
            <div className={`text-sm ${data.change24h >= 0 ? 'text-green-400' : 'text-red-400'}`}>
              {data.change24h >= 0 ? '+' : ''}{data.change24h.toFixed(2)}%
            </div>
            
            <div className="text-xs text-gray-400">
              Vol: ${(data.volume24h / 1000000).toFixed(1)}M
            </div>
            
            <div className="w-full bg-gray-700 rounded-full h-1 mt-2">
              <div
                className={`h-1 rounded-full ${
                  data.sentiment > 0 ? 'bg-green-500' : 'bg-red-500'
                }`}
                style={{ width: `${Math.abs(data.sentiment) * 50}%` }}
              />
            </div>
          </div>
        </Card>
      ))}
    </div>
  );

  const renderTradingSignals = () => (
    <Card className="p-4">
      <div className="flex justify-between items-center mb-4">
        <h3 className="text-lg font-semibold text-white">AI Trading Signals</h3>
        <Button
          onClick={toggleAutoTrading}
          variant={autoTradingEnabled ? 'default' : 'outline'}
          className={autoTradingEnabled ? 'bg-green-600' : ''}
        >
          {autoTradingEnabled ? '🤖 Auto Trading ON' : '🤖 Auto Trading OFF'}
        </Button>
      </div>
      
      <div className="space-y-3 max-h-96 overflow-y-auto">
        {tradingSignals.map((signal, index) => (
          <div
            key={index}
            className="flex items-center justify-between p-3 bg-gray-800 rounded-lg"
          >
            <div className="flex items-center space-x-3">
              <Badge
                variant={
                  signal.signal === 'BUY' ? 'success' :
                  signal.signal === 'SELL' ? 'destructive' : 'secondary'
                }
              >
                {signal.signal}
              </Badge>
              
              <div>
                <div className="font-semibold text-white">{signal.symbol}</div>
                <div className="text-sm text-gray-400">{signal.source}</div>
              </div>
            </div>
            
            <div className="text-right">
              <div className="text-sm font-semibold text-white">
                {(signal.confidence * 100).toFixed(1)}%
              </div>
              <div className="text-xs text-gray-400">
                {new Date(signal.timestamp).toLocaleTimeString()}
              </div>
            </div>
          </div>
        ))}
      </div>
    </Card>
  );

  const renderPortfolio = () => (
    <Card className="p-4">
      <h3 className="text-lg font-semibold text-white mb-4">Portfolio Overview</h3>
      
      <div className="space-y-3">
        {portfolio.map(position => (
          <div
            key={position.symbol}
            className="flex items-center justify-between p-3 bg-gray-800 rounded-lg"
          >
            <div className="flex items-center space-x-3">
              <div>
                <div className="font-semibold text-white">{position.symbol}</div>
                <div className="text-sm text-gray-400">
                  {position.amount.toFixed(4)} tokens
                </div>
              </div>
            </div>
            
            <div className="text-right">
              <div className="text-sm font-semibold text-white">
                ${position.currentPrice.toFixed(2)}
              </div>
              <div className={`text-sm ${position.pnl >= 0 ? 'text-green-400' : 'text-red-400'}`}>
                {position.pnl >= 0 ? '+' : ''}${position.pnl.toFixed(2)} 
                ({position.pnlPercent >= 0 ? '+' : ''}{position.pnlPercent.toFixed(2)}%)
              </div>
            </div>
          </div>
        ))}
      </div>
      
      <div className="mt-4 pt-4 border-t border-gray-700">
        <div className="flex justify-between items-center">
          <span className="text-gray-400">Total P&L:</span>
          <span className={`font-semibold ${
            portfolio.reduce((sum, pos) => sum + pos.pnl, 0) >= 0 ? 'text-green-400' : 'text-red-400'
          }`}>
            ${portfolio.reduce((sum, pos) => sum + pos.pnl, 0).toFixed(2)}
          </span>
        </div>
      </div>
    </Card>
  );

  const tabs = [
    { id: 'charts', label: 'Trading Charts' },
    { id: 'signals', label: 'AI Signals' },
    { id: 'portfolio', label: 'Portfolio' },
    { id: 'analytics', label: 'Analytics' },
  ];

  return (
    <div className="p-6 space-y-6">
      {/* Header Controls */}
      <div className="flex flex-col md:flex-row justify-between items-start md:items-center space-y-4 md:space-y-0">
        <div className="flex items-center space-x-4">
          <h1 className="text-2xl font-bold text-white">Trading Dashboard</h1>
          <Badge variant={isConnected ? 'success' : 'destructive'}>
            {isConnected ? 'Connected' : 'Disconnected'}
          </Badge>
        </div>
        
        <div className="flex items-center space-x-4">
          <Select
            value={selectedSymbol}
            onValueChange={setSelectedSymbol}
            options={WATCHLIST_SYMBOLS.map(symbol => ({ value: symbol, label: symbol }))}
          />
          
          <Select
            value={chartLayout}
            onValueChange={setChartLayout}
            options={CHART_LAYOUTS}
          />
        </div>
      </div>

      {/* Market Overview */}
      {renderMarketOverview()}

      {/* Main Content */}
      <Tabs value={activeTab} onValueChange={setActiveTab} tabs={tabs}>
        <div className="mt-6">
          {activeTab === 'charts' && (
            <div className="space-y-6">
              {renderChartLayout()}
              <LatencyChart />
            </div>
          )}
          
          {activeTab === 'signals' && (
            <div className="grid grid-cols-1 lg:grid-cols-2 gap-6">
              {renderTradingSignals()}
              {renderPortfolio()}
            </div>
          )}
          
          {activeTab === 'portfolio' && (
            <div className="grid grid-cols-1 lg:grid-cols-3 gap-6">
              {renderPortfolio()}
              {renderTradingSignals()}
              <Card className="p-4">
                <h3 className="text-lg font-semibold text-white mb-4">Performance Metrics</h3>
                <div className="space-y-3">
                  <div className="flex justify-between">
                    <span className="text-gray-400">Win Rate:</span>
                    <span className="text-green-400">73.2%</span>
                  </div>
                  <div className="flex justify-between">
                    <span className="text-gray-400">Sharpe Ratio:</span>
                    <span className="text-white">2.14</span>
                  </div>
                  <div className="flex justify-between">
                    <span className="text-gray-400">Max Drawdown:</span>
                    <span className="text-red-400">-8.3%</span>
                  </div>
                  <div className="flex justify-between">
                    <span className="text-gray-400">Total Trades:</span>
                    <span className="text-white">1,247</span>
                  </div>
                </div>
              </Card>
            </div>
          )}
          
          {activeTab === 'analytics' && (
            <div className="grid grid-cols-1 lg:grid-cols-2 gap-6">
              <LatencyChart />
              <Card className="p-4">
                <h3 className="text-lg font-semibold text-white mb-4">System Analytics</h3>
                <div className="space-y-4">
                  <div>
                    <div className="flex justify-between text-sm">
                      <span className="text-gray-400">CPU Usage</span>
                      <span className="text-white">34%</span>
                    </div>
                    <div className="w-full bg-gray-700 rounded-full h-2 mt-1">
                      <div className="bg-blue-500 h-2 rounded-full" style={{ width: '34%' }} />
                    </div>
                  </div>
                  
                  <div>
                    <div className="flex justify-between text-sm">
                      <span className="text-gray-400">Memory Usage</span>
                      <span className="text-white">67%</span>
                    </div>
                    <div className="w-full bg-gray-700 rounded-full h-2 mt-1">
                      <div className="bg-yellow-500 h-2 rounded-full" style={{ width: '67%' }} />
                    </div>
                  </div>
                  
                  <div>
                    <div className="flex justify-between text-sm">
                      <span className="text-gray-400">Network I/O</span>
                      <span className="text-white">89%</span>
                    </div>
                    <div className="w-full bg-gray-700 rounded-full h-2 mt-1">
                      <div className="bg-red-500 h-2 rounded-full" style={{ width: '89%' }} />
                    </div>
                  </div>
                  
                  <div className="pt-4 border-t border-gray-700">
                    <div className="text-sm text-gray-400">Active Strategies: <span className="text-white">7</span></div>
                    <div className="text-sm text-gray-400">Orders/sec: <span className="text-white">245</span></div>
                    <div className="text-sm text-gray-400">Latency: <span className="text-green-400">12ms</span></div>
                  </div>
                </div>
              </Card>
            </div>
          )}
        </div>
      </Tabs>
    </div>
  );
};
