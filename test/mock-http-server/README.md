# Mock HTTP Server for painlessMesh Bridge Testing

A lightweight HTTP server that simulates Internet endpoints for testing `sendToInternet()` functionality without requiring actual Internet connectivity.

## Purpose

Testing bridge functionality in painlessMesh traditionally requires:
- Physical hardware (ESP32/ESP8266)
- Actual Internet connectivity
- Real external APIs (e.g., Callmebot WhatsApp API)

This makes automated testing slow and debugging difficult. The mock HTTP server solves this by providing a local endpoint that:

✅ **Runs locally** - No Internet required  
✅ **Fast responses** - Instant testing cycles  
✅ **Configurable** - Simulate any HTTP scenario  
✅ **Portable** - Works on any platform with Python 3  
✅ **Docker-ready** - Consistent testing environment  

## Quick Start

### Run Directly

```bash
# Start server on default port 8080
python3 server.py

# Start on custom port
python3 server.py --port 9090

# Start with specific host
python3 server.py --host 127.0.0.1 --port 8080
```

### Run with Docker

```bash
# Build and run
docker build -t painlessmesh-mock-server .
docker run -p 8080:8080 painlessmesh-mock-server

# Or use Docker Compose (from repository root)
docker-compose up mock-http-server
```

### Test the Server

```bash
# Health check
curl http://localhost:8080/health

# Get default response
curl http://localhost:8080/

# Test specific status code
curl http://localhost:8080/status/200
curl http://localhost:8080/status/404
curl http://localhost:8080/status/500

# Test delayed response (2 seconds)
curl http://localhost:8080/delay/2

# Echo request details
curl -X POST http://localhost:8080/echo \
  -H "Content-Type: application/json" \
  -d '{"test":"data"}'

# Simulate WhatsApp API
curl "http://localhost:8080/whatsapp?phone=%2B1234567890&apikey=test&text=Hello"
```

## Available Endpoints

### `GET/POST /` - Default Response
Returns a success message with timestamp.

**Example:**
```bash
curl http://localhost:8080/
```

**Response (200 OK):**
```json
{
  "message": "painlessMesh Mock HTTP Server",
  "status": "ok",
  "timestamp": 1703012345.678
}
```

### `GET/POST /status/{code}` - Specific HTTP Status
Returns the requested HTTP status code.

**Example:**
```bash
curl http://localhost:8080/status/404
```

**Response (404 Not Found):**
```json
{
  "requested_status": 404,
  "message": "Mock response with status 404",
  "timestamp": 1703012345.678
}
```

**Use Cases:**
- Test success handling (200, 201, 202, 204)
- Test client errors (400, 401, 403, 404)
- Test server errors (500, 502, 503)
- Test ambiguous responses (203 Non-Authoritative)

### `GET/POST /delay/{seconds}` - Delayed Response
Responds after specified delay (0-300 seconds).

**Example:**
```bash
curl http://localhost:8080/delay/5
```

**Response (200 OK after 5 seconds):**
```json
{
  "message": "Response delayed by 5 seconds",
  "delay": 5.0,
  "timestamp": 1703012350.678
}
```

**Use Cases:**
- Test timeout handling
- Test retry logic
- Test concurrent request handling

### `GET/POST /timeout` - Timeout Simulation
Never responds - hangs the connection for timeout testing.

**Example:**
```bash
# This will hang until client timeout
curl --max-time 5 http://localhost:8080/timeout
```

**Use Cases:**
- Test request timeout behavior
- Test retry after timeout
- Test connection cleanup

### `GET/POST /echo` - Echo Request
Returns details of the received request.

**Example:**
```bash
curl -X POST http://localhost:8080/echo \
  -H "Content-Type: application/json" \
  -d '{"sensor":"temp","value":25.5}'
```

**Response (200 OK):**
```json
{
  "method": "POST",
  "path": "/echo",
  "headers": {
    "Content-Type": "application/json",
    "Content-Length": "31"
  },
  "query_params": {},
  "body": "{\"sensor\":\"temp\",\"value\":25.5}",
  "client": "127.0.0.1",
  "timestamp": 1703012345.678
}
```

**Use Cases:**
- Verify request formatting
- Debug payload issues
- Test header propagation

### `GET/POST /whatsapp` - WhatsApp API Simulation
Simulates the Callmebot WhatsApp API.

**Required Parameters:**
- `phone` - Phone number with country code
- `apikey` - API key
- `text` - Message text

**Example:**
```bash
curl "http://localhost:8080/whatsapp?phone=%2B1234567890&apikey=mykey&text=Hello+World"
```

**Response (200 OK):**
```json
{
  "message": "WhatsApp message queued successfully",
  "phone": "+1234567890",
  "text": "Hello World",
  "timestamp": 1703012345.678
}
```

**Response (400 Bad Request if parameters missing):**
```json
{
  "error": "Missing required parameters",
  "required": ["phone", "apikey", "text"],
  "provided": ["phone", "text"]
}
```

