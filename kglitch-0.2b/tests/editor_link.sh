#!/bin/sh
# SPDX-License-Identifier: MPL-2.0
# The sound editor's link (LOAF BREAKER 0.9.43, the site's editor/ page; ui/main.c ed_sysex) end to end in the host
# simulator: ED_INFO's sections and parts; a kick part, a drum page and the kick EQ read and turned (their texts change,
# the FM-1's screen stays where it was); FX1's effect and a knob of it; the kick's wave; KICK n; the Studio's ED_X (status, pages,
# a track written, the patterns, RGB, transport, the live scope); the sequencer (ED_SEQ,
# ED_STEP): a kick step, a snare step's HITS, the hat's LENGTH, a break step's SLICE, a FILL undone.
# Needs ./build.sh --host first.
#   tests/editor_link.sh
set -e
cd "$(dirname "$0")/.."
O=app/build/editor
mkdir -p "$O"
fr() { python3 -c "import sys; sys.path.insert(0,'tools'); import cu_frames as c; print('sysex ' + c.hexs(c.frame($1, bytes([$2]))))"; }
{
echo "wait 4200"
echo "sysex_log $PWD/$O/out.syx"
echo "sysex_out"
echo "expect page 0"
fr 7 ""                                   # ED_INFO
echo "wait 50"
fr 8 "0, 1, 0, 0"                          # the kick's SUB part, read
echo "wait 50"
fr 8 "0, 1, 0, 6"                          # its LEVEL, 6 clicks right
echo "wait 50"
fr 8 "1, 0, 1, 0"                          # the snare's SOUND page, read
echo "wait 50"
fr 8 "1, 0, 1, 250"                        # its TUNE, 6 clicks left
echo "wait 50"
fr 8 "0, 11, 0, 4"                         # the kick EQ (EQ B), LOW 4 clicks right
echo "wait 50"
fr 8 "0, 13, 0, 0"                         # FX1, read
echo "wait 50"
fr 8 "0, 13, 4, 2"                         # FX1: PRESETS, the effect two on
echo "wait 50"
fr 8 "0, 13, 0, 5, 0"                      # FX1: its K1 five clicks right, MAIN
echo "wait 50"
fr 10 "244, 1"                             # the kick's wave over 500 ms
echo "wait 50"
echo "expect page 0"
fr 9 "1, 4"                                # KICK 5
echo "wait 100"
echo "expect kslot 4"
echo "expect page 0"
fr 11 ""                                  # ED_SEQ: the pattern
echo "wait 50"
fr 12 "1, 0, 1, 0, 0"                      # the kick's step 2 on / off
echo "wait 50"
fr 12 "2, 1, 4, 1, 1"                      # the snare's step 5: HITS one more
echo "wait 50"
fr 12 "3, 2, 0, 3, 1"                      # the hat's lane: LENGTH longer
echo "wait 50"
fr 12 "4, 0, 3, 0, 2"                      # the break's step 4: SLICE two on
echo "wait 50"
fr 12 "3, 3, 0, 0, 3"                      # the clap's lane: FILL, three more hits
echo "wait 50"
fr 12 "6, 0, 0, 0, 0"                      # SEL + SAVE's undo: the FILL
echo "wait 50"
fr 13 "1, 0, 0, 0, 0, 0"                   # ED_X STATUS
echo "wait 50"
fr 13 "2, 5, 2, 0, 3, 1"                   # MIX's MASTER: its MAX OUT one click
echo "wait 50"
fr 13 "2, 2, 1, 0, 4, 1"                   # FX slot 2: the effect one on
echo "wait 50"
fr 13 "2, 3, 5, 0, 1, 5"                   # SYSTEM's RESET: refused
echo "wait 50"
fr 13 "4, 1, 1, 0, 5, 9, 254$(python3 -c "print(', 0' * 29 + ', 4, 2, 1' + ', 1' * 29 + ', 0' * 32 + ', 3, 0, 0' + ', 0' * 29)")"   # TRACK 2 written: P5 x4, P9 x2, a DROP
echo "wait 50"
fr 13 "6, 0"                               # the patterns P1-16
echo "wait 50"
fr 13 "7, 4, 0, 0"                         # the break's RGB wave
echo "wait 50"
fr 13 "8, 4, 0, 0"                         # the breaks' names
echo "wait 50"
fr 13 "8, 1, 124, 6"                       # the tempo 166.0
echo "wait 50"
fr 13 "8, 3, 0, 0"                         # PLAY
echo "wait 400"
fr 13 "1, 1, 0, 0, 0, 0"                   # STATUS with the live scope
echo "wait 50"
echo "expect page 0"
echo "sysex_log off"
} > "$O/editor.txt"
rm -f "$O/out.syx"
app/build/host/loaf-breaker_host "$O/editor.txt" app/build/scenarios > "$O/run.log" 2>&1 || { grep -E "FAIL|host:" "$O/run.log"; exit 1; }
python3 - "$O/out.syx" <<'P'
import sys
raw = open(sys.argv[1], "rb").read()
def unpack(p):
    u = bytearray()
    for i in range(0, len(p), 8):
        hi = p[i]
        u += bytes(p[i + j] | ((hi >> (j - 1)) & 1) << 7 for j in range(1, min(8, len(p) - i)))
    return bytes(u)
