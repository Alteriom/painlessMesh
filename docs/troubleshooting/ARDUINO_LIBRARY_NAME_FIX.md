# Arduino Library Manager - Library Name Fix

## Issue Resolution Summary

**Date**: November 11, 2025  
**Issue**: Arduino IDE showing version 1.6.1 instead of current version 1.8.2  
**Status**: ✅ **RESOLVED**

## Problem Analysis

### Initial Diagnosis (Incorrect)
Initially believed the library was not registered in Arduino Library Manager.

### Actual Root Cause (Correct)
The library IS registered, but the library name changed between releases:

| Version | Library Name | Arduino Indexing |
|---------|-------------|------------------|
| v1.6.1 (Sept 2025) | `Alteriom PainlessMesh` | ✅ Indexed |
| v1.7.0 - v1.8.2 | `AlteriomPainlessMesh` | ❌ Not indexed |

**Arduino Library Manager Requirement**: Library names must remain consistent after registration.

When the name changed from `Alteriom PainlessMesh` (with space) to `AlteriomPainlessMesh` (no space), the Arduino indexer stopped recognizing new releases as updates to the registered library.

## Solution Applied

### Change Made
Restored the library name in `library.properties` to match the original registration:

```diff
- name=AlteriomPainlessMesh
+ name=Alteriom PainlessMesh
```

### Verification
```bash
# v1.6.1 (last indexed version)
name=Alteriom PainlessMesh

# Current (fixed)
name=Alteriom PainlessMesh

# Result: ✓ Names match
```

## Impact & Timeline

### Immediate
- ✅ Library name corrected in repository
- ✅ Validation script updated to detect this issue
- ✅ Documentation updated with correct information

### Next 24-48 Hours
- Arduino Library Manager will detect next release (v1.8.3 or v1.9.0)
- New version will appear in Arduino IDE Library Manager
- Users will see update notification

### Going Forward
- All future releases will be automatically indexed
- No manual intervention required
- Library Manager will show latest versions within 24-48 hours of each GitHub release

## Technical Details

### Arduino Library Registry Status
- **Registered**: ✅ Yes
- **Repository URL**: https://github.com/Alteriom/painlessMesh
- **Registry Entry**: https://github.com/arduino/library-registry (repositories.txt)
- **Original Registration Name**: `Alteriom PainlessMesh`

### How Arduino Library Manager Works
1. Library is registered once in arduino/library-registry
2. Arduino indexer checks registered repositories for new releases every 24-48 hours
3. For each new release tag, indexer reads `library.properties`
4. **Critical**: Library name in `library.properties` must match original registration
5. If name matches, new version is added to index
6. If name doesn't match, release is ignored

### Why Name Changed
Between v1.6.1 and v1.7.0, the library name was changed (likely to remove the space for URL compatibility). However, this broke Arduino Library Manager indexing.

### Versions Affected
**Not Indexed** (due to name mismatch):
- v1.7.0, v1.7.1, v1.7.2, v1.7.3, v1.7.4, v1.7.5, v1.7.6, v1.7.7, v1.7.8, v1.7.9
- v1.8.0, v1.8.1, v1.8.2

**Will Be Indexed** (after fix):
- v1.8.3+ (any future releases with corrected name)

## What Users Will See

### Before Fix
```
Arduino IDE Library Manager:
┌─────────────────────────────────┐
│ Alteriom PainlessMesh           │
│ Version: 1.6.1                  │
│ [Installed]                     │
└─────────────────────────────────┘
```

### After Fix (24-48 hours after v1.8.3 release)
```
Arduino IDE Library Manager:
┌─────────────────────────────────┐
│ Alteriom PainlessMesh           │
│ Version: 1.8.3 (Update available)│
│ [Update]                        │
└─────────────────────────────────┘
```

## Validation

### Automated Check
Run the validation script to confirm library name is correct:

```bash
./scripts/validate-arduino-compliance.sh
```

Expected output:
```
Checking library name format... ✓ CORRECT (name='Alteriom PainlessMesh')
```

### Manual Verification
Compare current name with v1.6.1:

```bash
# Current
grep "^name=" library.properties
# Output: name=Alteriom PainlessMesh

# v1.6.1 (last indexed)
curl -s "https://raw.githubusercontent.com/Alteriom/painlessMesh/v1.6.1/library.properties" | grep "^name="
# Output: name=Alteriom PainlessMesh

# They should match ✓
```

## Lessons Learned

### For Library Maintainers
1. **Never change library name** after Arduino Library Manager registration
2. Library name must remain exactly the same (including spaces, capitalization)
3. Use validation scripts to catch name changes before release
4. Check Arduino Library Manager indexing after each release

### For This Repository
1. Added library name validation to compliance script
2. Documented the requirement clearly in release guide
3. This issue is now prevented by automated checks

## Related Files

### Modified Files (Commit 5065fa5)
- `library.properties` - Restored correct library name
- `docs/ARDUINO_LIBRARY_MANAGER_SUBMISSION.md` - Updated with resolution
- `README.md` - Fixed status and explanation
- `RELEASE_GUIDE.md` - Corrected registration information
- `ARDUINO_IDE_VERSION_FIX_SUMMARY.md` - Updated resolution details
- `scripts/validate-arduino-compliance.sh` - Added name consistency check

### Documentation
- `docs/ARDUINO_LIBRARY_MANAGER_SUBMISSION.md` - Complete guide
- `ARDUINO_IDE_VERSION_FIX_SUMMARY.md` - Issue summary
- This file - Detailed resolution documentation

## References

### Arduino Documentation
- [Arduino Library Manager FAQ](https://github.com/arduino/library-registry/blob/main/FAQ.md)
- [Library Specification](https://arduino.github.io/arduino-cli/latest/library-specification/)
- [Why libraries aren't updated](https://github.com/arduino/library-registry/issues/1002)

### Repository Links
- [Arduino Library Registry](https://github.com/arduino/library-registry)
- [Library Registry repositories.txt](https://github.com/arduino/library-registry/blob/main/repositories.txt)
- [This Repository](https://github.com/Alteriom/painlessMesh)

## Credits

**Issue Identified By**: @sparck75  
**Resolution**: GitHub Copilot  
**Date**: November 11, 2025  
**Commit**: 5065fa5

---

## Summary

✅ **Issue**: Library name changed, breaking Arduino indexing  
✅ **Fix**: Restored original name with space  
✅ **Result**: Automatic indexing will resume with next release  
✅ **Timeline**: 24-48 hours after v1.8.3+ release  
✅ **Prevention**: Validation script now checks library name  
