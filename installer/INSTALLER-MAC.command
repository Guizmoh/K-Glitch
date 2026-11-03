#!/bin/sh
# K-GLITCH : ouvre l'installateur dans Chrome ou Edge (le Web MIDI ne marche pas dans Safari ni Firefox)
cd "$(dirname "$0")"
PAGE="$PWD/K-GLITCH-0.9-installer.html"
open -a "Google Chrome" "$PAGE" 2>/dev/null || open -a "Microsoft Edge" "$PAGE" 2>/dev/null || {
  echo "Chrome ou Edge introuvable : ouvrez K-GLITCH-0.9-installer.html avec Chrome ou Edge."; read -r _; }
