#!/bin/sh
# Self-extracting installer (manual). dev/deploy.sh does not call this.
#   ./build/package.sh && ./build/install-onefile.sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
ZIP=$(ls -t "$ROOT"/dist/direct-link-*.zip 2>/dev/null | head -n 1)
[ -n "$ZIP" ] || { echo "run ./build/package.sh first" >&2; exit 1; }
HDR=$(mktemp)
OUT="$ROOT/dist/Install-Direct-Link.sh"
cat > "$HDR" << 'HDR'
#!/bin/sh
set -eu
DEST=${1:-.}
mkdir -p "$DEST"
ZIP=$(mktemp)
trap 'rm -f "$ZIP"' EXIT
tail -n +ARCHIVE_LINE "$0" > "$ZIP"
unzip -oq "$ZIP" -d "$DEST"
echo "Extracted to $DEST (direct-link/, Direct Link.sh, INSTALL.txt)"
exit 0
HDR
MARK=$(($(wc -l < "$HDR") + 1))
sed "s/ARCHIVE_LINE/$MARK/" "$HDR" > "$OUT"
cat "$ZIP" >> "$OUT"
chmod 755 "$OUT"
rm -f "$HDR"
echo "$OUT"