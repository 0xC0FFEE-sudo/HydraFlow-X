'use client';

import { Card } from '@/components/ui/Card';
import { Badge } from '@/components/ui/Badge';
import { 
  ArrowUpIcon, 
  ArrowDownIcon, 
  ClockIcon 
} from '@heroicons/react/24/outline';

interface Trade {
  id: string;
  symbol: string;
  type: 'buy' | 'sell';
  amount: number;
  price: number;
  timestamp: number;
  status: 'completed' | 'pending' | 'failed';
}

interface RecentTradesProps {
  trades?: Trade[];
}

export function RecentTrades({ trades }: RecentTradesProps) {
  const mockTrades: Trade[] = [
    {
      id: '1',
      symbol: 'PEPE/USDC',
      type: 'buy',
      amount: 1000000,
      price: 0.000012,
      timestamp: Date.now() - 30000,
      status: 'completed',
    },
    {
      id: '2',
      symbol: 'BONK/SOL',
      type: 'sell',
      amount: 500000,
      price: 0.000023,
      timestamp: Date.now() - 120000,
      status: 'completed',
    },
    {
      id: '3',
      symbol: 'SHIB/ETH',
      type: 'buy',
      amount: 750000,
      price: 0.000008,
      timestamp: Date.now() - 180000,
      status: 'pending',
    },
    {
      id: '4',
      symbol: 'DOGE/USDC',
      type: 'sell',
      amount: 25000,
      price: 0.082,
      timestamp: Date.now() - 240000,
      status: 'completed',
    },
    {
      id: '5',
      symbol: 'FLOKI/ETH',
      type: 'buy',
      amount: 2000000,
      price: 0.000015,
      timestamp: Date.now() - 300000,
      status: 'failed',
    },
  ];

  const data = trades || mockTrades;

  const formatAmount = (amount: number) => {
    if (amount >= 1000000) {
      return `${(amount / 1000000).toFixed(1)}M`;
    } else if (amount >= 1000) {
      return `${(amount / 1000).toFixed(1)}K`;
    }
    return amount.toString();
  };

  const formatPrice = (price: number) => {
    if (price < 0.001) {
      return price.toExponential(2);
    }
    return price.toFixed(6);
  };

  const formatTime = (timestamp: number) => {
    const diff = Date.now() - timestamp;
    const minutes = Math.floor(diff / 60000);
    const seconds = Math.floor((diff % 60000) / 1000);
    
    if (minutes > 0) {
      return `${minutes}m ago`;
    }
    return `${seconds}s ago`;
  };

  const getStatusBadge = (status: string) => {
    switch (status) {
      case 'completed':
        return <Badge variant="success">Completed</Badge>;
      case 'pending':
        return <Badge variant="warning">Pending</Badge>;
      case 'failed':
        return <Badge variant="destructive">Failed</Badge>;
      default:
        return <Badge variant="secondary">Unknown</Badge>;
    }
  };

  return (
    <Card className="p-6">
      <div className="flex items-center justify-between mb-4">
        <h3 className="text-lg font-semibold text-gray-100">Recent Trades</h3>
        <Badge variant="secondary">{data.length} Trades</Badge>
      </div>

      <div className="space-y-3 max-h-96 overflow-y-auto">
        {data.map((trade) => (
          <div key={trade.id} className="flex items-center justify-between p-3 bg-gray-700/30 rounded-lg hover:bg-gray-700/50 transition-colors">
            <div className="flex items-center">
              <div className={`w-8 h-8 rounded-full flex items-center justify-center mr-3 ${
                trade.type === 'buy' ? 'bg-green-600' : 'bg-red-600'
              }`}>
                {trade.type === 'buy' ? (
                  <ArrowUpIcon className="w-4 h-4 text-white" />
                ) : (
                  <ArrowDownIcon className="w-4 h-4 text-white" />
                )}
              </div>
              
              <div>
                <div className="flex items-center">
                  <span className="font-medium text-gray-100 mr-2">{trade.symbol}</span>
                  <span className={`text-sm font-medium ${
                    trade.type === 'buy' ? 'text-green-400' : 'text-red-400'
                  }`}>
                    {trade.type.toUpperCase()}
                  </span>
                </div>
                <div className="text-sm text-gray-400">
                  {formatAmount(trade.amount)} @ {formatPrice(trade.price)}
                </div>
              </div>
            </div>

            <div className="text-right">
              {getStatusBadge(trade.status)}
              <div className="flex items-center text-xs text-gray-400 mt-1">
                <ClockIcon className="w-3 h-3 mr-1" />
                {formatTime(trade.timestamp)}
              </div>
            </div>
          </div>
        ))}
      </div>

      {data.length === 0 && (
        <div className="text-center py-8 text-gray-400">
          <p>No recent trades</p>
        </div>
      )}

      <div className="mt-4 pt-4 border-t border-gray-700">
        <div className="grid grid-cols-3 gap-4 text-center text-sm">
          <div>
            <p className="text-gray-400">Total Volume</p>
            <p className="font-semibold text-gray-100">$24,567</p>
          </div>
          <div>
            <p className="text-gray-400">Success Rate</p>
            <p className="font-semibold text-green-400">96.8%</p>
          </div>
          <div>
            <p className="text-gray-400">Avg. Latency</p>
            <p className="font-semibold text-blue-400">14.2ms</p>
          </div>
        </div>
      </div>
    </Card>
  );
}
