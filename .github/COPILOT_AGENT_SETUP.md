# GitHub Copilot Agent Setup for AlteriomPainlessMesh

This guide explains how to set up and use the Release Agent as a GitHub Copilot custom agent (for GitHub Enterprise Cloud users) or through enhanced repository context (for all users).

## Overview

AlteriomPainlessMesh provides Release Agent functionality in three ways:

1. **GitHub Copilot Enterprise Agent** - AI assistant in Copilot Chat (Enterprise only)
2. **Enhanced Repository Context** - Automatic Copilot knowledge (all users)
3. **Scripts & Automation** - Executable tools and CI/CD (all users)

## Option 1: GitHub Copilot Enterprise Agent (Recommended for Enterprise)

### Requirements

- GitHub Enterprise Cloud subscription
- GitHub Copilot for Business enabled
- Organization admin access

### Setup Steps

#### Step 1: Verify Enterprise Access

Check if your organization has GitHub Copilot Enterprise:

1. Go to your organization settings
2. Navigate to "Copilot" section
3. Look for "Custom Agents" or "Copilot Extensions"

If you see these options, proceed to Step 2. If not, use Option 2 or 3 instead.

#### Step 2: Create Custom Agent

**Via GitHub UI:**

1. Go to: `https://github.com/organizations/[YOUR_ORG]/settings/copilot/agents`
2. Click "New Agent"
3. Configure the agent:

   **Basic Information:**
   - **Name**: `release-agent`
   - **Display Name**: Release Agent
   - **Description**: Assists with release management, version validation, and quality assurance for AlteriomPainlessMesh releases

   **Instructions:**
   Copy the instructions from `.github/copilot-agents.json` or use the following condensed version:

   ```
   You are the AlteriomPainlessMesh Release Agent. Help developers prepare releases by:
   
   - Checking version consistency (library.properties, library.json, package.json)
   - Validating CHANGELOG.md entries
   - Verifying tests pass: cmake -G Ninja . && ninja && run-parts --regex catch_ bin/
   - Ensuring clean git status
   - Running ./scripts/release-agent.sh for validation
   
   Always guide through the complete release checklist in .github/agents/release-agent.md
   ```

   **Knowledge Sources:**
   - Repository: `Alteriom/painlessMesh`
   - Files to include:
     - `.github/agents/release-agent.md`
     - `.github/AGENTS_INDEX.md`
     - `RELEASE_GUIDE.md`
     - `scripts/release-agent.sh`

   **Scope**: Repository
   
   **Visibility**: All organization members

4. Click "Create Agent"

#### Step 3: Use the Agent

In GitHub Copilot Chat:

```
@Alteriom/release-agent how do I prepare a release?
```

or in VS Code:

```
@Alteriom/release-agent check version consistency
```

### Verification

Test the agent is working:

1. Open Copilot Chat
2. Type `@` and look for `@Alteriom/release-agent` in suggestions
3. Ask: `@Alteriom/release-agent what checks do you perform?`
4. Agent should respond with release validation information

## Option 2: Enhanced Repository Context (All Users)

This option is automatically available to all GitHub Copilot users working in the repository.

### How It Works

GitHub Copilot automatically reads:
- `.github/copilot-instructions.md` - Complete agent instructions
- `.github/agents/release-agent.md` - Detailed specifications
- `.github/AGENTS_INDEX.md` - Agent index
- All documentation files

### Using Enhanced Context

Simply ask Copilot questions about releases:

**In Copilot Chat:**
```
How do I prepare a release for painlessMesh?
What does the release agent check?
Help me validate my release
```

**In Code Comments:**
```cpp
// TODO: prepare release 1.8.1
// [Copilot will suggest release steps based on context]
```

**In Terminal:**
```bash
# How to validate release?
# [Copilot will suggest: ./scripts/release-agent.sh]
```

### Verification

1. Open a file in the repository
2. Ask Copilot: "How do I validate a release?"
3. Response should reference release-agent.sh and validation steps

## Option 3: Scripts & Automation (All Users)

Use the Release Agent as executable scripts and CI/CD automation.

### Available Scripts

**Release Validation:**
```bash
./scripts/release-agent.sh
```

**Version Management:**
```bash
./scripts/bump-version.sh patch 1.8.1
```

**Basic Validation:**
```bash
./scripts/validate-release.sh
```

### CI/CD Integration

Release Agent is integrated into GitHub Actions:

**Validate Release Workflow:**
- File: `.github/workflows/validate-release.yml`
- Trigger: Every push to main/develop
- Action: Runs release validation

**Release Workflow:**
- File: `.github/workflows/release.yml`
- Trigger: Commits with `release:` prefix
- Action: Creates release, publishes packages

### Usage

1. **Manual Validation**
   ```bash
   cd /path/to/painlessMesh
   ./scripts/release-agent.sh
   ```

2. **In CI/CD**
   Already configured - runs automatically on every push

3. **Pre-commit Hook** (optional)
   ```bash
   # Add to .git/hooks/pre-commit
   #!/bin/bash
   if git log -1 --pretty=%B | grep -q "^release:"; then
       ./scripts/release-agent.sh || exit 1
   fi
   ```

## Comparison: Which Option to Use?

