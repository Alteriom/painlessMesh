# Version Management in AlteriomPainlessMesh

This document explains how versioning works in the AlteriomPainlessMesh library and clarifies common questions about version numbers in different files.

## 📋 Version Number Locations

The library version is maintained in multiple files across the repository:

### 1. **Official Version Files** (Source of Truth)

These files define the official library version:

- **`library.properties`** - Arduino Library Manager version
- **`library.json`** - PlatformIO Library Registry version  
- **`package.json`** - NPM package version

**All three files must always have the same version number.**

### 2. **Header File Version Comments** (Documentation)

These are **documentation comments** that indicate when the header file documentation was last updated:

- **`src/painlessMesh.h`** - `@version` in header comment
- **`src/AlteriomPainlessMesh.h`** - `ALTERIOM_PAINLESS_MESH_VERSION` defines

**Important:** Version numbers in header file comments reflect the overall library version at the time the header was documented, not file-specific versioning.

## ❓ Common Questions

### Q: Does the version in `painlessMesh.h` mean the file hasn't changed since that version?

**A: No.** The version comment in header files indicates the library version when the header documentation was last reviewed/updated, not the last time the file was modified.

**Example:**
```cpp
/**
 * @file painlessMesh.h
 * @version 1.8.7
 * @date 2025-11-12
 */
```

This means:
- ✅ The library version is 1.8.7
- ✅ The header documentation is current as of version 1.8.7
- ❌ It does NOT mean the file hasn't been modified since 1.8.7

### Q: Why might header version comments be out of sync?

**A:** During rapid development, header file documentation may not be updated every release. The comments are updated when:
- Significant API changes are made
- Documentation requires updating
- Major version milestones are reached
- Version consistency review is performed

### Q: Which version number should I trust?

**A:** Always refer to the official version files:
1. `library.properties` - Official Arduino version
2. `library.json` - Official PlatformIO version
3. `package.json` - Official NPM version
4. GitHub releases - Tagged release versions

Header file comments are for documentation reference only.

## 🔄 Version Update Process

### When Releasing a New Version:

1. **Update Official Version Files** (required)
   ```bash
   ./scripts/bump-version.sh patch  # or minor, major
   ```
   This updates: `library.properties`, `library.json`, `package.json`

2. **Update CHANGELOG.md** (required)
   - Move items from `[Unreleased]` to new version section
   - Add release date

3. **Update Header File Comments** (recommended)
   - Update `@version` in `src/painlessMesh.h`
   - Update version defines in `src/AlteriomPainlessMesh.h`

4. **Commit and Tag** (required)
   ```bash
   git commit -m "release: vX.Y.Z - Brief description"
   git push origin main
   ```
   GitHub Actions will automatically create the tag.

## 📝 Version Comment Best Practices

### In Header Files:

**Good Practice:**
```cpp
/**
 * @file painlessMesh.h
 * @brief Main header file for Alteriom painlessMesh library
 * 
 * @version 1.8.7
 * @date 2025-11-12
 * 
 * painlessMesh is a user-friendly library for creating mesh networks...
 */
```

**What This Means:**
- The library is at version 1.8.7
- Header documentation was reviewed/updated on 2025-11-12
- Always synchronized with library version during releases

### In Implementation Files:

Implementation files (`.cpp`, `.hpp`) typically do not need version comments. Version information in these files can be misleading and is unnecessary since:
- Git history tracks all changes with timestamps
- Version is centrally managed in the official version files
- Per-file versioning creates maintenance overhead

## 🎯 Version Management Workflow

### Developer Workflow:

1. **Check Current Version**
   ```bash
   grep "version=" library.properties
   ```

2. **Make Changes**
   - Implement features/fixes
   - Update documentation as needed
   - Add entries to CHANGELOG.md under `[Unreleased]`

3. **Prepare Release**
   ```bash
   ./scripts/release-agent.sh  # Validate release readiness
   ./scripts/bump-version.sh patch  # Update version
   ```

4. **Update Documentation**
   - Review and update header file version comments
   - Ensure CHANGELOG.md has the new version section
   - Verify all documentation references are current

5. **Release**
   ```bash
   git add library.properties library.json package.json CHANGELOG.md src/*.h
   git commit -m "release: v1.8.7 - Brief description"
   git push origin main
   ```

### Automated Process:

GitHub Actions automatically handles:
- ✅ Git tag creation
- ✅ GitHub release with notes
- ✅ NPM publishing
- ✅ PlatformIO registry update
- ✅ Documentation deployment

## 🔍 Version History Tracking

### To Check File History:

Use Git to see actual file modification history:

```bash
# See all commits that modified a file
git log --oneline -- src/painlessMesh.h

# See detailed changes to a file
git log -p -- src/painlessMesh.h

# See when a file was last modified
git log -1 --format="%ai %an" -- src/painlessMesh.h
```

### To Check Version History:

```bash
# List all version tags
git tag -l "v*"

# See changes in a specific version
git show v1.8.7

# Compare two versions
git diff v1.8.6..v1.8.7
```

## 📚 Related Documentation

- **[CHANGELOG.md](../CHANGELOG.md)** - Complete version history with changes
- **[RELEASE_GUIDE.md](../RELEASE_GUIDE.md)** - Detailed release process
- **[GitHub Releases](https://github.com/Alteriom/painlessMesh/releases)** - Official release notes

## 🎓 Summary

**Key Takeaways:**

1. **Official version** = `library.properties` / `library.json` / `package.json`
2. **Header comments** = Documentation reference, not file-specific versions
3. **Git history** = Actual source of truth for file modifications
4. **CHANGELOG.md** = Human-readable version history
5. **GitHub releases** = Tagged versions with release notes

**When in doubt:** Check `library.properties` for the official library version, and use `git log` to see actual file modification history.

---

**Version Management Process Owner:** @Alteriom  
**Last Updated:** 2025-11-12  
**Document Version:** 1.0
