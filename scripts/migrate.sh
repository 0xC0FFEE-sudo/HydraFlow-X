#!/bin/bash

# HydraFlow-X Database Migration Script
# Handles schema migrations for PostgreSQL and ClickHouse

set -e

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
MIGRATIONS_DIR="$SCRIPT_DIR/migrations"
PG_HOST=${HFX_DB_HOST:-localhost}
PG_PORT=${HFX_DB_PORT:-5432}
PG_DB=${HFX_DB_NAME:-hydraflow}
PG_USER=${HFX_DB_USER:-hydraflow}
CH_HOST=${HFX_CLICKHOUSE_HOST:-localhost}
CH_PORT=${HFX_CLICKHOUSE_PORT:-8123}
CH_DB=${HFX_CLICKHOUSE_DB:-hydraflow_analytics}
CH_USER=${HFX_CLICKHOUSE_USER:-default}

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

# Function to show usage
show_usage() {
    echo "HydraFlow-X Database Migration Script"
    echo "===================================="
    echo ""
    echo "Usage: $0 [COMMAND] [OPTIONS]"
    echo ""
    echo "Commands:"
    echo "  init              Initialize migration system"
    echo "  create <name>     Create a new migration"
    echo "  migrate           Run pending migrations"
    echo "  rollback [steps]  Rollback migrations (default: 1 step)"
    echo "  status            Show migration status"
    echo "  reset             Reset database and run all migrations"
    echo ""
    echo "Options:"
    echo "  --pg-only         PostgreSQL migrations only"
    echo "  --ch-only         ClickHouse migrations only"
    echo "  --dry-run         Show what would be executed"
    echo "  --force           Force execution (skip confirmations)"
    echo ""
    echo "Environment Variables:"
    echo "  HFX_DB_HOST       PostgreSQL host (default: localhost)"
    echo "  HFX_DB_PORT       PostgreSQL port (default: 5432)"
    echo "  HFX_DB_NAME       PostgreSQL database (default: hydraflow)"
    echo "  HFX_DB_USER       PostgreSQL user (default: hydraflow)"
    echo "  HFX_DB_PASSWORD   PostgreSQL password"
    echo "  HFX_CLICKHOUSE_HOST     ClickHouse host (default: localhost)"
    echo "  HFX_CLICKHOUSE_PORT     ClickHouse port (default: 8123)"
    echo "  HFX_CLICKHOUSE_DB       ClickHouse database (default: hydraflow_analytics)"
    echo "  HFX_CLICKHOUSE_USER     ClickHouse user (default: default)"
    echo "  HFX_CLICKHOUSE_PASSWORD ClickHouse password"
}

# Function to check database connections
check_connections() {
    print_info "Checking database connections..."
    
    # Check PostgreSQL
    if [ "$CH_ONLY" != "true" ]; then
        if ! PGPASSWORD="$HFX_DB_PASSWORD" psql -h "$PG_HOST" -p "$PG_PORT" -U "$PG_USER" -d "$PG_DB" -c "SELECT 1;" >/dev/null 2>&1; then
            print_error "Cannot connect to PostgreSQL database"
            print_info "Make sure PostgreSQL is running and credentials are correct"
            exit 1
        fi
        print_success "PostgreSQL connection successful"
    fi
    
    # Check ClickHouse
    if [ "$PG_ONLY" != "true" ]; then
        if ! curl -s "http://$CH_HOST:$CH_PORT/ping" >/dev/null 2>&1; then
            print_error "Cannot connect to ClickHouse database"
            print_info "Make sure ClickHouse is running and accessible"
            exit 1
        fi
        print_success "ClickHouse connection successful"
    fi
}

