# HydraFlow-X Core Backend

This directory contains the core backend implementation of the HydraFlow-X ultra-low latency DeFi trading platform.

## 🏗️ Architecture

The core backend is organized into specialized modules:

### Core Modules

- **`hfx-core/`** - Core engine components (event system, memory pools, lockfree queues)
- **`hfx-ultra/`** - Ultra-low latency trading engines (MEV, V3 tick engine, smart trading)
- **`hfx-ai/`** - AI/ML components (sentiment analysis, LLM decisions, backtesting)
- **`hfx-api/`** - REST API server for web dashboard integration
- **`hfx-net/`** - High-performance networking (WebSocket, QUIC, TLS)
- **`hfx-log/`** - Ultra-fast logging system with binary WAL

### Configuration & Security

- **`config/`** - Configuration management system
- **`security/`** - Security hardening and audit logging
- **`src/main.cpp`** - Main application entry point

## 🚀 Building

From the project root:

```bash
# Build core backend only
cmake -DBUILD_CORE_BACKEND=ON -DBUILD_WEB_DASHBOARD=OFF ..
make

# Build everything
cmake ..
make
```

## 🔌 Integration with Web Dashboard

The core backend provides:

- **REST API** (`hfx-api`) for configuration and control
- **WebSocket streaming** for real-time data feeds
- **Configuration endpoints** for dynamic parameter updates
- **Monitoring APIs** for performance metrics and alerts

The web dashboard (`../web-dashboard`) connects to these APIs to provide:

- Trading strategy configuration
- Real-time performance monitoring  
- Portfolio management interface
- Risk management controls
- Backtesting and strategy analysis

## 📊 Key Features

### Ultra-Low Latency
- Sub-20ms decision latency
- Zero-copy data structures
- Hardware-optimized assembly
- Custom memory allocators

### Multi-Chain Support
- Ethereum (Uniswap V3)
- Solana (Jupiter, Raydium)
- Base, Arbitrum
- Cross-chain arbitrage

### MEV Protection
- Flashbots integration
- Eden Network support
- Jito Block Engine
- Private mempool access

### AI-Powered Trading
- Real-time sentiment analysis
- LLM-based decision making
- Multi-source data fusion
- Autonomous strategy adaptation

### Risk Management
- Dynamic position sizing
- Real-time drawdown monitoring
- Automated emergency stops
- Hardware security modules (HSM)

## 🔧 Development

### Adding New Features

1. Choose the appropriate module (`hfx-*`)
2. Add headers to `include/` directory
3. Implement in `src/` directory
4. Update module's `CMakeLists.txt`
5. Add tests if `BUILD_TESTS=ON`

### Module Dependencies

```
hfx-ultra  →  hfx-core, hfx-net, hfx-log
hfx-ai     →  hfx-core, hfx-ultra  
hfx-api    →  hfx-ultra, hfx-ai
main.cpp   →  All modules
```

### Threading Model

- **Main Thread**: Coordination and lifecycle
- **Trading Threads**: One per trading engine
- **Network Threads**: WebSocket and API handling
- **AI Threads**: Sentiment analysis and LLM inference
- **Logging Thread**: Async log processing

## 📈 Performance Targets

- **Order Latency**: < 20ms end-to-end
- **Market Data**: < 1ms processing
- **Decision Making**: < 5ms (including AI)
- **Risk Checks**: < 100μs
- **Database Writes**: < 1ms

## 🛡️ Security

- Hardware Security Module (HSM) integration
- Encrypted private key storage
- Audit logging of all operations
- Rate limiting and DDoS protection
- Secure WebSocket connections (WSS)

## 📋 Status

✅ **Core Infrastructure** - Complete
✅ **Trading Engines** - Complete  
✅ **AI/ML Pipeline** - Complete
✅ **REST API** - Complete
✅ **WebSocket Streaming** - Complete
🟡 **Production Database** - In Progress
🟡 **NATS JetStream** - In Progress
🟡 **Comprehensive Testing** - In Progress
