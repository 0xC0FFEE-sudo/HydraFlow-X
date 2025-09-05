# HydraFlow-X API Documentation

## Overview

HydraFlow-X provides both REST and WebSocket APIs for programmatic access to trading functionality, market data, and system management.

## Authentication

All API requests require authentication using JWT tokens obtained through the login endpoint.

### Login

```http
POST /api/v1/auth/login
Content-Type: application/json

{
  "username": "your_username",
  "password": "your_password"
}
```

**Response:**
```json
{
  "success": true,
  "token": "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9...",
  "expires_in": 3600,
  "user": {
    "id": "user_123",
    "username": "your_username",
    "role": "trader"
  }
}
```

### Using JWT Tokens

Include the token in the Authorization header:

```
Authorization: Bearer eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9...
```

## REST API Endpoints

### System Management

#### Get System Status
```http
GET /api/v1/status
Authorization: Bearer <token>
```

**Response:**
```json
{
  "status": "healthy",
  "version": "1.0.0",
  "uptime": 3600,
  "timestamp": "2023-12-01T12:00:00Z"
}
```

#### Get System Information
```http
GET /api/v1/system/info
Authorization: Bearer <token>
```

**Response:**
```json
{
  "cpu_usage": 45.2,
  "memory_usage": 2.3,
  "disk_usage": 15.7,
  "network_connections": 150,
  "active_threads": 24,
  "latency_p95": 0.00005
}
```

### Trading Operations

#### Place Order
```http
POST /api/v1/orders
Authorization: Bearer <token>
Content-Type: application/json

{
  "symbol": "BTC",
  "side": "buy",
  "type": "market",
  "quantity": 0.1,
  "price": 45000.0
}
```

**Parameters:**
- `symbol` (string): Trading pair symbol
- `side` (string): "buy" or "sell"
- `type` (string): "market", "limit", "stop", "stop_limit"
- `quantity` (number): Order quantity
- `price` (number): Limit price (required for limit orders)

**Response:**
```json
{
  "order_id": "order_12345",
  "status": "pending",
  "timestamp": "2023-12-01T12:00:00Z",
  "symbol": "BTC",
  "side": "buy",
  "type": "market",
  "quantity": 0.1,
  "price": 45000.0
}
```

#### Get Order History
```http
GET /api/v1/orders?symbol=BTC&status=filled&limit=50
Authorization: Bearer <token>
```

**Query Parameters:**
- `symbol` (string): Filter by symbol
- `status` (string): Filter by status ("pending", "filled", "cancelled")
- `start_date` (string): Start date (ISO 8601)
- `end_date` (string): End date (ISO 8601)
- `limit` (number): Maximum results (default: 100)

**Response:**
```json
{
  "orders": [
    {
      "order_id": "order_12345",
      "symbol": "BTC",
      "side": "buy",
      "type": "market",
      "quantity": 0.1,
      "price": 45000.0,
      "status": "filled",
      "filled_quantity": 0.1,
      "filled_price": 44950.0,
      "timestamp": "2023-12-01T12:00:00Z",
      "fill_timestamp": "2023-12-01T12:00:01Z"
    }
  ],
  "total": 1,
  "page": 1
}
```

#### Cancel Order
```http
DELETE /api/v1/orders/{order_id}
Authorization: Bearer <token>
```

**Response:**
```json
{
  "success": true,
  "order_id": "order_12345",
  "status": "cancelled"
}
```

### Market Data

#### Get Current Prices
```http
GET /api/v1/market/prices?symbols=BTC,ETH,SOL
Authorization: Bearer <token>
```

**Response:**
```json
{
  "prices": {
    "BTC": {
      "price": 45000.0,
      "change_24h": 2.5,
      "volume_24h": 1500000.0,
      "timestamp": "2023-12-01T12:00:00Z"
    },
    "ETH": {
      "price": 2400.0,
      "change_24h": -1.2,
      "volume_24h": 800000.0,
      "timestamp": "2023-12-01T12:00:00Z"
    }
  }
}
```

#### Get Order Book
```http
GET /api/v1/market/orderbook?symbol=BTC&depth=20
Authorization: Bearer <token>
```

**Response:**
```json
{
  "symbol": "BTC",
  "bids": [
    [44950.0, 0.5],
    [44940.0, 1.2],
    [44930.0, 0.8]
  ],
  "asks": [
    [44960.0, 0.3],
    [44970.0, 0.9],
    [44980.0, 1.1]
  ],
  "timestamp": "2023-12-01T12:00:00Z"
}
```

### Portfolio Management

#### Get Portfolio Summary
```http
GET /api/v1/portfolio
Authorization: Bearer <token>
```

