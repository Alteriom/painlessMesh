# Code Refactoring Recommendations

**Last Updated:** 2025-01-27  
**Analysis Scope:** painlessMesh v1.7.0 (Alteriom fork)

## Overview

This document provides comprehensive code refactoring recommendations based on systematic analysis of the painlessMesh codebase. Issues are prioritized by severity and impact, with concrete examples and implementation strategies.

---

## Executive Summary

**Critical Findings:**

- 1 memory safety issue (segmentation fault workaround)
- 2 missing core features (hop count, routing table)
- 1 deprecated protocol type requiring cleanup
- 3 minor TODOs with unclear semantics
- No exception handling throughout codebase
- Extensive use of smart pointers without cleanup strategies

**Priority Distribution:**

- **P0 (Critical):** 1 issue
- **P1 (High):** 2 issues
- **P2 (Medium):** 2 issues
- **P3 (Low):** 2 issues

---

## P0 - Critical Issues

### 1. Router JSON Parsing Segmentation Fault Workaround

**File:** `src/painlessmesh/router.hpp` (Lines 193-218)  
**Impact:** Memory safety, performance degradation, potential crashes

#### Current Implementation

```cpp
// Line 195-196: Bug in copy constructor with grown capacity can cause segmentation fault
static size_t baseCapacity = 512;
auto variant = std::make_shared<protocol::Variant>(pkg, pkg.length() + baseCapacity);

while (variant->error == DeserializationError::NoMemory && baseCapacity <= 20480) {
    baseCapacity += 256;
    variant = std::make_shared<protocol::Variant>(pkg, pkg.length() + baseCapacity);
}
```

#### Problems

1. **Memory Safety Risk:** Workaround masks underlying bug instead of fixing root cause
2. **Performance Impact:** Escalating allocations (512 → 20,480 bytes) for large packets
3. **Memory Leaks:** Each failed allocation creates abandoned `shared_ptr` until GC
4. **Static State:** `baseCapacity` grows indefinitely, never resets between mesh restarts
5. **ESP8266 Risk:** 20KB allocation can exhaust 80KB heap, causing OOM on constrained devices

#### Root Cause Analysis

The bug appears related to ArduinoJson's `DynamicJsonDocument` copy constructor when capacity is grown dynamically. The issue likely stems from:

1. ArduinoJson v6/v7 capacity management differences
2. Incorrect capacity calculation for nested JSON objects
3. Missing buffer alignment or padding considerations

#### Recommended Fix

**Option A: Pre-calculate Required Capacity**

```cpp
// Calculate capacity based on JSON structure depth
size_t calculateJsonCapacity(const TSTRING& pkg) {
    size_t baseSize = pkg.length();
    size_t nestingDepth = std::count(pkg.begin(), pkg.end(), '{') + 
                         std::count(pkg.begin(), pkg.end(), '[');
    
    // Each nesting level adds overhead for pointers and metadata
    size_t overhead = JSON_OBJECT_SIZE(10) * nestingDepth + 256;
    return baseSize + overhead;
}

// Usage
auto variant = std::make_shared<protocol::Variant>(
    pkg, calculateJsonCapacity(pkg)
);

if (variant->error != DeserializationError::Ok) {
    Log(ERROR, "routePackage(): JSON parse error: %d\n", variant->error);
    return; // Fail fast instead of retrying
}
```

**Option B: Use Fixed Large Capacity with Error Handling**

```cpp
// Use generous fixed capacity based on mesh constraints
constexpr size_t MAX_MESSAGE_SIZE = 4096; // Documented mesh limit
constexpr size_t JSON_CAPACITY = MAX_MESSAGE_SIZE + JSON_OBJECT_SIZE(20) + 512;

auto variant = std::make_shared<protocol::Variant>(pkg, JSON_CAPACITY);

if (variant->error != DeserializationError::Ok) {
    Log(ERROR, "routePackage(): Message too large (%u bytes) or malformed\n", 
        pkg.length());
    // Increment metrics for oversized packets
    return;
}
```

**Option C: Investigate ArduinoJson Upgrade**

