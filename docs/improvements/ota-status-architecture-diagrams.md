# OTA and Status Architecture Diagrams

Visual reference for understanding the different implementation options.

---

## OTA Distribution Architectures

### Current Implementation (Sequential)

```
┌──────────────┐
│ Sender Node  │
│  (Root/SD)   │
└──────┬───────┘
       │ Announce broadcast every 60s
       │
       ▼
┌──────────────────────────────────────┐
│     All Nodes Receive Announce      │
└──┬───────┬───────┬───────┬──────────┘
   │       │       │       │
   ▼       ▼       ▼       ▼
┌─────┐ ┌─────┐ ┌─────┐ ┌─────┐
│Node1│ │Node2│ │Node3│ │Node4│
└──┬──┘ └──┬──┘ └──┬──┘ └──┬──┘
   │       │       │       │
   │ Request chunk 0
   ├──────────────────────────────────►
   │       │       │       │
   ◄───────┤       │       │
   │ Data chunk 0  │       │
   │       │       │       │
   │ Request chunk 1       │
   ├──────────────────────────────────►
   │       │       │       │
   ◄───────┤       │       │
   │ Data chunk 1  │       │
   │       │       │       │
   │       │ Request chunk 0
   │       ├──────────────────────────►
   │       │       │       │
   │       ◄───────┤       │
   │       │ Data chunk 0  │
   └───────┴───────┴───────┴───────────
   
Time: 60-120s for 4 nodes
Network: N * Firmware_Size (each node gets full copy)
```

---

### Option 1A: Mesh-Wide Broadcast

```
┌──────────────┐
│ Sender Node  │
│  (Root/SD)   │
└──────┬───────┘
       │
       │ Broadcast: Announce
       ├─────────────────────────────────┐
       │                                 │
       │ Broadcast: Chunk 0 (1s delay)  │
       ├─────────────────────────────────┤
       │                                 │
       │ Broadcast: Chunk 1 (1s delay)  │
       ├─────────────────────────────────┤
       │                                 │
       │ Broadcast: Chunk N (1s delay)  │
       ├─────────────────────────────────┤
       │                                 │
       ▼                                 ▼
┌────────────────────────────────────────────┐
│    All Nodes Receive All Broadcasts       │
│  (Assemble chunks, handle out-of-order)   │
└┬─────────┬─────────┬─────────┬────────────┘
 │         │         │         │
 ▼         ▼         ▼         ▼
┌────┐   ┌────┐   ┌────┐   ┌────┐
│Node│   │Node│   │Node│   │Node│
│ 1  │   │ 2  │   │ 3  │   │ 4  │
└─┬──┘   └─┬──┘   └─┬──┘   └─┬──┘
  │        │        │        │
  │ NAK: Missing chunk 5      │
  ├───────────────────────────┤
  │        │        │        │
  │ Broadcast: Chunk 5 resend │
  ◄────────┴────────┴─────────┘
  
Time: ~30s for any number of nodes
Network: 1 * Firmware_Size (broadcast to all)
Memory: +2-5KB per node (chunk tracking)
```

---

### Option 1B: Progressive Rollout

```
Phase 1: Canary (5-10 minutes)
┌──────────┐
│  Sender  │
└────┬─────┘
     │
     │ OTA to 1-2 test nodes
     ├──────────────┐
     │              │
     ▼              ▼
   ┌────┐        ┌────┐
   │ N1 │        │ N2 │
   └─┬──┘        └─┬──┘
     │              │
     │ Monitor for errors/crashes
     └──────┬───────┘
            │
            ▼
     [Health Check]
            │
            ├─ OK ──► Phase 2
            │
            └─ FAIL ─► ABORT & ALERT

Phase 2: Early Adopters (10-20% of nodes)
┌──────────┐
│  Sender  │
└────┬─────┘
     │
     │ OTA to 20% of remaining nodes
     ├─────┬─────┬─────┬─────┐
     │     │     │     │     │
     ▼     ▼     ▼     ▼     ▼
   ┌──┐  ┌──┐  ┌──┐  ┌──┐  ┌──┐
   │N3│  │N4│  │N5│  │N6│  │N7│
   └┬─┘  └┬─┘  └┬─┘  └┬─┘  └┬─┘
    │     │     │     │     │
    │ Monitor for stability
    └─────┴─────┴─────┴─────┘
            │
            ▼
     [Health Check]
            │
            ├─ OK ──► Phase 3
            │
            └─ FAIL ─► ROLLBACK

Phase 3: Full Rollout (All remaining)
┌──────────┐
│  Sender  │
└────┬─────┘
     │
     │ OTA to all remaining nodes
     │
     ▼
[All Other Nodes]
     │
     ▼
   [Complete]

Time: 30-60 minutes (safe but slow)
Safety: High (early failure detection)
```