# Function to initialize migration system
init_migrations() {
    print_info "Initializing migration system..."
    
    # Create migrations directory
    mkdir -p "$MIGRATIONS_DIR"/{postgresql,clickhouse}
    
    # Create migration tracking table in PostgreSQL
    if [ "$CH_ONLY" != "true" ]; then
        print_info "Creating PostgreSQL migration tracking table..."
        PGPASSWORD="$HFX_DB_PASSWORD" psql -h "$PG_HOST" -p "$PG_PORT" -U "$PG_USER" -d "$PG_DB" << EOF
CREATE SCHEMA IF NOT EXISTS migrations;

CREATE TABLE IF NOT EXISTS migrations.schema_migrations (
    id SERIAL PRIMARY KEY,
    version VARCHAR(255) NOT NULL UNIQUE,
    name VARCHAR(255) NOT NULL,
    applied_at TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
    execution_time_ms INTEGER,
    checksum VARCHAR(64)
);

CREATE INDEX IF NOT EXISTS idx_schema_migrations_version ON migrations.schema_migrations(version);
EOF
    fi
    
    # Create migration tracking table in ClickHouse
    if [ "$PG_ONLY" != "true" ]; then
        print_info "Creating ClickHouse migration tracking table..."
        curl -s "http://$CH_HOST:$CH_PORT/" --data "
CREATE DATABASE IF NOT EXISTS migrations;

CREATE TABLE IF NOT EXISTS migrations.schema_migrations
(
    version String,
    name String,
    applied_at DateTime DEFAULT now(),
    execution_time_ms UInt32,
    checksum String
)
ENGINE = MergeTree()
ORDER BY version;
"
    fi
    
    print_success "Migration system initialized"
}

# Function to create a new migration
create_migration() {
    local name="$1"
    
    if [ -z "$name" ]; then
        print_error "Migration name is required"
        echo "Usage: $0 create <migration_name>"
        exit 1
    fi
    
    local timestamp=$(date +"%Y%m%d%H%M%S")
    local version="${timestamp}_${name}"
    
    # Create PostgreSQL migration files
    if [ "$CH_ONLY" != "true" ]; then
        local pg_up_file="$MIGRATIONS_DIR/postgresql/${version}_up.sql"
        local pg_down_file="$MIGRATIONS_DIR/postgresql/${version}_down.sql"
        
        cat > "$pg_up_file" << EOF
-- HydraFlow-X PostgreSQL Migration: $name
-- Created: $(date)
-- Version: $version

-- Add your PostgreSQL migration SQL here
-- Example:
-- CREATE TABLE example_table (
--     id SERIAL PRIMARY KEY,
--     name VARCHAR(255) NOT NULL,
--     created_at TIMESTAMP WITH TIME ZONE DEFAULT NOW()
-- );

EOF
        
        cat > "$pg_down_file" << EOF
-- HydraFlow-X PostgreSQL Rollback: $name
-- Created: $(date)
-- Version: $version

-- Add your PostgreSQL rollback SQL here
-- Example:
-- DROP TABLE IF EXISTS example_table;

EOF
        
        print_success "Created PostgreSQL migration: $pg_up_file"
        print_success "Created PostgreSQL rollback: $pg_down_file"
    fi
    
    # Create ClickHouse migration files
    if [ "$PG_ONLY" != "true" ]; then
        local ch_up_file="$MIGRATIONS_DIR/clickhouse/${version}_up.sql"
        local ch_down_file="$MIGRATIONS_DIR/clickhouse/${version}_down.sql"
        
        cat > "$ch_up_file" << EOF
-- HydraFlow-X ClickHouse Migration: $name
-- Created: $(date)
-- Version: $version

-- Add your ClickHouse migration SQL here
-- Example:
-- CREATE TABLE IF NOT EXISTS example_table
-- (
--     id UInt64,
--     name String,
--     created_at DateTime DEFAULT now()
-- )
-- ENGINE = MergeTree()
-- ORDER BY id;

EOF
        
        cat > "$ch_down_file" << EOF
-- HydraFlow-X ClickHouse Rollback: $name
-- Created: $(date)
-- Version: $version

-- Add your ClickHouse rollback SQL here
-- Example:
-- DROP TABLE IF EXISTS example_table;

EOF
        
        print_success "Created ClickHouse migration: $ch_up_file"
        print_success "Created ClickHouse rollback: $ch_down_file"
    fi
    
    print_info "Next steps:"
    echo "  1. Edit the migration files to add your schema changes"
    echo "  2. Run: $0 migrate"
}

