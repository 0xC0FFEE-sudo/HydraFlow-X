'use client';

import React, { useState, useEffect } from 'react';
import { Card } from '../ui/Card';
import { Button } from '../ui/Button';
import { Badge } from '../ui/Badge';
import { Switch } from '../ui/Switch';
import { Select } from '../ui/Select';
import { Input } from '../ui/Input';
import { Tabs } from '../ui/Tabs';

interface RiskLimit {
  id: string;
  name: string;
  type: 'position_size' | 'daily_loss' | 'drawdown' | 'var' | 'leverage' | 'concentration';
  currentValue: number;
  limitValue: number;
  warningThreshold: number;
  enabled: boolean;
  status: 'safe' | 'warning' | 'breached';
  description: string;
}

interface RiskEvent {
  id: string;
  timestamp: number;
  type: 'limit_breach' | 'warning' | 'emergency_stop' | 'manual_override';
  severity: 'low' | 'medium' | 'high' | 'critical';
  message: string;
  affectedAssets: string[];
  action: string;
  resolved: boolean;
}

interface EmergencyAction {
  id: string;
  name: string;
  description: string;
  type: 'stop_all' | 'close_positions' | 'hedge_portfolio' | 'reduce_leverage' | 'pause_strategy';
  enabled: boolean;
  automated: boolean;
  confirmationRequired: boolean;
}

interface RiskScenario {
  id: string;
  name: string;
  description: string;
  probability: number;
  impactAmount: number;
  impactPercent: number;
  timeframe: string;
  mitigation: string;
  status: 'monitoring' | 'hedged' | 'ignored';
}

interface PositionLimit {
  symbol: string;
  maxPositionSize: number;
  currentPosition: number;
  utilizationPercent: number;
  stopLoss: number;
  takeProfit: number;
  enabled: boolean;
}

const RISK_TIMEFRAMES = [
  { value: 'real_time', label: 'Real Time' },
  { value: '1h', label: '1 Hour' },
  { value: '1d', label: '1 Day' },
  { value: '1w', label: '1 Week' },
];

