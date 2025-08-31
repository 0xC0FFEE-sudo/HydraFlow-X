# HydraFlow-X Deployment Guide

This guide covers deploying HydraFlow-X in various environments, from development to production-grade setups.

## 🎯 Deployment Options

### 1. Quick Start (Docker)
**Best for:** Testing and development
**Setup time:** 5 minutes

### 2. Native Installation
**Best for:** Maximum performance
**Setup time:** 15-30 minutes

### 3. Cloud Deployment
**Best for:** Production trading
**Setup time:** 30-60 minutes

### 4. Kubernetes
**Best for:** Enterprise/scaled production
**Setup time:** 1-2 hours

## 🐳 Docker Deployment (Recommended)

Docker deployment is the fastest way to get HydraFlow-X running with all dependencies.

### Prerequisites
- Docker 20.10+
- Docker Compose 2.0+
- 4GB+ RAM
- 2GB+ free disk space

### Quick Start

```bash
# Clone the repository
git clone https://github.com/yourusername/HydraFlow-X.git
cd HydraFlow-X

# Copy environment template
cp config.env.example .env

# Edit configuration (add your API keys)
nano .env

# Start all services
docker-compose up -d

# View logs
docker-compose logs -f hydraflow-x

# Access dashboard
open http://localhost:8080
```

### Environment Configuration

Edit `.env` file with your settings:

```bash
# Database passwords
HFX_DB_PASSWORD=your_secure_password
HFX_CLICKHOUSE_PASSWORD=your_secure_password

# Dashboard credentials
GRAFANA_USER=admin
GRAFANA_PASSWORD=your_admin_password

# API Keys (optional for testing)
HFX_TWITTER_API_KEY=your_twitter_key
HFX_REDDIT_API_KEY=your_reddit_key
HFX_ETHEREUM_RPC_API_KEY=your_ethereum_key
HFX_SOLANA_RPC_API_KEY=your_solana_key

# Trading settings (for production)
HFX_MAX_POSITION_SIZE=1000.0
HFX_PAPER_TRADING=false
HFX_TEST_MODE=false
```

### Service Architecture

The Docker deployment includes:

```
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│   HydraFlow-X   │    │   PostgreSQL    │    │   ClickHouse    │
│  Trading Engine │◄──►│   (OLTP Data)   │    │  (Analytics)    │
│   Port: 8080    │    │   Port: 5432    │    │  Port: 8123     │
└─────────────────┘    └─────────────────┘    └─────────────────┘
         │
         ▼
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│     Redis       │    │   Prometheus    │    │     Grafana     │
│   (Caching)     │    │   (Metrics)     │    │  (Dashboard)    │
│   Port: 6379    │    │   Port: 9091    │    │  Port: 3000     │
└─────────────────┘    └─────────────────┘    └─────────────────┘
```

### Docker Commands

```bash
# Start services
docker-compose up -d

# Stop services
docker-compose down

# View logs
docker-compose logs -f [service_name]

# Update to latest version
docker-compose pull
docker-compose up -d

# Backup data
docker-compose exec postgres pg_dump -U hydraflow hydraflow > backup.sql

# Monitor resources
docker stats

# Scale services
docker-compose up -d --scale hydraflow-x=2
```

## 🖥️ Native Installation

For maximum performance, install HydraFlow-X directly on the host system.

### Automated Installation

```bash
# Linux/macOS one-liner
curl -sSL https://raw.githubusercontent.com/yourusername/HydraFlow-X/main/scripts/install.sh | bash

# Windows PowerShell
Invoke-WebRequest -Uri "https://raw.githubusercontent.com/yourusername/HydraFlow-X/main/scripts/install.ps1" | Invoke-Expression
```

### Manual Installation

#### Ubuntu 22.04+

