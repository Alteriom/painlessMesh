# GitHub Copilot Agents - Quick Start Guide

## Overview

This repository now has **4 specialized GitHub Copilot Agents** that provide expert assistance for different aspects of painlessMesh development:

1. **Release Agent** - Release management and validation
2. **Mesh Development Agent** - ESP8266/ESP32 mesh networking
3. **Testing Agent** - Catch2 test generation and debugging
4. **Documentation Agent** - Documentation and examples

## How to Use Agents

### In VS Code Copilot Chat

Simply mention the agent using `@agent-name` in your Copilot chat:

```
@mesh-dev-agent How do I create a GPS tracking package?
@testing-agent Generate tests for my new package
@docs-agent Document this feature
@release-agent Am I ready to release?
```

### In GitHub Web UI

You can use agents directly on GitHub.com:

**In Pull Requests:**
1. Open any PR in your repository
2. Click the **Copilot** button/icon in the conversation
3. Type your query with agent mention:
   ```
   @release-agent Is this PR ready to merge?
   @testing-agent What tests are missing?
   ```

**In Issues:**
1. Open or create an issue
2. Use Copilot in the comment area
3. Mention the agent you need:
   ```
   @mesh-dev-agent How should I implement this?
   @docs-agent What documentation is needed?
   ```

**In GitHub Codespaces:**
- Full VS Code experience in browser
- All agents available immediately
- Same functionality as local VS Code

### Agent Selection

GitHub Copilot will automatically suggest the most appropriate agent based on your query, but you can explicitly invoke specific agents when you need specialized assistance.

**Agent Discovery:**
- Type `@` in Copilot Chat to see available agents
- Agents appear with their descriptions
- Select from dropdown or type full name

## Agent Capabilities Summary

### 🚀 Release Agent (`@release-agent`)

**Best for:**
- Pre-release validation
- Version consistency checks
- CHANGELOG validation
- Release readiness assessment

**Example Queries:**
```
@release-agent Validate my release
@release-agent Check version consistency
@release-agent What's blocking my release?
```

**Key Features:**
- Runs 21+ validation checks
- Verifies build and tests
- Checks git status
- Validates documentation

### 🌐 Mesh Development Agent (`@mesh-dev-agent`)

**Best for:**
- Creating Alteriom packages
- Memory optimization
- Network patterns
- Mesh debugging

**Example Queries:**
```
@mesh-dev-agent Create a package for GPS data
@mesh-dev-agent My ESP8266 is running out of memory
@mesh-dev-agent How do I send reliable commands?
@mesh-dev-agent Debug my connection issues
```

**Key Features:**
- Package templates
- Platform-specific optimization
- Memory management patterns
- Network reliability patterns

### 🧪 Testing Agent (`@testing-agent`)

**Best for:**
- Test generation
- Test debugging
- Coverage improvement
- Catch2 assistance

**Example Queries:**
```
@testing-agent Generate tests for GPSPackage
@testing-agent Why is my Approx test failing?
@testing-agent Test edge cases for my package
@testing-agent How do I test JSON serialization?
```

**Key Features:**
- Complete Catch2 test generation
- BDD-style test patterns
- Floating-point comparison help
- Serialization validation

### 📚 Documentation Agent (`@docs-agent`)

**Best for:**
- README creation
- Code examples
- API documentation
- User guides

**Example Queries:**
```
@docs-agent Document my GPS package
@docs-agent Create an example for sensor nodes
@docs-agent Update API docs for mesh.sendSingle()
@docs-agent Write a troubleshooting guide
```

**Key Features:**
- Structured documentation
- Complete code examples
- Doxygen comments
- Usage guides

## Typical Workflows

### Workflow 1: Developing a New Package

1. **Design** - `@mesh-dev-agent` 
   ```
   @mesh-dev-agent I need a package for GPS tracking with latitude, 
   longitude, altitude, and timestamp
   ```

2. **Test** - `@testing-agent`
   ```
   @testing-agent Generate comprehensive tests for my GPSPackage
   ```

