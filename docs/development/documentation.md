# Documentation Contributing Guide

Thank you for helping improve the painlessMesh documentation! This guide will help you contribute effectively.

## Documentation Structure

The documentation is organized into these main directories:

```
docs/
├── README.md                    # Documentation index
├── getting-started/             # Installation and quickstart guides
├── tutorials/                   # Step-by-step tutorials
├── api/                         # API reference documentation
├── architecture/                # System design and architecture docs
├── alteriom/                    # Alteriom-specific extensions
├── troubleshooting/             # Common issues and debugging
├── development/                 # Development and testing docs
├── releases/                    # Release notes and summaries
├── improvements/                # Feature proposals and enhancements
└── archive/                     # Historical/obsolete documentation
```

## Documentation Standards

### Markdown Style

We follow the [Markdown Style Guide](https://www.markdownguide.org/basic-syntax/) with these specific conventions:

**Headers:**

- Use ATX-style headers (`#` syntax)
- Add blank lines before and after headers
- Use sentence case (capitalize first word only)

```markdown
# Main heading

## Section heading

Content goes here.

### Subsection heading
```

**Lists:**

- Add blank lines before and after lists
- Use `-` for unordered lists
- Use `1.` for ordered lists
- Indent nested lists by 2 spaces

```markdown
Here's a list:

- First item
- Second item
  - Nested item
  - Another nested
- Third item

Back to content.
```

**Code Blocks:**

- Add blank lines before and after code blocks
- Always specify language for syntax highlighting
- Use `cpp` for Arduino/C++ code
- Use `bash` for shell commands
- Use `ini` for PlatformIO config

```markdown
Example code:

```cpp
#include "painlessMesh.h"

painlessMesh mesh;
void setup() {
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler);
}
```

Back to content.

```

**Links:**
- Use relative paths for internal documentation links
- Use descriptive link text (not "click here")
- Verify links work before committing

```markdown
Good: See the [debugging guide](../troubleshooting/debugging.md) for details.
Bad: For more info, click [here](../troubleshooting/debugging.md).
```

### File Naming

- Use lowercase with hyphens: `my-document.md`
- Be descriptive: `mqtt-bridge-setup.md` not `mqtt.md`
- Use consistent prefixes for related docs

### Content Guidelines

**Be Clear and Concise:**

- Write in active voice
- Use short sentences and paragraphs
- Define acronyms on first use
- Include examples for complex topics

**Be Accurate:**

- Test all code examples before documenting
- Verify API references against source code
- Update version numbers and compatibility info
- Link to related documentation

**Be Helpful:**

- Anticipate user questions
- Provide troubleshooting tips
- Include "common mistakes" sections
- Add "See Also" references

## Types of Documentation

### 1. Getting Started Guides

**Purpose:** Help new users get up and running quickly

**Should Include:**

- Prerequisites
- Installation steps
- First example
- Next steps / what to read next

**Example Structure:**

```markdown
# Quick Start Guide

## Prerequisites
- Hardware requirements
- Software requirements

## Installation
Step-by-step installation

## Your First Mesh
Simple working example with explanation

## Next Steps
- Link to tutorials
- Link to API reference
```

### 2. Tutorials

**Purpose:** Teach specific skills through hands-on practice

**Should Include:**

- Learning objectives
- Required materials/setup
- Step-by-step instructions
- Complete working code
- Explanation of concepts
- Troubleshooting section

**Example Structure:**

```markdown
# Tutorial: Building a Sensor Network

**What you'll learn:**
- How to use SensorPackage
- Broadcasting sensor data
- ...

**What you'll need:**
- 2+ ESP32 devices
- DHT22 sensor
- ...

## Step 1: Setup
...

## Step 2: Code
...

## Troubleshooting
...
```

### 3. API Documentation

**Purpose:** Comprehensive reference for all classes and methods

**Should Include:**

- Class/function signature
- Parameters with types
- Return values
- Description
- Usage examples
- Related functions

**Example Structure:**

