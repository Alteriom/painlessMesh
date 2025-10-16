# Docker Build & Test Scripts for painlessMesh

## Quick Start

### Prerequisites
- Install Docker Desktop for Windows: https://www.docker.com/products/docker-desktop/
- Ensure Docker is running (check system tray)

### Run Tests (Automated)

```powershell
# Build and run all tests
docker-compose up painlessmesh-test

# Or using plain Docker
docker build -t painlessmesh-test .
docker run --rm -v ${PWD}:/workspace painlessmesh-test
```

### Interactive Development Shell

```powershell
# Start interactive shell
docker-compose run --rm painlessmesh-dev

# Inside the container, you can:
cmake -G Ninja .
ninja
./bin/catch_mqtt_bridge
./bin/catch_alteriom_packages
# etc.
```

## Available Commands

### Build Docker Image
```powershell
docker-compose build
```

### Run Specific Test
```powershell
docker-compose run --rm painlessmesh-test ./bin/catch_mqtt_bridge
```

### Run All Tests
```powershell
docker-compose up painlessmesh-test
```

### Clean Build Artifacts
```powershell
docker-compose down -v
```

### Rebuild from Scratch
```powershell
docker-compose build --no-cache
docker-compose up painlessmesh-test
```

## CI/CD Integration

### GitHub Actions Example

```yaml
name: Test with Docker

on: [push, pull_request]

jobs:
  test:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
      
      - name: Build and Test
        run: |
          docker build -t painlessmesh-test .
          docker run --rm painlessmesh-test
```

### Local PowerShell Script

Create `test.ps1`:
```powershell
#!/usr/bin/env pwsh
Write-Host "Building Docker image..." -ForegroundColor Cyan
docker build -t painlessmesh-test .

Write-Host "`nRunning tests..." -ForegroundColor Cyan
docker run --rm painlessmesh-test

if ($LASTEXITCODE -eq 0) {
    Write-Host "`n✅ All tests passed!" -ForegroundColor Green
} else {
    Write-Host "`n❌ Tests failed!" -ForegroundColor Red
    exit 1
}
```

Run with: `.\test.ps1`

## Troubleshooting

### Docker Not Running
```
Error: Cannot connect to the Docker daemon
```
**Solution:** Start Docker Desktop from Windows Start Menu

### Port Conflicts
If you see port binding errors, stop conflicting containers:
```powershell
docker ps
docker stop <container_id>
```

### Disk Space Issues
Clean up unused images and containers:
```powershell
docker system prune -a
```

### Build Cache Issues
Force rebuild without cache:
```powershell
docker-compose build --no-cache
```

## What Gets Tested

The Docker environment runs all Catch2 tests:
- ✅ `catch_mqtt_bridge` - MQTT command routing and parameter parsing
- ✅ `catch_alteriom_packages` - Alteriom package serialization
- ✅ `catch_protocol` - Protocol validation
- ✅ `catch_router` - Mesh routing logic
- ✅ `catch_plugin` - Plugin system
- ✅ And all other `catch_*.cpp` tests

## Benefits of Docker Testing

1. **Consistent Environment** - Same results on all machines
2. **No Local Installation** - No need for CMake, Ninja, compilers
3. **CI/CD Ready** - Same container in development and CI
4. **Isolated** - Doesn't affect your Windows installation
5. **Reproducible** - Anyone can run tests identically

## Integration with VS Code

Install the "Docker" extension, then you can:
- Right-click `Dockerfile` → "Build Image"
- Right-click `docker-compose.yml` → "Compose Up"
- View logs in VS Code terminal

## Performance Tips

### Use BuildKit for Faster Builds
```powershell
$env:DOCKER_BUILDKIT=1
docker build -t painlessmesh-test .
```

### Mount Source as Volume (Faster Iteration)
```powershell
docker run --rm -v ${PWD}:/workspace painlessmesh-test
```

This way, changes to source files don't require rebuilding the image.

## Advanced Usage

### Run Single Test File
```powershell
docker-compose run --rm painlessmesh-test bash -c "cmake -G Ninja . && ninja && ./bin/catch_mqtt_bridge"
```

### Debug Build
```powershell
docker-compose run --rm painlessmesh-dev
# Inside container:
cmake -G Ninja -DCMAKE_BUILD_TYPE=Debug .
ninja
gdb ./bin/catch_mqtt_bridge
```

### Check for Memory Leaks (with Valgrind)
Add to Dockerfile:
```dockerfile
RUN apt-get update && apt-get install -y valgrind
```

Then run:
```powershell
docker-compose run --rm painlessmesh-test valgrind ./bin/catch_mqtt_bridge
```
