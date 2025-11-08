#!/bin/bash

# Release Agent - Automated Release Management for AlteriomPainlessMesh
# This script implements the release agent specification from .github/agents/release-agent.md

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(dirname "$SCRIPT_DIR")"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
MAGENTA='\033[0;35m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Counters for validation
PASSED=0
FAILED=0
WARNINGS=0

print_header() {
    echo -e "${CYAN}╔════════════════════════════════════════════════════════════╗${NC}"
    echo -e "${CYAN}║         AlteriomPainlessMesh Release Agent v1.0            ║${NC}"
    echo -e "${CYAN}║           Automated Release Quality Assurance              ║${NC}"
    echo -e "${CYAN}╚════════════════════════════════════════════════════════════╝${NC}"
    echo
}

print_section() {
    echo
    echo -e "${BLUE}▶ $1${NC}"
    echo -e "${BLUE}$(printf '─%.0s' {1..60})${NC}"
}

check_pass() {
    ((PASSED++))
    echo -e "${GREEN}  ✓ $1${NC}"
}

check_fail() {
    ((FAILED++))
    echo -e "${RED}  ✗ $1${NC}"
}

check_warn() {
    ((WARNINGS++))
    echo -e "${YELLOW}  ⚠ $1${NC}"
}

check_info() {
    echo -e "${CYAN}  ℹ $1${NC}"
}

# Check 1: Version Consistency
check_version_consistency() {
    print_section "Version Consistency Check"
    
    local prop_version=$(grep '^version=' "$ROOT_DIR/library.properties" | cut -d'=' -f2)
    local json_version=$(jq -r '.version' "$ROOT_DIR/library.json" 2>/dev/null || echo "ERROR")
    local pkg_version=$(jq -r '.version' "$ROOT_DIR/package.json" 2>/dev/null || echo "ERROR")
    
    check_info "library.properties: $prop_version"
    check_info "library.json: $json_version"
    check_info "package.json: $pkg_version"
    
    if [[ "$prop_version" == "$json_version" ]] && [[ "$prop_version" == "$pkg_version" ]]; then
        check_pass "All version files are consistent: $prop_version"
        echo "$prop_version" > /tmp/release_version.txt
        return 0
    else
        check_fail "Version mismatch detected!"
        echo "  Solution: Run './scripts/bump-version.sh patch $prop_version'"
        return 1
    fi
}

