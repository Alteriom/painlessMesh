# Two-Tier Agent Architecture

## Overview

The Alteriom organization uses a sophisticated two-tier agent architecture that combines universal capability with deep repository context.

## The Problem This Solves

**Traditional Single-Tier Approach Issues:**
- ❌ Generalist agents lack deep repo context → incomplete implementations
- ❌ Specialized agents lack tool access → can't execute tasks
- ❌ "Add a button" often means just UI code → missing tests, docs, backend
- ❌ No coordination between repos → inconsistent implementations

**Our Two-Tier Solution:**
- ✅ Org-level agent has full tool access
- ✅ Repo coordinator has complete context
- ✅ Tasks are fully decomposed with checklists
- ✅ Nothing is forgotten (tests, docs, examples, configs)
- ✅ Cross-repo consistency maintained

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    TIER 1: ORGANIZATION                      │
│                                                              │
│  ╔══════════════════════════════════════════════════════╗   │
│  ║         Alteriom AI Agent (@alteriom-ai-agent)       ║   │
│  ╠══════════════════════════════════════════════════════╣   │
│  ║ • Universal: Works in ANY repository                 ║   │
│  ║ • Full Tool Access: All MCP servers                  ║   │
│  ║ • Executor: Implements changes with tools            ║   │
│  ║ • Coordinator: Cross-repo operations                 ║   │
│  ╚══════════════════════════════════════════════════════╝   │
│                            │                                 │
│                    Delegates when needs                      │
│                    deep repo context                         │
│                            ↓                                 │
└─────────────────────────────────────────────────────────────┘
                             │
                             ↓
┌─────────────────────────────────────────────────────────────┐
│                  TIER 2: REPOSITORY COORDINATORS             │
│                                                              │
│  ╔═══════════════════════╗  ╔═══════════════════════╗       │
│  ║  painlessMesh         ║  ║  Other Repo           ║       │
│  ║  Coordinator          ║  ║  Coordinators         ║       │
│  ╠═══════════════════════╣  ╠═══════════════════════╣       │
│  ║ Deep Repo Context     ║  ║ Deep Repo Context     ║       │
│  ║ Task Decomposition    ║  ║ Task Decomposition    ║       │
│  ║ Completeness Checks   ║  ║ Completeness Checks   ║       │
│  ║ Convention Guardians  ║  ║ Convention Guardians  ║       │
│  ╚═══════════════════════╝  ╚═══════════════════════╝       │
│                                                              │
│           Specialized Agents (Domain Experts)                │
│  ╔═══════════╗ ╔═════════╗ ╔═══════════╗ ╔═══════════╗     │
│  ║ Release   ║ ║ Mesh    ║ ║ Docs      ║ ║ Testing   ║     │
│  ║ Agent     ║ ║ Dev     ║ ║ Agent     ║ ║ Agent     ║     │
│  ╚═══════════╝ ╚═════════╝ ╚═══════════╝ ╚═══════════╝     │
└─────────────────────────────────────────────────────────────┘
```

## Tier Responsibilities

### Tier 1: Alteriom AI Agent
**Role:** Universal Executor

**Capabilities:**
- Works across all Alteriom repositories
- Full access to MCP tools:
  - Filesystem (read/write)
  - Shell execution (bash, PowerShell)
  - Git operations
  - GitHub API
  - Database (PostgreSQL)
  - Containers (Docker)
  - Python environments
- Can execute any command
- Can modify any file
- Can create/update resources

**When to use:**
- Any task requiring execution
- Cross-repository operations
- File modifications
- Running commands or tests
- Creating issues/PRs
- Publishing releases

**Example:**
```
@alteriom-ai-agent Update version to 1.9.3 in all package files
→ Reads 5 version files
→ Updates all to 1.9.3
→ Runs validation
→ Done
```

### Tier 2: Repository Coordinators
**Role:** Context Expert & Task Decomposer

**painlessMesh Coordinator Capabilities:**
- Deep knowledge of repository structure
- Understands component relationships
- Knows what must be updated together
- Identifies all related tasks
- Creates comprehensive checklists
- Verifies completeness

**Tools:** Read & search only (not executor)

**When to use:**
- Planning new features
- Understanding what's needed for a change
- Verifying task completeness
- Ensuring conventions are followed
- Catching forgotten components

**Example:**
```
@painlessmesh-coordinator What's needed to add GPS support?

