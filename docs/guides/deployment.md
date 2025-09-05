# Deployment Guide

This guide covers various deployment options for HydraFlow-X in different environments.

## Quick Deployment Options

### 1. Docker Compose (Development)

Perfect for development and testing:

```yaml
# docker-compose.yml
version: '3.8'
services:
  hydraflow-backend:
    image: hydraflow/backend:latest
    ports:
      - "8080:8080"
    environment:
      - POSTGRES_HOST=postgres
      - REDIS_HOST=redis
    depends_on:
      - postgres
      - redis

  postgres:
    image: postgres:15-alpine
    environment:
      POSTGRES_DB: hydraflow

  redis:
    image: redis:7-alpine

  hydraflow-frontend:
    image: hydraflow/frontend:latest
    ports:
      - "3000:3000"
```

```bash
# Deploy
docker-compose up -d

# Scale services
docker-compose up -d --scale hydraflow-backend=3
```

### 2. Kubernetes (Production)

Recommended for production deployments:

```yaml
# k8s/deployment.yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: hydraflow-backend
spec:
  replicas: 3
  selector:
    matchLabels:
      app: hydraflow-backend
  template:
    metadata:
      labels:
        app: hydraflow-backend
    spec:
      containers:
      - name: backend
        image: hydraflow/backend:latest
        ports:
        - containerPort: 8080
        resources:
          requests:
            cpu: "1000m"
            memory: "2Gi"
          limits:
            cpu: "2000m"
            memory: "4Gi"
        livenessProbe:
          httpGet:
            path: /api/v1/health
            port: 8080
          initialDelaySeconds: 30
          periodSeconds: 10
        readinessProbe:
          httpGet:
            path: /api/v1/health
            port: 8080
          initialDelaySeconds: 5
          periodSeconds: 5
```

```bash
# Deploy to Kubernetes
kubectl apply -f k8s/

# Check deployment
kubectl get pods
kubectl get services

# Scale deployment
kubectl scale deployment hydraflow-backend --replicas=5
```

## Cloud Platform Deployments

### AWS

#### ECS (Elastic Container Service)

```yaml
# task-definition.json
{
  "family": "hydraflow-backend",
  "taskRoleArn": "arn:aws:iam::123456789012:role/ecsTaskExecutionRole",
  "executionRoleArn": "arn:aws:iam::123456789012:role/ecsTaskExecutionRole",
  "networkMode": "awsvpc",
  "requiresCompatibilities": ["FARGATE"],
  "cpu": "1024",
  "memory": "2048",
  "containerDefinitions": [
    {
      "name": "backend",
      "image": "hydraflow/backend:latest",
      "essential": true,
      "portMappings": [
        {
          "containerPort": 8080,
          "protocol": "tcp"
        }
      ],
      "environment": [
        {"name": "POSTGRES_HOST", "value": "your-rds-endpoint"},
        {"name": "REDIS_HOST", "value": "your-elasticache-endpoint"}
      ],
      "logConfiguration": {
        "logDriver": "awslogs",
        "options": {
          "awslogs-group": "/ecs/hydraflow-backend",
          "awslogs-region": "us-east-1",
          "awslogs-stream-prefix": "ecs"
        }
      }
    }
  ]
}
```

#### EKS (Elastic Kubernetes Service)

```bash
# Create EKS cluster
eksctl create cluster --name hydraflow-cluster --region us-east-1

# Deploy using Helm
helm install hydraflow ./helm/hydraflow \
  --set postgresql.enabled=true \
  --set redis.enabled=true \
  --set prometheus.enabled=true \
  --set grafana.enabled=true
```

### Google Cloud Platform

#### GKE (Google Kubernetes Engine)

```bash
# Create GKE cluster
gcloud container clusters create hydraflow-cluster \
  --region us-central1 \
  --num-nodes 3 \
  --machine-type n1-standard-4

# Deploy
kubectl apply -f k8s/
```

#### Cloud Run

```yaml
# service.yaml
apiVersion: serving.knative.dev/v1
kind: Service
metadata:
  name: hydraflow-backend
spec:
  template:
    spec:
      containers:
      - image: hydraflow/backend:latest
        ports:
        - containerPort: 8080
        env:
        - name: POSTGRES_HOST
          value: "/cloudsql/your-project:region:instance"
        resources:
          limits:
            cpu: "1000m"
            memory: "2Gi"
```

### Azure

#### AKS (Azure Kubernetes Service)

