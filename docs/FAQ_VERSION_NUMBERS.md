# FAQ: Version Numbers in Header Files

## ❓ Question: Does the version in `painlessMesh.h` mean the file hasn't been altered since that version?

### Short Answer

**No.** The version comment in header files indicates the library version when the documentation was last reviewed, not when the file was last modified.

### Detailed Explanation

When you see this in `painlessMesh.h`:

```cpp
/**
 * @file painlessMesh.h
 * @version 1.8.7
 * @date 2025-11-12
 */
```

This means:
- ✅ The library version is 1.8.7
- ✅ The header documentation was reviewed as of version 1.8.7
- ❌ It does NOT mean the file hasn't changed since 1.8.7

### Why This Can Be Confusing

During active development, code files may be modified in multiple releases, but documentation comments might not be updated every time. This led to the situation where:

- `library.properties` showed version 1.8.6
- `library.json` showed version 1.8.6
- `package.json` showed version 1.8.6
- But `painlessMesh.h` header comment still showed 1.8.4

This created confusion about whether the file had been modified in versions 1.8.5 and 1.8.6.

### How to Find When a File Was Last Modified

Use Git to see the actual modification history:

```bash
# See all commits that modified a file
git log --oneline -- src/painlessMesh.h

# See when a file was last modified
git log -1 --format="%ai %an" -- src/painlessMesh.h

# See detailed changes to a file
git log -p -- src/painlessMesh.h
```

### Where to Find the Official Library Version

The official library version is always defined in these three files (and they must match):

1. **`library.properties`** - Arduino Library Manager
   ```properties
   version=1.8.7
   ```

2. **`library.json`** - PlatformIO Registry
   ```json
   "version": "1.8.7"
   ```

3. **`package.json`** - NPM Package
   ```json
   "version": "1.8.7"
   ```

### Best Practice Going Forward

As of version 1.8.7, we've established a best practice:

1. **Update header version comments during releases** to keep them synchronized with the library version
2. **Refer to VERSION_MANAGEMENT.md** for complete versioning guidelines
3. **Use Git for file history** to track actual modifications
4. **Check library.properties** for the official version number

### Complete Version Management Documentation

For comprehensive information about version management in this library, see:

**📖 [VERSION_MANAGEMENT.md](VERSION_MANAGEMENT.md)**

This document covers:
- Official version file locations
- Header comment meanings and purpose
- How to track file modifications
- Version update workflow
- Best practices for contributors

## Related Questions

### Q: Why were the header versions out of sync?

**A:** During rapid development cycles, header documentation comments were not updated every release. We've now established a process to keep them synchronized.

### Q: Will this happen again?

**A:** We've updated the release process (see RELEASE_GUIDE.md) to include header version updates as a standard step, and created comprehensive documentation to prevent future confusion.

### Q: What about other version numbers I see in the code?

**A:** If you see version numbers in implementation files (`.cpp`, `.hpp`), those should generally be removed. Version information should only be in:
- Official version files (library.properties, library.json, package.json)
- Header documentation comments (as library version references)
- CHANGELOG.md (for release history)

### Q: How do I know if my library is up to date?

**A:** Check your installed version against the latest release:

**Arduino IDE:**
```
Sketch → Include Library → Manage Libraries → Search "AlteriomPainlessMesh"
```

**PlatformIO:**
```bash
pio pkg show alteriom/AlteriomPainlessMesh
```

**NPM:**
```bash
npm view @alteriom/painlessmesh version
```

**GitHub:**
Visit https://github.com/Alteriom/painlessMesh/releases

## Summary

| File Location | Purpose | Meaning |
|---------------|---------|---------|
| `library.properties` | Official version | **Source of truth** for library version |
| `library.json` | Official version | **Source of truth** for library version |
| `package.json` | Official version | **Source of truth** for library version |
| Header comments (`@version`) | Documentation | Library version when docs were reviewed |
| Git history (`git log`) | File tracking | **Actual modification history** |
| `CHANGELOG.md` | Release history | Human-readable version changes |

**Remember:** Always check `library.properties` for the official library version, and use `git log` to see actual file modification history.

---

**See Also:**
- [VERSION_MANAGEMENT.md](VERSION_MANAGEMENT.md) - Complete version management guide
- [RELEASE_GUIDE.md](../RELEASE_GUIDE.md) - Release process documentation
- [CHANGELOG.md](../CHANGELOG.md) - Release history

**Last Updated:** November 12, 2025