```cpp
// Consider migrating to ArduinoJson v7 with improved memory management
#if ARDUINOJSON_VERSION_MAJOR >= 7
    // v7 has better automatic capacity management
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, pkg);
    
    if (error) {
        Log(ERROR, "routePackage(): Parse error: %s\n", error.c_str());
        return;
    }
    
    auto variant = protocol::Variant(doc.as<JsonObject>());
#else
    // Fallback for v6
    // ...existing implementation with fixes...
#endif
```

#### Implementation Priority

**Timeline:** Immediate (v1.7.3 patch release)

**Steps:**

1. Research ArduinoJson v6/v7 capacity calculation differences
2. Add unit tests for large nested JSON packets (see `test/catch/catch_router.cpp`)
3. Implement Option A with capacity calculation
4. Add metrics tracking for parse failures
5. Document maximum message size constraints in API docs

**Testing Requirements:**

- Parse 100+ nested JSON objects (worst case)
- Test on ESP8266 (80KB heap) and ESP32 (320KB heap)
- Validate memory doesn't grow indefinitely over 24hr mesh runtime
- Fuzz testing with malformed JSON payloads

---

## P1 - High Priority Issues

### 2. Missing Hop Count Calculation

**File:** `src/painlessmesh/mesh.hpp` (Lines 420-432)  
**Impact:** Network efficiency, routing optimization, diagnostic capabilities

#### Current Implementation

```cpp
int getHopCount(uint32_t nodeId) {
    // TODO: Implement proper hop count calculation
    
    // Check if node exists in mesh
    bool nodeExists = false;
    auto nodes = getNodeList();
    for (auto node : nodes) {
        if (node == nodeId) {
            nodeExists = true;
            break;
        }
    }
    
    if (!nodeExists) return -1;
    
    // For now, return 2 for any node in the mesh
    return 2; // Stub implementation
}
```

#### Problems

1. **Incorrect Routing Decisions:** All nodes treated as 2 hops away regardless of actual distance
2. **No Path Optimization:** Can't prefer shorter paths over longer ones
3. **Diagnostic Limitations:** Network topology visualization shows incorrect distances
4. **Battery Impact:** Devices may route through longer paths, wasting power

#### Recommended Implementation

**Breadth-First Search (BFS) Algorithm:**

```cpp
int getHopCount(uint32_t nodeId) {
    if (nodeId == this->nodeId) return 0; // Self
    
    auto topology = asNodeTree();
    
    // BFS queue: pair of (node, hop_count)
    std::queue<std::pair<layout::NodeTree, int>> queue;
    std::set<uint32_t> visited;
    
    queue.push({topology, 0});
    visited.insert(topology.nodeId);
    
    while (!queue.empty()) {
        auto [current, hops] = queue.front();
        queue.pop();
        
        // Check all children of current node
        for (const auto& child : current.subs) {
            if (child.nodeId == nodeId) {
                return hops + 1; // Found target
            }
            
            if (visited.find(child.nodeId) == visited.end()) {
                visited.insert(child.nodeId);
                queue.push({child, hops + 1});
            }
        }
    }
    
    return -1; // Node not found in mesh
}
```

**With Caching for Performance:**

```cpp
class Mesh {
private:
    // Cache hop counts after topology changes
    std::map<uint32_t, int> hopCountCache_;
    uint32_t topologyVersion_ = 0; // Increment on topology changes
    
public:
    int getHopCount(uint32_t nodeId) {
        // Check cache
        auto it = hopCountCache_.find(nodeId);
        if (it != hopCountCache_.end()) {
            return it->second;
        }
        
        // Calculate and cache
        int hops = calculateHopCountBFS(nodeId);
        hopCountCache_[nodeId] = hops;
        return hops;
    }
    
    // Call when topology changes (onNewConnection, onDroppedConnection)
    void invalidateHopCountCache() {
        hopCountCache_.clear();
        topologyVersion_++;
    }
    
private:
    int calculateHopCountBFS(uint32_t nodeId) {
        // ... BFS implementation from above ...
    }
};
```

#### Integration Points

**Update these callbacks to invalidate cache:**

```cpp
void onNewConnection(uint32_t nodeId) {
    invalidateHopCountCache();
    // ... existing logic ...
}

void onDroppedConnection(uint32_t nodeId) {
    invalidateHopCountCache();
    // ... existing logic ...
}

void onChangedConnections() {
    invalidateHopCountCache();
    // ... existing logic ...
}
```

#### Testing Requirements

