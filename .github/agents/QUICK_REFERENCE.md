# Quick Agent Reference

## TL;DR

**Want something done?** → `@alteriom-ai-agent` (has all the tools)
**Want to know what's needed?** → `@painlessmesh-coordinator` (knows everything about this repo)
**Want specialized advice?** → Use domain-specific agents

## The Agents

### 🌟 Alteriom AI Agent
**The Holy Grail** - Does everything, works everywhere

```
@alteriom-ai-agent <your request>
```

**Can do:**
- ✅ Modify any file
- ✅ Run any command  
- ✅ Create/update resources
- ✅ Work across multiple repos
- ✅ Execute complete workflows

**Best for:**
- Actually implementing things
- Running tests/builds
- Creating PRs
- Publishing releases
- Cross-repo operations

### 🎯 painlessMesh Coordinator
**The Checklist Expert** - Knows what's needed

```
@painlessmesh-coordinator <your question>
```

**Can do:**
- ✅ Create complete checklists
- ✅ Identify all related tasks
- ✅ Verify completeness
- ✅ Ensure conventions followed
- ✅ Catch missing components

**Best for:**
- Planning features: "What's needed for GPS?"
- Verifying work: "Is GPS implementation complete?"
- Understanding repo: "What must change when adding a package?"

### 📦 Release Agent
**Release management**

```
@release-agent Am I ready to release?
```

**Best for:**
- Pre-release checks
- Version validation
- Release preparation

### 🔧 Mesh Dev Agent
**ESP32/ESP8266 expert**

```
@mesh-dev-agent How to optimize memory for ESP8266?
```

**Best for:**
- Memory optimization
- Platform-specific code
- Mesh networking patterns
- Embedded debugging

### 📝 Docs Agent
**Documentation specialist**

```
@docs-agent Document the new GPS package
```

**Best for:**
- Writing documentation
- Creating examples
- API documentation
- Maintaining docs site

### ✅ Testing Agent
**Test generation**

```
@testing-agent Create tests for GPS serialization
```

**Best for:**
- Generating Catch2 tests
- Debugging test failures
- Test coverage
- Test patterns

## Common Workflows

### 1. Simple Task (Just Do It)
```
@alteriom-ai-agent Fix the typo in README.md line 42
```

### 2. New Feature (Plan → Execute → Verify)
```
# Step 1: What's needed?
@painlessmesh-coordinator What's needed to add GPS support?

# Step 2: Do it
@alteriom-ai-agent Implement GPS per coordinator checklist

# Step 3: Verify
@painlessmesh-coordinator Verify GPS implementation complete
```

### 3. Optimization (Consult → Execute)
```
# Step 1: Get recommendations
@mesh-dev-agent Recommend ESP8266 memory optimizations

# Step 2: Apply them
@alteriom-ai-agent Apply these optimizations
```

### 4. Release (Check → Fix → Release)
```
# Step 1: Check readiness
@release-agent Am I ready to release?

# Step 2: Fix issues (if any)
@alteriom-ai-agent Fix version inconsistencies

# Step 3: Release
@release-agent Prepare release v1.9.3
```

## Cheat Sheet

| I want to... | Use this agent |
|-------------|---------------|
| Add a new feature | `@painlessmesh-coordinator` → `@alteriom-ai-agent` |
| Fix a bug | `@alteriom-ai-agent` |
| Optimize code | `@mesh-dev-agent` → `@alteriom-ai-agent` |
| Create tests | `@testing-agent` → `@alteriom-ai-agent` |
| Write docs | `@docs-agent` → `@alteriom-ai-agent` |
| Prepare release | `@release-agent` |
| Update version | `@alteriom-ai-agent` |
| Run tests | `@alteriom-ai-agent` |
| Create PR | `@alteriom-ai-agent` |
| Verify completeness | `@painlessmesh-coordinator` |

## Pro Tips

### 💡 Coordinator First for Features
For anything complex:
```
@painlessmesh-coordinator What's needed to <feature>?
```
→ Get complete checklist before starting

### 💡 Direct Execution for Simple Stuff
No need to overthink:
```
@alteriom-ai-agent Update copyright year to 2025
```

### 💡 Verify Big Changes
After major work:
```
@painlessmesh-coordinator Verify <feature> is complete
```

### 💡 Specialize When Needed
Use domain experts for advice:
```
@mesh-dev-agent Best practice for mesh broadcast frequency?
@testing-agent What should I test for GPS package?
@docs-agent How should I document this API?
```

## Examples

### Adding a Sensor Package
```bash
# 1. Plan
@painlessmesh-coordinator What's needed for TemperaturePackage?

# 2. Implement
@alteriom-ai-agent Create TemperaturePackage per checklist

# 3. Test
@alteriom-ai-agent Run all tests

# 4. Verify
@painlessmesh-coordinator Verify TemperaturePackage complete
```

### Memory Optimization
```bash
# 1. Get recommendations
@mesh-dev-agent My ESP8266 runs out of memory, help?

# 2. Apply fixes
@alteriom-ai-agent Apply suggested memory optimizations

# 3. Verify
@alteriom-ai-agent Build for ESP8266 and check memory usage
```

### Release Preparation
```bash
# 1. Check readiness
@release-agent Ready for v1.9.3?

# 2. Fix issues
@alteriom-ai-agent Fix reported issues

# 3. Verify again
@release-agent Ready now?

# 4. Release
@alteriom-ai-agent Create release commit for v1.9.3
```

## Quick Commands

### Version Management
```bash
@alteriom-ai-agent Update version to 1.9.3 in all files
@alteriom-ai-agent Check version consistency
```

### Testing
```bash
@alteriom-ai-agent Run all unit tests
@alteriom-ai-agent Run tests matching "gps"
@testing-agent Generate tests for GpsPackage
```

### Building
```bash
@alteriom-ai-agent Build for ESP32
@alteriom-ai-agent Build for ESP8266
@alteriom-ai-agent Build all tests
```

### Documentation
```bash
@docs-agent Document GpsPackage API
@alteriom-ai-agent Update README with GPS example
@docs-agent Check all documentation links
```

## When Things Go Wrong

### "I'm not sure what's needed"
→ Ask the coordinator:
```
@painlessmesh-coordinator What should I know about <topic>?
```

### "Implementation incomplete"
→ Ask coordinator to verify:
```
@painlessmesh-coordinator What am I missing for <feature>?
```

### "Tests failing"
→ Get help from testing agent:
```
@testing-agent Why is catch_alteriom_packages failing?
```

### "Memory issues on ESP8266"
→ Consult mesh dev agent:
```
@mesh-dev-agent Help with ESP8266 memory optimization
```

### "Documentation unclear"
→ Ask docs agent:
```
@docs-agent Improve documentation for <component>
```

## Learn More

- **Two-Tier Architecture**: `.github/agents/TWO_TIER_ARCHITECTURE.md`
- **Agent README**: `.github/agents/README.md`
- **Alteriom AI Agent**: `.github/agents/alteriom-ai-agent.md`
- **Coordinator**: `.github/agents/painlessmesh-coordinator.md`

---

**Remember:** 
- Coordinator = Plans & Verifies
- Alteriom AI = Executes
- Specialists = Advise
