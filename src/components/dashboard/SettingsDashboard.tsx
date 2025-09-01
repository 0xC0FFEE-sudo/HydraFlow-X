'use client';

import { motion } from 'framer-motion';
import { Card, CardHeader, CardTitle, CardContent } from '@/components/ui/Card';
import { Button } from '@/components/ui/Button';
import { Input } from '@/components/ui/Input';
import { FeatureSwitch } from '@/components/ui/Switch';
import { Settings, Save, RefreshCw, Shield, Zap } from 'lucide-react';

export function SettingsDashboard() {
  return (
    <motion.div
      initial={{ opacity: 0, y: 20 }}
      animate={{ opacity: 1, y: 0 }}
      className="space-y-6"
    >
      <div className="flex items-center justify-between">
        <div>
          <h1 className="text-3xl font-bold gradient-text">Settings</h1>
          <p className="text-secondary-400 mt-1">Configure trading parameters and system settings</p>
        </div>
        <div className="flex space-x-3">
          <Button variant="outline" leftIcon={<RefreshCw className="w-4 h-4" />}>
            Reset
          </Button>
          <Button variant="gradient" leftIcon={<Save className="w-4 h-4" />}>
            Save Changes
          </Button>
        </div>
      </div>

      <div className="grid grid-cols-1 lg:grid-cols-2 gap-6">
        <Card variant="glass">
          <CardHeader>
            <CardTitle className="flex items-center space-x-2">
              <Settings className="w-5 h-5 text-primary-400" />
              <span>Trading Configuration</span>
            </CardTitle>
          </CardHeader>
          <CardContent className="space-y-4">
            <Input label="Max Position Size" placeholder="10000" variant="glass" />
            <Input label="Max Slippage %" placeholder="2.0" variant="glass" />
            <Input label="Default Gas Price" placeholder="20" variant="glass" />
            
            <FeatureSwitch
              feature={{
                name: "Auto Trading",
                description: "Enable autonomous trading",
                icon: <Zap className="w-4 h-4" />
              }}
              enabled={true}
              onToggle={() => {}}
            />

            <FeatureSwitch
              feature={{
                name: "Risk Management",
                description: "Enable automatic risk controls",
                icon: <Shield className="w-4 h-4" />
              }}
              enabled={true}
              onToggle={() => {}}
            />
          </CardContent>
        </Card>

        <Card variant="glass">
          <CardHeader>
            <CardTitle className="flex items-center space-x-2">
              <Shield className="w-5 h-5 text-success-400" />
              <span>Security Settings</span>
            </CardTitle>
          </CardHeader>
          <CardContent className="space-y-4">
            <Input label="Session Timeout (minutes)" placeholder="60" variant="glass" />
            <Input label="API Rate Limit" placeholder="1000" variant="glass" />
            
            <FeatureSwitch
              feature={{
                name: "Two-Factor Authentication",
                description: "Require 2FA for sensitive operations",
                icon: <Shield className="w-4 h-4" />
              }}
              enabled={false}
              onToggle={() => {}}
            />

            <FeatureSwitch
              feature={{
                name: "IP Whitelist",
                description: "Restrict access by IP address",
                icon: <Shield className="w-4 h-4" />
              }}
              enabled={true}
              onToggle={() => {}}
            />
          </CardContent>
        </Card>
      </div>
    </motion.div>
  );
}
