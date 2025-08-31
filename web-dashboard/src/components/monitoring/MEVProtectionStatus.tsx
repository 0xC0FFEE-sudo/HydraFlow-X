'use client';

import { Card } from '@/components/ui/Card';
import { Badge } from '@/components/ui/Badge';
import { Switch } from '@/components/ui/Switch';
import { 
  ShieldCheckIcon, 
  ShieldExclamationIcon,
  BoltIcon,
  EyeSlashIcon
} from '@heroicons/react/24/outline';
import { useState } from 'react';

interface MEVProtectionStatusProps {
  expanded?: boolean;
}

export function MEVProtectionStatus({ expanded = false }: MEVProtectionStatusProps) {
  const [protectionSettings, setProtectionSettings] = useState({
    enableMEVProtection: true,
    enableSandwichProtection: true,
    enableFrontrunProtection: true,
    usePrivateRelay: true,
    enableJitoBundle: true,
  });

  const protectionStats = {
    attacksBlocked: 147,
    sandwichAttempts: 23,
    frontrunAttempts: 89,
    successRate: 97.3,
    activeRelays: 3,
  };

  const relayStatus = [
    { name: 'Flashbots', status: 'active', latency: '12ms' },
    { name: 'Eden Network', status: 'active', latency: '15ms' },
    { name: 'bloXroute', status: 'standby', latency: '18ms' },
    { name: 'Jito (Solana)', status: 'active', latency: '8ms' },
  ];

  const updateSetting = (key: string, value: boolean) => {
    setProtectionSettings(prev => ({ ...prev, [key]: value }));
  };

  return (
    <Card className="p-6">
      <div className="flex items-center justify-between mb-6">
        <div className="flex items-center">
          <ShieldCheckIcon className="w-6 h-6 text-green-500 mr-2" />
          <h3 className="text-lg font-semibold text-gray-100">MEV Protection</h3>
        </div>
        <Badge variant="success">97.3% Success Rate</Badge>
      </div>

      {/* Protection Statistics */}
      <div className="grid grid-cols-2 gap-4 mb-6">
        <div className="text-center p-3 bg-gray-700/30 rounded-lg">
          <ShieldCheckIcon className="w-8 h-8 text-green-500 mx-auto mb-2" />
          <p className="text-2xl font-bold text-gray-100">{protectionStats.attacksBlocked}</p>
          <p className="text-sm text-gray-400">Attacks Blocked</p>
        </div>
        
        <div className="text-center p-3 bg-gray-700/30 rounded-lg">
          <BoltIcon className="w-8 h-8 text-blue-500 mx-auto mb-2" />
          <p className="text-2xl font-bold text-gray-100">{protectionStats.activeRelays}</p>
          <p className="text-sm text-gray-400">Active Relays</p>
        </div>
      </div>

      {/* Protection Settings */}
      <div className="space-y-4 mb-6">
        <h4 className="text-sm font-medium text-gray-300">Protection Settings</h4>
        
        <div className="space-y-3">
          <div className="flex items-center justify-between">
            <div className="flex items-center">
              <ShieldCheckIcon className="w-4 h-4 text-green-500 mr-2" />
              <span className="text-sm text-gray-300">MEV Protection</span>
            </div>
            <Switch
              checked={protectionSettings.enableMEVProtection}
              onCheckedChange={(checked) => updateSetting('enableMEVProtection', checked)}
            />
          </div>
          
          <div className="flex items-center justify-between">
            <div className="flex items-center">
              <ShieldExclamationIcon className="w-4 h-4 text-yellow-500 mr-2" />
              <span className="text-sm text-gray-300">Sandwich Protection</span>
            </div>
            <Switch
              checked={protectionSettings.enableSandwichProtection}
              onCheckedChange={(checked) => updateSetting('enableSandwichProtection', checked)}
            />
          </div>
          
          <div className="flex items-center justify-between">
            <div className="flex items-center">
              <BoltIcon className="w-4 h-4 text-blue-500 mr-2" />
              <span className="text-sm text-gray-300">Frontrun Protection</span>
            </div>
            <Switch
              checked={protectionSettings.enableFrontrunProtection}
              onCheckedChange={(checked) => updateSetting('enableFrontrunProtection', checked)}
            />
          </div>
          
          <div className="flex items-center justify-between">
            <div className="flex items-center">
              <EyeSlashIcon className="w-4 h-4 text-purple-500 mr-2" />
              <span className="text-sm text-gray-300">Private Relay</span>
            </div>
            <Switch
              checked={protectionSettings.usePrivateRelay}
              onCheckedChange={(checked) => updateSetting('usePrivateRelay', checked)}
            />
          </div>
        </div>
      </div>

      {/* Relay Status */}
      {expanded && (
        <div className="space-y-3">
          <h4 className="text-sm font-medium text-gray-300">Relay Status</h4>
          {relayStatus.map((relay, index) => (
            <div key={index} className="flex items-center justify-between p-3 bg-gray-700/30 rounded-lg">
              <div className="flex items-center">
                <div className={`w-3 h-3 rounded-full mr-3 ${
                  relay.status === 'active' ? 'bg-green-500' : 'bg-yellow-500'
                }`} />
                <span className="text-sm text-gray-100">{relay.name}</span>
              </div>
              <div className="flex items-center space-x-3">
                <span className="text-xs text-gray-400">{relay.latency}</span>
                <Badge variant={relay.status === 'active' ? 'success' : 'warning'}>
                  {relay.status}
                </Badge>
              </div>
            </div>
          ))}
        </div>
      )}

      {!expanded && (
        <div className="grid grid-cols-2 gap-4 text-sm">
          <div>
            <p className="text-gray-400">Sandwich Blocked</p>
            <p className="font-semibold text-yellow-400">{protectionStats.sandwichAttempts}</p>
          </div>
          <div>
            <p className="text-gray-400">Frontrun Blocked</p>
            <p className="font-semibold text-blue-400">{protectionStats.frontrunAttempts}</p>
          </div>
        </div>
      )}
    </Card>
  );
}