```markdown
### mesh.sendBroadcast()

Sends a message to all nodes in the mesh.

**Signature:**
```cpp
bool sendBroadcast(String& msg);
```

**Parameters:**

- `msg` (String&): Message to broadcast (JSON recommended)

**Returns:**

- `true` if message queued successfully
- `false` if queue full or error

**Example:**

```cpp
String msg = "{\"type\":\"sensor\",\"value\":42}";
if (mesh.sendBroadcast(msg)) {
  Serial.println("Message sent");
}
```

**See Also:**

- `sendSingle()` - Send to specific node
- `onReceive()` - Receive messages

```

### 4. Architecture Documentation

**Purpose:** Explain system design and internals

**Should Include:**
- High-level overview
- Component diagrams
- Data flow diagrams
- Design decisions and rationale
- Performance characteristics

### 5. Troubleshooting Guides

**Purpose:** Help users solve problems

**Should Include:**
- Symptom description
- Diagnostic steps
- Common causes
- Solutions
- Prevention tips

**Example Structure:**
```markdown
## Problem: Nodes Not Connecting

**Symptoms:**
- Nodes don't appear in node list
- onNewConnection never fires

**Diagnostic Steps:**
1. Enable debug: `mesh.setDebugMsgTypes(ERROR | CONNECTION)`
2. Check serial output for errors
3. ...

**Common Causes:**
- Mismatched MESH_PREFIX
- Different MESH_PORT
- ...

**Solutions:**
- Verify all nodes use same credentials
- ...

**Prevention:**
- Use configuration file for mesh settings
- ...
```

## Contributing Process

### 1. Before You Start

- **Check existing issues:** Someone may already be working on it
- **Discuss major changes:** Open an issue for significant additions
- **Review existing docs:** Maintain consistency with current documentation

### 2. Making Changes

**For Minor Edits (typos, clarifications):**

1. Edit directly on GitHub
2. Submit pull request
3. Describe what you fixed

**For Major Additions:**

1. Fork the repository
2. Create a branch: `git checkout -b docs/mqtt-bridge-guide`
3. Make your changes
4. Test locally (see below)
5. Commit with descriptive message
6. Push and create pull request

### 3. Testing Your Changes

**Check Markdown Formatting:**

```bash
# Install markdownlint
npm install -g markdownlint-cli

# Check your files
markdownlint docs/**/*.md

# Auto-fix what's possible
markdownlint --fix docs/**/*.md
```

**Preview Locally:**

```bash
# Option 1: VS Code with Markdown Preview
# Just open .md file and press Ctrl+Shift+V

# Option 2: Serve docs locally
cd docs
python -m http.server 8000
# Open http://localhost:8000
```

**Verify Links:**

```bash
# Check for broken links
grep -r "](.*\.md)" docs/ | # Find all markdown links
while read line; do
  # Extract and check each link exists
  # (manual verification recommended)
done
```

### 4. Pull Request Guidelines

**PR Title:**

- `docs: add MQTT bridge tutorial`
- `docs: fix broken links in API reference`
- `docs: update installation guide for v1.7.0`

**PR Description:**

```markdown
## What
Brief description of changes

## Why
Why this documentation is needed

## Changes
- Added new tutorial for X
- Updated API reference for Y
- Fixed broken links in Z

## Checklist
- [ ] Markdown formatting checked
- [ ] Code examples tested
- [ ] Links verified
- [ ] Added to appropriate index/README
```

## Documentation Maintenance

### Keeping Docs Current

**Version Updates:**

- Update version numbers in examples
- Mark deprecated features
- Add "New in v1.X" callouts

**Code Examples:**

- Test examples with each release
- Update for API changes
- Verify dependencies

**Link Checking:**

- Periodically verify internal links
- Check external links still valid
- Update or remove dead links

### Documentation Reviews

When reviewing documentation PRs, check:

- [ ] **Accuracy:** Information is correct and up-to-date
- [ ] **Clarity:** Easy to understand for target audience
- [ ] **Completeness:** Covers the topic adequately
- [ ] **Examples:** Code examples work and are helpful
- [ ] **Style:** Follows our markdown conventions
- [ ] **Links:** All links work correctly
- [ ] **Grammar:** No typos or grammar errors
- [ ] **Navigation:** Added to appropriate index/README