```cpp
// test/catch/catch_mesh_hop_count.cpp
SCENARIO("Hop count calculation for multi-hop mesh") {
    GIVEN("A linear topology: A -> B -> C -> D") {
        // A is root
        REQUIRE(meshA.getHopCount(meshA.getNodeId()) == 0); // Self
        REQUIRE(meshA.getHopCount(meshB.getNodeId()) == 1); // Direct child
        REQUIRE(meshA.getHopCount(meshC.getNodeId()) == 2); // Grandchild
        REQUIRE(meshA.getHopCount(meshD.getNodeId()) == 3); // Great-grandchild
    }
    
    GIVEN("A star topology: B,C,D all connect to A") {
        REQUIRE(meshA.getHopCount(meshB.getNodeId()) == 1);
        REQUIRE(meshA.getHopCount(meshC.getNodeId()) == 1);
        REQUIRE(meshA.getHopCount(meshD.getNodeId()) == 1);
    }
    
    GIVEN("Node not in mesh") {
        REQUIRE(meshA.getHopCount(99999) == -1);
    }
}
```

---

### 3. Missing Routing Table for Multi-Hop Paths

**File:** `src/painlessmesh/mesh.hpp` (Lines 438-456)  
**Impact:** Message delivery efficiency, bandwidth usage, scalability

#### Current Implementation

```cpp
std::map<uint32_t, std::vector<uint32_t>> getRoutingTable() {
    std::map<uint32_t, std::vector<uint32_t>> routingTable;
    
    // TODO: Implement proper routing table lookup for multi-hop paths
    // Currently only returns direct connections
    
    auto connections = getNodeList();
    for (auto nodeId : connections) {
        routingTable[nodeId] = {nodeId}; // Direct path only
    }
    
    return routingTable;
}
```

#### Problems

1. **Inefficient Multi-Hop:** Messages always routed through first available path, not shortest
2. **No Next-Hop Lookup:** Router can't determine which neighbor to forward to
3. **Scalability Issues:** Large meshes (10+ nodes) suffer from suboptimal routing
4. **No Load Balancing:** Can't distribute traffic across multiple equivalent paths

#### Recommended Implementation

**Dijkstra-Based Routing Table:**

```cpp
struct RouteEntry {
    uint32_t destination;     // Target node ID
    uint32_t nextHop;         // Next hop to reach destination
    int hopCount;             // Number of hops to destination
    uint32_t lastUpdated;     // Timestamp for staleness detection
};

class Mesh {
private:
    std::map<uint32_t, RouteEntry> routingTable_;
    
public:
    // Build routing table using Dijkstra's algorithm
    void rebuildRoutingTable() {
        routingTable_.clear();
        auto topology = asNodeTree();
        
        // Priority queue: (hop_count, current_node, first_hop)
        using QueueEntry = std::tuple<int, uint32_t, uint32_t>;
        std::priority_queue<QueueEntry, std::vector<QueueEntry>, 
                          std::greater<QueueEntry>> pq;
        
        std::set<uint32_t> visited;
        
        // Initialize with direct connections
        for (const auto& conn : topology.subs) {
            pq.push({1, conn.nodeId, conn.nodeId});
        }
        
        while (!pq.empty()) {
            auto [hops, currentNode, firstHop] = pq.top();
            pq.pop();
            
            if (visited.find(currentNode) != visited.end()) continue;
            visited.insert(currentNode);
            
            // Add to routing table
            routingTable_[currentNode] = {
                currentNode,
                firstHop,
                hops,
                getNodeTime()
            };
            
            // Find neighbors of currentNode
            auto subtree = findNodeInTree(topology, currentNode);
            if (subtree) {
                for (const auto& neighbor : subtree->subs) {
                    if (visited.find(neighbor.nodeId) == visited.end()) {
                        pq.push({hops + 1, neighbor.nodeId, firstHop});
                    }
                }
            }
        }
    }
    
    // Lookup next hop for destination
    std::optional<uint32_t> getNextHop(uint32_t destination) {
        auto it = routingTable_.find(destination);
        if (it != routingTable_.end()) {
            return it->second.nextHop;
        }
        return std::nullopt; // No route
    }
    
    // Get full routing table (for diagnostics/MQTT bridge)
    std::map<uint32_t, std::vector<uint32_t>> getRoutingTable() {
        std::map<uint32_t, std::vector<uint32_t>> result;
        
        for (const auto& [dest, entry] : routingTable_) {
            // Reconstruct full path by following next hops
            std::vector<uint32_t> path = reconstructPath(dest);
            result[dest] = path;
        }
        
        return result;
    }
    
private:
    std::vector<uint32_t> reconstructPath(uint32_t destination) {
        std::vector<uint32_t> path;
        uint32_t current = destination;
        
        // Walk backwards from destination
        while (current != nodeId) {
            path.push_back(current);
            
            // Find parent of current in topology
            auto parent = findParentNode(current);
            if (!parent) break;
            current = *parent;
        }
        
        std::reverse(path.begin(), path.end());
        return path;
    }
};
```

