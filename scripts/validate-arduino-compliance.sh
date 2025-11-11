#!/bin/bash

# Arduino Library Manager Compliance Validation Script
# This script verifies that the library meets all Arduino Library Manager requirements

set -e

echo "=================================================="
echo "Arduino Library Manager Compliance Check"
echo "=================================================="
echo ""

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Track failures
FAILURES=0

# Function to check a requirement
check_requirement() {
    local name="$1"
    local check="$2"
    
    echo -n "Checking $name... "
    if eval "$check"; then
        echo -e "${GREEN}✓ PASS${NC}"
        return 0
    else
        echo -e "${RED}✗ FAIL${NC}"
        ((FAILURES++))
        return 1
    fi
}

# 1. Check library.properties exists
check_requirement "library.properties exists" "[ -f library.properties ]"

# 2. Check library.properties has required fields
if [ -f library.properties ]; then
    check_requirement "library.properties has name" "grep -q '^name=' library.properties"
    check_requirement "library.properties has version" "grep -q '^version=' library.properties"
    check_requirement "library.properties has author" "grep -q '^author=' library.properties"
    check_requirement "library.properties has maintainer" "grep -q '^maintainer=' library.properties"
    check_requirement "library.properties has sentence" "grep -q '^sentence=' library.properties"
    check_requirement "library.properties has paragraph" "grep -q '^paragraph=' library.properties"
    check_requirement "library.properties has category" "grep -q '^category=' library.properties"
    check_requirement "library.properties has url" "grep -q '^url=' library.properties"
    check_requirement "library.properties has architectures" "grep -q '^architectures=' library.properties"
fi

# 3. Check src/ directory exists
check_requirement "src/ directory exists" "[ -d src ]"

# 4. Check examples/ directory exists
check_requirement "examples/ directory exists" "[ -d examples ]"

# 5. Check for at least one example
check_requirement "at least one example exists" "[ $(find examples -name '*.ino' | wc -l) -gt 0 ]"

# 6. Check README.md exists
check_requirement "README.md exists" "[ -f README.md ]"

# 7. Check LICENSE file exists
check_requirement "LICENSE file exists" "[ -f LICENSE ]"

# 8. Check for .h files in src/
check_requirement "header files exist in src/" "[ $(find src -name '*.h' -o -name '*.hpp' | wc -l) -gt 0 ]"

# 9. Check version consistency
if [ -f library.properties ] && [ -f library.json ] && [ -f package.json ]; then
    VERSION_PROPS=$(grep '^version=' library.properties | cut -d'=' -f2)
    VERSION_JSON=$(grep '"version":' library.json | head -1 | sed 's/.*"version": "\(.*\)",/\1/')
    VERSION_PKG=$(grep '"version":' package.json | head -1 | sed 's/.*"version": "\(.*\)",/\1/')
    
    echo -n "Checking version consistency... "
    if [ "$VERSION_PROPS" = "$VERSION_JSON" ] && [ "$VERSION_PROPS" = "$VERSION_PKG" ]; then
        echo -e "${GREEN}✓ PASS${NC} (all version=$VERSION_PROPS)"
    else
        echo -e "${RED}✗ FAIL${NC}"
        echo "  library.properties: $VERSION_PROPS"
        echo "  library.json: $VERSION_JSON"
        echo "  package.json: $VERSION_PKG"
        ((FAILURES++))
    fi
fi

# 10. Check git tags
echo -n "Checking git tags... "
if git tag -l "v*" | grep -q "v"; then
    LATEST_TAG=$(git tag -l "v*" | sort -V | tail -1)
    echo -e "${GREEN}✓ PASS${NC} (latest: $LATEST_TAG)"
else
    echo -e "${RED}✗ FAIL${NC} (no version tags found)"
    ((FAILURES++))
fi

# 11. Check for keywords.txt (optional but recommended)
echo -n "Checking keywords.txt (optional)... "
if [ -f keywords.txt ]; then
    echo -e "${GREEN}✓ PRESENT${NC}"
else
    echo -e "${YELLOW}○ MISSING (optional)${NC}"
fi

# Summary
echo ""
echo "=================================================="
if [ $FAILURES -eq 0 ]; then
    echo -e "${GREEN}✓ ALL CHECKS PASSED${NC}"
    echo ""
    echo "Library is ready for Arduino Library Manager submission!"
    echo ""
    echo "Next steps:"
    echo "1. Review: docs/ARDUINO_LIBRARY_MANAGER_SUBMISSION.md"
    echo "2. Submit: https://github.com/arduino/library-registry"
    echo "3. Use template: .github/ARDUINO_LIBRARY_REGISTRY_SUBMISSION.md"
    exit 0
else
    echo -e "${RED}✗ $FAILURES CHECK(S) FAILED${NC}"
    echo ""
    echo "Please fix the failures above before submitting to Arduino Library Manager."
    exit 1
fi
