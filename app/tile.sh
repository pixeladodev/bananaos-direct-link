#!/bin/sh
# Apps shelf entry. The menu has already let go of both panels.
APP=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
chmod a+x "$APP/ui/direct-link" "$APP/wifi-link" "$APP/wifi-guard.sh" "$APP/host.sh" "$APP/join.sh" \
    "$APP/down.sh" "$APP/status.sh" "$APP/uninstall.sh" "$APP/fix-nest.sh" 2>/dev/null || true
exec "$APP/ui/direct-link"