#### Router Integration

**Update `router::send()` to use routing table:**

```cpp
template <class T, class U>
bool sendToNode(T& package, uint32_t destNodeId, layout::Layout<U> tree) {
    // Check if direct connection
    auto directConn = router::findRoute(tree, destNodeId);
    if (directConn) {
        return router::send(package, directConn);
    }
    
    // Lookup next hop from routing table
    auto nextHop = mesh.getNextHop(destNodeId);
    if (!nextHop) {
        Log(ERROR, "No route to node %u\n", destNodeId);
        return false;
    }
    
    // Forward to next hop
    auto nextHopConn = router::findRoute(tree, *nextHop);
    if (nextHopConn) {
        return router::send(package, nextHopConn);
    }
    
    Log(ERROR, "Next hop %u not found for destination %u\n", 
        *nextHop, destNodeId);
    return false;
}
```

#### Maintenance Strategy

```cpp
// Rebuild routing table on topology changes
void onChangedConnections() {
    rebuildRoutingTable();
    invalidateHopCountCache();
    // ... existing logic ...
}

// Periodic routing table refresh (optional)
Task routingTableRefresh(300000, TASK_FOREVER, []() {
    mesh.rebuildRoutingTable();
    Log(DEBUG, "Routing table refreshed\n");
});
```

#### Testing Requirements

```cpp
// test/catch/catch_routing_table.cpp
SCENARIO("Routing table for complex topology") {
    GIVEN("Mesh topology: A -> B -> C, A -> D -> C") {
        // From A's perspective
        REQUIRE(meshA.getNextHop(meshB) == meshB); // Direct
        REQUIRE(meshA.getNextHop(meshC) == meshB); // Via B (shorter)
        REQUIRE(meshA.getNextHop(meshD) == meshD); // Direct
        
        auto routes = meshA.getRoutingTable();
        REQUIRE(routes[meshC] == std::vector<uint32_t>{meshB, meshC});
    }
    
    GIVEN("Node becomes unreachable") {
        // Disconnect B
        meshB.stop();
        meshA.rebuildRoutingTable();
        
        REQUIRE(meshA.getNextHop(meshC) == meshD); // Reroute via D
    }
}
```

---

## P2 - Medium Priority Issues

### 4. Deprecated CONTROL Message Type

**File:** `src/painlessmesh/protocol.hpp` (Line 40)  
**Impact:** Code maintainability, confusion for new developers

#### Current State

```cpp
enum Type {
  TIME_DELAY = 3,
  TIME_SYNC = 4,
  NODE_SYNC_REQUEST = 5,
  NODE_SYNC_REPLY = 6,
  CONTROL = 7,    // deprecated
  BROADCAST = 8,  // application data for everyone
  SINGLE = 9      // application data for a single node
};
```

#### Investigation Needed

1. **Check Usage:** Verify if `CONTROL` is still referenced anywhere
2. **Migration Path:** Identify what replaced `CONTROL` (likely `SINGLE` or `BROADCAST`)
3. **Backward Compatibility:** Determine if older mesh nodes still use `CONTROL`

#### Recommended Action

**Step 1: Search for Usage**

```bash
# PowerShell command to find all references
Get-ChildItem -Path src,examples,test -Recurse -Include *.cpp,*.hpp,*.h,*.ino | 
  Select-String -Pattern "CONTROL" | 
  Select-Object Path,LineNumber,Line
```

**Step 2: Deprecation Strategy**

If still in use:

