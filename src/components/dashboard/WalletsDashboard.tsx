'use client';

import { motion } from 'framer-motion';
import { Card, CardHeader, CardTitle, CardContent } from '@/components/ui/Card';
import { Badge } from '@/components/ui/Badge';
import { Button } from '@/components/ui/Button';
import { formatters } from '@/lib/utils';
import { Wallet, Plus, Copy, ExternalLink, Activity } from 'lucide-react';

export function WalletsDashboard() {
  const wallets = [
    { id: '1', name: 'Main Trading', address: '0x1a2b3c4d5e6f...', balance: 125.75, tokens: 12, network: 'Ethereum' },
    { id: '2', name: 'Arbitrage Bot', address: '0x7g8h9i0j1k2l...', balance: 89.25, tokens: 8, network: 'Solana' },
    { id: '3', name: 'MEV Vault', address: '0xm3n4o5p6q7r8...', balance: 234.50, tokens: 15, network: 'Base' },
  ];

  return (
    <motion.div
      initial={{ opacity: 0, y: 20 }}
      animate={{ opacity: 1, y: 0 }}
      className="space-y-6"
    >
      <div className="flex items-center justify-between">
        <div>
          <h1 className="text-3xl font-bold gradient-text">Wallet Management</h1>
          <p className="text-secondary-400 mt-1">Manage wallets and token balances</p>
        </div>
        <Button variant="gradient" leftIcon={<Plus className="w-4 h-4" />}>
          Add Wallet
        </Button>
      </div>

      <div className="grid grid-cols-1 lg:grid-cols-2 xl:grid-cols-3 gap-6">
        {wallets.map((wallet) => (
          <Card key={wallet.id} variant="glass" className="hover:scale-105 transition-transform">
            <CardHeader>
              <CardTitle className="flex items-center space-x-2">
                <Wallet className="w-5 h-5 text-primary-400" />
                <span>{wallet.name}</span>
              </CardTitle>
            </CardHeader>
            <CardContent className="space-y-4">
              <div className="space-y-2">
                <div className="flex items-center justify-between">
                  <span className="text-secondary-400">Address</span>
                  <div className="flex items-center space-x-2">
                    <code className="text-sm text-white">{wallet.address}</code>
                    <Button variant="ghost" size="icon-sm">
                      <Copy className="w-3 h-3" />
                    </Button>
                  </div>
                </div>
                <div className="flex items-center justify-between">
                  <span className="text-secondary-400">Balance</span>
                  <span className="font-bold text-white">{formatters.currency(wallet.balance)}</span>
                </div>
                <div className="flex items-center justify-between">
                  <span className="text-secondary-400">Tokens</span>
                  <Badge variant="default">{wallet.tokens}</Badge>
                </div>
                <div className="flex items-center justify-between">
                  <span className="text-secondary-400">Network</span>
                  <Badge variant="success">{wallet.network}</Badge>
                </div>
              </div>
              <div className="flex space-x-2">
                <Button variant="outline" size="sm" className="flex-1">
                  <Activity className="w-4 h-4 mr-2" />
                  Activity
                </Button>
                <Button variant="ghost" size="icon-sm">
                  <ExternalLink className="w-4 h-4" />
                </Button>
              </div>
            </CardContent>
          </Card>
        ))}
      </div>
    </motion.div>
  );
}
