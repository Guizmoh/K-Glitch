#!/usr/bin/env bash
# SPDX-License-Identifier: MPL-2.0
# LOAF BREAKER's release numbers (.github/workflows/loaf.yml). Two numbers for each release:
#   V    what people see: the tag loaf-vV, the splash, the files. Since 12 Nov 2026 (FLEXOR) A.B.C: 0.9.1, 0.9.2 ...
#        (each release the last patch + 1); before, X.YY: 0.02 .. 0.267 (and GLOUPI BREAKS' gloupi-vX.YY)
#   PKG  the package's own, for fm1-base's build: X.Y[.Z], two digits each (the FM-1's identity FM-1_2623XXYYZZ); the
#        installer only installs a higher one over a lower one, so PKG must always go up with V:
#          0.02 .. 0.99      as they are
#          0.100 .. 0.198    0.99.1 .. 0.99.99
#          0.199 .. 0.267    1.0.0 .. 1.0.68 (0.220 is 1.0.21)
#          A.B.C             A+2 . B . C (0.9.1 is 2.9.1, 1.0.0 is 3.0.0: above every X.YY; A up to 97, B and C 99)
# ./LOAF_FROM: the lowest the next release may be (0.9.1). The next is the last A.B.C tag's patch + 1, or LOAF_FROM if
# that is higher: write 0.10.0 or 1.0.0 there (and push) for a new minor or major.
#   scripts/loaf_version.sh next [TAG ...]   the next V, from these tags' numbers (none given: origin's tags)
#   scripts/loaf_version.sh pkg V            V's PKG
#   scripts/loaf_version.sh check            the X.YY from 0.02 to 0.267, then A.B.C on: each PKG higher than the last
set -e
cd "$(dirname "$0")/.."

pkg() {                                  # V -> PKG
    case "$1" in
    *.*.*)                               # (A.B.C: A+2 . B . C)
        a="${1%%.*}"; r="${1#*.}"; b="${r%%.*}"; c="${r#*.}"
        case "$a.$b.$c" in *[!0-9.]*|*..*|.*|*.) echo "loaf_version: '$1' is not A.B.C" >&2; exit 1 ;; esac
        echo "$((10#$a + 2)).$((10#$b)).$((10#$c))"; return ;;
    esac
    maj="${1%%.*}"; min="${1#*.}"
    case "$maj.$min" in *[!0-9.]*|.*|*.) echo "loaf_version: '$1' is not X.YY" >&2; exit 1 ;; esac
    m=$((10#$min))
    if [ "$maj" -ge 1 ]; then
        echo "$((maj + 1)).$((m / 100)).$((m % 100))"
    elif [ "${#min}" -le 2 ]; then
        echo "$1"
    elif [ "$m" -ge 199 ]; then
        echo "1.$(((m - 199) / 100)).$(((m - 199) % 100))"
    else
        echo "0.99.$((m - 99))"
    fi
}

key() {                                  # PKG -> a number that sorts as the installer does (X, Y, Z)
    x="${1%%.*}"; r="${1#*.}"; y="${r%%.*}"; z=0
    [ "$r" != "$y" ] && z="${r#*.}"
    echo $((x * 10000 + 10#$y * 100 + 10#$z))
}

case "$1" in
pkg)
    pkg "$2" ;;
next)
    shift
    if [ $# -gt 0 ]; then
        NUMS="$(printf '%s\n' "$@")"
    else                                 # (loaf-v and gloupi-v: one count)
        NUMS="$(git ls-remote --tags origin 'refs/tags/loaf-v*' 'refs/tags/gloupi-v*' | sed 's|\^{}$||' \
                | sed -n 's|.*refs/tags/[a-z]*-v||p')"
    fi
    FROM="$(tr -d '[:space:]' < LOAF_FROM 2>/dev/null || echo 0.9.1)"
    LAST="$(printf '%s\n' "$NUMS" | grep -E '^[0-9]+\.[0-9]+\.[0-9]+$' | sort -t. -k1,1n -k2,2n -k3,3n | tail -1)"
    if [ -z "$LAST" ]; then
        echo "$FROM"
    else
        N="${LAST%.*}.$((10#${LAST##*.} + 1))"
        if [ "$(key "$(pkg "$FROM")")" -gt "$(key "$(pkg "$N")")" ]; then echo "$FROM"; else echo "$N"; fi
    fi ;;
check)
    last=-1; n=0
    for v in $(m=2; while [ $m -le 267 ]; do printf '0.%02d ' $m; m=$((m + 1)); done) \
             $(for b in 9 10 11 99; do for c in 0 1 2 50 99; do printf '0.%d.%d ' $b $c; done; done) \
             $(for a in 1 2 97; do for b in 0 1 99; do for c in 0 1 99; do printf '%d.%d.%d ' $a $b $c; done; done; done); do
        p="$(pkg "$v")"; k="$(key "$p")"
        for part in $(echo "$p" | tr '.' ' '); do
            [ "$part" -le 99 ] || { echo "loaf_version: $v -> $p, a part over 99" >&2; exit 1; }
        done
        [ "$k" -gt "$last" ] || { echo "loaf_version: $v -> $p is not above the one before" >&2; exit 1; }
        last="$k"; n=$((n + 1))
    done
    echo "loaf_version: $n versions, each package number above the last ($(pkg 0.267) for 0.267, $(pkg 0.9.1) for 0.9.1, $(pkg 1.0.0) for 1.0.0)" ;;
*)
    sed -n '3,15p' "$0" >&2; exit 1 ;;
esac