rep = []
for f in raw.split(b"\xf0")[1:]:
    f = f.split(b"\xf7")[0]
    if f[:3] == bytes([0x7D, 0x43, 0x48]) and 0x47 <= f[3] <= 0x4D:
        rep.append((f[3] & 15, f[4], unpack(f[5:])))
def knobs(d):
    return [(d[2 + 29 * i:14 + 29 * i].rstrip(b"\0").decode(), d[14 + 29 * i:30 + 29 * i].rstrip(b"\0").decode(), d[30 + 29 * i]) for i in range(4)]
info = rep[0][2]
assert rep[0][0] == 7 and info[0] == 1, "ED_INFO"
k, secs = 4, []
for s in range(info[3]):
    name = info[k:k + 8].rstrip(b"\0").decode(); n = info[k + 8]; k += 9
    parts = []
    for j in range(n):
        parts.append((info[k], info[k + 1:k + 13].rstrip(b"\0").decode(), info[k + 13])); k += 14
    secs.append((name, parts))
print("editor_link: sections " + ", ".join(f"{n} ({len(p)})" for n, p in secs))
sub0, sub1, sn0, sn1, eq, fx0, fx1, fx2, wave = rep[1:10]
a, b = knobs(sub0[2]), knobs(sub1[2])
assert a[0][0] == "LEVEL" and a[0][1] != b[0][1] and b[0][2] > a[0][2], f"kick SUB LEVEL {a[0]} -> {b[0]}"
c, d = knobs(sn0[2]), knobs(sn1[2])
assert c[1][0] == "TUNE" and c[1][1] != d[1][1], f"snare TUNE {c[1]} -> {d[1]}"
e = knobs(eq[2])
assert e[0][0] == "LOW" and e[0][1] != "0 DB", f"kick EQ LOW {e[0]}"
def fx(r):
    d = r[2]; k = 2 + 4 * 29
    return knobs(d), d[k:k + 12].rstrip(b"\0").decode(), d[k + 12], d[k + 14]
f0, f1, f2 = fx(fx0), fx(fx1), fx(fx2)
assert fx0[1] == 0 and f1[1] != f0[1] and f1[2] == 1 and f1[3] >= 1, f"FX1's effect {f0[1:]} -> {f1[1:]}"
assert f2[0][0][0] and f2[0][0][1] != f1[0][0][1], f"FX1's K1 {f1[0][0]} -> {f2[0][0]}"
assert wave[0] == 10 and len(wave[2]) == 240 and max(abs((v ^ 0x80) - 0x80) for v in wave[2]) > 20, "the kick's wave"
def sq(d):
    return dict(pat=d[1], bars=list(d[7:11]), mask=[[d[18 + 8 * r + 2 * c] | d[19 + 8 * r + 2 * c] << 8 for c in range(4)] for r in range(4)],
                sb=list(d[50:210]), brk=[tuple(d[274 + 3 * i:277 + 3 * i]) for i in range(32)], n=len(d))
