# HydraFlow-X REST API Documentation

The HydraFlow-X REST API provides programmatic access to all trading, configuration, and monitoring functionality. This API is designed for ultra-low latency with response times under 50ms.

## 🚀 Quick Start

### Base URL
```
http://localhost:8080/api
```

### Authentication
Currently, the API uses session-based authentication. Future versions will support API key authentication.

### Response Format
All responses are in JSON format with the following structure:

```json
{
  "success": true,
  "data": { ... },
  "timestamp": 1640995200,
  "latency_ms": 15.3
}
```

Error responses:
```json
{
  "success": false,
  "error": "Error description",
  "error_code": "INVALID_PARAMETER",
  "timestamp": 1640995200
}
```

## 📊 Trading API

### Start Trading
Start the trading engine with specified configuration.

**Endpoint:** `POST /api/trading/start`

**Request:**
```json
{
  "mode": "STANDARD_BUY",
  "settings": {
    "maxPositionSize": 1000.0,
    "maxSlippage": 2.0,
    "enableMEVProtection": true,
    "enableSniperMode": false
  }
}
```

**Response:**
```json
{
  "success": true,
  "data": {
    "status": "started",
    "strategy": "STANDARD_BUY",
    "timestamp": 1640995200
  }
}
```

### Stop Trading
Stop all trading activities immediately.

**Endpoint:** `POST /api/trading/stop`

**Response:**
```json
{
  "success": true,
  "data": {
    "status": "stopped",
    "positions_closed": 3,
    "timestamp": 1640995200
  }
}
```

### Get Trading Status
Retrieve current trading engine status.

**Endpoint:** `GET /api/trading/status`

**Response:**
```json
{
  "success": true,
  "data": {
    "active": true,
    "strategy": "MEMECOIN_SNIPER",
    "uptime_seconds": 3600,
    "total_trades": 147,
    "success_rate": 98.7,
    "avg_latency_ms": 15.2,
    "current_positions": 5,
    "available_balance": 9750.25
  }
}
```

### Place Order
Execute a manual trade order.

**Endpoint:** `POST /api/trading/order`

**Request:**
```json
{
  "symbol": "PEPE/USDC",
  "side": "buy",
  "amount": 1000000,
  "order_type": "market",
  "slippage_percent": 2.0,
  "gas_price_gwei": 20.0
}
```

**Response:**
```json
{
  "success": true,
  "data": {
    "order_id": "ord_123456789",
    "status": "submitted",
    "estimated_execution_time_ms": 250
  }
}
```

### Get Positions
Retrieve all current trading positions.

**Endpoint:** `GET /api/trading/positions`

**Query Parameters:**
- `status`: Filter by status (`active`, `closed`, `all`)
- `limit`: Maximum number of results (default: 50)
- `offset`: Pagination offset (default: 0)

**Response:**
```json
{
  "success": true,
  "data": {
    "positions": [
      {
        "id": "pos_001",
        "symbol": "PEPE/USDC",
        "amount": 1000000,
        "entry_price": 0.000012,
        "current_price": 0.000014,
        "unrealized_pnl": 145.32,
        "status": "active",
        "opened_at": 1640995200
      }
    ],
    "total_count": 5,
    "total_value_usd": 12450.75
  }
}
```

### Get Trade History
Retrieve historical trade data.

**Endpoint:** `GET /api/trading/trades`

**Query Parameters:**
- `start_date`: ISO date string (default: 24h ago)
- `end_date`: ISO date string (default: now)
- `symbol`: Filter by trading pair
- `status`: Filter by status
- `limit`: Maximum results (default: 100)

**Response:**
```json
{
  "success": true,
  "data": {
    "trades": [
      {
        "id": "trade_001",
        "symbol": "PEPE/USDC",
        "type": "buy",
        "amount": 1000000,
        "price": 0.000012,
        "fee": 0.25,
        "gas_used": 21000,
        "execution_time_ms": 187,
        "timestamp": 1640995200,
        "status": "completed"
      }
    ],
    "total_count": 147,
    "summary": {
      "total_volume_usd": 25400.50,
      "total_fees_usd": 127.25,
      "avg_execution_time_ms": 156.8
    }
  }
}
```

