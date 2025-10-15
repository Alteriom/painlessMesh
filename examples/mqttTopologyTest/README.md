# MQTT Topology Test

Hardware test sketch for painlessMesh topology reporting with @alteriom/mqtt-schema v0.5.0 compliance.

## 📋 Requirements

### Hardware
- **3x ESP32 boards** (or ESP8266, but ESP32 recommended for better performance)
- **3x USB cables** (for programming and Serial monitoring)
- **MQTT broker** (e.g., Mosquitto, HiveMQ, or cloud service)
- **WiFi network** accessible by all devices

### Software
- Arduino IDE 2.x or PlatformIO
- painlessMesh library (this repository)
- PubSubClient library (for MQTT)
- ArduinoJson library (v7.x)

## 🔧 Configuration

### 1. Configure WiFi (Gateway Only)

Edit these lines in `mqttTopologyTest.ino`:

```cpp
#define WIFI_SSID       "YourWiFiSSID"
#define WIFI_PASSWORD   "YourWiFiPassword"
```

### 2. Configure MQTT Broker (Gateway Only)

```cpp
#define MQTT_BROKER     "192.168.1.100"  // Your MQTT broker IP
#define MQTT_PORT       1883
#define MQTT_USER       ""  // Leave empty if no auth
#define MQTT_PASS       ""  // Leave empty if no auth
```

### 3. Configure Each Node

**Node 1 (Gateway):**
```cpp
#define IS_GATEWAY      true
#define NODE_ROLE       "gateway"
```

**Node 2 (Sensor):**
```cpp
#define IS_GATEWAY      false
#define NODE_ROLE       "sensor"
```

**Node 3 (Repeater):**
```cpp
#define IS_GATEWAY      false
#define NODE_ROLE       "repeater"
```

## 🚀 Installation & Testing

### Step 1: Setup MQTT Broker

**Option A: Local Mosquitto (Recommended for testing)**
```bash
# Install Mosquitto
sudo apt-get install mosquitto mosquitto-clients

# Start broker
sudo systemctl start mosquitto

# Test broker
mosquitto_sub -h localhost -t "alteriom/mesh/#" -v
```

**Option B: Docker Mosquitto**
```bash
docker run -d -p 1883:1883 -p 9001:9001 eclipse-mosquitto
```

**Option C: Cloud MQTT** (HiveMQ, CloudMQTT, etc.)

### Step 2: Flash Nodes

1. **Flash Node 1 (Gateway)**
   - Set `IS_GATEWAY = true`, `NODE_ROLE = "gateway"`
   - Upload sketch
   - Open Serial Monitor (115200 baud)

2. **Flash Node 2 (Sensor)**
   - Set `IS_GATEWAY = false`, `NODE_ROLE = "sensor"`
   - Upload sketch
   - Open Serial Monitor

3. **Flash Node 3 (Repeater)**
   - Set `IS_GATEWAY = false`, `NODE_ROLE = "repeater"`
   - Upload sketch
   - Open Serial Monitor

### Step 3: Monitor MQTT Messages

**Terminal 1: Monitor all mesh topics**
```bash
mosquitto_sub -h localhost -t "alteriom/mesh/#" -v
```

**Terminal 2: Monitor topology only**
```bash
mosquitto_sub -h localhost -t "alteriom/mesh/+/topology" -v
```

**Terminal 3: Monitor events only**
```bash
mosquitto_sub -h localhost -t "alteriom/mesh/+/events" -v
```

### Step 4: Test GET_TOPOLOGY Command

**Send command:**
```bash
mosquitto_pub -h localhost -t "alteriom/mesh/MESH-001/command" -m '{
  "event": "command",
  "command": "get_topology",
  "correlation_id": "test-001",
  "parameters": {
    "format": "full",
    "include_metrics": true
  }
}'
```

**Expected response on** `alteriom/mesh/MESH-001/topology/response`:
```json
{
  "schema_version": 1,
  "device_id": "ALT-...",
  "event": "command_response",
  "command": "get_topology",
  "correlation_id": "test-001",
  "success": true,
  "result": {
    "event": "mesh_topology",
    "nodes": [...],
    "connections": [...],
    "metrics": {...}
  },
  "latency_ms": 850
}
```

## 📊 Expected Behavior

### Mesh Formation (0-30 seconds)
```
✅ Node 2 joins mesh
✅ Node 3 joins mesh
🔄 Connections established
⏰ Time synchronized
```

