# Contributing to HydraFlow-X

Thank you for your interest in contributing to HydraFlow-X! This document provides guidelines and information for contributors.

## 🎯 Getting Started

### Prerequisites

- C++23 compatible compiler (GCC 13+ or Clang 15+)
- CMake 3.25+
- Git
- Basic understanding of algorithmic trading concepts
- Familiarity with blockchain technologies

### Development Environment Setup

1. **Fork and Clone**
   ```bash
   git clone https://github.com/yourusername/HydraFlow-X.git
   cd HydraFlow-X
   ```

2. **Install Dependencies**
   ```bash
   # Ubuntu/Debian
   sudo apt-get install build-essential cmake libssl-dev libpq-dev libcurl4-openssl-dev

   # macOS
   brew install cmake openssl postgresql curl
   ```

3. **Build and Test**
   ```bash
   mkdir build && cd build
   cmake -DCMAKE_BUILD_TYPE=Debug ..
   make -j$(nproc)
   make test
   ```

## 🏗️ Project Structure

```
HydraFlow-X/
├── config/                 # Configuration management
├── docs/                   # Documentation
├── hfx-ai/                 # AI and ML components
├── hfx-chain/              # Blockchain integrations
├── hfx-core/               # Core utilities
├── hfx-hft/                # High-frequency trading logic
├── hfx-ultra/              # Ultra-low latency components
├── scripts/                # Build and deployment scripts
├── web-dashboard/          # React web interface
└── third-party/            # External dependencies
```

## 📝 Contribution Types

### 🐛 Bug Fixes
- Fix compilation errors
- Resolve memory leaks
- Address performance issues
- Fix trading logic bugs

### ✨ New Features
- Add new trading strategies
- Implement additional blockchain support
- Enhance MEV protection
- Improve latency optimization

### 📚 Documentation
- API documentation
- Setup guides
- Strategy explanations
- Code comments

### 🧪 Testing
- Unit tests
- Integration tests
- Performance benchmarks
- Strategy backtesting

## 🔄 Development Workflow

### 1. Create an Issue
Before starting work, create an issue describing:
- Problem or feature request
- Proposed solution
- Implementation approach
- Breaking changes (if any)

### 2. Branch Naming Convention
```bash
git checkout -b feature/your-feature-name
git checkout -b bugfix/issue-description
git checkout -b docs/section-being-updated
```

### 3. Code Standards

#### C++ Guidelines
- Follow C++23 best practices
- Use `snake_case` for variables and functions
- Use `PascalCase` for classes and structs
- Use `SCREAMING_SNAKE_CASE` for constants
- Prefer smart pointers over raw pointers
- Use const correctness
- Include documentation for public APIs

#### Example Code Style
```cpp
#pragma once

#include <memory>
#include <string>

namespace hfx::trading {

/// @brief Ultra-fast trading engine for cryptocurrency markets
class TradingEngine {
public:
    /// @brief Initialize trading engine with configuration
    /// @param config Trading configuration parameters
    explicit TradingEngine(const TradingConfig& config);
    
    /// @brief Execute a buy order
    /// @param token_address Token contract address
    /// @param amount_in Input amount in wei
    /// @return Trade execution result
    TradeResult buy_token(const std::string& token_address, uint64_t amount_in);

private:
    static constexpr uint64_t MAX_GAS_PRICE = 100000000000ULL; // 100 gwei
    
    TradingConfig config_;
    std::unique_ptr<ExecutionEngine> execution_engine_;
    
    void validate_trade_parameters(const std::string& token, uint64_t amount) const;
};

} // namespace hfx::trading
```

#### Performance Guidelines
- Minimize memory allocations in hot paths
- Use lock-free data structures where possible
- Prefer stack allocation over heap allocation
- Profile performance-critical code
- Target <20ms decision latency
- Optimize for CPU cache locality

### 4. Testing Requirements

#### Unit Tests
```cpp
#include <gtest/gtest.h>
#include "trading_engine.hpp"

namespace hfx::trading::test {

class TradingEngineTest : public ::testing::Test {
protected:
    void SetUp() override {
        config_.max_slippage_bps = 50;
        engine_ = std::make_unique<TradingEngine>(config_);
    }
    
    TradingConfig config_;
    std::unique_ptr<TradingEngine> engine_;
};

TEST_F(TradingEngineTest, BuyTokenWithValidParameters) {
    const auto result = engine_->buy_token("0x123...", 1000000000);
    EXPECT_TRUE(result.successful);
    EXPECT_GT(result.actual_amount_out, 0);
}

} // namespace hfx::trading::test
```

