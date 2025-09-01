'use client';

import { useState } from 'react';
import { motion } from 'framer-motion';
import { Card, CardHeader, CardTitle, CardContent } from '@/components/ui/Card';
import { Badge, StatusBadge } from '@/components/ui/Badge';
import { Button } from '@/components/ui/Button';
import { FeatureSwitch } from '@/components/ui/Switch';
import { formatters } from '@/lib/utils';
import { 
  Shield,
  ShieldCheck,
  ShieldX,
  AlertTriangle,
  Activity,
  Clock,
  DollarSign,
  Zap,
  Settings,
  TrendingUp,
  CheckCircle,
  XCircle,
  Package,
} from 'lucide-react';

export function MEVDashboard() {
  const [mevConfig, setMevConfig] = useState({
    enabled: true,
    mode: 'aggressive' as 'aggressive' | 'balanced' | 'conservative',
    sandwichProtection: true,
    frontrunProtection: true,
    privateRelay: true,
    priorityFee: 2.5,
  });

  const mevStats = {
    protectionRate: 97.3,
    bundlesSubmitted: 1247,
    bundlesIncluded: 1213,
    protectedValue: 2847592.50,
    savedFromMev: 15847.25,
    relaysConnected: 8,
  };

  const recentBundles = [
    { id: '1', hash: '0x1a2b3c...', status: 'included', block: 18500245, gasPrice: 25.5, profit: 125.75, timestamp: Date.now() - 30000 },
    { id: '2', hash: '0x4d5e6f...', status: 'pending', block: 18500246, gasPrice: 28.2, profit: null, timestamp: Date.now() - 45000 },
    { id: '3', hash: '0x7g8h9i...', status: 'failed', block: 18500244, gasPrice: 22.1, profit: null, timestamp: Date.now() - 120000 },
    { id: '4', hash: '0xj1k2l3...', status: 'included', block: 18500243, gasPrice: 30.8, profit: 67.25, timestamp: Date.now() - 180000 },
  ];

  const protectionLogs = [
    { id: '1', type: 'sandwich', message: 'Sandwich attack blocked on ETH/USDC', amount: 2500.75, timestamp: Date.now() - 60000 },
    { id: '2', type: 'frontrun', message: 'Frontrun attempt detected and prevented', amount: 890.25, timestamp: Date.now() - 120000 },
    { id: '3', type: 'bundle', message: 'Bundle successfully submitted via Flashbots', amount: null, timestamp: Date.now() - 180000 },
    { id: '4', type: 'sandwich', message: 'MEV protection saved transaction', amount: 1250.50, timestamp: Date.now() - 240000 },
  ];

  const container = {
    hidden: { opacity: 0 },
    show: { opacity: 1, transition: { staggerChildren: 0.1 } },
  };

  const item = {
    hidden: { y: 20, opacity: 0 },
    show: { y: 0, opacity: 1 },
  };

  const handleConfigChange = (key: string, value: any) => {
    setMevConfig(prev => ({ ...prev, [key]: value }));
  };

  return (
    <motion.div
      variants={container}
      initial="hidden"
      animate="show"
      className="space-y-6 h-full overflow-auto custom-scrollbar"
    >
      {/* Header */}
      <motion.div variants={item} className="flex items-center justify-between">
        <div>
          <h1 className="text-3xl font-bold gradient-text">MEV Protection</h1>
          <p className="text-secondary-400 mt-1">
            Advanced MEV protection and bundle management
          </p>
        </div>
        <div className="flex items-center space-x-3">
          <StatusBadge status={mevConfig.enabled ? 'healthy' : 'error'} />
          <Badge variant="success" className="animate-pulse">
            {mevConfig.mode.toUpperCase()} Mode
          </Badge>
        </div>
      </motion.div>

      {/* MEV Protection Overview */}
      <motion.div variants={item} className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-4 gap-6">
        <Card variant="glass" glow="subtle">
          <CardContent className="space-y-3">
            <div className="flex items-center justify-between">
              <ShieldCheck className="w-8 h-8 text-success-400" />
              <TrendingUp className="w-4 h-4 text-success-400" />
            </div>
            <div>
              <p className="metric-label">Protection Rate</p>
              <p className="metric-value">{mevStats.protectionRate}%</p>
            </div>
          </CardContent>
        </Card>

        <Card variant="glass">
          <CardContent className="space-y-3">
            <div className="flex items-center justify-between">
              <Package className="w-8 h-8 text-primary-400" />
              <Activity className="w-4 h-4 text-primary-400" />
            </div>
            <div>
              <p className="metric-label">Bundles Included</p>
              <p className="metric-value">{mevStats.bundlesIncluded}/{mevStats.bundlesSubmitted}</p>
            </div>
          </CardContent>
        </Card>

        <Card variant="glass">
          <CardContent className="space-y-3">
            <div className="flex items-center justify-between">
              <DollarSign className="w-8 h-8 text-success-400" />
              <Shield className="w-4 h-4 text-success-400" />
            </div>
            <div>
              <p className="metric-label">Value Protected</p>
              <p className="metric-value">{formatters.compact(mevStats.protectedValue)}</p>
            </div>
          </CardContent>
        </Card>

        <Card variant="glass">
          <CardContent className="space-y-3">
            <div className="flex items-center justify-between">
              <Zap className="w-8 h-8 text-warning-400" />
              <Activity className="w-4 h-4 text-warning-400" />
            </div>
            <div>
              <p className="metric-label">Saved from MEV</p>
              <p className="metric-value">{formatters.currency(mevStats.savedFromMev)}</p>
            </div>
          </CardContent>
        </Card>
      </motion.div>

      {/* Configuration and Status */}
      <div className="grid grid-cols-1 xl:grid-cols-3 gap-6">
        {/* MEV Configuration */}
        <motion.div variants={item}>
          <Card variant="glass" className="h-[400px]">
            <CardHeader>
              <CardTitle className="flex items-center space-x-2">
                <Settings className="w-5 h-5 text-primary-400" />
                <span>Protection Settings</span>
              </CardTitle>
            </CardHeader>
            <CardContent className="space-y-4">
              <FeatureSwitch
                feature={{
                  name: "MEV Protection",
                  description: "Enable MEV protection globally",
                  icon: <Shield className="w-4 h-4" />
                }}
                enabled={mevConfig.enabled}
                onToggle={(enabled) => handleConfigChange('enabled', enabled)}
                variant="success"
              />

              <FeatureSwitch
                feature={{
                  name: "Sandwich Protection",
                  description: "Protect against sandwich attacks",
                  icon: <ShieldCheck className="w-4 h-4" />
                }}
                enabled={mevConfig.sandwichProtection}
                onToggle={(enabled) => handleConfigChange('sandwichProtection', enabled)}
                variant="success"
                disabled={!mevConfig.enabled}
              />

              <FeatureSwitch
                feature={{
                  name: "Frontrun Protection",
                  description: "Detect and prevent frontrunning",
                  icon: <ShieldX className="w-4 h-4" />
                }}
                enabled={mevConfig.frontrunProtection}
                onToggle={(enabled) => handleConfigChange('frontrunProtection', enabled)}
                variant="warning"
                disabled={!mevConfig.enabled}
              />

              <FeatureSwitch
                feature={{
                  name: "Private Relay",
                  description: "Use private mempool relays",
                  icon: <Zap className="w-4 h-4" />
                }}
                enabled={mevConfig.privateRelay}
                onToggle={(enabled) => handleConfigChange('privateRelay', enabled)}
                variant="default"
                disabled={!mevConfig.enabled}
              />

              <div className="pt-4 border-t border-white/10">
                <div className="space-y-2">
                  <label className="text-sm font-medium text-white">
                    Protection Mode
                  </label>
                  <div className="grid grid-cols-3 gap-2">
                    {(['conservative', 'balanced', 'aggressive'] as const).map((mode) => (
                      <Button
                        key={mode}
                        variant={mevConfig.mode === mode ? 'default' : 'outline'}
                        size="sm"
                        onClick={() => handleConfigChange('mode', mode)}
                        disabled={!mevConfig.enabled}
                      >
                        {mode}
                      </Button>
                    ))}
                  </div>
                </div>
              </div>
            </CardContent>
          </Card>
        </motion.div>

        {/* Recent Bundles */}
        <motion.div variants={item}>
          <Card variant="glass" className="h-[400px]">
            <CardHeader>
              <CardTitle className="flex items-center space-x-2">
                <Package className="w-5 h-5 text-primary-400" />
                <span>Recent Bundles</span>
              </CardTitle>
            </CardHeader>
            <CardContent className="space-y-3 overflow-auto">
              {recentBundles.map((bundle) => (
                <div
                  key={bundle.id}
                  className="p-3 rounded-lg bg-white/5 border border-white/10"
                >
                  <div className="flex items-center justify-between mb-2">
                    <code className="text-sm text-primary-400">{bundle.hash}</code>
                    <div className="flex items-center space-x-1">
                      {bundle.status === 'included' && <CheckCircle className="w-4 h-4 text-success-400" />}
                      {bundle.status === 'pending' && <Clock className="w-4 h-4 text-warning-400" />}
                      {bundle.status === 'failed' && <XCircle className="w-4 h-4 text-danger-400" />}
                      <Badge 
                        variant={
                          bundle.status === 'included' ? 'success' :
                          bundle.status === 'pending' ? 'warning' : 'danger'
                        }
                        size="sm"
                      >
                        {bundle.status}
                      </Badge>
                    </div>
                  </div>
                  
                  <div className="grid grid-cols-2 gap-2 text-xs">
                    <div>
                      <span className="text-secondary-400">Block:</span>
                      <span className="ml-1 text-white">{bundle.block.toLocaleString()}</span>
                    </div>
                    <div>
                      <span className="text-secondary-400">Gas:</span>
                      <span className="ml-1 text-white">{bundle.gasPrice} gwei</span>
                    </div>
                    {bundle.profit && (
                      <div className="col-span-2">
                        <span className="text-secondary-400">Profit:</span>
                        <span className="ml-1 text-success-400">
                          +{formatters.currency(bundle.profit)}
                        </span>
                      </div>
                    )}
                  </div>
                  
                  <div className="text-xs text-secondary-400 mt-2">
                    {formatters.timeAgo(bundle.timestamp)}
                  </div>
                </div>
              ))}
            </CardContent>
          </Card>
        </motion.div>

        {/* Protection Activity */}
        <motion.div variants={item}>
          <Card variant="glass" className="h-[400px]">
            <CardHeader>
              <CardTitle className="flex items-center space-x-2">
                <AlertTriangle className="w-5 h-5 text-warning-400" />
                <span>Protection Activity</span>
              </CardTitle>
            </CardHeader>
            <CardContent className="space-y-3 overflow-auto">
              {protectionLogs.map((log) => (
                <div
                  key={log.id}
                  className="p-3 rounded-lg bg-white/5 border border-white/10"
                >
                  <div className="flex items-start space-x-3">
                    <div className="mt-1">
                      {log.type === 'sandwich' && <ShieldCheck className="w-4 h-4 text-success-400" />}
                      {log.type === 'frontrun' && <ShieldX className="w-4 h-4 text-warning-400" />}
                      {log.type === 'bundle' && <Package className="w-4 h-4 text-primary-400" />}
                    </div>
                    <div className="flex-1">
                      <p className="text-sm text-white">{log.message}</p>
                      {log.amount && (
                        <p className="text-xs text-success-400 mt-1">
                          Saved: {formatters.currency(log.amount)}
                        </p>
                      )}
                      <p className="text-xs text-secondary-400 mt-1">
                        {formatters.timeAgo(log.timestamp)}
                      </p>
                    </div>
                  </div>
                </div>
              ))}
            </CardContent>
          </Card>
        </motion.div>
      </div>

      {/* Relay Status */}
      <motion.div variants={item}>
        <Card variant="glass">
          <CardHeader>
            <CardTitle className="flex items-center space-x-2">
              <Activity className="w-5 h-5 text-primary-400" />
              <span>Relay Network Status</span>
            </CardTitle>
          </CardHeader>
          <CardContent>
            <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-4 gap-4">
              {[
                { name: 'Flashbots', status: 'connected', latency: 12.5, bundles: 456 },
                { name: 'Eden Network', status: 'connected', latency: 18.2, bundles: 234 },
                { name: 'BloXroute', status: 'connected', latency: 22.1, bundles: 189 },
                { name: 'Manifold', status: 'warning', latency: 45.3, bundles: 67 },
              ].map((relay) => (
                <div
                  key={relay.name}
                  className="p-4 rounded-lg bg-white/5 border border-white/10"
                >
                  <div className="flex items-center justify-between mb-2">
                    <span className="font-medium text-white">{relay.name}</span>
                    <StatusBadge 
                      status={relay.status === 'connected' ? 'healthy' : 'warning'} 
                    />
                  </div>
                  <div className="text-sm space-y-1">
                    <div className="flex justify-between">
                      <span className="text-secondary-400">Latency:</span>
                      <span className="text-white">{relay.latency}ms</span>
                    </div>
                    <div className="flex justify-between">
                      <span className="text-secondary-400">Bundles:</span>
                      <span className="text-white">{relay.bundles}</span>
                    </div>
                  </div>
                </div>
              ))}
            </div>
          </CardContent>
        </Card>
      </motion.div>
    </motion.div>
  );
}