# Function to get pending migrations
get_pending_migrations() {
    local db_type="$1"
    local migrations_path="$MIGRATIONS_DIR/$db_type"
    
    if [ ! -d "$migrations_path" ]; then
        return
    fi
    
    # Get applied migrations
    local applied_migrations
    if [ "$db_type" = "postgresql" ]; then
        applied_migrations=$(PGPASSWORD="$HFX_DB_PASSWORD" psql -h "$PG_HOST" -p "$PG_PORT" -U "$PG_USER" -d "$PG_DB" -t -c "SELECT version FROM migrations.schema_migrations ORDER BY version;" 2>/dev/null | tr -d ' ')
    else
        applied_migrations=$(curl -s "http://$CH_HOST:$CH_PORT/" --data "SELECT version FROM migrations.schema_migrations ORDER BY version FORMAT TSV" 2>/dev/null)
    fi
    
    # Get all migration files
    local all_migrations=$(find "$migrations_path" -name "*_up.sql" -type f | sort | sed 's/.*\/\([0-9_a-zA-Z]*\)_up\.sql/\1/')
    
    # Find pending migrations
    local pending_migrations=""
    for migration in $all_migrations; do
        if ! echo "$applied_migrations" | grep -q "^$migration$"; then
            pending_migrations="$pending_migrations $migration"
        fi
    done
    
    echo "$pending_migrations"
}

# Function to apply a single migration
apply_migration() {
    local db_type="$1"
    local version="$2"
    local migration_file="$MIGRATIONS_DIR/$db_type/${version}_up.sql"
    
    if [ ! -f "$migration_file" ]; then
        print_error "Migration file not found: $migration_file"
        return 1
    fi
    
    print_info "Applying $db_type migration: $version"
    
    local start_time=$(date +%s%3N)
    local checksum=$(sha256sum "$migration_file" | cut -d' ' -f1)
    local name=$(echo "$version" | sed 's/^[0-9]*_//')
    
    if [ "$DRY_RUN" = "true" ]; then
        print_info "DRY RUN - Would execute:"
        cat "$migration_file"
        return 0
    fi
    
    # Execute migration
    if [ "$db_type" = "postgresql" ]; then
        if ! PGPASSWORD="$HFX_DB_PASSWORD" psql -h "$PG_HOST" -p "$PG_PORT" -U "$PG_USER" -d "$PG_DB" -f "$migration_file"; then
            print_error "Failed to apply PostgreSQL migration: $version"
            return 1
        fi
        
        # Record migration
        local end_time=$(date +%s%3N)
        local execution_time=$((end_time - start_time))
        
        PGPASSWORD="$HFX_DB_PASSWORD" psql -h "$PG_HOST" -p "$PG_PORT" -U "$PG_USER" -d "$PG_DB" -c "
            INSERT INTO migrations.schema_migrations (version, name, execution_time_ms, checksum) 
            VALUES ('$version', '$name', $execution_time, '$checksum');
        "
    else
        if ! curl -s "http://$CH_HOST:$CH_PORT/" --data-binary @"$migration_file"; then
            print_error "Failed to apply ClickHouse migration: $version"
            return 1
        fi
        
        # Record migration
        local end_time=$(date +%s%3N)
        local execution_time=$((end_time - start_time))
        
        curl -s "http://$CH_HOST:$CH_PORT/" --data "
            INSERT INTO migrations.schema_migrations (version, name, execution_time_ms, checksum) 
            VALUES ('$version', '$name', $execution_time, '$checksum');
        "
    fi
    
    print_success "Applied $db_type migration: $version (${execution_time}ms)"
}

