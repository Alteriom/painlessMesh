# Custom Agent Visibility Analysis

## Question

"why I don't see the custom agent in the list in GitHub?"

## Investigation Summary

The `.github/agents/` directory contains documentation files, not GitHub Copilot custom agent configurations.

## Current State

### What Exists in the Repository

#### 1. Agent Documentation Directory

```
.github/agents/
├── README.md           - Documentation index for release processes
└── release-agent.md    - Release agent specification (328 lines)
```

#### 2. Release Agent Script

```
scripts/release-agent.sh - Executable shell script (290 lines)
```

#### 3. Supporting Documentation

```
.github/copilot-instructions.md
.github/copilot-quick-reference.md
.github/copilot-troubleshooting.md
```

### What These Files Are

**Purpose:** Documentation and automation tools

1. **release-agent.md** - Human-readable specification
   - Pre-release validation checklist
   - Release process workflow
   - Error recovery procedures
   - Configuration requirements

2. **release-agent.sh** - Automated validation script
   - Checks version consistency
   - Validates CHANGELOG entries
   - Verifies git state
   - Tests build system

3. **Documentation files** - Context for developers
   - Coding guidelines
   - Project conventions
   - Troubleshooting guides

### What These Files Are NOT

❌ **GitHub Copilot Custom Agents**
- Not AI assistants
- Not available in GitHub UI
- Not executable by Copilot directly

❌ **GitHub Actions Workflows**
- Not automated CI/CD (though used by workflows)
- Not triggered by GitHub events directly

❌ **Copilot Extensions/Plugins**
- Not MCP servers
- Not tool extensions

## Understanding GitHub Copilot Agents

### Types of "Agents" in Context

#### 1. GitHub Copilot Custom Agents (Enterprise Feature)

**What they are:**
- AI assistants with specialized knowledge
- Available in GitHub Copilot Chat
- Require GitHub Enterprise Cloud + Copilot for Business
- Configured at organization level

**How they're created:**
- Organization admins create agents
- Define agent purpose and instructions
- Specify knowledge sources
- Available to all org members

**Example:**
```
@my-org/release-agent - "Help with release processes"
@my-org/security-agent - "Security review assistant"
```

**Visibility:**
- Appear in Copilot Chat agent list
- Prefixed with `@organization-name/agent-name`
- Available in GitHub UI, VS Code, etc.

**Requirements:**
- GitHub Enterprise Cloud subscription
- Copilot for Business license
- Organization admin access to create
- Specific configuration format

#### 2. GitHub Copilot Instructions (Repository Context)

**What they are:**
- Markdown files in `.github/` directory
- Provide context to Copilot
- Guide Copilot's responses
- Available in current repository

**How they work:**
- Copilot reads these files automatically
- Uses content as context for suggestions
- No explicit "agent" invocation needed
- Scoped to repository

**Example files:**
```
.github/copilot-instructions.md
.github/instructions/testing.instructions.md
```

**Visibility:**
- Not visible in agent list
- Automatically used by Copilot
- Influence all Copilot suggestions in repo

#### 3. Documentation "Agents" (This Repository)

**What they are:**
- Documentation about processes
- Specifications for manual procedures
- Reference guides for humans

**Purpose:**
- Document release procedures
- Provide checklists
- Guide manual processes

**The `.github/agents/` directory in this repository:**
- Documentation ABOUT agent-like processes
- Not actual AI agents
- For human reference primarily

## Why the Custom Agent Isn't Visible

### Possible Interpretations

#### Interpretation 1: Expecting GitHub Copilot Enterprise Agent

**If you expected to see:**
```
@Alteriom/release-agent
```

**Why it's not visible:**
1. **Not configured as Copilot Agent** - The release-agent.md is documentation, not an agent configuration
2. **Requires Enterprise** - Copilot custom agents need GitHub Enterprise Cloud
3. **Organization-level setup** - Must be created by org admins in GitHub settings
4. **Wrong location** - Agents aren't created by committing markdown to `.github/agents/`

**How to check if available:**
1. Open GitHub Copilot Chat
2. Type `@` to see available agents
3. Look for `@Alteriom/...` agents

#### Interpretation 2: Expecting Documentation Visibility

**If you expected to see:**
- Agent documentation in some GitHub UI
- List of available "agents" (processes)

**Why it might not be visible:**
1. **It's just documentation** - Files in `.github/agents/` are markdown docs
2. **No special GitHub UI** - GitHub doesn't render `.github/agents/` specially
3. **Manual navigation needed** - Browse to `.github/agents/` to see files

**How to access:**
1. Navigate to `.github/agents/` in repository
2. Read README.md for index
3. Open release-agent.md for specifications

#### Interpretation 3: Expecting Tool/Script Availability

**If you expected:**
- Automated tool in CI/CD
- Executable agent in workflows

**Where they actually are:**
1. **Script:** `scripts/release-agent.sh` (executable)
2. **Workflows:** `.github/workflows/validate-release.yml`
3. **Run manually:** `./scripts/release-agent.sh`

## How to Make a "Custom Agent" Visible

