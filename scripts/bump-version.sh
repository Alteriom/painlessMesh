#!/bin/bash

# Set the release version in every file that carries it.
# Usage: ./scripts/bump-version.sh [major|minor|patch] [new_version] [--yes]

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(dirname "$SCRIPT_DIR")"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

print_usage() {
    echo "Usage: $0 [major|minor|patch] [specific_version] [--yes]"
    echo ""
    echo "Examples:"
    echo "  $0 patch              # 1.5.6 -> 1.5.7"
    echo "  $0 minor              # 1.5.6 -> 1.6.0"
    echo "  $0 major              # 1.5.6 -> 2.0.0"
    echo "  $0 patch 1.5.7        # Set specific version"
    echo "  $0 patch 1.5.7 --yes  # No confirmation prompt (CI, agents)"
    echo ""
    echo "Updates every file that carries the release version:"
    echo "  library.properties, library.json, package.json, package-lock.json,"
    echo "  src/AlteriomPainlessMesh.h, doxygen/Doxyfile"
}

# Whether this run may use jq. Without it the fallbacks below edit the files
# with awk, which is the path CI exercises (test/ci/test_bump_version.sh) and
# the one a Windows Git Bash checkout usually takes, jq not being installed
# there. BUMP_VERSION_NO_JQ is how that test reaches the fallback on a machine
# that does have jq; nothing else sets it.
has_jq() {
    if [[ -n "${BUMP_VERSION_NO_JQ:-}" ]]; then
        return 1
    fi
    command -v jq >/dev/null 2>&1
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

# Replace the *package's own* "version" in a JSON file, and nothing else.
#
# The previous fallback was a plain sed for `"version": "..."`, which replaced
# every such key in the file. In library.json that rewrote the dependency
# ranges -- AsyncTCP's ^3.4.7, TaskScheduler's ^2.0.0 -- with the package
# version, so a release prepared without jq silently pinned every dependency
# to the library's own number. The package's key is the one at two-space
# indentation; a dependency's sits deeper.
set_top_level_version() {
    local file="$1"
    local new_version="$2"

    awk -v version="$new_version" '
        !replaced && /^  "version"[[:space:]]*:[[:space:]]*"[^"]*"/ {
            sub(/"version"[[:space:]]*:[[:space:]]*"[^"]*"/, "\"version\": \"" version "\"")
            replaced = 1
        }
        { print }
    ' "$file" > "${file}.tmp" && mv "${file}.tmp" "$file"
}

# The package's own version as written in a JSON file: the first key at
# two-space indentation. Reading every `"version"` line (what verify_consistency
# did) returns one line per dependency as well, and the comparison that follows
# then reports a mismatch naming a version nobody set.
read_top_level_version() {
    local file="$1"

    grep -m1 '^  "version"[[:space:]]*:' "$file" | sed 's/.*"version"[[:space:]]*:[[:space:]]*"\([^"]*\)".*/\1/'
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
        if has_jq; then
            jq --arg version "$new_version" '.version = $version' "$file" > "${file}.tmp" && mv "${file}.tmp" "$file"
        else
            set_top_level_version "$file" "$new_version"
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
        if has_jq; then
            jq --arg version "$new_version" '.version = $version' "$file" > "${file}.tmp" && mv "${file}.tmp" "$file"
        else
            set_top_level_version "$file" "$new_version"
        fi
        echo -e "${GREEN}✓ Updated package.json to version $new_version${NC}"
    else
        echo -e "${YELLOW}Warning: package.json not found, skipping NPM version update${NC}"
    fi
}

# npm keeps the package's version twice in the lock file: at the root and in
# the entry for the package itself, `packages[""]`. npm rewrites both when it
# publishes; leaving them behind makes `npm ci` disagree with package.json.
# Every other "version" in the file belongs to a dependency.
update_package_lock() {
    local new_version="$1"
    local file="$ROOT_DIR/package-lock.json"

    if [[ ! -f "$file" ]]; then
        echo -e "${YELLOW}Warning: package-lock.json not found, skipping${NC}"
        return
    fi

    if has_jq; then
        jq --arg version "$new_version" \
            '.version = $version | if (.packages? | has("")) then .packages[""].version = $version else . end' \
            "$file" > "${file}.tmp" && mv "${file}.tmp" "$file"
    else
        awk -v version="$new_version" '
            !root && /^  "version"[[:space:]]*:[[:space:]]*"[^"]*"/ {
                sub(/"version"[[:space:]]*:[[:space:]]*"[^"]*"/, "\"version\": \"" version "\"")
                root = 1
                print
                next
            }
            /^    ""[[:space:]]*:[[:space:]]*\{/ { self = 1; print; next }
            self && /^      "version"[[:space:]]*:[[:space:]]*"[^"]*"/ {
                sub(/"version"[[:space:]]*:[[:space:]]*"[^"]*"/, "\"version\": \"" version "\"")
                self = 0
                print
                next
            }
            { print }
        ' "$file" > "${file}.tmp" && mv "${file}.tmp" "$file"
    fi
    echo -e "${GREEN}✓ Updated package-lock.json to version $new_version${NC}"
}

