#!/bin/sh
# Move store/apps/direct-link/direct-link/ up one level. The UI asks first.
set -eu
INNER=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
OUTER=$(CDPATH= cd -- "$INNER/.." && pwd)
APPS=${DIRECT_LINK_APPS:-/data/store/apps}
TILES=${DIRECT_LINK_TILES:-/data/store/tiles}
case $INNER in
    "$APPS/direct-link/direct-link") ;;
    *) echo "not a nested copy"; exit 1 ;;
esac
if [ -f "$OUTER/manifest" ] || [ -x "$OUTER/wifi-link" ]; then
    echo "store/apps/direct-link already has the app"
    exit 1
fi
mkdir -p "$TILES"
if [ -f "$OUTER/Direct Link.sh" ]; then
    mv -f "$OUTER/Direct Link.sh" "$TILES/Direct Link.sh"
fi
rm -f "$OUTER/INSTALL.txt" "$OUTER/.DS_Store" 2>/dev/null || true
rm -f "$OUTER"/._* 2>/dev/null || true
cd "$INNER"
for f in .[!.]* ..?* *; do
    [ -e "$f" ] || [ -L "$f" ] || continue
    mv "$f" "$OUTER/$f"
done
cd /
rmdir "$INNER" 2>/dev/null || true
echo moved
