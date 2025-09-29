#!/bin/bash

# Script to validate release readiness
# Checks version consistency, changelog, and build status

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(dirname "$SCRIPT_DIR")"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

print_header() {
    echo -e "${BLUE}=== painlessMesh Release Validation ===${NC}"
    echo
}

check_version_consistency() {
    echo -e "${YELLOW}Checking version consistency...${NC}"
    
    local prop_version
    prop_version=$(grep '^version=' "$ROOT_DIR/library.properties" | cut -d'=' -f2)
    
    local json_version
    if command -v jq >/dev/null 2>&1; then
        json_version=$(jq -r '.version' "$ROOT_DIR/library.json")
    else
        json_version=$(grep '"version"' "$ROOT_DIR/library.json" | sed 's/.*"version": "\([^"]*\)".*/\1/')
    fi
    
    if [[ "$prop_version" != "$json_version" ]]; then
        echo -e "${RED}❌ Version mismatch:${NC}"
        echo "  library.properties: $prop_version"
        echo "  library.json: $json_version"
        return 1
    fi
    
    echo -e "${GREEN}✓ Version consistency: $prop_version${NC}"
    echo "current_version=$prop_version" >> "$GITHUB_OUTPUT" 2>/dev/null || true
    return 0
}

check_changelog() {
    echo -e "${YELLOW}Checking changelog...${NC}"
    
    if [[ ! -f "$ROOT_DIR/CHANGELOG.md" ]]; then
        echo -e "${RED}❌ CHANGELOG.md not found${NC}"
        return 1
    fi
    
    local version
    version=$(grep '^version=' "$ROOT_DIR/library.properties" | cut -d'=' -f2)
    
    # Check if version exists in changelog
    if grep -q "^\[?${version}\]?\\|^## v${version}" "$ROOT_DIR/CHANGELOG.md"; then
        echo -e "${GREEN}✓ Changelog entry found for version $version${NC}"
        return 0
    else
        echo -e "${YELLOW}⚠ No changelog entry found for version $version${NC}"
        echo "  Consider adding changes to CHANGELOG.md"
        return 0  # Don't fail, just warn
    fi
}

check_git_status() {
    echo -e "${YELLOW}Checking git status...${NC}"
    
    if ! git diff-index --quiet HEAD --; then
        echo -e "${YELLOW}⚠ Working tree has uncommitted changes${NC}"
        git status --porcelain
        echo "  Consider committing changes before release"
        return 0  # Don't fail, just warn
    fi
    
    echo -e "${GREEN}✓ Working tree is clean${NC}"
    return 0
}

check_tag_exists() {
    echo -e "${YELLOW}Checking if release tag exists...${NC}"
    
    local version
    version=$(grep '^version=' "$ROOT_DIR/library.properties" | cut -d'=' -f2)
    
    if git rev-parse -q --verify "refs/tags/v${version}" >/dev/null 2>&1; then
        echo -e "${YELLOW}⚠ Tag v${version} already exists${NC}"
        echo "  This version has already been released"
        return 1
    fi
    
    echo -e "${GREEN}✓ Tag v${version} does not exist - ready for release${NC}"
    return 0
}