# The version a sketch can read at compile time. The three numeric defines are
# what `#if ALTERIOM_PAINLESS_MESH_VERSION_MAJOR >= 2` tests, so they have to
# move with the string.
update_version_header() {
    local new_version="$1"
    local file="$ROOT_DIR/src/AlteriomPainlessMesh.h"

    if [[ ! -f "$file" ]]; then
        echo -e "${YELLOW}Warning: src/AlteriomPainlessMesh.h not found, skipping${NC}"
        return
    fi

    IFS='.' read -r -a parts <<< "$new_version"
    sed -i \
        -e "s/^#define ALTERIOM_PAINLESS_MESH_VERSION \".*\"/#define ALTERIOM_PAINLESS_MESH_VERSION \"$new_version\"/" \
        -e "s/^#define ALTERIOM_PAINLESS_MESH_VERSION_MAJOR .*/#define ALTERIOM_PAINLESS_MESH_VERSION_MAJOR ${parts[0]}/" \
        -e "s/^#define ALTERIOM_PAINLESS_MESH_VERSION_MINOR .*/#define ALTERIOM_PAINLESS_MESH_VERSION_MINOR ${parts[1]}/" \
        -e "s/^#define ALTERIOM_PAINLESS_MESH_VERSION_PATCH .*/#define ALTERIOM_PAINLESS_MESH_VERSION_PATCH ${parts[2]}/" \
        "$file"
    echo -e "${GREEN}✓ Updated src/AlteriomPainlessMesh.h to version $new_version${NC}"
}

# The number on every generated API page.
update_doxyfile() {
    local new_version="$1"
    local file="$ROOT_DIR/doxygen/Doxyfile"

    if [[ ! -f "$file" ]]; then
        echo -e "${YELLOW}Warning: doxygen/Doxyfile not found, skipping${NC}"
        return
    fi

    sed -i -E "s/^(PROJECT_NUMBER[[:space:]]*=[[:space:]]*).*/\1\"v$new_version\"/" "$file"
    echo -e "${GREEN}✓ Updated doxygen/Doxyfile to version $new_version${NC}"
}

