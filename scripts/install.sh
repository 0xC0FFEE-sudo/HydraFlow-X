#!/bin/bash

# HydraFlow-X Installation Script
# Supports Ubuntu 20.04+, Debian 11+, macOS, and CentOS/RHEL 8+

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
HYDRAFLOW_VERSION=${HYDRAFLOW_VERSION:-"latest"}
INSTALL_DIR=${INSTALL_DIR:-"/opt/hydraflow"}
USER_INSTALL=${USER_INSTALL:-false}
SKIP_DOCKER=${SKIP_DOCKER:-false}
DEVELOPMENT=${DEVELOPMENT:-false}

# Function to print colored output
print_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Function to detect OS
detect_os() {
    if [[ "$OSTYPE" == "linux-gnu"* ]]; then
        if [ -f /etc/os-release ]; then
            . /etc/os-release
            OS=$ID
            OS_VERSION=$VERSION_ID
        else
            print_error "Cannot detect Linux distribution"
            exit 1
        fi
    elif [[ "$OSTYPE" == "darwin"* ]]; then
        OS="macos"
        OS_VERSION=$(sw_vers -productVersion)
    else
        print_error "Unsupported operating system: $OSTYPE"
        exit 1
    fi
    
    print_info "Detected OS: $OS $OS_VERSION"
}

# Function to check if running as root (when needed)
check_root() {
    if [ "$EUID" -ne 0 ] && [ "$USER_INSTALL" != "true" ]; then
        print_error "This script needs to be run as root (use sudo) or set USER_INSTALL=true"
        exit 1
    fi
}

# Function to install dependencies on Ubuntu/Debian
install_deps_ubuntu() {
    print_info "Installing dependencies for Ubuntu/Debian..."
    
    apt-get update
    apt-get install -y \
        curl \
        wget \
        git \
        build-essential \
        cmake \
        pkg-config \
        libssl-dev \
        libpq-dev \
        libcurl4-openssl-dev \
        libnlohmann-json3-dev \
        libboost-all-dev \
        ninja-build \
        clang-15 \
        libc++-15-dev \
        libc++abi-15-dev \
        postgresql-client \
        redis-tools \
        jq \
        htop \
        net-tools
    
    # Install Node.js 18
    curl -fsSL https://deb.nodesource.com/setup_18.x | bash -
    apt-get install -y nodejs
    
    print_success "Dependencies installed successfully"
}

# Function to install dependencies on CentOS/RHEL
install_deps_centos() {
    print_info "Installing dependencies for CentOS/RHEL..."
    
    # Enable EPEL repository
    dnf install -y epel-release
    dnf config-manager --set-enabled powertools || dnf config-manager --set-enabled crb
    
    dnf install -y \
        curl \
        wget \
        git \
        gcc-c++ \
        make \
        cmake \
        pkgconfig \
        openssl-devel \
        postgresql-devel \
        libcurl-devel \
        boost-devel \
        ninja-build \
        clang \
        postgresql \
        redis \
        jq \
        htop \
        net-tools
    
    # Install Node.js 18
    curl -fsSL https://rpm.nodesource.com/setup_18.x | bash -
    dnf install -y nodejs
    
    print_success "Dependencies installed successfully"
}

# Function to install dependencies on macOS
install_deps_macos() {
    print_info "Installing dependencies for macOS..."
    
    # Check if Homebrew is installed
    if ! command -v brew &> /dev/null; then
        print_info "Installing Homebrew..."
        /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
    fi
    
    # Update Homebrew
    brew update
    
    # Install dependencies
    brew install \
        cmake \
        pkg-config \
        openssl \
        postgresql \
        curl \
        nlohmann-json \
        boost \
        ninja \
        llvm \
        redis \
        jq \
        htop \
        node@18
    
    # Link Node.js 18
    brew link node@18 --force
    
    print_success "Dependencies installed successfully"
}