```bash
# Install dependencies
sudo apt update
sudo apt install -y build-essential cmake ninja-build pkg-config \
  libssl-dev libpq-dev libcurl4-openssl-dev libnlohmann-json3-dev \
  libboost-all-dev clang-15 postgresql redis-server

# Install Node.js 18
curl -fsSL https://deb.nodesource.com/setup_18.x | sudo -E bash -
sudo apt install -y nodejs

# Clone and build
git clone https://github.com/yourusername/HydraFlow-X.git
cd HydraFlow-X
mkdir build && cd build
cmake -G Ninja -DCMAKE_BUILD_TYPE=Release ..
ninja -j$(nproc)

# Install binary
sudo cp hydraflow-x /usr/local/bin/
sudo mkdir -p /etc/hydraflow-x
sudo cp -r ../config/* /etc/hydraflow-x/

# Create systemd service
sudo cp ../scripts/hydraflow-x.service /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable hydraflow-x
```

#### macOS

```bash
# Install Homebrew dependencies
brew install cmake ninja pkg-config openssl postgresql curl \
  nlohmann-json boost llvm redis node@18

# Clone and build
git clone https://github.com/yourusername/HydraFlow-X.git
cd HydraFlow-X
mkdir build && cd build
cmake -G Ninja -DCMAKE_BUILD_TYPE=Release ..
ninja -j$(sysctl -n hw.ncpu)

# Install binary
cp hydraflow-x /usr/local/bin/
mkdir -p ~/Library/Application\ Support/HydraFlow-X
cp -r ../config/* ~/Library/Application\ Support/HydraFlow-X/
```

### Performance Tuning

#### System Limits

Add to `/etc/security/limits.conf`:
```
hydraflow soft nofile 65536
hydraflow hard nofile 65536
hydraflow soft nproc 32768
hydraflow hard nproc 32768
```

#### Kernel Parameters

Add to `/etc/sysctl.conf`:
```
# Network performance
net.core.rmem_max = 134217728
net.core.wmem_max = 134217728
net.ipv4.tcp_rmem = 4096 65536 134217728
net.ipv4.tcp_wmem = 4096 65536 134217728

# Low latency
net.core.netdev_max_backlog = 5000
net.ipv4.tcp_congestion_control = bbr
```

Apply changes:
```bash
sudo sysctl -p
```

## ☁️ Cloud Deployment

### AWS Deployment

#### Using EC2

1. **Launch EC2 Instance**
   - Instance type: `c6i.xlarge` or higher
   - AMI: Ubuntu 22.04 LTS
   - Storage: 50GB gp3 SSD
   - Security group: Allow ports 22, 8080, 8081

2. **Setup Script**

```bash
#!/bin/bash
# AWS EC2 setup script

# Install Docker
curl -fsSL https://get.docker.com -o get-docker.sh
sh get-docker.sh
sudo usermod -aG docker ubuntu

# Install Docker Compose
sudo curl -L "https://github.com/docker/compose/releases/latest/download/docker-compose-$(uname -s)-$(uname -m)" -o /usr/local/bin/docker-compose
sudo chmod +x /usr/local/bin/docker-compose

# Clone and configure
git clone https://github.com/yourusername/HydraFlow-X.git
cd HydraFlow-X
cp config.env.example .env

# Configure for production
sed -i 's/HFX_ENVIRONMENT=development/HFX_ENVIRONMENT=production/' .env
sed -i 's/HFX_PAPER_TRADING=true/HFX_PAPER_TRADING=false/' .env

# Start services
docker-compose up -d

# Setup log rotation
sudo tee /etc/logrotate.d/hydraflow-x > /dev/null << EOF
/var/lib/docker/containers/*/*-json.log {
    daily
    rotate 7
    compress
    delaycompress
    missingok
    notifempty
    create 644 root root
    postrotate
        docker kill --signal="USR1" $(docker ps -q) 2>/dev/null || true
    endscript
}
EOF
```

3. **Load Balancer Setup**

Use AWS Application Load Balancer:
- Target group: EC2 instances on port 8080
- Health check: `/api/health`
- SSL termination with ACM certificate

#### Using ECS