export const RiskManagementDashboard: React.FC = () => {
  const [activeTab, setActiveTab] = useState('overview');
  const [timeframe, setTimeframe] = useState('real_time');
  const [riskLimits, setRiskLimits] = useState<RiskLimit[]>([]);
  const [riskEvents, setRiskEvents] = useState<RiskEvent[]>([]);
  const [emergencyActions, setEmergencyActions] = useState<EmergencyAction[]>([]);
  const [riskScenarios, setRiskScenarios] = useState<RiskScenario[]>([]);
  const [positionLimits, setPositionLimits] = useState<PositionLimit[]>([]);
  const [emergencyMode, setEmergencyMode] = useState(false);

  // Generate mock data
  useEffect(() => {
    generateMockRiskData();
  }, []);

  const generateMockRiskData = () => {
    // Risk Limits
    const limits: RiskLimit[] = [
      {
        id: 'max_position',
        name: 'Maximum Position Size',
        type: 'position_size',
        currentValue: 15.2,
        limitValue: 20.0,
        warningThreshold: 18.0,
        enabled: true,
        status: 'safe',
        description: 'Maximum single position as % of portfolio',
      },
      {
        id: 'daily_loss',
        name: 'Daily Loss Limit',
        type: 'daily_loss',
        currentValue: -3.2,
        limitValue: -5.0,
        warningThreshold: -4.0,
        enabled: true,
        status: 'safe',
        description: 'Maximum daily portfolio loss',
      },
      {
        id: 'max_drawdown',
        name: 'Maximum Drawdown',
        type: 'drawdown',
        currentValue: -8.7,
        limitValue: -15.0,
        warningThreshold: -12.0,
        enabled: true,
        status: 'safe',
        description: 'Maximum portfolio drawdown from peak',
      },
      {
        id: 'portfolio_var',
        name: 'Portfolio VaR (95%)',
        type: 'var',
        currentValue: -12000,
        limitValue: -20000,
        warningThreshold: -17000,
        enabled: true,
        status: 'safe',
        description: 'Value at Risk at 95% confidence',
      },
      {
        id: 'leverage_ratio',
        name: 'Leverage Ratio',
        type: 'leverage',
        currentValue: 1.3,
        limitValue: 2.0,
        warningThreshold: 1.8,
        enabled: true,
        status: 'safe',
        description: 'Portfolio leverage multiplier',
      },
      {
        id: 'concentration',
        name: 'Concentration Risk',
        type: 'concentration',
        currentValue: 25.4,
        limitValue: 30.0,
        warningThreshold: 28.0,
        enabled: true,
        status: 'warning',
        description: 'Largest position concentration',
      },
    ];

    // Update status based on current values
    limits.forEach(limit => {
      if (limit.type === 'daily_loss' || limit.type === 'drawdown' || limit.type === 'var') {
        // For negative limits (loss, drawdown, VaR)
        if (limit.currentValue <= limit.limitValue) {
          limit.status = 'breached';
        } else if (limit.currentValue <= limit.warningThreshold) {
          limit.status = 'warning';
        } else {
          limit.status = 'safe';
        }
      } else {
        // For positive limits (position size, leverage, concentration)
        if (limit.currentValue >= limit.limitValue) {
          limit.status = 'breached';
        } else if (limit.currentValue >= limit.warningThreshold) {
          limit.status = 'warning';
        } else {
          limit.status = 'safe';
        }
      }
    });

    setRiskLimits(limits);

    // Risk Events
    const events: RiskEvent[] = [
      {
        id: 'event_1',
        timestamp: Date.now() - 3600000, // 1 hour ago
        type: 'warning',
        severity: 'medium',
        message: 'BTC position approaching concentration limit (25.4%)',
        affectedAssets: ['BTC'],
        action: 'Monitor closely',
        resolved: false,
      },
      {
        id: 'event_2',
        timestamp: Date.now() - 7200000, // 2 hours ago
        type: 'limit_breach',
        severity: 'high',
        message: 'Daily loss limit temporarily exceeded',
        affectedAssets: ['Portfolio'],
        action: 'Position sizing reduced automatically',
        resolved: true,
      },
      {
        id: 'event_3',
        timestamp: Date.now() - 14400000, // 4 hours ago
        type: 'manual_override',
        severity: 'low',
        message: 'Risk manager manually disabled ETH stop loss',
        affectedAssets: ['ETH'],
        action: 'Manual override logged',
        resolved: true,
      },
    ];

    setRiskEvents(events);

    // Emergency Actions
    const actions: EmergencyAction[] = [
      {
        id: 'stop_all',
        name: 'Emergency Stop All',
        description: 'Immediately stop all trading and close pending orders',
        type: 'stop_all',
        enabled: true,
        automated: false,
        confirmationRequired: true,
      },
      {
        id: 'close_positions',
        name: 'Close All Positions',
        description: 'Close all open positions at market prices',
        type: 'close_positions',
        enabled: true,
        automated: false,
        confirmationRequired: true,
      },
      {
        id: 'reduce_leverage',
        name: 'Reduce Leverage',
        description: 'Automatically reduce leverage to safe levels',
        type: 'reduce_leverage',
        enabled: true,
        automated: true,
        confirmationRequired: false,
      },
      {
        id: 'hedge_portfolio',
        name: 'Hedge Portfolio',
        description: 'Apply hedging strategies to reduce risk',
        type: 'hedge_portfolio',
        enabled: false,
        automated: false,
        confirmationRequired: true,
      },
    ];

    setEmergencyActions(actions);

    // Risk Scenarios
    const scenarios: RiskScenario[] = [
      {
        id: 'market_crash',
        name: 'Market Crash (-30%)',
        description: 'Broad crypto market decline of 30%',
        probability: 15,
        impactAmount: -45000,
        impactPercent: -30,
        timeframe: '1-7 days',
        mitigation: 'Diversification, stop losses',
        status: 'monitoring',
      },
      {
        id: 'flash_crash',
        name: 'Flash Crash (-15%)',
        description: 'Sudden liquidity crisis causing flash crash',
        probability: 8,
        impactAmount: -22500,
        impactPercent: -15,
        timeframe: 'Minutes to hours',
        mitigation: 'Circuit breakers, position limits',
        status: 'hedged',
      },
      {
        id: 'regulatory_ban',
        name: 'Regulatory Restriction',
        description: 'Major regulatory announcement affecting trading',
        probability: 25,
        impactAmount: -30000,
        impactPercent: -20,
        timeframe: 'Days to weeks',
        mitigation: 'Geographic diversification',
        status: 'monitoring',
      },
    ];

    setRiskScenarios(scenarios);

    // Position Limits
    const positions: PositionLimit[] = [
      { symbol: 'BTC', maxPositionSize: 50000, currentPosition: 38000, utilizationPercent: 76, stopLoss: -5, takeProfit: 15, enabled: true },
      { symbol: 'ETH', maxPositionSize: 30000, currentPosition: 22000, utilizationPercent: 73, stopLoss: -4, takeProfit: 12, enabled: true },
      { symbol: 'SOL', maxPositionSize: 15000, currentPosition: 8500, utilizationPercent: 57, stopLoss: -6, takeProfit: 18, enabled: true },
      { symbol: 'MATIC', maxPositionSize: 10000, currentPosition: 5200, utilizationPercent: 52, stopLoss: -7, takeProfit: 20, enabled: true },
    ];

    setPositionLimits(positions);
  };

  const formatCurrency = (value: number): string => {
    return new Intl.NumberFormat('en-US', {
      style: 'currency',
      currency: 'USD',
      minimumFractionDigits: 0,
      maximumFractionDigits: 0,
    }).format(value);
  };

  const formatPercent = (value: number): string => {
    return `${value >= 0 ? '+' : ''}${value.toFixed(2)}%`;
  };

  const getStatusColor = (status: string): string => {
    switch (status) {
      case 'safe': return 'text-green-400';
      case 'warning': return 'text-yellow-400';
      case 'breached': return 'text-red-400';
      default: return 'text-gray-400';
    }
  };

  const getStatusBadge = (status: string) => {
    switch (status) {
      case 'safe':
        return <Badge variant="success">Safe</Badge>;
      case 'warning':
        return <Badge className="bg-yellow-600">Warning</Badge>;
      case 'breached':
        return <Badge variant="destructive">Breached</Badge>;
      default:
        return <Badge variant="secondary">{status}</Badge>;
    }
  };

  const getSeverityBadge = (severity: string) => {
    switch (severity) {
      case 'low':
        return <Badge variant="secondary">Low</Badge>;
      case 'medium':
        return <Badge className="bg-yellow-600">Medium</Badge>;
      case 'high':
        return <Badge className="bg-orange-600">High</Badge>;
      case 'critical':
        return <Badge variant="destructive">Critical</Badge>;
      default:
        return <Badge variant="secondary">{severity}</Badge>;
    }
  };

  const executeEmergencyAction = (actionId: string) => {
    const action = emergencyActions.find(a => a.id === actionId);
    if (!action) return;

    if (action.confirmationRequired) {
      if (!confirm(`Are you sure you want to execute "${action.name}"? This action cannot be undone.`)) {
        return;
      }
    }

    console.log(`Executing emergency action: ${action.name}`);
    
    // Add to risk events
    const newEvent: RiskEvent = {
      id: `emergency_${Date.now()}`,
      timestamp: Date.now(),
      type: 'emergency_stop',
      severity: 'critical',
      message: `Emergency action executed: ${action.name}`,
      affectedAssets: ['Portfolio'],
      action: action.description,
      resolved: false,
    };

    setRiskEvents(prev => [newEvent, ...prev]);
    setEmergencyMode(true);
  };

  const toggleRiskLimit = (limitId: string) => {
    setRiskLimits(prev => prev.map(limit =>
      limit.id === limitId ? { ...limit, enabled: !limit.enabled } : limit
    ));
  };

  const renderOverview = () => (
    <div className="space-y-6">
      {/* Emergency Mode Alert */}
      {emergencyMode && (
        <Card className="p-4 border-l-4 border-red-500 bg-red-900/20">
          <div className="flex items-center justify-between">
            <div>
              <h3 className="text-lg font-semibold text-red-400">🚨 Emergency Mode Active</h3>
              <p className="text-red-300">All trading has been suspended. Manual intervention required.</p>
            </div>
            <Button variant="outline" onClick={() => setEmergencyMode(false)}>
              Resolve
            </Button>
          </div>
        </Card>
      )}

      {/* Risk Status Summary */}
      <div className="grid grid-cols-2 md:grid-cols-4 gap-4">
        <Card className="p-4">
          <div className="text-sm text-gray-400">Active Limits</div>
          <div className="text-2xl font-bold text-white">
            {riskLimits.filter(l => l.enabled).length}/{riskLimits.length}
          </div>
        </Card>

        <Card className="p-4">
          <div className="text-sm text-gray-400">Breached Limits</div>
          <div className="text-2xl font-bold text-red-400">
            {riskLimits.filter(l => l.status === 'breached').length}
          </div>
        </Card>

        <Card className="p-4">
          <div className="text-sm text-gray-400">Warning Limits</div>
          <div className="text-2xl font-bold text-yellow-400">
            {riskLimits.filter(l => l.status === 'warning').length}
          </div>
        </Card>

        <Card className="p-4">
          <div className="text-sm text-gray-400">Risk Score</div>
          <div className="text-2xl font-bold text-white">7.2/10</div>
        </Card>
      </div>

      {/* Risk Limits */}
      <Card className="p-6">
        <h3 className="text-lg font-semibold text-white mb-4">Risk Limits</h3>
        <div className="space-y-4">
          {riskLimits.map(limit => (
            <div key={limit.id} className="flex items-center justify-between p-4 bg-gray-800 rounded-lg">
              <div className="flex items-center space-x-4">
                <Switch
                  checked={limit.enabled}
                  onCheckedChange={() => toggleRiskLimit(limit.id)}
                />
                <div>
                  <div className="font-semibold text-white">{limit.name}</div>
                  <div className="text-sm text-gray-400">{limit.description}</div>
                </div>
              </div>
              
              <div className="flex items-center space-x-4">
                <div className="text-right">
                  <div className="text-white">
                    {limit.type === 'var' ? formatCurrency(limit.currentValue) : 
                     limit.type === 'leverage' ? limit.currentValue.toFixed(2) + 'x' :
                     formatPercent(limit.currentValue)}
                  </div>
                  <div className="text-sm text-gray-400">
                    Limit: {limit.type === 'var' ? formatCurrency(limit.limitValue) : 
                            limit.type === 'leverage' ? limit.limitValue.toFixed(2) + 'x' :
                            formatPercent(limit.limitValue)}
                  </div>
                </div>
                {getStatusBadge(limit.status)}
              </div>
            </div>
          ))}
        </div>
      </Card>

      {/* Recent Risk Events */}
      <Card className="p-6">
        <h3 className="text-lg font-semibold text-white mb-4">Recent Risk Events</h3>
        <div className="space-y-3">
          {riskEvents.slice(0, 5).map(event => (
            <div key={event.id} className="flex items-start justify-between p-3 bg-gray-800 rounded-lg">
              <div className="flex-1">
                <div className="flex items-center space-x-2 mb-1">
                  {getSeverityBadge(event.severity)}
                  <span className="text-sm text-gray-400 uppercase">{event.type.replace('_', ' ')}</span>
                  <span className="text-xs text-gray-500">
                    {new Date(event.timestamp).toLocaleTimeString()}
                  </span>
                </div>
                <div className="text-white mb-1">{event.message}</div>
                <div className="text-sm text-gray-400">Action: {event.action}</div>
              </div>
              <div>
                <Badge variant={event.resolved ? 'success' : 'destructive'}>
                  {event.resolved ? 'Resolved' : 'Active'}
                </Badge>
              </div>
            </div>
          ))}
        </div>
      </Card>
    </div>
  );

  const renderEmergencyActions = () => (
    <div className="space-y-6">
      <Card className="p-6">
        <h3 className="text-lg font-semibold text-white mb-4">Emergency Actions</h3>
        <div className="grid grid-cols-1 md:grid-cols-2 gap-4">
          {emergencyActions.map(action => (
            <Card key={action.id} className="p-4 border border-red-500">
              <div className="space-y-4">
                <div>
                  <h4 className="font-semibold text-white">{action.name}</h4>
                  <p className="text-sm text-gray-400 mt-1">{action.description}</p>
                </div>
                
                <div className="flex items-center justify-between">
                  <div className="flex items-center space-x-4">
                    <label className="flex items-center space-x-2">
                      <Switch checked={action.enabled} onCheckedChange={() => {}} />
                      <span className="text-sm text-gray-400">Enabled</span>
                    </label>
                    
                    <label className="flex items-center space-x-2">
                      <Switch checked={action.automated} onCheckedChange={() => {}} />
                      <span className="text-sm text-gray-400">Auto</span>
                    </label>
                  </div>
                  
                  <Button
                    variant="destructive"
                    size="sm"
                    onClick={() => executeEmergencyAction(action.id)}
                    disabled={!action.enabled}
                  >
                    Execute
                  </Button>
                </div>
              </div>
            </Card>
          ))}
        </div>
      </Card>

      {/* Risk Scenarios */}
      <Card className="p-6">
        <h3 className="text-lg font-semibold text-white mb-4">Risk Scenarios</h3>
        <div className="space-y-4">
          {riskScenarios.map(scenario => (
            <div key={scenario.id} className="p-4 bg-gray-800 rounded-lg">
              <div className="flex items-start justify-between mb-2">
                <div>
                  <h4 className="font-semibold text-white">{scenario.name}</h4>
                  <p className="text-sm text-gray-400">{scenario.description}</p>
                </div>
                <Badge variant={scenario.status === 'hedged' ? 'success' : 'secondary'}>
                  {scenario.status}
                </Badge>
              </div>
              
              <div className="grid grid-cols-2 md:grid-cols-4 gap-4 mt-3">
                <div>
                  <div className="text-xs text-gray-400">Probability</div>
                  <div className="text-white">{scenario.probability}%</div>
                </div>
                <div>
                  <div className="text-xs text-gray-400">Impact</div>
                  <div className="text-red-400">{formatCurrency(scenario.impactAmount)}</div>
                </div>
                <div>
                  <div className="text-xs text-gray-400">Timeframe</div>
                  <div className="text-white">{scenario.timeframe}</div>
                </div>
                <div>
                  <div className="text-xs text-gray-400">Mitigation</div>
                  <div className="text-white">{scenario.mitigation}</div>
                </div>
              </div>
            </div>
          ))}
        </div>
      </Card>
    </div>
  );

  const renderPositionLimits = () => (
    <div className="space-y-6">
      <Card className="p-6">
        <h3 className="text-lg font-semibold text-white mb-4">Position Limits</h3>
        <div className="overflow-x-auto">
          <table className="w-full text-sm">
            <thead>
              <tr className="border-b border-gray-700">
                <th className="text-left py-2 text-gray-400">Symbol</th>
                <th className="text-right py-2 text-gray-400">Current</th>
                <th className="text-right py-2 text-gray-400">Max Size</th>
                <th className="text-right py-2 text-gray-400">Utilization</th>
                <th className="text-right py-2 text-gray-400">Stop Loss</th>
                <th className="text-right py-2 text-gray-400">Take Profit</th>
                <th className="text-center py-2 text-gray-400">Status</th>
              </tr>
            </thead>
            <tbody>
              {positionLimits.map(position => (
                <tr key={position.symbol} className="border-b border-gray-800">
                  <td className="py-2 text-white font-semibold">{position.symbol}</td>
                  <td className="py-2 text-right text-white">
                    {formatCurrency(position.currentPosition)}
                  </td>
                  <td className="py-2 text-right text-white">
                    {formatCurrency(position.maxPositionSize)}
                  </td>
                  <td className="py-2 text-right">
                    <div className={`${position.utilizationPercent > 80 ? 'text-red-400' : 
                                    position.utilizationPercent > 60 ? 'text-yellow-400' : 'text-green-400'}`}>
                      {position.utilizationPercent}%
                    </div>
                  </td>
                  <td className="py-2 text-right text-red-400">
                    {formatPercent(position.stopLoss)}
                  </td>
                  <td className="py-2 text-right text-green-400">
                    {formatPercent(position.takeProfit)}
                  </td>
                  <td className="py-2 text-center">
                    <Switch checked={position.enabled} onCheckedChange={() => {}} />
                  </td>
                </tr>
              ))}
            </tbody>
          </table>
        </div>
      </Card>

      {/* Limit Configuration */}
      <Card className="p-6">
        <h3 className="text-lg font-semibold text-white mb-4">Configure New Limit</h3>
        <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-4">
          <div>
            <label className="block text-sm text-gray-400 mb-2">Symbol</label>
            <Select
              value=""
              onValueChange={() => {}}
              options={[
                { value: 'BTC', label: 'Bitcoin' },
                { value: 'ETH', label: 'Ethereum' },
                { value: 'SOL', label: 'Solana' },
              ]}
            />
          </div>
          
          <div>
            <label className="block text-sm text-gray-400 mb-2">Max Position Size ($)</label>
            <Input type="number" placeholder="50000" />
          </div>
          
          <div>
            <label className="block text-sm text-gray-400 mb-2">Stop Loss (%)</label>
            <Input type="number" placeholder="-5" />
          </div>
          
          <div>
            <label className="block text-sm text-gray-400 mb-2">Take Profit (%)</label>
            <Input type="number" placeholder="15" />
          </div>
          
          <div className="flex items-end">
            <Button>Add Limit</Button>
          </div>
        </div>
      </Card>
    </div>
  );

  const tabs = [
    { id: 'overview', label: 'Overview' },
    { id: 'emergency', label: 'Emergency Actions' },
    { id: 'positions', label: 'Position Limits' },
  ];

  return (
    <div className="space-y-6">
      {/* Header */}
      <div className="flex justify-between items-center">
        <h2 className="text-2xl font-bold text-white">Risk Management</h2>
        <div className="flex items-center space-x-4">
          <Select
            value={timeframe}
            onValueChange={setTimeframe}
            options={RISK_TIMEFRAMES}
          />
          <Button variant="outline">Export Risk Report</Button>
          <Button variant="destructive" onClick={() => setEmergencyMode(!emergencyMode)}>
            🚨 Emergency Stop
          </Button>
        </div>
      </div>

      {/* Tabs */}
      <Tabs value={activeTab} onValueChange={setActiveTab} tabs={tabs}>
        <div className="mt-6">
          {activeTab === 'overview' && renderOverview()}
          {activeTab === 'emergency' && renderEmergencyActions()}
          {activeTab === 'positions' && renderPositionLimits()}
        </div>
      </Tabs>
    </div>
  );
};
