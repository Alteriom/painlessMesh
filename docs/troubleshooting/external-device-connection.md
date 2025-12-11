# Connecting External Devices to painlessMesh Bridge

This guide explains how to connect external devices (phones, computers, test equipment) to a painlessMesh bridge node's WiFi Access Point for debugging and testing purposes.

## Overview

Each painlessMesh node operates in AP+STA mode, broadcasting a WiFi Access Point (AP) with the mesh SSID. External devices can connect to this AP, though they typically don't get internet access (unless using shared gateway mode).

## When to Connect External Devices

You might want to connect external devices to the mesh AP when:
- Debugging mesh connectivity issues
- Running diagnostic tools (ping, network scanners)
- Testing DHCP configuration
- Monitoring mesh traffic
- Developing custom mesh applications

## Connection Details

### Basic Information

| Setting | Value |
|---------|-------|
| **SSID** | Your `MESH_PREFIX` value (e.g., "FishFarmMesh", "whateverYouLike") |
| **Password** | Your `MESH_PASSWORD` value (e.g., "securepass", "somethingSneaky") |
| **Security** | WPA2-PSK |
| **IP Range** | 10.x.x.x/24 (automatically assigned via DHCP) |
| **Gateway** | 10.x.x.1 (the bridge node itself) |
| **DNS** | 10.x.x.1 (the bridge node) |

### Node-Specific IP Addressing

Each mesh node gets a unique IP address based on its Node ID:
```
IP = 10.(NodeID >> 8).(NodeID & 0xFF).1
```

Example: If Node ID is `0x1A2B`, the AP IP would be `10.26.43.1`

Connected clients receive IPs in the same subnet, typically starting from `.2`

## Connection Limits

The number of devices that can connect simultaneously depends on the platform:

| Platform | Max Connections | Notes |
|----------|----------------|-------|
| **ESP32** | 10 (default) | Configurable via `MAX_CONN` |
| **ESP8266** | 4 (default) | Configurable via `MAX_CONN` |

**Important**: Mesh nodes also count toward this limit! If 3 mesh nodes are connected to a bridge, only 7 slots remain for external devices on ESP32 (or 1 on ESP8266).

## Step-by-Step Connection Guide

### 1. Verify Bridge is Running

Check the serial output for these messages:
```
init(): Mesh channel set to X
apInit(): AP configured - SSID: YourMeshName, Channel: X, IP: 10.x.x.1
apInit(): AP active - Max connections: 10
```

### 2. Connect Your Device

#### On Android:
1. Open WiFi settings
2. Look for network with your MESH_PREFIX name
3. Enter your MESH_PASSWORD
4. Wait for connection (may take 5-10 seconds)
5. Check IP address (should be 10.x.x.x)

#### On Windows 11:
1. Click WiFi icon in system tray
2. Find network with your MESH_PREFIX name
3. Click "Connect"
4. Enter your MESH_PASSWORD
5. Open Command Prompt and run `ipconfig` to verify IP

#### On macOS:
1. Click WiFi icon in menu bar
2. Select network with your MESH_PREFIX name
3. Enter your MESH_PASSWORD
4. Open Terminal and run `ifconfig` to verify IP

#### On Linux:
1. Use NetworkManager GUI or command line:
   ```bash
   nmcli device wifi connect "FishFarmMesh" password "securepass"
   ```
2. Verify connection:
   ```bash
   ip addr show
   ```

### 3. Test Connectivity

Once connected, test basic connectivity:

```bash
# Ping the bridge/gateway
ping 10.x.x.1

# Check if you got an IP via DHCP
# Windows: ipconfig
# Linux/Mac: ifconfig or ip addr

# Try to reach other mesh nodes (if you know their IPs)
ping 10.y.y.1
```

## Troubleshooting

### Can't See the SSID

**Possible Causes:**
1. Bridge hasn't finished initializing (wait 10-15 seconds after boot)
2. Channel conflict with nearby WiFi networks
3. WiFi range issue
4. AP not properly started

**Solutions:**
1. Check serial output for "AP configured" message
2. Ensure `CONNECTION` debug level is enabled:
   ```cpp
   mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION);
   ```
3. Try power cycling the bridge
4. Check if the AP is hidden:
   ```cpp
   // In your sketch, ensure:
   mesh.init(MESH_PREFIX, MESH_PASSWORD, &scheduler, MESH_PORT, 
             WIFI_AP_STA, channel, 0);  // 0 = not hidden
   ```

### Can Connect But Don't Get IP Address

**Possible Causes:**
1. DHCP server not initialized
2. Too many devices connected (limit reached)
3. IP conflict
4. WiFi stack timing issue