```bash
# Create AKS cluster
az aks create --resource-group hydraflow-rg \
  --name hydraflow-cluster \
  --node-count 3 \
  --node-vm-size Standard_D4s_v3

# Deploy
kubectl apply -f k8s/
```

## Performance Optimization

### CPU Optimization

```yaml
# For high-frequency trading
spec:
  nodeSelector:
    node-type: high-performance
  containers:
  - name: backend
    resources:
      requests:
        cpu: "2000m"
      limits:
        cpu: "4000m"
    affinity:
      nodeAffinity:
        preferredDuringSchedulingIgnoredDuringExecution:
        - weight: 100
          preference:
            matchExpressions:
            - key: cpu-type
              operator: In
              values:
              - intel-xeon
```

### Memory Optimization

```yaml
containers:
- name: backend
  resources:
    requests:
      memory: "4Gi"
    limits:
      memory: "8Gi"
  env:
  - name: MALLOC_ARENA_MAX
    value: "1"  # Reduce memory fragmentation
```

### Network Optimization

```yaml
# Use host networking for lowest latency
spec:
  hostNetwork: true
  dnsPolicy: ClusterFirstWithHostNet

# Or use node-local networking
annotations:
  k8s.v1.cni.cncf.io/networks: '[{"name": "node-local", "interface": "eth0"}]'
```

## Monitoring and Observability

### Prometheus & Grafana Setup

```yaml
# prometheus-config.yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: prometheus-config
data:
  prometheus.yml: |
    global:
      scrape_interval: 15s
    scrape_configs:
    - job_name: 'hydraflow-backend'
      static_configs:
      - targets: ['hydraflow-backend:8080']
      metrics_path: '/api/v1/metrics/prometheus'
```

### Logging

#### Centralized Logging with ELK Stack

```yaml
# fluent-bit-config.yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: fluent-bit-config
data:
  fluent-bit.conf: |
    [INPUT]
        Name tail
        Path /var/log/containers/*hydraflow*.log
        Parser docker

    [OUTPUT]
        Name elasticsearch
        Host elasticsearch-master
        Port 9200
        Index hydraflow-logs
```

### Alerting

```yaml
# alert-rules.yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: hydraflow-alerts
spec:
  groups:
  - name: hydraflow
    rules:
    - alert: HighLatency
      expr: hydraflow_request_duration_seconds{quantile="0.95"} > 0.1
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "High request latency detected"
```

## Security Hardening

### TLS/SSL Configuration

```yaml
# ingress-tls.yaml
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: hydraflow-ingress
  annotations:
    cert-manager.io/cluster-issuer: "letsencrypt-prod"
    nginx.ingress.kubernetes.io/ssl-redirect: "true"
spec:
  tls:
  - hosts:
    - api.hydraflow.com
    secretName: hydraflow-tls
  rules:
  - host: api.hydraflow.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: hydraflow-backend
            port:
              number: 80
```

### Network Policies

```yaml
# network-policy.yaml
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: hydraflow-backend-policy
spec:
  podSelector:
    matchLabels:
      app: hydraflow-backend
  policyTypes:
  - Ingress
  - Egress
  ingress:
  - from:
    - podSelector:
        matchLabels:
          app: hydraflow-frontend
    ports:
    - protocol: TCP
      port: 8080
  egress:
  - to:
    - podSelector:
        matchLabels:
          app: postgres
    ports:
    - protocol: TCP
      port: 5432
  - to: []  # Deny all other egress traffic
```

### Secrets Management

```yaml
# Use external secret management
apiVersion: external-secrets.io/v1beta1
kind: ExternalSecret
metadata:
  name: hydraflow-secrets
spec:
  secretStoreRef:
    name: aws-secretsmanager
    kind: SecretStore
  target:
    name: hydraflow-secret
    creationPolicy: Owner
  data:
  - secretKey: jwt-secret
    remoteRef:
      key: prod/hydraflow/jwt-secret
  - secretKey: db-password
    remoteRef:
      key: prod/hydraflow/db-password
```

## High Availability

### Multi-Region Deployment

```yaml
# Multi-region service
apiVersion: v1
kind: Service
metadata:
  name: hydraflow-backend-global
  annotations:
    service.beta.kubernetes.io/aws-load-balancer-cross-zone-load-balancing-enabled: "true"
    service.beta.kubernetes.io/aws-load-balancer-type: nlb
spec:
  type: LoadBalancer
  selector:
    app: hydraflow-backend
  ports:
  - port: 80
    targetPort: 8080
```

### Database Replication

