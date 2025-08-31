# HydraFlow-X Windows Installation Script
# Supports Windows 10/11 with PowerShell 5.1+

param(
    [string]$InstallDir = "C:\HydraFlow",
    [string]$Version = "latest",
    [switch]$Development = $false,
    [switch]$SkipDocker = $false,
    [switch]$UserInstall = $false,
    [switch]$Help = $false
)

# Configuration
$ErrorActionPreference = "Stop"
$ProgressPreference = "SilentlyContinue"

# Colors for output
function Write-ColorOutput {
    param(
        [string]$Message,
        [string]$Color = "White"
    )
    
    $colorMap = @{
        "Red" = [ConsoleColor]::Red
        "Green" = [ConsoleColor]::Green
        "Yellow" = [ConsoleColor]::Yellow
        "Blue" = [ConsoleColor]::Blue
        "White" = [ConsoleColor]::White
    }
    
    Write-Host $Message -ForegroundColor $colorMap[$Color]
}

function Write-Info {
    param([string]$Message)
    Write-ColorOutput "[INFO] $Message" -Color "Blue"
}

function Write-Success {
    param([string]$Message)
    Write-ColorOutput "[SUCCESS] $Message" -Color "Green"
}

function Write-Warning {
    param([string]$Message)
    Write-ColorOutput "[WARNING] $Message" -Color "Yellow"
}

function Write-Error {
    param([string]$Message)
    Write-ColorOutput "[ERROR] $Message" -Color "Red"
}

# Function to show help
function Show-Help {
    Write-Host "HydraFlow-X Windows Installation Script"
    Write-Host "======================================"
    Write-Host ""
    Write-Host "Usage: .\install.ps1 [OPTIONS]"
    Write-Host ""
    Write-Host "Options:"
    Write-Host "  -InstallDir DIR    Custom installation directory (default: C:\HydraFlow)"
    Write-Host "  -Version VERSION   Install specific version (default: latest)"
    Write-Host "  -Development       Install development version"
    Write-Host "  -SkipDocker        Skip Docker Desktop installation"
    Write-Host "  -UserInstall       Install in user directory"
    Write-Host "  -Help              Show this help message"
    Write-Host ""
    Write-Host "Examples:"
    Write-Host "  .\install.ps1"
    Write-Host "  .\install.ps1 -UserInstall -SkipDocker"
    Write-Host "  .\install.ps1 -InstallDir D:\Trading\HydraFlow"
}

# Function to check if running as administrator
function Test-Administrator {
    $currentUser = [Security.Principal.WindowsIdentity]::GetCurrent()
    $principal = New-Object Security.Principal.WindowsPrincipal($currentUser)
    return $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
}

# Function to check if Chocolatey is installed
function Test-Chocolatey {
    return (Get-Command choco -ErrorAction SilentlyContinue) -ne $null
}

# Function to install Chocolatey
function Install-Chocolatey {
    Write-Info "Installing Chocolatey package manager..."
    
    Set-ExecutionPolicy Bypass -Scope Process -Force
    [System.Net.ServicePointManager]::SecurityProtocol = [System.Net.ServicePointManager]::SecurityProtocol -bor 3072
    
    Invoke-Expression ((New-Object System.Net.WebClient).DownloadString('https://community.chocolatey.org/install.ps1'))
    
    # Refresh environment variables
    $env:Path = [System.Environment]::GetEnvironmentVariable("Path","Machine") + ";" + [System.Environment]::GetEnvironmentVariable("Path","User")
    
    Write-Success "Chocolatey installed successfully"
}

# Function to install dependencies
function Install-Dependencies {
    Write-Info "Installing dependencies..."
    
    # Check if Chocolatey is installed
    if (-not (Test-Chocolatey)) {
        Install-Chocolatey
    }
    
    # Install required packages
    $packages = @(
        "git",
        "cmake",
        "ninja",
        "llvm",
        "nodejs",
        "postgresql",
        "redis-64",
        "curl",
        "jq"
    )
    
    foreach ($package in $packages) {
        Write-Info "Installing $package..."
        try {
            choco install $package -y --no-progress
        }
        catch {
            Write-Warning "Failed to install $package, continuing..."
        }
    }
    
    # Install Visual Studio Build Tools if not present
    if (-not (Get-Command cl -ErrorAction SilentlyContinue)) {
        Write-Info "Installing Visual Studio Build Tools..."
        choco install visualstudio2022buildtools --package-parameters "--add Microsoft.VisualStudio.Workload.VCTools" -y
    }
    
    Write-Success "Dependencies installed successfully"
}