---

### Option 1C: Peer-to-Peer Distribution

```
Initial Wave:
┌──────────────┐
│ Sender Node  │
│   (Source)   │
└──────┬───────┘
       │
       │ Direct OTA to first 3 nodes
       ├───────┬───────┬───────┐
       │       │       │       │
       ▼       ▼       ▼       ▼
     ┌───┐   ┌───┐   ┌───┐
     │ A │   │ B │   │ C │
     └─┬─┘   └─┬─┘   └─┬─┘
       │       │       │
       │ Cache firmware in flash
       │       │       │
       
Second Wave (exponential growth):
┌────────┬────────┬────────┬────────┐
│        │        │        │        │
▼        ▼        ▼        ▼        ▼
Sender   A        B        C
│        │        │        │
├───┬────┤        │        │
│   │    │        │        │
▼   ▼    ▼        ▼        ▼
D   E    F        G        H
                           
Third Wave:
D, E, F, G, H all become sources...
│  │  │  │  │
▼  ▼  ▼  ▼  ▼
I  J  K  L  M, etc.

Time: ~15s for large mesh (exponential)
Memory: +200-500KB flash (cached firmware)
Scalability: Excellent (viral propagation)
```

---

### Option 1E: Compressed OTA Transfer

```
Build Process:
┌──────────────┐
│firmware.bin  │
│   (200KB)    │
└──────┬───────┘
       │
       │ gzip -9
       ▼
┌──────────────┐
│firmware.bin.gz│
│   (~100KB)   │  40-60% size reduction
└──────┬───────┘
       │
       │ Split into chunks
       ▼
       
Distribution (same topology as current):
┌──────────────┐
│ Sender Node  │
└──────┬───────┘
       │
       │ Announce (with compression flag)
       │
       ▼
     Node receives compressed chunks
       │
       │ ┌─────────────────┐
       │ │ Decompression   │
       │ │ Buffer (8KB)    │
       ├─┤                 │
       │ │ Streaming       │
       │ │ decompress to   │
       │ │ flash           │
       │ └─────────────────┘
       ▼
     Flash Write
     
Time Saved: 40-60% (less data to transfer)
Memory: +4-8KB (decompression buffer)
CPU: Minimal (streaming decompression)
Compatibility: Works with all distribution methods
```

---

## Mesh Status Monitoring Architectures

### Current State (Manual)

```
Application Code:
┌───────────────────────────┐
│ User Application          │
│                           │
│ Manually poll nodes:      │
│ - getNodeList()           │
│ - subConnectionJson()     │
│ - getNodeTime()           │
│                           │
│ Store and process data    │
└───────────────────────────┘
       │
       │ Multiple API calls
       │
       ▼
┌───────────────────────────┐
│  painlessMesh Library     │
└───────────────────────────┘
       │
       ▼
   [Mesh Nodes]
   
Manual status collection
No standardization
Application-specific
```

---

### Option 2A: Enhanced StatusPackage

```
Each Node (periodic broadcast):
┌──────────────────────────┐
│  Node Application        │
└──────┬───────────────────┘
       │
       │ Create status (every 60s)
       ▼
┌──────────────────────────┐
│ EnhancedStatusPackage    │
│ - uptime                 │
│ - memory                 │
│ - message stats          │
│ - network info           │
│ - alerts                 │
└──────┬───────────────────┘
       │
       │ mesh.sendBroadcast()
       ▼
┌──────────────────────────┐
│     Mesh Network         │
└──────┬───────────────────┘
       │
       │ All nodes receive
       ▼
Collection Node:
┌──────────────────────────┐
│  onReceive() callback    │
│                          │
│  if (type == 202) {      │
│    processStatus()       │
│    storeMetrics()        │
│    checkAlerts()         │
│  }                       │
└──────────────────────────┘

Frequency: 30-60s typical
Overhead: ~500 bytes per update
Integration: Simple, builds on existing
```

---

### Option 2B: Mesh Status Service