| Feature | Enterprise Agent | Enhanced Context | Scripts & Automation |
|---------|-----------------|------------------|---------------------|
| **Requirements** | Enterprise Cloud | Copilot license | None |
| **Setup Effort** | Medium (admin) | None (automatic) | None (pre-configured) |
| **AI Assistance** | Interactive chat | Inline suggestions | None |
| **Validation** | Via prompts | Via prompts | Direct execution |
| **CI/CD Integration** | No | No | Yes ✅ |
| **Availability** | Organization-wide | Repository-only | Always |
| **Best For** | Large teams | Individual devs | Automation |

### Recommendation

- **Have Enterprise Cloud?** → Use Option 1 + Option 3
- **Have Copilot only?** → Use Option 2 + Option 3
- **No Copilot?** → Use Option 3

**Ideal Setup:** Use all three options for maximum benefit!

## Troubleshooting

### Enterprise Agent Not Showing

**Problem:** `@Alteriom/release-agent` doesn't appear in agent list

**Solutions:**
1. Verify organization has Enterprise Cloud subscription
2. Check Copilot for Business is enabled
3. Confirm you have access to create agents
4. Try refreshing GitHub/VS Code
5. Check agent scope includes your repository

### Copilot Not Using Context

**Problem:** Copilot doesn't reference release agent in responses

**Solutions:**
1. Ensure you're working in the repository
2. Refresh VS Code workspace
3. Restart Copilot extension
4. Try explicit queries: "According to release-agent.md, how..."
5. Check `.github/copilot-instructions.md` exists

### Scripts Not Executable

**Problem:** Permission denied when running scripts

**Solution:**
```bash
chmod +x scripts/release-agent.sh
chmod +x scripts/bump-version.sh
chmod +x scripts/validate-release.sh
```

### Agent Gives Wrong Information

**Problem:** Agent provides outdated or incorrect info

**Solutions:**
1. Update repository (agent learns from latest docs)
2. For Enterprise: Update agent knowledge sources
3. For Context: Ensure copilot-instructions.md is current
4. Report issue with `agent` label

## Advanced Configuration

### Customizing Enterprise Agent

Edit agent in GitHub organization settings:

1. Update instructions for team-specific workflows
2. Add additional knowledge sources
3. Customize examples for common scenarios
4. Set visibility (organization, team, or repository)

### Extending Repository Context

Add team-specific information to `.github/copilot-instructions.md`:

```markdown
## Team-Specific Release Process

[Your custom instructions]
```

### Creating Additional Agents

Follow the pattern in `.github/copilot-agents.json` to create:

- Test Agent
- Documentation Agent
- Security Agent
- Migration Agent

See `.github/AGENTS_INDEX.md` for planning future agents.

## Examples

### Example 1: Preparing a Release (Enterprise Agent)

```
Developer: @Alteriom/release-agent I want to release version 1.8.1

Agent: To release version 1.8.1, follow these steps:

1. Validate current state:
   ./scripts/release-agent.sh

2. If validation passes:
   git add .
   git commit -m "release: v1.8.1 - [description]"
   git push origin main

3. GitHub Actions will automatically:
   - Create tag v1.8.1
   - Build and publish to NPM
   - Update documentation
   - Create GitHub release

Need help with any specific step?
```

### Example 2: Version Mismatch (Enhanced Context)

```
Developer: [In Copilot Chat] version numbers don't match, how to fix?

Copilot: To fix version mismatch in painlessMesh, use the bump-version script:

./scripts/bump-version.sh patch 1.8.1

This will synchronize versions across:
- library.properties
- library.json  
- package.json

Then run ./scripts/release-agent.sh to verify.
```

### Example 3: CI/CD Validation (Scripts)

```bash
# In GitHub Actions workflow
- name: Validate Release
  run: |
    ./scripts/release-agent.sh
    if [ $? -ne 0 ]; then
      echo "Release validation failed"
      exit 1
    fi
```

## Resources

### Documentation
- [Agent Index](.github/AGENTS_INDEX.md) - All agents and tools
- [Release Agent Spec](.github/agents/release-agent.md) - Complete specification
- [Release Guide](../RELEASE_GUIDE.md) - Detailed release process
- [Copilot Instructions](.github/copilot-instructions.md) - Full Copilot context

### Scripts
- `scripts/release-agent.sh` - Main validation script
- `scripts/bump-version.sh` - Version management
- `scripts/validate-release.sh` - Basic validation

### Workflows
- `.github/workflows/validate-release.yml` - Validation automation
- `.github/workflows/release.yml` - Release automation

## Getting Help

### For Setup Issues
1. Check this guide for troubleshooting
2. Review [AGENTS_INDEX.md](.github/AGENTS_INDEX.md)
3. Open issue with `agent` label

### For Usage Questions
1. Ask the agent (if configured)
2. Check [release-agent.md](.github/agents/release-agent.md)
3. Review [RELEASE_GUIDE.md](../RELEASE_GUIDE.md)

### For Bug Reports
1. Run with verbose mode: `./scripts/release-agent.sh --verbose`
2. Collect output and logs
3. Open issue with reproduction steps

---

**Last Updated:** November 10, 2024  
**Maintained By:** AlteriomPainlessMesh Team  
**Questions?** Open an issue or ask your agent!
