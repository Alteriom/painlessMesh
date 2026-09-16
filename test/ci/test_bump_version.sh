#!/bin/bash
#
# scripts/bump-version.sh, on a copy of the repository's version metadata.
#
# Two defects found while preparing 2.0.3 from a Windows Git Bash checkout,
# where jq is not installed:
#
#   * the jq-less fallback was `sed s/"version": "..."/.../`, which replaced
#     every such key. In library.json that overwrote the dependency ranges
#     (AsyncTCP ^3.4.7, TaskScheduler ^2.0.0) with the package version, and
#     verify_consistency then read one line per dependency and reported a
#     mismatch naming a version nobody had set;
#   * main() always stopped at a confirmation prompt, so it could not run in
#     CI or under an agent.
#
# The fallback is the path this test exercises, on every machine: jq is
# masked with BUMP_VERSION_NO_JQ. When jq is installed the run is repeated
# with it, and both paths must leave the same files.

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
VERSION="9.8.7"

pass=0
fail=0

check() {
    local label="$1" found="$2" want="$3"
    if [[ "$found" == "$want" ]]; then
        printf '  ok   %s\n' "$label"
        pass=$((pass + 1))
    else
        printf '  FAIL %s: %s != %s\n' "$label" "${found:-<empty>}" "$want"
        fail=$((fail + 1))
    fi
}

# A copy of the repository's version metadata, and nothing else: the script
# must not need a git checkout to run.
stage_copy() {
    local work="$1"
    mkdir -p "$work/scripts" "$work/src" "$work/doxygen"
    cp "$ROOT_DIR/scripts/bump-version.sh" "$work/scripts/"
    cp "$ROOT_DIR/library.properties" "$ROOT_DIR/library.json" "$work/"
    # Written as ifs, not `[[ -f x ]] && cp`: under `set -e` a false test is a
    # failing command and would end the run here.
    if [[ -f "$ROOT_DIR/package.json" ]]; then cp "$ROOT_DIR/package.json" "$work/"; fi
    if [[ -f "$ROOT_DIR/package-lock.json" ]]; then cp "$ROOT_DIR/package-lock.json" "$work/"; fi
    if [[ -f "$ROOT_DIR/src/AlteriomPainlessMesh.h" ]]; then cp "$ROOT_DIR/src/AlteriomPainlessMesh.h" "$work/src/"; fi
    if [[ -f "$ROOT_DIR/doxygen/Doxyfile" ]]; then cp "$ROOT_DIR/doxygen/Doxyfile" "$work/doxygen/"; fi
}

# Every dependency range in library.json, as `owner/name version` lines. The
# whole point: these must read the same before and after a version bump.
dependency_ranges() {
    awk '
        /"dependencies"/ { inside = 1 }
        inside && /"name"[[:space:]]*:/ { name = $0; sub(/.*"name"[[:space:]]*:[[:space:]]*"/, "", name); sub(/".*/, "", name) }
        inside && /^      "version"[[:space:]]*:/ {
            range = $0
            sub(/.*"version"[[:space:]]*:[[:space:]]*"/, "", range)
            sub(/".*/, "", range)
            print name " " range
        }
    ' "$1"
}

top_level_version() {
    grep -m1 '^  "version"[[:space:]]*:' "$1" | sed 's/.*"version"[[:space:]]*:[[:space:]]*"\([^"]*\)".*/\1/'
}

run_bump() {
    local work="$1"
    shift
    # </dev/null proves it does not wait for an answer: without --yes the read
    # would take the EOF as "no" and the assertions below would fail.
    (cd "$work" && "$@" ./scripts/bump-version.sh patch "$VERSION" --yes) < /dev/null
}

echo "=== bump-version.sh, jq masked (the fallback path)"
work="$(mktemp -d)"
trap 'rm -rf "$work" "${work_jq:-}"' EXIT
stage_copy "$work"
before="$(dependency_ranges "$work/library.json")"
run_bump "$work" env BUMP_VERSION_NO_JQ=1 > "$work/output.txt" 2>&1 || {
    echo "  FAIL the script exited non-zero"
    cat "$work/output.txt"
    exit 1
}

after="$(dependency_ranges "$work/library.json")"
if [[ "$before" == "$after" && -n "$before" ]]; then
    printf '  ok   library.json dependency ranges unchanged (%s)\n' "$(echo "$before" | tr '\n' ';')"
    pass=$((pass + 1))
else
    echo "  FAIL library.json dependency ranges were rewritten"
    diff <(echo "$before") <(echo "$after") || true
    fail=$((fail + 1))
fi

check "library.properties" "$(grep '^version=' "$work/library.properties" | cut -d= -f2)" "$VERSION"
check "library.json" "$(top_level_version "$work/library.json")" "$VERSION"
check "package.json" "$(top_level_version "$work/package.json")" "$VERSION"
check "package-lock.json root" "$(top_level_version "$work/package-lock.json")" "$VERSION"
check "package-lock.json packages[\"\"]" "$(awk '
    /^    ""[[:space:]]*:[[:space:]]*\{/ { self = 1; next }
    self && /^      "version"[[:space:]]*:/ {
        sub(/.*"version"[[:space:]]*:[[:space:]]*"/, ""); sub(/".*/, ""); print; exit
    }' "$work/package-lock.json")" "$VERSION"
check "header string" "$(grep -m1 '^#define ALTERIOM_PAINLESS_MESH_VERSION "' "$work/src/AlteriomPainlessMesh.h" | sed 's/.*"\([^"]*\)".*/\1/')" "$VERSION"
check "header major" "$(grep -m1 '_VERSION_MAJOR' "$work/src/AlteriomPainlessMesh.h" | awk '{print $3}')" "9"
check "header minor" "$(grep -m1 '_VERSION_MINOR' "$work/src/AlteriomPainlessMesh.h" | awk '{print $3}')" "8"
check "header patch" "$(grep -m1 '_VERSION_PATCH' "$work/src/AlteriomPainlessMesh.h" | awk '{print $3}')" "7"
check "Doxyfile" "$(grep -m1 '^PROJECT_NUMBER' "$work/doxygen/Doxyfile" | sed 's/.*"v\{0,1\}\([0-9][^"]*\)".*/\1/')" "$VERSION"

# A dependency range is not a JSON key the package owns: the lock file's own
# dependency versions must be untouched too.
check "package-lock dependency count unchanged" \
    "$(grep -c '"version"' "$work/package-lock.json")" \
    "$(grep -c '"version"' "$ROOT_DIR/package-lock.json")"

echo "=== the same run, with jq if it is installed"
if command -v jq >/dev/null 2>&1; then
    work_jq="$(mktemp -d)"
    stage_copy "$work_jq"
    run_bump "$work_jq" env > "$work_jq/output.txt" 2>&1 || {
        echo "  FAIL the script exited non-zero with jq"
        cat "$work_jq/output.txt"
        exit 1
    }
    for file in library.json package.json package-lock.json; do
        # jq reformats what it rewrites, so compare the values, not the bytes.
        check "jq and fallback agree on $file" \
            "$(top_level_version "$work_jq/$file")" "$(top_level_version "$work/$file")"
    done
    check "jq path keeps the dependency ranges" \
        "$(dependency_ranges "$work_jq/library.json")" "$before"
else
    echo "  skipped: jq is not installed here (the fallback above is the path that matters)"
fi

echo
if [[ "$fail" -gt 0 ]]; then
    echo "bump-version.sh: $fail check(s) failed, $pass passed"
    exit 1
fi
echo "bump-version.sh: all $pass checks passed"