**Response:**
```json
{
  "total_value": 125000.0,
  "cash": 25000.0,
  "equity": 100000.0,
  "unrealized_pnl": 5000.0,
  "realized_pnl": 15000.0,
  "total_pnl": 20000.0,
  "positions": [
    {
      "symbol": "BTC",
      "quantity": 1.5,
      "entry_price": 42000.0,
      "current_price": 45000.0,
      "unrealized_pnl": 4500.0,
      "pnl_percentage": 7.14
    }
  ],
  "timestamp": "2023-12-01T12:00:00Z"
}
```

#### Get Account Balances
```http
GET /api/v1/balances
Authorization: Bearer <token>
```

**Response:**
```json
{
  "balances": {
    "BTC": {
      "total": 2.5,
      "available": 1.0,
      "locked": 1.5
    },
    "ETH": {
      "total": 10.0,
      "available": 10.0,
      "locked": 0.0
    },
    "USDT": {
      "total": 50000.0,
      "available": 25000.0,
      "locked": 25000.0
    }
  },
  "timestamp": "2023-12-01T12:00:00Z"
}
```

### Risk Management

#### Get Risk Metrics
```http
GET /api/v1/risk/metrics
Authorization: Bearer <token>
```

**Response:**
```json
{
  "portfolio_value": 125000.0,
  "total_pnl": 20000.0,
  "sharpe_ratio": 2.1,
  "max_drawdown": 8.5,
  "var_95": 5000.0,
  "position_count": 3,
  "concentration_risk": 15.2,
  "timestamp": "2023-12-01T12:00:00Z"
}
```

#### Set Risk Limits
```http
POST /api/v1/risk/limits
Authorization: Bearer <token>
Content-Type: application/json

{
  "max_position_size": 100000,
  "max_portfolio_var": 0.05,
  "max_drawdown_limit": 0.1,
  "max_concentration": 0.2
}
```

**Response:**
```json
{
  "success": true,
  "limits": {
    "max_position_size": 100000,
    "max_portfolio_var": 0.05,
    "max_drawdown_limit": 0.1,
    "max_concentration": 0.2
  }
}
```

### MEV Protection

#### Get MEV Status
```http
GET /api/v1/mev/status
Authorization: Bearer <token>
```

**Response:**
```json
{
  "protection_enabled": true,
  "preferred_relays": ["flashbots", "eden", "bloxroute"],
  "active_protections": ["private_transaction", "bundle_routing"],
  "stats": {
    "transactions_protected": 150,
    "attacks_prevented": 5,
    "avg_protection_time": 0.0001
  }
}
```

#### Protect Transaction
```http
POST /api/v1/mev/protect-transaction
Authorization: Bearer <token>
Content-Type: application/json

{
  "chain": "ethereum",
  "transaction": {
    "to": "0x123...",
    "value": "1000000000000000000",
    "data": "0x...",
    "gas_limit": 21000,
    "max_fee_per_gas": 50000000000,
    "max_priority_fee_per_gas": 2000000000
  },
  "protection_level": "maximum"
}
```

**Response:**
```json
{
  "transaction_hash": "0xabc...",
  "protection_applied": true,
  "relay_used": "flashbots",
  "estimated_protection_cost": 0.001,
  "estimated_confirmation_time": 30
}
```

### Monitoring

#### Get Metrics
```http
GET /api/v1/metrics
Authorization: Bearer <token>
```

**Response:**
```json
{
  "orders_per_second": 150.5,
  "trades_per_second": 145.2,
  "average_latency": 0.00005,
  "error_rate": 0.001,
  "memory_usage": 2.3,
  "cpu_usage": 45.2,
  "active_connections": 150,
  "timestamp": "2023-12-01T12:00:00Z"
}
```

#### Get Health Status
```http
GET /api/v1/health
Authorization: Bearer <token>
```

**Response:**
```json
{
  "status": "healthy",
  "checks": {
    "database": "healthy",
    "redis": "healthy",
    "blockchain_rpc": "healthy",
    "websocket_server": "healthy",
    "risk_engine": "healthy"
  },
  "timestamp": "2023-12-01T12:00:00Z"
}
```

## WebSocket API

### Connection

```javascript
const ws = new WebSocket('ws://localhost:8081');

// Connection established
ws.onopen = () => {
  console.log('Connected to HydraFlow-X WebSocket');
};

// Handle messages
ws.onmessage = (event) => {
  const data = JSON.parse(event.data);
  handleMessage(data);
};

// Handle errors
ws.onerror = (error) => {
  console.error('WebSocket error:', error);
};

// Handle disconnection
ws.onclose = () => {
  console.log('Disconnected from HydraFlow-X WebSocket');
};
```

### Authentication

```javascript
// Authenticate after connection
ws.send(JSON.stringify({
  type: 'auth',
  token: 'your_jwt_token_here'
}));
```

### Subscriptions

