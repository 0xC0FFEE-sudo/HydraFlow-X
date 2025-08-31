#!/bin/bash

# HydraFlow-X Quick Installation Script
# One-liner installer for quick setup

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

print_info() { echo -e "${BLUE}[INFO]${NC} $1"; }
print_success() { echo -e "${GREEN}[SUCCESS]${NC} $1"; }
print_warning() { echo -e "${YELLOW}[WARNING]${NC} $1"; }
print_error() { echo -e "${RED}[ERROR]${NC} $1"; }

echo "🚀 HydraFlow-X Quick Installer"
echo "============================="
echo ""

# Check if Docker is available
if command -v docker &> /dev/null && command -v docker-compose &> /dev/null; then
    print_info "Docker detected - using containerized installation"
    
    # Create temporary directory
    TEMP_DIR=$(mktemp -d)
    cd "$TEMP_DIR"
    
    # Download docker-compose.yml
    print_info "Downloading Docker Compose configuration..."
    curl -sSL https://raw.githubusercontent.com/yourusername/HydraFlow-X/main/docker-compose.yml -o docker-compose.yml
    
    # Create basic .env file
    cat > .env << EOF
# Quick Install Configuration
HFX_DB_PASSWORD=changeme123
HFX_CLICKHOUSE_PASSWORD=changeme123
GRAFANA_USER=admin
GRAFANA_PASSWORD=changeme123
EOF
    
    # Create config directory
    mkdir -p config
    
    # Download example config
    curl -sSL https://raw.githubusercontent.com/yourusername/HydraFlow-X/main/config/hydraflow.example.json -o config/hydraflow.json
    curl -sSL https://raw.githubusercontent.com/yourusername/HydraFlow-X/main/config.env.example -o config/.env
    
    print_info "Starting HydraFlow-X with Docker..."
    docker-compose up -d
    
    print_success "✅ HydraFlow-X is running!"
    echo ""
    print_info "🌐 Web Dashboard: http://localhost:8080"
    print_info "📊 Grafana Dashboard: http://localhost:3000 (admin/changeme123)"
    print_info "📈 Metrics: http://localhost:9091"
    echo ""
    print_warning "⚠️  Quick install uses default passwords. Please change them!"
    print_info "Configuration files are in: $TEMP_DIR/config/"
    echo ""
    print_info "To stop: cd $TEMP_DIR && docker-compose down"
    print_info "To view logs: cd $TEMP_DIR && docker-compose logs -f"
    
    # Save location for user
    echo "$TEMP_DIR" > ~/.hydraflow-quick-install-path
    
else
    print_info "Docker not detected - using native installation"
    
    # Download and run full installer
    curl -sSL https://raw.githubusercontent.com/yourusername/HydraFlow-X/main/scripts/install.sh | bash -s -- --user-install
fi

echo ""
print_success "🎉 HydraFlow-X Quick Installation Complete!"
print_info "Visit https://github.com/yourusername/HydraFlow-X for documentation"
