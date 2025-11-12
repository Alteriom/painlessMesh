# Publishing Agents to GitHub Marketplace

This document explains how to make your Copilot agents visible in GitHub's agent directory.

## Current Status

✅ **Configured** - Agents are configured in `copilot-agents.json`
⏳ **Publishing** - Awaiting GitHub Enterprise/Organization setup

## What's Needed

### Prerequisites

1. **GitHub Enterprise or Organization Account**
   - Personal repositories can use agents locally
   - Enterprise/Org accounts can publish to marketplace
   - Check: https://github.com/organizations/Alteriom/settings/copilot

2. **Copilot Business or Enterprise License**
   - Required for agent publishing
   - Contact GitHub Sales if needed

3. **Agent Configuration** ✅
   - Already configured in `copilot-agents.json`
   - Publisher information added
   - Visibility set to "public"

## Publishing Steps

### Step 1: Organization Setup

1. Go to your organization settings:
   ```
   https://github.com/organizations/Alteriom/settings/copilot
   ```

2. Navigate to **Copilot** → **Agents**

3. Click **"Add Agent Repository"**

4. Select `Alteriom/painlessMesh`

### Step 2: Verify Agent Configuration

Your agents are configured with:

- ✅ Publisher information (Alteriom)
- ✅ Repository URL
- ✅ Public visibility
- ✅ Categories assigned
- ✅ Comprehensive documentation

### Step 3: Enable Agent Discovery

Once linked, your agents will appear at:

```
https://github.com/copilot/agents
```

Search by:
- **Category**: development-tools, code-generation, testing, documentation
- **Tags**: mesh-networking, esp8266, esp32, embedded, iot
- **Organization**: Alteriom

### Step 4: Test Agent Access

After publishing, users can:

1. **Discover** - Find in GitHub's agent marketplace
2. **Install** - Add to their Copilot environment
3. **Use** - Invoke with `@agent-name` syntax

## Agent Visibility Levels

### Current Configuration

All agents are set to **"public"** visibility:

```json
{
  "id": "release-agent",
  "visibility": "public",
  ...
}
```

**Visibility Options:**
- `public` - Anyone can discover and use
- `organization` - Only organization members
- `private` - Only repository collaborators

### Changing Visibility

Edit `copilot-agents.json` and change the `visibility` field:

```json
{
  "id": "mesh-dev-agent",
  "visibility": "organization",  // Changed from public
  ...
}
```

## Agent Categories

Your agents are categorized for discovery:

| Agent | Category | Why |
|-------|----------|-----|
| Release Agent | `development-tools` | Build/release automation |
| Mesh Dev Agent | `code-generation` | Code scaffolding and patterns |
| Testing Agent | `testing` | Test generation and validation |
| Docs Agent | `documentation` | Documentation creation |

## Verification Checklist

Before publishing, verify:

- [x] `copilot-agents.json` has publisher information
- [x] All agents have `visibility` field set
- [x] All agents have `category` field
- [x] Documentation exists in `.github/agents/`
- [x] Examples are provided for each agent
- [x] README explains agent usage
- [ ] GitHub Organization has Copilot enabled
- [ ] Repository is linked in Org settings
- [ ] Agents appear in marketplace

## Alternative: Local/Private Use

If you can't publish to GitHub's marketplace yet, agents still work:

### Repository-Level Access
- Anyone who opens this repository gets the agents
- Works in VS Code and GitHub Codespaces
- Requires repository checkout

### Organization-Level Access
- Set visibility to `organization`
- All org members can access
- Even in other repositories (if configured)

## Troubleshooting

### Agents Don't Appear in Marketplace

**Check:**
1. Organization has Copilot Business/Enterprise
2. Repository is linked in Org settings
3. `copilot-agents.json` is valid JSON
4. `visibility` is set to `public` or `organization`

**Solution:**
- Verify at: https://github.com/organizations/Alteriom/settings/copilot
- Check workflow logs: `.github/workflows/publish-agents.yml`

### Agents Work Locally But Not in GitHub UI

**Issue:** Agents need repository context

**Solution:**
- Open repository in Codespaces (full context)
- Link to specific files in PR/Issue queries
- Use `@agent-name` with detailed context

### Users Can't Find Agents

**Issue:** Discovery/search not working

**Check:**
1. Agents published successfully
2. Categories and tags are correct
3. Descriptions are clear and searchable
4. Visibility allows user access

## GitHub App Setup (Optional)

For advanced integration, create a GitHub App:

1. Visit: https://github.com/organizations/Alteriom/settings/apps/new

2. Use manifest from: `github-app-manifest.json`

3. Configure permissions:
   - Contents: read
   - Issues: write
   - Pull requests: write

4. Link app to agent repository

## Publishing Workflow

The repository includes an automated workflow:

**File:** `.github/workflows/publish-agents.yml`

**Triggers:**
- Push to main branch (changes to agent files)
- Manual workflow dispatch

**Actions:**
1. Validates `copilot-agents.json`
2. Generates documentation
3. Prepares for publication
4. Creates GitHub App manifest

**Run manually:**
```bash
gh workflow run publish-agents.yml
```

## Support

### For Publishing Issues
- GitHub Support: https://support.github.com/
- Copilot Enterprise docs: https://docs.github.com/en/copilot/
- Organization admins: Check Copilot settings

### For Agent Development
- See: `.github/COPILOT_AGENTS_GUIDE.md`
- See: `.github/AGENTS_INDEX.md`
- See: `.github/agents/` for individual agent docs

## Next Steps

1. **Contact GitHub** - Verify Copilot Enterprise access
2. **Link Repository** - Connect in Org settings
3. **Test Publication** - Verify agents appear
4. **Share** - Let organization members know
5. **Iterate** - Update based on feedback

---

**Last Updated:** November 11, 2024  
**Status:** Configured, awaiting publication  
**Contact:** Alteriom Organization Admins
