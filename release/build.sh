#!/usr/bin/env bash
# release/build.sh — build grbl-k-firmware.deb from this repo, using
# konin-deployment's Dockerfile (shared build recipe, not duplicated
# per-repo).
#
# Usage:
#   release/build.sh [ref] [--out DIR]
#
#   No architecture argument -- AVR machine code isn't tied to the build
#   host's architecture (native avr-gcc, no cross-compilation/qemu needed).
#   [ref]     git ref to build from (tag, branch, sha) -- default HEAD.
#   --out DIR output directory -- default ./dist
#
# Version: `git describe --tags --always <ref>`, stripped of its leading
# 'v'. On a clean tag this is just the tag (a real release); off a tag it
# naturally includes '-<N>-g<shortsha>' -- git's own standard mechanism,
# no hand-rolled dev-version scheme needed.
#
# Requires: KONIN_DEPLOYMENT_DIR pointing at a konin-deployment checkout
# (this repo's own build recipe lives there:
# releases/docker/grbl-k-firmware.Dockerfile). Never flashes anything --
# that's install/lib/grbl-k-flash.sh's job (konin-deployment),
# operator-triggered.
set -euo pipefail
die() { echo "release/build.sh: error: $*" >&2; exit 1; }

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO="$(cd "$HERE/.." && pwd)"

REF="HEAD" OUT="$REPO/dist"
while [ $# -gt 0 ]; do
  case "$1" in
    --out) OUT="$2"; shift 2 ;;
    -*) die "unknown flag: $1" ;;
    *) REF="$1"; shift ;;
  esac
done

[ -n "${KONIN_DEPLOYMENT_DIR:-}" ] \
  || die "KONIN_DEPLOYMENT_DIR is not set -- point it at a konin-deployment checkout (needed for releases/docker/grbl-k-firmware.Dockerfile)"
[ -d "$KONIN_DEPLOYMENT_DIR" ] \
  || die "KONIN_DEPLOYMENT_DIR=$KONIN_DEPLOYMENT_DIR does not exist"
DOCKERFILE="$KONIN_DEPLOYMENT_DIR/releases/docker/grbl-k-firmware.Dockerfile"
[ -f "$DOCKERFILE" ] \
  || die "$DOCKERFILE not found -- is KONIN_DEPLOYMENT_DIR pointing at a real konin-deployment checkout?"

git -C "$REPO" rev-parse -q --verify "$REF^{commit}" >/dev/null \
  || die "ref '$REF' not found in this repo"
DESCRIBE="$(git -C "$REPO" describe --tags --always "$REF")"
VERSION="v${DESCRIBE#v}"
DEB_REVISION="${DEB_REVISION:-1}"

STAGE="$(mktemp -d)"
trap 'rm -rf "$STAGE"' EXIT
mkdir -p "$STAGE/src"
git -C "$REPO" archive "$REF" | tar -x -C "$STAGE/src"
[ -d "$STAGE/src/debian" ] || die "ref '$REF' has no debian/ packaging directory"
cp -r "$STAGE/src/debian" "$STAGE/debian"

docker buildx build --pull --platform linux/amd64 \
  -f "$DOCKERFILE" \
  --target export \
  --build-arg "VERSION=$VERSION" --build-arg "DEB_REVISION=$DEB_REVISION" \
  --output "type=local,dest=$STAGE/out" \
  "$STAGE"

[ -f "$STAGE/out/grbl-k.hex" ] || die "build produced no .hex"
[ -f "$STAGE/out/grbl-k-firmware.deb" ] || die "build produced no .deb"
mkdir -p "$OUT"
cp "$STAGE/out/grbl-k.hex" "$OUT/grbl-k.hex"
cp "$STAGE/out/grbl-k-firmware.deb" "$OUT/grbl-k-firmware.deb"
echo "built $OUT/grbl-k-firmware.deb ($(du -h "$OUT/grbl-k-firmware.deb" | cut -f1)), version $VERSION"
