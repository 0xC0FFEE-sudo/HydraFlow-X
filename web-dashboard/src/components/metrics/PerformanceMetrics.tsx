'use client';

import { Card } from '@/components/ui/Card';
import { Badge } from '@/components/ui/Badge';
import { 
  ChartBarIcon, 
  ClockIcon, 
  TrendingUpIcon, 
  CpuChipIcon 
} from '@heroicons/react/24/outline';

interface PerformanceMetricsProps {
  metrics?: {
    avgLatency: number;
    totalTrades: number;
    successRate: number;
    uptime: number;
  };
}

export function PerformanceMetrics({ metrics }: PerformanceMetricsProps) {
  const defaultMetrics = {
    avgLatency: 15.2,
    totalTrades: 1247,
    successRate: 98.7,
    uptime: 99.98,
  };

  const data = metrics || defaultMetrics;

  const metricCards = [
    {
      title: 'Avg Latency',
      value: `${data.avgLatency}ms`,
      icon: ClockIcon,
      trend: 'down',
      trendValue: '2.1ms',
      variant: data.avgLatency < 20 ? 'success' : 'warning',
    },
    {
      title: 'Total Trades',
      value: data.totalTrades.toLocaleString(),
      icon: ChartBarIcon,
      trend: 'up',
      trendValue: '+47',
      variant: 'default',
    },
    {
      title: 'Success Rate',
      value: `${data.successRate}%`,
      icon: TrendingUpIcon,
      trend: 'up',
      trendValue: '+0.3%',
      variant: data.successRate > 95 ? 'success' : 'warning',
    },
    {
      title: 'Uptime',
      value: `${data.uptime}%`,
      icon: CpuChipIcon,
      trend: 'stable',
      trendValue: '24h',
      variant: data.uptime > 99 ? 'success' : 'destructive',
    },
  ];

  return (
    <>
      {metricCards.map((metric, index) => {
        const Icon = metric.icon;
        
        return (
          <Card key={index} className="p-6">
            <div className="flex items-center justify-between">
              <div className="flex items-center">
                <Icon className="w-8 h-8 text-blue-500 mr-3" />
                <div>
                  <p className="text-sm font-medium text-gray-400">{metric.title}</p>
                  <p className="text-2xl font-bold text-gray-100">{metric.value}</p>
                </div>
              </div>
              <Badge variant={metric.variant as any}>
                {metric.trend === 'up' && '↗ '}
                {metric.trend === 'down' && '↘ '}
                {metric.trendValue}
              </Badge>
            </div>
          </Card>
        );
      })}
    </>
  );
}