```yaml
# ecs-task-definition.json
{
  "family": "hydraflow-x",
  "networkMode": "awsvpc",
  "requiresCompatibilities": ["FARGATE"],
  "cpu": "2048",
  "memory": "4096",
  "executionRoleArn": "arn:aws:iam::account:role/ecsTaskExecutionRole",
  "containerDefinitions": [
    {
      "name": "hydraflow-x",
      "image": "hydraflow/hydraflow-x:latest",
      "portMappings": [
        {
          "containerPort": 8080,
          "protocol": "tcp"
        }
      ],
      "environment": [
        {
          "name": "HFX_ENVIRONMENT",
          "value": "production"
        }
      ],
      "secrets": [
        {
          "name": "HFX_DB_PASSWORD",
          "valueFrom": "arn:aws:secretsmanager:region:account:secret:hydraflow-db-password"
        }
      ],
      "logConfiguration": {
        "logDriver": "awslogs",
        "options": {
          "awslogs-group": "/ecs/hydraflow-x",
          "awslogs-region": "us-east-1",
          "awslogs-stream-prefix": "ecs"
        }
      }
    }
  ]
}
```

### Google Cloud Platform

```bash
# Create GKE cluster
gcloud container clusters create hydraflow-cluster \
  --num-nodes=3 \
  --machine-type=c2-standard-4 \
  --zone=us-central1-a

# Deploy with Kubernetes
kubectl apply -f kubernetes/
```

### Azure

```bash
# Create AKS cluster
az aks create \
  --resource-group hydraflow-rg \
  --name hydraflow-cluster \
  --node-count 3 \
  --node-vm-size Standard_D4s_v3 \
  --location eastus

# Deploy application
kubectl apply -f kubernetes/
```

## ⚙️ Kubernetes Deployment

### Prerequisites

- Kubernetes 1.20+
- kubectl configured
- Helm 3.0+ (optional)

### Deployment Manifests

#### Namespace

```yaml
# kubernetes/namespace.yaml
apiVersion: v1
kind: Namespace
metadata:
  name: hydraflow
```

#### ConfigMap

```yaml
# kubernetes/configmap.yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: hydraflow-config
  namespace: hydraflow
data:
  HFX_ENVIRONMENT: "production"
  HFX_LOG_LEVEL: "INFO"
  HFX_WEB_PORT: "8080"
  HFX_DB_HOST: "postgresql"
  HFX_REDIS_HOST: "redis"
```

#### Secrets

```yaml
# kubernetes/secrets.yaml
apiVersion: v1
kind: Secret
metadata:
  name: hydraflow-secrets
  namespace: hydraflow
type: Opaque
data:
  db-password: <base64-encoded-password>
  twitter-api-key: <base64-encoded-key>
  ethereum-rpc-key: <base64-encoded-key>
```

#### Deployment

```yaml
# kubernetes/deployment.yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: hydraflow-x
  namespace: hydraflow
spec:
  replicas: 3
  selector:
    matchLabels:
      app: hydraflow-x
  template:
    metadata:
      labels:
        app: hydraflow-x
    spec:
      containers:
      - name: hydraflow-x
        image: hydraflow/hydraflow-x:latest
        ports:
        - containerPort: 8080
        envFrom:
        - configMapRef:
            name: hydraflow-config
        - secretRef:
            name: hydraflow-secrets
        resources:
          requests:
            memory: "2Gi"
            cpu: "1000m"
          limits:
            memory: "4Gi"
            cpu: "2000m"
        livenessProbe:
          httpGet:
            path: /api/health
            port: 8080
          initialDelaySeconds: 30
          periodSeconds: 10
        readinessProbe:
          httpGet:
            path: /api/health
            port: 8080
          initialDelaySeconds: 5
          periodSeconds: 5
```

#### Service

```yaml
# kubernetes/service.yaml
apiVersion: v1
kind: Service
metadata:
  name: hydraflow-x-service
  namespace: hydraflow
spec:
  selector:
    app: hydraflow-x
  ports:
  - protocol: TCP
    port: 80
    targetPort: 8080
  type: LoadBalancer
```

### Deploy to Kubernetes

```bash
# Apply all manifests
kubectl apply -f kubernetes/

# Check deployment status
kubectl get pods -n hydraflow

# View logs
kubectl logs -f deployment/hydraflow-x -n hydraflow

# Port forward for testing
kubectl port-forward service/hydraflow-x-service 8080:80 -n hydraflow
```

### Helm Chart (Optional)

```bash
# Install with Helm
helm repo add hydraflow https://charts.hydraflow.com
helm install my-hydraflow hydraflow/hydraflow-x \
  --namespace hydraflow \
  --create-namespace \
  --set environment.production=true \
  --set secrets.dbPassword="your-password"
```