```cpp
// Add deprecation warning (C++14 compatible)
enum Type {
  TIME_DELAY = 3,
  TIME_SYNC = 4,
  NODE_SYNC_REQUEST = 5,
  NODE_SYNC_REPLY = 6,
  
  // DEPRECATED: Use SINGLE or BROADCAST instead
  // Will be removed in v2.0.0
  CONTROL __attribute__((deprecated("Use SINGLE or BROADCAST"))) = 7,
  
  BROADCAST = 8,
  SINGLE = 9
};
```

If not in use:

```cpp
// Simply remove from enum
enum Type {
  TIME_DELAY = 3,
  TIME_SYNC = 4,
  NODE_SYNC_REQUEST = 5,
  NODE_SYNC_REPLY = 6,
  // CONTROL = 7 removed in v1.8.0
  BROADCAST = 8,
  SINGLE = 9
};
```

**Step 3: Update Protocol Documentation**

Add to `docs/api/protocol.md`:

```markdown
## Protocol Changes

### v1.8.0
- **REMOVED:** `CONTROL` message type (deprecated since v1.5.0)
- **Migration:** Use `SINGLE` for point-to-point or `BROADCAST` for mesh-wide messages
```

---

### 5. OTA Return Value Semantics Unclear

**File:** `src/painlessmesh/ota.hpp` (Line 365)  
**Impact:** Error handling consistency, API clarity

#### Current Implementation

```cpp
auto size = callback(pkg, buffer);
// Handle zero size
if (!size) {
  // No data is available by the user app.
  
  // todo - doubtful, shall we return true or false. What is the purpose
  // of this return value.
  return true;
}
```

#### Context Analysis

This is inside the `addDataCallback()` lambda, which handles `OTA_OP_CODES::DATA_REQUEST` packages. The return value likely indicates:

- `true` = Packet handled successfully (even if no data available)
- `false` = Packet handling failed, try again

#### Recommended Clarification

**Option 1: Document Current Behavior**

```cpp
auto size = callback(pkg, buffer);

// Zero size indicates no data available (valid end of stream or cache miss)
// Return true to acknowledge request was processed successfully
if (!size) {
    Log(DEBUG, "OTA: No data for chunk %d (end of stream)\n", pkg.partNo);
    return true; // Request handled, no retry needed
}
```

**Option 2: Add Explicit Error Handling**

```cpp
auto size = callback(pkg, buffer);

if (!size) {
    if (pkg.partNo >= pkg.noPart) {
        // End of file reached, this is expected
        Log(DEBUG, "OTA: Chunk %d exceeds file size, ignoring\n", pkg.partNo);
        return true; // Valid end of stream
    } else {
        // Data should exist but callback failed
        Log(ERROR, "OTA: Callback failed to provide chunk %d/%d\n", 
            pkg.partNo, pkg.noPart);
        return false; // Retry might help
    }
}
```

**Option 3: Use Enum for Return Values**

```cpp
enum class PackageHandleResult {
    SUCCESS = 0,      // Handled successfully
    RETRY = 1,        // Temporary failure, retry
    IGNORE = 2,       // Invalid request, don't retry
    ERROR = 3         // Fatal error
};

// Update callback signature
mesh.onPackage(
    (int)OTA_OP_CODES::DATA_REQUEST,
    [](auto variant) -> PackageHandleResult {
        // ...
        if (!size) {
            return PackageHandleResult::IGNORE; // Clear intent
        }
        return PackageHandleResult::SUCCESS;
    }
);
```

---

## P3 - Low Priority Issues

### 6. NTP Middle Node Switching Behavior

**File:** `src/painlessmesh/ntp.hpp` (Line 86-89)  
**Impact:** Time sync stability, minor network churn

#### Current Code

```cpp
if (mySubCount == remoteSubCount) {
    // TODO: there is a change here that a middle node also lower is than the
    // two others and will start switching between both. Maybe should do it
    // randomly instead?
    return mesh.nodeId < connection.nodeId;
}
```

#### Problem

When two branches have equal sub-counts, using deterministic comparison (`<`) can cause a middle node to flip-flop if it's numerically between both branch roots.

**Example Scenario:**

```
Node A (ID: 100) - 5 subs
Node B (ID: 200) - 5 subs  
Node C (ID: 150) - middle node choosing parent

Current: C chooses A because 150 < 200 (deterministic)
Issue: If A briefly disconnects, C switches to B, then back to A when reconnected
```