verify_consistency() {
    local expected_version="$1"

    local prop_version
    prop_version=$(grep '^version=' "$ROOT_DIR/library.properties" | cut -d'=' -f2)

    local json_version
    if has_jq; then
        json_version=$(jq -r '.version' "$ROOT_DIR/library.json")
    else
        json_version=$(read_top_level_version "$ROOT_DIR/library.json")
    fi

    local pkg_version=""
    if [[ -f "$ROOT_DIR/package.json" ]]; then
        if has_jq; then
            pkg_version=$(jq -r '.version' "$ROOT_DIR/package.json")
        else
            pkg_version=$(read_top_level_version "$ROOT_DIR/package.json")
        fi
    fi

    local lock_version=""
    local lock_self_version=""
    if [[ -f "$ROOT_DIR/package-lock.json" ]]; then
        if has_jq; then
            lock_version=$(jq -r '.version' "$ROOT_DIR/package-lock.json")
            lock_self_version=$(jq -r '.packages[""].version // ""' "$ROOT_DIR/package-lock.json")
        else
            lock_version=$(read_top_level_version "$ROOT_DIR/package-lock.json")
            lock_self_version=$(awk '
                /^    ""[[:space:]]*:[[:space:]]*\{/ { self = 1; next }
                self && /^      "version"[[:space:]]*:/ {
                    sub(/.*"version"[[:space:]]*:[[:space:]]*"/, "")
                    sub(/".*/, "")
                    print
                    exit
                }
            ' "$ROOT_DIR/package-lock.json")
        fi
    fi

    local header_version=""
    if [[ -f "$ROOT_DIR/src/AlteriomPainlessMesh.h" ]]; then
        header_version=$(grep -m1 '^#define ALTERIOM_PAINLESS_MESH_VERSION "' "$ROOT_DIR/src/AlteriomPainlessMesh.h" \
            | sed 's/.*"\([^"]*\)".*/\1/')
    fi

    local doxygen_version=""
    if [[ -f "$ROOT_DIR/doxygen/Doxyfile" ]]; then
        doxygen_version=$(grep -m1 '^PROJECT_NUMBER' "$ROOT_DIR/doxygen/Doxyfile" \
            | sed 's/.*"v\{0,1\}\([0-9][^"]*\)".*/\1/')
    fi

    local has_error=false

    check_one() {
        local label="$1" found="$2" want="$3"
        if [[ -n "$found" && "$found" != "$want" ]]; then
            echo -e "${RED}Error: $label version mismatch: $found != $want${NC}" >&2
            has_error=true
        fi
    }

    if [[ "$prop_version" != "$expected_version" ]]; then
        echo -e "${RED}Error: library.properties version mismatch: $prop_version != $expected_version${NC}" >&2
        has_error=true
    fi
    if [[ "$json_version" != "$expected_version" ]]; then
        echo -e "${RED}Error: library.json version mismatch: $json_version != $expected_version${NC}" >&2
        has_error=true
    fi
    check_one "package.json" "$pkg_version" "$expected_version"
    check_one "package-lock.json" "$lock_version" "$expected_version"
    check_one "package-lock.json packages[\"\"]" "$lock_self_version" "$expected_version"
    check_one "src/AlteriomPainlessMesh.h" "$header_version" "$expected_version"
    check_one "doxygen/Doxyfile" "$doxygen_version" "$expected_version"

    if [[ "$has_error" == "true" ]]; then
        echo -e "${RED}Version inconsistency detected${NC}" >&2
        echo "  library.properties: $prop_version"
        echo "  library.json: $json_version"
        [[ -n "$pkg_version" ]] && echo "  package.json: $pkg_version"
        [[ -n "$lock_version" ]] && echo "  package-lock.json: $lock_version"
        [[ -n "$lock_self_version" ]] && echo "  package-lock.json packages[\"\"]: $lock_self_version"
        [[ -n "$header_version" ]] && echo "  src/AlteriomPainlessMesh.h: $header_version"
        [[ -n "$doxygen_version" ]] && echo "  doxygen/Doxyfile: $doxygen_version"
        echo "  expected: $expected_version"
        exit 1
    fi

    echo -e "${GREEN}✓ Version consistency verified: $expected_version${NC}"
}

main() {
    local assume_yes=false
    local positional=()

    for argument in "$@"; do
        case "$argument" in
            -y|--yes)
                assume_yes=true
                ;;
            -h|--help)
                print_usage
                exit 0
                ;;
            -*)
                echo -e "${RED}Error: Unknown option '$argument'${NC}" >&2
                print_usage
                exit 1
                ;;
            *)
                positional+=("$argument")
                ;;
        esac
    done

    if [[ ${#positional[@]} -eq 0 ]]; then
        print_usage
        exit 0
    fi

    local current_version
    current_version=$(get_current_version)
    echo -e "${YELLOW}Current version: $current_version${NC}"

    local new_version

    if [[ ${#positional[@]} -eq 2 ]]; then
        # Specific version provided
        new_version="${positional[1]}"
        validate_version_format "$new_version"
    elif [[ ${#positional[@]} -eq 1 ]]; then
        # Bump type provided
        new_version=$(bump_version "$current_version" "${positional[0]}")
    else
        echo -e "${RED}Error: Invalid number of arguments${NC}" >&2
        print_usage
        exit 1
    fi

    echo -e "${YELLOW}New version: $new_version${NC}"

    # Confirm before proceeding, unless the caller already has. Without --yes
    # this read blocks forever in CI or under an agent, where there is no
    # terminal to answer it.
    if [[ "$assume_yes" != "true" ]]; then
        read -p "Continue with version update? (y/N): " -n 1 -r
        echo
        if [[ ! $REPLY =~ ^[Yy]$ ]]; then
            echo "Cancelled."
            exit 0
        fi
    fi

    # Update files
    update_library_properties "$new_version"
    update_library_json "$new_version"
    update_package_json "$new_version"
    update_package_lock "$new_version"
    update_version_header "$new_version"
    update_doxyfile "$new_version"

    # Verify consistency
    verify_consistency "$new_version"

    echo -e "${GREEN}✅ Version successfully updated to $new_version${NC}"
    echo ""
    echo -e "${YELLOW}Next steps:${NC}"
    echo "1. Update CHANGELOG.md with your changes"
    echo "2. Stage the version metadata:"
    echo "   git add library.properties library.json package.json package-lock.json \\"
    echo "       src/AlteriomPainlessMesh.h doxygen/Doxyfile CHANGELOG.md"
    echo "3. Commit message: git commit -m \"release: v$new_version\""
    echo "4. Push: git push origin main"
    echo ""
    echo "This will trigger automated release with NPM publishing, GitHub Packages, and wiki updates."
}

main "$@"
