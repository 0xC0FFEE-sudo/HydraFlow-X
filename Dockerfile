# HydraFlow-X Ultra-Low Latency Trading Platform
# Multi-stage build for production optimization

# Stage 1: Build environment
FROM ubuntu:22.04 AS builder

# Set environment variables
ENV DEBIAN_FRONTEND=noninteractive
ENV CMAKE_BUILD_TYPE=Release
ENV CXX_STANDARD=23

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
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
    && rm -rf /var/lib/apt/lists/*

# Set compiler to clang for better optimization
ENV CC=clang-15
ENV CXX=clang++-15

# Create app directory
WORKDIR /app

# Copy source code
COPY . .

# Build the application
RUN mkdir -p build && cd build && \
    cmake -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_STANDARD=23 \
    -DCMAKE_CXX_FLAGS="-O3 -march=native -mtune=native -flto" \
    -DCMAKE_EXE_LINKER_FLAGS="-flto" \
    .. && \
    ninja -j$(nproc)

# Stage 2: Runtime environment  
FROM ubuntu:22.04 AS runtime

# Install runtime dependencies
RUN apt-get update && apt-get install -y \
    libssl3 \
    libpq5 \
    libcurl4 \
    ca-certificates \
    curl \
    htop \
    net-tools \
    && rm -rf /var/lib/apt/lists/*

# Create non-root user for security
RUN useradd -r -s /bin/false -m -d /opt/hydraflow hydraflow

# Create necessary directories
RUN mkdir -p /opt/hydraflow/{bin,config,logs,data,backups} && \
    chown -R hydraflow:hydraflow /opt/hydraflow

# Copy built application
COPY --from=builder /app/build/hydraflow-x /opt/hydraflow/bin/
COPY --from=builder /app/config/ /opt/hydraflow/config/
COPY --from=builder /app/scripts/ /opt/hydraflow/scripts/

# Copy configuration templates
COPY config.env.example /opt/hydraflow/config/
COPY config/hydraflow.example.json /opt/hydraflow/config/

# Set permissions
RUN chmod +x /opt/hydraflow/bin/hydraflow-x && \
    chmod +x /opt/hydraflow/scripts/*.sh && \
    chown -R hydraflow:hydraflow /opt/hydraflow

# Create volume mounts
VOLUME ["/opt/hydraflow/config", "/opt/hydraflow/logs", "/opt/hydraflow/data"]

# Expose ports
EXPOSE 8080 9090 3000

# Health check
HEALTHCHECK --interval=30s --timeout=10s --start-period=5s --retries=3 \
    CMD curl -f http://localhost:8080/api/health || exit 1

# Switch to non-root user
USER hydraflow
WORKDIR /opt/hydraflow

# Set environment variables
ENV PATH="/opt/hydraflow/bin:$PATH"
ENV HFX_CONFIG_PATH="/opt/hydraflow/config"
ENV HFX_LOG_PATH="/opt/hydraflow/logs"
ENV HFX_DATA_PATH="/opt/hydraflow/data"

# Default command
CMD ["hydraflow-x", "--config", "/opt/hydraflow/config/hydraflow.json"]
