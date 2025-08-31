'use client';

import React, { useEffect, useRef, useState, useCallback } from 'react';
import { Card } from '../ui/Card';
import { Button } from '../ui/Button';
import { Select } from '../ui/Select';
import { useWebSocket } from '../../hooks/useWebSocket';

interface PriceData {
  time: number;
  open: number;
  high: number;
  low: number;
  close: number;
  volume: number;
}

interface TechnicalIndicator {
  id: string;
  name: string;
  enabled: boolean;
  color: string;
  settings: Record<string, any>;
}

interface TradingViewChartProps {
  symbol: string;
  onOrderPlace?: (side: 'buy' | 'sell', price: number, amount: number) => void;
  className?: string;
}

const TIMEFRAMES = [
  { value: '1m', label: '1 Minute' },
  { value: '5m', label: '5 Minutes' },
  { value: '15m', label: '15 Minutes' },
  { value: '1h', label: '1 Hour' },
  { value: '4h', label: '4 Hours' },
  { value: '1d', label: '1 Day' },
];

const INDICATORS = [
  { id: 'sma', name: 'SMA (20)', color: '#3B82F6' },
  { id: 'ema', name: 'EMA (12)', color: '#EF4444' },
  { id: 'bollinger', name: 'Bollinger Bands', color: '#8B5CF6' },
  { id: 'rsi', name: 'RSI (14)', color: '#F59E0B' },
  { id: 'macd', name: 'MACD', color: '#10B981' },
  { id: 'volume', name: 'Volume', color: '#6B7280' },
];