## 🔧 Configuration API

### Get Configuration
Retrieve current system configuration.

**Endpoint:** `GET /api/config`

**Response:**
```json
{
  "success": true,
  "data": {
    "apis": {
      "twitter": {
        "provider": "twitter",
        "enabled": false,
        "status": "disconnected"
      }
    },
    "rpcs": {
      "ethereum": {
        "chain": "ethereum",
        "endpoint": "https://...",
        "enabled": true,
        "status": "connected"
      }
    }
  }
}
```

### Update Configuration
Update system configuration.

**Endpoint:** `PUT /api/config`

**Request:**
```json
{
  "apis": {
    "twitter": {
      "api_key": "your_api_key",
      "enabled": true
    }
  }
}
```

### Test Connection
Test connectivity to external services.

**Endpoint:** `POST /api/test-connection`

**Request:**
```json
{
  "type": "api",
  "name": "twitter",
  "config": {
    "api_key": "test_key",
    "endpoint": "https://api.twitter.com"
  }
}
```

**Response:**
```json
{
  "success": true,
  "data": {
    "connection_status": "success",
    "latency_ms": 245,
    "details": "Connection established successfully"
  }
}
```

## 💰 Wallet Management API

### Get Wallets
Retrieve all configured wallets.

**Endpoint:** `GET /api/wallets`

**Response:**
```json
{
  "success": true,
  "data": {
    "wallets": [
      {
        "id": "wallet_001",
        "address": "0x742d35Cc6634C0532925a3b8D371D6E1DaE38000",
        "name": "Primary Trading Wallet",
        "balance_eth": 1.245,
        "balance_usd": 2450.75,
        "active_trades": 3,
        "is_primary": true,
        "enabled": true
      }
    ],
    "total_count": 3,
    "total_balance_usd": 15750.50
  }
}
```

### Add Wallet
Add a new wallet to the system.

**Endpoint:** `POST /api/wallets`

**Request:**
```json
{
  "name": "Secondary Wallet",
  "private_key": "0x...",
  "enabled": true
}
```

### Remove Wallet
Remove a wallet from the system.

**Endpoint:** `DELETE /api/wallets/{wallet_id}`

## 📊 Monitoring API

### System Status
Get overall system health and status.

**Endpoint:** `GET /api/system/status`

**Response:**
```json
{
  "success": true,
  "data": {
    "status": "healthy",
    "uptime_seconds": 86400,
    "cpu_usage_percent": 45.2,
    "memory_usage_percent": 32.1,
    "active_connections": 847,
    "components": {
      "trading_engine": "healthy",
      "mev_shield": "healthy",
      "api_gateway": "healthy",
      "database": "healthy"
    }
  }
}
```

### Performance Metrics
Get detailed performance metrics.

**Endpoint:** `GET /api/metrics`

**Response:**
```json
{
  "success": true,
  "data": {
    "trading": {
      "avg_latency_ms": 15.3,
      "total_trades": 1247,
      "success_rate_percent": 98.7,
      "trades_per_second": 12.5
    },
    "system": {
      "cpu_cores": 8,
      "memory_total_gb": 16,
      "disk_usage_percent": 25.8
    },
    "network": {
      "requests_per_second": 150,
      "avg_response_time_ms": 45,
      "error_rate_percent": 0.1
    }
  }
}
```

### Alerts
Get system alerts and notifications.

**Endpoint:** `GET /api/alerts`

**Query Parameters:**
- `severity`: Filter by severity (`info`, `warning`, `error`, `critical`)
- `start_date`: ISO date string
- `limit`: Maximum results

**Response:**
```json
{
  "success": true,
  "data": {
    "alerts": [
      {
        "id": "alert_001",
        "type": "performance",
        "severity": "warning",
        "title": "High Latency Detected",
        "description": "Average execution latency above 50ms threshold",
        "timestamp": 1640995200,
        "acknowledged": false
      }
    ],
    "total_count": 5,
    "unacknowledged_count": 2
  }
}
```