## 📊 Monitoring and Observability

### Prometheus Metrics

HydraFlow-X exposes Prometheus metrics at `/metrics`:

```bash
# Scrape metrics
curl http://localhost:8080/metrics
```

Key metrics:
- `hydraflow_trades_total`: Total number of trades
- `hydraflow_latency_seconds`: Request latency histogram
- `hydraflow_errors_total`: Error count by type
- `hydraflow_balance_usd`: Current portfolio balance

### Grafana Dashboard

Import the included Grafana dashboard:

```bash
# Import dashboard
curl -X POST \
  http://admin:password@localhost:3000/api/dashboards/db \
  -H 'Content-Type: application/json' \
  -d @monitoring/grafana/dashboards/hydraflow-overview.json
```

### Log Aggregation

For production deployments, use centralized logging:

#### ELK Stack

```yaml
# docker-compose.logging.yml
version: '3.8'
services:
  elasticsearch:
    image: elasticsearch:8.5.0
    environment:
      - discovery.type=single-node
      - xpack.security.enabled=false
    
  logstash:
    image: logstash:8.5.0
    volumes:
      - ./logstash.conf:/usr/share/logstash/pipeline/logstash.conf
    
  kibana:
    image: kibana:8.5.0
    ports:
      - "5601:5601"
```

#### Fluentd

```yaml
# fluentd configuration
<source>
  @type tail
  path /var/log/hydraflow/*.log
  pos_file /var/log/fluentd/hydraflow.log.pos
  tag hydraflow.*
  format json
</source>

<match hydraflow.**>
  @type elasticsearch
  host elasticsearch
  port 9200
  index_name hydraflow
</match>
```

## 🔒 Security Hardening

### Production Security Checklist

- [ ] Use strong passwords for all services
- [ ] Enable SSL/TLS for all endpoints
- [ ] Configure firewall rules
- [ ] Set up VPN access for management
- [ ] Enable audit logging
- [ ] Regular security updates
- [ ] API rate limiting
- [ ] Input validation
- [ ] Secrets management
- [ ] Network segmentation

### SSL/TLS Configuration

#### Let's Encrypt with Nginx

```nginx
# /etc/nginx/sites-available/hydraflow
server {
    listen 80;
    server_name your-domain.com;
    return 301 https://$server_name$request_uri;
}

server {
    listen 443 ssl http2;
    server_name your-domain.com;
    
    ssl_certificate /etc/letsencrypt/live/your-domain.com/fullchain.pem;
    ssl_certificate_key /etc/letsencrypt/live/your-domain.com/privkey.pem;
    
    # SSL configuration
    ssl_protocols TLSv1.2 TLSv1.3;
    ssl_ciphers ECDHE-RSA-AES128-GCM-SHA256:ECDHE-RSA-AES256-GCM-SHA384;
    ssl_prefer_server_ciphers off;
    
    location / {
        proxy_pass http://localhost:8080;
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
        proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
        proxy_set_header X-Forwarded-Proto $scheme;
    }
}
```

### Firewall Configuration

```bash
# UFW (Ubuntu)
sudo ufw allow ssh
sudo ufw allow 80/tcp
sudo ufw allow 443/tcp
sudo ufw deny 5432/tcp  # PostgreSQL - internal only
sudo ufw deny 6379/tcp  # Redis - internal only
sudo ufw enable

# iptables
iptables -A INPUT -p tcp --dport 22 -j ACCEPT
iptables -A INPUT -p tcp --dport 80 -j ACCEPT
iptables -A INPUT -p tcp --dport 443 -j ACCEPT
iptables -A INPUT -p tcp --dport 5432 -j DROP
iptables -A INPUT -p tcp --dport 6379 -j DROP
```

## 🚀 Performance Optimization

### Hardware Recommendations

#### Minimum Requirements
- CPU: 4 cores, 2.5GHz+
- RAM: 8GB
- Storage: 100GB SSD
- Network: 100Mbps

#### Recommended Production
- CPU: 8+ cores, 3.0GHz+ (Intel/AMD) or Apple M1+
- RAM: 16GB+
- Storage: 500GB NVMe SSD
- Network: 1Gbps+