→ Analyzes request
→ Creates checklist:
  1. GpsPackage class (type 203)
  2. Test cases in catch_alteriom_packages.cpp
  3. Example sketch gps_mesh.ino
  4. Documentation in README.md
  5. Type registry update
  6. Platform testing (ESP32, ESP8266)

→ Provides to executor
```

## How They Work Together

### Example: Adding GPS Support

#### Step 1: Planning
```
User: @painlessmesh-coordinator I want to add GPS support

Coordinator: GPS support is a new package. Complete implementation needs:

Core Package (examples/alteriom/alteriom_sensor_package.hpp):
□ GpsPackage class (type 203, BroadcastPackage)
  - Fields: latitude, longitude, altitude, timestamp, accuracy, satellites
  - Implement constructor, addTo(), jsonObjectSize()

Testing (test/catch/catch_alteriom_packages.cpp):
□ SCENARIO("GpsPackage serialization works correctly")
  - Test to/from JSON
  - Test Variant conversion

Example (examples/alteriom/gps_mesh.ino):
□ GPS mesh example sketch
  - Setup mesh, read GPS, broadcast packets
  - Display received GPS from other nodes

Documentation:
□ Update examples/alteriom/README.md with GpsPackage
□ Add docs/GPS_INTEGRATION.md

Platform Testing:
□ Test ESP32 build
□ Test ESP8266 build (check memory)
□ Run unit tests
```

#### Step 2: Execution
```
User: @alteriom-ai-agent Implement GPS feature per checklist

Agent: 
[Using filesystem MCP] Creating GpsPackage class...
[Using filesystem MCP] Adding test cases...
[Using bash MCP] Building project: cmake -G Ninja . && ninja
[Using bash MCP] Running tests: run-parts --regex catch_ bin/
[Using filesystem MCP] Creating example sketch...
[Using filesystem MCP] Updating documentation...
[Using bash MCP] Testing ESP32 build: pio run -e esp32dev
[Using bash MCP] Testing ESP8266 build: pio run -e esp8266
✓ All 6 checklist items completed
```

#### Step 3: Verification
```
User: @painlessmesh-coordinator Verify GPS implementation

Coordinator: Checking GPS implementation...
✓ GpsPackage class implemented with all required fields
✓ Test cases added and passing
✓ Example sketch created and compiles
✓ Documentation updated
✓ Builds successfully on both platforms
✓ All checklist items complete

