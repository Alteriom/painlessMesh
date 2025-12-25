# Mock HTTP Server - Quick Start Guide

## What is This?

A lightweight HTTP server that lets you test painlessMesh's `sendToInternet()` functionality **without actual Internet access**. No more waiting for slow external APIs or debugging network issues!

## Why Use It?

- 🚀 **50-100x faster** - Instant responses instead of 5-10 second waits
- 💻 **Offline development** - No Internet connection required
- 🎯 **Reproducible tests** - Control all scenarios precisely
- 🤖 **CI/CD ready** - Automated testing enabled

## Quick Start (30 seconds)

### 1. Start the Server

```bash
cd test/mock-http-server
python3 server.py
```

You'll see:
```
============================================================
painlessMesh Mock HTTP Server
============================================================
Listening on: http://0.0.0.0:8080
...
```

### 2. Test It

```bash
# In another terminal
curl http://localhost:8080/health
```

You should get:
```json
{
    "status": "healthy",
    "server": "painlessMesh-MockServer",
    ...
}
```

### 3. Use in Your ESP Code

```cpp
#define MOCK_SERVER_IP "192.168.1.100"  // Your computer's IP
String url = String("http://") + MOCK_SERVER_IP + ":8080/status/200";

mesh.sendToInternet(url, "", [](bool success, uint16_t status, String error) {
    Serial.printf("Result: %s (HTTP %d)\n", success ? "OK" : "FAILED", status);
});
```

## Common Test Scenarios

```bash
# Test success
curl http://localhost:8080/status/200

# Test errors  
curl http://localhost:8080/status/404
curl http://localhost:8080/status/500

# Test WhatsApp API
curl "http://localhost:8080/whatsapp?phone=%2B123&apikey=test&text=Hello"

# Test delayed response (2 seconds)
curl http://localhost:8080/delay/2
```

## Full Documentation

- 📖 [Complete Server Guide](README.md) - All endpoints and configuration
- 🔧 [Integration Guide](TESTING_GUIDE.md) - ESP32/ESP8266 examples
- 💡 [Example Sketch](../../examples/sendToInternet/mock_server_test.ino) - Working test code

## Docker Version

```bash
# From repository root
docker-compose up mock-http-server
```

Access at `http://localhost:8080`

## Troubleshooting

**"Connection refused"**
- Check server is running: `curl http://localhost:8080/health`
- Verify your computer's IP address
- Ensure ESP and computer are on same network

**"No such file or directory"**
- Make sure you're in the correct directory
- Python 3 required: `python3 --version`

## What's Next?

1. ✅ Read the [Complete Guide](README.md) for all features
2. ✅ Try the [Example Sketch](../../examples/sendToInternet/mock_server_test.ino)
3. ✅ Set up [Docker Integration](README.md#docker-setup) for CI/CD

---

**Need help?** Check [TESTING_GUIDE.md](TESTING_GUIDE.md) or open an issue on GitHub.
