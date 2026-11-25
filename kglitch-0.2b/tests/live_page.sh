#!/bin/sh
# SPDX-License-Identifier: MPL-2.0
# The LIVE page (LOAF BREAKER 0.9.71, the app's: app/site/live.html) end to end, in Chromium at a phone's size, its Web
# MIDI wired to the host simulator as an FM-1 over USB (tests/live_page.mjs): connect, the screen, PLAY, ALGORITHM, a
# key, DROP. The site's app pages built first (tools/site_style.py on a bare site). Needs ./build.sh --host and
# Playwright with its Chromium; without Playwright it says so and passes.
#   tests/live_page.sh
set -e
cd "$(dirname "$0")/.."
if ! node -e "require('playwright')" 2>/dev/null; then
    echo "live_page: no Playwright here, skipped"
    exit 0
fi
O=app/build/live
S="$O/site"
rm -rf "$S"
mkdir -p "$S"
cp -r app/site/img "$S/img"
python3 tools/site_style.py "$S" > /dev/null
timeout -k 5 150 node tests/live_page.mjs "$S" "$O"