export const TradingViewChart: React.FC<TradingViewChartProps> = ({
  symbol,
  onOrderPlace,
  className = '',
}) => {
  const canvasRef = useRef<HTMLCanvasElement>(null);
  const containerRef = useRef<HTMLDivElement>(null);
  const [timeframe, setTimeframe] = useState('15m');
  const [indicators, setIndicators] = useState<TechnicalIndicator[]>(
    INDICATORS.map(ind => ({ ...ind, enabled: false, settings: {} }))
  );
  const [priceData, setPriceData] = useState<PriceData[]>([]);
  const [currentPrice, setCurrentPrice] = useState<number>(0);
  const [orderPrice, setOrderPrice] = useState<number>(0);
  const [orderAmount, setOrderAmount] = useState<number>(0);
  const [chartDimensions, setChartDimensions] = useState({ width: 800, height: 400 });
  const [mousePosition, setMousePosition] = useState({ x: 0, y: 0 });
  const [crosshairVisible, setCrosshairVisible] = useState(false);

  // WebSocket connection for real-time data
  const { isConnected, subscribe, unsubscribe } = useWebSocket('ws://localhost:8080/ws');

  // Generate mock historical data
  const generateMockData = useCallback((symbol: string, timeframe: string): PriceData[] => {
    const data: PriceData[] = [];
    const now = Date.now();
    const interval = timeframe === '1m' ? 60000 : 
                    timeframe === '5m' ? 300000 :
                    timeframe === '15m' ? 900000 :
                    timeframe === '1h' ? 3600000 :
                    timeframe === '4h' ? 14400000 : 86400000;
    
    let price = symbol === 'BTC' ? 43000 : symbol === 'ETH' ? 2600 : 105;
    
    for (let i = 100; i >= 0; i--) {
      const time = now - (i * interval);
      const volatility = 0.02;
      const change = (Math.random() - 0.5) * volatility;
      
      const open = price;
      const close = price * (1 + change);
      const high = Math.max(open, close) * (1 + Math.random() * 0.01);
      const low = Math.min(open, close) * (1 - Math.random() * 0.01);
      const volume = 1000000 + Math.random() * 5000000;
      
      data.push({ time, open, high, low, close, volume });
      price = close;
    }
    
    return data;
  }, []);

  // Calculate technical indicators
  const calculateSMA = (data: PriceData[], period: number): number[] => {
    const sma: number[] = [];
    for (let i = 0; i < data.length; i++) {
      if (i < period - 1) {
        sma.push(NaN);
      } else {
        const sum = data.slice(i - period + 1, i + 1).reduce((acc, candle) => acc + candle.close, 0);
        sma.push(sum / period);
      }
    }
    return sma;
  };

  const calculateEMA = (data: PriceData[], period: number): number[] => {
    const ema: number[] = [];
    const multiplier = 2 / (period + 1);
    
    for (let i = 0; i < data.length; i++) {
      if (i === 0) {
        ema.push(data[i].close);
      } else {
        ema.push((data[i].close - ema[i - 1]) * multiplier + ema[i - 1]);
      }
    }
    return ema;
  };

  const calculateRSI = (data: PriceData[], period: number): number[] => {
    const rsi: number[] = [];
    const gains: number[] = [];
    const losses: number[] = [];
    
    for (let i = 1; i < data.length; i++) {
      const change = data[i].close - data[i - 1].close;
      gains.push(change > 0 ? change : 0);
      losses.push(change < 0 ? -change : 0);
    }
    
    for (let i = 0; i < gains.length; i++) {
      if (i < period - 1) {
        rsi.push(NaN);
      } else {
        const avgGain = gains.slice(i - period + 1, i + 1).reduce((a, b) => a + b) / period;
        const avgLoss = losses.slice(i - period + 1, i + 1).reduce((a, b) => a + b) / period;
        const rs = avgGain / (avgLoss || 0.01);
        rsi.push(100 - (100 / (1 + rs)));
      }
    }
    
    return [NaN, ...rsi]; // Add NaN for first element
  };

  // Drawing functions
  const drawCandlestick = (
    ctx: CanvasRenderingContext2D,
    candle: PriceData,
    x: number,
    candleWidth: number,
    priceToY: (price: number) => number
  ) => {
    const openY = priceToY(candle.open);
    const closeY = priceToY(candle.close);
    const highY = priceToY(candle.high);
    const lowY = priceToY(candle.low);
    
    const isGreen = candle.close > candle.open;
    ctx.fillStyle = isGreen ? '#10B981' : '#EF4444';
    ctx.strokeStyle = isGreen ? '#10B981' : '#EF4444';
    
    // Draw wick
    ctx.beginPath();
    ctx.moveTo(x + candleWidth / 2, highY);
    ctx.lineTo(x + candleWidth / 2, lowY);
    ctx.lineWidth = 1;
    ctx.stroke();
    
    // Draw body
    const bodyHeight = Math.abs(closeY - openY);
    const bodyY = Math.min(openY, closeY);
    
    if (bodyHeight > 1) {
      ctx.fillRect(x + 1, bodyY, candleWidth - 2, bodyHeight);
    } else {
      // Doji - draw thin line
      ctx.fillRect(x + 1, bodyY, candleWidth - 2, 1);
    }
  };

  const drawIndicator = (
    ctx: CanvasRenderingContext2D,
    data: number[],
    color: string,
    priceToY: (price: number) => number,
    xPositions: number[]
  ) => {
    ctx.strokeStyle = color;
    ctx.lineWidth = 2;
    ctx.beginPath();
    
    let pathStarted = false;
    for (let i = 0; i < data.length; i++) {
      if (!isNaN(data[i])) {
        const y = priceToY(data[i]);
        if (!pathStarted) {
          ctx.moveTo(xPositions[i], y);
          pathStarted = true;
        } else {
          ctx.lineTo(xPositions[i], y);
        }
      }
    }
    ctx.stroke();
  };

  const drawCrosshair = (ctx: CanvasRenderingContext2D, x: number, y: number) => {
    if (!crosshairVisible) return;
    
    ctx.strokeStyle = '#6B7280';
    ctx.lineWidth = 1;
    ctx.setLineDash([5, 5]);
    
    // Vertical line
    ctx.beginPath();
    ctx.moveTo(x, 0);
    ctx.lineTo(x, chartDimensions.height);
    ctx.stroke();
    
    // Horizontal line
    ctx.beginPath();
    ctx.moveTo(0, y);
    ctx.lineTo(chartDimensions.width, y);
    ctx.stroke();
    
    ctx.setLineDash([]);
  };

  const drawPriceAxis = (ctx: CanvasRenderingContext2D, minPrice: number, maxPrice: number) => {
    const axisWidth = 80;
    const steps = 10;
    const priceStep = (maxPrice - minPrice) / steps;
    
    ctx.fillStyle = '#1F2937';
    ctx.fillRect(chartDimensions.width - axisWidth, 0, axisWidth, chartDimensions.height);
    
    ctx.fillStyle = '#9CA3AF';
    ctx.font = '12px monospace';
    ctx.textAlign = 'right';
    
    for (let i = 0; i <= steps; i++) {
      const price = minPrice + (i * priceStep);
      const y = chartDimensions.height - (i * (chartDimensions.height / steps));
      
      ctx.fillText(price.toFixed(2), chartDimensions.width - 10, y + 4);
      
      // Grid lines
      ctx.strokeStyle = '#374151';
      ctx.lineWidth = 0.5;
      ctx.beginPath();
      ctx.moveTo(0, y);
      ctx.lineTo(chartDimensions.width - axisWidth, y);
      ctx.stroke();
    }
  };

  // Main chart rendering
  const renderChart = useCallback(() => {
    const canvas = canvasRef.current;
    if (!canvas) return;
    
    const ctx = canvas.getContext('2d');
    if (!ctx) return;
    
    // Clear canvas
    ctx.fillStyle = '#111827';
    ctx.fillRect(0, 0, chartDimensions.width, chartDimensions.height);
    
    if (priceData.length === 0) return;
    
    // Calculate price range
    const prices = priceData.flatMap(candle => [candle.high, candle.low]);
    const minPrice = Math.min(...prices) * 0.99;
    const maxPrice = Math.max(...prices) * 1.01;
    const priceRange = maxPrice - minPrice;
    
    const chartWidth = chartDimensions.width - 80; // Leave space for price axis
    const candleWidth = chartWidth / priceData.length * 0.8;
    
    const priceToY = (price: number) => {
      return chartDimensions.height - ((price - minPrice) / priceRange) * chartDimensions.height;
    };
    
    const xPositions = priceData.map((_, i) => (i * chartWidth / priceData.length) + candleWidth / 2);
    
    // Draw candlesticks
    priceData.forEach((candle, i) => {
      const x = i * chartWidth / priceData.length;
      drawCandlestick(ctx, candle, x, candleWidth, priceToY);
    });
    
    // Draw enabled indicators
    indicators.forEach(indicator => {
      if (!indicator.enabled) return;
      
      let data: number[] = [];
      switch (indicator.id) {
        case 'sma':
          data = calculateSMA(priceData, 20);
          break;
        case 'ema':
          data = calculateEMA(priceData, 12);
          break;
        case 'rsi':
          // RSI needs separate scaling (0-100)
          const rsiData = calculateRSI(priceData, 14);
          data = rsiData.map(val => isNaN(val) ? NaN : minPrice + (val / 100) * priceRange);
          break;
      }
      
      if (data.length > 0) {
        drawIndicator(ctx, data, indicator.color, priceToY, xPositions);
      }
    });
    
    // Draw price axis
    drawPriceAxis(ctx, minPrice, maxPrice);
    
    // Draw crosshair
    drawCrosshair(ctx, mousePosition.x, mousePosition.y);
    
    // Update current price
    if (priceData.length > 0) {
      setCurrentPrice(priceData[priceData.length - 1].close);
    }
    
  }, [priceData, indicators, chartDimensions, mousePosition, crosshairVisible]);

  // Handle canvas mouse events
  const handleMouseMove = (event: React.MouseEvent<HTMLCanvasElement>) => {
    const canvas = canvasRef.current;
    if (!canvas) return;
    
    const rect = canvas.getBoundingClientRect();
    setMousePosition({
      x: event.clientX - rect.left,
      y: event.clientY - rect.top,
    });
  };

  const handleMouseEnter = () => setCrosshairVisible(true);
  const handleMouseLeave = () => setCrosshairVisible(false);

  const handleCanvasClick = (event: React.MouseEvent<HTMLCanvasElement>) => {
    if (!onOrderPlace) return;
    
    const canvas = canvasRef.current;
    if (!canvas) return;
    
    const rect = canvas.getBoundingClientRect();
    const y = event.clientY - rect.top;
    
    // Calculate price at click position
    const prices = priceData.flatMap(candle => [candle.high, candle.low]);
    const minPrice = Math.min(...prices) * 0.99;
    const maxPrice = Math.max(...prices) * 1.01;
    const priceRange = maxPrice - minPrice;
    
    const clickPrice = maxPrice - ((y / chartDimensions.height) * priceRange);
    setOrderPrice(clickPrice);
  };

  // Initialize data
  useEffect(() => {
    const data = generateMockData(symbol, timeframe);
    setPriceData(data);
  }, [symbol, timeframe, generateMockData]);

  // Setup canvas dimensions
  useEffect(() => {
    const updateDimensions = () => {
      if (containerRef.current) {
        const { clientWidth, clientHeight } = containerRef.current;
        setChartDimensions({
          width: clientWidth - 32, // Account for padding
          height: clientHeight - 120, // Account for controls
        });
      }
    };
    
    updateDimensions();
    window.addEventListener('resize', updateDimensions);
    return () => window.removeEventListener('resize', updateDimensions);
  }, []);

  // Render chart when data or dimensions change
  useEffect(() => {
    renderChart();
  }, [renderChart]);

  // Real-time data subscription
  useEffect(() => {
    if (isConnected) {
      subscribe(`price.${symbol}`, (data: any) => {
        // Update last candle with real-time data
        setPriceData(prev => {
          const updated = [...prev];
          if (updated.length > 0) {
            updated[updated.length - 1] = {
              ...updated[updated.length - 1],
              close: data.price,
              high: Math.max(updated[updated.length - 1].high, data.price),
              low: Math.min(updated[updated.length - 1].low, data.price),
            };
          }
          return updated;
        });
      });
      
      return () => unsubscribe(`price.${symbol}`);
    }
  }, [isConnected, symbol, subscribe, unsubscribe]);

  const toggleIndicator = (indicatorId: string) => {
    setIndicators(prev =>
      prev.map(ind =>
        ind.id === indicatorId ? { ...ind, enabled: !ind.enabled } : ind
      )
    );
  };

  const handlePlaceOrder = (side: 'buy' | 'sell') => {
    if (onOrderPlace && orderPrice > 0 && orderAmount > 0) {
      onOrderPlace(side, orderPrice, orderAmount);
      setOrderPrice(0);
      setOrderAmount(0);
    }
  };

  return (
    <Card className={`p-4 ${className}`}>
      <div ref={containerRef} className="h-[600px]">
        {/* Chart Controls */}
        <div className="flex items-center justify-between mb-4">
          <div className="flex items-center space-x-4">
            <h3 className="text-lg font-semibold text-white">
              {symbol}/USD - ${currentPrice.toFixed(2)}
            </h3>
            
            <Select
              value={timeframe}
              onValueChange={setTimeframe}
              options={TIMEFRAMES}
            />
          </div>
          
          <div className="flex items-center space-x-2">
            {indicators.map(indicator => (
              <Button
                key={indicator.id}
                variant={indicator.enabled ? 'default' : 'outline'}
                size="sm"
                onClick={() => toggleIndicator(indicator.id)}
                style={{
                  borderColor: indicator.enabled ? indicator.color : undefined,
                  backgroundColor: indicator.enabled ? `${indicator.color}20` : undefined,
                }}
              >
                {indicator.name}
              </Button>
            ))}
          </div>
        </div>

        {/* Chart Canvas */}
        <canvas
          ref={canvasRef}
          width={chartDimensions.width}
          height={chartDimensions.height}
          className="border border-gray-700 rounded cursor-crosshair"
          onMouseMove={handleMouseMove}
          onMouseEnter={handleMouseEnter}
          onMouseLeave={handleMouseLeave}
          onClick={handleCanvasClick}
        />

        {/* Order Controls */}
        {onOrderPlace && (
          <div className="mt-4 p-3 bg-gray-800 rounded-lg">
            <div className="flex items-center space-x-4">
              <div className="flex-1">
                <label className="block text-sm text-gray-400 mb-1">Price</label>
                <input
                  type="number"
                  value={orderPrice}
                  onChange={(e) => setOrderPrice(parseFloat(e.target.value) || 0)}
                  className="w-full px-3 py-1 bg-gray-700 border border-gray-600 rounded text-white"
                  placeholder="Click chart to set price"
                />
              </div>
              
              <div className="flex-1">
                <label className="block text-sm text-gray-400 mb-1">Amount</label>
                <input
                  type="number"
                  value={orderAmount}
                  onChange={(e) => setOrderAmount(parseFloat(e.target.value) || 0)}
                  className="w-full px-3 py-1 bg-gray-700 border border-gray-600 rounded text-white"
                  placeholder="Enter amount"
                />
              </div>
              
              <div className="flex space-x-2">
                <Button
                  onClick={() => handlePlaceOrder('buy')}
                  className="bg-green-600 hover:bg-green-700"
                  disabled={!orderPrice || !orderAmount}
                >
                  Buy
                </Button>
                <Button
                  onClick={() => handlePlaceOrder('sell')}
                  className="bg-red-600 hover:bg-red-700"
                  disabled={!orderPrice || !orderAmount}
                >
                  Sell
                </Button>
              </div>
            </div>
          </div>
        )}
      </div>
    </Card>
  );
};