**Use Cases:**
- Test WhatsApp integration without real API
- Test parameter validation
- Test message formatting

### `GET/POST /health` - Health Check
Returns server health status.

**Example:**
```bash
curl http://localhost:8080/health
```

**Response (200 OK):**
```json
{
  "status": "healthy",
  "server": "painlessMesh-MockServer",
  "version": "1.0.0",
  "uptime": 123.456,
  "timestamp": 1703012345.678
}
```

**Use Cases:**
- Verify server is running
- Monitor server availability
- Integration with health checks

## Configuration

### Environment Variables

```bash
# Port to listen on (default: 8080)
export MOCK_HTTP_PORT=9090

# Host to bind to (default: 0.0.0.0)
export MOCK_HTTP_HOST=127.0.0.1

# Default response delay in seconds (default: 0)
export MOCK_HTTP_DELAY=0.5

# Default HTTP status code (default: 200)
export MOCK_HTTP_STATUS=200

# Run with configuration
python3 server.py
```

### Command Line Arguments

```bash
python3 server.py --help
python3 server.py --port 9090 --host 127.0.0.1
```

## Integration with painlessMesh Tests

### Example Test Configuration

```cpp
// In your test setup, configure mesh to use mock server
#define TEST_HTTP_ENDPOINT "http://localhost:8080"

void setup() {
    // Initialize mesh
    mesh.init(...);
    
    // Send to mock server instead of real Internet
    mesh.sendToInternet(
        TEST_HTTP_ENDPOINT "/whatsapp?phone=+123&apikey=test&text=Hello",
        "",
        [](bool success, uint16_t httpStatus, String error) {
            // Verify response
            REQUIRE(success == true);
            REQUIRE(httpStatus == 200);
        }
    );
}
```

### Docker Compose Integration

Update `docker-compose.yml` in repository root:

```yaml
services:
  painlessmesh-test:
    build: .
    depends_on:
      - mock-http-server
    environment:
      - TEST_HTTP_ENDPOINT=http://mock-http-server:8080
  
  mock-http-server:
    build: ./test/mock-http-server
    ports:
      - "8080:8080"
```

## Testing Scenarios

### Test Success Cases

```bash
# 200 OK
curl http://localhost:8080/status/200

# 201 Created
curl http://localhost:8080/status/201

# 202 Accepted
curl http://localhost:8080/status/202

# 204 No Content
curl http://localhost:8080/status/204
```

### Test Failure Cases

```bash
# 203 Non-Authoritative (ambiguous - treated as failure)
curl http://localhost:8080/status/203

# 400 Bad Request
curl http://localhost:8080/status/400

# 404 Not Found
curl http://localhost:8080/status/404

# 500 Internal Server Error
curl http://localhost:8080/status/500
```

### Test Error Handling

```bash
# Timeout (connection hangs)
curl --max-time 5 http://localhost:8080/timeout

# Delayed response
curl http://localhost:8080/delay/10
```

### Test WhatsApp Integration

```bash
# Successful message
curl "http://localhost:8080/whatsapp?phone=%2B1234567890&apikey=test&text=Alarm"

# Missing parameters
curl "http://localhost:8080/whatsapp?phone=%2B1234567890&text=Alarm"
```

## Troubleshooting

### Port Already in Use

```bash
# Find process using port 8080
lsof -i :8080

# Kill the process
kill -9 <PID>

# Or use a different port
python3 server.py --port 9090
```

### Permission Denied

```bash
# Make server executable
chmod +x server.py

# Or run with python3
python3 server.py
```

### Python Not Found

```bash
# Install Python 3
sudo apt-get install python3

# Or use Docker
docker-compose up mock-http-server
```

## Development

### Adding New Endpoints

Edit `server.py` and add a new handler method:

```python
def _handle_my_endpoint(self, query_params):
    """Handle /my-endpoint"""
    response = {
        "message": "Custom endpoint response",
        "timestamp": time.time()
    }
    self._send_response(200, body=json.dumps(response))
```

Then add routing in `_handle_request()`:

```python
elif path_parts[0] == "my-endpoint":
    self._handle_my_endpoint(query_params)
```

### Running Tests

```bash
# Start server
python3 server.py &

# Test endpoints
./test_endpoints.sh

# Stop server
pkill -f server.py
```

## Performance

- **Latency:** < 1ms for local requests
- **Throughput:** 1000+ requests/second
- **Concurrency:** Threaded - handles multiple requests simultaneously
- **Memory:** < 20MB typical usage

## Requirements

- Python 3.6+
- No external dependencies (uses standard library only)

## License

Part of the painlessMesh project - see main repository LICENSE.

## Contributing

Found a bug or want to add features? Submit a PR to the main painlessMesh repository.

## See Also

- [painlessMesh Documentation](https://alteriom.github.io/painlessMesh/)
- [sendToInternet Example](../../examples/sendToInternet/)
- [Bridge Documentation](../../BRIDGE_TO_INTERNET.md)
