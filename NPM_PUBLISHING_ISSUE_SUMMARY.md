# NPM and GitHub Packages Publishing Issue - Summary

## Issue Identified

When PR#19 was merged to main with the commit message:
```
Release v1.7.7 - Complete mqtt-schema v0.7.2 implementation
```

The automated release workflow created the tag and GitHub release successfully, **but the NPM and GitHub Packages publishing jobs were skipped**.

## Root Cause

The automated workflow file `.github/workflows/release.yml` has a condition for NPM and GitHub Packages publishing:

```yaml
npm-publish:
  needs: [tag-and-release]
  runs-on: ubuntu-latest
  if: startsWith(github.event.head_commit.message, 'release:')
```

**Problem**: The condition checks if the commit message starts with `release:` (lowercase with colon), but the merge commit was:
-  Tag created: v1.7.7
-  GitHub Release created
-  NPM publish skipped (message started with "Release" not "release:")
-  GitHub Packages publish skipped (same reason)

## Solution Implemented

### 1. Manual Publishing Workflow Created

Created `.github/workflows/manual-publish.yml` with:
- Manual trigger via GitHub Actions UI
- Checkboxes to select which registries to publish to:
  -  Publish to NPM Registry
  -  Publish to GitHub Packages
- Reads version from `library.properties` automatically
- Validates authentication tokens
- Provides clear success/failure feedback

### 2. How to Use Manual Publishing

**Via GitHub UI:**
1. Go to https://github.com/Alteriom/painlessMesh/actions
2. Select "Manual Package Publishing" workflow
3. Click "Run workflow" button
4. Select desired options (both checked by default)
5. Click "Run workflow"

**Via GitHub CLI:**
```bash
gh workflow run manual-publish.yml
```

### 3. Documentation Updated

Updated `RELEASE_GUIDE.md` with:
- Explanation of the commit message requirement
- Troubleshooting section for this specific issue
- Instructions for using the manual publishing workflow
- Examples of correct vs incorrect commit messages

## Immediate Action Required

To publish v1.7.7 to NPM and GitHub Packages:

1. Navigate to: https://github.com/Alteriom/painlessMesh/actions/workflows/manual-publish.yml
2. Click "Run workflow"
3. Ensure both checkboxes are selected:
   -  Publish to NPM Registry
   -  Publish to GitHub Packages
4. Click "Run workflow"

The workflow will:
- Read version 1.7.7 from library.properties
- Publish @alteriom/painlessmesh@1.7.7 to npmjs.org
- Publish to GitHub Packages registry

## Prevention for Future Releases

To avoid this issue in future releases, ensure commit messages start with `release:` (lowercase with colon):

** Correct:**
```bash
git commit -m "release: v1.7.8 - Next version description"
```

** Wrong:**
```bash
git commit -m "Release v1.7.8 - Next version description"
```

## Additional Notes

- The tag v1.7.7 exists and is correct
- GitHub Release exists and is correct
- Only NPM/GitHub Packages publishing needs to be done manually this time
- All other release channels (PlatformIO, Arduino Library Manager) are unaffected
- This is a one-time manual fix; future releases will work automatically if commit message is correct

## Files Changed

1. `.github/workflows/manual-publish.yml` - New manual publishing workflow
2. `RELEASE_GUIDE.md` - Updated documentation with troubleshooting

---

**Status**: Ready to manually publish v1.7.7 packages
**Action**: Run manual-publish.yml workflow via GitHub Actions UI
