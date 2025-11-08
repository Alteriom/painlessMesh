#!/bin/bash

# Script to create missing GitHub releases for v1.7.8 and v1.7.9
# This script should be run with appropriate GitHub token permissions

set -e

REPO="Alteriom/painlessMesh"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}================================${NC}"
echo -e "${BLUE}Create Missing GitHub Releases${NC}"
echo -e "${BLUE}================================${NC}"
echo ""

# Check if gh CLI is available
if ! command -v gh &> /dev/null; then
    echo -e "${RED}❌ GitHub CLI (gh) is not installed${NC}"
    echo ""
    echo "Install it from: https://cli.github.com/"
    echo ""
    echo "Or use this script as a reference to create releases manually:"
    echo "  1. Go to: https://github.com/$REPO/releases/new"
    echo "  2. Create releases for v1.7.8 and v1.7.9"
    echo "  3. Use the generated release notes from /tmp/release_notes_*.txt"
    exit 1
fi

# Check if authenticated
if ! gh auth status &> /dev/null; then
    echo -e "${RED}❌ Not authenticated with GitHub${NC}"
    echo ""
    echo "Run: gh auth login"
    exit 1
fi

echo -e "${GREEN}✅ GitHub CLI is installed and authenticated${NC}"
echo ""

cd "$REPO_DIR"

# Extract release notes from CHANGELOG.md
echo -e "${BLUE}📝 Extracting release notes from CHANGELOG.md...${NC}"

# Extract v1.7.8 release notes
awk '/^## \[1\.7\.8\]/ {flag=1; next} /^## \[/ {flag=0} flag' CHANGELOG.md > /tmp/release_notes_1.7.8.txt
echo -e "${GREEN}✅ Extracted v1.7.8 release notes ($(wc -l < /tmp/release_notes_1.7.8.txt) lines)${NC}"

# Extract v1.7.9 release notes
awk '/^## \[1\.7\.9\]/ {flag=1; next} /^## \[/ {flag=0} flag' CHANGELOG.md > /tmp/release_notes_1.7.9.txt
echo -e "${GREEN}✅ Extracted v1.7.9 release notes ($(wc -l < /tmp/release_notes_1.7.9.txt) lines)${NC}"

echo ""

# Function to create release
create_release() {
    local version=$1
    local notes_file=$2
    local tag="v${version}"
    
    echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    echo -e "${BLUE}Creating Release: ${tag}${NC}"
    echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    
    # Check if tag already exists
    if git rev-parse "$tag" &> /dev/null; then
        echo -e "${YELLOW}⚠️  Tag $tag already exists${NC}"
    else
        echo -e "${BLUE}📌 Creating git tag: $tag${NC}"
        git tag -a "$tag" -m "painlessMesh $tag"
        echo -e "${GREEN}✅ Tag created${NC}"
        
        echo -e "${BLUE}📤 Pushing tag to remote...${NC}"
        git push origin "$tag"
        echo -e "${GREEN}✅ Tag pushed${NC}"
    fi
    
    # Check if release already exists
    if gh release view "$tag" --repo "$REPO" &> /dev/null; then
        echo -e "${YELLOW}⚠️  Release $tag already exists${NC}"
        echo ""
        read -p "Do you want to delete and recreate it? (y/N): " -n 1 -r
        echo
        if [[ $REPLY =~ ^[Yy]$ ]]; then
            echo -e "${BLUE}🗑️  Deleting existing release...${NC}"
            gh release delete "$tag" --repo "$REPO" --yes
            echo -e "${GREEN}✅ Existing release deleted${NC}"
        else
            echo -e "${YELLOW}⏭️  Skipping $tag${NC}"
            echo ""
            return
        fi
    fi
    
    echo -e "${BLUE}🎉 Creating GitHub release...${NC}"
    gh release create "$tag" \
        --repo "$REPO" \
        --title "painlessMesh $tag" \
        --notes-file "$notes_file"
    
    echo -e "${GREEN}✅ Release created successfully!${NC}"
    echo -e "${GREEN}🔗 View at: https://github.com/$REPO/releases/tag/$tag${NC}"
    echo ""
}

# Create releases
echo -e "${YELLOW}This script will create GitHub releases for v1.7.8 and v1.7.9${NC}"
echo ""
read -p "Continue? (y/N): " -n 1 -r
echo
if [[ ! $REPLY =~ ^[Yy]$ ]]; then
    echo -e "${YELLOW}Cancelled${NC}"
    exit 0
fi
echo ""

# Create v1.7.8 release
create_release "1.7.8" "/tmp/release_notes_1.7.8.txt"

# Create v1.7.9 release
create_release "1.7.9" "/tmp/release_notes_1.7.9.txt"

echo -e "${GREEN}================================${NC}"
echo -e "${GREEN}✅ All Releases Created!${NC}"
echo -e "${GREEN}================================${NC}"
echo ""
echo -e "${BLUE}📋 Next Steps:${NC}"
echo "  1. Verify releases: https://github.com/$REPO/releases"
echo "  2. Check that release notes are correct"
echo "  3. Consider triggering the manual-publish workflow for NPM/GitHub Packages"
echo ""
echo -e "${BLUE}💡 To publish packages:${NC}"
echo "  gh workflow run manual-publish.yml --repo $REPO"
echo ""