3. **Document** - `@docs-agent`
   ```
   @docs-agent Document my GPSPackage with usage examples
   ```

4. **Release** - `@release-agent`
   ```
   @release-agent Validate my changes for release
   ```

### Workflow 2: Debugging Issues

1. **Identify Problem** - `@mesh-dev-agent`
   ```
   @mesh-dev-agent My ESP8266 crashes when sending large messages
   ```

2. **Create Tests** - `@testing-agent`
   ```
   @testing-agent Create tests to reproduce memory issues
   ```

3. **Document Solution** - `@docs-agent`
   ```
   @docs-agent Document the memory optimization solution
   ```

### Workflow 3: Improving Test Coverage

1. **Generate Tests** - `@testing-agent`
   ```
   @testing-agent What edge cases should I test for SensorPackage?
   ```

2. **Review Implementation** - `@mesh-dev-agent`
   ```
   @mesh-dev-agent Are there edge cases I'm not handling?
   ```

3. **Document Patterns** - `@docs-agent`
   ```
   @docs-agent Document the testing patterns we use
   ```

## Agent Configuration

All agents are configured in `copilot-agents.json` at the repository root. Each agent has:

- **ID**: Unique identifier for the agent
- **Name**: Display name
- **Description**: What the agent does
- **Instructions**: Detailed behavior and knowledge
- **Capabilities**: List of skills
- **Knowledge Sources**: Files the agent has deep knowledge of
- **Examples**: Sample interactions
- **Tags**: Categories for discovery

## Documentation

Detailed documentation for each agent is available in `.github/agents/`:

- [Release Agent](.github/agents/release-agent.md)
- [Mesh Development Agent](.github/agents/mesh-dev-agent.md)
- [Testing Agent](.github/agents/testing-agent.md)
- [Documentation Agent](.github/agents/docs-agent.md)

Complete agent index: [.github/AGENTS_INDEX.md](.github/AGENTS_INDEX.md)

## Tips for Best Results

### 1. Be Specific

**Less effective:**
```
@mesh-dev-agent Help with packages
```

**More effective:**
```
@mesh-dev-agent Create a BroadcastPackage for GPS data with latitude, 
longitude, altitude, timestamp, and accuracy fields
```

### 2. Provide Context

Include relevant information:
- Target platform (ESP32/ESP8266)
- Current code snippets
- Error messages
- Constraints (memory, performance)

### 3. Ask Follow-up Questions

Agents maintain conversation context:
```
@mesh-dev-agent Create a GPS package
// Agent provides code
// Then ask:
How do I optimize this for ESP8266's limited memory?
```

### 4. Request Multiple Aspects

```
@mesh-dev-agent Create a GPS package with:
- Memory-efficient design
- Error handling
- Example usage
- Integration with existing SensorPackage
```

### 5. Combine Agents

Work with multiple agents in sequence:
```
@mesh-dev-agent Create package [get implementation]
@testing-agent Test this package [get tests]
@docs-agent Document this [get docs]
```

## Troubleshooting

### Agent Not Responding Correctly

**Problem:** Agent gives generic advice instead of repository-specific guidance

**Solution:**
- Mention specific classes, files, or concepts from the repo
- Reference existing packages (SensorPackage, CommandPackage)
- Use repository terminology (painlessMesh, Alteriom)

### Generated Code Doesn't Compile

**Problem:** Code has syntax or compilation errors

**Solution:**
- Provide more context about your environment
- Specify platform (ESP32/ESP8266)
- Ask agent to verify includes and dependencies
- Share the specific error message

### Agent Suggests Wrong Patterns

**Problem:** Agent recommends approaches that don't fit the codebase

**Solution:**
- Reference the Copilot instructions file
- Point to similar existing implementations
- Ask agent to follow repository conventions
- Mention specific style requirements

## Advanced Usage

### Custom Agent Combinations

Create custom workflows by chaining agents:

**Feature Development Pipeline:**
1. `@mesh-dev-agent` - Design and implement
2. `@testing-agent` - Generate comprehensive tests
3. `@docs-agent` - Create documentation
4. `@release-agent` - Validate for release

