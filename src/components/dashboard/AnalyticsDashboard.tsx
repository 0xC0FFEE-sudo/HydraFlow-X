'use client';

import { motion } from 'framer-motion';
import { Card, CardHeader, CardTitle, CardContent } from '@/components/ui/Card';
import { formatters } from '@/lib/utils';
import { BarChart3, TrendingUp, DollarSign, Activity } from 'lucide-react';

export function AnalyticsDashboard() {
  return (
    <motion.div
      initial={{ opacity: 0, y: 20 }}
      animate={{ opacity: 1, y: 0 }}
      className="space-y-6"
    >
      <div>
        <h1 className="text-3xl font-bold gradient-text">Analytics & Insights</h1>
        <p className="text-secondary-400 mt-1">Trading performance and market analysis</p>
      </div>

      <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-4 gap-6">
        <Card variant="glass">
          <CardContent className="space-y-3">
            <div className="flex items-center justify-between">
              <DollarSign className="w-8 h-8 text-success-400" />
              <TrendingUp className="w-4 h-4 text-success-400" />
            </div>
            <div>
              <p className="metric-label">Total P&L</p>
              <p className="metric-value">{formatters.currency(25847.32)}</p>
            </div>
          </CardContent>
        </Card>

        <Card variant="glass">
          <CardContent className="space-y-3">
            <div className="flex items-center justify-between">
              <BarChart3 className="w-8 h-8 text-primary-400" />
            </div>
            <div>
              <p className="metric-label">Win Rate</p>
              <p className="metric-value">98.7%</p>
            </div>
          </CardContent>
        </Card>

        <Card variant="glass">
          <CardContent className="space-y-3">
            <div className="flex items-center justify-between">
              <Activity className="w-8 h-8 text-warning-400" />
            </div>
            <div>
              <p className="metric-label">Total Trades</p>
              <p className="metric-value">15,847</p>
            </div>
          </CardContent>
        </Card>

        <Card variant="glass">
          <CardContent className="space-y-3">
            <div className="flex items-center justify-between">
              <TrendingUp className="w-8 h-8 text-success-400" />
            </div>
            <div>
              <p className="metric-label">Sharpe Ratio</p>
              <p className="metric-value">2.34</p>
            </div>
          </CardContent>
        </Card>
      </div>

      <div className="grid grid-cols-1 lg:grid-cols-2 gap-6">
        <Card variant="glass" className="h-80">
          <CardHeader>
            <CardTitle>Performance Chart</CardTitle>
          </CardHeader>
          <CardContent className="h-60 flex items-center justify-center">
            <div className="text-center text-secondary-400">
              <BarChart3 className="w-12 h-12 mx-auto mb-4 opacity-50" />
              <p>Performance Chart Component</p>
            </div>
          </CardContent>
        </Card>

        <Card variant="glass" className="h-80">
          <CardHeader>
            <CardTitle>Strategy Breakdown</CardTitle>
          </CardHeader>
          <CardContent className="h-60 flex items-center justify-center">
            <div className="text-center text-secondary-400">
              <Activity className="w-12 h-12 mx-auto mb-4 opacity-50" />
              <p>Strategy Analysis Component</p>
            </div>
          </CardContent>
        </Card>
      </div>
    </motion.div>
  );
}
