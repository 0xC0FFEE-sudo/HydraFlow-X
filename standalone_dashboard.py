#!/usr/bin/env python3
"""
HydraFlow-X Standalone Dashboard
Ultra-Fast Crypto Trading Visualization
"""

import asyncio
import websockets
import json
import time
import random
import threading
from datetime import datetime
from http.server import HTTPServer, SimpleHTTPRequestHandler
import os

class HydraFlowDashboard:
    def __init__(self):
        self.clients = set()
        self.running = True
        self.metrics = {
            'orders_executed': 0,
            'total_volume': 0.0,
            'avg_latency_us': 0.0,
            'success_rate': 99.8,
            'active_strategies': 12,
            'ai_confidence': 0.85,
            'risk_score': 0.15,
            'memecoin_scanned': 0,
            'profits_usd': 0.0
        }
        self.live_trades = []
        self.tokens = ["BTC", "ETH", "SOL", "PEPE", "SHIB", "DOGE", "BONK", "WIF", "POPCAT", "MOODENG"]
        
    async def register_client(self, websocket, path):
        self.clients.add(websocket)
        print(f"🔌 Dashboard client connected: {len(self.clients)} total clients")
        try:
            await websocket.wait_closed()
        finally:
            self.clients.remove(websocket)
            print(f"🔌 Dashboard client disconnected: {len(self.clients)} total clients")

    async def broadcast_data(self):
        while self.running:
            if self.clients:
                # Generate live trading data
                self.generate_live_data()
                
                data = {
                    'type': 'update',
                    'timestamp': datetime.now().isoformat(),
                    'metrics': self.metrics,
                    'live_trades': self.live_trades[-10:],  # Last 10 trades
                    'system_status': {
                        'core_engine': 'ONLINE',
                        'ai_system': 'PROCESSING',
                        'execution_engine': 'ULTRA_FAST',
                        'risk_manager': 'PROTECTED',
                        'network': 'OPTIMAL'
                    }
                }
                
                # Broadcast to all connected clients
                if self.clients:
                    await asyncio.gather(
                        *[client.send(json.dumps(data)) for client in self.clients],
                        return_exceptions=True
                    )
            
            await asyncio.sleep(0.1)  # 100ms updates for real-time feel

    def generate_live_data(self):
        # Update metrics
        self.metrics['orders_executed'] += random.randint(1, 5)
        self.metrics['total_volume'] += random.uniform(1000, 50000)
        self.metrics['avg_latency_us'] = random.uniform(50, 150)
        self.metrics['ai_confidence'] = 0.75 + random.uniform(0, 0.25)
        self.metrics['risk_score'] = random.uniform(0.05, 0.25)
        self.metrics['memecoin_scanned'] += random.randint(0, 3)
        self.metrics['profits_usd'] += random.uniform(-100, 500)
        
        # Generate new trade
        if random.random() > 0.7:  # 30% chance of new trade
            token = random.choice(self.tokens)
            side = random.choice(['BUY', 'SELL'])
            amount = random.uniform(1000, 25000)
            price = random.uniform(0.001, 100000)
            latency = random.uniform(45, 200)
            
            trade = {
                'id': f"T{int(time.time() * 1000)}",
                'timestamp': datetime.now().isoformat(),
                'token': token,
                'side': side,
                'amount_usd': round(amount, 2),
                'price': round(price, 6),
                'latency_us': round(latency, 1),
                'status': 'EXECUTED',
                'source': random.choice(['AI_SIGNAL', 'ARBITRAGE', 'MOMENTUM', 'SENTIMENT']),
                'exchange': random.choice(['RAYDIUM', 'ORCA', 'JUPITER', 'UNISWAP'])
            }
            
            self.live_trades.append(trade)
            
            # Keep only last 100 trades
            if len(self.live_trades) > 100:
                self.live_trades = self.live_trades[-100:]

class HTTPHandler(SimpleHTTPRequestHandler):
    def do_GET(self):
        if self.path == '/':
            self.path = '/dashboard.html'
        super().do_GET()