# Function to install Docker
install_docker() {
    if [ "$SKIP_DOCKER" = "true" ]; then
        print_info "Skipping Docker installation"
        return
    fi
    
    print_info "Installing Docker..."
    
    case $OS in
        ubuntu|debian)
            # Remove old versions
            apt-get remove -y docker docker-engine docker.io containerd runc || true
            
            # Install Docker
            curl -fsSL https://get.docker.com -o get-docker.sh
            sh get-docker.sh
            rm get-docker.sh
            
            # Install Docker Compose
            curl -L "https://github.com/docker/compose/releases/latest/download/docker-compose-$(uname -s)-$(uname -m)" -o /usr/local/bin/docker-compose
            chmod +x /usr/local/bin/docker-compose
            
            # Add user to docker group if not root
            if [ "$USER_INSTALL" = "true" ]; then
                usermod -aG docker $SUDO_USER || usermod -aG docker $USER
            fi
            ;;
        centos|rhel|fedora)
            dnf config-manager --add-repo https://download.docker.com/linux/centos/docker-ce.repo
            dnf install -y docker-ce docker-ce-cli containerd.io docker-compose-plugin
            systemctl enable --now docker
            
            # Add user to docker group if not root
            if [ "$USER_INSTALL" = "true" ]; then
                usermod -aG docker $SUDO_USER || usermod -aG docker $USER
            fi
            ;;
        macos)
            if ! command -v docker &> /dev/null; then
                print_info "Please install Docker Desktop for Mac from https://www.docker.com/products/docker-desktop"
                print_warning "Docker Desktop installation requires manual intervention"
            else
                print_success "Docker already installed"
            fi
            ;;
    esac
    
    print_success "Docker installation completed"
}

# Function to create directory structure
create_directories() {
    print_info "Creating directory structure..."
    
    if [ "$USER_INSTALL" = "true" ]; then
        INSTALL_DIR="$HOME/hydraflow"
    fi
    
    mkdir -p "$INSTALL_DIR"/{bin,config,logs,data,backups,scripts}
    
    if [ "$USER_INSTALL" != "true" ]; then
        chown -R $SUDO_USER:$SUDO_USER "$INSTALL_DIR" 2>/dev/null || true
    fi
    
    print_success "Directories created at $INSTALL_DIR"
}

