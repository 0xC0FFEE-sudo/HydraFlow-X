# Getting Started with HydraFlow-X

This guide will walk you through setting up and using HydraFlow-X for algorithmic trading.

## Prerequisites

Before you begin, ensure you have the following:

- **Operating System**: Linux (Ubuntu 20.04+), macOS (10.15+), or Windows (10+ with WSL2)
- **Hardware Requirements**:
  - CPU: 4+ cores (8+ recommended for high-frequency trading)
  - RAM: 8GB minimum (16GB+ recommended)
  - Storage: 50GB+ SSD for data and logs
  - Network: Stable internet connection (100Mbps+ recommended)
- **Software Dependencies**:
  - Docker & Docker Compose (recommended)
  - Node.js 18+ (for frontend development)
  - Git

## Installation Options

### Option 1: Docker Deployment (Recommended)

This is the easiest way to get started and is recommended for most users.

```bash
# 1. Clone the repository
git clone https://github.com/hydraflow/hydraflow-x.git
cd hydraflow-x

# 2. Start all services
docker-compose up -d

# 3. Check that services are running
docker-compose ps

# 4. View logs (optional)
docker-compose logs -f hydraflow-backend
```

The application will be available at:
- **Frontend**: http://localhost:3000
- **Backend API**: http://localhost:8080
- **WebSocket**: ws://localhost:8081
- **Grafana**: http://localhost:3001
- **Prometheus**: http://localhost:9090

### Option 2: Manual Installation

For development or custom deployments.

#### Backend Setup

```bash
# 1. Install system dependencies
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    cmake \
    git \
    pkg-config \
    libssl-dev \
    libcurl4-openssl-dev \
    libpq-dev \
    libboost-all-dev \
    nlohmann-json3-dev \
    postgresql \
    redis-server

# 2. Clone and build
git clone https://github.com/hydraflow/hydraflow-x.git
cd hydraflow-x

mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)

# 3. Start database services
sudo systemctl start postgresql redis

# 4. Initialize database
sudo -u postgres createdb hydraflow
sudo -u postgres psql -d hydraflow -f ../config/init-db.sql

# 5. Start the backend
./core-backend/hydraflow-x
```

#### Frontend Setup

```bash
# 1. Navigate to frontend directory
cd hydraflow-x/frontend

# 2. Install dependencies
npm install

# 3. Configure environment
cp .env.example .env.local

# Edit .env.local with your configuration
# NEXT_PUBLIC_API_URL=http://localhost:8080
# NEXT_PUBLIC_WS_URL=ws://localhost:8081

# 4. Start development server
npm run dev
```

## Initial Configuration

### Database Configuration

1. **PostgreSQL Setup**:
   ```sql
   -- Create database and user
   CREATE DATABASE hydraflow;
   CREATE USER hydraflow WITH PASSWORD 'your_password';
   GRANT ALL PRIVILEGES ON DATABASE hydraflow TO hydraflow;

   -- Initialize schema
   \i /path/to/hydraflow-x/config/init-db.sql
   ```

2. **Redis Configuration**:
   Redis is used for caching and session management. The default configuration should work out of the box.

### API Keys and Secrets