def create_dashboard_html():
    html_content = """
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>HydraFlow-X | Ultra-Fast Crypto Trading Dashboard</title>
    <style>
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }
        
        body {
            font-family: 'Monaco', 'Consolas', monospace;
            background: linear-gradient(135deg, #0a0a0a 0%, #1a1a2e 50%, #16213e 100%);
            color: #00ff88;
            overflow-x: hidden;
            min-height: 100vh;
        }
        
        .header {
            background: rgba(0, 0, 0, 0.9);
            padding: 10px 20px;
            border-bottom: 2px solid #00ff88;
            box-shadow: 0 0 20px rgba(0, 255, 136, 0.3);
        }
        
        .logo {
            font-size: 24px;
            font-weight: bold;
            color: #00ff88;
            text-shadow: 0 0 10px rgba(0, 255, 136, 0.5);
        }
        
        .subtitle {
            font-size: 12px;
            color: #888;
            margin-top: 2px;
        }
        
        .dashboard {
            display: grid;
            grid-template-columns: 1fr 1fr 1fr;
            gap: 20px;
            padding: 20px;
            max-width: 1400px;
            margin: 0 auto;
        }
        
        .panel {
            background: rgba(0, 0, 0, 0.8);
            border: 1px solid #00ff88;
            border-radius: 8px;
            padding: 15px;
            box-shadow: 0 0 15px rgba(0, 255, 136, 0.2);
            animation: pulse 2s infinite alternate;
        }
        
        @keyframes pulse {
            0% { box-shadow: 0 0 15px rgba(0, 255, 136, 0.2); }
            100% { box-shadow: 0 0 25px rgba(0, 255, 136, 0.4); }
        }
        
        .panel-title {
            font-size: 14px;
            font-weight: bold;
            margin-bottom: 10px;
            color: #00ff88;
            border-bottom: 1px solid #333;
            padding-bottom: 5px;
        }
        
        .metric {
            display: flex;
            justify-content: space-between;
            margin: 8px 0;
            font-size: 12px;
        }
        
        .metric-value {
            color: #fff;
            font-weight: bold;
        }
        
        .status-online { color: #00ff88; }
        .status-warning { color: #ffaa00; }
        .status-error { color: #ff4444; }
        
        .trades-panel {
            grid-column: span 3;
            max-height: 400px;
            overflow-y: auto;
        }
        
        .trade-item {
            display: grid;
            grid-template-columns: 80px 60px 60px 100px 80px 80px 100px;
            gap: 10px;
            padding: 8px;
            border-bottom: 1px solid #333;
            font-size: 11px;
            transition: background 0.3s;
        }
        
        .trade-item:hover {
            background: rgba(0, 255, 136, 0.1);
        }
        
        .trade-buy { color: #00ff88; }
        .trade-sell { color: #ff6b6b; }
        
        .connection-status {
            position: fixed;
            top: 20px;
            right: 20px;
            padding: 8px 12px;
            border-radius: 4px;
            font-size: 12px;
            font-weight: bold;
        }
        
        .connected {
            background: rgba(0, 255, 136, 0.2);
            border: 1px solid #00ff88;
            color: #00ff88;
        }
        
        .disconnected {
            background: rgba(255, 68, 68, 0.2);
            border: 1px solid #ff4444;
            color: #ff4444;
        }
        
        .loading {
            text-align: center;
            padding: 40px;
            font-size: 16px;
            color: #888;
        }
        
        .number-glow {
            text-shadow: 0 0 5px currentColor;
        }
        
        .ascii-art {
            font-family: monospace;
            font-size: 8px;
            line-height: 1;
            color: #00ff88;
            opacity: 0.6;
            margin: 10px 0;
        }
    </style>
</head>
<body>
    <div class="header">
        <div class="logo">⚡ HYDRAFLOW-X</div>
        <div class="subtitle">Ultra-Fast AI-Driven Cryptocurrency Trading Engine</div>
    </div>
    
    <div class="connection-status" id="connectionStatus">
        <span id="statusText">CONNECTING...</span>
    </div>
    
    <div class="dashboard" id="dashboard">
        <div class="loading">
            <div class="ascii-art">
    ╔══════════════════════════════════════╗
    ║  🚀 INITIALIZING HYDRAFLOW-X v1.0   ║
    ║  ⚡ Ultra-Fast Trading Engine        ║
    ║  🤖 AI-Powered Decision System       ║
    ║  🛡️ Advanced Risk Management         ║
    ╚══════════════════════════════════════╝
            </div>
            <p>Loading real-time trading dashboard...</p>
        </div>
    </div>

    <script>
        let ws = null;
        let reconnectAttempts = 0;
        const maxReconnectAttempts = 5;
        
        function formatNumber(num, decimals = 2) {
            if (num >= 1000000) {
                return (num / 1000000).toFixed(1) + 'M';
            } else if (num >= 1000) {
                return (num / 1000).toFixed(1) + 'K';
            }
            return num.toFixed(decimals);
        }
        
        function formatCurrency(amount) {
            return new Intl.NumberFormat('en-US', {
                style: 'currency',
                currency: 'USD'
            }).format(amount);
        }
        
        function connectWebSocket() {
            try {
                ws = new WebSocket('ws://localhost:8765');
                
                ws.onopen = function() {
                    console.log('🔌 Connected to HydraFlow-X');
                    document.getElementById('connectionStatus').className = 'connection-status connected';
                    document.getElementById('statusText').textContent = 'LIVE';
                    reconnectAttempts = 0;
                };
                
                ws.onmessage = function(event) {
                    const data = JSON.parse(event.data);
                    updateDashboard(data);
                };
                
                ws.onclose = function() {
                    console.log('🔌 Disconnected from HydraFlow-X');
                    document.getElementById('connectionStatus').className = 'connection-status disconnected';
                    document.getElementById('statusText').textContent = 'OFFLINE';
                    
                    if (reconnectAttempts < maxReconnectAttempts) {
                        setTimeout(() => {
                            reconnectAttempts++;
                            console.log(`🔄 Reconnection attempt ${reconnectAttempts}/${maxReconnectAttempts}`);
                            connectWebSocket();
                        }, 2000);
                    }
                };
                
                ws.onerror = function(error) {
                    console.error('❌ WebSocket error:', error);
                };
                
            } catch (error) {
                console.error('❌ Connection error:', error);
                document.getElementById('connectionStatus').className = 'connection-status disconnected';
                document.getElementById('statusText').textContent = 'ERROR';
            }
        }
        
        function updateDashboard(data) {
            if (data.type === 'update') {
                const dashboard = document.getElementById('dashboard');
                
                dashboard.innerHTML = `
                    <!-- Performance Metrics -->
                    <div class="panel">
                        <div class="panel-title">🚀 EXECUTION METRICS</div>
                        <div class="metric">
                            <span>Orders Executed:</span>
                            <span class="metric-value number-glow">${formatNumber(data.metrics.orders_executed)}</span>
                        </div>
                        <div class="metric">
                            <span>Total Volume:</span>
                            <span class="metric-value number-glow">${formatCurrency(data.metrics.total_volume)}</span>
                        </div>
                        <div class="metric">
                            <span>Avg Latency:</span>
                            <span class="metric-value number-glow">${data.metrics.avg_latency_us.toFixed(1)}μs</span>
                        </div>
                        <div class="metric">
                            <span>Success Rate:</span>
                            <span class="metric-value number-glow">${data.metrics.success_rate.toFixed(1)}%</span>
                        </div>
                    </div>
                    
                    <!-- AI & Strategy Status -->
                    <div class="panel">
                        <div class="panel-title">🤖 AI SYSTEM STATUS</div>
                        <div class="metric">
                            <span>AI Confidence:</span>
                            <span class="metric-value number-glow">${(data.metrics.ai_confidence * 100).toFixed(1)}%</span>
                        </div>
                        <div class="metric">
                            <span>Active Strategies:</span>
                            <span class="metric-value number-glow">${data.metrics.active_strategies}</span>
                        </div>
                        <div class="metric">
                            <span>Risk Score:</span>
                            <span class="metric-value number-glow">${(data.metrics.risk_score * 100).toFixed(1)}%</span>
                        </div>
                        <div class="metric">
                            <span>Memecoins Scanned:</span>
                            <span class="metric-value number-glow">${formatNumber(data.metrics.memecoin_scanned)}</span>
                        </div>
                    </div>
                    
                    <!-- System Health -->
                    <div class="panel">
                        <div class="panel-title">🛡️ SYSTEM HEALTH</div>
                        <div class="metric">
                            <span>Core Engine:</span>
                            <span class="metric-value status-online">${data.system_status.core_engine}</span>
                        </div>
                        <div class="metric">
                            <span>AI System:</span>
                            <span class="metric-value status-online">${data.system_status.ai_system}</span>
                        </div>
                        <div class="metric">
                            <span>Execution:</span>
                            <span class="metric-value status-online">${data.system_status.execution_engine}</span>
                        </div>
                        <div class="metric">
                            <span>P&L Today:</span>
                            <span class="metric-value number-glow ${data.metrics.profits_usd >= 0 ? 'trade-buy' : 'trade-sell'}">${formatCurrency(data.metrics.profits_usd)}</span>
                        </div>
                    </div>
                    
                    <!-- Live Trades -->
                    <div class="panel trades-panel">
                        <div class="panel-title">📊 LIVE TRADES</div>
                        <div class="trade-item" style="font-weight: bold; border-bottom: 2px solid #00ff88;">
                            <span>TOKEN</span>
                            <span>SIDE</span>
                            <span>AMOUNT</span>
                            <span>PRICE</span>
                            <span>LATENCY</span>
                            <span>SOURCE</span>
                            <span>EXCHANGE</span>
                        </div>
                        ${data.live_trades.slice().reverse().map(trade => `
                            <div class="trade-item">
                                <span class="metric-value">${trade.token}</span>
                                <span class="metric-value ${trade.side === 'BUY' ? 'trade-buy' : 'trade-sell'}">${trade.side}</span>
                                <span class="metric-value">$${formatNumber(trade.amount_usd, 0)}</span>
                                <span class="metric-value">$${trade.price}</span>
                                <span class="metric-value">${trade.latency_us}μs</span>
                                <span class="metric-value">${trade.source}</span>
                                <span class="metric-value">${trade.exchange}</span>
                            </div>
                        `).join('')}
                    </div>
                `;
            }
        }
        
        // Start connection when page loads
        document.addEventListener('DOMContentLoaded', function() {
            connectWebSocket();
        });
    </script>
</body>
</html>
    """
    
    with open('/Users/sam/Desktop/HydraFlow/build/dashboard.html', 'w') as f:
        f.write(html_content)

async def main():
    dashboard = HydraFlowDashboard()
    
    # Create the HTML dashboard file
    create_dashboard_html()
    
    print("🚀 Starting HydraFlow-X Dashboard Server...")
    print("📊 Dashboard: http://localhost:8000")
    print("🔌 WebSocket: ws://localhost:8765")
    print("⚡ Ultra-Fast Trading Visualization Loading...")
    
    # Start HTTP server in a separate thread
    def start_http_server():
        os.chdir('/Users/sam/Desktop/HydraFlow/build')
        httpd = HTTPServer(('localhost', 8000), HTTPHandler)
        httpd.serve_forever()
    
    http_thread = threading.Thread(target=start_http_server, daemon=True)
    http_thread.start()
    
    # Start WebSocket server
    start_server = websockets.serve(dashboard.register_client, "localhost", 8765)
    
    # Start data broadcasting
    await asyncio.gather(
        start_server,
        dashboard.broadcast_data()
    )

if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        print("\n🛑 Dashboard stopped")
