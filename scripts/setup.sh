#!/bin/bash
# HydraFlow-X Setup Script
# Sets up the complete development and runtime environment

set -e  # Exit on any error

echo "🚀 Setting up HydraFlow-X Ultra-Low Latency DeFi HFT Engine"
echo "==============================================================="

# Detect platform
if [[ "$OSTYPE" == "darwin"* ]]; then
    PLATFORM="macos"
    echo "✅ Detected macOS platform"
elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
    PLATFORM="linux"
    echo "✅ Detected Linux platform"
else
    echo "❌ Unsupported platform: $OSTYPE"
    exit 1
fi

# Check if running on Apple Silicon
if [[ "$PLATFORM" == "macos" ]] && [[ $(uname -m) == "arm64" ]]; then
    APPLE_SILICON=true
    echo "🍎 Apple Silicon (ARM64) detected - enabling optimizations"
else
    APPLE_SILICON=false
fi

# Function to check if command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Function to install Homebrew on macOS
install_homebrew() {
    if ! command_exists brew; then
        echo "📦 Installing Homebrew..."
        /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
    else
        echo "✅ Homebrew already installed"
    fi
}

# Function to install system dependencies
install_system_dependencies() {
    echo "📦 Installing system dependencies..."
    
    if [[ "$PLATFORM" == "macos" ]]; then
        install_homebrew
        
        # Update Homebrew
        brew update
        
        # Install essential build tools
        brew install cmake
        brew install llvm
        brew install ninja
        brew install pkg-config
        
        # Install networking libraries
        brew install openssl
        brew install libev
        brew install libuv
        
        # Install compression libraries
        brew install zlib
        brew install zstd
        brew install lz4
        
        # Install Python
        brew install python@3.11
        
        # Install additional macOS-specific tools
        if [[ "$APPLE_SILICON" == true ]]; then
            echo "🍎 Installing Apple Silicon specific tools..."
            # Any Apple Silicon specific packages
        fi
        
    elif [[ "$PLATFORM" == "linux" ]]; then
        # Linux dependency installation
        if command_exists apt-get; then
            sudo apt-get update
            sudo apt-get install -y \
                cmake \
                ninja-build \
                clang-17 \
                libc++-dev \
                libc++abi-dev \
                pkg-config \
                libssl-dev \
                libev-dev \
                libuv1-dev \
                zlib1g-dev \
                libzstd-dev \
                liblz4-dev \
                python3.11 \
                python3.11-dev \
                python3-pip
        elif command_exists yum; then
            sudo yum install -y \
                cmake \
                ninja-build \
                clang \
                openssl-devel \
                libev-devel \
                libuv-devel \
                zlib-devel \
                libzstd-devel \
                lz4-devel \
                python3.11 \
                python3.11-devel
        fi
    fi
}

# Function to setup Python environment
setup_python_environment() {
    echo "🐍 Setting up Python environment..."
    
    # Create virtual environment
    if [[ "$PLATFORM" == "macos" ]]; then
        PYTHON_CMD="python3.11"
    else
        PYTHON_CMD="python3.11"
    fi
    
    if [[ ! -d "venv" ]]; then
        echo "Creating Python virtual environment..."
        $PYTHON_CMD -m venv venv
    fi
    
    # Activate virtual environment
    source venv/bin/activate
    
    # Upgrade pip
    pip install --upgrade pip
    
    # Install Python dependencies
    echo "📦 Installing Python quantitative finance libraries..."
    pip install -r requirements.txt
    
    # Install Apple Silicon optimizations if available
    if [[ "$APPLE_SILICON" == true ]]; then
        echo "🍎 Installing Apple Silicon Python optimizations..."
        # Install TensorFlow Metal plugin if available
        pip install tensorflow-metal || echo "⚠️  tensorflow-metal not available"
        # Install other Apple Silicon optimized packages
        pip install torch-audio --index-url https://download.pytorch.org/whl/cpu || true
    fi
    
    echo "✅ Python environment setup complete"
}