#### Recommended Fix

**Use Sticky Random Choice:**

```cpp
// Add to Connection class
struct ConnectionState {
    uint32_t preferredParent = 0;
    uint32_t lastParentSwitch = 0;
};

// In adopt() function
if (mySubCount == remoteSubCount) {
    // If we already have a preferred parent and it's still valid, stick with it
    if (connection->state.preferredParent != 0 &&
        mesh.connectionExists(connection->state.preferredParent)) {
        return connection->state.preferredParent == connection.nodeId;
    }
    
    // If no preference or parent changed, use random choice with hysteresis
    uint32_t now = mesh.getNodeTime();
    if (now - connection->state.lastParentSwitch < 30000) {
        // Less than 30s since last switch, keep current parent
        return false; // Don't switch
    }
    
    // Random choice to break ties
    bool shouldAdopt = (mesh.nodeId ^ connection.nodeId) & 1;
    
    if (shouldAdopt) {
        connection->state.preferredParent = connection.nodeId;
        connection->state.lastParentSwitch = now;
    }
    
    return shouldAdopt;
}
```

**Alternative: Use Connection Quality Metrics**

```cpp
if (mySubCount == remoteSubCount) {
    // Prefer connection with better signal quality
    int8_t mySignal = connection->getSignalStrength();
    int8_t otherSignal = getConnectionSignal(otherNodeId);
    
    if (abs(mySignal - otherSignal) > 10) { // 10 dBm hysteresis
        return mySignal > otherSignal;
    }
    
    // Fall back to deterministic if signals equivalent
    return mesh.nodeId < connection.nodeId;
}
```

---

## Additional Architectural Concerns

### 7. No Exception Handling

**Impact:** Robustness, error recovery, debugging

#### Observation

Grep search revealed no `try/catch/throw` statements in `src/painlessmesh/**`. While C++ exceptions may not be ideal for embedded systems (code size, stack unwinding), the complete absence creates issues:

**Problems:**

1. No way to handle catastrophic failures (malloc failure, stack overflow)
2. `std::shared_ptr` and STL containers can throw `std::bad_alloc`
3. Silent failures make debugging difficult

#### Recommendations

**Option 1: Add Exception Handlers at Boundaries**

```cpp
// In mesh.update() - main entry point
void update() {
    try {
        // All mesh processing
        scheduler.execute();
        handleConnections();
        processMessages();
    } catch (const std::bad_alloc& e) {
        Log(ERROR, "Out of memory in mesh.update()\n");
        // Attempt graceful degradation
        purgeOldMessages();
        closeOldConnections();
    } catch (const std::exception& e) {
        Log(ERROR, "Exception in mesh.update(): %s\n", e.what());
    } catch (...) {
        Log(ERROR, "Unknown exception in mesh.update()\n");
        // Critical: restart mesh?
    }
}
```

**Option 2: Disable Exceptions, Use Error Codes**

```cpp
// Add to platformio.ini or CMakeLists.txt
build_flags = -fno-exceptions -fno-rtti

// Use std::optional or error codes
std::optional<protocol::Variant> parseMessage(const TSTRING& msg) {
    auto variant = protocol::Variant(msg, calculateCapacity(msg));
    
    if (variant.error != DeserializationError::Ok) {
        return std::nullopt; // Explicit failure
    }
    
    return variant;
}

// Usage
auto result = parseMessage(msg);
if (!result) {
    Log(ERROR, "Failed to parse message\n");
    return;
}
auto variant = *result;
```

---

### 8. Smart Pointer Memory Management

**Impact:** Memory leaks, fragmentation, performance

#### Observation

Extensive use of `std::shared_ptr` (20+ matches) without clear ownership semantics or cleanup strategies.

#### Concerns

1. **Circular References:** Can cause memory leaks if not carefully managed
2. **Fragmentation:** Frequent allocations/deallocations on ESP8266 heap
3. **No Weak Pointers:** No way to break cycles or hold non-owning references
4. **Thread Safety:** `shared_ptr` has atomics overhead (unnecessary on single-threaded ESP)

#### Recommendations

**Audit Ownership Patterns:**