# Function to install Docker Desktop
function Install-Docker {
    if ($SkipDocker) {
        Write-Info "Skipping Docker installation"
        return
    }
    
    Write-Info "Installing Docker Desktop..."
    
    # Check if Docker is already installed
    if (Get-Command docker -ErrorAction SilentlyContinue) {
        Write-Success "Docker already installed"
        return
    }
    
    try {
        choco install docker-desktop -y
        Write-Success "Docker Desktop installed successfully"
        Write-Warning "Please restart your computer to complete Docker installation"
    }
    catch {
        Write-Warning "Failed to install Docker Desktop automatically"
        Write-Info "Please download and install Docker Desktop from https://www.docker.com/products/docker-desktop"
    }
}

# Function to create directory structure
function New-DirectoryStructure {
    Write-Info "Creating directory structure..."
    
    if ($UserInstall) {
        $InstallDir = Join-Path $env:USERPROFILE "HydraFlow"
    }
    
    $directories = @(
        $InstallDir,
        "$InstallDir\bin",
        "$InstallDir\config",
        "$InstallDir\logs",
        "$InstallDir\data",
        "$InstallDir\backups",
        "$InstallDir\scripts"
    )
    
    foreach ($dir in $directories) {
        if (-not (Test-Path $dir)) {
            New-Item -ItemType Directory -Path $dir -Force | Out-Null
        }
    }
    
    Write-Success "Directories created at $InstallDir"
    return $InstallDir
}

# Function to download and build HydraFlow-X
function Install-HydraFlow {
    param([string]$InstallPath)
    
    Write-Info "Downloading and building HydraFlow-X..."
    
    $tempDir = New-TemporaryFile | ForEach-Object { Remove-Item $_; New-Item -ItemType Directory -Path $_ }
    
    try {
        Set-Location $tempDir
        
        # Clone repository
        if ($Version -eq "latest") {
            git clone https://github.com/yourusername/HydraFlow-X.git
        }
        else {
            git clone --branch $Version https://github.com/yourusername/HydraFlow-X.git
        }
        
        Set-Location "HydraFlow-X"
        
        # Create build directory
        New-Item -ItemType Directory -Path "build" -Force | Out-Null
        Set-Location "build"
        
        # Configure CMake
        $cmakeArgs = @(
            "-G", "Ninja",
            "-DCMAKE_CXX_STANDARD=23",
            ".."
        )
        
        if ($Development) {
            $cmakeArgs += "-DCMAKE_BUILD_TYPE=Debug"
        }
        else {
            $cmakeArgs += "-DCMAKE_BUILD_TYPE=Release"
        }
        
        & cmake @cmakeArgs
        
        if ($LASTEXITCODE -ne 0) {
            throw "CMake configuration failed"
        }
        
        # Build the project
        & ninja
        
        if ($LASTEXITCODE -ne 0) {
            throw "Build failed"
        }
        
        # Install binary
        Copy-Item "hydraflow-x.exe" "$InstallPath\bin\" -Force
        
        # Copy configuration templates
        $configSource = Join-Path (Split-Path $PWD) "config"
        if (Test-Path $configSource) {
            Copy-Item "$configSource\*" "$InstallPath\config\" -Recurse -Force
        }
        
        Copy-Item (Join-Path (Split-Path $PWD) "config.env.example") "$InstallPath\config\" -Force
        
        # Copy scripts
        $scriptsSource = Join-Path (Split-Path $PWD) "scripts"
        if (Test-Path $scriptsSource) {
            Copy-Item "$scriptsSource\*" "$InstallPath\scripts\" -Recurse -Force
        }
        
        Write-Success "HydraFlow-X installed successfully"
    }
    finally {
        # Cleanup
        Set-Location $env:TEMP
        Remove-Item $tempDir -Recurse -Force -ErrorAction SilentlyContinue
    }
}

# Function to create Windows service
function New-WindowsService {
    param([string]$InstallPath)
    
    if ($UserInstall) {
        Write-Info "Skipping Windows service creation for user installation"
        return
    }
    
    Write-Info "Creating Windows service..."
    
    $serviceName = "HydraFlow-X"
    $serviceDisplayName = "HydraFlow-X Ultra-Low Latency Trading Platform"
    $serviceDescription = "Ultra-low latency algorithmic trading platform for cryptocurrency markets"
    $servicePath = "$InstallPath\bin\hydraflow-x.exe"
    $serviceArgs = "--config `"$InstallPath\config\hydraflow.json`""
    
    # Check if service already exists
    $existingService = Get-Service -Name $serviceName -ErrorAction SilentlyContinue
    if ($existingService) {
        Write-Info "Service already exists, removing old service..."
        Stop-Service -Name $serviceName -Force -ErrorAction SilentlyContinue
        & sc.exe delete $serviceName
        Start-Sleep 2
    }
    
    # Create new service
    & sc.exe create $serviceName binPath= "`"$servicePath`" $serviceArgs" DisplayName= $serviceDisplayName start= auto
    & sc.exe description $serviceName $serviceDescription
    
    # Configure service recovery
    & sc.exe failure $serviceName reset= 86400 actions= restart/60000/restart/60000/restart/60000
    
    Write-Success "Windows service created successfully"
}

