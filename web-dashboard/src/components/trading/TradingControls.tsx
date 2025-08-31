'use client';

import { useState } from 'react';
import { PlayIcon, PauseIcon, StopIcon, CogIcon } from '@heroicons/react/24/outline';
import { Button } from '@/components/ui/Button';
import { Card } from '@/components/ui/Card';
import { Switch } from '@/components/ui/Switch';
import { Select } from '@/components/ui/Select';
import { Input } from '@/components/ui/Input';
import { Badge } from '@/components/ui/Badge';
import toast from 'react-hot-toast';

interface TradingControlsProps {
  expanded?: boolean;
}

export function TradingControls({ expanded = false }: TradingControlsProps) {
  const [isTrading, setIsTrading] = useState(false);
  const [tradingMode, setTradingMode] = useState('STANDARD_BUY');
  const [settings, setSettings] = useState({
    maxPositionSize: 10000,
    maxSlippage: 2.0,
    enableMEVProtection: true,
    enableSniperMode: false,
    enableCopyTrading: false,
    enableAutonomousMode: false,
  });

  const handleStartTrading = async () => {
    try {
      const response = await fetch('/api/trading/start', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ mode: tradingMode, settings }),
      });
      
      if (response.ok) {
        setIsTrading(true);
        toast.success('Trading started successfully');
      } else {
        throw new Error('Failed to start trading');
      }
    } catch (error) {
      toast.error('Failed to start trading');
    }
  };

  const handleStopTrading = async () => {
    try {
      const response = await fetch('/api/trading/stop', { method: 'POST' });
      
      if (response.ok) {
        setIsTrading(false);
        toast.success('Trading stopped successfully');
      } else {
        throw new Error('Failed to stop trading');
      }
    } catch (error) {
      toast.error('Failed to stop trading');
    }
  };

  const tradingModes = [
    { value: 'STANDARD_BUY', label: 'Standard Trading' },
    { value: 'SNIPER_MODE', label: 'Sniper Mode' },
    { value: 'COPY_TRADING', label: 'Copy Trading' },
    { value: 'AUTONOMOUS_MODE', label: 'Autonomous Mode' },
    { value: 'MULTI_WALLET', label: 'Multi-Wallet' },
  ];

  return (
    <Card className="p-6">
      <div className="flex items-center justify-between mb-6">
        <h3 className="text-lg font-semibold text-gray-100">Trading Controls</h3>
        <Badge variant={isTrading ? 'success' : 'secondary'}>
          {isTrading ? 'ACTIVE' : 'STOPPED'}
        </Badge>
      </div>

      <div className="space-y-4">
        {/* Main Controls */}
        <div className="flex gap-2">
          {!isTrading ? (
            <Button
              onClick={handleStartTrading}
              className="flex-1 bg-green-600 hover:bg-green-700"
            >
              <PlayIcon className="w-4 h-4 mr-2" />
              Start Trading
            </Button>
          ) : (
            <Button
              onClick={handleStopTrading}
              className="flex-1 bg-red-600 hover:bg-red-700"
            >
              <StopIcon className="w-4 h-4 mr-2" />
              Stop Trading
            </Button>
          )}
        </div>

        {/* Trading Mode Selection */}
        <div>
          <label className="block text-sm font-medium text-gray-300 mb-2">
            Trading Mode
          </label>
          <Select
            value={tradingMode}
            onValueChange={setTradingMode}
            disabled={isTrading}
          >
            {tradingModes.map((mode) => (
              <option key={mode.value} value={mode.value}>
                {mode.label}
              </option>
            ))}
          </Select>
        </div>

        {expanded && (
          <>
            {/* Trading Settings */}
            <div className="space-y-4 pt-4 border-t border-gray-700">
              <h4 className="text-sm font-medium text-gray-300">Settings</h4>
              
              <div className="grid grid-cols-2 gap-4">
                <div>
                  <label className="block text-xs text-gray-400 mb-1">
                    Max Position Size (USD)
                  </label>
                  <Input
                    type="number"
                    value={settings.maxPositionSize}
                    onChange={(e) => setSettings(prev => ({
                      ...prev,
                      maxPositionSize: Number(e.target.value)
                    }))}
                    disabled={isTrading}
                  />
                </div>
                
                <div>
                  <label className="block text-xs text-gray-400 mb-1">
                    Max Slippage (%)
                  </label>
                  <Input
                    type="number"
                    step="0.1"
                    value={settings.maxSlippage}
                    onChange={(e) => setSettings(prev => ({
                      ...prev,
                      maxSlippage: Number(e.target.value)
                    }))}
                    disabled={isTrading}
                  />
                </div>
              </div>

              {/* Feature Toggles */}
              <div className="space-y-3">
                <div className="flex items-center justify-between">
                  <span className="text-sm text-gray-300">MEV Protection</span>
                  <Switch
                    checked={settings.enableMEVProtection}
                    onCheckedChange={(checked) => setSettings(prev => ({
                      ...prev,
                      enableMEVProtection: checked
                    }))}
                    disabled={isTrading}
                  />
                </div>
                
                <div className="flex items-center justify-between">
                  <span className="text-sm text-gray-300">Sniper Mode</span>
                  <Switch
                    checked={settings.enableSniperMode}
                    onCheckedChange={(checked) => setSettings(prev => ({
                      ...prev,
                      enableSniperMode: checked
                    }))}
                    disabled={isTrading}
                  />
                </div>
                
                <div className="flex items-center justify-between">
                  <span className="text-sm text-gray-300">Copy Trading</span>
                  <Switch
                    checked={settings.enableCopyTrading}
                    onCheckedChange={(checked) => setSettings(prev => ({
                      ...prev,
                      enableCopyTrading: checked
                    }))}
                    disabled={isTrading}
                  />
                </div>
                
                <div className="flex items-center justify-between">
                  <span className="text-sm text-gray-300">Autonomous Mode</span>
                  <Switch
                    checked={settings.enableAutonomousMode}
                    onCheckedChange={(checked) => setSettings(prev => ({
                      ...prev,
                      enableAutonomousMode: checked
                    }))}
                    disabled={isTrading}
                  />
                </div>
              </div>
            </div>
          </>
        )}
      </div>
    </Card>
  );
}