### Full Topology Publishing (every 60 seconds)
```json
{
  "event": "mesh_topology",
  "mesh_id": "MESH-001",
  "gateway_node_id": "ALT-6825DD341CA4",
  "update_type": "full",
  "nodes": [
    {
      "node_id": "ALT-6825DD341CA4",
      "role": "gateway",
      "status": "online",
      "connection_count": 2
    },
    {
      "node_id": "ALT-441D64F804A0",
      "role": "sensor",
      "status": "online"
    },
    {
      "node_id": "ALT-7F8E9D0A1B2C",
      "role": "repeater",
      "status": "online"
    }
  ],
  "connections": [
    {
      "from_node": "ALT-6825DD341CA4",
      "to_node": "ALT-441D64F804A0",
      "quality": 95,
      "latency_ms": 12,
      "rssi": -42,
      "hop_count": 1
    },
    {
      "from_node": "ALT-6825DD341CA4",
      "to_node": "ALT-7F8E9D0A1B2C",
      "quality": 88,
      "latency_ms": 18,
      "rssi": -55,
      "hop_count": 1
    }
  ],
  "metrics": {
    "total_nodes": 3,
    "online_nodes": 3,
    "network_diameter": 1,
    "avg_connection_quality": 91
  }
}
```

### Incremental Updates (within 5 seconds of change)

**When Node 4 joins:**
```json
{
  "event": "mesh_topology",
  "update_type": "incremental",
  "nodes": [
    {
      "node_id": "ALT-NEW12345678",
      "role": "sensor",
      "status": "online"
    }
  ],
  "connections": [
    {
      "from_node": "ALT-6825DD341CA4",
      "to_node": "ALT-NEW12345678",
      "quality": 78
    }
  ]
}
```

**Followed by event:**
```json
{
  "event": "mesh_event",
  "event_type": "node_join",
  "affected_nodes": ["ALT-NEW12345678"]
}
```

### Node Leave Event

**When node disconnects:**
```json
{
  "event": "mesh_event",
  "event_type": "node_leave",
  "affected_nodes": ["ALT-441D64F804A0"],
  "details": {
    "reason": "timeout",
    "last_seen": "2025-10-14T15:08:45Z",
    "connections_lost": 1
  }
}
```

## 🧪 Test Scenarios

### Test 1: Mesh Formation
1. Power on all 3 nodes
2. Watch Serial monitors for mesh formation
3. Verify full topology published within 60 seconds
4. ✅ **Pass:** All nodes appear in topology with correct roles

### Test 2: Incremental Updates
1. With mesh running, power off Node 3
2. Watch for `node_leave` event within 5 seconds
3. Verify incremental topology update shows Node 3 as offline
4. Power on Node 3 again
5. Watch for `node_join` event
6. ✅ **Pass:** Events published within 5 seconds of change

### Test 3: GET_TOPOLOGY Command
1. Send `get_topology` command via MQTT
2. Verify response on `/topology/response` topic
3. Check response includes `correlation_id`
4. Verify `latency_ms` is reasonable (< 2000ms)
5. ✅ **Pass:** Response received with correct structure

### Test 4: Schema Compliance
1. Copy any published message
2. Validate against @alteriom/mqtt-schema v0.5.0
3. Check all required envelope fields present
4. Verify Device ID format: `ALT-XXXXXXXXXXXX` (uppercase hex)
5. Verify quality values: 0-100 range
6. Verify RSSI values: negative dBm
7. ✅ **Pass:** All messages validate successfully

### Test 5: Quality Metrics
1. Move nodes physically closer/farther apart
2. Watch quality values change in topology updates
3. Verify RSSI becomes more negative with distance
4. Verify latency increases with poor connections
5. ✅ **Pass:** Metrics reflect connection quality

### Test 6: Network Diameter
1. Start with 2 nodes (gateway + 1 sensor)
2. Add 3rd node in range of sensor but not gateway
3. Verify `network_diameter` increases to 2
4. Verify `hop_count` shows multi-hop connections
5. ✅ **Pass:** Topology correctly represents mesh structure

## 🐛 Troubleshooting

### Issue: Nodes won't join mesh

**Symptoms:**
- Serial shows "Connecting to mesh..."
- No "New connection" messages

**Solutions:**
1. Verify all nodes have same `MESH_PREFIX` and `MESH_PASSWORD`
2. Check nodes are within WiFi range (< 30m indoors)
3. Try changing `MESH_PORT` to avoid conflicts
4. Disable WiFi on gateway temporarily to test sensor-to-sensor mesh

### Issue: Gateway won't connect to MQTT

**Symptoms:**
- "Connecting to MQTT broker... failed, rc=-2"

**Solutions:**
1. Verify `MQTT_BROKER` IP is correct
2. Check broker is running: `mosquitto -v`
3. Test broker connectivity: `mosquitto_pub -h <IP> -t test -m hello`
4. Check firewall allows port 1883
5. If using authentication, verify `MQTT_USER` and `MQTT_PASS`