# Function to download and build HydraFlow-X
install_hydraflow() {
    print_info "Downloading and building HydraFlow-X..."
    
    local temp_dir=$(mktemp -d)
    cd "$temp_dir"
    
    # Clone repository
    if [ "$HYDRAFLOW_VERSION" = "latest" ]; then
        git clone https://github.com/yourusername/HydraFlow-X.git
    else
        git clone --branch "$HYDRAFLOW_VERSION" https://github.com/yourusername/HydraFlow-X.git
    fi
    
    cd HydraFlow-X
    
    # Build the project
    mkdir build && cd build
    
    if [ "$DEVELOPMENT" = "true" ]; then
        cmake -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_STANDARD=23 ..
    else
        cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_STANDARD=23 ..
    fi
    
    ninja -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
    
    # Install binary
    cp hydraflow-x "$INSTALL_DIR/bin/"
    chmod +x "$INSTALL_DIR/bin/hydraflow-x"
    
    # Copy configuration templates
    cp -r ../config/* "$INSTALL_DIR/config/" 2>/dev/null || true
    cp ../config.env.example "$INSTALL_DIR/config/"
    
    # Copy scripts
    cp -r ../scripts/* "$INSTALL_DIR/scripts/" 2>/dev/null || true
    chmod +x "$INSTALL_DIR/scripts"/*.sh 2>/dev/null || true
    
    # Cleanup
    cd /
    rm -rf "$temp_dir"
    
    print_success "HydraFlow-X installed successfully"
}

# Function to create systemd service
create_systemd_service() {
    if [ "$USER_INSTALL" = "true" ]; then
        print_info "Skipping systemd service creation for user installation"
        return
    fi
    
    print_info "Creating systemd service..."
    
    cat > /etc/systemd/system/hydraflow-x.service << EOF
[Unit]
Description=HydraFlow-X Ultra-Low Latency Trading Platform
After=network.target postgresql.service redis.service
Wants=postgresql.service redis.service

[Service]
Type=simple
User=hydraflow
Group=hydraflow
WorkingDirectory=$INSTALL_DIR
ExecStart=$INSTALL_DIR/bin/hydraflow-x --config $INSTALL_DIR/config/hydraflow.json
Restart=always
RestartSec=10
StandardOutput=journal
StandardError=journal
SyslogIdentifier=hydraflow-x

# Security settings
NoNewPrivileges=true
PrivateTmp=true
ProtectSystem=strict
ProtectHome=true
ReadWritePaths=$INSTALL_DIR/logs $INSTALL_DIR/data $INSTALL_DIR/backups
CapabilityBoundingSet=CAP_NET_BIND_SERVICE

# Performance settings
LimitNOFILE=65536
LimitNPROC=32768

# Environment
Environment=HFX_CONFIG_PATH=$INSTALL_DIR/config
Environment=HFX_LOG_PATH=$INSTALL_DIR/logs
Environment=HFX_DATA_PATH=$INSTALL_DIR/data

[Install]
WantedBy=multi-user.target
EOF
    
    # Create hydraflow user if it doesn't exist
    if ! id "hydraflow" &>/dev/null; then
        useradd -r -s /bin/false -d "$INSTALL_DIR" hydraflow
        chown -R hydraflow:hydraflow "$INSTALL_DIR"
    fi
    
    systemctl daemon-reload
    systemctl enable hydraflow-x
    
    print_success "Systemd service created and enabled"
}

# Function to create configuration
create_config() {
    print_info "Creating configuration files..."
    
    local config_file="$INSTALL_DIR/config/hydraflow.json"
    local env_file="$INSTALL_DIR/config/.env"
    
    # Copy example configuration if it doesn't exist
    if [ ! -f "$config_file" ]; then
        cp "$INSTALL_DIR/config/hydraflow.example.json" "$config_file"
        print_info "Created configuration file: $config_file"
    fi
    
    # Create environment file if it doesn't exist
    if [ ! -f "$env_file" ]; then
        cp "$INSTALL_DIR/config/config.env.example" "$env_file"
        chmod 600 "$env_file"
        print_info "Created environment file: $env_file"
    fi
    
    print_warning "Please edit $config_file and $env_file with your API keys and configuration"
}

# Function to display post-installation information
show_post_install_info() {
    print_success "🎉 HydraFlow-X installation completed successfully!"
    echo ""
    print_info "Installation directory: $INSTALL_DIR"
    print_info "Configuration files: $INSTALL_DIR/config/"
    print_info "Log files: $INSTALL_DIR/logs/"
    echo ""
    print_info "Next steps:"
    echo "  1. Edit configuration: $INSTALL_DIR/config/hydraflow.json"
    echo "  2. Set API keys: $INSTALL_DIR/config/.env"
    echo "  3. Start the service:"
    
    if [ "$USER_INSTALL" = "true" ]; then
        echo "     $INSTALL_DIR/bin/hydraflow-x --config $INSTALL_DIR/config/hydraflow.json"
    else
        echo "     sudo systemctl start hydraflow-x"
        echo "     sudo systemctl status hydraflow-x"
    fi
    
    echo ""
    print_info "Web Dashboard: http://localhost:8080"
    print_info "Metrics: http://localhost:9090"
    echo ""
    
    if [ "$SKIP_DOCKER" != "true" ]; then
        print_info "Docker Compose alternative:"
        echo "  cd $INSTALL_DIR && docker-compose up -d"
    fi
    
    echo ""
    print_warning "⚠️  Remember to:"
    echo "  - Configure your API keys before starting"
    echo "  - Set up your database (PostgreSQL)"
    echo "  - Configure firewall rules if needed"
    echo "  - Review security settings"
}

# Main installation function
main() {
    echo "🚀 HydraFlow-X Installation Script"
    echo "=================================="
    echo ""
    
    # Parse command line arguments
    while [[ $# -gt 0 ]]; do
        case $1 in
            --user-install)
                USER_INSTALL=true
                shift
                ;;
            --skip-docker)
                SKIP_DOCKER=true
                shift
                ;;
            --development)
                DEVELOPMENT=true
                shift
                ;;
            --install-dir)
                INSTALL_DIR="$2"
                shift 2
                ;;
            --version)
                HYDRAFLOW_VERSION="$2"
                shift 2
                ;;
            --help)
                echo "Usage: $0 [OPTIONS]"
                echo ""
                echo "Options:"
                echo "  --user-install     Install in user home directory"
                echo "  --skip-docker      Skip Docker installation"
                echo "  --development      Install development version"
                echo "  --install-dir DIR  Custom installation directory"
                echo "  --version VERSION  Install specific version"
                echo "  --help             Show this help message"
                exit 0
                ;;
            *)
                print_error "Unknown option: $1"
                exit 1
                ;;
        esac
    done
    
    # Detect operating system
    detect_os
    
    # Check root privileges if needed
    check_root
    
    # Install dependencies
    case $OS in
        ubuntu|debian)
            install_deps_ubuntu
            ;;
        centos|rhel|fedora)
            install_deps_centos
            ;;
        macos)
            install_deps_macos
            ;;
        *)
            print_error "Unsupported OS: $OS"
            exit 1
            ;;
    esac
    
    # Install Docker
    install_docker
    
    # Create directories
    create_directories
    
    # Install HydraFlow-X
    install_hydraflow
    
    # Create systemd service (Linux only)
    if [[ "$OS" != "macos" ]]; then
        create_systemd_service
    fi
    
    # Create configuration
    create_config
    
    # Show post-installation information
    show_post_install_info
}

# Run main function
main "$@"
