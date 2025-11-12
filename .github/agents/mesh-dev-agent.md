# Mesh Development Agent

## Overview

The Mesh Development Agent is a specialized Copilot assistant for ESP8266/ESP32 mesh network development with painlessMesh. It provides expert guidance on Alteriom package development, memory optimization, network patterns, and debugging mesh applications.

## Purpose

Help developers build robust, memory-efficient mesh network applications by providing:
- Code generation following painlessMesh best practices
- Alteriom package development templates
- Memory optimization strategies for ESP8266/ESP32
- Debugging assistance for mesh network issues
- Message routing and reliability patterns

## Activation

### In VS Code Copilot Chat

```
@mesh-dev-agent How do I create a new Alteriom package?
@mesh-dev-agent My ESP8266 is running out of memory
@mesh-dev-agent Show me how to send a command to a specific node
```

### Via Copilot Agents Configuration

The agent is automatically available when you open this repository in VS Code with GitHub Copilot enabled.

## Capabilities

### 1. Alteriom Package Development

**Available Package Types:**
- **SensorPackage (Type 200)**: Environmental sensor data
- **CommandPackage (Type 201)**: Device control commands
- **StatusPackage (Type 202)**: Node health monitoring

**What the agent can do:**
- Generate new package classes
- Provide serialization templates
- Create test cases for packages
- Explain JSON format for packages
- Help with type ID allocation

### 2. Memory Optimization

**Target Platforms:**
- ESP32: ~320KB RAM
- ESP8266: ~80KB RAM (critical constraints)

**Optimization techniques:**
- TSTRING usage instead of String
- Stack vs heap allocation
- StaticJsonDocument sizing
- String concatenation alternatives
- Memory leak prevention

### 3. Network Pattern Implementation

**Message Types:**
- SinglePackage: Point-to-point communication
- BroadcastPackage: Network-wide messages

**Reliability Patterns:**
- Exponential backoff for retries
- Message acknowledgment
- Connection state management
- Network congestion handling

### 4. Code Generation

**Templates for:**
- Mesh initialization
- Message sending/receiving
- Callback functions
- Error handling
- Logging patterns

## Usage Examples

### Example 1: Create New GPS Package

**Query:**
```
@mesh-dev-agent I need to create a GPS tracking package that broadcasts latitude, longitude, and altitude
```

**Response includes:**
- Complete C++ class definition
- JSON serialization implementation
- Usage example
- Integration guidance
- Test recommendations

### Example 2: Debug Memory Issues

**Query:**
```
@mesh-dev-agent My ESP8266 keeps crashing with out of memory errors when sending messages
```

**Response includes:**
- Memory monitoring code
- String handling improvements
- JSON document sizing
- Connection limit configuration
- Diagnostic techniques

### Example 3: Implement Reliable Messaging

**Query:**
```
@mesh-dev-agent How do I ensure critical commands reach their destination?
```

**Response includes:**
- Retry mechanism with exponential backoff
- Acknowledgment pattern
- Timeout handling
- Error recovery strategies

## Best Practices Enforced

### Code Style
- ✅ Use TSTRING for cross-platform compatibility
- ✅ Include proper header guards
- ✅ Follow 2-space indentation
- ✅ Use alteriom:: namespace for custom packages
- ✅ Allocate unique type IDs (203+)

### Memory Safety
- ✅ Monitor free heap regularly
- ✅ Avoid String concatenation in loops
- ✅ Use StaticJsonDocument when size is known
- ✅ Clear JSON documents after use
- ✅ Limit connection count on ESP8266

### Network Reliability
- ✅ Implement retry logic for critical messages
- ✅ Validate JSON before processing
- ✅ Handle connection callbacks
- ✅ Use appropriate package types
- ✅ Consider network congestion

### Error Handling
- ✅ Check return values from mesh operations
- ✅ Validate JSON parsing
- ✅ Log errors appropriately
- ✅ Implement fallback strategies
- ✅ Monitor connection state

## Knowledge Sources

The agent has deep knowledge of:

1. **Copilot Instructions**: `.github/copilot-instructions.md`
   - Complete coding patterns
   - Package templates
   - Best practices

2. **Development Guide**: `.github/README-DEVELOPMENT.md`
   - Build system
   - Development workflow
   - Platform setup

3. **Core Library**: `src/painlessmesh/`
   - mesh.hpp - Core mesh functionality
   - plugin.hpp - Package system
   - protocol.hpp - Network protocol

4. **Alteriom Packages**: `examples/alteriom/`
   - Package definitions
   - Implementation examples
   - Usage patterns