### Issue: Topology messages not publishing

**Symptoms:**
- Mesh formed but no MQTT messages
- Serial shows "Failed to publish"

**Solutions:**
1. Check MQTT buffer size (should be 2048+)
2. Verify topic permissions on broker
3. Check JSON message size (< 2KB recommended)
4. Monitor broker logs for errors
5. Reduce `FULL_TOPOLOGY_INTERVAL` to 10s for testing

### Issue: Device ID format incorrect

**Symptoms:**
- Device IDs like "ALT-0" or "ALT-12345"

**Solutions:**
1. Verify using ESP32 (ESP8266 has different chip ID)
2. Check `getDeviceId()` function uses `%012llX` format
3. Confirm uppercase hex output
4. Test: Should be 16 characters total (ALT- + 12 hex digits)

### Issue: Quality metrics always 0 or 100

**Symptoms:**
- Quality stuck at boundary values
- RSSI always -90 or 0

**Solutions:**
1. Implement real RSSI measurement (this test uses simulation)
2. Add WiFi signal strength reading: `WiFi.RSSI()`
3. Calculate quality from packet loss, not simulation
4. Add latency measurement with ping/pong messages

### Issue: Incremental updates too slow

**Symptoms:**
- Node joins but update takes > 10 seconds

**Solutions:**
1. Reduce `INCREMENTAL_INTERVAL` to 1000ms for testing
2. Check `lastTopologyUpdate` is being reset on connection changes
3. Verify callbacks are triggering: add Serial.println in `newConnectionCallback`
4. Confirm MQTT client is connected when change occurs

## 📈 Performance Optimization

### For Large Meshes (10+ nodes)

1. **Increase MQTT buffer:**
   ```cpp
   mqttClient.setBufferSize(4096);
   ```

2. **Reduce full topology frequency:**
   ```cpp
   #define FULL_TOPOLOGY_INTERVAL  300000  // 5 minutes
   ```

3. **Implement topology compression:**
   - Only send changed fields in incremental updates
   - Use shorter field names
   - Remove optional fields when empty

4. **Add QoS levels:**
   ```cpp
   mqttClient.publish(topic.c_str(), jsonString.c_str(), true);  // Retained
   ```

5. **Batch incremental updates:**
   - Wait 10s for multiple changes
   - Send single update with all changes

### For ESP8266 (Limited Memory)

1. **Reduce JSON buffer size:**
   ```cpp
   JsonDocument doc;  // Uses 1KB by default
   ```

2. **Stream JSON directly to MQTT:**
   ```cpp
   // Instead of String, use streaming
   mqttClient.beginPublish(topic.c_str(), measureJson(doc), false);
   serializeJson(doc, mqttClient);
   mqttClient.endPublish();
   ```

3. **Disable debug messages:**
   ```cpp
   mesh.setDebugMsgTypes(ERROR);
   ```

## 📚 Schema Reference

This test implements these schemas from @alteriom/mqtt-schema v0.5.0:

- ✅ **Envelope** (schema_version, device_id, device_type, timestamp, firmware_version)
- ✅ **mesh_topology** (full and incremental updates)
- ✅ **mesh_event** (node_join, node_leave events)
- ✅ **command** (GET_TOPOLOGY command ID 300)
- ✅ **command_response** (topology response with correlation_id)

## 🎯 Success Criteria

Test is successful when:

- [x] All 3 nodes join mesh within 30 seconds
- [x] Full topology published every 60 seconds
- [x] Incremental updates published within 5 seconds of changes
- [x] Node join/leave events published immediately
- [x] GET_TOPOLOGY command returns valid response
- [x] All messages validate against @alteriom/mqtt-schema
- [x] Device IDs follow ALT-XXXXXXXXXXXX format
- [x] Quality metrics in 0-100 range
- [x] RSSI values are negative
- [x] Connection count matches actual mesh connections

## 📝 Notes

- This test uses **simulated quality metrics** (RSSI, latency) based on node IDs
- For production, implement **real measurements** using `WiFi.RSSI()` and ping/pong
- The test uses **simplified topology** (all nodes connect to gateway)
- For multi-hop meshes, implement **hop count tracking** from routing table
- Message timing is **approximate** due to mesh synchronization delays
- **Schema compliance** is validated by test/catch/catch_topology_schema.cpp

## 🔗 Related Documentation

- [MQTT Schema Review](../../docs/MQTT_SCHEMA_REVIEW.md)
- [MQTT Bridge Commands](../../docs/MQTT_BRIDGE_COMMANDS.md)
- [Schema Validation Tests](../../test/catch/catch_topology_schema.cpp)
- [@alteriom/mqtt-schema Package](https://www.npmjs.com/package/@alteriom/mqtt-schema)