### Option 1: Create GitHub Copilot Enterprise Agent (Recommended if Enterprise)

**Requirements:**
- GitHub Enterprise Cloud account
- Copilot for Business subscription
- Organization admin privileges

**Steps:**
1. Go to organization settings
2. Navigate to Copilot settings
3. Create new custom agent
4. Name it `release-agent`
5. Provide instructions from `.github/agents/release-agent.md`
6. Specify knowledge sources (this repository)
7. Make available to organization

**Result:**
- `@Alteriom/release-agent` appears in Copilot Chat
- All org members can use `@Alteriom/release-agent` for help
- Agent has knowledge of release processes

### Option 2: Move to Copilot Instructions (Easier, Works Now)

**Benefit:**
- Available immediately
- No Enterprise required
- Works in any repository with Copilot

**Steps:**
1. Move key content from `.github/agents/release-agent.md`
2. Add to `.github/copilot-instructions.md`
3. Format as instructions for Copilot

**Example addition to copilot-instructions.md:**
```markdown
## Release Agent Instructions

When asked about releases or deployment:
- Check version consistency across library.properties, library.json, package.json
- Verify CHANGELOG.md has entry for version
- Ensure all tests pass
- Validate git status (no uncommitted changes)
- Run ./scripts/release-agent.sh for validation
- Follow checklist in .github/agents/release-agent.md
```

**Result:**
- Copilot automatically uses this context
- No special syntax needed
- Works for all repository contributors

### Option 3: Keep as Documentation (Current State)

**Benefit:**
- Already working
- Clear documentation
- Human-readable
- Works with scripts

**Usage:**
1. Developers read `.github/agents/release-agent.md`
2. Run `./scripts/release-agent.sh` for validation
3. Follow documented procedures

**Visibility:**
- In repository file structure
- Linked from README if desired
- Available in git

## Recommendations

### Immediate Action: Clarify Use Case

**Please specify what you need:**

1. **GitHub Copilot Enterprise Agent?**
   - Question: Do you have GitHub Enterprise Cloud?
   - Question: Do you want `@Alteriom/release-agent` in Copilot Chat?
   - Action: If yes, create through GitHub organization settings

2. **Improve Copilot Context?**
   - Question: Do you want Copilot to know about release processes?
   - Action: Move content to `.github/copilot-instructions.md`

3. **Better Documentation Visibility?**
   - Question: Should agents be easier to find?
   - Action: Add links to README, update documentation

4. **Automated Tool Integration?**
   - Question: Should agents run in CI/CD automatically?
   - Action: Already done - `.github/workflows/validate-release.yml`

### Suggested Improvements (Any Scenario)

#### 1. Add Link to README

**In main README.md:**
```markdown
## Development

- [Release Agent Documentation](.github/agents/release-agent.md)
- Run release validation: `./scripts/release-agent.sh`
```

#### 2. Enhance Copilot Instructions

**Add to .github/copilot-instructions.md:**
```markdown
## Release Process

For release-related questions:
1. Consult .github/agents/release-agent.md
2. Run validation: ./scripts/release-agent.sh
3. Follow documented checklist
```

#### 3. Create Agent Index

**New file: `.github/AGENTS.md`:**
```markdown
# Available Agents and Tools

## Release Agent
- **Documentation:** [release-agent.md](agents/release-agent.md)
- **Script:** `scripts/release-agent.sh`
- **Workflow:** `.github/workflows/validate-release.yml`
```

## Current Status

### What Works Now ✅

1. **Documentation accessible:**
   - Files in `.github/agents/` directory
   - Readable markdown
   - Clear specifications

2. **Scripts executable:**
   - `scripts/release-agent.sh` works
   - Validates releases
   - Integrated with CI/CD

3. **Workflows active:**
   - validate-release.yml runs automatically
   - Uses release agent logic

4. **Copilot context available:**
   - Repository instructions present
   - Copilot reads `.github/` files
   - Provides contextual help

### What Doesn't Work ❌

1. **No explicit agent list in GitHub UI**
   - `.github/agents/` not special to GitHub
   - No automatic agent registry

2. **Not GitHub Copilot Enterprise agents**
   - Can't invoke with `@Alteriom/release-agent`
   - Not in Copilot Chat agent list

3. **Manual discovery needed**
   - Must browse to `.github/agents/`
   - Not advertised in UI

## Conclusion

The "custom agent" documentation exists and works, but isn't visible as a GitHub Copilot enterprise agent because:

1. **It's documentation, not a configured agent**
2. **GitHub Copilot agents require Enterprise + organization setup**
3. **No automatic agent registry in GitHub for markdown files**

### Next Steps Needed

Please clarify what type of visibility you need:
1. GitHub Copilot Enterprise agent (`@Alteriom/release-agent`)?
2. Better documentation navigation?
3. Enhanced Copilot context?
4. Something else?

Based on your answer, I can:
- Set up Enterprise agent (if you have access)
- Improve documentation visibility
- Enhance Copilot instructions
- Add README links

---

**Analysis Date:** November 10, 2024
**Status:** Awaiting clarification on visibility requirements