```
Root/Bridge Node initiates:
┌──────────────────────────┐
│  Root Node               │
│  (Status Collector)      │
└──────┬───────────────────┘
       │
       │ StatusQuery(node_list)
       ├────────┬────────┬────────┐
       │        │        │        │
       ▼        ▼        ▼        ▼
     ┌───┐    ┌───┐    ┌───┐    ┌───┐
     │ A │    │ B │    │ C │    │ D │
     └─┬─┘    └─┬─┘    └─┬─┘    └─┬─┘
       │        │        │        │
       │ StatusReport (metrics)
       ├────────┼────────┼────────┤
       │        │        │        │
       ▼        ▼        ▼        ▼
┌──────────────────────────────────┐
│  Root Node Aggregation           │
│                                  │
│  {                               │
│    "nodeA": {...},               │
│    "nodeB": {...},               │
│    "nodeC": {...},               │
│    "nodeD": {...}                │
│  }                               │
└──────┬───────────────────────────┘
       │
       │ Publish via MQTT or API
       ▼
┌──────────────────────────────────┐
│  External Monitoring System      │
└──────────────────────────────────┘

Query Mode: On-demand or periodic
Aggregation: Centralized at root
Response Timeout: 5-10 seconds
```

---

### Option 2C: Telemetry Stream

```
Continuous streaming from all nodes:

Node 1:     Node 2:     Node 3:     Node 4:
  │           │           │           │
  │ Telemetry (every 60s) │           │
  ├───────────┴───────────┴───────────┤
  │                                   │
  ▼                                   ▼
┌─────────────────────────────────────────┐
│          Root Node                      │
│      (State Maintenance)                │
│                                         │
│  ┌───────────────────────────────────┐ │
│  │  Internal State Model             │ │
│  │  - Node 1: [metrics]              │ │
│  │  - Node 2: [metrics]              │ │
│  │  - Node 3: [metrics]              │ │
│  │  - Node 4: [metrics]              │ │
│  └───────────────────────────────────┘ │
│                                         │
│  ┌───────────────────────────────────┐ │
│  │  Anomaly Detection                │ │
│  │  - Memory critically low on N2    │ │
│  │  - High packet loss on N3         │ │
│  └───────────────────────────────────┘ │
└─────────┬───────────────────────────────┘
          │
          │ Export alerts & metrics
          ▼
┌─────────────────────────────────────────┐
│   Time-Series Database / MQTT           │
│   (InfluxDB, Prometheus, etc.)          │
└─────────────────────────────────────────┘

Format: Compact binary (64 bytes)
Encoding: Delta (only changes)
Frequency: 30-60s
Overhead: Very low (~1KB/hour per node)
```

---

### Option 2D: Health Dashboard

```
┌─────────────────────────────────────────┐
│       Dashboard Node (ESP32)            │
│                                         │
│  ┌───────────────────────────────────┐ │
│  │  Status Collector Service         │ │
│  │  (Polls all nodes every 30s)      │ │
│  └───────┬───────────────────────────┘ │
│          │                             │
│  ┌───────▼───────────────────────────┐ │
│  │  LittleFS Storage                 │ │
│  │  - Current status                 │ │
│  │  - Historical data (24h)          │ │
│  │  - Alert history                  │ │
│  └───────┬───────────────────────────┘ │
│          │                             │
│  ┌───────▼───────────────────────────┐ │
│  │  AsyncWebServer                   │ │
│  │  - REST API                       │ │
│  │  - WebSocket (live updates)       │ │
│  │  - Static files (HTML/CSS/JS)     │ │
│  └───────┬───────────────────────────┘ │
└──────────┼─────────────────────────────┘
           │
           │ HTTP/WebSocket
           ▼
┌─────────────────────────────────────────┐
│        Web Browser / Mobile             │
│                                         │
│  ┌─────────────────────────────────┐   │
│  │  Real-Time Mesh Visualization   │   │
│  │  ┌─────┐  ┌─────┐  ┌─────┐     │   │
│  │  │  A  ├──┤  B  ├──┤  C  │     │   │
│  │  └─────┘  └─────┘  └──┬──┘     │   │
│  │                       │         │   │
│  │                    ┌──┴──┐      │   │
│  │                    │  D  │      │   │
│  │                    └─────┘      │   │
│  └─────────────────────────────────┘   │
│                                         │
│  ┌─────────────────────────────────┐   │
│  │  Per-Node Metrics               │   │
│  │  - Memory: [====>    ] 60%      │   │
│  │  - Uptime: 2d 14h               │   │
│  │  - Messages: 12,345             │   │
│  └─────────────────────────────────┘   │
│                                         │
│  ┌─────────────────────────────────┐   │
│  │  Alerts & Events                │   │
│  │  ⚠ Node 3: Low memory           │   │
│  │  ⚠ Node 7: High packet loss     │   │
│  └─────────────────────────────────┘   │
└─────────────────────────────────────────┘

Features:
- Live topology visualization
- Historical graphs (ChartJS)
- Alert management
- Firmware tracking
- Mobile responsive
```

---

### Option 2E: MQTT Status Bridge

