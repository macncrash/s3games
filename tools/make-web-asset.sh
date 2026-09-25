#!/bin/sh
# Pack one cartridge into a single web file, then refresh the index.
# Usage: tools/make-web-asset.sh s3pins
#        tools/make-web-asset.sh --all
#        tools/make-web-asset.sh --index
# The asset is web/play/<slug>/index.html (wasm embedded, one file).
# No local paths are written into that file or the index.
set -eu
cd "$(dirname "$0")/.."
root=$(pwd)
shell="$root/web/shell.html"

index_only=0
all=0
slug=""
if [ "${1:-}" = "--index" ]; then
  index_only=1
elif [ "${1:-}" = "--all" ]; then
  all=1
elif [ -n "${1:-}" ]; then
  slug=$1
else
  echo "usage: tools/make-web-asset.sh <slug>|--all|--index" >&2
  exit 2
fi

pack_one() {
  if ! command -v em++ >/dev/null 2>&1; then
    echo "em++ is not on PATH. Install Emscripten, then run this again." >&2
    exit 1
  fi
  name=$1
  game="$root/$name"
  if ! find "$game/src" -name '*.cpp' | grep -q .; then
    echo "skip $name (no source)" >&2
    return 0
  fi
  out="$root/web/play/$name"
  mkdir -p "$out" "$root/web/assets"
  engine=${S3_ENGINE:-$(CDPATH= cd "$root/../csys/s3rally/src" && pwd)}
  # The cartridge's own headers must come first. The console tree also
  # contains a rally game, and -I that tree first steals "game/art.h".
  srcs=$(find "$game/src" "$engine/console" -name '*.cpp' | sort)
  # shellcheck disable=SC2086
  em++ -std=c++17 -O2 -I"$game/src" -I"$engine" -DS3_BUILD='"web"' -DS3_ORG='"s3games"' -DS3_PREF="\"$name\"" \
    -sUSE_SDL=2 -sALLOW_MEMORY_GROWTH=1 -sINITIAL_MEMORY=134217728 -sSTACK_SIZE=1048576 \
    -sENVIRONMENT=web -sSINGLE_FILE=1 --shell-file "$shell" \
    $srcs -o "$out/index.html"
  printf '%s\n' "{\"slug\":\"$name\",\"page\":\"play/$name/\"}" > "$root/web/assets/$name.json"
  echo "packed $name"
}

if [ "$all" = 1 ]; then
  for dir in "$root"/s3*; do
    find "$dir/src" -name '*.cpp' | grep -q . || continue
    [ -f "$dir/Makefile" ] || continue
    base=$(basename "$dir")
    case $base in
      s3|s3rally) continue ;;
    esac
    pack_one "$base" || echo "FAILED $base" >&2
  done
elif [ "$index_only" = 0 ]; then
  pack_one "$slug"
fi

# Index. Links are relative. A packed game points at its one-file page.
{
  echo '<!doctype html><html lang="en"><head><meta charset="utf-8">'
  echo '<meta name="viewport" content="width=device-width, initial-scale=1">'
  echo '<title>S3 games</title>'
  echo '<style>body{margin:0;background:#101014;color:#f2f2f4;font:15px/1.5 ui-monospace,Menlo,monospace}'
  echo 'main{max-width:720px;margin:0 auto;padding:32px 20px}a{color:#ffc21a}li{margin:6px 0}</style></head><body><main>'
  echo '<h1>S3 games</h1><p>Each link opens that cartridge in the browser.</p><ol>'
  for dir in "$root"/s3*; do
    find "$dir/src" -name '*.cpp' | grep -q . || continue
    base=$(basename "$dir")
    [ "$base" = "s3" ] && continue
    echo "<li><a href=\"games/$base/\">$base</a> · <a href=\"play/$base/\">play</a></li>"
  done
  echo '</ol></main></body></html>'
} > "$root/web/index.html"
echo "wrote web/index.html"