#### High-Frequency Trading
- CPU: 16+ cores, 3.5GHz+, low-latency
- RAM: 32GB+
- Storage: 1TB+ NVMe SSD with high IOPS
- Network: 10Gbps+ with low latency
- Co-location: Near exchange data centers

### OS Tuning

#### Linux Kernel Parameters

```bash
# Add to /etc/sysctl.conf
vm.swappiness=1
vm.dirty_ratio=15
vm.dirty_background_ratio=5
net.core.rmem_max=134217728
net.core.wmem_max=134217728
net.ipv4.tcp_rmem=4096 65536 134217728
net.ipv4.tcp_wmem=4096 65536 134217728
net.core.netdev_max_backlog=5000
```

#### CPU Isolation

```bash
# Isolate CPUs for trading threads
echo "isolcpus=4-7" >> /etc/default/grub
update-grub
reboot
```

## 🔄 Backup and Recovery

### Database Backup

```bash
#!/bin/bash
# backup-database.sh

BACKUP_DIR="/backup/hydraflow"
DATE=$(date +%Y%m%d_%H%M%S)

# PostgreSQL backup
pg_dump -h localhost -U hydraflow hydraflow > "$BACKUP_DIR/postgres_$DATE.sql"

# ClickHouse backup
clickhouse-client --query="BACKUP DATABASE hydraflow_analytics TO S3('s3://bucket/backups/clickhouse_$DATE')"

# Compress and encrypt
gzip "$BACKUP_DIR/postgres_$DATE.sql"
gpg --encrypt --recipient admin@hydraflow.com "$BACKUP_DIR/postgres_$DATE.sql.gz"

# Upload to S3
aws s3 cp "$BACKUP_DIR/postgres_$DATE.sql.gz.gpg" s3://hydraflow-backups/

# Cleanup old backups (keep 30 days)
find "$BACKUP_DIR" -name "*.sql.gz.gpg" -mtime +30 -delete
```

### Configuration Backup

```bash
# Backup configuration
tar -czf config_backup_$(date +%Y%m%d).tar.gz \
  /etc/hydraflow-x/ \
  /opt/hydraflow/config/ \
  /home/hydraflow/.hydraflow/
```

### Recovery Procedure

```bash
# Stop services
docker-compose down

# Restore database
gunzip postgres_backup.sql.gz
psql -h localhost -U hydraflow hydraflow < postgres_backup.sql

# Restore configuration
tar -xzf config_backup.tar.gz -C /

# Start services
docker-compose up -d

# Verify restoration
curl http://localhost:8080/api/health
```

## 🔧 Troubleshooting

### Common Issues

#### High Memory Usage
```bash
# Check memory usage
docker stats
free -h

# Optimize PostgreSQL
echo "shared_buffers = 256MB" >> /etc/postgresql/14/main/postgresql.conf
echo "effective_cache_size = 1GB" >> /etc/postgresql/14/main/postgresql.conf
```

#### High Latency
```bash
# Check network latency
ping -c 10 api.ethereum.org

# Monitor system load
top
iostat -x 1

# Check for CPU throttling
cat /proc/cpuinfo | grep MHz
```

#### Database Connection Issues
```bash
# Check PostgreSQL status
systemctl status postgresql

# Check connections
netstat -ant | grep 5432

# View PostgreSQL logs
tail -f /var/log/postgresql/postgresql-14-main.log
```

### Log Analysis

```bash
# View application logs
docker-compose logs -f hydraflow-x | grep ERROR

# Monitor trading activity
tail -f /opt/hydraflow/logs/trading.log | grep "TRADE_"

# Check system metrics
journalctl -u hydraflow-x -f
```

### Performance Profiling

```bash
# CPU profiling
perf record -g ./hydraflow-x
perf report

# Memory profiling
valgrind --tool=massif ./hydraflow-x

# Network monitoring
iftop -i eth0
```

## 📞 Support

For deployment assistance:

1. **Documentation**: Check this guide and [README.md](../README.md)
2. **GitHub Issues**: Report problems or ask questions
3. **Discord Community**: Real-time chat support
4. **Email Support**: For enterprise deployments

---

**Remember**: Always test deployments in a staging environment before production. Cryptocurrency trading involves substantial risk.
