# 🎉 painlessMesh v1.8.6 Released!

We're excited to announce **painlessMesh v1.8.6** - a patch release that fixes a critical bridge failover issue.

## 🔧 What's Fixed

**Bridge Auto-Election** - Meshes now automatically elect a bridge when all nodes start without a designated initial bridge. Previously, the network would remain bridgeless indefinitely in this scenario.

### The Problem
```
--- Bridge Status ---
I am bridge: NO
Internet available: NO  
Known bridges: 0
No primary bridge available! ❌
```

### The Solution
After a 60-second startup grace period, the mesh automatically detects the absence of a bridge and triggers an election. The node with the best WiFi signal (RSSI) becomes the bridge. ✅

## 🚀 Key Features

- **Automatic Recovery** - No manual intervention needed
- **Smart Timing** - 60s grace period for network stabilization
- **Randomized Delays** - Prevents election conflicts
- **Fully Backward Compatible** - Existing setups work unchanged

## 📦 Installation

```bash
# PlatformIO
pio pkg update alteriom/AlteriomPainlessMesh@^1.8.6

# NPM
npm update @alteriom/painlessmesh

# Arduino Library Manager
Search for "AlteriomPainlessMesh" and update
```

## 🎯 Perfect For

✅ Dynamic mesh networks with changing topologies  
✅ IoT deployments requiring automatic bridge recovery  
✅ Development environments without pre-configured bridges  
✅ Fault-tolerant systems needing zero-touch setup  

## 🙏 Thanks

Special thanks to **@woodlist** for reporting the issue and providing detailed logs!

## 📚 Learn More

- 📖 [Full Release Notes](https://github.com/Alteriom/painlessMesh/blob/main/docs/releases/RELEASE_NOTES_v1.8.6.md)
- 🔗 [GitHub Release](https://github.com/Alteriom/painlessMesh/releases/tag/v1.8.6)
- 📝 [Changelog](https://github.com/Alteriom/painlessMesh/blob/main/CHANGELOG.md)
- 💻 [Bridge Failover Example](https://github.com/Alteriom/painlessMesh/tree/main/examples/bridge_failover)

---

**Upgrade today and enjoy seamless bridge failover! 🎊**

#painlessMesh #ESP32 #ESP8266 #IoT #MeshNetwork #Arduino #Alteriom