1. **Blockchain RPC URLs**:
   - Get an API key from [Alchemy](https://alchemy.com) or [Infura](https://infura.io)
   - Configure in your environment or config file

2. **JWT Secret**:
   Generate a secure random string for JWT token signing:
   ```bash
   openssl rand -hex 32
   ```

3. **Update Configuration**:
   ```json
   {
     "blockchain": {
       "ethereum": {
         "rpc_url": "https://eth-mainnet.g.alchemy.com/v2/YOUR_API_KEY"
       },
       "solana": {
         "rpc_url": "https://api.mainnet-beta.solana.com"
       }
     },
     "auth": {
       "jwt_secret": "your_generated_secret"
     }
   }
   ```

## First Steps

### 1. Access the Dashboard

Open your browser and navigate to http://localhost:3000 (or your configured frontend URL).

### 2. Create an Account

If this is your first time:
1. Click "Sign Up" or "Register"
2. Fill in your details
3. Verify your email (if email verification is enabled)

### 3. Connect Your Wallet

1. Click on "Connect Wallet" in the top navigation
2. Choose your wallet type (MetaMask, Phantom, etc.)
3. Approve the connection request
4. Fund your account with test tokens

### 4. Explore the Interface

The main dashboard includes:
- **Portfolio Overview**: Your current positions and P&L
- **Market Data**: Real-time prices and charts
- **Trading Panel**: Place orders and manage positions
- **Analytics**: Performance metrics and reports

## Basic Trading

### Placing Your First Order

1. **Navigate to Trading**:
   - Go to the "Trading" tab
   - Select a trading pair (e.g., BTC/USDT)

2. **Choose Order Type**:
   - **Market Order**: Buy/sell at current market price
   - **Limit Order**: Set a specific price to buy/sell at
   - **Stop Order**: Automatically trigger when price reaches a level

3. **Enter Order Details**:
   ```javascript
   // Example market buy order
   {
     symbol: "BTC",
     side: "buy",
     type: "market",
     quantity: 0.01  // 0.01 BTC
   }
   ```

4. **Review and Confirm**:
   - Check the order summary
   - Confirm the transaction in your wallet
   - Wait for execution

### Monitoring Orders

1. **Order History**: View all your past orders
2. **Open Orders**: See orders that haven't been filled yet
3. **Order Status**: Track the lifecycle of your orders

```javascript
// Get order status
const orderStatus = await client.getOrder('order_12345');
console.log(orderStatus); // { status: 'filled', filled_price: 45000 }
```

## Advanced Features

### Strategy Development

1. **Built-in Strategies**:
   - Moving Average Crossover
   - RSI Divergence
   - Mean Reversion
   - Momentum Trading

2. **Custom Strategies**:
   ```cpp
   class MyStrategy : public ITradingStrategy {
   public:
       std::vector<TradingSignal> process_data(
           const MarketData& data,
           const PortfolioSnapshot& portfolio) override {
           // Your strategy logic here
           return signals;
       }
   };
   ```

3. **Backtesting**:
   ```cpp
   BacktestEngine engine;
   auto strategy = StrategyFactory::create_moving_average_strategy(10, 30);
   engine.add_strategy(std::move(strategy));

   BacktestResult result = engine.run_backtest();
   std::cout << "Sharpe Ratio: " << result.metrics.sharpe_ratio << std::endl;
   ```

### Risk Management

1. **Set Risk Limits**:
   ```javascript
   await client.setRiskLimits({
     max_position_size: 100000,
     max_portfolio_var: 0.05,
     max_drawdown_limit: 0.1
   });
   ```

2. **Monitor Risk Metrics**:
   ```javascript
   const riskMetrics = await client.getRiskMetrics();
   console.log('VaR 95%:', riskMetrics.var_95);
   ```

### API Integration

1. **REST API Usage**:
   ```bash
   # Get market prices
   curl -H "Authorization: Bearer YOUR_TOKEN" \
        http://localhost:8080/api/v1/market/prices?symbols=BTC,ETH
   ```

2. **WebSocket Streaming**:
   ```javascript
   const ws = new WebSocket('ws://localhost:8081');
   ws.onopen = () => {
     ws.send(JSON.stringify({
       type: 'subscribe',
       channel: 'market_data',
       symbols: ['BTC']
     }));
   };
   ```

## Monitoring and Maintenance

### Health Checks

```bash
# Check system health
curl http://localhost:8080/api/v1/health

# Check metrics
curl http://localhost:8080/api/v1/metrics
```

### Log Management

```bash
# View application logs
docker-compose logs -f hydraflow-backend

# View specific service logs
docker-compose logs -f postgres
```

### Performance Monitoring

1. **Grafana Dashboards**:
   - Trading Performance
   - System Health
   - Risk Metrics

2. **Prometheus Metrics**:
   - Order processing latency
   - Memory and CPU usage
   - Error rates

## Troubleshooting

### Common Issues

1. **Port Already in Use**:
   ```bash
   # Find process using port
   lsof -i :8080

   # Kill the process
   kill -9 <PID>

   # Or use a different port in docker-compose.yml
   ```

2. **Database Connection Issues**:
   ```bash
   # Check PostgreSQL status
   sudo systemctl status postgresql

   # Check connection
   psql -h localhost -U hydraflow -d hydraflow
   ```

3. **Build Failures**:
   ```bash
   # Clean build
   rm -rf build/
   mkdir build && cd build

   # Rebuild with verbose output
   cmake .. -DCMAKE_BUILD_TYPE=Debug -DCMAKE_VERBOSE_MAKEFILE=ON
   make VERBOSE=1
   ```

### Getting Help

- **Documentation**: https://docs.hydraflow.com
- **GitHub Issues**: Report bugs and request features
- **Community Forum**: Get help from other users
- **Professional Support**: Enterprise support available

## Next Steps

Now that you have HydraFlow-X running, you can:

1. **Explore Advanced Features**:
   - Multi-strategy portfolios
   - Risk parity allocation
   - Cross-exchange arbitrage

2. **Develop Custom Strategies**:
   - Machine learning models
   - Sentiment analysis
   - Options strategies

3. **Scale Your Setup**:
   - Kubernetes deployment
   - Multi-region deployment
   - High-frequency optimizations

4. **Integrate with Other Systems**:
   - TradingView webhooks
   - Excel integration
   - Custom APIs

Happy trading with HydraFlow-X! 🚀