s0, s1, s2, s3, s4, s5, s6 = (sq(r[2]) for r in rep[11:18])
assert all(r[0] == 11 for r in rep[11:18]) and s0["n"] == 370, f"ED_SEQ {s0['n']}"
assert (s1["mask"][0][0] ^ s0["mask"][0][0]) == 2, "the kick's step 2"
assert (s2["sb"][64 + 4] & 3) == (s1["sb"][64 + 4] & 3) + 1, "the snare's step 5 HITS"
assert s3["bars"][2] > s2["bars"][2], f"the hat's LENGTH {s2['bars']} -> {s3['bars']}"
assert s4["brk"][3][0] == s3["brk"][3][0] + 2, f"the break's step 4 {s3['brk'][3]} -> {s4['brk'][3]}"
assert bin(s5["mask"][3][0]).count("1") > bin(s4["mask"][3][0]).count("1") and s6["mask"][3][0] == s4["mask"][3][0], "FILL, undone"
print(f"editor_link: the sequencer: a kick step, a snare step's HITS, the hat {16 * s2['bars'][2]} -> {16 * s3['bars'][2]} steps, a break step's slice, the clap's FILL undone")
x = rep[18:]
assert all(r[0] == 13 for r in x) and len(x) == 11, f"ED_X {[(r[0], len(r[2])) for r in x]}"
st, mx, fxs, rst, trk, bnk, rgb, names, tempo, play, live = x
def s12(d, a, n=12):
    return d[a:a + n].split(b"\0")[0].decode()
assert st[2][0] == 1 and (st[2][3] | st[2][4] << 8) >= 400, "STATUS"
assert s12(mx[2], 4 + 3 * 29) == "MAX OUT" and mx[1] == 0, f"MIX MASTER {s12(mx[2], 4 + 3 * 29)}"
assert fxs[1] == 0 and len(fxs[2]) > 120 and s12(fxs[2], 120), "an FX slot's effect"
assert rst[1] == 2, "RESET refused"
t = trk[2]
assert list(t[7:11]) == [5, 9, 254, 0] and t[39] == 4 and t[40] == 2, f"TRACK 2 {list(t[7:11])} {list(t[39:42])}"
assert len(bnk[2]) == 2 + 16 * 40, "the patterns"
assert len(rgb[2]) >= 2 + 960 + 2 and rgb[2][963] > 0 and max(rgb[2][5:962:4]) > 0, "the break's RGB"
n = names[2][1]
assert n >= 1 and s12(names[2], 2), "the breaks' names"
assert (tempo[2][3] | tempo[2][4] << 8) == 1660, "the tempo"
assert play[2][0] == 1 and live[2][2] == 1 and live[2][31] > 0, f"PLAY, the live scope ({live[2][31]} columns)"
print(f"editor_link: the Studio: STATUS, MIX MASTER, FX slot 2 {s12(fxs[2], 120)}, RESET refused, TRACK 2 written, the patterns, the break's RGB ({rgb[2][963]} slices), {n} breaks, 166 BPM, PLAY, {live[2][31]} live columns")
print(f"editor_link: kick SUB LEVEL {a[0][1]} -> {b[0][1]}, snare TUNE {c[1][1]} -> {d[1][1]}, kick EQ LOW {e[0][1]}; FX1 {f0[1]} -> {f1[1]}, its {f2[0][0][0]} {f1[0][0][1]} -> {f2[0][0][1]}; the wave; KICK 5; the screen kept")
P