**Optimization Pipeline:**
1. `@mesh-dev-agent` - Identify optimization opportunities
2. `@testing-agent` - Add performance tests
3. `@docs-agent` - Document optimization techniques

### Context-Aware Queries

Leverage agent knowledge of repository structure:

```
@mesh-dev-agent Following the pattern in examples/alteriom/alteriom_sensor_package.hpp,
create a new package for GPS tracking
```

### Integration with Scripts

Combine agent guidance with repository scripts:

```
@release-agent What checks will fail if I run ./scripts/release-agent.sh?
// Agent analyzes and provides preemptive fixes
// Then run the script:
./scripts/release-agent.sh
```

## Using Agents in Different Environments

### VS Code (Desktop)
- ✅ Full agent functionality
- ✅ All knowledge sources available
- ✅ Repository context loaded
- **How:** Copilot Chat panel → `@agent-name`

### GitHub Codespaces
- ✅ Full agent functionality (same as VS Code)
- ✅ Browser-based development
- ✅ No local setup required
- **How:** Copilot Chat in Codespaces → `@agent-name`

### GitHub Web (Pull Requests)
- ✅ Agent access in PR conversations
- ✅ Code review assistance
- ⚠️ May have limited repository context
- **How:** Copilot button in PR → `@agent-name`

### GitHub Web (Issues)
- ✅ Agent access in issue discussions
- ✅ Feature planning assistance
- ⚠️ May have limited repository context
- **How:** Copilot in issue comments → `@agent-name`

### GitHub Mobile App
- ✅ Basic agent access
- ⚠️ Limited context and functionality
- **How:** Copilot in PR/Issue → `@agent-name`

### Tips for Web UI Usage

**For Best Results:**
1. **Link to code:** Reference specific files or lines
2. **Provide context:** Include relevant code snippets in your query
3. **Be specific:** Mention exact package names, classes, or functions
4. **Use Codespaces:** For complex tasks, use Codespaces for full functionality

**Example Queries in GitHub PR:**
```
@release-agent Review this PR against our release checklist

@testing-agent Looking at src/painlessmesh/mesh.hpp changes, 
what tests should be added?

@docs-agent These changes need documentation. What should I add?

@mesh-dev-agent The changes in examples/alteriom/ - are they 
memory-efficient for ESP8266?
```

## Getting Help

### For Agent Issues

1. Check agent documentation in `.github/agents/`
2. Review [AGENTS_INDEX.md](.github/AGENTS_INDEX.md)
3. Check [copilot-instructions.md](.github/copilot-instructions.md)
4. Open an issue with `agent` label

### For General Development

1. Review [README-DEVELOPMENT.md](.github/README-DEVELOPMENT.md)
2. Check [copilot-quick-reference.md](.github/copilot-quick-reference.md)
3. Consult [API Documentation](https://alteriom.github.io/painlessMesh/)

## Benefits

### Faster Development
- Quick package scaffolding
- Instant test generation
- Rapid documentation creation

### Higher Quality
- Consistent code patterns
- Comprehensive test coverage
- Thorough documentation

### Better Maintenance
- Release validation automation
- Documentation accuracy
- Code consistency

### Knowledge Preservation
- Repository patterns captured
- Best practices enforced
- Institutional knowledge preserved

## Next Steps

1. **Try Each Agent**
   - Experiment with different queries
   - Learn each agent's strengths
   - Understand their specializations

2. **Integrate into Workflow**
   - Use agents during development
   - Include in code review process
   - Leverage for documentation

3. **Provide Feedback**
   - Report issues with `agent` label
   - Suggest improvements
   - Share successful patterns

4. **Extend and Customize**
   - Suggest new agent capabilities
   - Contribute to agent documentation
   - Help improve agent responses

---

**Last Updated:** November 11, 2024  
**Agents Version:** 1.0  
**Repository:** AlteriomPainlessMesh

For the latest information, see [AGENTS_INDEX.md](.github/AGENTS_INDEX.md)
