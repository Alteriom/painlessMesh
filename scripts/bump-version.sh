#!/bin/bash

# Script to bump version in library.properties and library.json
# Usage: ./scripts/bump-version.sh [major|minor|patch] [new_version]

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(dirname "$SCRIPT_DIR")"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

print_usage() {
    echo "Usage: $0 [major|minor|patch] [specific_version]"
    echo ""
    echo "Examples:"
    echo "  $0 patch              # 1.5.6 -> 1.5.7"
    echo "  $0 minor              # 1.5.6 -> 1.6.0"
    echo "  $0 major              # 1.5.6 -> 2.0.0"
    echo "  $0 patch 1.5.7       # Set specific version"
    echo ""
    echo "This will update both library.properties and library.json"
}

get_current_version() {
    if [[ ! -f "$ROOT_DIR/library.properties" ]]; then
        echo -e "${RED}Error: library.properties not found${NC}" >&2
        exit 1
    fi
    
    grep '^version=' "$ROOT_DIR/library.properties" | cut -d'=' -f2
}

validate_version_format() {
    local version="$1"
    if [[ ! $version =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
        echo -e "${RED}Error: Invalid version format '$version'. Expected format: X.Y.Z${NC}" >&2
        exit 1
    fi
}

bump_version() {
    local current="$1"
    local bump_type="$2"
    
    IFS='.' read -r -a version_parts <<< "$current"
    local major="${version_parts[0]}"
    local minor="${version_parts[1]}"
    local patch="${version_parts[2]}"
    
    case "$bump_type" in
        "major")
            major=$((major + 1))
            minor=0
            patch=0
            ;;
        "minor")
            minor=$((minor + 1))
            patch=0
            ;;
        "patch")
            patch=$((patch + 1))
            ;;
        *)
            echo -e "${RED}Error: Invalid bump type '$bump_type'${NC}" >&2
            exit 1
            ;;
    esac
    
    echo "$major.$minor.$patch"
}

update_library_properties() {
    local new_version="$1"
    local file="$ROOT_DIR/library.properties"
    
    if [[ -f "$file" ]]; then
        sed -i "s/^version=.*/version=$new_version/" "$file"
        echo -e "${GREEN}✓ Updated library.properties to version $new_version${NC}"
    else
        echo -e "${RED}Error: library.properties not found${NC}" >&2
        exit 1
    fi
}

update_library_json() {
    local new_version="$1"
    local file="$ROOT_DIR/library.json"
    
    if [[ -f "$file" ]]; then
        if command -v jq >/dev/null 2>&1; then
            # Use jq if available for better JSON handling
            jq --arg version "$new_version" '.version = $version' "$file" > "${file}.tmp" && mv "${file}.tmp" "$file"
        else
            # Fallback to sed
            sed -i "s/\"version\": \"[^\"]*\"/\"version\": \"$new_version\"/" "$file"
        fi
        echo -e "${GREEN}✓ Updated library.json to version $new_version${NC}"
    else
        echo -e "${RED}Error: library.json not found${NC}" >&2
        exit 1
    fi
}

update_package_json() {
    local new_version="$1"
    local file="$ROOT_DIR/package.json"
    
    if [[ -f "$file" ]]; then
        if command -v jq >/dev/null 2>&1; then
            # Use jq if available for better JSON handling
            jq --arg version "$new_version" '.version = $version' "$file" > "${file}.tmp" && mv "${file}.tmp" "$file"
        else
            # Fallback to sed
            sed -i "s/\"version\": \"[^\"]*\"/\"version\": \"$new_version\"/" "$file"
        fi
        echo -e "${GREEN}✓ Updated package.json to version $new_version${NC}"
    else
        echo -e "${YELLOW}Warning: package.json not found, skipping NPM version update${NC}"
    fi
}

verify_consistency() {
    local expected_version="$1"
    
    local prop_version
    prop_version=$(grep '^version=' "$ROOT_DIR/library.properties" | cut -d'=' -f2)
    
    local json_version
    if command -v jq >/dev/null 2>&1; then
        json_version=$(jq -r '.version' "$ROOT_DIR/library.json")
    else
        json_version=$(grep '"version"' "$ROOT_DIR/library.json" | sed 's/.*"version": "\([^"]*\)".*/\1/')
    fi
    
    local pkg_version=""
    if [[ -f "$ROOT_DIR/package.json" ]]; then
        if command -v jq >/dev/null 2>&1; then
            pkg_version=$(jq -r '.version' "$ROOT_DIR/package.json")
        else
            pkg_version=$(grep '"version"' "$ROOT_DIR/package.json" | sed 's/.*"version": "\([^"]*\)".*/\1/')
        fi
    fi
    
    local has_error=false
    
    if [[ "$prop_version" != "$expected_version" ]]; then
        echo -e "${RED}Error: library.properties version mismatch: $prop_version != $expected_version${NC}" >&2
        has_error=true
    fi
    
    if [[ "$json_version" != "$expected_version" ]]; then
        echo -e "${RED}Error: library.json version mismatch: $json_version != $expected_version${NC}" >&2
        has_error=true
    fi
    
    if [[ -n "$pkg_version" && "$pkg_version" != "$expected_version" ]]; then
        echo -e "${RED}Error: package.json version mismatch: $pkg_version != $expected_version${NC}" >&2
        has_error=true
    fi
    
    if [[ "$has_error" == "true" ]]; then
        echo -e "${RED}Version inconsistency detected${NC}" >&2
        echo "  library.properties: $prop_version"
        echo "  library.json: $json_version"
        if [[ -n "$pkg_version" ]]; then
            echo "  package.json: $pkg_version"
        fi
        echo "  expected: $expected_version"
        exit 1
    fi
    
    echo -e "${GREEN}✓ Version consistency verified: $expected_version${NC}"
}

main() {
    if [[ $# -eq 0 ]] || [[ "$1" == "-h" ]] || [[ "$1" == "--help" ]]; then
        print_usage
        exit 0
    fi
    
    local current_version
    current_version=$(get_current_version)
    echo -e "${YELLOW}Current version: $current_version${NC}"
    
    local new_version
    
    if [[ $# -eq 2 ]]; then
        # Specific version provided
        new_version="$2"
        validate_version_format "$new_version"
    elif [[ $# -eq 1 ]]; then
        # Bump type provided
        new_version=$(bump_version "$current_version" "$1")
    else
        echo -e "${RED}Error: Invalid number of arguments${NC}" >&2
        print_usage
        exit 1
    fi
    
    echo -e "${YELLOW}New version: $new_version${NC}"
    
    # Confirm before proceeding
    read -p "Continue with version update? (y/N): " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        echo "Cancelled."
        exit 0
    fi
    
    # Update files
    update_library_properties "$new_version"
    update_library_json "$new_version"
    update_package_json "$new_version"
    
    # Verify consistency
    verify_consistency "$new_version"
    
    echo -e "${GREEN}✅ Version successfully updated to $new_version${NC}"
    echo ""
    echo -e "${YELLOW}Next steps:${NC}"
    echo "1. Update CHANGELOG.md with your changes"
    echo "2. Commit and push: git add library.properties library.json package.json CHANGELOG.md"
    echo "3. Commit message: git commit -m \"release: v$new_version\""
    echo "4. Push: git push origin main"
    echo ""
    echo "This will trigger automated release with NPM publishing, GitHub Packages, and wiki updates."
}

main "$@"