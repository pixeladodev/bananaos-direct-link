#!/bin/sh
# Disconnect, then delete this app and its Apps tile. The UI asks first.
set -eu
APP=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
case $APP in
    /data/store/apps/direct-link) ;;
    *) echo "refusing to remove $APP" >&2; exit 1 ;;
esac
sh "$APP/wifi-link" down || true
rm -f "/data/store/tiles/Direct Link.sh" "/data/store/tiles/._Direct Link.sh" \
    "/data/store/tiles/.DS_Store"
cd /
rm -rf "$APP"
rm -f "/data/store/apps/._direct-link" "/data/store/apps/.DS_Store"