```yaml
# PostgreSQL replication
apiVersion: acid.zalan.do/v1
kind: postgresql
metadata:
  name: hydraflow-db
spec:
  teamId: hydraflow
  volume:
    size: 50Gi
    storageClass: fast-ssd
  numberOfInstances: 3
  users:
    hydraflow:
    - superuser
    - createdb
  databases:
    hydraflow: hydraflow
  postgresql:
    version: "15"
```

## Backup and Recovery

### Database Backup

```yaml
# PostgreSQL backup
apiVersion: batch/v1
kind: CronJob
metadata:
  name: postgres-backup
spec:
  schedule: "0 2 * * *"  # Daily at 2 AM
  jobTemplate:
    spec:
      template:
        spec:
          containers:
          - name: backup
            image: postgres:15-alpine
            command:
            - /bin/sh
            - -c
            - |
              pg_dump -h postgres -U hydraflow hydraflow | gzip > /backup/hydraflow-$(date +%Y%m%d-%H%M%S).sql.gz
            volumeMounts:
            - name: backup-volume
              mountPath: /backup
          volumes:
          - name: backup-volume
            persistentVolumeClaim:
              claimName: backup-pvc
          restartPolicy: OnFailure
```

### Disaster Recovery

```yaml
# Disaster recovery plan
apiVersion: batch/v1
kind: Job
metadata:
  name: disaster-recovery
spec:
  template:
    spec:
      containers:
      - name: recovery
        image: hydraflow/recovery:latest
        command: ["/app/recovery.sh"]
        env:
        - name: BACKUP_SOURCE
          value: "s3://hydraflow-backups/latest/"
        - name: RECOVERY_TARGET
          value: "postgres:5432"
      restartPolicy: Never
```

## Cost Optimization

### Auto-scaling

```yaml
# Horizontal Pod Autoscaler
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: hydraflow-backend-hpa
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: hydraflow-backend
  minReplicas: 2
  maxReplicas: 10
  metrics:
  - type: Resource
    resource:
      name: cpu
      target:
        type: Utilization
        averageUtilization: 70
  - type: Resource
    resource:
      name: memory
      target:
        type: Utilization
        averageUtilization: 80
```

### Spot Instances

```yaml
# Use spot instances for cost savings
spec:
  template:
    spec:
      nodeSelector:
        lifecycle: spot
      tolerations:
      - key: lifecycle
        operator: Equal
        value: spot
        effect: NoSchedule
```

## Troubleshooting

### Common Deployment Issues

1. **Pod Crashes**
   ```bash
   # Check pod logs
   kubectl logs -f deployment/hydraflow-backend

   # Check pod events
   kubectl describe pod <pod-name>
   ```

2. **Service Unavailable**
   ```bash
   # Check service endpoints
   kubectl get endpoints hydraflow-backend

   # Check service configuration
   kubectl describe service hydraflow-backend
   ```

3. **Resource Constraints**
   ```bash
   # Check resource usage
   kubectl top pods

   # Check node resources
   kubectl describe nodes
   ```

### Performance Monitoring

```bash
# Monitor pod performance
kubectl top pods --all-namespaces

# Monitor cluster resources
kubectl get nodes --show-labels

# Check network policies
kubectl get networkpolicies
```

### Log Analysis

```bash
# Stream logs from all pods
kubectl logs -f -l app=hydraflow-backend --tail=100

# Search logs for errors
kubectl logs -l app=hydraflow-backend | grep ERROR

# Export logs for analysis
kubectl logs -l app=hydraflow-backend > hydraflow-logs.txt
```

## Maintenance

### Rolling Updates

```bash
# Update deployment image
kubectl set image deployment/hydraflow-backend backend=hydraflow/backend:v2.0.0

# Check rollout status
kubectl rollout status deployment/hydraflow-backend

# Rollback if needed
kubectl rollout undo deployment/hydraflow-backend
```

### Database Maintenance

```bash
# Vacuum database
kubectl exec -it postgres-pod -- psql -U hydraflow -d hydraflow -c "VACUUM ANALYZE;"

# Reindex tables
kubectl exec -it postgres-pod -- psql -U hydraflow -d hydraflow -c "REINDEX DATABASE hydraflow;"
```

### Certificate Renewal

```bash
# Renew TLS certificates
kubectl delete secret hydraflow-tls

# Certificate will be automatically renewed by cert-manager
kubectl get certificates
```

Remember to regularly update your deployments, monitor performance, and maintain backups for business continuity.
