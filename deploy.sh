#!/bin/bash

# HydraFlow-X Production Deployment Script
# Usage: ./deploy.sh [mode]
# Modes: build, install, run, stop

set -e

# Configuration
BUILD_DIR="build"
INSTALL_DIR="/opt/hydraflow-x"
CONFIG_DIR="/etc/hydraflow"
LOG_DIR="/var/log/hydraflow"
SERVICE_USER="hydraflow"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

function print_banner() {
    echo -e "${BLUE}"
    echo "    ╭─────────────────────────────────────────────────────╮"
    echo "    │                                                     │"
    echo "    │         🏛️  HydraFlow-X Deployment Manager          │"
    echo "    │                                                     │"
    echo "    │    Production-Ready High Frequency Trading         │"
    echo "    │                                                     │"
    echo "    ╰─────────────────────────────────────────────────────╯"
    echo -e "${NC}"
}

function log() {
    echo -e "${GREEN}[$(date '+%Y-%m-%d %H:%M:%S')] $1${NC}"
}

function warn() {
    echo -e "${YELLOW}[WARNING] $1${NC}"
}

function error() {
    echo -e "${RED}[ERROR] $1${NC}"
    exit 1
}

function check_dependencies() {
    log "Checking system dependencies..."
    
    # Check for required packages
    local deps=("cmake" "g++" "make" "pkg-config" "libssl-dev" "libcurl4-openssl-dev")
    local missing_deps=()
    
    for dep in "${deps[@]}"; do
        if ! dpkg -l | grep -q "$dep" 2>/dev/null && ! command -v "$dep" &> /dev/null; then
            missing_deps+=("$dep")
        fi
    done
    
    if [ ${#missing_deps[@]} -ne 0 ]; then
        warn "Missing dependencies: ${missing_deps[*]}"
        log "Installing missing dependencies..."
        sudo apt-get update
        sudo apt-get install -y "${missing_deps[@]}"
    fi
    
    log "✅ All dependencies satisfied"
}

function build_system() {
    log "Building HydraFlow-X production system..."
    
    # Clean previous build
    if [ -d "$BUILD_DIR" ]; then
        rm -rf "$BUILD_DIR"
    fi
    
    # Create build directory
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"
    
    # Configure build
    cmake -DCMAKE_BUILD_TYPE=Release \
          -DBUILD_CORE_BACKEND=ON \
          -DBUILD_WEB_DASHBOARD=OFF \
          -DBUILD_TESTS=OFF \
          ..
    
    # Build with all available cores
    make -j$(nproc)
    
    # Verify executable
    if [ ! -f "hydraflow-x" ]; then
        error "Build failed - executable not found"
    fi
    
    log "✅ Build completed successfully"
    cd ..
}

function install_system() {
    log "Installing HydraFlow-X to production environment..."
    
    # Create system user if not exists
    if ! id "$SERVICE_USER" &>/dev/null; then
        log "Creating system user: $SERVICE_USER"
        sudo useradd -r -s /bin/false -d "$INSTALL_DIR" "$SERVICE_USER"
    fi
    
    # Create directories
    sudo mkdir -p "$INSTALL_DIR" "$CONFIG_DIR" "$LOG_DIR"
    sudo chown "$SERVICE_USER:$SERVICE_USER" "$INSTALL_DIR" "$LOG_DIR"
    sudo chmod 755 "$CONFIG_DIR"
    
    # Install executable
    sudo cp "$BUILD_DIR/hydraflow-x" "$INSTALL_DIR/"
    sudo chown "$SERVICE_USER:$SERVICE_USER" "$INSTALL_DIR/hydraflow-x"
    sudo chmod 755 "$INSTALL_DIR/hydraflow-x"
    
    # Create configuration template
    cat > /tmp/trading.conf << EOF
# HydraFlow-X Configuration
# Copy this file to $CONFIG_DIR/trading.conf and configure your API keys

# Coinbase Pro API Configuration
coinbase_api_key=your_api_key_here
coinbase_api_secret=your_api_secret_here
coinbase_passphrase=your_passphrase_here

# Trading Configuration
sandbox_mode=true
max_position_size=0.1
stop_loss_pct=0.02
take_profit_pct=0.04

# Trading Pairs
trading_pairs=BTC-USD,ETH-USD,ADA-USD
EOF
    
    sudo mv /tmp/trading.conf "$CONFIG_DIR/trading.conf.example"
    sudo chown root:root "$CONFIG_DIR/trading.conf.example"
    
    # Create systemd service
    cat > /tmp/hydraflow-x.service << EOF
[Unit]
Description=HydraFlow-X High Frequency Trading Engine
Documentation=https://github.com/your-repo/hydraflow-x
After=network.target

[Service]
Type=simple
User=$SERVICE_USER
Group=$SERVICE_USER
WorkingDirectory=$INSTALL_DIR
ExecStart=$INSTALL_DIR/hydraflow-x
Restart=always
RestartSec=10
StandardOutput=journal
StandardError=journal
SyslogIdentifier=hydraflow-x

# Security settings
NoNewPrivileges=true
ProtectSystem=strict
ProtectHome=true
ReadWritePaths=$LOG_DIR
PrivateTmp=true
ProtectKernelTunables=true
ProtectControlGroups=true
RestrictRealtime=true

# Resource limits
LimitNOFILE=65536
LimitNPROC=32768

[Install]
WantedBy=multi-user.target
EOF
    
    sudo mv /tmp/hydraflow-x.service /etc/systemd/system/
    sudo systemctl daemon-reload
    
    log "✅ Installation completed"
    log "📝 Next steps:"
    log "   1. Configure API keys in: $CONFIG_DIR/trading.conf"
    log "   2. Start service: sudo systemctl start hydraflow-x"
    log "   3. Enable auto-start: sudo systemctl enable hydraflow-x"
}

function start_service() {
    log "Starting HydraFlow-X service..."
    
    if [ ! -f "$CONFIG_DIR/trading.conf" ]; then
        warn "Configuration file not found: $CONFIG_DIR/trading.conf"
        warn "Please copy and configure: $CONFIG_DIR/trading.conf.example"
        return 1
    fi
    
    sudo systemctl start hydraflow-x
    sleep 2
    sudo systemctl status hydraflow-x
    
    log "✅ HydraFlow-X is now running!"
    log "📊 Monitor logs: sudo journalctl -u hydraflow-x -f"
}

function stop_service() {
    log "Stopping HydraFlow-X service..."
    sudo systemctl stop hydraflow-x
    log "✅ HydraFlow-X stopped"
}

function show_status() {
    log "HydraFlow-X System Status:"
    echo ""
    
    if systemctl is-active --quiet hydraflow-x; then
        echo -e "🟢 Service Status: ${GREEN}RUNNING${NC}"
    else
        echo -e "🔴 Service Status: ${RED}STOPPED${NC}"
    fi
    
    if [ -f "$CONFIG_DIR/trading.conf" ]; then
        echo -e "🟢 Configuration: ${GREEN}CONFIGURED${NC}"
    else
        echo -e "🟡 Configuration: ${YELLOW}NEEDS SETUP${NC}"
    fi
    
    if [ -f "$INSTALL_DIR/hydraflow-x" ]; then
        echo -e "🟢 Installation: ${GREEN}COMPLETE${NC}"
    else
        echo -e "🔴 Installation: ${RED}NOT INSTALLED${NC}"
    fi
}

# Main script logic
print_banner

case "${1:-help}" in
    "build")
        check_dependencies
        build_system
        ;;
    "install")
        if [ ! -f "$BUILD_DIR/hydraflow-x" ]; then
            error "Build not found. Run './deploy.sh build' first."
        fi
        install_system
        ;;
    "run"|"start")
        start_service
        ;;
    "stop")
        stop_service
        ;;
    "status")
        show_status
        ;;
    "full-deploy")
        check_dependencies
        build_system
        install_system
        log "🚀 Full deployment completed!"
        log "Configure API keys and run: sudo systemctl start hydraflow-x"
        ;;
    "help"|*)
        echo "HydraFlow-X Deployment Manager"
        echo ""
        echo "Usage: $0 [command]"
        echo ""
        echo "Commands:"
        echo "  build        - Build the trading system"
        echo "  install      - Install to production environment"
        echo "  start|run    - Start the trading service"
        echo "  stop         - Stop the trading service"
        echo "  status       - Show system status"
        echo "  full-deploy  - Complete build and install"
        echo "  help         - Show this help message"
        echo ""
        ;;
esac
