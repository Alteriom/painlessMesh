#!/bin/bash
# Create Arduino IDE compatible ZIP file for manual installation
# This script creates a properly formatted ZIP file that can be imported
# directly into Arduino IDE via "Sketch -> Include Library -> Add .ZIP Library"

set -e

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Get version from library.properties
VERSION=$(grep '^version=' library.properties | cut -d'=' -f2)
LIBRARY_NAME="painlessMesh"
OUTPUT_DIR="dist"
PACKAGE_NAME="${LIBRARY_NAME}-v${VERSION}"

echo -e "${BLUE}================================${NC}"
echo -e "${BLUE}Arduino IDE ZIP Package Creator${NC}"
echo -e "${BLUE}================================${NC}"
echo ""
echo -e "Library: ${GREEN}${LIBRARY_NAME}${NC}"
echo -e "Version: ${GREEN}${VERSION}${NC}"
echo ""

# Check if we're in the repository root
if [ ! -f "library.properties" ]; then
    echo -e "${RED}❌ Error: library.properties not found${NC}"
    echo "Please run this script from the repository root directory"
    exit 1
fi

# Create output directory
mkdir -p "${OUTPUT_DIR}"

# Create temporary directory for package assembly
TEMP_DIR=$(mktemp -d)
PACKAGE_DIR="${TEMP_DIR}/${LIBRARY_NAME}"

echo -e "${YELLOW}📦 Preparing package contents...${NC}"

# Create package directory structure
mkdir -p "${PACKAGE_DIR}"

# Copy essential files and directories
echo "  ✓ Copying source files..."
cp -r src "${PACKAGE_DIR}/"

echo "  ✓ Copying examples..."
cp -r examples "${PACKAGE_DIR}/"

echo "  ✓ Copying library metadata..."
cp library.properties "${PACKAGE_DIR}/"

echo "  ✓ Copying documentation..."
cp README.md "${PACKAGE_DIR}/"
cp LICENSE "${PACKAGE_DIR}/"

# Optional: Include CHANGELOG if it exists
if [ -f "CHANGELOG.md" ]; then
    echo "  ✓ Including CHANGELOG..."
    cp CHANGELOG.md "${PACKAGE_DIR}/"
fi

# Optional: Include keywords.txt if it exists
if [ -f "keywords.txt" ]; then
    echo "  ✓ Including keywords.txt..."
    cp keywords.txt "${PACKAGE_DIR}/"
fi

# Create the ZIP file
echo ""
echo -e "${YELLOW}📦 Creating ZIP archive...${NC}"
cd "${TEMP_DIR}"
ZIP_FILE="${PACKAGE_NAME}.zip"
zip -r "${ZIP_FILE}" "${LIBRARY_NAME}/" > /dev/null 2>&1

# Move ZIP to output directory
mv "${ZIP_FILE}" "${OLDPWD}/${OUTPUT_DIR}/"
cd "${OLDPWD}"

# Cleanup
rm -rf "${TEMP_DIR}"

# Get file size
FILE_SIZE=$(du -h "${OUTPUT_DIR}/${ZIP_FILE}" | cut -f1)

echo -e "${GREEN}✅ Package created successfully!${NC}"
echo ""
echo -e "${BLUE}📁 Output:${NC} ${OUTPUT_DIR}/${ZIP_FILE}"
echo -e "${BLUE}📊 Size:${NC} ${FILE_SIZE}"
echo ""
echo -e "${BLUE}================================${NC}"
echo -e "${GREEN}Installation Instructions:${NC}"
echo -e "${BLUE}================================${NC}"
echo ""
echo "1. Open Arduino IDE"
echo "2. Go to: Sketch → Include Library → Add .ZIP Library..."
echo "3. Select: ${OUTPUT_DIR}/${ZIP_FILE}"
echo "4. Wait for installation to complete"
echo "5. Verify: Sketch → Include Library → painlessMesh"
echo ""
echo -e "${YELLOW}Note:${NC} You'll also need to install dependencies:"
echo "  - ArduinoJson (v6.21.x or v7.x)"
echo "  - TaskScheduler (v3.7.0+)"
echo ""
echo "Install dependencies via: Sketch → Include Library → Manage Libraries"
echo ""
echo -e "${GREEN}✅ Done!${NC}"
echo ""