```
Mesh Network:
┌────┐  ┌────┐  ┌────┐  ┌────┐
│ N1 │  │ N2 │  │ N3 │  │ N4 │
└─┬──┘  └─┬──┘  └─┬──┘  └─┬──┘
  │       │       │       │
  │   Mesh communication   │
  └───────┴───────┴────────┘
          │
          │ Status collection
          ▼
┌─────────────────────────────┐
│   Bridge Node (Root)        │
│                             │
│  ┌──────────────────────┐  │
│  │ Status Collector     │  │
│  │ - Polls mesh nodes   │  │
│  │ - Aggregates data    │  │
│  └──────┬───────────────┘  │
│         │                  │
│  ┌──────▼───────────────┐  │
│  │ MQTT Publisher       │  │
│  │ - JSON formatting    │  │
│  │ - Topic routing      │  │
│  └──────┬───────────────┘  │
└─────────┼───────────────────┘
          │
          │ MQTT/TLS
          ▼
┌─────────────────────────────┐
│     MQTT Broker             │
│   (Mosquitto, HiveMQ)       │
└─────────┬───────────────────┘
          │
          │ Topics:
          │ - mesh/status/nodes
          │ - mesh/status/topology
          │ - mesh/status/metrics
          │ - mesh/status/alerts
          │ - mesh/status/node/{id}
          │
          ├──────┬──────┬──────┬──────┐
          ▼      ▼      ▼      ▼      ▼
      ┌─────┐ ┌────┐ ┌────┐ ┌────┐ ┌────┐
      │Grafa│ │Prom│ │Home│ │Node│ │Cust│
      │ na  │ │eth │ │Asst│ │RED │ │ om │
      └─────┘ └────┘ └────┘ └────┘ └────┘

JSON Payload Example:
{
  "timestamp": 1638360000,
  "nodes": [
    {
      "id": 123456789,
      "uptime": 86400,
      "memory": 45000,
      "rssi": -65,
      "version": "v2.1.0"
    },
    ...
  ]
}
```

---

## Comparison: OTA Performance

### Time to Update 20 Nodes (200KB firmware)

```
Current (Sequential):
[▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓] 20 min

+ Compression (1E):
[▓▓▓▓▓▓▓▓▓▓▓▓] 12 min (40% faster)

+ Broadcast (1A):
[▓▓▓▓▓] 5 min (75% faster)

+ Broadcast + Compression:
[▓▓▓] 3 min (85% faster)

+ Peer-to-Peer (1C):
[▓] 1 min (95% faster)
```

### Network Bandwidth Usage

```
Current: 20 nodes × 200KB = 4000KB total

Broadcast: 1 × 200KB = 200KB total
(20x reduction!)

Compressed: 20 × 100KB = 2000KB
(or 1 × 100KB = 100KB if broadcast)

Peer-to-Peer: ~400KB total
(viral propagation, shared load)
```

---

## Comparison: Status Monitoring Overhead

### Network Traffic (10 nodes, 1 hour)

```
Option 2A (StatusPackage @ 5min):
│█│█│█│█│█│█│█│█│█│█│█│ ~60KB/hour
(12 updates × 500B × 10 nodes)

Option 2B (Status Service @ 30s):
│███│ ~15KB/hour
(Queries + responses, aggregated)

Option 2C (Telemetry @ 60s):
│█│ ~4KB/hour
(Delta encoding, very efficient)

Option 2D (Dashboard - polling @ 30s):
│██│ ~10KB/hour
(Collector queries nodes)

Option 2E (MQTT Bridge @ 30s):
│███│ ~20KB/hour
(JSON overhead, but external)
```

---

## Decision Tree

```
                  ┌─────────────────┐
                  │  Do you need    │
                  │  OTA or Status? │
                  └────┬────┬───────┘
                       │    │
           ┌───────────┘    └──────────┐
           │                           │
           ▼                           ▼
    ┌─────────────┐            ┌─────────────┐
    │     OTA     │            │   STATUS    │
    └──────┬──────┘            └──────┬──────┘
           │                          │
           ▼                          ▼
    What's your         How many nodes?
    priority?                  │
           │                   │
    ┌──────┼──────┐            ├──────┬──────┐
    ▼      ▼      ▼            ▼      ▼      ▼
  Speed  Safety  Simple    <10    10-50   >50
    │      │      │          │      │      │
    ▼      ▼      ▼          ▼      ▼      ▼
  1A+1E  1B+1E   1E        2A     2E    2C+2E
```

---

**See also:**
- [Full Proposal](ota-and-status-enhancements.md)
- [Quick Reference](ota-status-quick-reference.md)
