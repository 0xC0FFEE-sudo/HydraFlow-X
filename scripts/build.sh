#!/bin/bash

# HydraFlow-X AI Trading System Build Script
# Optimized for current architecture with AI components

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

echo "🤖 HydraFlow-X AI Trading System Build"
echo "====================================="

# Detect current architecture
CURRENT_ARCH=$(uname -m)
HOST_OS=$(uname -s)

echo -e "${CYAN}🔍 System Detection:${NC}"
echo -e "   OS: ${HOST_OS}"
echo -e "   Architecture: ${CURRENT_ARCH}"

if [[ "$HOST_OS" != "Darwin" ]]; then
    echo -e "${RED}❌ Error: This system is optimized for macOS${NC}"
    exit 1
fi

# Project root
cd "$(dirname "$(dirname "$(realpath "$0")")")"

echo -e "${BLUE}🧹 Cleaning previous builds...${NC}"
rm -rf build
mkdir -p build logs

echo -e "${BLUE}📦 Installing dependencies...${NC}"

# Check for Homebrew
if ! command -v brew &> /dev/null; then
    echo -e "${YELLOW}Installing Homebrew...${NC}"
    /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
    
    if [[ "$CURRENT_ARCH" == "arm64" ]]; then
        eval "$(/opt/homebrew/bin/brew shellenv)"
    else
        eval "$(/usr/local/bin/brew shellenv)"
    fi
fi

# Core dependencies
brew update > /dev/null 2>&1
for pkg in cmake ninja pkg-config llvm python@3.11 node npm; do
    if ! brew list "$pkg" &> /dev/null; then
        echo -e "   Installing ${pkg}..."
        brew install "$pkg" > /dev/null 2>&1
    fi
done

# AI/ML dependencies (skip heavy packages for now, focus on build)
echo -e "   ✅ AI/ML dependencies (will install as needed)"

echo -e "${BLUE}🐍 Setting up Python environment...${NC}"
if [[ ! -d "venv" ]]; then
    python3 -m venv venv
fi
source venv/bin/activate
pip install --upgrade pip > /dev/null 2>&1
pip install -r requirements.txt > /dev/null 2>&1

echo -e "${PURPLE}🏗️  Building HydraFlow-X AI...${NC}"
cd build

# Configure CMake for current architecture
cmake .. \
    -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_STANDARD=23 \
    -DHFX_ENABLE_AI=ON \
    -DHFX_ENABLE_VISUALIZATION=ON \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
    > ../logs/cmake.log 2>&1

if [ $? -ne 0 ]; then
    echo -e "${RED}❌ CMake configuration failed${NC}"
    echo -e "   Check logs/cmake.log for details"
    exit 1
fi

# Build
ninja -j$(sysctl -n hw.ncpu) > ../logs/build.log 2>&1

if [ $? -ne 0 ]; then
    echo -e "${RED}❌ Build failed${NC}"
    echo -e "   Check logs/build.log for details"
    exit 1
fi

echo -e "${GREEN}✅ Build successful!${NC}"

# Verify binary
if [[ -f "hydraflow-x" ]]; then
    local binary_arch=$(file hydraflow-x | grep -o "arm64\|x86_64")
    echo -e "${BLUE}   📦 Binary architecture: ${binary_arch}${NC}"
fi

cd ..

# Setup web dashboard
echo -e "${BLUE}🌐 Setting up AI Dashboard...${NC}"
cd web-dashboard

if [[ ! -d "node_modules" ]]; then
    npm install > /dev/null 2>&1
fi

npm run build > /dev/null 2>&1
cd ..

echo ""
echo -e "${GREEN}🎉 HydraFlow-X AI Trading System Ready!${NC}"
echo -e "${CYAN}=========================================${NC}"
echo ""
echo -e "${YELLOW}🚀 Quick Start:${NC}"
echo -e "   • Run AI Engine: ${CYAN}./build/hydraflow-x${NC}"
echo -e "   • Web Dashboard: ${CYAN}cd web-dashboard && npm run dev${NC}"
echo -e "   • View Logs: ${CYAN}tail -f logs/*.log${NC}"
echo ""
echo -e "${PURPLE}🤖 AI Features Enabled:${NC}"
echo -e "   • Multi-source sentiment analysis"
echo -e "   • LLM-driven trading decisions" 
echo -e "   • Autonomous research system"
echo -e "   • Real-time strategy adaptation"
echo ""
echo -e "${GREEN}✨ Ready for autonomous crypto trading!${NC}"
