#!/bin/bash

# HydraFlow-X Universal Build Script
# Supports both Apple Silicon (ARM64) and Intel (x86_64) Macs
# Auto-detects architecture and applies optimal configurations

set -e

echo "🚀 HydraFlow-X Universal Build System"
echo "======================================"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Detect current architecture
CURRENT_ARCH=$(uname -m)
HOST_OS=$(uname -s)

echo -e "${CYAN}🔍 System Detection:${NC}"
echo -e "   OS: ${HOST_OS}"
echo -e "   Architecture: ${CURRENT_ARCH}"

if [[ "$HOST_OS" != "Darwin" ]]; then
    echo -e "${RED}❌ Error: This script is designed for macOS only${NC}"
    exit 1
fi

# Architecture detection and setup
case $CURRENT_ARCH in
    "arm64")
        DETECTED_ARCH="Apple Silicon (M1/M2/M3)"
        CMAKE_ARCH="arm64"
        OPTIMIZATION_FLAGS="-mcpu=apple-m1 -mtune=apple-m1"
        VECTORIZATION="-fvectorize"
        ;;
    "x86_64")
        DETECTED_ARCH="Intel x86_64"
        CMAKE_ARCH="x86_64"
        OPTIMIZATION_FLAGS="-march=native -mtune=native"
        VECTORIZATION="-mavx2 -mfma"
        ;;
    *)
        echo -e "${RED}❌ Unsupported architecture: $CURRENT_ARCH${NC}"
        exit 1
        ;;
esac

echo -e "   Detected: ${GREEN}${DETECTED_ARCH}${NC}"
echo ""

# Build mode selection
echo -e "${YELLOW}📋 Build Configuration Options:${NC}"
echo "1) Quick Build (Current Architecture: $CMAKE_ARCH)"
echo "2) Universal Binary (Both ARM64 + x86_64) [Recommended]"
echo "3) Apple Silicon Only (ARM64)"
echo "4) Intel Only (x86_64)"
echo "5) Debug Build (Current Architecture)"
echo ""

read -p "🎯 Select build option [1-5]: " BUILD_CHOICE

case $BUILD_CHOICE in
    1)
        BUILD_TYPE="QUICK"
        TARGET_ARCHS=("$CMAKE_ARCH")
        ;;
    2)
        BUILD_TYPE="UNIVERSAL"
        TARGET_ARCHS=("arm64" "x86_64")
        ;;
    3)
        BUILD_TYPE="ARM64_ONLY"
        TARGET_ARCHS=("arm64")
        ;;
    4)
        BUILD_TYPE="X86_64_ONLY"
        TARGET_ARCHS=("x86_64")
        ;;
    5)
        BUILD_TYPE="DEBUG"
        TARGET_ARCHS=("$CMAKE_ARCH")
        ;;
    *)
        echo -e "${RED}❌ Invalid option. Using Quick Build.${NC}"
        BUILD_TYPE="QUICK"
        TARGET_ARCHS=("$CMAKE_ARCH")
        ;;
esac

echo ""
echo -e "${BLUE}🔧 Build Configuration:${NC}"
echo -e "   Type: ${BUILD_TYPE}"
echo -e "   Target Architectures: ${TARGET_ARCHS[*]}"
echo ""

# Project root
SCRIPT_DIR=$(dirname "$(realpath "$0")")
PROJECT_ROOT=$(dirname "$SCRIPT_DIR")
cd "$PROJECT_ROOT"

# Cleanup function
cleanup_build_dirs() {
    echo -e "${YELLOW}🧹 Cleaning previous builds...${NC}"
    rm -rf build-arm64 build-x86_64 build-universal build
    mkdir -p logs
}

# Install dependencies if needed
install_dependencies() {
    echo -e "${BLUE}📦 Checking dependencies...${NC}"
    
    if ! command -v brew &> /dev/null; then
        echo -e "${RED}❌ Homebrew not found. Installing...${NC}"
        /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
        eval "$(/opt/homebrew/bin/brew shellenv)"
    fi
    
    # Architecture-specific Homebrew path
    if [[ "$CURRENT_ARCH" == "arm64" ]]; then
        BREW_PREFIX="/opt/homebrew"
    else
        BREW_PREFIX="/usr/local"
    fi
    
    export PATH="${BREW_PREFIX}/bin:$PATH"
    
    echo -e "${GREEN}✅ Using Homebrew at: ${BREW_PREFIX}${NC}"
    
    # Update and install dependencies
    brew update > /dev/null 2>&1
    
    # Core build tools
    DEPS=(
        "cmake"
        "ninja"
        "pkg-config"
        "llvm"
        "python@3.11"
    )
    
    # Visualization dependencies
    VIZ_DEPS=(
        "glfw"
        "glew"
        "freetype"
        "libpng"
        "jpeg-turbo"
    )
    
    # Network and performance libraries
    PERF_DEPS=(
        "boost"
        "openssl@3"
        "zlib"
        "zstd"
        "lz4"
        "libuv"
        "libev"
    )
    
    ALL_DEPS=("${DEPS[@]}" "${VIZ_DEPS[@]}" "${PERF_DEPS[@]}")
    
    for dep in "${ALL_DEPS[@]}"; do
        if ! brew list "$dep" &> /dev/null; then
            echo -e "   Installing ${dep}..."
            brew install "$dep" > /dev/null 2>&1
        else
            echo -e "   ✅ ${dep}"
        fi
    done
}

