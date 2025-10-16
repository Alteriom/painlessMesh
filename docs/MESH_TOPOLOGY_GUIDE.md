# Mesh Topology Visualization Guide

**Version:** 1.0.0  
**Date:** October 14, 2025  
**Schema:** @alteriom/mqtt-schema v0.5.0

A comprehensive guide for building web dashboards and monitoring tools to visualize painlessMesh network topology using MQTT.

---

## Table of Contents

1. [Quick Start](#quick-start)
2. [D3.js Force-Directed Graph](#d3js-force-directed-graph)
3. [Cytoscape.js Network View](#cytoscapejs-network-view)
4. [Real-Time Dashboard](#real-time-dashboard)
5. [Python Monitoring](#python-monitoring)
6. [Node-RED Flows](#node-red-flows)
7. [Troubleshooting](#troubleshooting)
8. [Performance Considerations](#performance-considerations)

---

## Quick Start

### Prerequisites

- MQTT broker (Mosquitto, HiveMQ, etc.)
- painlessMesh gateway publishing topology
- Modern web browser (Chrome, Firefox, Edge)
- Basic JavaScript/HTML knowledge

### Test Your Setup

```bash
# Subscribe to topology messages
mosquitto_sub -h localhost -t "alteriom/mesh/+/topology" -v

# You should see messages like:
# alteriom/mesh/MESH-001/topology {"event":"mesh_topology",...}
```

---

## D3.js Force-Directed Graph

### Complete HTML Example

Save as `mesh-viewer-d3.html`:

```html
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Alteriom Mesh Viewer - D3.js</title>
    <script src="https://d3js.org/d3.v7.min.js"></script>
    <script src="https://unpkg.com/mqtt/dist/mqtt.min.js"></script>
    <style>
        body {
            margin: 0;
            font-family: Arial, sans-serif;
            background: #1a1a2e;
            color: #eee;
        }
        #container {
            display: flex;
            height: 100vh;
        }
        #graph {
            flex: 1;
            background: #16213e;
        }
        #sidebar {
            width: 300px;
            background: #0f3460;
            padding: 20px;
            overflow-y: auto;
        }
        .node {
            cursor: pointer;
        }
        .node-gateway { fill: #00d4ff; }
        .node-sensor { fill: #4ecca3; }
        .node-repeater { fill: #f39c12; }
        .node-offline { fill: #e74c3c; }
        .link { stroke: #555; stroke-opacity: 0.6; }
        .link-excellent { stroke: #4ecca3; }
        .link-good { stroke: #f39c12; }
        .link-poor { stroke: #e74c3c; }
        .node-label {
            font-size: 10px;
            fill: #eee;
            pointer-events: none;
        }
        #stats {
            background: #16213e;
            padding: 15px;
            margin-bottom: 15px;
            border-radius: 5px;
        }
        .stat-item {
            display: flex;
            justify-content: space-between;
            margin: 5px 0;
        }
        #event-log {
            background: #16213e;
            padding: 15px;
            border-radius: 5px;
            max-height: 300px;
            overflow-y: auto;
        }
        .event {
            padding: 8px;
            margin: 5px 0;
            border-left: 3px solid #00d4ff;
            background: #0f3460;
            font-size: 12px;
        }
        .event-join { border-left-color: #4ecca3; }
        .event-leave { border-left-color: #e74c3c; }
    </style>
</head>
<body>
    <div id="container">
        <div id="graph"></div>
        <div id="sidebar">
            <h2>Mesh Network</h2>
            <div id="stats">
                <div class="stat-item">
                    <span>Total Nodes:</span>
                    <span id="stat-total">0</span>
                </div>
                <div class="stat-item">
                    <span>Online:</span>
                    <span id="stat-online">0</span>
                </div>
                <div class="stat-item">
                    <span>Avg Quality:</span>
                    <span id="stat-quality">0%</span>
                </div>
                <div class="stat-item">
                    <span>Network Diameter:</span>
                    <span id="stat-diameter">0</span>
                </div>
            </div>
            <h3>Recent Events</h3>
            <div id="event-log"></div>
        </div>
    </div>

    <script>
        // Configuration
        const MQTT_BROKER = 'ws://localhost:9001'; // WebSocket port
        const MESH_ID = 'MESH-001';
        
        // D3 Setup
        const width = window.innerWidth - 300;
        const height = window.innerHeight;
        
        const svg = d3.select('#graph')
            .append('svg')
            .attr('width', width)
            .attr('height', height);
        
        const g = svg.append('g');
        
        // Zoom behavior
        svg.call(d3.zoom()
            .extent([[0, 0], [width, height]])
            .scaleExtent([0.1, 8])
            .on('zoom', (event) => g.attr('transform', event.transform)));
        
        // Force simulation
        const simulation = d3.forceSimulation()
            .force('link', d3.forceLink().id(d => d.id).distance(150))
            .force('charge', d3.forceManyBody().strength(-400))
            .force('center', d3.forceCenter(width / 2, height / 2))
            .force('collision', d3.forceCollide().radius(50));
        
        let link = g.append('g').selectAll('line');
        let node = g.append('g').selectAll('g');
        let label = g.append('g').selectAll('text');
        
        // MQTT Connection
        const client = mqtt.connect(MQTT_BROKER);
        
        client.on('connect', () => {
            console.log('✅ Connected to MQTT');
            client.subscribe(`alteriom/mesh/${MESH_ID}/topology`);
            client.subscribe(`alteriom/mesh/${MESH_ID}/events`);
            addEvent('Connected to MQTT broker', 'join');
        });
        
        client.on('message', (topic, message) => {
            const data = JSON.parse(message.toString());
            
            if (data.event === 'mesh_topology') {
                updateGraph(data);
            } else if (data.event === 'mesh_event') {
                handleEvent(data);
            }
        });
        
        function updateGraph(topology) {
            // Update statistics
            if (topology.metrics) {
                document.getElementById('stat-total').textContent = topology.metrics.total_nodes;
                document.getElementById('stat-online').textContent = topology.metrics.online_nodes;
                document.getElementById('stat-quality').textContent = 
                    topology.metrics.avg_connection_quality + '%';
                document.getElementById('stat-diameter').textContent = topology.metrics.network_diameter;
            }
            
            // Prepare nodes and links
            const nodes = topology.nodes.map(n => ({
                id: n.node_id,
                role: n.role,
                status: n.status,
                memory: n.free_memory_kb,
                connections: n.connection_count
            }));
            
            const links = topology.connections.map(c => ({
                source: c.from_node,
                target: c.to_node,
                quality: c.quality,
                rssi: c.rssi,
                latency: c.latency_ms
            }));
            
            // Update D3 visualization
            link = link.data(links, d => `${d.source}-${d.target}`);
            link.exit().remove();
            link = link.enter()
                .append('line')
                .attr('class', d => {
                    if (d.quality > 80) return 'link link-excellent';
                    if (d.quality > 50) return 'link link-good';
                    return 'link link-poor';
                })
                .attr('stroke-width', d => d.quality / 20)
                .merge(link);
            
            node = node.data(nodes, d => d.id);
            node.exit().remove();
            
            const nodeEnter = node.enter()
                .append('g')
                .attr('class', 'node')
                .call(d3.drag()
                    .on('start', dragstarted)
                    .on('drag', dragged)
                    .on('end', dragended));
            
            nodeEnter.append('circle')
                .attr('r', 20)
                .attr('class', d => {
                    if (d.status === 'offline') return 'node-offline';
                    return `node-${d.role}`;
                });
            
            nodeEnter.append('title')
                .text(d => `${d.id}\nRole: ${d.role}\nMemory: ${d.memory}KB`);
            
            node = nodeEnter.merge(node);
            
            // Update labels
            label = label.data(nodes, d => d.id);
            label.exit().remove();
            label = label.enter()
                .append('text')
                .attr('class', 'node-label')
                .attr('text-anchor', 'middle')
                .attr('dy', 35)
                .text(d => d.id.substring(4, 10))
                .merge(label);
            
            // Restart simulation
            simulation.nodes(nodes);
            simulation.force('link').links(links);
            simulation.alpha(1).restart();
            
            simulation.on('tick', () => {
                link
                    .attr('x1', d => d.source.x)
                    .attr('y1', d => d.source.y)
                    .attr('x2', d => d.target.x)
                    .attr('y2', d => d.target.y);
                
                node.attr('transform', d => `translate(${d.x},${d.y})`);
                label.attr('x', d => d.x).attr('y', d => d.y);
            });
        }
        
        function handleEvent(event) {
            const type = event.event_type;
            const nodes = event.affected_nodes.join(', ');
            let message = '';
            
            switch(type) {
                case 'node_join':
                    message = `Node ${nodes} joined mesh`;
                    break;
                case 'node_leave':
                    message = `Node ${nodes} left mesh`;
                    break;
                default:
                    message = `Event: ${type}`;
            }
            
            addEvent(message, type);
        }
        
        function addEvent(message, type = '') {
            const log = document.getElementById('event-log');
            const event = document.createElement('div');
            event.className = `event event-${type}`;
            event.textContent = `${new Date().toLocaleTimeString()}: ${message}`;
            log.insertBefore(event, log.firstChild);
            
            // Keep only last 20 events
            while (log.children.length > 20) {
                log.removeChild(log.lastChild);
            }
        }
        
        function dragstarted(event, d) {
            if (!event.active) simulation.alphaTarget(0.3).restart();
            d.fx = d.x;
            d.fy = d.y;
        }
        
        function dragged(event, d) {
            d.fx = event.x;
            d.fy = event.y;
        }
        
        function dragended(event, d) {
            if (!event.active) simulation.alphaTarget(0);
            d.fx = null;
            d.fy = null;
        }
    </script>
</body>
</html>
```

### Usage

1. **Start MQTT broker with WebSocket support:**
   ```bash
   # Mosquitto with WebSocket on port 9001
   mosquitto -c mosquitto.conf
   ```

2. **Configure mosquitto.conf:**
   ```
   listener 1883
   protocol mqtt
   
   listener 9001
   protocol websockets
   ```

3. **Open in browser:**
   ```bash
   # Simply open the HTML file
   open mesh-viewer-d3.html
   ```

---

## Cytoscape.js Network View

### Complete HTML Example

Save as `mesh-viewer-cytoscape.html`:

```html
<!DOCTYPE html>
<html>
<head>
    <title>Alteriom Mesh - Cytoscape</title>
    <script src="https://unpkg.com/cytoscape/dist/cytoscape.min.js"></script>
    <script src="https://unpkg.com/mqtt/dist/mqtt.min.js"></script>
    <style>
        body {
            margin: 0;
            font-family: Arial, sans-serif;
        }
        #cy {
            width: 100%;
            height: 100vh;
            background: #1a1a2e;
        }
        #controls {
            position: absolute;
            top: 20px;
            right: 20px;
            background: rgba(15, 52, 96, 0.9);
            padding: 15px;
            border-radius: 8px;
            color: white;
            min-width: 200px;
        }
        button {
            width: 100%;
            padding: 10px;
            margin: 5px 0;
            background: #00d4ff;
            border: none;
            border-radius: 5px;
            color: #1a1a2e;
            cursor: pointer;
            font-weight: bold;
        }
        button:hover {
            background: #00a8cc;
        }
    </style>
</head>
<body>
    <div id="cy"></div>
    <div id="controls">
        <h3>Controls</h3>
        <button onclick="cy.fit()">Fit to Screen</button>
        <button onclick="cy.layout({name: 'cose'}).run()">Re-layout</button>
        <button onclick="highlightGateway()">Highlight Gateway</button>
    </div>

    <script>
        const cy = cytoscape({
            container: document.getElementById('cy'),
            style: [
                {
                    selector: 'node',
                    style: {
                        'background-color': '#00d4ff',
                        'label': 'data(label)',
                        'width': 40,
                        'height': 40,
                        'font-size': 10,
                        'color': '#fff',
                        'text-valign': 'bottom',
                        'text-halign': 'center',
                        'text-margin-y': 5
                    }
                },
                {
                    selector: 'node[role="gateway"]',
                    style: {
                        'background-color': '#00d4ff',
                        'width': 60,
                        'height': 60,
                        'border-width': 3,
                        'border-color': '#fff'
                    }
                },
                {
                    selector: 'node[role="sensor"]',
                    style: {
                        'background-color': '#4ecca3'
                    }
                },
                {
                    selector: 'node[role="repeater"]',
                    style: {
                        'background-color': '#f39c12'
                    }
                },
                {
                    selector: 'node[status="offline"]',
                    style: {
                        'background-color': '#e74c3c',
                        'opacity': 0.5
                    }
                },
                {
                    selector: 'edge',
                    style: {
                        'width': 'data(width)',
                        'line-color': 'data(color)',
                        'target-arrow-shape': 'triangle',
                        'target-arrow-color': 'data(color)',
                        'curve-style': 'bezier'
                    }
                }
            ],
            layout: {
                name: 'cose',
                animate: true,
                animationDuration: 1000
            }
        });

        const client = mqtt.connect('ws://localhost:9001');
        
        client.on('connect', () => {
            console.log('Connected to MQTT');
            client.subscribe('alteriom/mesh/+/topology');
        });
        
        client.on('message', (topic, message) => {
            const data = JSON.parse(message.toString());
            if (data.event === 'mesh_topology') {
                updateCytoscape(data);
            }
        });
        
        function updateCytoscape(topology) {
            cy.elements().remove();
            
            // Add nodes
            topology.nodes.forEach(n => {
                cy.add({
                    group: 'nodes',
                    data: {
                        id: n.node_id,
                        label: n.node_id.substring(4, 10),
                        role: n.role,
                        status: n.status,
                        memory: n.free_memory_kb
                    }
                });
            });
            
            // Add edges
            topology.connections.forEach(c => {
                const color = c.quality > 80 ? '#4ecca3' : 
                             c.quality > 50 ? '#f39c12' : '#e74c3c';
                const width = Math.max(1, c.quality / 20);
                
                cy.add({
                    group: 'edges',
                    data: {
                        source: c.from_node,
                        target: c.to_node,
                        width: width,
                        color: color,
                        quality: c.quality
                    }
                });
            });
            
            cy.layout({name: 'cose', animate: true}).run();
        }
        
        function highlightGateway() {
            const gateway = cy.nodes('[role="gateway"]');
            cy.fit(gateway, 50);
            gateway.flashClass('highlighted', 2000);
        }
        
        // Node click handler
        cy.on('tap', 'node', function(evt) {
            const node = evt.target;
            alert(`Node: ${node.data('id')}\nRole: ${node.data('role')}\nMemory: ${node.data('memory')}KB`);
        });
    </script>
</body>
</html>
```

---

## Real-Time Dashboard

### Node.js + Express + Socket.IO

**server.js:**

```javascript
const express = require('express');
const http = require('http');
const socketIO = require('socket.io');
const mqtt = require('mqtt');

const app = express();
const server = http.createServer(app);
const io = socketIO(server);

// MQTT Configuration
const mqttClient = mqtt.connect('mqtt://localhost:1883');

mqttClient.on('connect', () => {
    console.log('✅ Connected to MQTT broker');
    mqttClient.subscribe('alteriom/mesh/+/topology');
    mqttClient.subscribe('alteriom/mesh/+/events');
});

mqttClient.on('message', (topic, message) => {
    const data = JSON.parse(message.toString());
    
    // Forward to all connected web clients
    io.emit('mesh-data', {
        topic: topic,
        data: data
    });
});

app.use(express.static('public'));

io.on('connection', (socket) => {
    console.log('Client connected');
    
    socket.on('disconnect', () => {
        console.log('Client disconnected');
    });
});

const PORT = process.env.PORT || 3000;
server.listen(PORT, () => {
    console.log(`🚀 Dashboard running on http://localhost:${PORT}`);
});
```

**public/index.html:**

```html
<!DOCTYPE html>
<html>
<head>
    <title>Mesh Dashboard</title>
    <script src="/socket.io/socket.io.js"></script>
    <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
</head>
<body>
    <h1>Mesh Network Dashboard</h1>
    <div id="stats"></div>
    <canvas id="qualityChart" width="400" height="200"></canvas>
    
    <script>
        const socket = io();
        const ctx = document.getElementById('qualityChart').getContext('2d');
        const chart = new Chart(ctx, {
            type: 'line',
            data: {
                labels: [],
                datasets: [{
                    label: 'Average Quality',
                    data: [],
                    borderColor: '#4ecca3',
                    tension: 0.1
                }]
            },
            options: {
                scales: {
                    y: {
                        beginAtZero: true,
                        max: 100
                    }
                }
            }
        });
        
        socket.on('mesh-data', (message) => {
            if (message.data.event === 'mesh_topology') {
                updateDashboard(message.data);
            }
        });
        
        function updateDashboard(topology) {
            document.getElementById('stats').innerHTML = `
                <p>Nodes: ${topology.metrics.total_nodes}</p>
                <p>Quality: ${topology.metrics.avg_connection_quality}%</p>
            `;
            
            // Update chart
            const now = new Date().toLocaleTimeString();
            chart.data.labels.push(now);
            chart.data.datasets[0].data.push(topology.metrics.avg_connection_quality);
            
            if (chart.data.labels.length > 20) {
                chart.data.labels.shift();
                chart.data.datasets[0].data.shift();
            }
            
            chart.update();
        }
    </script>
</body>
</html>
```

**package.json:**

```json
{
  "name": "mesh-dashboard",
  "version": "1.0.0",
  "dependencies": {
    "express": "^4.18.0",
    "socket.io": "^4.5.0",
    "mqtt": "^4.3.0"
  },
  "scripts": {
    "start": "node server.js"
  }
}
```

**Run:**

```bash
npm install
npm start
```

---

## Python Monitoring

### Simple Console Monitor

```python
#!/usr/bin/env python3
"""
Alteriom Mesh Topology Monitor
Displays mesh network status in terminal
"""

import paho.mqtt.client as mqtt
import json
from datetime import datetime
from rich.console import Console
from rich.table import Table
from rich.live import Live

console = Console()

# Configuration
BROKER = "localhost"
PORT = 1883
MESH_ID = "MESH-001"

topology_data = None

def on_connect(client, userdata, flags, rc):
    console.print(f"[green]✅ Connected to MQTT broker[/green]")
    client.subscribe(f"alteriom/mesh/{MESH_ID}/topology")
    client.subscribe(f"alteriom/mesh/{MESH_ID}/events")

def on_message(client, userdata, msg):
    global topology_data
    data = json.loads(msg.payload.decode())
    
    if data.get('event') == 'mesh_topology':
        topology_data = data
    elif data.get('event') == 'mesh_event':
        event_type = data.get('event_type')
        nodes = ', '.join(data.get('affected_nodes', []))
        console.print(f"[yellow]🔔 Event: {event_type} - {nodes}[/yellow]")

def create_table():
    if not topology_data:
        return Table(title="Waiting for topology data...")
    
    table = Table(title=f"Mesh Network - {datetime.now().strftime('%H:%M:%S')}")
    
    table.add_column("Node ID", style="cyan")
    table.add_column("Role", style="magenta")
    table.add_column("Status", style="green")
    table.add_column("Memory", justify="right")
    table.add_column("Connections", justify="right")
    
    for node in topology_data.get('nodes', []):
        status_color = "green" if node['status'] == 'online' else "red"
        table.add_row(
            node['node_id'][-12:],
            node['role'],
            f"[{status_color}]{node['status']}[/{status_color}]",
            f"{node.get('free_memory_kb', 0)} KB",
            str(node.get('connection_count', 0))
        )
    
    # Add metrics
    metrics = topology_data.get('metrics', {})
    table.add_section()
    table.add_row(
        "[bold]METRICS[/bold]",
        f"Quality: {metrics.get('avg_connection_quality', 0)}%",
        f"Diameter: {metrics.get('network_diameter', 0)}",
        f"Total: {metrics.get('total_nodes', 0)}",
        f"Online: {metrics.get('online_nodes', 0)}"
    )
    
    return table

def main():
    client = mqtt.Client()
    client.on_connect = on_connect
    client.on_message = on_message
    
    client.connect(BROKER, PORT, 60)
    client.loop_start()
    
    try:
        with Live(create_table(), refresh_per_second=1) as live:
            while True:
                live.update(create_table())
    except KeyboardInterrupt:
        console.print("\n[red]Disconnecting...[/red]")
        client.loop_stop()
        client.disconnect()

if __name__ == "__main__":
    main()
```

**Install dependencies:**

```bash
pip install paho-mqtt rich
python mesh_monitor.py
```

---

## Node-RED Flows

### Import this flow into Node-RED:

```json
[
    {
        "id": "mqtt-in",
        "type": "mqtt in",
        "broker": "mqtt-broker",
        "topic": "alteriom/mesh/+/topology",
        "name": "Mesh Topology"
    },
    {
        "id": "parse-json",
        "type": "json",
        "name": "Parse JSON"
    },
    {
        "id": "extract-metrics",
        "type": "function",
        "name": "Extract Metrics",
        "func": "msg.payload = {\n    total_nodes: msg.payload.metrics.total_nodes,\n    avg_quality: msg.payload.metrics.avg_connection_quality,\n    online: msg.payload.metrics.online_nodes\n};\nreturn msg;"
    },
    {
        "id": "dashboard-gauge",
        "type": "ui_gauge",
        "group": "mesh-stats",
        "name": "Avg Quality",
        "min": 0,
        "max": 100
    },
    {
        "id": "mqtt-broker",
        "type": "mqtt-broker",
        "broker": "localhost",
        "port": "1883"
    }
]
```

---

## Troubleshooting

### Problem: No topology messages appearing

**Symptoms:**
- Empty graph
- No MQTT messages received
- Console shows "Waiting for data..."

**Solutions:**

1. **Check MQTT broker:**
   ```bash
   mosquitto_sub -h localhost -t "alteriom/mesh/#" -v
   ```

2. **Verify WebSocket port:**
   ```bash
   # Check if port 9001 is open
   netstat -an | grep 9001
   ```

3. **Check browser console:**
   - Open DevTools (F12)
   - Look for connection errors
   - Verify MQTT connection message

4. **Test with mosquitto_pub:**
   ```bash
   mosquitto_pub -h localhost -t "alteriom/mesh/MESH-001/topology" -m '{"event":"mesh_topology","nodes":[]}'
   ```

### Problem: Nodes not appearing in graph

**Check:**
- Node ID format is `ALT-XXXXXXXXXXXX`
- `nodes` array is not empty
- `status` field is "online"
- JSON is valid

### Problem: Quality metrics showing 0

**Solutions:**
- Verify `quality` field in connections (0-100)
- Check `rssi` is negative (-30 to -90)
- Ensure `latency_ms` is positive

### Problem: Events not publishing

**Check:**
- Subscribed to correct topic: `alteriom/mesh/+/events`
- Event type is one of: `node_join`, `node_leave`, `node_timeout`
- `affected_nodes` array contains valid node IDs

---

## Performance Considerations

### Message Frequency

**Recommendations:**
- Full topology: Every 60 seconds
- Incremental updates: Every 5 seconds
- Events: Immediate (on change)

**For large meshes (50+ nodes):**
```javascript
// Throttle updates
let lastUpdate = 0;
const UPDATE_INTERVAL = 2000; // 2 seconds

client.on('message', (topic, message) => {
    const now = Date.now();
    if (now - lastUpdate < UPDATE_INTERVAL) return;
    lastUpdate = now;
    
    updateGraph(JSON.parse(message));
});
```

### Buffer Sizes

**MQTT Client:**
```javascript
const client = mqtt.connect('ws://localhost:9001', {
    clientId: 'mesh-viewer-' + Math.random(),
    clean: true,
    reconnectPeriod: 1000,
    keepalive: 60
});
```

**For ESP32:**
```cpp
mqttClient.setBufferSize(2048); // Increase for large topologies
```

### QoS Levels

**Recommended:**
- Topology messages: QoS 0 (fire and forget)
- Events: QoS 1 (at least once)
- Commands: QoS 1 (at least once)

### Network Scaling

| Nodes | Update Frequency | Buffer Size | Notes |
|-------|-----------------|-------------|-------|
| 1-10  | 1s              | 1KB         | Default |
| 10-30 | 5s              | 2KB         | Recommended |
| 30-50 | 10s             | 4KB         | Increase buffer |
| 50+   | 30s             | 8KB         | Consider incremental only |

---

## Additional Resources

- [MQTT Schema Review](./MQTT_SCHEMA_REVIEW.md)
- [Hardware Test Sketch](../examples/mqttTopologyTest/)
- [D3.js Documentation](https://d3js.org/)
- [Cytoscape.js Documentation](https://js.cytoscape.org/)
- [@alteriom/mqtt-schema Package](https://www.npmjs.com/package/@alteriom/mqtt-schema)

---

**Created by:** Alteriom Development Team  
**Last Updated:** October 14, 2025  
**License:** MIT
