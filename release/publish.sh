#!/usr/bin/env bash
# release/publish.sh — publish either a tagged release or a snapshot package
# for this repo.
#
# Usage:
#   release/publish.sh [major|minor|patch]
#   release/publish.sh snapshot [ref]
#
# Bump defaults to "patch" (0.1.2 -> 0.1.3); pass "minor"/"major" to bump
# those instead (resets lower components to 0, SemVer, D-034). Tags this
# repo's default branch tip (auto-detected via gh, signed with the pinned
# git-signing key, KEY-POLICY.md), builds grbl-k-firmware.deb
# (release/build.sh -- AVR machine code, no arch selection needed), and
# creates a GitHub Release with the tag, auto-generated notes, and the .deb
# + .hex assets attached.
#
# Snapshot mode publishes the current state of [ref] into konin-releases using
# the build script's own hash-suffixed version string. It does not create a
# git tag or GitHub Release.
# This is GPLv3, already-public source (github.com/pantadeusz/grbl-k) --
# the release itself is a public GitHub Release, same as any other tag on
# this repo.
#
# Requires:
#   - gh (GitHub CLI), authenticated
#   - KONIN_DEPLOYMENT_DIR (see release/build.sh's own requirement)
#
# Any failure after the release tag is pushed — a build failure, gh release
# create failing, or package publish failing — rolls the tag back (local +
# remote) rather than leaving a stray pushed tag with no matching release
# behind.
set -euo pipefail
die() { echo "release/publish.sh: error: $*" >&2; exit 1; }

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO="$(cd "$HERE/.." && pwd)"
COMPONENT="$(basename "$REPO")"

MODE="${1:-patch}"
REF_ARG="${2:-}"
case "$MODE" in
  major|minor|patch|snapshot) : ;;
  *) die "usage: release/publish.sh [major|minor|patch] | snapshot [ref]" ;;
esac

command -v gh >/dev/null || die "gh (GitHub CLI) not installed"
gh auth status >/dev/null 2>&1 || die "gh not authenticated — run: gh auth login"

[ -n "${KONIN_DEPLOYMENT_DIR:-}" ] \
  || die "KONIN_DEPLOYMENT_DIR is not set -- point it at a konin-deployment checkout"
[ -d "$KONIN_DEPLOYMENT_DIR" ] \
  || die "KONIN_DEPLOYMENT_DIR=$KONIN_DEPLOYMENT_DIR does not exist"
HELPERS_LIB="$KONIN_DEPLOYMENT_DIR/releases/lib/release-helpers.sh"
[ -f "$HELPERS_LIB" ] \
  || die "$HELPERS_LIB not found -- is KONIN_DEPLOYMENT_DIR pointing at a real konin-deployment checkout?"
. "$HELPERS_LIB"
PUSH_TO_RELEASES="$KONIN_DEPLOYMENT_DIR/releases/lib/push-to-konin-releases.sh"
[ -f "$PUSH_TO_RELEASES" ] \
  || die "$PUSH_TO_RELEASES not found -- is KONIN_DEPLOYMENT_DIR pointing at a real konin-deployment checkout?"

REPO_SLUG="pantadeusz/$COMPONENT"
DEFAULT_BRANCH="$(gh repo view "$REPO_SLUG" --json defaultBranchRef -q .defaultBranchRef.name)" \
  || die "could not query default branch for $REPO_SLUG (network? gh auth?)"
[ -n "$DEFAULT_BRANCH" ] || die "gh returned an empty default branch for $REPO_SLUG"

git -C "$REPO" fetch -q origin "$DEFAULT_BRANCH" || die "fetch origin/$DEFAULT_BRANCH failed"
TARGET_SHA="$(git -C "$REPO" rev-parse "origin/$DEFAULT_BRANCH")"

CURRENT="$(git -C "$REPO" describe --tags --abbrev=0 --match 'v[0-9]*.[0-9]*.[0-9]*' "$TARGET_SHA" 2>/dev/null || echo v0.0.0)"
if [ "$MODE" = snapshot ]; then
  SNAPSHOT_REF="${REF_ARG:-HEAD}"
  git -C "$REPO" rev-parse -q --verify "$SNAPSHOT_REF^{commit}" >/dev/null \
    || die "snapshot ref '$SNAPSHOT_REF' not found"
  VERSION="v$(git -C "$REPO" describe --tags --always "$SNAPSHOT_REF")"
  OUT="$(mktemp -d)"
  trap 'rm -rf "$OUT"' EXIT
  echo "▶ $COMPONENT snapshot: $VERSION (ref=$SNAPSHOT_REF)"
  echo "▶ Building"
  "$HERE/build.sh" "$SNAPSHOT_REF" --out "$OUT"
  DEB_NAME="grbl-k-firmware_${VERSION}_all.deb"
  echo "▶ Publishing package + hex into konin-releases/packages/grbl-k"
  "$PUSH_TO_RELEASES" grbl-k "$OUT/$DEB_NAME" "$OUT/grbl-k.hex"
  echo "✓ $COMPONENT snapshot $VERSION published to konin-releases/packages/grbl-k"
  exit 0
fi

IFS='.' read -r CUR_MAJOR CUR_MINOR CUR_PATCH <<< "${CURRENT#v}"
case "$MODE" in
  major) NEW="v$((CUR_MAJOR + 1)).0.0" ;;
  minor) NEW="v${CUR_MAJOR}.$((CUR_MINOR + 1)).0" ;;
  patch) NEW="v${CUR_MAJOR}.${CUR_MINOR}.$((CUR_PATCH + 1))" ;;
esac
git -C "$REPO" rev-parse -q --verify "refs/tags/$NEW" >/dev/null \
  && die "$NEW already exists — something is out of sync (fetch and check manually)"

echo "▶ $COMPONENT: $CURRENT -> $NEW (bump=$MODE, target=origin/$DEFAULT_BRANCH @ ${TARGET_SHA:0:12})"

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
DEB_NAME="grbl-k-firmware_${NEW}_all.deb"

gh release create "$NEW" --repo "$REPO_SLUG" --target "$TARGET_SHA" \
  --title "$COMPONENT $NEW" --generate-notes "$OUT"/*.deb "$OUT"/grbl-k.hex \
  || die "gh release create failed for $COMPONENT $NEW -- the tag was already pushed and both artifacts were built at $OUT; create the release manually: gh release create $NEW --repo $REPO_SLUG --generate-notes $OUT/*.deb $OUT/grbl-k.hex"
RELEASED=1

echo "▶ Publishing package + hex into konin-releases/packages/grbl-k"
"$PUSH_TO_RELEASES" grbl-k "$OUT/$DEB_NAME" "$OUT/grbl-k.hex"

echo "✓ $COMPONENT $NEW released: https://github.com/$REPO_SLUG/releases/tag/$NEW"