check_dependencies() {
    echo -e "${YELLOW}Checking library dependencies...${NC}"
    
    local required_deps=("ArduinoJson" "TaskScheduler" "AsyncTCP" "ESPAsyncTCP")
    local missing_deps=()
    
    # Check library.properties
    local depends_line
    depends_line=$(grep '^depends=' "$ROOT_DIR/library.properties" 2>/dev/null || echo "")
    
    for dep in "${required_deps[@]}"; do
        if [[ ! $depends_line =~ $dep ]]; then
            case $dep in
                "AsyncTCP"|"ESPAsyncTCP")
                    # These are platform-specific, check library.json
                    if ! grep -q "\"name\": \"$dep\"" "$ROOT_DIR/library.json"; then
                        missing_deps+=("$dep")
                    fi
                    ;;
                *)
                    missing_deps+=("$dep")
                    ;;
            esac
        fi
    done
    
    if [[ ${#missing_deps[@]} -gt 0 ]]; then
        echo -e "${YELLOW}⚠ Some dependencies may be missing or incorrectly specified:${NC}"
        printf '  %s\n' "${missing_deps[@]}"
        return 0  # Don't fail, just warn
    fi
    
    echo -e "${GREEN}✓ Dependencies look good${NC}"
    return 0
}

check_build_files() {
    echo -e "${YELLOW}Checking essential build files...${NC}"
    
    local essential_files=("library.properties" "library.json" "src/painlessMesh.h" "CMakeLists.txt")
    local missing_files=()
    
    for file in "${essential_files[@]}"; do
        if [[ ! -f "$ROOT_DIR/$file" ]]; then
            missing_files+=("$file")
        fi
    done
    
    if [[ ${#missing_files[@]} -gt 0 ]]; then
        echo -e "${RED}❌ Missing essential files:${NC}"
        printf '  %s\n' "${missing_files[@]}"
        return 1
    fi
    
    echo -e "${GREEN}✓ All essential files present${NC}"
    return 0
}

run_quick_build_test() {
    echo -e "${YELLOW}Running quick build test...${NC}"
    
    # Initialize submodules if needed for the test
    if [[ -f "$ROOT_DIR/.gitmodules" ]]; then
        echo "  Initializing submodules for build test..."
        if ! git submodule update --init --quiet 2>/dev/null; then
            echo -e "${YELLOW}⚠ Could not initialize submodules - skipping full build test${NC}"
            echo "  Note: This is normal in CI environments without git history"
            return 0
        fi
    fi
    
    # Check if we can configure CMake
    if ! cmake -G Ninja "$ROOT_DIR" -B "$ROOT_DIR/build-test" >/dev/null 2>&1; then
        echo -e "${RED}❌ CMake configuration failed${NC}"
        # Show the actual error for debugging
        echo "  CMake error details:"
        cmake -G Ninja "$ROOT_DIR" -B "$ROOT_DIR/build-test" 2>&1 | head -5 | sed 's/^/    /'
        return 1
    fi
    
    echo -e "${GREEN}✓ Build configuration successful${NC}"
    
    # Clean up
    rm -rf "$ROOT_DIR/build-test"
    return 0
}

generate_release_summary() {
    echo
    echo -e "${BLUE}=== Release Summary ===${NC}"
    
    local version
    version=$(grep '^version=' "$ROOT_DIR/library.properties" | cut -d'=' -f2)
    
    echo "Version: $version"
    echo "Date: $(date -u +"%Y-%m-%d %H:%M:%S UTC")"
    
    if [[ -f "$ROOT_DIR/CHANGELOG.md" ]]; then
        echo
        echo "Recent changes:"
        # Extract the most recent changelog entry
        awk '/^## \[/{if(++n==1) flag=1; if(n==2) flag=0} flag && !/^## \[/{print}' "$ROOT_DIR/CHANGELOG.md" | head -10
    fi
    
    echo
    echo "To release:"
    echo "  git commit -am 'release: v$version'"
    echo "  git push origin main"
}

main() {
    print_header
    
    local exit_code=0
    
    # Run all checks
    check_version_consistency || exit_code=1
    echo
    
    check_changelog || exit_code=1
    echo
    
    check_git_status || exit_code=1
    echo
    
    check_tag_exists || exit_code=1
    echo
    
    check_dependencies || exit_code=1
    echo
    
    check_build_files || exit_code=1
    echo
    
    run_quick_build_test || exit_code=1
    echo
    
    if [[ $exit_code -eq 0 ]]; then
        echo -e "${GREEN}✅ All validation checks passed!${NC}"
        generate_release_summary
    else
        echo -e "${RED}❌ Some validation checks failed!${NC}"
        echo "Please address the issues above before releasing."
    fi
    
    exit $exit_code
}

main "$@"