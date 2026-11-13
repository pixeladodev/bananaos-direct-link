#!/bin/sh
# Apps shelf entry. The menu has already let go of both panels.
APP=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
chmod a+x "$APP/ui/direct-link" "$APP/wifi-link" "$APP/wifi-guard.sh" "$APP/host.sh" "$APP/join.sh" \
    "$APP/down.sh" "$APP/status.sh" "$APP/uninstall.sh" "$APP/fix-nest.sh" 2>/dev/null || true
# the shelf's picture for this tile: banana-ui looks for store/tiles/media/box/<tile name>.png
BOX=/data/store/tiles/media/box
if [ -f "$APP/tile.png" ] && ! cmp -s "$APP/tile.png" "$BOX/Direct Link.png" 2>/dev/null; then
    mkdir -p "$BOX" 2>/dev/null && cp "$APP/tile.png" "$BOX/Direct Link.png" 2>/dev/null || true
fi
exec "$APP/ui/direct-link"