GPS implementation is complete and follows repository conventions.
```

## Benefits

### 1. Complete Implementations
**Before (single agent):**
```
User: Add GPS support
Agent: [Creates GpsPackage class]
→ Missing: tests, examples, docs
→ Incomplete feature
```

**After (two-tier):**
```
User: Add GPS support
Coordinator: [Lists 6 required items]
Executor: [Implements all 6 items]
Coordinator: [Verifies completeness]
→ Complete feature with tests, docs, examples
```

### 2. Repository Consistency
Coordinator ensures:
- ✅ Coding conventions followed
- ✅ Type IDs allocated correctly
- ✅ Tests use proper patterns
- ✅ Documentation is comprehensive
- ✅ Platform compatibility checked
- ✅ Memory constraints considered

### 3. Tool Access
Alteriom AI Agent can:
- ✅ Read and write files
- ✅ Execute build commands
- ✅ Run tests
- ✅ Commit changes
- ✅ Create PRs
- ✅ Publish releases
- ✅ Access databases
- ✅ Manage containers

### 4. Cross-Repository Support
Alteriom AI Agent:
- ✅ Works in any Alteriom repo
- ✅ Coordinates changes across repos
- ✅ Applies org-wide standards
- ✅ Delegates to repo coordinators for context

## Specialized Agents

Specialized agents provide domain expertise:

**Release Agent** - Release management
- Version validation
- CHANGELOG verification
- Test execution
- Release automation

**Mesh Dev Agent** - ESP32/ESP8266 development
- Memory optimization
- Platform-specific code
- Mesh networking patterns
- Embedded debugging

**Docs Agent** - Documentation
- API documentation
- Example creation
- Docusaurus management
- Link validation

**Testing Agent** - Unit testing
- Catch2 test generation
- Test debugging
- Coverage analysis
- Test patterns

## Usage Guidelines

### When to Use Each Agent

| Task | Agent | Reason |
|------|-------|--------|
| Fix typo | `@alteriom-ai-agent` | Simple, direct execution |
| New feature | `@painlessmesh-coordinator` first | Need complete checklist |
| Optimize memory | `@mesh-dev-agent` | Domain expertise |
| Create tests | `@testing-agent` | Testing specialist |
| Release | `@release-agent` | Release validation |
| Update docs | `@docs-agent` | Documentation expert |

### Recommended Workflow

**For Complex Tasks:**
1. **Plan** → Ask coordinator: "What's needed?"
2. **Execute** → Alteriom AI Agent implements checklist
3. **Verify** → Coordinator confirms completeness

**For Simple Tasks:**
- **Execute** → Alteriom AI Agent does it directly

**For Specialized Tasks:**
- **Consult** → Ask specialist for recommendations
- **Execute** → Alteriom AI Agent applies recommendations

## Configuration

### MCP Server Setup

Alteriom AI Agent requires MCP servers configured:

```json
{
  "mcpServers": {
    "filesystem": {
      "command": "npx",
      "args": ["-y", "@modelcontextprotocol/server-filesystem", "/path/to/repo"]
    },
    "bash": {
      "command": "npx",
      "args": ["-y", "@modelcontextprotocol/server-bash"]
    },
    "git": {
      "command": "npx",
      "args": ["-y", "@modelcontextprotocol/server-git", "--repository", "/path/to/repo"]
    }
  }
}
```

### Repository Coordinator Setup

Each repository should have:
1. **Coordinator agent file** (`.github/agents/REPO-coordinator.md`)
2. **Custom instructions** (`.github/copilot-instructions.md`)
3. **Instruction files** (`.github/instructions/`)
4. **Specialized agents** as needed

## Examples

### Example 1: Simple Task
```
@alteriom-ai-agent Update copyright year to 2025 in all source files
```
→ Direct execution, no planning needed

### Example 2: Complete Feature
```
@painlessmesh-coordinator What's needed to add shared gateway mode?
[Coordinator provides detailed checklist]

@alteriom-ai-agent Implement shared gateway per checklist
[Agent implements all items]

@painlessmesh-coordinator Verify shared gateway complete
[Coordinator confirms]
```

### Example 3: Cross-Repository
```
@alteriom-ai-agent Update CI workflow template across all Alteriom repos
```
→ Org-level agent handles cross-repo task

### Example 4: Specialized Optimization
```
@mesh-dev-agent Recommend memory optimizations for ESP8266

@alteriom-ai-agent Apply recommended optimizations
```

## Success Metrics

Two-tier architecture is successful when:
- ✅ Features are implemented completely (code + tests + docs + examples)
- ✅ Repository conventions are consistently followed
- ✅ No tasks require follow-up for missing components
- ✅ Cross-repo changes are coordinated properly
- ✅ User satisfaction with completeness

## Future Enhancements

Potential additions:
- More specialized domain agents
- Additional repo coordinators for other repositories
- Enhanced cross-repo coordination
- Automated checklist validation
- Integration with project management tools

## References

- Alteriom AI Agent: `.github/agents/alteriom-ai-agent.md`
- painlessMesh Coordinator: `.github/agents/painlessmesh-coordinator.md`
- Agent README: `.github/agents/README.md`
- MCP Configuration: `.github/mcp-server.json`
- Custom MCP Server: `.github/mcp-painlessmesh.js`
