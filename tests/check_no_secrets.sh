#!/usr/bin/env bash
# Heuristic scan of tracked/untracked-but-not-ignored files for credential-like
# content. Prints file names only, never matched values.
set -u
root="$(cd "$(dirname "$0")/.." && pwd)"; cd "$root"
status=0
files=$(git ls-files --cached --others --exclude-standard | grep -v -E '\.(png|jpg|jpeg)$')
# 32-hex strings (typical API key shape), and quoted non-placeholder SSID/password/key assignments
for f in $files; do
  if grep -E -q '[0-9a-fA-F]{32}' "$f"; then echo "possible key-like string: $f"; status=1; fi
  if grep -E -q '#define +(WIFI_SSID|WIFI_PASSWORD|WEATHER_API_KEY) +"' "$f" \
     && grep -E '#define +(WIFI_SSID|WIFI_PASSWORD|WEATHER_API_KEY) +"' "$f" | grep -v -q 'YOUR_'; then
    echo "non-placeholder credential define: $f"; status=1; fi
done
git check-ignore -q secrets.h || { echo "secrets.h is not git-ignored"; status=1; }
git ls-files --error-unmatch secrets.h >/dev/null 2>&1 && { echo "secrets.h is tracked"; status=1; }
[ $status = 0 ] && echo "secret scan OK"
exit $status
