#!/bin/bash

# Where every remote branch stands against main: how far ahead and behind it
# is, when it was last touched, and how much of it is not in main under any
# commit.
#
# That last number is the one that matters. Maintainers squash, so a merged
# branch's commit is in main under a different SHA and `ahead` still says 1;
# `git cherry` compares the changes themselves. A branch with unmerged=0 can
# be deleted without losing work. See CONTRIBUTING.md, "Branches".
#
# Usage:
#   ./scripts/branch-status.sh            every remote branch, newest first
#   ./scripts/branch-status.sh --stale    only branches with unmerged=0
#   ./scripts/branch-status.sh --remote upstream

set -euo pipefail

REMOTE="origin"
STALE_ONLY=0

while [ $# -gt 0 ]; do
  case "$1" in
    --stale) STALE_ONLY=1 ;;
    --remote) shift; REMOTE="${1:?--remote needs a name}" ;;
    -h|--help)
      sed -n '3,16p' "$0" | sed 's/^# \{0,1\}//'
      exit 0
      ;;
    *) echo "unknown argument: $1" >&2; exit 2 ;;
  esac
  shift
done

MAIN="$REMOTE/main"
git rev-parse --verify --quiet "$MAIN" >/dev/null || {
  echo "no $MAIN — fetch first, or pass --remote" >&2
  exit 1
}

printf '%-56s %7s %7s %9s  %s\n' BRANCH AHEAD BEHIND UNMERGED LAST
# Newest first: the list is read to decide what to delete, and the bottom of
# it is where the answer usually is.
git for-each-ref --sort=-committerdate \
    --format='%(refname:short) %(committerdate:short)' \
    "refs/remotes/$REMOTE" |
while read -r branch date; do
  case "$branch" in
    "$REMOTE"|"$REMOTE/HEAD"|"$MAIN") continue ;;
  esac
  ahead=$(git rev-list --count "$MAIN..$branch")
  behind=$(git rev-list --count "$branch..$MAIN")
  unmerged=$(git cherry "$MAIN" "$branch" 2>/dev/null | grep -c '^+' || true)
  if [ "$STALE_ONLY" = 1 ] && [ "$unmerged" != 0 ]; then
    continue
  fi
  printf '%-56s %7s %7s %9s  %s\n' "$branch" "$ahead" "$behind" "$unmerged" "$date"
done