# Architecture-specific build function
build_for_arch() {
    local arch=$1
    local build_dir="build-${arch}"
    
    echo -e "${PURPLE}🏗️  Building for ${arch}...${NC}"
    
    # Create build directory
    mkdir -p "$build_dir"
    cd "$build_dir"
    
    # Architecture-specific compiler settings
    case $arch in
        "arm64")
            if [[ "$CURRENT_ARCH" == "x86_64" ]]; then
                # Cross-compilation from Intel to ARM64
                export CC="clang -target arm64-apple-macos11"
                export CXX="clang++ -target arm64-apple-macos11"
                CROSS_COMPILE_FLAGS="-target arm64-apple-macos11"
            else
                # Native ARM64 compilation
                export CC="clang"
                export CXX="clang++"
                CROSS_COMPILE_FLAGS=""
            fi
            ARCH_OPTIMIZATION="-mcpu=apple-m1 -mtune=apple-m1"
            VECTORIZATION="-fvectorize"
            ;;
        "x86_64")
            if [[ "$CURRENT_ARCH" == "arm64" ]]; then
                # Cross-compilation from ARM64 to Intel
                export CC="clang -target x86_64-apple-macos10.15"
                export CXX="clang++ -target x86_64-apple-macos10.15"
                CROSS_COMPILE_FLAGS="-target x86_64-apple-macos10.15"
            else
                # Native x86_64 compilation
                export CC="clang"
                export CXX="clang++"
                CROSS_COMPILE_FLAGS=""
            fi
            ARCH_OPTIMIZATION="-march=native -mtune=native"
            VECTORIZATION="-mavx2 -mfma -msse4.2"
            ;;
    esac
    
    # Build type configuration
    if [[ "$BUILD_TYPE" == "DEBUG" ]]; then
        CMAKE_BUILD_TYPE="Debug"
        OPTIMIZATION_LEVEL="-O0 -g"
        EXTRA_FLAGS="-DDEBUG=1 -fno-omit-frame-pointer"
    else
        CMAKE_BUILD_TYPE="Release"
        OPTIMIZATION_LEVEL="-O3 -DNDEBUG"
        EXTRA_FLAGS="-flto -fomit-frame-pointer"
    fi
    
    # Configure with CMake
    echo -e "   🔧 Configuring CMake for ${arch}..."
    cmake .. \
        -G Ninja \
        -DCMAKE_BUILD_TYPE="$CMAKE_BUILD_TYPE" \
        -DCMAKE_OSX_ARCHITECTURES="$arch" \
        -DCMAKE_C_COMPILER="$CC" \
        -DCMAKE_CXX_COMPILER="$CXX" \
        -DCMAKE_C_FLAGS="$CROSS_COMPILE_FLAGS $OPTIMIZATION_LEVEL $ARCH_OPTIMIZATION $EXTRA_FLAGS" \
        -DCMAKE_CXX_FLAGS="$CROSS_COMPILE_FLAGS $OPTIMIZATION_LEVEL $ARCH_OPTIMIZATION $VECTORIZATION $EXTRA_FLAGS" \
        -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON \
        -DHFX_TARGET_ARCH="$arch" \
        -DHFX_ENABLE_VISUALIZATION=ON \
        -DHFX_ENABLE_WEBSOCKET_SERVER=ON \
        -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
        > "../logs/cmake-${arch}.log" 2>&1
    
    if [ $? -ne 0 ]; then
        echo -e "${RED}❌ CMake configuration failed for ${arch}${NC}"
        echo -e "   Check logs/cmake-${arch}.log for details"
        return 1
    fi
    
    # Build
    echo -e "   🔨 Compiling for ${arch}..."
    ninja -j$(sysctl -n hw.ncpu) > "../logs/build-${arch}.log" 2>&1
    
    if [ $? -ne 0 ]; then
        echo -e "${RED}❌ Build failed for ${arch}${NC}"
        echo -e "   Check logs/build-${arch}.log for details"
        return 1
    fi
    
    echo -e "${GREEN}✅ Successfully built for ${arch}${NC}"
    
    # Verify binary
    if [[ -f "hydraflow-x" ]]; then
        local binary_arch=$(file hydraflow-x | grep -o "arm64\|x86_64")
        echo -e "   📦 Binary architecture: ${binary_arch}"
        
        # Copy to architecture-specific location
        cp hydraflow-x "../hydraflow-x-${arch}"
    fi
    
    cd "$PROJECT_ROOT"
    return 0
}