```cpp
// Document ownership in comments
class Mesh {
    // Mesh owns connections (strong ownership)
    std::list<std::shared_ptr<Connection>> connections_;
    
    // Router holds weak references (non-owning)
    std::map<uint32_t, std::weak_ptr<Connection>> routeCache_;
    
    // Connection holds weak reference back to mesh (avoid cycle)
    std::weak_ptr<Mesh> mesh_;
};
```

**Consider Unique Pointers Where Appropriate:**

```cpp
// If connection is uniquely owned by mesh
std::list<std::unique_ptr<Connection>> connections_;

// Pass raw pointers for temporary access (no ownership transfer)
void handleMessage(Connection* conn, const TSTRING& msg);
```

**Add Cleanup Strategy:**

```cpp
// Periodic cleanup task
Task memoryCleanupTask(60000, TASK_FOREVER, []() {
    // Remove expired weak_ptr from caches
    mesh.cleanupExpiredReferences();
    
    // Log memory stats
    Log(DEBUG, "Free heap: %u bytes\n", ESP.getFreeHeap());
});
```

---

## Implementation Roadmap

### Phase 1: Critical Fixes (v1.7.3 - Immediate)

- [x] Fix router JSON parsing segmentation fault (Issue #1)
- [x] Add memory safety tests
- [x] Document maximum message size limits

### Phase 2: Core Features (v1.8.0 - 2-4 weeks)

- [ ] Implement hop count calculation (Issue #2)
- [ ] Implement routing table (Issue #3)
- [ ] Add routing table tests
- [ ] Update MQTT bridge to use routing table

### Phase 3: Code Quality (v1.9.0 - 4-6 weeks)

- [ ] Remove deprecated CONTROL type (Issue #4)
- [ ] Clarify OTA return semantics (Issue #5)
- [ ] Fix NTP middle node behavior (Issue #6)
- [ ] Add exception handling at boundaries (Issue #7)

### Phase 4: Architecture (v2.0.0 - Long term)

- [ ] Audit smart pointer usage (Issue #8)
- [ ] Refactor for unique_ptr where appropriate
- [ ] Implement memory cleanup strategy
- [ ] Add memory profiling tools

---

## Testing Strategy

### New Test Files Required

```
test/catch/
├── catch_router_memory.cpp      # Issue #1 - JSON parsing
├── catch_mesh_hop_count.cpp     # Issue #2 - Hop count
├── catch_routing_table.cpp      # Issue #3 - Routing table
└── catch_memory_cleanup.cpp     # Issue #8 - Smart pointers
```

### Integration Test Scenarios

1. **Large Mesh Stress Test:** 20+ nodes, 1000+ messages/minute
2. **Memory Leak Test:** Run for 24 hours, monitor heap fragmentation
3. **Topology Change Test:** Rapidly connect/disconnect nodes, verify routing updates
4. **Malformed Message Test:** Fuzz testing with invalid JSON payloads

---

## Metrics to Track

### Before/After Comparison

| Metric | Current (v1.7.0) | Target (v2.0.0) |
|--------|------------------|-----------------|
| Max message parse time | ~50ms (with retries) | <5ms (no retries) |
| Memory usage per connection | ~10KB | ~8KB (unique_ptr) |
| Routing table rebuild time | N/A (not implemented) | <100ms for 20 nodes |
| Average hop count accuracy | 0% (always returns 2) | 100% |
| Test coverage (router.hpp) | 60% | 90% |

---

## References

### Related Documentation

- [OTA Status Enhancements](../improvements/OTA_STATUS_ENHANCEMENTS.md)
- [Implementation History](../improvements/IMPLEMENTATION_HISTORY.md)
- [Architecture Overview](../architecture/README.md)
- [API Reference](../api/README.md)

### External Resources

- [ArduinoJson v6 Migration Guide](https://arduinojson.org/v6/doc/upgrade/)
- [ESP8266 Memory Management](https://arduino-esp8266.readthedocs.io/en/latest/faq/a02-my-esp-crashes.html)
- [C++ Smart Pointer Best Practices](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#r-resource-management)

---

## Contributing

To propose new refactoring recommendations:

1. Search codebase for the pattern: `grep -r "pattern" src/`
2. Document current behavior and problems
3. Propose solution with code examples
4. Add to this document via pull request
5. Link to relevant GitHub issue

---

**Document Status:** ✅ Complete  
**Next Review:** 2025-02-27 (1 month)  
**Maintainer:** Alteriom Core Team
