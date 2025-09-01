// MEV protection and blockchain integration types

export interface MEVProtectionStatus {
  enabled: boolean;
  mode: 'aggressive' | 'balanced' | 'conservative';
  successRate: number; // percentage 0-100
  bundlesSubmitted: number;
  bundlesIncluded: number;
  protectedValue: number; // USD value protected
  savedFromMEV: number; // USD value saved
  relaysConnected: number;
  priorityFee: number; // gwei
}

export interface MEVBundle {
  id: string;
  transactions: string[]; // transaction hashes
  blockNumber: number;
  gasPrice: number;
  priorityFee: number;
  status: 'pending' | 'included' | 'failed' | 'cancelled';
  submittedAt: number;
  includedAt?: number;
  relay: string;
  profit?: number; // USD
}

export interface BlockchainNetwork {
  name: string;
  chainId: number;
  status: 'connected' | 'disconnected' | 'syncing';
  blockHeight: number;
  gasPrice: number;
  baseFee?: number; // EIP-1559 networks
  priorityFee?: number; // EIP-1559 networks
  tps: number; // transactions per second
  latency: number; // milliseconds to RPC
  peers: number;
}

export interface Wallet {
  id: string;
  address: string;
  name: string;
  network: string;
  balance: number; // native token balance
  tokens: TokenBalance[];
  nonce: number;
  gasUsed: number; // total gas used
  totalTrades: number;
  profitLoss: number; // USD
  isActive: boolean;
  riskLevel: 'low' | 'medium' | 'high';
}

export interface TokenBalance {
  symbol: string;
  contractAddress: string;
  balance: number;
  valueUSD: number;
  decimals: number;
}

export interface ArbitrageOpportunity {
  id: string;
  tokenPair: string;
  exchange1: string;
  exchange2: string;
  price1: number;
  price2: number;
  spread: number; // percentage
  profitUSD: number;
  gasEstimate: number;
  netProfit: number; // after gas
  confidence: number; // percentage 0-100
  discoveredAt: number;
  expiresAt: number;
}
