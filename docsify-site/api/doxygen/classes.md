# 📚 Class Reference

> Complete class documentation automatically generated from painlessMesh source code using Doxygen.

## �️ Interactive Documentation Viewer

<div class="docsify-tabs">
<div data-tab="📖 Embedded View">

<div style="background: #f6f8fa; border: 1px solid #d0d7de; border-radius: 6px; padding: 16px; margin: 16px 0;">
  <div style="margin-bottom: 12px;">
    <strong>📋 API Reference - Classes</strong>
    <span style="float: right;">
      <a href="../../api-reference/annotated.html" target="_blank" style="color: #0969da; text-decoration: none;">
        🔗 Open in New Tab
      </a>
    </span>
  </div>
  
  <iframe 
    src="../../api-reference/annotated.html" 
    width="100%" 
    height="700px" 
    frameborder="0"
    style="border: 1px solid #d0d7de; border-radius: 6px; background: white;"
    loading="lazy"
    title="Doxygen Class Documentation">
    <p style="padding: 20px; text-align: center; color: #656d76;">
      📄 Your browser doesn't support iframes. 
      <a href="../../api-reference/annotated.html" target="_blank">Open documentation in new tab</a>
    </p>
  </iframe>
</div>

</div>
<div data-tab="🔗 Direct Links">

### 🎯 Quick Access Links

| Class Category | Description | Direct Link |
|---------------|-------------|-------------|
| **Core Classes** | Main mesh networking functionality | [📖 View Classes](../../api-reference/annotated.html) |
| **Class Index** | Alphabetical class listing | [📋 Browse Index](../../api-reference/classes.html) |
| **Class Hierarchy** | Inheritance relationships | [🌳 View Hierarchy](../../api-reference/hierarchy.html) |
| **Class Members** | All class methods and variables | [⚙️ Browse Members](../../api-reference/functions.html) |

### 🔌 Key Classes by Category

**🌐 Networking Core**
- [`painlessMesh`](../../api-reference/classpainlessMesh.html) - Main mesh network management
- [`Connection`](../../api-reference/classConnection.html) - Individual node connections  
- [`Router`](../../api-reference/classRouter.html) - Message routing and forwarding

**⏰ Task Management**
- [`Scheduler`](../../api-reference/classScheduler.html) - Task scheduling and timing
- [`Task`](../../api-reference/classTask.html) - Individual task representation

**📦 Package System**
- [`SinglePackage`](../../api-reference/classsinglemessage_1_1SinglePackage.html) - Point-to-point messages
- [`BroadcastPackage`](../../api-reference/classbroadcastmessage_1_1BroadcastPackage.html) - Network broadcasts
- [`Variant`](../../api-reference/classprotocol_1_1Variant.html) - Type-safe message container

**🔧 Utilities**
- [`Logger`](../../api-reference/classLogger.html) - Debug and logging functionality
- [`Buffer`](../../api-reference/classBuffer.html) - Memory management utilities

</div>
</div>

## 💡 Usage Tips

### 🔍 Finding What You Need

- **Search by name**: Use Ctrl+F to find specific classes
- **Browse by category**: Use the hierarchy view for organized browsing  
- **Check inheritance**: See which classes extend others
- **View all members**: Find all methods and properties for a class

### 📖 Reading Class Documentation

Each class page includes:
- **Brief description** - What the class does
- **Detailed description** - How it works and when to use it
- **Constructor documentation** - How to create instances
- **Method documentation** - All available functions
- **Member variables** - Class properties and fields
- **Usage examples** - Practical code samples

### 🎯 Quick Examples

```cpp
// Core mesh operations
painlessMesh mesh;
mesh.init("MyMesh", "password", &scheduler, 5555);

// Task scheduling  
Task myTask(1000, TASK_FOREVER, &myCallback);
scheduler.addTask(myTask);

// Package messaging
SensorPackage sensor;
sensor.temperature = 25.5;
mesh.sendPackage(&sensor);
```

## 🚀 Next Steps

After reviewing the class documentation:

1. **Try examples** - See practical usage in [Basic Examples](../../tutorials/basic-examples.md)
2. **Read guides** - Learn patterns in [Core API Guide](../core-api.md)  
3. **Check functions** - Browse [Function Reference](functions.md)
4. **View files** - Explore [File Reference](files.md)

?> **💡 Tip**: If the embedded documentation doesn't load, check the [troubleshooting guide](../../troubleshooting/common-issues.md) or use the direct links above.