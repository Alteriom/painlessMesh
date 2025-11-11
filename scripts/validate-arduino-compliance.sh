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

# 12. Check library name consistency (critical for Arduino Library Manager)
if [ -f library.properties ]; then
    LIBRARY_NAME=$(grep '^name=' library.properties | cut -d'=' -f2)
    echo -n "Checking library name format... "
    if [ "$LIBRARY_NAME" = "Alteriom PainlessMesh" ]; then
        echo -e "${GREEN}✓ CORRECT${NC} (name='$LIBRARY_NAME')"
    elif [ "$LIBRARY_NAME" = "AlteriomPainlessMesh" ]; then
        echo -e "${RED}✗ INCORRECT${NC}"
        echo "  Current: '$LIBRARY_NAME'"
        echo "  Expected: 'Alteriom PainlessMesh' (with space)"
        echo "  Issue: Library was registered with space in name."
        echo "  Arduino Library Manager requires consistent naming."
        ((FAILURES++))
    else
        echo -e "${YELLOW}○ WARNING${NC} (name='$LIBRARY_NAME')"
        echo "  Note: Library registered as 'Alteriom PainlessMesh'"
    fi
fi

# 13. Check includes field lists all main headers
if [ -f library.properties ]; then
    echo -n "Checking includes field... "
    INCLUDES=$(grep '^includes=' library.properties | cut -d'=' -f2)
    
    if [[ "$INCLUDES" == *"painlessMesh.h"* ]] && [[ "$INCLUDES" == *"AlteriomPainlessMesh.h"* ]]; then
        echo -e "${GREEN}✓ CORRECT${NC} (includes both main headers)"
    elif [[ "$INCLUDES" == "painlessMesh.h" ]]; then
        echo -e "${RED}✗ INCOMPLETE${NC}"
        echo "  Current: includes=$INCLUDES"
        echo "  Expected: includes=painlessMesh.h,AlteriomPainlessMesh.h"
        echo "  Issue: Missing AlteriomPainlessMesh.h in includes field"
        echo "  This prevents users from discovering the Alteriom header"
        ((FAILURES++))
    else
        echo -e "${YELLOW}○ WARNING${NC} (includes=$INCLUDES)"
        echo "  Note: Should include both painlessMesh.h and AlteriomPainlessMesh.h"
    fi
fi

# Summary
echo ""
echo "=================================================="
if [ $FAILURES -eq 0 ]; then
    echo -e "${GREEN}✓ ALL CHECKS PASSED${NC}"
    echo ""
    echo "Library is ready for Arduino Library Manager indexing!"
    echo ""
    echo "Note: Library is already registered in Arduino Library Manager."
    echo "New releases with correct library name will be automatically indexed"
    echo "within 24-48 hours after creating a GitHub release."
    exit 0
else
    echo -e "${RED}✗ $FAILURES CHECK(S) FAILED${NC}"
    echo ""
    echo "Please fix the failures above before creating a new release."
    exit 1
fi