# Function to configure build environment
configure_build_environment() {
    echo "🔧 Configuring build environment..."
    
    # Create build directory
    mkdir -p build
    cd build
    
    # Configure CMake with optimal settings
    CMAKE_ARGS="-DCMAKE_BUILD_TYPE=Release"
    CMAKE_ARGS="$CMAKE_ARGS -DCMAKE_EXPORT_COMPILE_COMMANDS=ON"
    CMAKE_ARGS="$CMAKE_ARGS -DBUILD_TESTING=ON"
    
    if [[ "$PLATFORM" == "macos" ]]; then
        # macOS specific configuration
        CMAKE_ARGS="$CMAKE_ARGS -DCMAKE_OSX_DEPLOYMENT_TARGET=14.0"
        
        if [[ "$APPLE_SILICON" == true ]]; then
            CMAKE_ARGS="$CMAKE_ARGS -DCMAKE_OSX_ARCHITECTURES=arm64"
            CMAKE_ARGS="$CMAKE_ARGS -DCMAKE_CXX_FLAGS=-mcpu=apple-m3"
        fi
        
        # Use Homebrew LLVM
        CMAKE_ARGS="$CMAKE_ARGS -DCMAKE_C_COMPILER=$(brew --prefix llvm)/bin/clang"
        CMAKE_ARGS="$CMAKE_ARGS -DCMAKE_CXX_COMPILER=$(brew --prefix llvm)/bin/clang++"
    elif [[ "$PLATFORM" == "linux" ]]; then
        # Linux specific configuration
        CMAKE_ARGS="$CMAKE_ARGS -DCMAKE_C_COMPILER=clang-17"
        CMAKE_ARGS="$CMAKE_ARGS -DCMAKE_CXX_COMPILER=clang++-17"
    fi
    
    echo "🔧 Running CMake configuration..."
    cmake $CMAKE_ARGS -G Ninja ..
    
    cd ..
    echo "✅ Build environment configured"
}

# Function to build the project
build_project() {
    echo "🔨 Building HydraFlow-X..."
    
    cd build
    
    # Build with all available cores
    ninja -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
    
    cd ..
    echo "✅ Build complete"
}

# Function to run tests
run_tests() {
    echo "🧪 Running tests..."
    
    cd build
    ctest --output-on-failure --parallel $(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
    cd ..
    
    echo "✅ Tests complete"
}

# Function to setup configuration
setup_configuration() {
    echo "⚙️  Setting up configuration..."
    
    # Copy configuration files if they don't exist
    if [[ ! -f "configs/local_config.toml" ]]; then
        cp configs/main_config.toml configs/local_config.toml
        echo "📝 Created local configuration file: configs/local_config.toml"
        echo "🔧 Please edit this file with your API keys and endpoints"
    fi
    
    # Create directories for runtime data
    mkdir -p logs
    mkdir -p wal
    mkdir -p data
    
    echo "✅ Configuration setup complete"
}

# Function to optimize system settings (macOS)
optimize_system_settings() {
    if [[ "$PLATFORM" == "macos" ]]; then
        echo "🚀 Optimizing macOS system settings for HFT..."
        
        # Increase file descriptor limits
        echo "kern.maxfiles=65536" | sudo tee -a /etc/sysctl.conf >/dev/null || true
        echo "kern.maxfilesperproc=32768" | sudo tee -a /etc/sysctl.conf >/dev/null || true
        
        # Network optimizations
        echo "net.inet.tcp.sendspace=262144" | sudo tee -a /etc/sysctl.conf >/dev/null || true
        echo "net.inet.tcp.recvspace=262144" | sudo tee -a /etc/sysctl.conf >/dev/null || true
        
        echo "🔧 System optimizations applied (may require reboot)"
    fi
}

# Main execution
main() {
    # Change to project directory
    cd "$(dirname "$0")/.."
    
    echo "📁 Working directory: $(pwd)"
    
    # Run setup steps
    install_system_dependencies
    setup_python_environment
    configure_build_environment
    build_project
    run_tests
    setup_configuration
    optimize_system_settings
    
    echo ""
    echo "🎉 HydraFlow-X setup complete!"
    echo ""
    echo "🚀 Next steps:"
    echo "1. Edit configs/local_config.toml with your API keys"
    echo "2. Run: ./build/hydraflow-x"
    echo "3. Monitor logs in the ./logs directory"
    echo ""
    echo "📊 Performance monitoring:"
    echo "- Binary WAL files: ./wal/"
    echo "- Telemetry data: exported to configured endpoint"
    echo ""
    echo "🍎 Apple Silicon optimizations: $APPLE_SILICON"
    echo "💻 Platform: $PLATFORM"
    echo ""
}

# Run main function
main "$@"
