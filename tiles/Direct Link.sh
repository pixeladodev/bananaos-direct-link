#!/bin/sh
# Apps shelf tile. bash runs this, so this file needs no +x.
# One folder too deep (store/apps/direct-link/direct-link/) still opens.
ROOT=${DIRECT_LINK_APPS:-/data/store/apps}
APP=$ROOT/direct-link
if [ ! -f "$APP/tile.sh" ] && [ -f "$APP/direct-link/tile.sh" ]; then
    exec sh "$APP/direct-link/tile.sh"
fi
exec sh "$APP/tile.sh"