# Function to run migrations
run_migrations() {
    print_info "Running pending migrations..."
    
    # Run PostgreSQL migrations
    if [ "$CH_ONLY" != "true" ]; then
        local pg_pending=$(get_pending_migrations "postgresql")
        if [ -n "$pg_pending" ]; then
            print_info "PostgreSQL pending migrations: $pg_pending"
            for migration in $pg_pending; do
                apply_migration "postgresql" "$migration"
            done
        else
            print_info "No pending PostgreSQL migrations"
        fi
    fi
    
    # Run ClickHouse migrations
    if [ "$PG_ONLY" != "true" ]; then
        local ch_pending=$(get_pending_migrations "clickhouse")
        if [ -n "$ch_pending" ]; then
            print_info "ClickHouse pending migrations: $ch_pending"
            for migration in $ch_pending; do
                apply_migration "clickhouse" "$migration"
            done
        else
            print_info "No pending ClickHouse migrations"
        fi
    fi
    
    print_success "All migrations completed"
}

# Function to show migration status
show_status() {
    print_info "Migration Status"
    echo "================"
    
    # PostgreSQL status
    if [ "$CH_ONLY" != "true" ]; then
        echo ""
        print_info "PostgreSQL Migrations:"
        local pg_applied=$(PGPASSWORD="$HFX_DB_PASSWORD" psql -h "$PG_HOST" -p "$PG_PORT" -U "$PG_USER" -d "$PG_DB" -c "
            SELECT version, name, applied_at, execution_time_ms || 'ms' as duration 
            FROM migrations.schema_migrations 
            ORDER BY version DESC LIMIT 10;
        " 2>/dev/null || echo "No migrations found")
        echo "$pg_applied"
        
        local pg_pending=$(get_pending_migrations "postgresql")
        if [ -n "$pg_pending" ]; then
            echo ""
            print_warning "Pending PostgreSQL migrations: $pg_pending"
        fi
    fi
    
    # ClickHouse status
    if [ "$PG_ONLY" != "true" ]; then
        echo ""
        print_info "ClickHouse Migrations:"
        local ch_applied=$(curl -s "http://$CH_HOST:$CH_PORT/" --data "
            SELECT version, name, applied_at, toString(execution_time_ms) || 'ms' as duration 
            FROM migrations.schema_migrations 
            ORDER BY version DESC LIMIT 10 
            FORMAT PrettyCompact
        " 2>/dev/null || echo "No migrations found")
        echo "$ch_applied"
        
        local ch_pending=$(get_pending_migrations "clickhouse")
        if [ -n "$ch_pending" ]; then
            echo ""
            print_warning "Pending ClickHouse migrations: $ch_pending"
        fi
    fi
}

# Parse command line arguments
COMMAND=""
PG_ONLY=false
CH_ONLY=false
DRY_RUN=false
FORCE=false

while [[ $# -gt 0 ]]; do
    case $1 in
        init|create|migrate|rollback|status|reset)
            COMMAND="$1"
            shift
            ;;
        --pg-only)
            PG_ONLY=true
            shift
            ;;
        --ch-only)
            CH_ONLY=true
            shift
            ;;
        --dry-run)
            DRY_RUN=true
            shift
            ;;
        --force)
            FORCE=true
            shift
            ;;
        --help|-h)
            show_usage
            exit 0
            ;;
        *)
            if [ -z "$COMMAND" ]; then
                print_error "Unknown command: $1"
                show_usage
                exit 1
            else
                # This is likely a parameter for the command
                break
            fi
            ;;
    esac
done

# Validate command
if [ -z "$COMMAND" ]; then
    print_error "No command specified"
    show_usage
    exit 1
fi

# Check database connections
check_connections

# Execute command
case $COMMAND in
    init)
        init_migrations
        ;;
    create)
        create_migration "$1"
        ;;
    migrate)
        run_migrations
        ;;
    status)
        show_status
        ;;
    *)
        print_error "Command '$COMMAND' not fully implemented yet"
        exit 1
        ;;
esac
