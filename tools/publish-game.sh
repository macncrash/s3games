#!/bin/sh
# Commit one finished cartridge and push it. The message is the slug only.
# Refuses to commit if a home-directory path or the retired console name is in the files.
# Usage: tools/publish-game.sh s3pins
set -eu
cd "$(dirname "$0")/.."
slug=${1:?usage: tools/publish-game.sh <slug>}
case $slug in
  s3*) ;;
  *) echo "slug must start with s3" >&2; exit 2 ;;
esac
if [ ! -d "$slug/src" ]; then
  echo "no source for $slug" >&2
  exit 1
fi
home=$(printf '%s%s' '/Us' 'ers/')
old=$(printf '%s%s' 'GEN' 'SYS')
if grep -R -n -I -e "$home" -e "$old" "$slug" --include='*.cpp' --include='*.h' --include='*.html' --include='Makefile' --include='*.mk' >/dev/null 2>&1; then
  echo "refusing $slug: local path or retired name in source" >&2
  exit 1
fi
git add -- "$slug/Makefile" "$slug/src"
if [ -d "$slug/web" ]; then
  git add -- "$slug/web"
fi
if [ -f "$slug/README.md" ]; then
  git add -- "$slug/README.md"
fi
if git diff --cached --quiet; then
  echo "nothing to commit for $slug"
  exit 0
fi
git -c user.name=macncrash -c user.email=macncrash@users.noreply.github.com commit -m "Add $slug."
if [ "${NO_PUSH:-}" != 1 ]; then
  git push origin HEAD
fi
