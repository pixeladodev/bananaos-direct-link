#!/bin/sh
# Script checks. No handheld, no radio, no compiler.
set -eu
ROOT=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
T=$(mktemp -d)
trap 'rm -rf "$T"' EXIT
fail=0
bad() { echo "FAIL $1"; fail=1; }

for s in wifi-link host.sh join.sh down.sh status.sh tile.sh uninstall.sh fix-nest.sh; do
    sh -n "$ROOT/app/$s" || bad "syntax $s"
done
sh -n "$ROOT/tiles/Direct Link.sh" || bad "syntax tile"
sh -n "$ROOT/build/package.sh" || bad "syntax package"

grep -q 'store/apps/direct-link' "$ROOT/app/uninstall.sh" || bad "uninstall path"
grep -q 'Direct Link.sh' "$ROOT/app/uninstall.sh" || bad "uninstall tile"
grep -q '10.42.0.1' "$ROOT/app/wifi-link" || bad "host address"
grep -q '10.42.0.$n' "$ROOT/app/wifi-link" || bad "player range"
grep -q '/run/banana}/direct-link' "$ROOT/app/wifi-link" || bad "state dir"
grep -q 'direct-link.conf' "$ROOT/app/wifi-link" || bad "conf path"
grep -q 'MAX)' "$ROOT/app/wifi-link" || bad "max from conf"
grep '^exec' "$ROOT/app/host.sh" | grep -q -- '--owner' && bad "host passes --owner"
grep '^exec' "$ROOT/app/join.sh" | grep -q -- '--owner' && bad "join passes --owner"

export BANANA_LINK_DIR=$T/run BANANA_LINK_LOG=$T/link.log BANANA_LINK_CONF=$T/direct-link.conf
printf 'CHANNEL=6\nMAX=90\n' > "$BANANA_LINK_CONF"
out=$(sh "$ROOT/app/status.sh")
printf '%s\n' "$out" | grep -qx 'state=down' || bad "status down: $out"
printf '%s\n' "$out" | grep -qx 'version=0.1.0' || bad "version: $out"

if sh "$ROOT/app/uninstall.sh" >/dev/null 2>"$T/unerr"; then
    bad "uninstall ran outside the store"
else
    grep -q refusing "$T/unerr" || bad "uninstall message"
fi

APPS=$T/apps
TILES=$T/tiles
mkdir -p "$APPS/direct-link/direct-link" "$TILES"
printf 'marker\n' > "$APPS/direct-link/direct-link/marker"
cp "$ROOT/app/fix-nest.sh" "$APPS/direct-link/direct-link/fix-nest.sh"
cp "$ROOT/app/wifi-link" "$APPS/direct-link/direct-link/wifi-link"
printf 'tile\n' > "$APPS/direct-link/Direct Link.sh"
DIRECT_LINK_APPS=$APPS DIRECT_LINK_TILES=$TILES sh "$APPS/direct-link/direct-link/fix-nest.sh" >/dev/null
[ -f "$APPS/direct-link/marker" ] || bad "nest did not move the app"
[ -f "$TILES/Direct Link.sh" ] || bad "nest did not move the tile"
[ ! -d "$APPS/direct-link/direct-link" ] || bad "inner folder stayed"

[ "$fail" = 0 ] && echo "selftest ok" || { echo "selftest failed"; exit 1; }
