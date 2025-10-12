#!/usr/bin/env pwsh
# PowerShell script to run painlessMesh tests in Docker
# No local build tools required!

param(
    [Parameter(Position=0)]
    [ValidateSet("test", "build", "shell", "clean", "help")]
    [string]$Command = "test"
)

function Show-Help {
    Write-Host @"

painlessMesh Docker Test Runner
================================

Usage: .\docker-test.ps1 [command]

Commands:
  test    - Build and run all tests (default)
  build   - Build Docker image only
  shell   - Start interactive development shell
  clean   - Remove Docker containers and volumes
  help    - Show this help message

Examples:
  .\docker-test.ps1              # Run all tests
  .\docker-test.ps1 test         # Run all tests
  .\docker-test.ps1 build        # Build image
  .\docker-test.ps1 shell        # Interactive shell
  .\docker-test.ps1 clean        # Clean up

"@ -ForegroundColor Cyan
}

function Test-DockerRunning {
    try {
        docker info > $null 2>&1
        return $?
    } catch {
        return $false
    }
}

function Build-Image {
    Write-Host "`n🐳 Building Docker image..." -ForegroundColor Cyan
    Write-Host "This may take a few minutes on first run...`n" -ForegroundColor Yellow
    
    docker-compose build painlessmesh-test
    
    if ($LASTEXITCODE -eq 0) {
        Write-Host "`n✅ Docker image built successfully!" -ForegroundColor Green
        return $true
    } else {
        Write-Host "`n❌ Docker build failed!" -ForegroundColor Red
        return $false
    }
}

function Run-Tests {
    Write-Host "`n🧪 Running painlessMesh tests in Docker..." -ForegroundColor Cyan
    Write-Host "=" * 50 -ForegroundColor Gray
    
    docker-compose up --abort-on-container-exit painlessmesh-test
    
    Write-Host "`n" + ("=" * 50) -ForegroundColor Gray
    
    if ($LASTEXITCODE -eq 0) {
        Write-Host "`n✅ All tests passed!" -ForegroundColor Green
        Write-Host "   Your MQTT bridge implementation is working correctly!`n" -ForegroundColor Green
        return $true
    } else {
        Write-Host "`n❌ Some tests failed!" -ForegroundColor Red
        Write-Host "   Check the output above for details.`n" -ForegroundColor Red
        return $false
    }
}

function Start-Shell {
    Write-Host "`n🐚 Starting interactive development shell..." -ForegroundColor Cyan
    Write-Host "   Type 'exit' to return to Windows`n" -ForegroundColor Yellow
    
    docker-compose run --rm painlessmesh-dev
}

function Clean-Docker {
    Write-Host "`n🧹 Cleaning up Docker resources..." -ForegroundColor Cyan
    
    docker-compose down -v
    
    if ($LASTEXITCODE -eq 0) {
        Write-Host "`n✅ Cleanup completed!" -ForegroundColor Green
    }
}

# Main script
Write-Host @"

╔════════════════════════════════════════╗
║   painlessMesh Docker Test Runner     ║
║   MQTT Bridge Command Implementation  ║
╚════════════════════════════════════════╝

"@ -ForegroundColor Cyan

# Check if Docker is running
if (-not (Test-DockerRunning)) {
    Write-Host "❌ Docker is not running!" -ForegroundColor Red
    Write-Host "`nPlease start Docker Desktop and try again.`n" -ForegroundColor Yellow
    Write-Host "Download Docker Desktop: https://www.docker.com/products/docker-desktop/`n" -ForegroundColor Cyan
    exit 1
}

Write-Host "✅ Docker is running`n" -ForegroundColor Green

# Execute command
switch ($Command) {
    "help" {
        Show-Help
    }
    "build" {
        Build-Image
    }
    "test" {
        if (Build-Image) {
            Run-Tests
        }
    }
    "shell" {
        if (Build-Image) {
            Start-Shell
        }
    }
    "clean" {
        Clean-Docker
    }
}

exit $LASTEXITCODE
