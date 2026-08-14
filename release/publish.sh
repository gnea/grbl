#!/usr/bin/env bash
# release/publish.sh — tag, build, sign, and publish a GitHub Release for
# this repo.
#
# Usage:
#   release/publish.sh [major|minor|patch]
#
# Bump defaults to "patch" (0.1.2 -> 0.1.3); pass "minor"/"major" to bump
# those instead (resets lower components to 0, SemVer, D-034). Tags this
# repo's default branch tip (auto-detected via gh, signed with the pinned
# git-signing key, KEY-POLICY.md), builds grbl-k-firmware.deb
# (release/build.sh -- AVR machine code, no arch selection needed), signs
# it with the release minisign key, and creates a GitHub Release with the
# tag, auto-generated notes, and the signed .deb + .hex assets attached.
# This is GPLv3, already-public source (github.com/pantadeusz/grbl-k) --
# the release itself is a public GitHub Release, same as any other tag on
# this repo.
#
# Requires:
#   - gh (GitHub CLI), authenticated
#   - minisign
#   - KONIN_DEPLOYMENT_DIR (see release/build.sh's own requirement)
#   - the release-signing minisign secret key -- KONIN_RELEASE_SECKEY env
#     var, or the mounted release-key drive (security/KEY-POLICY.md in
#     konin-deployment). Fails loudly, before doing anything else, if
#     neither is available -- never silently skips signing.
#
# A mistyped signing passphrase gets up to 3 attempts before giving up
# (sign_with_retry). Any failure after the release tag is pushed --
# exhausted signing attempts, a build failure, gh release create failing --
# rolls the tag back (local + remote) rather than leaving a stray pushed
# tag with no matching release behind.
set -euo pipefail
die() { echo "release/publish.sh: error: $*" >&2; exit 1; }

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO="$(cd "$HERE/.." && pwd)"
COMPONENT="$(basename "$REPO")"

BUMP="${1:-patch}"
case "$BUMP" in
  major|minor|patch) : ;;
  *) die "usage: release/publish.sh [major|minor|patch]" ;;
esac

command -v gh >/dev/null || die "gh (GitHub CLI) not installed"
gh auth status >/dev/null 2>&1 || die "gh not authenticated — run: gh auth login"
command -v minisign >/dev/null || die "minisign not installed (sudo apt install minisign)"

[ -n "${KONIN_DEPLOYMENT_DIR:-}" ] \
  || die "KONIN_DEPLOYMENT_DIR is not set -- point it at a konin-deployment checkout"
[ -d "$KONIN_DEPLOYMENT_DIR" ] \
  || die "KONIN_DEPLOYMENT_DIR=$KONIN_DEPLOYMENT_DIR does not exist"
SECKEY_LIB="$KONIN_DEPLOYMENT_DIR/releases/lib/find-release-seckey.sh"
[ -f "$SECKEY_LIB" ] \
  || die "$SECKEY_LIB not found -- is KONIN_DEPLOYMENT_DIR pointing at a real konin-deployment checkout? (needed to find the release-signing key: \$KONIN_RELEASE_SECKEY, or a mounted drive at /media/\$USER/*/konin-release.key)"
. "$SECKEY_LIB"
SECKEY="$(find_release_seckey)"

REPO_SLUG="pantadeusz/$COMPONENT"
DEFAULT_BRANCH="$(gh repo view "$REPO_SLUG" --json defaultBranchRef -q .defaultBranchRef.name)" \
  || die "could not query default branch for $REPO_SLUG (network? gh auth?)"
[ -n "$DEFAULT_BRANCH" ] || die "gh returned an empty default branch for $REPO_SLUG"

git -C "$REPO" fetch -q origin "$DEFAULT_BRANCH" || die "fetch origin/$DEFAULT_BRANCH failed"
TARGET_SHA="$(git -C "$REPO" rev-parse "origin/$DEFAULT_BRANCH")"

CURRENT="$(git -C "$REPO" describe --tags --abbrev=0 --match 'v[0-9]*.[0-9]*.[0-9]*' "$TARGET_SHA" 2>/dev/null || echo v0.0.0)"
IFS='.' read -r CUR_MAJOR CUR_MINOR CUR_PATCH <<< "${CURRENT#v}"
case "$BUMP" in
  major) NEW="v$((CUR_MAJOR + 1)).0.0" ;;
  minor) NEW="v${CUR_MAJOR}.$((CUR_MINOR + 1)).0" ;;
  patch) NEW="v${CUR_MAJOR}.${CUR_MINOR}.$((CUR_PATCH + 1))" ;;
esac
git -C "$REPO" rev-parse -q --verify "refs/tags/$NEW" >/dev/null \
  && die "$NEW already exists — something is out of sync (fetch and check manually)"

echo "▶ $COMPONENT: $CURRENT -> $NEW (bump=$BUMP, target=origin/$DEFAULT_BRANCH @ ${TARGET_SHA:0:12})"

git -C "$REPO" tag -s "$NEW" "$TARGET_SHA" -m "$COMPONENT $NEW" \
  || die "signing tag $NEW failed — is git signing configured? (KEY-POLICY.md)"
git -C "$REPO" push origin "$NEW" || die "push of tag $NEW failed"

OUT="$(mktemp -d)"
RELEASED=0
cleanup() {
  rm -rf "$OUT"
  [ "$RELEASED" -eq 1 ] || release_rollback_tag "$REPO" "$NEW"
}
trap cleanup EXIT
echo "▶ Building"
"$HERE/build.sh" "$NEW" --out "$OUT"

echo "▶ Signing artifacts"
DEB_NAME="grbl-k-firmware_${NEW}_all.deb"
sign_with_retry "$OUT/$DEB_NAME" || die "signing $DEB_NAME failed -- wrong password entered too many times"

gh release create "$NEW" --repo "$REPO_SLUG" --target "$TARGET_SHA" \
  --title "$COMPONENT $NEW" --generate-notes "$OUT"/*.deb "$OUT"/*.deb.minisig "$OUT"/grbl-k.hex \
  || die "gh release create failed for $COMPONENT $NEW -- the tag was already pushed and the .deb was built+signed at $OUT; create the release manually: gh release create $NEW --repo $REPO_SLUG --generate-notes $OUT/*.deb $OUT/*.deb.minisig $OUT/grbl-k.hex"
RELEASED=1

echo "✓ $COMPONENT $NEW released: https://github.com/$REPO_SLUG/releases/tag/$NEW"
