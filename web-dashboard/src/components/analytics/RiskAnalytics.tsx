'use client';

import React, { useState, useEffect } from 'react';
import { Card } from '../ui/Card';
import { Badge } from '../ui/Badge';
import { Button } from '../ui/Button';
import { Select } from '../ui/Select';

interface RiskMetrics {
  portfolioVar: number;
  portfolioVarPercent: number;
  expectedShortfall: number;
  maxDrawdown: number;
  sharpeRatio: number;
  sortinoRatio: number;
  calmarRatio: number;
  beta: number;
  alpha: number;
  trackingError: number;
  informationRatio: number;
  treynorRatio: number;
}

interface PositionRisk {
  symbol: string;
  exposure: number;
  exposurePercent: number;
  var95: number;
  beta: number;
  correlation: number;
  riskContribution: number;
  concentration: number;
  liquidityRisk: 'Low' | 'Medium' | 'High';
  modelConfidence: number;
}

interface RiskScenario {
  name: string;
  description: string;
  probability: number;
  portfolioImpact: number;
  duration: string;
  hedgingSuggestion: string;
  riskLevel: 'Low' | 'Medium' | 'High' | 'Critical';
}

interface RiskAlert {
  id: string;
  type: 'concentration' | 'correlation' | 'volatility' | 'liquidity' | 'drawdown';
  severity: 'Low' | 'Medium' | 'High' | 'Critical';
  message: string;
  affectedPositions: string[];
  recommendation: string;
  timestamp: number;
}

interface StressTestResult {
  scenario: string;
  portfolioReturn: number;
  maxLoss: number;
  recoveryTime: number;
  survivability: number;
  recommendations: string[];
}

const RISK_TIMEFRAMES = [
  { value: '1d', label: '1 Day' },
  { value: '1w', label: '1 Week' },
  { value: '1m', label: '1 Month' },
  { value: '3m', label: '3 Months' },
  { value: '1y', label: '1 Year' },
];

const CONFIDENCE_LEVELS = [
  { value: '95', label: '95%' },
  { value: '99', label: '99%' },
  { value: '99.9', label: '99.9%' },
];

