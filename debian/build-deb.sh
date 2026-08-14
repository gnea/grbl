#!/bin/sh
# build-deb.sh — assemble grbl-k-firmware.deb: the compiled .hex under
# /usr/share/grbl-k-firmware/ (read-only, arch-independent data, like a
# Debian firmware-* package) plus the full GPLv3 source under
# /usr/share/doc/grbl-k-firmware/ (D-032: shipping source alongside the
# binary is the clean way to satisfy the license, no confidentiality reason
# to trim it — it's already public at github.com/pantadeusz/grbl-k).
# Architecture: all — the .hex is AVR machine code, but the *package* isn't
# tied to the host's architecture, same reasoning Debian firmware-* packages
# use. Never auto-flashes; that's install/lib/grbl-k-flash.sh's job,
# operator-triggered (D-032/D-034).
#
# Usage: build-deb.sh <src-tree> <hex-file> <version> <deb-revision> <output-path>
#   <version> must be SemVer, vMAJOR.MINOR.PATCH[-pre][+build] (D-034).
set -eu

SRCTREE="$1"   # extracted grbl-k source (full tree, git archive)
HEX="$2"       # compiled grbl.hex
SRC=/debian-src
PKGROOT=/pkg
VERSION="$3"
DEB_REVISION="$4"
OUT="$5"

case "$VERSION" in
  v[0-9]*.[0-9]*.[0-9]*) : ;;
  *) echo "build-deb.sh: version must be SemVer, vMAJOR.MINOR.PATCH[-pre][+build] — got '$VERSION'" >&2; exit 1 ;;
esac
DEB_VERSION="${VERSION#v}-${DEB_REVISION}"

install -d "$PKGROOT/usr/share/grbl-k-firmware" \
           "$PKGROOT/usr/share/doc/grbl-k-firmware" \
           "$PKGROOT/DEBIAN"

install -m 644 "$HEX" "$PKGROOT/usr/share/grbl-k-firmware/grbl-k.hex"
sha256sum "$HEX" | cut -d' ' -f1 > "$PKGROOT/usr/share/grbl-k-firmware/grbl-k.hex.sha256"

tar -C "$SRCTREE" -czf "$PKGROOT/usr/share/doc/grbl-k-firmware/source.tar.gz" .
install -m 644 "$SRC/copyright" "$PKGROOT/usr/share/doc/grbl-k-firmware/copyright"

INSTALLED_SIZE=$(du -sk "$PKGROOT/usr" | awk '{s+=$1} END {print s}')

sed -e "s/@VERSION@/$DEB_VERSION/" -e "s/@INSTALLED_SIZE@/$INSTALLED_SIZE/" \
  "$SRC/control" > "$PKGROOT/DEBIAN/control"

dpkg-deb --build --root-owner-group "$PKGROOT" "$OUT"
echo "--- built $OUT ---"
dpkg-deb --info "$OUT"
