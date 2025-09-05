# HydraFlow-X: Ultra-Low Latency DeFi Trading Platform

[![Build Status](https://img.shields.io/badge/build-passing-brightgreen)](https://github.com/hydraflow/hydraflow-x)
[![License](https://img.shields.io/badge/license-MIT-blue)](LICENSE)
[![C++](https://img.shields.io/badge/C%2B%2B-23-orange)](https://isocpp.org/)
[![Docker](https://img.shields.io/badge/docker-ready-blue)](https://docker.com)

HydraFlow-X is a next-generation algorithmic trading platform designed for ultra-low latency execution in DeFi markets. Built with modern C++23, it provides sub-50μs order-to-wire latency while maintaining enterprise-grade reliability and security.

## 🚀 Key Features

### ⚡ Performance
- **Sub-50μs order-to-wire latency** through optimized C++ architecture
- **NUMA-aware multi-threading** with CPU affinity isolation
- **Hardware timestamping (RDTSC)** for precise timing
- **Zero-copy networking** with DPDK bypass capabilities
- **Real-time kernel optimizations** (PREEMPT_RT)

### 🔒 Security & Reliability
- **Multi-layered security** with rate limiting, DDoS protection, and API security
- **MEV protection** with private transactions and bundle routing
- **Comprehensive risk management** with circuit breakers and position limits
- **Enterprise-grade monitoring** with Prometheus/Grafana integration
- **Fault-tolerant architecture** with automatic failover

### 📊 Analytics & Intelligence
- **Real-time sentiment analysis** from news, social media, and on-chain data
- **Advanced backtesting engine** with Monte Carlo simulations
- **Machine learning integration** for predictive analytics
- **Portfolio optimization** with modern portfolio theory
- **Live market data** from multiple DEXs and CEXs

### 🌐 Blockchain Integration
- **Ethereum RPC client** with L2 support (Arbitrum, Optimism, Base)
- **Solana RPC client** with Jito MEV bundle support
- **Multi-chain DEX integration** (Uniswap V3, Raydium, PancakeSwap)
- **Permit2 gasless approvals** for seamless trading
- **Flashbots integration** for MEV protection

## 🏗️ Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                    Frontend (Next.js 14)                        │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │ Real-time Charts │ Analytics Dashboard │ Trading Panel │   │
│  └─────────────────────────────────────────────────────────┘   │
└─────────────────────────────────┬───────────────────────────────┘
                                  │ WebSocket/REST API
┌─────────────────────────────────┴───────────────────────────────┐
│                    Backend (C++23)                              │
│                                                                 │
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐ ┌─────────────┐ │
│  │Event Engine │ │Network Mgr  │ │Chain Manager│ │Strategy Mgr │ │
│  └─────────────┘ └─────────────┘ └─────────────┘ └─────────────┘ │
│                                                                 │
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐ ┌─────────────┐ │
│  │Risk Manager │ │MEV Shield   │ │Order Router │ │Position Mgr │ │
│  └─────────────┘ └─────────────┘ └─────────────┘ └─────────────┘ │
│                                                                 │
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐ ┌─────────────┐ │
│  │Logger       │ │Telemetry    │ │WebSocket Svr│ │REST API Svr │ │
│  └─────────────┘ └─────────────┘ └─────────────┘ └─────────────┘ │
└─────────────────────────────────────────────────────────────────┘
                                  │
                    ┌─────────────┴─────────────┐
                    │ Database & Cache Layer   │
                    │ PostgreSQL │ Redis │ CH  │
                    └──────────────────────────┘
```

## 📋 Table of Contents

- [Quick Start](#quick-start)
- [Installation](#installation)
- [Configuration](#configuration)
- [API Documentation](#api-documentation)
- [Trading Strategies](#trading-strategies)
- [Backtesting](#backtesting)
- [Monitoring](#monitoring)
- [Deployment](#deployment)
- [Contributing](#contributing)
- [License](#license)

## 🚀 Quick Start

### Prerequisites

- **C++23 compatible compiler** (GCC 12+, Clang 15+, MSVC 2022+)
- **CMake 3.20+**
- **Docker & Docker Compose**
- **PostgreSQL 15+**
- **Redis 7+**
- **Node.js 18+** (for frontend)

### Docker Deployment (Recommended)

```bash
# Clone the repository
git clone https://github.com/hydraflow/hydraflow-x.git
cd hydraflow-x

# Start all services
docker-compose up -d

# Check service status
docker-compose ps

# View logs
docker-compose logs -f hydraflow-backend
```

### Manual Installation

```bash
# Install system dependencies
sudo apt-get update
sudo apt-get install -y build-essential cmake git pkg-config \
    libssl-dev libcurl4-openssl-dev libpq-dev libboost-all-dev \
    nlohmann-json3-dev postgresql redis-server

# Clone and build
git clone https://github.com/hydraflow/hydraflow-x.git
cd hydraflow-x

mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
make install

# Start services
sudo systemctl start postgresql redis
./hydraflow-x
```

## 📖 API Documentation

### REST API Endpoints

#### System Management
```http
GET  /api/v1/status           # System status
GET  /api/v1/system/info      # System information
GET  /api/v1/health           # Health check
```

#### Trading Operations
```http
POST /api/v1/orders           # Place new order
GET  /api/v1/orders           # Get order history
GET  /api/v1/orders/{id}      # Get specific order
DELETE /api/v1/orders/{id}    # Cancel order
```

#### Market Data
```http
GET  /api/v1/market/prices    # Current market prices
GET  /api/v1/market/tickers   # Market tickers
GET  /api/v1/market/orderbook # Order book data
```

#### Portfolio Management
```http
GET  /api/v1/portfolio        # Portfolio summary
GET  /api/v1/positions        # Current positions
GET  /api/v1/balances         # Account balances
```

#### Risk Management
```http
GET  /api/v1/risk/metrics     # Risk metrics
POST /api/v1/risk/limits      # Set risk limits
GET  /api/v1/risk/violations  # Risk violations
```

#### Authentication
```http
POST /api/v1/auth/login       # User login
POST /api/v1/auth/logout      # User logout
POST /api/v1/auth/refresh     # Refresh token
GET  /api/v1/auth/verify      # Verify token
```

### WebSocket API

```javascript
// Connect to WebSocket
const ws = new WebSocket('ws://localhost:8081');

// Subscribe to market data
ws.send(JSON.stringify({
  type: 'subscribe',
  channel: 'market_data',
  symbols: ['BTC', 'ETH']
}));

// Subscribe to portfolio updates
ws.send(JSON.stringify({
  type: 'subscribe',
  channel: 'portfolio'
}));

// Handle incoming messages
ws.onmessage = (event) => {
  const data = JSON.parse(event.data);
  console.log('Received:', data);
};
```

### Authentication

All API requests require authentication using JWT tokens:

```bash
# Login to get token
curl -X POST http://localhost:8080/api/v1/auth/login \
  -H "Content-Type: application/json" \
  -d '{"username":"admin","password":"password"}'

# Use token in subsequent requests
curl -H "Authorization: Bearer YOUR_JWT_TOKEN" \
  http://localhost:8080/api/v1/portfolio
```

## 📈 Trading Strategies

### Built-in Strategies

1. **Moving Average Crossover**
   - Fast/Slow MA crossover signals
   - Configurable periods and thresholds

2. **RSI Divergence**
   - RSI overbought/oversold signals
   - Divergence detection

3. **Mean Reversion**
   - Statistical arbitrage
   - Z-score based entry/exit

4. **Momentum Trading**
   - Rate of change signals
   - Volume confirmation

### Custom Strategy Development

```cpp
class MyStrategy : public ITradingStrategy {
public:
    void initialize(const StrategyParameters& params) override {
        // Initialize strategy parameters
    }

    std::vector<TradingSignal> process_data(
        const MarketData& data,
        const PortfolioSnapshot& portfolio) override {

        std::vector<TradingSignal> signals;

        // Implement your strategy logic here
        // Example: Simple trend following
        if (data.close > data.open * 1.02) { // 2% up
            TradingSignal signal;
            signal.type = SignalType::BUY;
            signal.symbol = data.symbol;
            signal.quantity = 1.0;
            signals.push_back(signal);
        }

        return signals;
    }

    // ... other required methods
};
```

## 🔬 Backtesting

### Quick Backtest

```cpp
// Create backtesting engine
BacktestEngine engine;

// Configure backtest
BacktestConfig config;
config.symbols = {"BTC", "ETH"};
config.initial_capital = 100000.0;
config.start_date = std::chrono::sys_days{2023y/1/1};
config.end_date = std::chrono::sys_days{2023y/12/31};

// Add strategy
auto strategy = StrategyFactory::create_moving_average_strategy(10, 30);
engine.add_strategy(std::move(strategy));

// Set data source
auto data_source = std::make_unique<CSVDataSource>("/path/to/data");
engine.set_data_source(std::move(data_source));

// Run backtest
BacktestResult result = engine.run_backtest();

// Print results
std::cout << "Total Return: " << result.metrics.total_return * 100 << "%" << std::endl;
std::cout << "Sharpe Ratio: " << result.metrics.sharpe_ratio << std::endl;
std::cout << "Max Drawdown: " << result.metrics.max_drawdown << "%" << std::endl;
```

### Advanced Backtesting Features

- **Walk-forward analysis** for strategy validation
- **Monte Carlo simulation** for risk assessment
- **Parameter optimization** using genetic algorithms
- **Multi-strategy portfolios**
- **Transaction cost modeling**
- **Slippage simulation**

## 📊 Monitoring & Analytics

### Prometheus Metrics

```yaml
# Key metrics to monitor
hydraflow_orders_total: Total orders processed
hydraflow_trades_total: Total trades executed
hydraflow_pnl_current: Current P&L
hydraflow_latency_orders: Order processing latency
hydraflow_error_rate: Error rate by component
hydraflow_memory_usage: Memory usage by component
hydraflow_cpu_usage: CPU usage by thread
```

### Grafana Dashboards

Pre-configured dashboards for:
- **Trading Performance**: P&L, Sharpe ratio, drawdown
- **System Health**: CPU, memory, latency, error rates
- **Market Data**: Price feeds, order book depth
- **Risk Metrics**: VaR, position sizes, leverage
- **Strategy Analytics**: Win rate, profit factor, holding time

### Alerting

Built-in alerts for:
- High latency (>100μs)
- Memory usage (>90%)
- Error rate spikes
- Risk limit breaches
- Strategy performance degradation

## 🚀 Deployment

### Kubernetes Deployment

```bash
# Deploy to Kubernetes
kubectl apply -f k8s/

# Check deployment status
kubectl get pods
kubectl get services

# Scale the deployment
kubectl scale deployment hydraflow-backend --replicas=3
```

### Docker Compose (Development)

```yaml
version: '3.8'
services:
  hydraflow-backend:
    image: hydraflow/backend:latest
    ports:
      - "8080:8080"
    environment:
      - POSTGRES_HOST=postgres
      - REDIS_HOST=redis

  postgres:
    image: postgres:15-alpine
    environment:
      POSTGRES_DB: hydraflow

  redis:
    image: redis:7-alpine
```

### Production Considerations

- **Load Balancing**: Use nginx or HAProxy for load distribution
- **Database Clustering**: PostgreSQL with streaming replication
- **Redis Clustering**: Redis Cluster for high availability
- **Monitoring Stack**: Prometheus + Grafana + AlertManager
- **Security**: TLS 1.3, API rate limiting, DDoS protection
- **Backup Strategy**: Automated backups with point-in-time recovery

## 🤝 Contributing

We welcome contributions! Please see our [Contributing Guide](CONTRIBUTING.md) for details.

### Development Setup

```bash
# Fork and clone the repository
git clone https://github.com/your-username/hydraflow-x.git
cd hydraflow-x

# Create feature branch
git checkout -b feature/your-feature-name

# Build and test
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
make -j$(nproc)
ctest

# Run tests
./tests/hfx-tests-unit
./tests/hfx-tests-performance
```

### Code Style

- **C++**: Follow Modern C++ guidelines (C++ Core Guidelines)
- **Documentation**: Doxygen comments for all public APIs
- **Testing**: Unit tests for all new features
- **Performance**: Profile and optimize critical paths

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 🙏 Acknowledgments

- [Boost C++ Libraries](https://www.boost.org/)
- [nlohmann/json](https://github.com/nlohmann/json)
- [Prometheus](https://prometheus.io/)
- [Grafana](https://grafana.com/)

## 📞 Support

- **Documentation**: [docs.hydraflow.com](https://docs.hydraflow.com)
- **Issues**: [GitHub Issues](https://github.com/hydraflow/hydraflow-x/issues)
- **Discussions**: [GitHub Discussions](https://github.com/hydraflow/hydraflow-x/discussions)
- **Email**: support@hydraflow.com

---

**Built with ❤️ for the DeFi community**