# Function to create configuration files
function New-Configuration {
    param([string]$InstallPath)
    
    Write-Info "Creating configuration files..."
    
    $configFile = "$InstallPath\config\hydraflow.json"
    $envFile = "$InstallPath\config\.env"
    
    # Copy example configuration if it doesn't exist
    if (-not (Test-Path $configFile)) {
        $exampleConfig = "$InstallPath\config\hydraflow.example.json"
        if (Test-Path $exampleConfig) {
            Copy-Item $exampleConfig $configFile
            Write-Info "Created configuration file: $configFile"
        }
    }
    
    # Create environment file if it doesn't exist
    if (-not (Test-Path $envFile)) {
        $exampleEnv = "$InstallPath\config\config.env.example"
        if (Test-Path $exampleEnv) {
            Copy-Item $exampleEnv $envFile
            Write-Info "Created environment file: $envFile"
        }
    }
    
    Write-Warning "Please edit $configFile and $envFile with your API keys and configuration"
}

# Function to add to PATH
function Add-ToPath {
    param([string]$InstallPath)
    
    $binPath = "$InstallPath\bin"
    $currentPath = [Environment]::GetEnvironmentVariable("Path", "User")
    
    if ($currentPath -notlike "*$binPath*") {
        $newPath = "$currentPath;$binPath"
        [Environment]::SetEnvironmentVariable("Path", $newPath, "User")
        Write-Success "Added $binPath to user PATH"
    }
}

# Function to show post-installation information
function Show-PostInstallInfo {
    param([string]$InstallPath)
    
    Write-Success "🎉 HydraFlow-X installation completed successfully!"
    Write-Host ""
    Write-Info "Installation directory: $InstallPath"
    Write-Info "Configuration files: $InstallPath\config\"
    Write-Info "Log files: $InstallPath\logs\"
    Write-Host ""
    Write-Info "Next steps:"
    Write-Host "  1. Edit configuration: $InstallPath\config\hydraflow.json"
    Write-Host "  2. Set API keys: $InstallPath\config\.env"
    Write-Host "  3. Start the service:"
    
    if ($UserInstall) {
        Write-Host "     $InstallPath\bin\hydraflow-x.exe --config `"$InstallPath\config\hydraflow.json`""
    }
    else {
        Write-Host "     Start-Service HydraFlow-X"
        Write-Host "     Get-Service HydraFlow-X"
    }
    
    Write-Host ""
    Write-Info "Web Dashboard: http://localhost:8080"
    Write-Info "Metrics: http://localhost:9090"
    Write-Host ""
    
    if (-not $SkipDocker) {
        Write-Info "Docker Compose alternative:"
        Write-Host "  cd $InstallPath"
        Write-Host "  docker-compose up -d"
    }
    
    Write-Host ""
    Write-Warning "⚠️  Remember to:"
    Write-Host "  - Configure your API keys before starting"
    Write-Host "  - Set up your database (PostgreSQL)"
    Write-Host "  - Configure Windows Firewall if needed"
    Write-Host "  - Review security settings"
    Write-Host ""
    Write-Info "For support, visit: https://github.com/yourusername/HydraFlow-X"
}

# Main installation function
function main {
    Write-Host "🚀 HydraFlow-X Windows Installation Script"
    Write-Host "========================================"
    Write-Host ""
    
    if ($Help) {
        Show-Help
        return
    }
    
    # Check if running as administrator (if not user install)
    if (-not $UserInstall -and -not (Test-Administrator)) {
        Write-Error "This script needs to be run as Administrator or use -UserInstall switch"
        Write-Info "Right-click PowerShell and select 'Run as Administrator'"
        return
    }
    
    try {
        # Install dependencies
        Install-Dependencies
        
        # Install Docker
        Install-Docker
        
        # Create directories
        $finalInstallDir = New-DirectoryStructure
        
        # Install HydraFlow-X
        Install-HydraFlow -InstallPath $finalInstallDir
        
        # Create Windows service (if not user install)
        if (-not $UserInstall) {
            New-WindowsService -InstallPath $finalInstallDir
        }
        
        # Create configuration
        New-Configuration -InstallPath $finalInstallDir
        
        # Add to PATH
        Add-ToPath -InstallPath $finalInstallDir
        
        # Show post-installation information
        Show-PostInstallInfo -InstallPath $finalInstallDir
    }
    catch {
        Write-Error "Installation failed: $($_.Exception.Message)"
        Write-Info "Please check the error above and try again"
        exit 1
    }
}

# Run main function
main