## Common Documentation Tasks

### Adding a New Tutorial

1. Create file in `docs/tutorials/`
2. Follow tutorial structure (see above)
3. Add entry to `docs/README.md`
4. Add entry to `docs/tutorials/README.md` (if exists)
5. Link from related documents

### Updating API Reference

1. Check source code for changes
2. Update method signatures
3. Update parameter descriptions
4. Add/update examples
5. Mark deprecated methods
6. Update version compatibility info

### Fixing Broken Links

1. Find broken links: `grep -r "](.*\.md)" docs/`
2. Verify which files moved/renamed
3. Update all references
4. Check git history if uncertain
5. Test links in preview

### Adding Code Examples

**Best Practices:**

- Keep examples minimal and focused
- Include necessary includes
- Comment complex sections
- Test before documenting
- Show both setup and usage
- Include error handling

**Example Template:**

```cpp
/**
 * Example: Sensor Data Broadcasting
 * 
 * Demonstrates how to broadcast sensor readings
 * using the Alteriom SensorPackage.
 * 
 * Hardware: ESP32 + DHT22 sensor
 * Libraries: painlessMesh, DHT sensor library
 */

#include "painlessMesh.h"
#include "examples/alteriom/alteriom_sensor_package.hpp"
#include "DHT.h"

#define MESH_PREFIX     "SensorMesh"
#define MESH_PASSWORD   "password123"
#define MESH_PORT       5555
#define DHT_PIN         4
#define DHT_TYPE        DHT22

Scheduler userScheduler;
painlessMesh mesh;
DHT dht(DHT_PIN, DHT_TYPE);

void sendSensorData() {
  // Read sensor
  float temp = dht.readTemperature();
  float humidity = dht.readHumidity();
  
  // Check for errors
  if (isnan(temp) || isnan(humidity)) {
    Serial.println("Failed to read from DHT sensor");
    return;
  }
  
  // Create package
  alteriom::SensorPackage pkg;
  pkg.sensorId = mesh.getNodeId();
  pkg.temperature = temp;
  pkg.humidity = humidity;
  pkg.timestamp = mesh.getNodeTime();
  
  // Convert to JSON and broadcast
  auto var = painlessmesh::protocol::Variant(&pkg);
  String msg = var.to<String>();
  
  if (mesh.sendBroadcast(msg)) {
    Serial.printf("Sent: T=%.1f°C H=%.1f%%\n", temp, humidity);
  }
}

Task sensorTask(TASK_MINUTE, TASK_FOREVER, &sendSensorData);

void setup() {
  Serial.begin(115200);
  
  // Initialize sensor
  dht.begin();
  
  // Initialize mesh
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
  
  // Add task
  userScheduler.addTask(sensorTask);
  sensorTask.enable();
}

void loop() {
  mesh.update();
}
```

## Style Cheatsheet

### Quick Reference

```markdown
# H1 - Main page title (one per page)

## H2 - Major sections

### H3 - Subsections

**Bold** for emphasis, `code` for technical terms

- Unordered lists
  - Nested items
1. Ordered lists
2. For sequential steps

```cpp
// Code blocks with language
void example() {
  // Code here
}
```

> Blockquotes for important notes

| Tables | Are | Supported |
|--------|-----|-----------|
| Use    | For | Structured data |

[Link text](relative/path/to/file.md)

```

## Questions?

- **General questions:** Open a [discussion](https://github.com/Alteriom/painlessMesh/discussions)
- **Found an error:** Open an [issue](https://github.com/Alteriom/painlessMesh/issues)
- **Want to help:** Check [good first issues](https://github.com/Alteriom/painlessMesh/labels/good%20first%20issue)

## See Also

- [Contributing Guidelines](../../CONTRIBUTING.md) - General contribution guide
- [Code Style Guide](contributing.md) - Code contribution standards
- [Release Guide](../../RELEASE_GUIDE.md) - Release process documentation