# Create universal binary
create_universal_binary() {
    echo -e "${CYAN}🔗 Creating Universal Binary...${NC}"
    
    if [[ -f "hydraflow-x-arm64" && -f "hydraflow-x-x86_64" ]]; then
        lipo -create -output hydraflow-x-universal hydraflow-x-arm64 hydraflow-x-x86_64
        
        if [ $? -eq 0 ]; then
            echo -e "${GREEN}✅ Universal binary created successfully${NC}"
            echo -e "   📦 File: hydraflow-x-universal"
            
            # Verify universal binary
            echo -e "${BLUE}🔍 Binary verification:${NC}"
            file hydraflow-x-universal
            lipo -info hydraflow-x-universal
        else
            echo -e "${RED}❌ Failed to create universal binary${NC}"
            return 1
        fi
    else
        echo -e "${RED}❌ Missing architecture-specific binaries${NC}"
        return 1
    fi
}

# Setup Python environment
setup_python_env() {
    echo -e "${BLUE}🐍 Setting up Python environment...${NC}"
    
    # Use architecture-appropriate Python
    if [[ "$CURRENT_ARCH" == "arm64" ]]; then
        PYTHON_PATH="/opt/homebrew/bin/python3.11"
    else
        PYTHON_PATH="/usr/local/bin/python3.11"
    fi
    
    if [[ ! -f "$PYTHON_PATH" ]]; then
        PYTHON_PATH="python3"
    fi
    
    # Create virtual environment
    if [[ ! -d "venv" ]]; then
        echo -e "   Creating virtual environment..."
        $PYTHON_PATH -m venv venv
    fi
    
    # Activate and install dependencies
    source venv/bin/activate
    pip install --upgrade pip > /dev/null 2>&1
    pip install -r requirements.txt > /dev/null 2>&1
    
    echo -e "${GREEN}✅ Python environment ready${NC}"
}

# Setup web dashboard
setup_web_dashboard() {
    echo -e "${BLUE}🌐 Setting up Web Dashboard...${NC}"
    
    cd web-dashboard
    
    # Install Node.js via Homebrew if not present
    if ! command -v node &> /dev/null; then
        echo -e "   Installing Node.js..."
        brew install node > /dev/null 2>&1
    fi
    
    # Install dependencies
    if [[ ! -d "node_modules" ]]; then
        echo -e "   Installing Node.js dependencies..."
        npm install > /dev/null 2>&1
    fi
    
    # Build for production
    echo -e "   Building web dashboard..."
    npm run build > /dev/null 2>&1
    
    cd "$PROJECT_ROOT"
    echo -e "${GREEN}✅ Web dashboard ready${NC}"
}

# Main execution
main() {
    echo -e "${CYAN}🚀 Starting HydraFlow-X Universal Build Process${NC}"
    echo ""
    
    # Clean previous builds
    cleanup_build_dirs
    
    # Install dependencies
    install_dependencies
    
    # Setup Python environment
    setup_python_env
    
    # Build for each target architecture
    local build_success=true
    for arch in "${TARGET_ARCHS[@]}"; do
        if ! build_for_arch "$arch"; then
            build_success=false
            break
        fi
    done
    
    if [[ "$build_success" != true ]]; then
        echo -e "${RED}❌ Build process failed${NC}"
        exit 1
    fi
    
    # Create universal binary if building for multiple architectures
    if [[ "${#TARGET_ARCHS[@]}" -gt 1 ]]; then
        create_universal_binary
    fi
    
    # Setup web dashboard
    setup_web_dashboard
    
    echo ""
    echo -e "${GREEN}🎉 HydraFlow-X Build Complete!${NC}"
    echo -e "${CYAN}=============================${NC}"
    
    # Display available binaries
    echo -e "${YELLOW}📦 Available Binaries:${NC}"
    for arch in "${TARGET_ARCHS[@]}"; do
        if [[ -f "hydraflow-x-${arch}" ]]; then
            local size=$(ls -lh "hydraflow-x-${arch}" | awk '{print $5}')
            echo -e "   • hydraflow-x-${arch} (${size})"
        fi
    done
    
    if [[ -f "hydraflow-x-universal" ]]; then
        local size=$(ls -lh hydraflow-x-universal | awk '{print $5}')
        echo -e "   • hydraflow-x-universal (${size})"
    fi
    
    echo ""
    echo -e "${YELLOW}🚀 Quick Start:${NC}"
    echo -e "   • Run HFT Engine: ${CYAN}./hydraflow-x-${CURRENT_ARCH}${NC}"
    if [[ -f "hydraflow-x-universal" ]]; then
        echo -e "   • Run Universal: ${CYAN}./hydraflow-x-universal${NC}"
    fi
    echo -e "   • Web Dashboard: ${CYAN}cd web-dashboard && npm run dev${NC}"
    echo -e "   • View Logs: ${CYAN}tail -f logs/*.log${NC}"
    
    echo ""
    echo -e "${GREEN}✨ Ready for ultra-low latency HFT trading!${NC}"
}

# Trap Ctrl+C
trap 'echo -e "\n${RED}❌ Build interrupted by user${NC}"; exit 1' INT

# Run main function
main "$@"
