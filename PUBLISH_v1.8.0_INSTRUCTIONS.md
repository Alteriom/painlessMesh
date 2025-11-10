# Publishing painlessMesh v1.8.0 to NPM and PlatformIO

## Background

The GitHub release for v1.8.0 was successfully created on November 9, 2025, but the automated publishing to NPM and PlatformIO did not complete due to a workflow error ("Cannot upload assets to an immutable release").

## Current Status

- ✅ **GitHub Release v1.8.0**: Published successfully
- ❌ **NPM Package**: Currently at version 1.7.9 (needs update to 1.8.0)
- ❌ **PlatformIO Library**: Currently at version 1.7.9 (needs update to 1.8.0)

## Manual Publishing Instructions

### Step 1: Publish to NPM

1. Navigate to the [Manual Package Publishing workflow](https://github.com/Alteriom/painlessMesh/actions/workflows/manual-publish.yml)
2. Click the **"Run workflow"** button (top right)
3. Configure the workflow:
   - **Branch**: Select `main`
   - **Publish to NPM Registry**: ✅ Check this box
   - **Publish to GitHub Packages**: (Optional - check if needed)
4. Click **"Run workflow"**
5. Monitor the workflow execution at https://github.com/Alteriom/painlessMesh/actions
6. Verify success (all jobs should show green checkmarks)

**Prerequisites:**
- The `NPM_TOKEN` secret must be configured in repository settings
- The token must have publishing permissions for `@alteriom/painlessmesh`

### Step 2: Publish to PlatformIO

1. Navigate to the [PlatformIO Library Publishing workflow](https://github.com/Alteriom/painlessMesh/actions/workflows/platformio-publish.yml)
2. Click the **"Run workflow"** button (top right)
3. Configure the workflow:
   - **Branch**: Select `main`
   - **Version to publish**: Enter `1.8.0`
   - **Force publish**: Leave unchecked (unless the version already exists)
4. Click **"Run workflow"**
5. Monitor the workflow execution at https://github.com/Alteriom/painlessMesh/actions
6. Verify success (all jobs should show green checkmarks)

**Prerequisites:**
- The `PLATFORMIO_AUTH_TOKEN` secret must be configured in repository settings
- The token must have publishing permissions for the library

## Verification

### Verify NPM Publication

```bash
# Check the latest version
npm view @alteriom/painlessmesh version
```

Expected output: `1.8.0`

Or visit: https://www.npmjs.com/package/@alteriom/painlessmesh

### Verify PlatformIO Publication

```bash
# Install PlatformIO if not already installed
pip install platformio

# Search for the library
pio pkg search AlteriomPainlessMesh
```

The output should show version `1.8.0` for `alteriom/AlteriomPainlessMesh`

Or visit: https://registry.platformio.org/libraries/alteriom/AlteriomPainlessMesh

## Alternative Method: Re-trigger Release

If manual workflows encounter issues:

1. **Delete the GitHub release** (but keep the tag):
   - Go to https://github.com/Alteriom/painlessMesh/releases/tag/v1.8.0
   - Click "Delete release" (this keeps the git tag)

2. **Recreate the release**:
   - Go to https://github.com/Alteriom/painlessMesh/releases/new
   - Select tag: `v1.8.0`
   - Fill in the release notes (copy from previous release)
   - Click "Publish release"

3. **Monitor the automated workflows**:
   - The release event should trigger both npm and PlatformIO publishing workflows
   - Check https://github.com/Alteriom/painlessMesh/actions

**Note:** This approach will send duplicate notifications to watchers and should only be used if manual triggering fails.

## Root Cause & Future Prevention

### What Went Wrong

The automated release workflow (`release.yml`) failed at the "Upload library package" step with this error:

```
HTTP 422: Cannot upload assets to an immutable release.
```

This occurs because:
1. The release was created and published
2. GitHub marked it as "immutable" 
3. The workflow tried to upload additional assets after publication
4. GitHub rejected the upload
5. The workflow failed, preventing npm and PlatformIO jobs from running

### Recommended Fix

Update `.github/workflows/release.yml` to:

1. **Check if release already has assets** before uploading:
   ```yaml
   - name: Check if assets already uploaded
     id: check_assets
     run: |
       ASSETS=$(gh release view "v${{ steps.version.outputs.version }}" --json assets --jq '.assets | length')
       echo "asset_count=$ASSETS" >> $GITHUB_OUTPUT
   
   - name: Upload library package
     if: steps.check_assets.outputs.asset_count == '0'
     run: |
       gh release upload "v${{ steps.version.outputs.version }}" \
         "./painlessMesh-v${{ steps.version.outputs.version }}.zip" \
         --repo ${{ github.repository }}
   ```

2. **Or make asset upload non-blocking**:
   ```yaml
   - name: Upload library package
     continue-on-error: true  # Don't fail workflow if upload fails
     run: |
       gh release upload "v${{ steps.version.outputs.version }}" \
         "./painlessMesh-v${{ steps.version.outputs.version }}.zip" \
         --repo ${{ github.repository }}
   ```

## Related Files

- `.github/workflows/manual-publish.yml` - Manual NPM/GitHub Packages publishing workflow
- `.github/workflows/platformio-publish.yml` - PlatformIO library publishing workflow  
- `.github/workflows/release.yml` - Automated release workflow (contains the bug)
- `library.json` - PlatformIO library metadata (version: 1.8.0)
- `package.json` - NPM package metadata (version: 1.8.0)
- `library.properties` - Arduino library metadata (version: 1.8.0)

## Completion Checklist

Once publishing is complete:

- [ ] NPM package at v1.8.0 verified
- [ ] PlatformIO library at v1.8.0 verified
- [ ] Update this document with completion timestamp
- [ ] (Optional) Fix release.yml workflow to prevent future occurrences
- [ ] (Optional) Delete this instruction file once no longer needed

---

**Last Updated**: 2025-11-10  
**Status**: In Progress - Awaiting manual workflow triggers
