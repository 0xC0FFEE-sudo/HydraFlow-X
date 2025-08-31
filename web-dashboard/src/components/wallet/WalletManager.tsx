'use client';

import { useState } from 'react';
import { Card } from '@/components/ui/Card';
import { Button } from '@/components/ui/Button';
import { Input } from '@/components/ui/Input';
import { Badge } from '@/components/ui/Badge';
import { Switch } from '@/components/ui/Switch';
import { 
  WalletIcon, 
  PlusIcon, 
  TrashIcon, 
  EyeIcon,
  EyeSlashIcon,
  StarIcon
} from '@heroicons/react/24/outline';
import toast from 'react-hot-toast';

interface Wallet {
  id: string;
  address: string;
  balance: number;
  activeTrades: number;
  isPrimary: boolean;
  enabled: boolean;
  name?: string;
}

export function WalletManager() {
  const [wallets, setWallets] = useState<Wallet[]>([
    {
      id: '1',
      address: '0x742d35Cc6634C0532925a3b8D371D6E1DaE38000',
      balance: 1.245,
      activeTrades: 3,
      isPrimary: true,
      enabled: true,
      name: 'Primary Trading Wallet',
    },
    {
      id: '2',
      address: '0x8ba1f109551bD432803012645Hac136c6e5e5e8c',
      balance: 0.567,
      activeTrades: 1,
      isPrimary: false,
      enabled: true,
      name: 'Secondary Wallet',
    },
    {
      id: '3',
      address: '0x1a1ec25DC08e98e5E93F1104B5e5cd870Ae912f3',
      balance: 0.891,
      activeTrades: 0,
      isPrimary: false,
      enabled: false,
      name: 'Backup Wallet',
    },
  ]);

  const [showAddWallet, setShowAddWallet] = useState(false);
  const [newWalletData, setNewWalletData] = useState({
    privateKey: '',
    name: '',
  });
  const [showPrivateKeys, setShowPrivateKeys] = useState<Record<string, boolean>>({});

  const handleAddWallet = async () => {
    if (!newWalletData.privateKey.trim()) {
      toast.error('Please enter a private key');
      return;
    }

    try {
      // Mock wallet addition - in real implementation, this would validate and add the wallet
      const newWallet: Wallet = {
        id: Date.now().toString(),
        address: '0x' + Math.random().toString(16).substr(2, 40),
        balance: 0,
        activeTrades: 0,
        isPrimary: false,
        enabled: true,
        name: newWalletData.name || `Wallet ${wallets.length + 1}`,
      };

      setWallets(prev => [...prev, newWallet]);
      setNewWalletData({ privateKey: '', name: '' });
      setShowAddWallet(false);
      toast.success('Wallet added successfully');
    } catch (error) {
      toast.error('Failed to add wallet');
    }
  };

  const handleRemoveWallet = (walletId: string) => {
    const wallet = wallets.find(w => w.id === walletId);
    if (wallet?.isPrimary) {
      toast.error('Cannot remove primary wallet');
      return;
    }

    if (wallet?.activeTrades && wallet.activeTrades > 0) {
      toast.error('Cannot remove wallet with active trades');
      return;
    }

    setWallets(prev => prev.filter(w => w.id !== walletId));
    toast.success('Wallet removed successfully');
  };

  const handleSetPrimary = (walletId: string) => {
    setWallets(prev => prev.map(wallet => ({
      ...wallet,
      isPrimary: wallet.id === walletId,
    })));
    toast.success('Primary wallet updated');
  };

  const handleToggleWallet = (walletId: string) => {
    setWallets(prev => prev.map(wallet => 
      wallet.id === walletId ? { ...wallet, enabled: !wallet.enabled } : wallet
    ));
  };

  const formatAddress = (address: string) => {
    return `${address.slice(0, 6)}...${address.slice(-4)}`;
  };

  const togglePrivateKeyVisibility = (walletId: string) => {
    setShowPrivateKeys(prev => ({
      ...prev,
      [walletId]: !prev[walletId],
    }));
  };

  return (
    <div className="space-y-6">
      {/* Add Wallet Section */}
      <Card className="p-6">
        <div className="flex items-center justify-between mb-4">
          <h3 className="text-lg font-semibold text-gray-100">Add New Wallet</h3>
          <Button
            onClick={() => setShowAddWallet(!showAddWallet)}
            size="sm"
          >
            <PlusIcon className="w-4 h-4 mr-2" />
            Add Wallet
          </Button>
        </div>

        {showAddWallet && (
          <div className="space-y-4">
            <div>
              <label className="block text-sm font-medium text-gray-300 mb-2">
                Wallet Name (Optional)
              </label>
              <Input
                value={newWalletData.name}
                onChange={(e) => setNewWalletData(prev => ({ ...prev, name: e.target.value }))}
                placeholder="e.g., Trading Wallet 2"
              />
            </div>

            <div>
              <label className="block text-sm font-medium text-gray-300 mb-2">
                Private Key
              </label>
              <div className="relative">
                <Input
                  type="password"
                  value={newWalletData.privateKey}
                  onChange={(e) => setNewWalletData(prev => ({ ...prev, privateKey: e.target.value }))}
                  placeholder="Enter wallet private key"
                  className="pr-10"
                />
              </div>
              <p className="text-xs text-yellow-400 mt-1">
                ⚠️ Private keys are encrypted and stored securely
              </p>
            </div>

            <div className="flex gap-3">
              <Button onClick={handleAddWallet}>
                Add Wallet
              </Button>
              <Button
                variant="outline"
                onClick={() => setShowAddWallet(false)}
              >
                Cancel
              </Button>
            </div>
          </div>
        )}
      </Card>

      {/* Wallets List */}
      <Card className="p-6">
        <div className="flex items-center justify-between mb-6">
          <h3 className="text-lg font-semibold text-gray-100">Managed Wallets</h3>
          <Badge variant="secondary">{wallets.length} Wallets</Badge>
        </div>

        <div className="space-y-4">
          {wallets.map((wallet) => (
            <div key={wallet.id} className="p-4 bg-gray-700/30 rounded-lg">
              <div className="flex items-center justify-between mb-3">
                <div className="flex items-center">
                  <WalletIcon className="w-6 h-6 text-blue-500 mr-3" />
                  <div>
                    <div className="flex items-center">
                      <span className="font-medium text-gray-100 mr-2">
                        {wallet.name || `Wallet ${wallet.id}`}
                      </span>
                      {wallet.isPrimary && (
                        <StarIcon className="w-4 h-4 text-yellow-500 fill-current" />
                      )}
                    </div>
                    <p className="text-sm text-gray-400">{formatAddress(wallet.address)}</p>
                  </div>
                </div>

                <div className="flex items-center gap-3">
                  <Switch
                    checked={wallet.enabled}
                    onCheckedChange={() => handleToggleWallet(wallet.id)}
                  />
                  
                  {!wallet.isPrimary && (
                    <Button
                      size="sm"
                      variant="outline"
                      onClick={() => handleSetPrimary(wallet.id)}
                    >
                      Set Primary
                    </Button>
                  )}
                  
                  {!wallet.isPrimary && wallet.activeTrades === 0 && (
                    <Button
                      size="sm"
                      variant="destructive"
                      onClick={() => handleRemoveWallet(wallet.id)}
                    >
                      <TrashIcon className="w-4 h-4" />
                    </Button>
                  )}
                </div>
              </div>

              <div className="grid grid-cols-3 gap-4 text-sm">
                <div>
                  <p className="text-gray-400">Balance</p>
                  <p className="font-semibold text-gray-100">{wallet.balance.toFixed(3)} ETH</p>
                </div>
                <div>
                  <p className="text-gray-400">Active Trades</p>
                  <p className="font-semibold text-gray-100">{wallet.activeTrades}</p>
                </div>
                <div>
                  <p className="text-gray-400">Status</p>
                  <Badge variant={wallet.enabled ? 'success' : 'secondary'}>
                    {wallet.enabled ? 'Active' : 'Disabled'}
                  </Badge>
                </div>
              </div>

              {wallet.isPrimary && (
                <div className="mt-3 p-2 bg-yellow-600/20 border border-yellow-600/30 rounded">
                  <p className="text-xs text-yellow-300">
                    🌟 Primary wallet - used for all new trades
                  </p>
                </div>
              )}
            </div>
          ))}
        </div>

        {wallets.length === 0 && (
          <div className="text-center py-8 text-gray-400">
            <WalletIcon className="w-12 h-12 mx-auto mb-3 opacity-50" />
            <p>No wallets configured</p>
            <p className="text-sm">Add a wallet to start trading</p>
          </div>
        )}
      </Card>

      {/* Wallet Settings */}
      <Card className="p-6">
        <h3 className="text-lg font-semibold text-gray-100 mb-4">Wallet Settings</h3>
        
        <div className="space-y-4">
          <div className="flex items-center justify-between">
            <div>
              <p className="text-sm font-medium text-gray-300">Auto-rotation</p>
              <p className="text-xs text-gray-400">Automatically rotate wallets to distribute risk</p>
            </div>
            <Switch checked={true} onCheckedChange={() => {}} />
          </div>

          <div className="flex items-center justify-between">
            <div>
              <p className="text-sm font-medium text-gray-300">Balance monitoring</p>
              <p className="text-xs text-gray-400">Monitor wallet balances and send alerts</p>
            </div>
            <Switch checked={true} onCheckedChange={() => {}} />
          </div>

          <div className="flex items-center justify-between">
            <div>
              <p className="text-sm font-medium text-gray-300">Gas optimization</p>
              <p className="text-xs text-gray-400">Optimize gas usage across wallets</p>
            </div>
            <Switch checked={true} onCheckedChange={() => {}} />
          </div>
        </div>
      </Card>
    </div>
  );
}