export const RiskAnalytics: React.FC = () => {
  const [timeframe, setTimeframe] = useState('1m');
  const [confidenceLevel, setConfidenceLevel] = useState('95');
  const [riskMetrics, setRiskMetrics] = useState<RiskMetrics | null>(null);
  const [positionRisks, setPositionRisks] = useState<PositionRisk[]>([]);
  const [riskScenarios, setRiskScenarios] = useState<RiskScenario[]>([]);
  const [riskAlerts, setRiskAlerts] = useState<RiskAlert[]>([]);
  const [stressTests, setStressTests] = useState<StressTestResult[]>([]);

  // Generate mock risk metrics
  const generateRiskMetrics = (): RiskMetrics => ({
    portfolioVar: -(5000 + Math.random() * 15000), // -$5k to -$20k
    portfolioVarPercent: -(2 + Math.random() * 8), // -2% to -10%
    expectedShortfall: -(8000 + Math.random() * 12000), // -$8k to -$20k
    maxDrawdown: -(5 + Math.random() * 15), // -5% to -20%
    sharpeRatio: 0.8 + Math.random() * 2.2, // 0.8 to 3.0
    sortinoRatio: 1.2 + Math.random() * 2.8, // 1.2 to 4.0
    calmarRatio: 0.5 + Math.random() * 2.5, // 0.5 to 3.0
    beta: 0.7 + Math.random() * 0.8, // 0.7 to 1.5
    alpha: (Math.random() - 0.3) * 10, // -3% to 7%
    trackingError: 2 + Math.random() * 8, // 2% to 10%
    informationRatio: (Math.random() - 0.3) * 2, // -0.6 to 1.4
    treynorRatio: 5 + Math.random() * 15, // 5 to 20
  });

  // Generate position risk data
  const generatePositionRisks = (): PositionRisk[] => {
    const symbols = ['BTC', 'ETH', 'SOL', 'MATIC', 'AVAX', 'ADA', 'DOT'];
    const liquidityLevels: ('Low' | 'Medium' | 'High')[] = ['Low', 'Medium', 'High'];
    
    return symbols.map(symbol => {
      const exposure = 10000 + Math.random() * 40000; // $10k-$50k
      const portfolioSize = 200000; // $200k portfolio
      
      return {
        symbol,
        exposure,
        exposurePercent: (exposure / portfolioSize) * 100,
        var95: -(500 + Math.random() * 2500), // -$500 to -$3k
        beta: 0.5 + Math.random() * 1.5, // 0.5 to 2.0
        correlation: 0.3 + Math.random() * 0.6, // 0.3 to 0.9 with market
        riskContribution: 5 + Math.random() * 25, // 5% to 30%
        concentration: (exposure / portfolioSize) * 100,
        liquidityRisk: liquidityLevels[Math.floor(Math.random() * liquidityLevels.length)],
        modelConfidence: 0.7 + Math.random() * 0.3, // 70% to 100%
      };
    });
  };

  // Generate risk scenarios
  const generateRiskScenarios = (): RiskScenario[] => {
    const scenarios = [
      {
        name: 'Market Crash',
        description: 'Broad crypto market decline of 30-50%',
        probability: 15,
        portfolioImpact: -35,
        duration: '2-6 months',
        hedgingSuggestion: 'Increase cash reserves, buy put options',
        riskLevel: 'High' as const,
      },
      {
        name: 'Regulatory Crackdown',
        description: 'Major regulatory restrictions in key markets',
        probability: 25,
        portfolioImpact: -20,
        duration: '6-18 months',
        hedgingSuggestion: 'Diversify into compliant assets',
        riskLevel: 'Medium' as const,
      },
      {
        name: 'DeFi Protocol Exploit',
        description: 'Major smart contract vulnerability discovered',
        probability: 30,
        portfolioImpact: -15,
        duration: '1-3 months',
        hedgingSuggestion: 'Reduce DeFi exposure, increase audited protocols',
        riskLevel: 'Medium' as const,
      },
      {
        name: 'Exchange Hack',
        description: 'Major centralized exchange security breach',
        probability: 20,
        portfolioImpact: -10,
        duration: '2-8 weeks',
        hedgingSuggestion: 'Use hardware wallets, avoid centralized storage',
        riskLevel: 'Medium' as const,
      },
      {
        name: 'Macroeconomic Shock',
        description: 'Global financial crisis affecting risk assets',
        probability: 10,
        portfolioImpact: -45,
        duration: '12-24 months',
        hedgingSuggestion: 'Hedge with traditional safe havens',
        riskLevel: 'Critical' as const,
      },
    ];

    return scenarios.map(scenario => ({
      ...scenario,
      probability: scenario.probability + (Math.random() - 0.5) * 10,
      portfolioImpact: scenario.portfolioImpact + (Math.random() - 0.5) * 10,
    }));
  };

  // Generate risk alerts
  const generateRiskAlerts = (): RiskAlert[] => {
    const alertTypes: RiskAlert['type'][] = ['concentration', 'correlation', 'volatility', 'liquidity', 'drawdown'];
    const severities: RiskAlert['severity'][] = ['Low', 'Medium', 'High', 'Critical'];
    
    return Array.from({ length: 4 }, (_, index) => {
      const type = alertTypes[Math.floor(Math.random() * alertTypes.length)];
      const severity = severities[Math.floor(Math.random() * severities.length)];
      
      return {
        id: `alert_${index}`,
        type,
        severity,
        message: generateAlertMessage(type, severity),
        affectedPositions: ['BTC', 'ETH'].slice(0, Math.floor(1 + Math.random() * 2)),
        recommendation: generateRecommendation(type),
        timestamp: Date.now() - Math.random() * 3600000, // Last hour
      };
    });
  };

  const generateAlertMessage = (type: RiskAlert['type'], severity: RiskAlert['severity']): string => {
    switch (type) {
      case 'concentration':
        return `${severity} concentration risk: Single position exceeds risk limits`;
      case 'correlation':
        return `${severity} correlation spike: Portfolio positions moving in lockstep`;
      case 'volatility':
        return `${severity} volatility surge: Market volatility above normal levels`;
      case 'liquidity':
        return `${severity} liquidity warning: Reduced market depth detected`;
      case 'drawdown':
        return `${severity} drawdown alert: Portfolio down from recent peak`;
      default:
        return `${severity} risk alert detected`;
    }
  };

  const generateRecommendation = (type: RiskAlert['type']): string => {
    switch (type) {
      case 'concentration':
        return 'Consider reducing position size or hedging exposure';
      case 'correlation':
        return 'Diversify into uncorrelated assets or reduce overall exposure';
      case 'volatility':
        return 'Reduce leverage and increase cash reserves';
      case 'liquidity':
        return 'Avoid large trades, consider alternative execution strategies';
      case 'drawdown':
        return 'Review stop-loss levels and consider defensive positioning';
      default:
        return 'Monitor closely and consider risk reduction measures';
    }
  };

  // Generate stress test results
  const generateStressTests = (): StressTestResult[] => {
    const scenarios = [
      'Black Swan Event',
      '2008 Financial Crisis',
      'COVID-19 Pandemic',
      'Regulatory Ban',
      'Technology Failure',
    ];

    return scenarios.map(scenario => ({
      scenario,
      portfolioReturn: -(20 + Math.random() * 40), // -20% to -60%
      maxLoss: -(15000 + Math.random() * 35000), // -$15k to -$50k
      recoveryTime: 3 + Math.random() * 18, // 3-21 months
      survivability: 60 + Math.random() * 35, // 60-95%
      recommendations: [
        'Increase hedging positions',
        'Reduce leverage',
        'Diversify across uncorrelated assets',
        'Maintain larger cash buffer',
      ].slice(0, Math.floor(2 + Math.random() * 3)),
    }));
  };

  // Initialize data
  useEffect(() => {
    setRiskMetrics(generateRiskMetrics());
    setPositionRisks(generatePositionRisks());
    setRiskScenarios(generateRiskScenarios());
    setRiskAlerts(generateRiskAlerts());
    setStressTests(generateStressTests());
  }, [timeframe, confidenceLevel]);

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

  const getRiskColor = (value: number): string => {
    return value >= 0 ? 'text-green-400' : 'text-red-400';
  };

  const getSeverityColor = (severity: string): string => {
    switch (severity) {
      case 'Low': return 'text-green-400';
      case 'Medium': return 'text-yellow-400';
      case 'High': return 'text-orange-400';
      case 'Critical': return 'text-red-400';
      default: return 'text-gray-400';
    }
  };

  const getSeverityBadge = (severity: string) => {
    switch (severity) {
      case 'Low':
        return <Badge variant="success">Low</Badge>;
      case 'Medium':
        return <Badge variant="secondary">Medium</Badge>;
      case 'High':
        return <Badge variant="destructive">High</Badge>;
      case 'Critical':
        return <Badge variant="destructive">Critical</Badge>;
      default:
        return <Badge variant="secondary">{severity}</Badge>;
    }
  };

  const getLiquidityBadge = (risk: string) => {
    switch (risk) {
      case 'Low':
        return <Badge variant="success">Low Risk</Badge>;
      case 'Medium':
        return <Badge variant="secondary">Medium Risk</Badge>;
      case 'High':
        return <Badge variant="destructive">High Risk</Badge>;
      default:
        return <Badge variant="secondary">{risk}</Badge>;
    }
  };

  return (
    <div className="space-y-6">
      {/* Header */}
      <div className="flex justify-between items-center">
        <h2 className="text-2xl font-bold text-white">Risk Analytics</h2>
        <div className="flex items-center space-x-4">
          <Select
            value={timeframe}
            onValueChange={setTimeframe}
            options={RISK_TIMEFRAMES}
          />
          <Select
            value={confidenceLevel}
            onValueChange={setConfidenceLevel}
            options={CONFIDENCE_LEVELS}
          />
          <Button variant="outline">Export Risk Report</Button>
        </div>
      </div>

      {/* Risk Alerts */}
      {riskAlerts.length > 0 && (
        <Card className="p-4 border-l-4 border-red-500">
          <h3 className="text-lg font-semibold text-white mb-3">🚨 Active Risk Alerts</h3>
          <div className="space-y-3">
            {riskAlerts.map(alert => (
              <div key={alert.id} className="flex items-start justify-between p-3 bg-gray-800 rounded-lg">
                <div className="flex-1">
                  <div className="flex items-center space-x-2 mb-1">
                    {getSeverityBadge(alert.severity)}
                    <span className="text-sm text-gray-400 uppercase">{alert.type}</span>
                  </div>
                  <div className="text-white mb-1">{alert.message}</div>
                  <div className="text-sm text-gray-400">{alert.recommendation}</div>
                </div>
                <div className="text-xs text-gray-500">
                  {new Date(alert.timestamp).toLocaleTimeString()}
                </div>
              </div>
            ))}
          </div>
        </Card>
      )}

      {/* Key Risk Metrics */}
      <div className="grid grid-cols-2 md:grid-cols-4 gap-4">
        <Card className="p-4">
          <div className="text-sm text-gray-400">Portfolio VaR ({confidenceLevel}%)</div>
          <div className="text-2xl font-bold text-red-400">
            {formatCurrency(riskMetrics?.portfolioVar || 0)}
          </div>
          <div className="text-sm text-red-400">
            {formatPercent(riskMetrics?.portfolioVarPercent || 0)}
          </div>
        </Card>

        <Card className="p-4">
          <div className="text-sm text-gray-400">Expected Shortfall</div>
          <div className="text-2xl font-bold text-red-400">
            {formatCurrency(riskMetrics?.expectedShortfall || 0)}
          </div>
          <div className="text-sm text-gray-400">Tail risk</div>
        </Card>

        <Card className="p-4">
          <div className="text-sm text-gray-400">Max Drawdown</div>
          <div className="text-2xl font-bold text-red-400">
            {formatPercent(riskMetrics?.maxDrawdown || 0)}
          </div>
          <div className="text-sm text-gray-400">Peak to trough</div>
        </Card>

        <Card className="p-4">
          <div className="text-sm text-gray-400">Sharpe Ratio</div>
          <div className="text-2xl font-bold text-white">
            {riskMetrics?.sharpeRatio.toFixed(2)}
          </div>
          <div className="text-sm text-gray-400">Risk-adjusted return</div>
        </Card>
      </div>

      <div className="grid grid-cols-1 lg:grid-cols-2 gap-6">
        {/* Position Risk Breakdown */}
        <Card className="p-6">
          <h3 className="text-lg font-semibold text-white mb-4">Position Risk Analysis</h3>
          <div className="overflow-x-auto">
            <table className="w-full text-sm">
              <thead>
                <tr className="border-b border-gray-700">
                  <th className="text-left py-2 text-gray-400">Asset</th>
                  <th className="text-right py-2 text-gray-400">Exposure</th>
                  <th className="text-right py-2 text-gray-400">VaR</th>
                  <th className="text-right py-2 text-gray-400">Beta</th>
                  <th className="text-center py-2 text-gray-400">Liquidity</th>
                </tr>
              </thead>
              <tbody>
                {positionRisks.map(position => (
                  <tr key={position.symbol} className="border-b border-gray-800">
                    <td className="py-2 text-white font-semibold">{position.symbol}</td>
                    <td className="py-2 text-right">
                      <div className="text-white">{formatCurrency(position.exposure)}</div>
                      <div className="text-xs text-gray-400">{position.exposurePercent.toFixed(1)}%</div>
                    </td>
                    <td className="py-2 text-right text-red-400">
                      {formatCurrency(position.var95)}
                    </td>
                    <td className="py-2 text-right text-white">
                      {position.beta.toFixed(2)}
                    </td>
                    <td className="py-2 text-center">
                      {getLiquidityBadge(position.liquidityRisk)}
                    </td>
                  </tr>
                ))}
              </tbody>
            </table>
          </div>
        </Card>

        {/* Advanced Risk Metrics */}
        <Card className="p-6">
          <h3 className="text-lg font-semibold text-white mb-4">Advanced Risk Metrics</h3>
          <div className="space-y-4">
            <div className="grid grid-cols-2 gap-4">
              <div className="flex justify-between">
                <span className="text-gray-400">Portfolio Beta:</span>
                <span className="text-white">{riskMetrics?.beta.toFixed(3)}</span>
              </div>
              <div className="flex justify-between">
                <span className="text-gray-400">Portfolio Alpha:</span>
                <span className={getRiskColor(riskMetrics?.alpha || 0)}>
                  {formatPercent(riskMetrics?.alpha || 0)}
                </span>
              </div>
            </div>
            
            <div className="grid grid-cols-2 gap-4">
              <div className="flex justify-between">
                <span className="text-gray-400">Sortino Ratio:</span>
                <span className="text-white">{riskMetrics?.sortinoRatio.toFixed(2)}</span>
              </div>
              <div className="flex justify-between">
                <span className="text-gray-400">Calmar Ratio:</span>
                <span className="text-white">{riskMetrics?.calmarRatio.toFixed(2)}</span>
              </div>
            </div>
            
            <div className="grid grid-cols-2 gap-4">
              <div className="flex justify-between">
                <span className="text-gray-400">Tracking Error:</span>
                <span className="text-white">{formatPercent(riskMetrics?.trackingError || 0)}</span>
              </div>
              <div className="flex justify-between">
                <span className="text-gray-400">Information Ratio:</span>
                <span className={getRiskColor(riskMetrics?.informationRatio || 0)}>
                  {riskMetrics?.informationRatio.toFixed(3)}
                </span>
              </div>
            </div>
            
            <div className="pt-4 border-t border-gray-700">
              <div className="flex justify-between">
                <span className="text-gray-400">Treynor Ratio:</span>
                <span className="text-white">{riskMetrics?.treynorRatio.toFixed(2)}</span>
              </div>
            </div>
          </div>
        </Card>
      </div>

      {/* Risk Scenarios */}
      <Card className="p-6">
        <h3 className="text-lg font-semibold text-white mb-4">Risk Scenario Analysis</h3>
        <div className="space-y-4">
          {riskScenarios.map(scenario => (
            <div key={scenario.name} className="p-4 bg-gray-800 rounded-lg">
              <div className="flex items-start justify-between mb-2">
                <div>
                  <div className="flex items-center space-x-2">
                    <h4 className="font-semibold text-white">{scenario.name}</h4>
                    {getSeverityBadge(scenario.riskLevel)}
                  </div>
                  <p className="text-sm text-gray-400 mt-1">{scenario.description}</p>
                </div>
                <div className="text-right">
                  <div className="text-lg font-bold text-red-400">
                    {formatPercent(scenario.portfolioImpact)}
                  </div>
                  <div className="text-sm text-gray-400">{scenario.probability.toFixed(1)}% probability</div>
                </div>
              </div>
              
              <div className="grid grid-cols-1 md:grid-cols-2 gap-4 mt-3 pt-3 border-t border-gray-700">
                <div>
                  <div className="text-sm text-gray-400">Duration:</div>
                  <div className="text-white">{scenario.duration}</div>
                </div>
                <div>
                  <div className="text-sm text-gray-400">Hedging Suggestion:</div>
                  <div className="text-white">{scenario.hedgingSuggestion}</div>
                </div>
              </div>
            </div>
          ))}
        </div>
      </Card>

      {/* Stress Test Results */}
      <Card className="p-6">
        <h3 className="text-lg font-semibold text-white mb-4">Stress Test Results</h3>
        <div className="overflow-x-auto">
          <table className="w-full text-sm">
            <thead>
              <tr className="border-b border-gray-700">
                <th className="text-left py-2 text-gray-400">Scenario</th>
                <th className="text-right py-2 text-gray-400">Portfolio Return</th>
                <th className="text-right py-2 text-gray-400">Max Loss</th>
                <th className="text-right py-2 text-gray-400">Recovery Time</th>
                <th className="text-right py-2 text-gray-400">Survivability</th>
              </tr>
            </thead>
            <tbody>
              {stressTests.map(test => (
                <tr key={test.scenario} className="border-b border-gray-800">
                  <td className="py-2 text-white">{test.scenario}</td>
                  <td className="py-2 text-right text-red-400">
                    {formatPercent(test.portfolioReturn)}
                  </td>
                  <td className="py-2 text-right text-red-400">
                    {formatCurrency(test.maxLoss)}
                  </td>
                  <td className="py-2 text-right text-white">
                    {test.recoveryTime.toFixed(1)} months
                  </td>
                  <td className="py-2 text-right">
                    <span className={test.survivability > 80 ? 'text-green-400' : 
                                   test.survivability > 60 ? 'text-yellow-400' : 'text-red-400'}>
                      {test.survivability.toFixed(1)}%
                    </span>
                  </td>
                </tr>
              ))}
            </tbody>
          </table>
        </div>
      </Card>

      {/* Action Panel */}
      <div className="flex justify-center space-x-4">
        <Button variant="outline">Schedule Risk Review</Button>
        <Button variant="outline">Update Risk Limits</Button>
        <Button className="bg-red-600 hover:bg-red-700">Emergency Risk Off</Button>
      </div>
    </div>
  );
};
