#!/bin/bash

# HydraFlow-X Development Build Script
# This script builds the project in development mode with debugging support

set -e

echo "🚀 HydraFlow-X Development Build Starting..."

# Change to app directory
cd /app

# Clean previous build if it exists
if [ -d "build" ]; then
    echo "🧹 Cleaning previous build..."
    rm -rf build
fi

# Create build directory
mkdir -p build
cd build

# Configure CMake for development
echo "⚙️  Configuring CMake for development..."
cmake -G Ninja \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_CXX_STANDARD=23 \
    -DCMAKE_CXX_FLAGS="-g -O0 -DDEBUG -fsanitize=address -fno-omit-frame-pointer" \
    -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address" \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
    ..

# Build the project
echo "🔨 Building HydraFlow-X..."
ninja -j$(nproc)

# Copy configuration if it doesn't exist
if [ ! -f "/opt/hydraflow/config/hydraflow.json" ]; then
    echo "📋 Setting up development configuration..."
    cp /opt/hydraflow/config/hydraflow.example.json /opt/hydraflow/config/hydraflow.json
fi

# Create development environment file if it doesn't exist
if [ ! -f "/opt/hydraflow/config/.env" ]; then
    echo "🔧 Creating development environment file..."
    cat > /opt/hydraflow/config/.env << EOF
# Development Environment Configuration
HFX_ENVIRONMENT=development
HFX_DEBUG_MODE=true
HFX_LOG_LEVEL=DEBUG
HFX_WEB_PORT=8080
HFX_PROMETHEUS_PORT=9090

# Database Configuration (Development)
HFX_DB_HOST=postgres-dev
HFX_DB_PORT=5432
HFX_DB_NAME=hydraflow_dev
HFX_DB_USER=hydraflow
HFX_DB_PASSWORD=dev_password

# Redis Configuration (Development)
HFX_REDIS_HOST=redis-dev
HFX_REDIS_PORT=6379

# API Configuration (Leave empty for development)
HFX_TWITTER_API_KEY=
HFX_REDDIT_API_KEY=
HFX_DEXSCREENER_API_KEY=
HFX_ETHEREUM_RPC_API_KEY=
HFX_SOLANA_RPC_API_KEY=

# Trading Configuration (Safe defaults for development)
HFX_MAX_POSITION_SIZE=100.0
HFX_PAPER_TRADING=true
HFX_TEST_MODE=true
EOF
    chmod 600 /opt/hydraflow/config/.env
fi

echo "✅ Build completed successfully!"

# Start the application in development mode
echo "🎯 Starting HydraFlow-X in development mode..."
echo "   - Web Dashboard: http://localhost:8080"
echo "   - Metrics: http://localhost:9090"
echo "   - Debug Port: 2345"
echo ""

# Set up signal handlers for graceful shutdown
trap 'echo "🛑 Shutting down HydraFlow-X..."; kill $APP_PID 2>/dev/null || true; exit 0' SIGTERM SIGINT

# Run the application
cd /app/build
./hydraflow-x --config /opt/hydraflow/config/hydraflow.json &
APP_PID=$!

# Wait for the application to exit
wait $APP_PID