# Check 2: Semantic Versioning Format
check_version_format() {
    print_section "Version Format Validation"
    
    local version=$(grep '^version=' "$ROOT_DIR/library.properties" | cut -d'=' -f2)
    
    if [[ $version =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
        check_pass "Version follows semantic versioning: $version"
        return 0
    else
        check_fail "Invalid version format: $version (expected X.Y.Z)"
        return 1
    fi
}

# Check 3: Git Tag Existence
check_tag_existence() {
    print_section "Git Tag Validation"
    
    local version=$(grep '^version=' "$ROOT_DIR/library.properties" | cut -d'=' -f2)
    
    if git rev-parse -q --verify "refs/tags/v${version}" >/dev/null 2>&1; then
        check_fail "Tag v${version} already exists - this version has been released"
        echo "  Solution: Bump version or delete tag"
        return 1
    else
        check_pass "Tag v${version} does not exist - ready for new release"
        return 0
    fi
}

# Check 4: CHANGELOG Validation
check_changelog() {
    print_section "CHANGELOG Validation"
    
    if [[ ! -f "$ROOT_DIR/CHANGELOG.md" ]]; then
        check_fail "CHANGELOG.md not found"
        return 1
    fi
    
    local version=$(grep '^version=' "$ROOT_DIR/library.properties" | cut -d'=' -f2)
    
    # Check for version entry
    if grep -qE "^## \[${version}\]|^## v${version}" "$ROOT_DIR/CHANGELOG.md"; then
        check_pass "CHANGELOG has entry for version $version"
        
        # Check for date
        if grep -E "^## \[${version}\]" "$ROOT_DIR/CHANGELOG.md" | grep -qE "[0-9]{4}-[0-9]{2}-[0-9]{2}"; then
            check_pass "CHANGELOG entry includes date"
        else
            check_warn "CHANGELOG entry missing date (format: YYYY-MM-DD)"
        fi
        
        # Check for content sections
        local entry=$(awk "/^## \[${version}\]|^## v${version}/ {flag=1; next} /^## / {flag=0} flag" "$ROOT_DIR/CHANGELOG.md")
        if echo "$entry" | grep -qE "^### (Added|Changed|Fixed|Deprecated)"; then
            check_pass "CHANGELOG entry has content sections"
        else
            check_warn "CHANGELOG entry may be missing standard sections (Added/Changed/Fixed)"
        fi
        
        return 0
    else
        check_fail "No CHANGELOG entry found for version $version"
        echo "  Solution: Add '## [$version] - $(date +%Y-%m-%d)' section"
        return 1
    fi
}

# Check 5: Build System
check_build_system() {
    print_section "Build System Validation"
    
    if [[ ! -f "$ROOT_DIR/CMakeLists.txt" ]]; then
        check_fail "CMakeLists.txt not found"
        return 1
    fi
    check_pass "CMakeLists.txt exists"
    
    # Check for essential files
    local essential_files=("library.properties" "library.json" "package.json" "src/painlessMesh.h")
    local all_present=true
    
    for file in "${essential_files[@]}"; do
        if [[ -f "$ROOT_DIR/$file" ]]; then
            check_pass "Found: $file"
        else
            check_fail "Missing: $file"
            all_present=false
        fi
    done
    
    [[ "$all_present" == "true" ]]
}

# Check 6: Dependencies
check_dependencies() {
    print_section "Dependency Validation"
    
    local prop_deps=$(grep '^depends=' "$ROOT_DIR/library.properties" | cut -d'=' -f2)
    
    # Check for required dependencies
    local required=("ArduinoJson" "TaskScheduler")
    local all_found=true
    
    for dep in "${required[@]}"; do
        if [[ $prop_deps =~ $dep ]]; then
            check_pass "Found dependency: $dep"
        else
            check_fail "Missing dependency in library.properties: $dep"
            all_found=false
        fi
    done
    
    # Check library.json dependencies
    if jq -e '.dependencies' "$ROOT_DIR/library.json" >/dev/null 2>&1; then
        local json_dep_count=$(jq '.dependencies | length' "$ROOT_DIR/library.json")
        check_pass "library.json has $json_dep_count dependencies configured"
    else
        check_warn "library.json missing dependencies array"
    fi
    
    [[ "$all_found" == "true" ]]
}

# Check 7: Git Status
check_git_status() {
    print_section "Git Working Tree Status"
    
    # Skip in CI
    if [[ -n "${CI}" || -n "${GITHUB_ACTIONS}" ]]; then
        check_info "Skipping git status check in CI environment"
        return 0
    fi
    
    if git diff-index --quiet HEAD -- 2>/dev/null; then
        check_pass "Working tree is clean"
        return 0
    else
        check_warn "Working tree has uncommitted changes"
        git status --porcelain | head -10 | while read line; do
            check_info "$line"
        done
        return 0  # Warning only, don't fail
    fi
}

# Check 8: Test Suite
check_tests() {
    print_section "Test Suite Validation"
    
    if [[ ! -d "$ROOT_DIR/test" ]]; then
        check_warn "Test directory not found"
        return 0
    fi
    
    # Check if built
    if [[ ! -d "$ROOT_DIR/bin" ]] || [[ $(ls -1 "$ROOT_DIR/bin/catch_"* 2>/dev/null | wc -l) -eq 0 ]]; then
        check_info "Tests not built - run 'cmake -G Ninja . && ninja' first"
        check_warn "Skipping test execution"
        return 0
    fi
    
    check_pass "Test binaries found"
    
    # Quick test count
    local test_count=$(ls -1 "$ROOT_DIR/bin/catch_"* 2>/dev/null | wc -l)
    check_info "Found $test_count test suites"
    
    return 0
}

# Check 9: Release Workflow Configuration
check_release_workflow() {
    print_section "Release Workflow Configuration"
    
    if [[ ! -f "$ROOT_DIR/.github/workflows/release.yml" ]]; then
        check_fail "Release workflow not found"
        return 1
    fi
    check_pass "Release workflow exists"
    
    # Check for required permissions
    if grep -q "contents: write" "$ROOT_DIR/.github/workflows/release.yml"; then
        check_pass "Release workflow has 'contents: write' permission"
    else
        check_warn "Release workflow may be missing 'contents: write' permission"
    fi
    
    if grep -q "packages: write" "$ROOT_DIR/.github/workflows/release.yml"; then
        check_pass "Release workflow has 'packages: write' permission"
    else
        check_warn "Release workflow may be missing 'packages: write' permission"
    fi
    
    # Check for NPM publishing step
    if grep -q "npm publish" "$ROOT_DIR/.github/workflows/release.yml"; then
        check_pass "Release workflow includes NPM publishing"
    else
        check_warn "Release workflow may be missing NPM publishing step"
    fi
    
    return 0
}

# Check 10: Documentation Links
check_documentation() {
    print_section "Documentation Validation"
    
    if [[ ! -f "$ROOT_DIR/README.md" ]]; then
        check_fail "README.md not found"
        return 1
    fi
    check_pass "README.md exists"
    
    if [[ ! -f "$ROOT_DIR/RELEASE_GUIDE.md" ]]; then
        check_warn "RELEASE_GUIDE.md not found"
    else
        check_pass "RELEASE_GUIDE.md exists"
    fi
    
    # Check for broken internal links (basic check)
    local broken_links=0
    while IFS= read -r link; do
        local file=$(echo "$link" | sed 's/.*(\([^)]*\)).*/\1/' | sed 's/#.*//')
        if [[ "$file" =~ ^http ]]; then
            continue  # Skip external links
        fi
        if [[ -n "$file" ]] && [[ ! -f "$ROOT_DIR/$file" ]]; then
            check_warn "Potentially broken link: $file"
            ((broken_links++))
        fi
    done < <(grep -oE '\[.*\]\([^)]+\)' "$ROOT_DIR/README.md" | head -20)
    
    if [[ $broken_links -eq 0 ]]; then
        check_pass "No broken internal links detected (basic check)"
    fi
    
    return 0
}

# Generate Release Summary
generate_summary() {
    print_section "Release Summary"
    
    local version=$(cat /tmp/release_version.txt 2>/dev/null || grep '^version=' "$ROOT_DIR/library.properties" | cut -d'=' -f2)
    
    echo
    echo -e "${MAGENTA}╔════════════════════════════════════════════════════════════╗${NC}"
    echo -e "${MAGENTA}║                    RELEASE READINESS                       ║${NC}"
    echo -e "${MAGENTA}╠════════════════════════════════════════════════════════════╣${NC}"
    echo -e "${MAGENTA}║${NC} Version:        ${CYAN}$version${NC}"
    echo -e "${MAGENTA}║${NC} Checks Passed:  ${GREEN}$PASSED${NC}"
    echo -e "${MAGENTA}║${NC} Checks Failed:  ${RED}$FAILED${NC}"
    echo -e "${MAGENTA}║${NC} Warnings:       ${YELLOW}$WARNINGS${NC}"
    echo -e "${MAGENTA}╠════════════════════════════════════════════════════════════╣${NC}"
    
    if [[ $FAILED -eq 0 ]]; then
        echo -e "${MAGENTA}║${NC} ${GREEN}✓ READY FOR RELEASE${NC}"
        echo -e "${MAGENTA}╚════════════════════════════════════════════════════════════╝${NC}"
        echo
        echo -e "${GREEN}Next steps:${NC}"
        echo "  1. Review CHANGELOG.md for completeness"
        echo "  2. Commit all changes: git add ."
        echo "  3. Create release commit: git commit -m 'release: v$version - <description>'"
        echo "  4. Push to main: git push origin main"
        echo "  5. Monitor: https://github.com/Alteriom/painlessMesh/actions"
        echo
        echo -e "${CYAN}Distribution channels:${NC}"
        echo "  • GitHub Releases: https://github.com/Alteriom/painlessMesh/releases"
        echo "  • NPM: https://www.npmjs.com/package/@alteriom/painlessmesh"
        echo "  • PlatformIO: https://registry.platformio.org/libraries/alteriom/painlessMesh"
        echo "  • Arduino Library Manager (after first submission)"
        return 0
    else
        echo -e "${MAGENTA}║${NC} ${RED}✗ NOT READY FOR RELEASE${NC}"
        echo -e "${MAGENTA}╚════════════════════════════════════════════════════════════╝${NC}"
        echo
        echo -e "${RED}Please fix the failed checks above before attempting release.${NC}"
        return 1
    fi
}

# Main execution
main() {
    print_header
    
    # Run all validation checks
    check_version_consistency || true
    check_version_format || true
    check_tag_existence || true
    check_changelog || true
    check_build_system || true
    check_dependencies || true
    check_git_status || true
    check_tests || true
    check_release_workflow || true
    check_documentation || true
    
    # Generate summary and determine exit code
    echo
    generate_summary
}

# Handle command line arguments
case "${1:-}" in
    --help|-h)
        echo "Release Agent - Automated Release Quality Assurance"
        echo
        echo "Usage: $0 [OPTIONS]"
        echo
        echo "Options:"
        echo "  --help, -h     Show this help message"
        echo "  --version, -v  Show version information"
        echo
        echo "This script validates release readiness by checking:"
        echo "  • Version consistency across all files"
        echo "  • CHANGELOG completeness"
        echo "  • Build system configuration"
        echo "  • Dependencies"
        echo "  • Git status"
        echo "  • Release workflow configuration"
        echo "  • Documentation"
        echo
        echo "See .github/agents/release-agent.md for complete documentation."
        exit 0
        ;;
    --version|-v)
        echo "Release Agent v1.0"
        echo "Part of AlteriomPainlessMesh project"
        exit 0
        ;;
    *)
        main "$@"
        ;;
esac
