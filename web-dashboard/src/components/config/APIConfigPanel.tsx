'use client';

import { useState } from 'react';
import { EyeIcon, EyeSlashIcon, CheckCircleIcon, XCircleIcon } from '@heroicons/react/24/outline';
import { Card } from '@/components/ui/Card';
import { Button } from '@/components/ui/Button';
import { Input } from '@/components/ui/Input';
import { Switch } from '@/components/ui/Switch';
import { Badge } from '@/components/ui/Badge';
import { Tabs, TabsContent, TabsList, TabsTrigger } from '@/components/ui/Tabs';
import toast from 'react-hot-toast';

interface APIConfig {
  apis: Record<string, {
    provider: string;
    api_key: string;
    secret_key?: string;
    enabled: boolean;
    status: 'connected' | 'disconnected' | 'testing';
  }>;
  rpcs: Record<string, {
    chain: string;
    endpoint: string;
    api_key: string;
    enabled: boolean;
    status: 'connected' | 'disconnected' | 'testing';
  }>;
}

interface APIConfigPanelProps {
  config: APIConfig;
  onConfigUpdate: (config: Partial<APIConfig>) => void;
}

export function APIConfigPanel({ config, onConfigUpdate }: APIConfigPanelProps) {
  const [showKeys, setShowKeys] = useState<Record<string, boolean>>({});
  const [testingConnections, setTestingConnections] = useState<Set<string>>(new Set());

  const toggleKeyVisibility = (key: string) => {
    setShowKeys(prev => ({ ...prev, [key]: !prev[key] }));
  };

  const testConnection = async (type: 'api' | 'rpc', name: string) => {
    setTestingConnections(prev => new Set([...prev, `${type}-${name}`]));
    
    try {
      const response = await fetch(`/api/test-connection`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ type, name, config: config[type === 'api' ? 'apis' : 'rpcs'][name] }),
      });

      if (response.ok) {
        toast.success(`${name} connection successful`);
        // Update status to connected
        const updatedConfig = {
          [type === 'api' ? 'apis' : 'rpcs']: {
            ...config[type === 'api' ? 'apis' : 'rpcs'],
            [name]: {
              ...config[type === 'api' ? 'apis' : 'rpcs'][name],
              status: 'connected' as const,
            },
          },
        };
        onConfigUpdate(updatedConfig);
      } else {
        throw new Error('Connection failed');
      }
    } catch (error) {
      toast.error(`${name} connection failed`);
      // Update status to disconnected
      const updatedConfig = {
        [type === 'api' ? 'apis' : 'rpcs']: {
          ...config[type === 'api' ? 'apis' : 'rpcs'],
          [name]: {
            ...config[type === 'api' ? 'apis' : 'rpcs'][name],
            status: 'disconnected' as const,
          },
        },
      };
      onConfigUpdate(updatedConfig);
    } finally {
      setTestingConnections(prev => {
        const newSet = new Set(prev);
        newSet.delete(`${type}-${name}`);
        return newSet;
      });
    }
  };

  const updateAPIConfig = (name: string, field: string, value: any) => {
    const updatedConfig = {
      apis: {
        ...config.apis,
        [name]: {
          ...config.apis[name],
          [field]: value,
        },
      },
    };
    onConfigUpdate(updatedConfig);
  };

  const updateRPCConfig = (name: string, field: string, value: any) => {
    const updatedConfig = {
      rpcs: {
        ...config.rpcs,
        [name]: {
          ...config.rpcs[name],
          [field]: value,
        },
      },
    };
    onConfigUpdate(updatedConfig);
  };

  const renderAPICard = (name: string, apiConfig: any) => (
    <Card key={name} className="p-6">
      <div className="flex items-center justify-between mb-4">
        <div className="flex items-center gap-3">
          <h3 className="text-lg font-semibold capitalize text-gray-100">{name}</h3>
          <Badge variant={apiConfig.status === 'connected' ? 'success' : 'destructive'}>
            {apiConfig.status === 'connected' ? (
              <CheckCircleIcon className="w-3 h-3 mr-1" />
            ) : (
              <XCircleIcon className="w-3 h-3 mr-1" />
            )}
            {apiConfig.status}
          </Badge>
        </div>
        <Switch
          checked={apiConfig.enabled}
          onCheckedChange={(checked) => updateAPIConfig(name, 'enabled', checked)}
        />
      </div>

      <div className="space-y-4">
        <div>
          <label className="block text-sm font-medium text-gray-300 mb-2">
            API Key
          </label>
          <div className="relative">
            <Input
              type={showKeys[`api-${name}`] ? 'text' : 'password'}
              value={apiConfig.api_key || ''}
              onChange={(e) => updateAPIConfig(name, 'api_key', e.target.value)}
              placeholder="Enter your API key"
              className="pr-10"
            />
            <button
              type="button"
              onClick={() => toggleKeyVisibility(`api-${name}`)}
              className="absolute inset-y-0 right-0 pr-3 flex items-center"
            >
              {showKeys[`api-${name}`] ? (
                <EyeSlashIcon className="h-4 w-4 text-gray-400" />
              ) : (
                <EyeIcon className="h-4 w-4 text-gray-400" />
              )}
            </button>
          </div>
        </div>

        {apiConfig.secret_key !== undefined && (
          <div>
            <label className="block text-sm font-medium text-gray-300 mb-2">
              Secret Key
            </label>
            <div className="relative">
              <Input
                type={showKeys[`secret-${name}`] ? 'text' : 'password'}
                value={apiConfig.secret_key || ''}
                onChange={(e) => updateAPIConfig(name, 'secret_key', e.target.value)}
                placeholder="Enter your secret key"
                className="pr-10"
              />
              <button
                type="button"
                onClick={() => toggleKeyVisibility(`secret-${name}`)}
                className="absolute inset-y-0 right-0 pr-3 flex items-center"
              >
                {showKeys[`secret-${name}`] ? (
                  <EyeSlashIcon className="h-4 w-4 text-gray-400" />
                ) : (
                  <EyeIcon className="h-4 w-4 text-gray-400" />
                )}
              </button>
            </div>
          </div>
        )}

        <Button
          onClick={() => testConnection('api', name)}
          disabled={!apiConfig.api_key || testingConnections.has(`api-${name}`)}
          className="w-full"
          variant="outline"
        >
          {testingConnections.has(`api-${name}`) ? 'Testing...' : 'Test Connection'}
        </Button>
      </div>
    </Card>
  );

  const renderRPCCard = (name: string, rpcConfig: any) => (
    <Card key={name} className="p-6">
      <div className="flex items-center justify-between mb-4">
        <div className="flex items-center gap-3">
          <h3 className="text-lg font-semibold capitalize text-gray-100">{name}</h3>
          <Badge variant={rpcConfig.status === 'connected' ? 'success' : 'destructive'}>
            {rpcConfig.status === 'connected' ? (
              <CheckCircleIcon className="w-3 h-3 mr-1" />
            ) : (
              <XCircleIcon className="w-3 h-3 mr-1" />
            )}
            {rpcConfig.status}
          </Badge>
        </div>
        <Switch
          checked={rpcConfig.enabled}
          onCheckedChange={(checked) => updateRPCConfig(name, 'enabled', checked)}
        />
      </div>

      <div className="space-y-4">
        <div>
          <label className="block text-sm font-medium text-gray-300 mb-2">
            RPC Endpoint
          </label>
          <Input
            value={rpcConfig.endpoint || ''}
            onChange={(e) => updateRPCConfig(name, 'endpoint', e.target.value)}
            placeholder="https://..."
          />
        </div>

        <div>
          <label className="block text-sm font-medium text-gray-300 mb-2">
            API Key
          </label>
          <div className="relative">
            <Input
              type={showKeys[`rpc-${name}`] ? 'text' : 'password'}
              value={rpcConfig.api_key || ''}
              onChange={(e) => updateRPCConfig(name, 'api_key', e.target.value)}
              placeholder="Enter your API key"
              className="pr-10"
            />
            <button
              type="button"
              onClick={() => toggleKeyVisibility(`rpc-${name}`)}
              className="absolute inset-y-0 right-0 pr-3 flex items-center"
            >
              {showKeys[`rpc-${name}`] ? (
                <EyeSlashIcon className="h-4 w-4 text-gray-400" />
              ) : (
                <EyeIcon className="h-4 w-4 text-gray-400" />
              )}
            </button>
          </div>
        </div>

        <Button
          onClick={() => testConnection('rpc', name)}
          disabled={!rpcConfig.endpoint || testingConnections.has(`rpc-${name}`)}
          className="w-full"
          variant="outline"
        >
          {testingConnections.has(`rpc-${name}`) ? 'Testing...' : 'Test Connection'}
        </Button>
      </div>
    </Card>
  );

  return (
    <div className="space-y-6">
      <Tabs defaultValue="apis" className="w-full">
        <TabsList className="grid w-full grid-cols-2">
          <TabsTrigger value="apis">Data APIs</TabsTrigger>
          <TabsTrigger value="rpcs">Blockchain RPCs</TabsTrigger>
        </TabsList>

        <TabsContent value="apis" className="space-y-6">
          <div className="grid grid-cols-1 md:grid-cols-2 gap-6">
            {Object.entries(config.apis || {}).map(([name, apiConfig]) =>
              renderAPICard(name, apiConfig)
            )}
          </div>
        </TabsContent>

        <TabsContent value="rpcs" className="space-y-6">
          <div className="grid grid-cols-1 md:grid-cols-2 gap-6">
            {Object.entries(config.rpcs || {}).map(([name, rpcConfig]) =>
              renderRPCCard(name, rpcConfig)
            )}
          </div>
        </TabsContent>
      </Tabs>
    </div>
  );
}
