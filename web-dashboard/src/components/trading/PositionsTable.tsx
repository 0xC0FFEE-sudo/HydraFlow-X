'use client';

import { Card } from '@/components/ui/Card';
import { Badge } from '@/components/ui/Badge';
import { Button } from '@/components/ui/Button';
import { XMarkIcon } from '@heroicons/react/24/outline';

interface Position {
  id: string;
  symbol: string;
  amount: number;
  value: number;
  pnl: number;
  status: 'active' | 'closed';
}

interface PositionsTableProps {
  positions?: Position[];
}

export function PositionsTable({ positions }: PositionsTableProps) {
  const mockPositions: Position[] = [
    {
      id: '1',
      symbol: 'PEPE/USDC',
      amount: 1000000,
      value: 2450.75,
      pnl: 145.32,
      status: 'active',
    },
    {
      id: '2',
      symbol: 'BONK/SOL',
      amount: 500000,
      value: 1280.50,
      pnl: -23.45,
      status: 'active',
    },
    {
      id: '3',
      symbol: 'SHIB/ETH',
      amount: 750000,
      value: 890.25,
      pnl: 67.89,
      status: 'active',
    },
  ];

  const data = positions || mockPositions;

  const handleClosePosition = (positionId: string) => {
    console.log('Closing position:', positionId);
    // Implement close position logic
  };

  const formatAmount = (amount: number) => {
    if (amount >= 1000000) {
      return `${(amount / 1000000).toFixed(1)}M`;
    } else if (amount >= 1000) {
      return `${(amount / 1000).toFixed(1)}K`;
    }
    return amount.toString();
  };

  const formatCurrency = (value: number) => {
    return new Intl.NumberFormat('en-US', {
      style: 'currency',
      currency: 'USD',
      minimumFractionDigits: 2,
    }).format(value);
  };

  return (
    <Card className="p-6">
      <div className="flex items-center justify-between mb-4">
        <h3 className="text-lg font-semibold text-gray-100">Open Positions</h3>
        <Badge variant="secondary">{data.length} Active</Badge>
      </div>

      <div className="overflow-x-auto">
        <table className="w-full">
          <thead>
            <tr className="border-b border-gray-700">
              <th className="text-left py-3 px-2 text-sm font-medium text-gray-400">Token</th>
              <th className="text-right py-3 px-2 text-sm font-medium text-gray-400">Amount</th>
              <th className="text-right py-3 px-2 text-sm font-medium text-gray-400">Value</th>
              <th className="text-right py-3 px-2 text-sm font-medium text-gray-400">P&L</th>
              <th className="text-center py-3 px-2 text-sm font-medium text-gray-400">Status</th>
              <th className="text-center py-3 px-2 text-sm font-medium text-gray-400">Action</th>
            </tr>
          </thead>
          <tbody>
            {data.map((position) => (
              <tr key={position.id} className="border-b border-gray-700/50 hover:bg-gray-700/20">
                <td className="py-3 px-2">
                  <div className="flex items-center">
                    <div className="w-8 h-8 bg-gradient-to-br from-blue-500 to-purple-600 rounded-full flex items-center justify-center mr-3">
                      <span className="text-xs font-bold text-white">
                        {position.symbol.split('/')[0].slice(0, 2)}
                      </span>
                    </div>
                    <span className="font-medium text-gray-100">{position.symbol}</span>
                  </div>
                </td>
                <td className="py-3 px-2 text-right text-gray-300">
                  {formatAmount(position.amount)}
                </td>
                <td className="py-3 px-2 text-right text-gray-100 font-medium">
                  {formatCurrency(position.value)}
                </td>
                <td className="py-3 px-2 text-right">
                  <span className={`font-medium ${
                    position.pnl >= 0 ? 'text-green-400' : 'text-red-400'
                  }`}>
                    {position.pnl >= 0 ? '+' : ''}{formatCurrency(position.pnl)}
                  </span>
                </td>
                <td className="py-3 px-2 text-center">
                  <Badge variant={position.status === 'active' ? 'success' : 'secondary'}>
                    {position.status}
                  </Badge>
                </td>
                <td className="py-3 px-2 text-center">
                  {position.status === 'active' && (
                    <Button
                      size="sm"
                      variant="outline"
                      onClick={() => handleClosePosition(position.id)}
                      className="p-1 h-7 w-7"
                    >
                      <XMarkIcon className="w-4 h-4" />
                    </Button>
                  )}
                </td>
              </tr>
            ))}
          </tbody>
        </table>
      </div>

      {data.length === 0 && (
        <div className="text-center py-8 text-gray-400">
          <p>No open positions</p>
        </div>
      )}
    </Card>
  );
}
