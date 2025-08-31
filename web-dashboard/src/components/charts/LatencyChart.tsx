'use client';

import { Card } from '@/components/ui/Card';
import { LineChart, Line, XAxis, YAxis, CartesianGrid, Tooltip, ResponsiveContainer } from 'recharts';

interface LatencyChartProps {
  data?: Array<{
    timestamp: number;
    latency: number;
  }>;
}

export function LatencyChart({ data }: LatencyChartProps) {
  // Generate mock data if none provided
  const mockData = Array.from({ length: 20 }, (_, i) => ({
    timestamp: Date.now() - (19 - i) * 60000, // Last 20 minutes
    latency: Math.random() * 10 + 10, // Random latency between 10-20ms
  }));

  const chartData = (data || mockData).map(point => ({
    ...point,
    time: new Date(point.timestamp).toLocaleTimeString('en-US', {
      hour12: false,
      minute: '2-digit',
      second: '2-digit',
    }),
  }));

  return (
    <Card className="p-6">
      <div className="flex items-center justify-between mb-4">
        <h3 className="text-lg font-semibold text-gray-100">Decision Latency</h3>
        <div className="flex items-center space-x-4 text-sm text-gray-400">
          <div className="flex items-center">
            <div className="w-3 h-3 bg-blue-500 rounded-full mr-2" />
            <span>Real-time</span>
          </div>
          <div className="flex items-center">
            <div className="w-3 h-3 bg-red-500 rounded-full mr-2" />
            <span>Target: &lt;20ms</span>
          </div>
        </div>
      </div>

      <div className="h-64">
        <ResponsiveContainer width="100%" height="100%">
          <LineChart data={chartData}>
            <CartesianGrid strokeDasharray="3 3" stroke="#374151" />
            <XAxis 
              dataKey="time"
              stroke="#9ca3af"
              fontSize={12}
              tickLine={false}
              axisLine={false}
            />
            <YAxis 
              stroke="#9ca3af"
              fontSize={12}
              tickLine={false}
              axisLine={false}
              domain={[0, 30]}
              label={{ 
                value: 'Latency (ms)', 
                angle: -90, 
                position: 'insideLeft',
                style: { textAnchor: 'middle', fill: '#9ca3af' }
              }}
            />
            <Tooltip
              contentStyle={{
                backgroundColor: '#1f2937',
                border: '1px solid #374151',
                borderRadius: '6px',
                color: '#f3f4f6',
              }}
              formatter={(value: any) => [`${value.toFixed(1)}ms`, 'Latency']}
              labelStyle={{ color: '#9ca3af' }}
            />
            <Line
              type="monotone"
              dataKey="latency"
              stroke="#3b82f6"
              strokeWidth={2}
              dot={false}
              activeDot={{ r: 4, stroke: '#3b82f6', strokeWidth: 2, fill: '#1f2937' }}
            />
            {/* Target line at 20ms */}
            <Line
              type="monotone"
              dataKey={() => 20}
              stroke="#ef4444"
              strokeWidth={1}
              strokeDasharray="5 5"
              dot={false}
            />
          </LineChart>
        </ResponsiveContainer>
      </div>

      <div className="mt-4 grid grid-cols-3 gap-4 text-center">
        <div>
          <p className="text-sm text-gray-400">Current</p>
          <p className="text-lg font-semibold text-blue-400">
            {chartData[chartData.length - 1]?.latency.toFixed(1)}ms
          </p>
        </div>
        <div>
          <p className="text-sm text-gray-400">Average</p>
          <p className="text-lg font-semibold text-gray-100">
            {(chartData.reduce((sum, point) => sum + point.latency, 0) / chartData.length).toFixed(1)}ms
          </p>
        </div>
        <div>
          <p className="text-sm text-gray-400">Max</p>
          <p className="text-lg font-semibold text-yellow-400">
            {Math.max(...chartData.map(point => point.latency)).toFixed(1)}ms
          </p>
        </div>
      </div>
    </Card>
  );
}
