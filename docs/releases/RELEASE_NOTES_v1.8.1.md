# painlessMesh v1.8.1 Release Notes

**Release Date:** November 10, 2025  
**Version:** 1.8.1  
**Type:** Patch Release  
**Compatibility:** 100% backward compatible with v1.8.0

---

## 🎯 Executive Summary

Version 1.8.1 is a patch release that enhances developer experience by adding GitHub Copilot custom agent support. This release makes the Release Agent discoverable to GitHub Copilot, enabling AI-assisted release management for all developers working with the repository.

### Key Highlights

✨ **GitHub Copilot Integration** - Custom agent now discoverable by GitHub Copilot  
🤖 **AI-Assisted Release Management** - Release Agent available in Copilot Chat  
📚 **Enhanced Developer Context** - Improved repository context for all Copilot users  
🔧 **Zero Breaking Changes** - Purely additive improvements

---

## 🚀 What's New

### 1. GitHub Copilot Custom Agent Support

The Release Agent configuration is now properly exposed for GitHub Copilot discovery.

**Key Features:**
- ✅ `copilot-agents.json` moved to repository root for automatic discovery
- ✅ Release Agent available as `@release-agent` in GitHub Copilot Chat (Enterprise)
- ✅ Enhanced repository context for all GitHub Copilot users
- ✅ Complete agent documentation in `.github/agents/` directory
- ✅ Knowledge sources properly linked for context-aware assistance

**For GitHub Copilot Enterprise Users:**
```
@release-agent How do I prepare a release?
@release-agent Check version consistency
@release-agent What validation checks do you perform?
```

**For All GitHub Copilot Users:**
Enhanced context includes:
- Release validation procedures
- Version management best practices
- CHANGELOG format guidelines
- Automated release workflow details

**Configuration:**
The agent is configured with:
- **Capabilities**: release-validation, version-management, changelog-validation, test-execution, git-operations, documentation-verification
- **Knowledge Sources**: Release agent spec, RELEASE_GUIDE.md, validation scripts, workflows
- **Scope**: Repository-wide assistance

**Documentation:** `.github/COPILOT_AGENT_SETUP.md`, `.github/AGENTS_INDEX.md`

---

## 📝 Changes

### Added

- **GitHub Copilot Custom Agent Support** - Custom agent configuration now discoverable by GitHub
  - Moved `copilot-agents.json` to repository root for automatic GitHub Copilot integration
  - Release Agent now available as `@release-agent` in GitHub Copilot Chat (Enterprise)
  - Enhanced repository context for all GitHub Copilot users
  - Complete agent documentation in `.github/agents/` directory

### Changed

- **Documentation Updates** - Improved clarity for custom agent setup
  - Updated `COPILOT_AGENT_SETUP.md` with root file location
  - Enhanced `AGENTS_INDEX.md` with discovery information
  - Added examples for using custom agents in development workflow

### Fixed

- **Custom Agent Visibility** - Resolved issue where custom agent tasks were not showing in GitHub
  - GitHub Copilot now automatically discovers the release agent configuration
  - Agent appears in Copilot Chat suggestions when available
  - Knowledge sources properly linked for enhanced context

---

## 🛠️ Technical Details

### File Changes

**New Files:**
- `copilot-agents.json` - Custom agent configuration (repository root)

**Updated Files:**
- `library.properties` - Version bumped to 1.8.1
- `library.json` - Version bumped to 1.8.1
- `package.json` - Version bumped to 1.8.1
- `CHANGELOG.md` - Added v1.8.1 entry
- `.github/COPILOT_AGENT_SETUP.md` - Updated file location references
- `.github/AGENTS_INDEX.md` - Added configuration file location

### GitHub Copilot Agent Configuration

```json
{
  "$schema": "https://github.com/github/copilot-schemas/blob/main/schemas/copilot-agent-config.schema.json",
  "version": "1.0",
  "agents": [
    {
      "id": "release-agent",
      "name": "Release Agent",
      "description": "Assists with release management, version validation, and quality assurance for AlteriomPainlessMesh releases",
      "scope": "repository",
      "knowledge_sources": [
        ".github/agents/release-agent.md",
        ".github/agents/README.md",
        ".github/AGENTS_INDEX.md",
        "RELEASE_GUIDE.md",
        "scripts/release-agent.sh",
        "scripts/bump-version.sh",
        ".github/workflows/release.yml",
        ".github/workflows/validate-release.yml"
      ]
    }
  ]
}
```

---

## 📦 Distribution

This release is available through all standard distribution channels:

- **GitHub Releases**: [v1.8.1](https://github.com/Alteriom/painlessMesh/releases/tag/v1.8.1)
- **NPM**: `npm install @alteriom/painlessmesh@1.8.1`
- **GitHub Packages**: Available with authentication
- **PlatformIO**: `alteriom/AlteriomPainlessMesh@^1.8.1`
- **Arduino Library Manager**: Search for "AlteriomPainlessMesh"

---

## 🔄 Upgrade Guide

### From v1.8.0 to v1.8.1

This is a seamless upgrade with no breaking changes:

1. **Update Package Version:**
   ```bash
   # PlatformIO
   pio pkg update alteriom/AlteriomPainlessMesh@^1.8.1
   
   # NPM
   npm update @alteriom/painlessmesh
   
   # Arduino Library Manager
   # Update through IDE's Library Manager
   ```

2. **No Code Changes Required** - All existing code works without modification

3. **Enjoy GitHub Copilot Integration** (if enabled)

---

## 📚 Documentation

### Updated Documentation

- **COPILOT_AGENT_SETUP.md** - GitHub Copilot custom agent setup guide
- **AGENTS_INDEX.md** - Complete agent documentation index
- **RELEASE_GUIDE.md** - Comprehensive release process guide

### Key Resources

- [Release Agent Specification](/.github/agents/release-agent.md)
- [Agent Index](/.github/AGENTS_INDEX.md)
- [Copilot Setup Guide](/.github/COPILOT_AGENT_SETUP.md)
- [Release Guide](/RELEASE_GUIDE.md)
- [Complete Changelog](/CHANGELOG.md)

---

## ✅ Quality Assurance

### Release Validation

- ✅ **21 validation checks passed** (release-agent.sh)
- ✅ **All tests passing** (100+ test assertions)
- ✅ **Build successful** (100/100 targets)
- ✅ **Version consistency verified** across all package files
- ✅ **JSON configuration validated** (copilot-agents.json)
- ✅ **Security checks passed** (CodeQL analysis)

### Testing Summary

- **Unit Tests**: All passing
- **Build System**: CMake + Ninja successful
- **Platform Tests**: ESP32, ESP8266 compatible
- **Integration Tests**: Boost.Asio tests passing

---

## 🙏 Acknowledgments

Thanks to all contributors who helped make this release possible:
- @sparck75 for reviewing and requesting documentation updates
- GitHub Copilot team for the custom agent framework

---

## 📞 Support

- **Issues**: [GitHub Issues](https://github.com/Alteriom/painlessMesh/issues)
- **Discussions**: [GitHub Discussions](https://github.com/Alteriom/painlessMesh/discussions)
- **Documentation**: [https://alteriom.github.io/painlessMesh/](https://alteriom.github.io/painlessMesh/)
- **Wiki**: [GitHub Wiki](https://github.com/Alteriom/painlessMesh/wiki)

---

**Full Changelog**: [v1.8.0...v1.8.1](https://github.com/Alteriom/painlessMesh/compare/v1.8.0...v1.8.1)