## 🔒 Security Features

### Rate Limiting
All API endpoints are subject to rate limiting:

- **Global**: 1000 requests per minute
- **Trading endpoints**: 100 requests per minute
- **Burst**: Up to 50 requests in 10 seconds

Rate limit headers are included in all responses:
```
X-RateLimit-Limit: 1000
X-RateLimit-Remaining: 999
X-RateLimit-Reset: 1640995260
```

### Input Validation
All inputs are validated for:
- Data type correctness
- Range validation (amounts, percentages)
- Format validation (addresses, symbols)
- XSS and injection prevention

### Audit Logging
All API calls are logged with:
- Request details
- User identification
- Response status
- Execution time
- IP address

## 📡 WebSocket API

Real-time data is available via WebSocket connection.

### Connection
```javascript
const ws = new WebSocket('ws://localhost:8081');
```

### Message Types

#### System Metrics
```json
{
  "type": "system_metrics",
  "data": {
    "cpu": 45.2,
    "memory": 32.1,
    "timestamp": 1640995200
  }
}
```

#### Trading Updates
```json
{
  "type": "trading_update",
  "data": {
    "trade_id": "trade_001",
    "symbol": "PEPE/USDC",
    "status": "completed",
    "pnl": 145.32
  }
}
```

#### Market Data
```json
{
  "type": "market_data",
  "data": {
    "symbol": "PEPE/USDC",
    "price": 0.000014,
    "volume_24h": 1000000,
    "change_24h": 12.5
  }
}
```

## 🚨 Error Codes

| Code | Description |
|------|-------------|
| `INVALID_PARAMETER` | Request parameter is invalid |
| `INSUFFICIENT_BALANCE` | Wallet balance too low |
| `MARKET_CLOSED` | Trading not available |
| `RATE_LIMITED` | Too many requests |
| `UNAUTHORIZED` | Authentication required |
| `TRADING_DISABLED` | Trading engine stopped |
| `NETWORK_ERROR` | External service unavailable |
| `VALIDATION_ERROR` | Input validation failed |

## 📊 Response Times

Target response times for different endpoints:

| Endpoint Type | Target | 95th Percentile |
|---------------|--------|-----------------|
| Trading operations | <50ms | <100ms |
| Configuration | <100ms | <200ms |
| Monitoring | <25ms | <50ms |
| Historical data | <500ms | <1s |

## 📝 SDK Examples

### Python
```python
import requests

class HydraFlowAPI:
    def __init__(self, base_url="http://localhost:8080/api"):
        self.base_url = base_url
    
    def start_trading(self, mode="STANDARD_BUY"):
        response = requests.post(
            f"{self.base_url}/trading/start",
            json={"mode": mode}
        )
        return response.json()
    
    def get_positions(self):
        response = requests.get(f"{self.base_url}/trading/positions")
        return response.json()

# Usage
api = HydraFlowAPI()
result = api.start_trading("MEMECOIN_SNIPER")
print(result)
```

### JavaScript
```javascript
class HydraFlowAPI {
    constructor(baseUrl = 'http://localhost:8080/api') {
        this.baseUrl = baseUrl;
    }
    
    async startTrading(mode = 'STANDARD_BUY') {
        const response = await fetch(`${this.baseUrl}/trading/start`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ mode })
        });
        return response.json();
    }
    
    async getPositions() {
        const response = await fetch(`${this.baseUrl}/trading/positions`);
        return response.json();
    }
}

// Usage
const api = new HydraFlowAPI();
const result = await api.startTrading('MEMECOIN_SNIPER');
console.log(result);
```

## 🔧 Testing

Use the included API testing tools:

```bash
# Test all endpoints
./scripts/test-api.sh

# Test specific endpoint
curl -X GET http://localhost:8080/api/trading/status

# Performance test
./scripts/benchmark-api.sh
```

---

For more information, see the [main documentation](../README.md) or join our [Discord community](https://discord.gg/hydraflow).