#### Market Data
```javascript
// Subscribe to market data
ws.send(JSON.stringify({
  type: 'subscribe',
  channel: 'market_data',
  symbols: ['BTC', 'ETH', 'SOL']
}));

// Unsubscribe from market data
ws.send(JSON.stringify({
  type: 'unsubscribe',
  channel: 'market_data',
  symbols: ['BTC']
}));
```

#### Portfolio Updates
```javascript
// Subscribe to portfolio updates
ws.send(JSON.stringify({
  type: 'subscribe',
  channel: 'portfolio'
}));
```

#### Order Updates
```javascript
// Subscribe to order updates
ws.send(JSON.stringify({
  type: 'subscribe',
  channel: 'orders'
}));
```

#### Risk Alerts
```javascript
// Subscribe to risk alerts
ws.send(JSON.stringify({
  type: 'subscribe',
  channel: 'risk_alerts'
}));
```

### Message Format

#### Market Data Update
```json
{
  "type": "market_data",
  "channel": "market_data",
  "data": {
    "symbol": "BTC",
    "price": 45000.0,
    "change_24h": 2.5,
    "volume_24h": 1500000.0,
    "timestamp": "2023-12-01T12:00:00Z"
  }
}
```

#### Portfolio Update
```json
{
  "type": "portfolio_update",
  "channel": "portfolio",
  "data": {
    "total_value": 125000.0,
    "unrealized_pnl": 5000.0,
    "positions": [
      {
        "symbol": "BTC",
        "quantity": 1.5,
        "current_price": 45000.0,
        "unrealized_pnl": 4500.0
      }
    ],
    "timestamp": "2023-12-01T12:00:00Z"
  }
}
```

#### Order Update
```json
{
  "type": "order_update",
  "channel": "orders",
  "data": {
    "order_id": "order_12345",
    "symbol": "BTC",
    "status": "filled",
    "filled_quantity": 0.1,
    "filled_price": 44950.0,
    "timestamp": "2023-12-01T12:00:01Z"
  }
}
```

#### Risk Alert
```json
{
  "type": "risk_alert",
  "channel": "risk_alerts",
  "data": {
    "alert_type": "position_limit_exceeded",
    "symbol": "BTC",
    "current_size": 150000,
    "limit": 100000,
    "severity": "high",
    "timestamp": "2023-12-01T12:00:00Z"
  }
}
```

## Error Handling

### HTTP Status Codes

- `200 OK`: Request successful
- `201 Created`: Resource created successfully
- `400 Bad Request`: Invalid request parameters
- `401 Unauthorized`: Authentication required or invalid token
- `403 Forbidden`: Insufficient permissions
- `404 Not Found`: Resource not found
- `429 Too Many Requests`: Rate limit exceeded
- `500 Internal Server Error`: Server error
- `503 Service Unavailable`: Service temporarily unavailable

### Error Response Format

```json
{
  "error": {
    "code": "INVALID_REQUEST",
    "message": "Invalid request parameters",
    "details": {
      "field": "quantity",
      "issue": "must be positive"
    }
  },
  "timestamp": "2023-12-01T12:00:00Z"
}
```

### Rate Limiting

API requests are rate limited to prevent abuse:

- **REST API**: 100 requests per minute per IP
- **WebSocket**: 1000 messages per minute per connection
- **Trading operations**: 50 orders per minute per user

Rate limit headers are included in responses:

```
X-RateLimit-Limit: 100
X-RateLimit-Remaining: 95
X-RateLimit-Reset: 1701436800
Retry-After: 60
```

## SDKs and Libraries

### Python SDK

```python
from hydraflow import Client

# Initialize client
client = Client(api_key='your_api_key', secret='your_secret')

# Get market prices
prices = client.get_prices(['BTC', 'ETH'])
print(prices)

# Place order
order = client.place_order(
    symbol='BTC',
    side='buy',
    type='market',
    quantity=0.1
)
print(order)

# Subscribe to WebSocket
client.subscribe_market_data(['BTC'], callback=on_market_data)
```

### JavaScript SDK

```javascript
import { HydraFlowClient } from 'hydraflow-js-sdk';

const client = new HydraFlowClient({
  apiKey: 'your_api_key',
  secret: 'your_secret'
});

// Get portfolio
const portfolio = await client.getPortfolio();
console.log(portfolio);

// WebSocket subscription
client.subscribe('market_data', ['BTC'], (data) => {
  console.log('Market data:', data);
});
```

## Best Practices

### Authentication
- Store JWT tokens securely
- Implement token refresh logic
- Handle token expiration gracefully

### Error Handling
- Implement exponential backoff for retries
- Monitor rate limits and handle 429 responses
- Log errors for debugging

### Performance
- Use WebSocket for real-time data
- Batch requests when possible
- Implement connection pooling

### Security
- Use HTTPS for all API calls
- Validate input data on the client side
- Implement proper session management