**Solutions:**
1. Disconnect and reconnect after 10 seconds
2. Check serial output for connection count
3. Try rebooting the bridge node
4. Ensure you're using the latest painlessMesh version with DHCP fixes

### Connection Drops Frequently

**Possible Causes:**
1. Channel change during mesh discovery
2. Weak signal strength
3. Network congestion
4. Too many mesh topology changes

**Solutions:**
1. This is normal during initial mesh formation when channels are being discovered
2. After 30-60 seconds, the mesh should stabilize on one channel
3. Move closer to the bridge node
4. Use a fixed channel if you know your router's channel:
   ```cpp
   mesh.init(MESH_PREFIX, MESH_PASSWORD, &scheduler, MESH_PORT, 
             WIFI_AP_STA, 6);  // Force channel 6
   ```

### Can't Access Internet

**This is expected behavior!** Regular mesh nodes don't provide internet routing by default.

**Options for Internet Access:**

1. **Use Shared Gateway Mode**: All nodes connect to router
   ```cpp
   mesh.initAsSharedGateway(MESH_PREFIX, MESH_PASSWORD,
                            ROUTER_SSID, ROUTER_PASSWORD,
                            &scheduler, MESH_PORT);
   ```

2. **Connect to the Router**: Connect your device to the router WiFi instead, then communicate with mesh nodes via the bridge

3. **Custom Routing**: Implement custom NAT/routing on the bridge (advanced)

## Advanced: Using with Test Tools

### ESPping or Similar Tools

If you're using tools like ESPping (https://github.com/dvarrel/ESPping) to debug mesh connectivity:

1. Connect the test device to the mesh AP
2. You'll get an IP in the 10.x.x.x range
3. You can now ping mesh nodes directly:
   ```bash
   ping 10.x.x.1  # The bridge you're connected to
   ```
4. To find other mesh nodes, check the bridge's serial output for their IPs

### Network Scanners

Tools like `nmap`, `arp-scan`, or Android apps like "Network Analyzer" can help:

```bash
# Scan the mesh network
sudo nmap -sn 10.x.x.0/24

# Or use arp-scan
sudo arp-scan --interface=wlan0 10.x.x.0/24
```

### Packet Analysis

If you need to capture mesh traffic:

1. Connect your computer to the mesh AP
2. Use Wireshark or tcpdump to capture packets
3. Filter for TCP port 5555 (default mesh port)
   ```
   tcp.port == 5555
   ```

## Example Debug Session

Here's a complete example of connecting and debugging:

```bash
# 1. Connect to mesh AP
nmcli device wifi connect "FishFarmMesh" password "securepass"

# 2. Check your IP
ip addr show wlan0
# Should show: inet 10.26.43.2/24

# 3. Ping the gateway (bridge)
ping -c 3 10.26.43.1
# Should get replies

# 4. Check DHCP lease
cat /var/lib/NetworkManager/dhclient-*.lease
# Shows lease details from 10.26.43.1

# 5. Scan for other mesh nodes
sudo nmap -sn 10.0.0.0/8 --exclude 10.26.43.2
# May find other nodes on 10.x.x.1 addresses

# 6. Try connecting to mesh TCP port
nc -v 10.26.43.1 5555
# Should connect if node is accepting connections
```

## Security Considerations

### Important Warnings

1. **Don't use weak passwords**: The mesh password protects your entire network
2. **Change default credentials**: Always change from example values like "whateverYouLike"
3. **No internet isolation**: External devices on mesh AP can potentially communicate with all mesh nodes
4. **Production vs. Debug**: Consider disabling external connections in production:
   ```cpp
   // Limit max connections to only mesh nodes
   mesh.init(MESH_PREFIX, MESH_PASSWORD, &scheduler, MESH_PORT, 
             WIFI_AP_STA, channel, 0, 4);  // Max 4 on ESP8266
   ```

### Best Practices

1. **Use strong passwords**: At least 8 characters, mixed case, numbers
2. **Monitor connections**: Log when devices connect/disconnect
3. **Implement timeouts**: Automatically disconnect idle external devices
4. **Network segmentation**: Use VLANs if possible for mesh vs. debug traffic

## Related Documentation

- [Bridge Setup Guide](../BRIDGE_TO_INTERNET.md)
- [Shared Gateway Mode](../api/shared-gateway.md)
- [Common Issues](common-issues.md)
- [ESP32-C6 Compatibility](ESP32_C6_COMPATIBILITY.md)

## Changelog

- **Unreleased**: Initial documentation for external device connections
- Added DHCP server initialization fixes for ESP32
- Improved AP restart timing for channel changes