5. **Documentation**: `README.md`, `docs/`
   - API reference
   - User guides
   - Troubleshooting

## Common Scenarios

### Scenario 1: New Node Type Development

**Situation**: Developer needs to create a new type of mesh node

**Agent helps with:**
1. Mesh initialization boilerplate
2. Callback function setup
3. Message handling logic
4. Memory-efficient patterns
5. Testing recommendations

### Scenario 2: Package Design

**Situation**: Need custom message format for specific application

**Agent helps with:**
1. Package class structure
2. Field selection and types
3. Serialization implementation
4. Type ID allocation
5. Test generation

### Scenario 3: Performance Optimization

**Situation**: Mesh network is slow or nodes are crashing

**Agent helps with:**
1. Memory profiling
2. Code optimization
3. Network pattern analysis
4. Bottleneck identification
5. Best practice application

### Scenario 4: Integration

**Situation**: Integrating mesh with sensors or actuators

**Agent helps with:**
1. Hardware communication code
2. Data packaging
3. Timing coordination
4. Error handling
5. Power management

## Troubleshooting

### Agent Not Responding Appropriately

**Issue**: Agent provides generic advice instead of painlessMesh-specific guidance

**Solutions**:
- Mention "painlessMesh" or "Alteriom" in your query
- Reference specific classes or functions
- Provide code context
- Ask for examples from the repository

### Code Doesn't Compile

**Issue**: Generated code has compilation errors

**Solutions**:
- Check that all includes are present
- Verify platform-specific code paths
- Ensure correct namespace usage
- Check for type mismatches
- Validate header guards

### Memory Issues Persist

**Issue**: Optimizations don't resolve memory problems

**Solutions**:
- Ask agent to audit entire code flow
- Request platform-specific optimization
- Get heap monitoring implementation
- Review connection count limits
- Consider architectural changes

## Integration with Other Agents

The Mesh Development Agent works well with:

### Testing Agent
- **When**: After creating new packages
- **Why**: Generate comprehensive tests
- **How**: "@testing-agent create tests for my GPS package"

### Documentation Agent
- **When**: New features are implemented
- **Why**: Document for other developers
- **How**: "@docs-agent document my new package"

### Release Agent
- **When**: Ready to release changes
- **Why**: Validate release readiness
- **How**: "@release-agent validate my package changes"

## Advanced Usage

### Custom Package Hierarchies

Create complex package systems with inheritance:

```
@mesh-dev-agent How do I create a base package with common fields that other packages extend?
```

### Multi-Hop Message Routing

Implement sophisticated routing logic:

```
@mesh-dev-agent I need to route messages through intermediate nodes with hop count limiting
```

### Time-Synchronized Actions

Coordinate actions across mesh nodes:

```
@mesh-dev-agent How can I synchronize LED patterns across all nodes using mesh time sync?
```

### OTA Updates

Implement over-the-air firmware updates:

```
@mesh-dev-agent Guide me through implementing OTA updates for my mesh nodes
```

## Performance Characteristics

### Expected Response Times
- Simple queries: < 3 seconds
- Code generation: 5-10 seconds
- Complex debugging: 10-20 seconds

### Response Quality
- Code follows repository conventions
- Includes error handling
- Memory-efficient patterns
- Platform-appropriate solutions

## Feedback and Improvement

### Providing Feedback

If the agent provides incorrect or unhelpful responses:

1. Provide more context in your query
2. Reference specific files or functions
3. Include error messages or symptoms
4. Specify target platform (ESP32/ESP8266)

### Agent Limitations

The agent may struggle with:
- Very new Copilot features not in training data
- Proprietary hardware-specific code
- Complex multi-file refactoring
- Platform bugs or limitations
- Highly custom architectural patterns

**Solution**: Break complex tasks into smaller, specific queries

## Related Resources

### Documentation
- [Copilot Instructions](.github/copilot-instructions.md) - Complete development guide
- [README-DEVELOPMENT](.github/README-DEVELOPMENT.md) - Setup and workflow
- [painlessMesh Docs](https://alteriom.github.io/painlessMesh/) - API reference

### Examples
- `examples/alteriom/` - Alteriom package examples
- `examples/basic/` - Core mesh examples
- `test/catch/` - Test examples

### Other Agents
- [Testing Agent](testing-agent.md) - Test generation
- [Documentation Agent](docs-agent.md) - Documentation maintenance
- [Release Agent](release-agent.md) - Release management

---

**Last Updated**: November 11, 2024  
**Agent Version**: 1.0  
**Maintained By**: AlteriomPainlessMesh Team
