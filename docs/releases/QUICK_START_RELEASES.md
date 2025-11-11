# Quick Start: Create Missing Releases

## TL;DR - Create v1.7.8 and v1.7.9 GitHub Releases

### Prerequisites
```bash
# Install GitHub CLI if not already installed
# macOS: brew install gh
# Linux: https://github.com/cli/cli/blob/trunk/docs/install_linux.md
# Windows: https://github.com/cli/cli#windows

# Authenticate (if not already)
gh auth login
```

### Execute (One Command)
```bash
./scripts/create-missing-releases.sh
```

That's it! The script will:
- ✅ Extract release notes from CHANGELOG.md
- ✅ Create tags v1.7.8 and v1.7.9
- ✅ Push tags to GitHub
- ✅ Create GitHub releases with proper notes

### Verify
```bash
# Check releases were created
gh release list --repo Alteriom/painlessMesh

# Should show:
# v1.7.9  painlessMesh v1.7.9  Latest
# v1.7.8  painlessMesh v1.7.8
```

### Optional: Publish Packages
```bash
# If NPM/GitHub Packages weren't published automatically
gh workflow run manual-publish.yml --repo Alteriom/painlessMesh
```

---

## Alternative Methods

### Manual via GitHub CLI

```bash
# v1.7.8
awk '/^## \[1\.7\.8\]/ {flag=1; next} /^## \[/ {flag=0} flag' CHANGELOG.md > /tmp/notes_1.7.8.txt
git tag -a v1.7.8 -m "painlessMesh v1.7.8" && git push origin v1.7.8
gh release create v1.7.8 --title "painlessMesh v1.7.8" --notes-file /tmp/notes_1.7.8.txt

# v1.7.9
awk '/^## \[1\.7\.9\]/ {flag=1; next} /^## \[/ {flag=0} flag' CHANGELOG.md > /tmp/notes_1.7.9.txt
git tag -a v1.7.9 -m "painlessMesh v1.7.9" && git push origin v1.7.9
gh release create v1.7.9 --title "painlessMesh v1.7.9" --notes-file /tmp/notes_1.7.9.txt
```

### Manual via Web UI

1. Go to: https://github.com/Alteriom/painlessMesh/releases/new
2. **For v1.7.8:**
   - Tag: `v1.7.8`
   - Title: `painlessMesh v1.7.8`
   - Description: Copy from CHANGELOG.md section `[1.7.8]` (lines 58-135)
   - Click "Publish release"
3. **For v1.7.9:**
   - Tag: `v1.7.9`
   - Title: `painlessMesh v1.7.9`
   - Description: Copy from CHANGELOG.md section `[1.7.9]` (lines 22-56)
   - Click "Publish release"

---

## Future Releases (Prevention)

Always use this workflow to ensure releases are created properly:

```bash
# 1. Update version
./scripts/bump-version.sh patch  # or minor, major

# 2. Update CHANGELOG.md with your changes

# 3. Validate (optional but recommended)
./scripts/release-agent.sh

# 4. Commit and push (triggers automatic release)
git add library.properties library.json package.json CHANGELOG.md
git commit -m "release: vX.Y.Z - Brief description"
git push origin main

# 5. Watch GitHub Actions to confirm
# https://github.com/Alteriom/painlessMesh/actions/workflows/release.yml
```

The workflow automatically creates:
- ✅ Git tag
- ✅ GitHub release
- ✅ NPM package
- ✅ GitHub Packages
- ✅ PlatformIO Library
- ✅ Wiki updates

---

## Need More Details?

- **Complete Documentation**: `docs/CREATE_MISSING_RELEASES.md`
- **Release Guide**: `RELEASE_GUIDE.md`
- **Scripts Reference**: `scripts/README.md`
