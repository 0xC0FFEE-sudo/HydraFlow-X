'use client';

import { useState, useEffect, useRef } from 'react';
import { motion } from 'framer-motion';
import { Card, CardHeader, CardTitle, CardContent } from '@/components/ui/Card';
import { Badge } from '@/components/ui/Badge';
import { Button } from '@/components/ui/Button';
import { apiService } from '@/lib/api-service';
import {
  TrendingUp,
  TrendingDown,
  Activity,
  RefreshCw,
  Play,
  Pause,
  Zap,
  Shield,
  Clock,
  BarChart3,
  Target,
  AlertTriangle,
  CheckCircle,
  XCircle,
} from 'lucide-react';

export interface ChartDataPoint {
  timestamp: number;
  value: number;
  label?: string;
  category?: string;
}

export interface RealTimeChartProps {
  title: string;
  subtitle?: string;
  endpoint: string;
  refreshInterval?: number;
  height?: number;
  showControls?: boolean;
  type?: 'line' | 'area' | 'bar' | 'pie' | 'scatter';
  yAxisLabel?: string;
  xAxisLabel?: string;
  color?: string;
  showLegend?: boolean;
  showGrid?: boolean;
  animate?: boolean;
}

export function RealTimeChart({
  title,
  subtitle,
  endpoint,
  refreshInterval = 5000,
  height = 300,
  showControls = true,
  type = 'line',
  yAxisLabel,
  xAxisLabel,
  color = '#3b82f6',
  showLegend = false,
  showGrid = true,
  animate = true,
}: RealTimeChartProps) {
  const [data, setData] = useState<ChartDataPoint[]>([]);
  const [isLoading, setIsLoading] = useState(true);
  const [error, setError] = useState<string | null>(null);
  const [isPaused, setIsPaused] = useState(false);
  const [lastUpdate, setLastUpdate] = useState<Date | null>(null);
  const intervalRef = useRef<NodeJS.Timeout | null>(null);

  const fetchData = async () => {
    try {
      const response = await apiService.get(endpoint);
      const newData = Array.isArray(response.data) ? response.data : [response.data];

      // Transform data to chart format
      const transformedData = newData.map((item: any, index: number) => ({
        timestamp: item.timestamp || Date.now() - (newData.length - index) * 1000,
        value: item.value || item.price || item.amount || 0,
        label: item.label || new Date(item.timestamp || Date.now()).toLocaleTimeString(),
        category: item.category || item.type,
      }));

      setData(transformedData);
      setLastUpdate(new Date());
      setError(null);
    } catch (err) {
      setError('Failed to fetch data');
      console.error('Chart data fetch error:', err);
    } finally {
      setIsLoading(false);
    }
  };

  useEffect(() => {
    fetchData();

    if (!isPaused) {
      intervalRef.current = setInterval(fetchData, refreshInterval);
    }

    return () => {
      if (intervalRef.current) {
        clearInterval(intervalRef.current);
      }
    };
  }, [endpoint, refreshInterval, isPaused]);

  const togglePause = () => {
    setIsPaused(!isPaused);
  };

  const refreshData = () => {
    fetchData();
  };

  const formatValue = (value: number) => {
    if (value >= 1000000) return `${(value / 1000000).toFixed(2)}M`;
    if (value >= 1000) return `${(value / 1000).toFixed(2)}K`;
    return value.toFixed(2);
  };

  const formatTimestamp = (timestamp: number) => {
    return new Date(timestamp).toLocaleTimeString();
  };

  const renderChart = () => {
    if (data.length === 0) {
      return (
        <div className="flex items-center justify-center h-full text-slate-400">
          <div className="text-center">
            <Activity className="w-12 h-12 mx-auto mb-4 opacity-50" />
            <p>No data available</p>
          </div>
        </div>
      );
    }

    const maxValue = Math.max(...data.map(d => d.value));
    const minValue = Math.min(...data.map(d => d.value));
    const range = maxValue - minValue || 1;

    // Create SVG path for line/area charts
    const createPath = () => {
      const points = data.map((point, index) => {
        const x = (index / (data.length - 1)) * 100;
        const y = 100 - ((point.value - minValue) / range) * 80;
        return `${index === 0 ? 'M' : 'L'} ${x} ${y}`;
      }).join(' ');
      return points;
    };

    const pathData = createPath();

    switch (type) {
      case 'area':
        return (
          <div className="relative h-full">
            <svg viewBox="0 0 100 100" className="w-full h-full">
              <defs>
                <linearGradient id={`area-gradient-${title}`} x1="0%" y1="0%" x2="0%" y2="100%">
                  <stop offset="0%" stopColor={color} stopOpacity="0.4" />
                  <stop offset="100%" stopColor={color} stopOpacity="0.1" />
                </linearGradient>
              </defs>
              {showGrid && (
                <g className="opacity-20">
                  <line x1="0" y1="20" x2="100" y2="20" stroke="#374151" strokeWidth="0.5" />
                  <line x1="0" y1="40" x2="100" y2="40" stroke="#374151" strokeWidth="0.5" />
                  <line x1="0" y1="60" x2="100" y2="60" stroke="#374151" strokeWidth="0.5" />
                  <line x1="0" y1="80" x2="100" y2="80" stroke="#374151" strokeWidth="0.5" />
                </g>
              )}
              <path
                d={pathData}
                fill="none"
                stroke={color}
                strokeWidth="2"
                strokeLinecap="round"
                strokeLinejoin="round"
              />
              <path
                d={`${pathData} L 100 100 L 0 100 Z`}
                fill={`url(#area-gradient-${title})`}
              />
            </svg>
          </div>
        );

      case 'bar':
        return (
          <div className="flex items-end justify-between h-full space-x-1 px-2">
            {data.map((point, index) => {
              const height = ((point.value - minValue) / range) * 80 + 5;
              return (
                <motion.div
                  key={index}
                  className="flex-1 bg-current rounded-t"
                  style={{ 
                    height: `${height}%`,
                    backgroundColor: color,
                    opacity: 0.8
                  }}
                  initial={{ height: 0 }}
                  animate={{ height: `${height}%` }}
                  transition={{ duration: animate ? 0.3 : 0, delay: index * 0.1 }}
                  title={`${formatTimestamp(point.timestamp)}: ${formatValue(point.value)}`}
                />
              );
            })}
          </div>
        );

      case 'pie':
        const total = data.reduce((sum, point) => sum + point.value, 0);
        let currentAngle = 0;
        return (
          <div className="flex items-center justify-center h-full">
            <svg viewBox="0 0 200 200" className="w-48 h-48">
              {data.map((point, index) => {
                const percentage = point.value / total;
                const angle = percentage * 360;
                const x1 = 100 + 80 * Math.cos((currentAngle - 90) * Math.PI / 180);
                const y1 = 100 + 80 * Math.sin((currentAngle - 90) * Math.PI / 180);
                const x2 = 100 + 80 * Math.cos((currentAngle + angle - 90) * Math.PI / 180);
                const y2 = 100 + 80 * Math.sin((currentAngle + angle - 90) * Math.PI / 180);
                
                const largeArc = angle > 180 ? 1 : 0;
                const pathData = `M 100,100 L ${x1},${y1} A 80,80 0 ${largeArc},1 ${x2},${y2} Z`;
                
                currentAngle += angle;
                
                return (
                  <path
                    key={index}
                    d={pathData}
                    fill={color}
                    opacity={0.8 - index * 0.1}
                    stroke="#1e293b"
                    strokeWidth="2"
                  />
                );
              })}
            </svg>
          </div>
        );

      default: // line and scatter
        return (
          <div className="relative h-full">
            <svg viewBox="0 0 100 100" className="w-full h-full">
              {showGrid && (
                <g className="opacity-20">
                  <line x1="0" y1="20" x2="100" y2="20" stroke="#374151" strokeWidth="0.5" />
                  <line x1="0" y1="40" x2="100" y2="40" stroke="#374151" strokeWidth="0.5" />
                  <line x1="0" y1="60" x2="100" y2="60" stroke="#374151" strokeWidth="0.5" />
                  <line x1="0" y1="80" x2="100" y2="80" stroke="#374151" strokeWidth="0.5" />
                </g>
              )}
              {type === 'scatter' ? (
                // Render scatter points
                data.map((point, index) => {
                  const x = (index / (data.length - 1)) * 100;
                  const y = 100 - ((point.value - minValue) / range) * 80;
                  return (
                    <circle
                      key={index}
                      cx={x}
                      cy={y}
                      r="2"
                      fill={color}
                      opacity="0.8"
                    />
                  );
                })
              ) : (
                // Render line
                <path
                  d={pathData}
                  fill="none"
                  stroke={color}
                  strokeWidth="2"
                  strokeLinecap="round"
                  strokeLinejoin="round"
                />
              )}
            </svg>
            {/* Y-axis labels */}
            <div className="absolute left-0 top-0 h-full flex flex-col justify-between text-xs text-slate-400 pr-2">
              <span>{formatValue(maxValue)}</span>
              <span>{formatValue((maxValue + minValue) / 2)}</span>
              <span>{formatValue(minValue)}</span>
            </div>
          </div>
        );
    }
  };

  const getStatusIcon = () => {
    if (error) return <XCircle className="w-4 h-4 text-red-500" />;
    if (isLoading) return <Activity className="w-4 h-4 text-blue-500 animate-pulse" />;
    return <CheckCircle className="w-4 h-4 text-green-500" />;
  };

  const getStatusText = () => {
    if (error) return 'Error';
    if (isLoading) return 'Loading...';
    if (isPaused) return 'Paused';
    return 'Live';
  };

  return (
    <motion.div
      initial={{ opacity: 0, y: 20 }}
      animate={{ opacity: 1, y: 0 }}
      transition={{ duration: 0.3 }}
    >
      <Card className="h-full">
        <CardHeader className="pb-3">
          <div className="flex items-center justify-between">
            <div>
              <CardTitle className="text-lg font-semibold flex items-center gap-2">
                {getStatusIcon()}
                {title}
              </CardTitle>
              {subtitle && (
                <p className="text-sm text-muted-foreground mt-1">{subtitle}</p>
              )}
            </div>

            {showControls && (
              <div className="flex items-center gap-2">
                <Badge variant={error ? 'danger' : isPaused ? 'secondary' : 'default'}>
                  {getStatusText()}
                </Badge>

                <Button
                  variant="outline"
                  size="sm"
                  onClick={togglePause}
                  disabled={isLoading}
                >
                  {isPaused ? <Play className="w-4 h-4" /> : <Pause className="w-4 h-4" />}
                </Button>

                <Button
                  variant="outline"
                  size="sm"
                  onClick={refreshData}
                  disabled={isLoading}
                >
                  <RefreshCw className={`w-4 h-4 ${isLoading ? 'animate-spin' : ''}`} />
                </Button>
              </div>
            )}
          </div>

          {lastUpdate && (
            <div className="text-xs text-muted-foreground">
              Last updated: {lastUpdate.toLocaleTimeString()}
            </div>
          )}
        </CardHeader>

        <CardContent className="pt-0">
          {error ? (
            <div className="flex items-center justify-center h-[300px] text-red-500">
              <div className="text-center">
                <AlertTriangle className="w-8 h-8 mx-auto mb-2" />
                <p>{error}</p>
                <Button
                  variant="outline"
                  size="sm"
                  onClick={refreshData}
                  className="mt-2"
                >
                  Retry
                </Button>
              </div>
            </div>
          ) : (
            <div style={{ height: `${height}px` }}>
              {renderChart()}
            </div>
          )}
        </CardContent>
      </Card>
    </motion.div>
  );
}