#### Integration Tests
- Test with real blockchain testnets
- Validate MEV protection effectiveness
- Verify cross-chain functionality
- Test error handling and recovery

#### Performance Tests
- Latency benchmarks
- Throughput measurements
- Memory usage profiling
- Stress testing under load

### 5. Commit Message Format

Use conventional commits format:

```
type(scope): description

[optional body]

[optional footer]
```

**Types:**
- `feat`: New feature
- `fix`: Bug fix
- `docs`: Documentation changes
- `style`: Code style changes
- `refactor`: Code refactoring
- `perf`: Performance improvements
- `test`: Adding or updating tests
- `chore`: Maintenance tasks

**Examples:**
```
feat(trading): add multi-wallet sniper mode

Implement support for distributing trades across multiple wallets
to reduce detection and improve execution speed.

Closes #123
```

```
fix(mev): resolve sandwich attack detection false positives

Updated detection algorithm to reduce false positive rate from 5% to 1%
while maintaining 99% true positive rate.

Fixes #456
```

### 6. Pull Request Process

#### Before Submitting
- [ ] Code compiles without warnings
- [ ] All tests pass
- [ ] Performance benchmarks show no regression
- [ ] Documentation updated if needed
- [ ] Breaking changes documented

#### PR Template
```markdown
## Description
Brief description of changes and motivation.

## Type of Change
- [ ] Bug fix (non-breaking change which fixes an issue)
- [ ] New feature (non-breaking change which adds functionality)
- [ ] Breaking change (fix or feature that would cause existing functionality to not work as expected)
- [ ] Documentation update

## Testing
- [ ] Unit tests added/updated
- [ ] Integration tests added/updated
- [ ] Performance tests added/updated
- [ ] Manual testing performed

## Performance Impact
- Latency: [no impact/improved/degraded by X ms]
- Memory: [no impact/reduced/increased by X MB]
- CPU: [no impact/optimized/increased by X%]

## Checklist
- [ ] Code follows project style guidelines
- [ ] Self-review of code completed
- [ ] Code is well documented
- [ ] Tests added for new functionality
- [ ] All tests pass locally
- [ ] Performance regression tests pass
```

### 7. Review Process

#### Automated Checks
- Build verification
- Test suite execution
- Static analysis (clang-tidy, cppcheck)
- Performance regression detection
- Security vulnerability scanning

#### Manual Review
- Code quality and style
- Architecture and design
- Security considerations
- Performance impact
- Documentation completeness

## 🚨 Security Guidelines

### Sensitive Data
- Never commit private keys or API keys
- Use environment variables for secrets
- Encrypt sensitive configuration
- Implement proper key rotation

### Trading Security
- Validate all external inputs
- Implement rate limiting
- Use secure random number generation
- Protect against MEV attacks
- Audit smart contract interactions

### Code Security
- Avoid buffer overflows
- Use safe string operations
- Validate array bounds
- Handle exceptions properly
- Use RAII for resource management

## 🏆 Recognition

Contributors will be recognized in:
- Repository contributors list
- Release notes
- Project documentation
- Community announcements

## 📞 Getting Help

### Communication Channels
- **GitHub Issues**: Bug reports and feature requests
- **Discord**: Real-time discussion and help
- **Telegram**: Community support
- **Email**: Direct contact for sensitive issues

### Code Review Process
- All PRs require at least 2 reviews
- Performance-critical changes require additional review
- Security-related changes require security team review
- Breaking changes require core team approval

### Mentorship
New contributors can request mentorship for:
- Project architecture understanding
- Performance optimization techniques
- Trading strategy development
- Blockchain integration

## 📋 Issue Labels

- `bug`: Something isn't working
- `enhancement`: New feature or request
- `documentation`: Improvements or additions to docs
- `performance`: Performance optimization
- `security`: Security-related issue
- `good first issue`: Good for newcomers
- `help wanted`: Extra attention is needed
- `priority:high`: High priority issue
- `breaking-change`: Breaking change

## 🎯 Roadmap Contributions

Priority areas for contributions:
1. **Latency Optimization**: Sub-10ms decision latency
2. **MEV Protection**: Advanced protection strategies
3. **Multi-Chain**: Additional blockchain support
4. **AI Enhancement**: Improved ML models
5. **Documentation**: Comprehensive guides
6. **Testing**: Increased test coverage
7. **Security**: Enhanced security measures

Thank you for contributing to HydraFlow-X! Together, we're building the fastest and most advanced open-source trading platform.
