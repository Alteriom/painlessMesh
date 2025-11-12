# Quick Reference: Version Management

## 🎯 Where to Find Version Information

| File | Purpose | When to Update |
|------|---------|----------------|
| `library.properties` | **Official Arduino version** | Every release |
| `library.json` | **Official PlatformIO version** | Every release |
| `package.json` | **Official NPM version** | Every release |
| `src/painlessMesh.h` | Header documentation | Every release (recommended) |
| `src/AlteriomPainlessMesh.h` | Version defines | Every release (recommended) |
| `CHANGELOG.md` | Release history | Every release |

## 🚀 Quick Release Checklist

```bash
# 1. Bump version
./scripts/bump-version.sh patch  # or minor, major

# 2. Update CHANGELOG.md
# Move [Unreleased] items to new [X.Y.Z] section

# 3. Update header files
# Edit src/painlessMesh.h - update @version comment
# Edit src/AlteriomPainlessMesh.h - update version defines

# 4. Validate
./scripts/release-agent.sh

# 5. Commit and push
git add library.properties library.json package.json CHANGELOG.md src/*.h
git commit -m "release: vX.Y.Z - Brief description"
git push origin main
```

## 📝 Version Comment Format

### In `src/painlessMesh.h`:
```cpp
/**
 * @file painlessMesh.h
 * @brief Main header file for Alteriom painlessMesh library
 * 
 * @version 1.8.7      // ← Library version
 * @date 2025-11-12    // ← Current date
 * 
 * painlessMesh is a user-friendly library...
 */
```

### In `src/AlteriomPainlessMesh.h`:
```cpp
/**
 * @brief AlteriomPainlessMesh library version information
 */
#define ALTERIOM_PAINLESS_MESH_VERSION "1.8.7"
#define ALTERIOM_PAINLESS_MESH_VERSION_MAJOR 1
#define ALTERIOM_PAINLESS_MESH_VERSION_MINOR 8
#define ALTERIOM_PAINLESS_MESH_VERSION_PATCH 7
```

## 🔍 Quick Commands

### Check current version:
```bash
grep "version=" library.properties
```

### Verify all versions match:
```bash
grep -E "version|VERSION" library.properties library.json package.json src/*.h
```

### See file modification history:
```bash
git log --oneline -- src/painlessMesh.h
```

### See last modification date:
```bash
git log -1 --format="%ai" -- src/painlessMesh.h
```

## ❓ Common Questions

**Q: What version is the library?**  
→ Check `library.properties`, line 2

**Q: When was a file last modified?**  
→ Use `git log -1 -- path/to/file`

**Q: Why is header version different from library.properties?**  
→ Header comments may not have been updated. Trust `library.properties`.

**Q: Do I need to update header versions?**  
→ Recommended but not critical. Update during releases.

## 📚 Full Documentation

For complete information:
- **[VERSION_MANAGEMENT.md](VERSION_MANAGEMENT.md)** - Complete guide
- **[FAQ_VERSION_NUMBERS.md](FAQ_VERSION_NUMBERS.md)** - Common questions
- **[RELEASE_GUIDE.md](../RELEASE_GUIDE.md)** - Release process

## 🎓 Key Principles

1. **Single Source of Truth**: `library.properties` / `library.json` / `package.json`
2. **Header Comments**: Documentation only, not source of truth
3. **Git History**: Actual record of file modifications
4. **Synchronize on Release**: Keep all versions aligned during releases
5. **When in Doubt**: Check `library.properties`

---

**Quick Access:**
```bash
# View this file
cat docs/QUICK_REFERENCE_VERSIONING.md

# View full guide
cat docs/VERSION_MANAGEMENT.md

# View FAQ
cat docs/FAQ_VERSION_NUMBERS.md
```

**Last Updated:** November 12, 2025
