#!/bin/bash

# Publish the package to npmjs.org, translating npm's failure codes into the
# action the operator actually has to take.
#
# npm reports credential problems as short codes (E401, EOTP, E403) whose fix is
# a specific, non-obvious click on npmjs.com. Issue #381 cost two operator
# round-trips for exactly this reason: an expired token (E401), then a
# replacement token that authenticated but could not publish (EOTP).
#
# Used by .github/workflows/release.yml and .github/workflows/manual-publish.yml.
# Expects NODE_AUTH_TOKEN in the environment and an .npmrc that consumes it
# (actions/setup-node with registry-url writes one).

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(dirname "$SCRIPT_DIR")"
cd "$ROOT_DIR"

PACKAGE_NAME="$(jq -r '.name' package.json)"
PACKAGE_VERSION="$(jq -r '.version' package.json)"
TOKEN_URL="https://www.npmjs.com/settings/tokens"

echo "📦 Publishing to NPM..."
echo "Registry: $(npm config get registry)"
echo "Package name: $PACKAGE_NAME"
echo "Package version: $PACKAGE_VERSION"

# Verify we're publishing to the correct registry
if [ "$(npm config get registry)" != "https://registry.npmjs.org/" ]; then
    echo "⚠️  Warning: NPM registry is not set to public NPM!"
    npm config set registry https://registry.npmjs.org/
fi

set +e
PUBLISH_OUT=$(npm publish --access public 2>&1)
PUBLISH_RC=$?
set -e

echo "$PUBLISH_OUT"

if [ "$PUBLISH_RC" -eq 0 ]; then
    exit 0
fi

echo ""
echo "=================================================================="

case "$PUBLISH_OUT" in
*EOTP* | *"one-time password"*)
    cat <<EOF
❌ npm demanded a one-time password (EOTP).

The token is valid — it authenticated — but it is not permitted to publish
without a 2FA challenge, which CI cannot answer.

Fix: mint a replacement granular token at
  $TOKEN_URL
  • Packages and scopes: Read and write, limited to $PACKAGE_NAME
  • Enable "Bypass 2FA"   <-- the setting that is missing here

npm removed the legacy "Automation" token type in November 2025. A granular
token without "Bypass 2FA" behaves like the old "Publish" token and lands
exactly here, so a token minted by muscle memory hits this.

Then update the NPM_TOKEN repository secret and re-run.

Longer term: npm is removing direct publish from bypass-2FA tokens
(targeted January 2027). Migrating to trusted publishing (OIDC) retires
NPM_TOKEN entirely — see RELEASE_GUIDE.md, "Trusted publishing (OIDC)".
EOF
    ;;
*E401* | *"401 Unauthorized"*)
    cat <<EOF
❌ npm rejected the credentials (401).

The NPM_TOKEN secret has expired or been revoked. Mint a replacement at
  $TOKEN_URL
  • Packages and scopes: Read and write, limited to $PACKAGE_NAME
  • Enable "Bypass 2FA" (CI cannot answer a 2FA prompt)

Verify it before saving the secret:
  curl -sS -H "Authorization: Bearer <new-token>" https://registry.npmjs.org/-/whoami

Then update the NPM_TOKEN repository secret and re-run.
EOF
    ;;
*"cannot publish over"* | *"You cannot publish over the previously published versions"*)
    cat <<EOF
✅/❌ Version $PACKAGE_VERSION is already published to npm.

Nothing to do if this is a re-run of an already-successful release. If this
is a new release, the version was not bumped — npm versions are immutable, so
publish a new version rather than trying to overwrite this one.
EOF
    ;;
*E403* | *"403 Forbidden"*)
    cat <<EOF
❌ npm accepted the credentials but refused the write (403).

The token authenticates but lacks write access to $PACKAGE_NAME, or the
account is not a maintainer of it. Check the token's package scope at
  $TOKEN_URL
and the maintainer list at
  https://www.npmjs.com/package/$PACKAGE_NAME/access
EOF
    ;;
*)
    echo "❌ npm publish failed (exit $PUBLISH_RC). See the npm output above."
    ;;
esac

echo "=================================================================="
echo ""
echo "Note: the GitHub Release, GitHub Packages, and PlatformIO publication"
echo "are independent of this job and may well have succeeded. Confirm what"
echo "actually landed on npm with:"
echo "  npm view $PACKAGE_NAME version"

exit "$PUBLISH_RC"
