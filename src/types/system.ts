// System monitoring and performance types

export interface SystemMetrics {
  timestamp: number;
  cpu: number;
  memory: number;
  network_rx: number;
  network_tx: number;
  latency: number;
  uptime: number;
}

export interface PerformanceMetrics {
  trades_per_second: number;
  avg_latency_ms: number;
  success_rate: number;
  total_trades: number;
  active_positions: number;
  system_load: number;
}

export interface MEVProtectionStatus {
  enabled: boolean;
  active_bundles: number;
  protection_score: number;
  recent_blocks: number;
  mev_extracted: number;
}

export interface SystemHealth {
  status: 'healthy' | 'warning' | 'critical';
  components: {
    trading_engine: boolean;
    mempool_monitor: boolean;
    websocket_server: boolean;
    database: boolean;
    network: boolean;
  };
  alerts: SystemAlert[];
}

export interface SystemAlert {
  id: string;
  type: 'info' | 'warning' | 'error' | 'critical';
  title: string;
  message: string;
  timestamp: number;
  acknowledged: boolean;
}