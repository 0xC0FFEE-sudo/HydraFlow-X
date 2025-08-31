# HydraFlow-X Example Configurations

This directory contains example configurations and templates for different trading strategies that can be used with HydraFlow-X.

## 📁 Directory Structure

```
examples/
├── strategies/          # Trading strategy configurations
│   ├── memecoin_sniper.json         # High-risk memecoin launch sniper
│   ├── conservative_arbitrage.json  # Low-risk cross-DEX arbitrage
│   └── ai_sentiment_trading.json    # AI-driven sentiment analysis
├── environments/        # Environment-specific configs
│   ├── development.json
│   ├── staging.json
│   └── production.json
└── wallets/            # Wallet configuration templates
    ├── single_wallet.json
    ├── multi_wallet.json
    └── hsm_wallet.json
```

## 🚀 Quick Start

1. **Choose a Strategy**: Copy one of the strategy files from the `strategies/` directory
2. **Customize Settings**: Modify the parameters to match your risk tolerance and goals
3. **Configure APIs**: Set up your API keys in the main configuration
4. **Load Strategy**: Use the HydraFlow-X dashboard or CLI to load your strategy

```bash
# Copy a strategy template
cp examples/strategies/conservative_arbitrage.json config/my_strategy.json

# Edit the configuration
nano config/my_strategy.json

# Load the strategy via CLI
./hydraflow-x --strategy config/my_strategy.json
```

## 📊 Available Strategies

### 1. Memecoin Sniper Strategy
**File**: `strategies/memecoin_sniper.json`
**Risk Level**: High
**Description**: Ultra-fast detection and immediate buying of newly launched memecoins

**Key Features**:
- ⚡ Sub-500ms execution time
- 🛡️ Built-in honeypot detection
- 💰 Automatic profit taking at multiple levels
- 🔄 Multi-chain support (Ethereum, Solana, Base)
- 🚀 MEV protection with private relays

**Recommended For**: Experienced traders comfortable with high-risk, high-reward scenarios

### 2. Conservative Arbitrage Strategy
**File**: `strategies/conservative_arbitrage.json`
**Risk Level**: Low
**Description**: Cross-DEX arbitrage opportunities with stable profit margins

**Key Features**:
- 📈 Minimum 0.3% profit threshold
- ⚖️ Risk-balanced position sizing
- 🔍 Pre-execution simulation
- 💎 Focus on established tokens
- 🔄 Cross-chain arbitrage support

**Recommended For**: Risk-averse traders seeking steady returns

### 3. AI Sentiment Trading Strategy
**File**: `strategies/ai_sentiment_trading.json`
**Risk Level**: Medium
**Description**: Advanced AI-driven sentiment analysis with real-time social media monitoring

**Key Features**:
- 🤖 Multi-source sentiment analysis (Twitter, Reddit, News)
- 📱 Real-time social media monitoring
- 🧠 Ensemble AI models with confidence scoring
- 📊 Technical + sentiment fusion
- ⏰ Adaptive position sizing based on confidence

**Recommended For**: Traders interested in combining fundamental sentiment with technical analysis

## ⚙️ Configuration Parameters

### Common Parameters

| Parameter | Description | Default | Range |
|-----------|-------------|---------|-------|
| `max_position_size_usd` | Maximum USD per trade | 1000 | 1-100000 |
| `max_slippage_percent` | Maximum acceptable slippage | 2.0 | 0.1-10.0 |
| `execution_timeout_ms` | Trade execution timeout | 1000 | 100-5000 |
| `mev_protection_level` | MEV protection strength | HIGH | LOW/MEDIUM/HIGH |

### Strategy-Specific Parameters

#### Memecoin Sniper
- `min_liquidity_usd`: Minimum liquidity to consider (default: 10000)
- `scan_interval_ms`: How often to scan for new tokens (default: 100)
- `honeypot_check_enabled`: Enable honeypot detection (default: true)

#### Arbitrage
- `min_profit_percent`: Minimum profit to execute trade (default: 0.3)
- `simulate_before_execute`: Run simulation first (default: true)
- `max_concurrent_arbitrages`: Max simultaneous trades (default: 3)

#### AI Sentiment
- `sentiment_threshold`: Minimum sentiment score (default: 0.7)
- `confidence_threshold`: Minimum AI confidence (default: 0.8)
- `aggregation_window_minutes`: Signal smoothing window (default: 5)

## 🔧 Customization Guide

### 1. Risk Management
Adjust these parameters based on your risk tolerance:

```json
{
  "risk_management": {
    "max_daily_trades": 50,
    "max_exposure_per_token_percent": 25.0,
    "emergency_stop_loss_percent": 5.0,
    "circuit_breaker_enabled": true
  }
}
```

### 2. Gas Optimization
Configure gas settings for each chain:

```json
{
  "chains": {
    "ethereum": {
      "gas_limit": 500000,
      "priority_fee_gwei": 2.0,
      "gas_price_multiplier": 1.2
    }
  }
}
```

### 3. Profit Taking
Set up automated profit-taking rules:

```json
{
  "profit_taking": {
    "target_profit_percent": 100.0,
    "stop_loss_percent": 50.0,
    "partial_sell_levels": [
      {"profit_percent": 50.0, "sell_percent": 25.0},
      {"profit_percent": 100.0, "sell_percent": 50.0}
    ]
  }
}
```

## 🔒 Security Considerations

1. **API Keys**: Never commit API keys to version control
2. **Position Limits**: Always set maximum position sizes
3. **Stop Losses**: Configure appropriate stop-loss levels
4. **Testing**: Test strategies in paper trading mode first
5. **Monitoring**: Enable alerts for unusual activity

## 📈 Performance Monitoring

Each strategy includes monitoring configuration:

```json
{
  "monitoring": {
    "track_performance": true,
    "alert_on_large_gains": true,
    "alert_on_large_losses": true,
    "daily_pnl_limit_usd": 5000.0
  }
}
```

## 🚨 Disclaimers

- **High Risk**: Cryptocurrency trading involves substantial risk
- **No Guarantees**: Past performance does not guarantee future results
- **Do Your Research**: Always understand the risks before trading
- **Start Small**: Begin with small position sizes when testing
- **Paper Trading**: Use paper trading mode for initial strategy validation

## 🛠️ Advanced Features

### Multi-Wallet Support
Configure multiple wallets for enhanced security and performance:

```json
{
  "wallets": {
    "primary_wallet": "0x...",
    "backup_wallets": ["0x...", "0x..."],
    "rotation_enabled": true,
    "max_trades_per_wallet": 10
  }
}
```

### MEV Protection
Enable various levels of MEV protection:

```json
{
  "mev_protection": {
    "level": "HIGH",
    "use_private_relays": true,
    "bundle_transactions": true,
    "frontrun_protection": true
  }
}
```

### Cross-Chain Trading
Configure multi-chain trading strategies:

```json
{
  "chains": {
    "ethereum": {"enabled": true, "weight": 0.5},
    "solana": {"enabled": true, "weight": 0.3},
    "base": {"enabled": true, "weight": 0.2}
  }
}
```

## 📞 Support

For questions about configuration:
1. Check the main [README.md](../README.md)
2. Review the [documentation](../docs/)
3. Join our [Discord community](https://discord.gg/hydraflow)
4. Create an issue on [GitHub](https://github.com/yourusername/HydraFlow-X)

---

**Remember**: Always thoroughly test strategies in a safe environment before deploying with real funds. The cryptocurrency market is highly volatile and unpredictable.
