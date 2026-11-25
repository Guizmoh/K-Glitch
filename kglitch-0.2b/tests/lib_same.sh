#!/bin/sh
# SPDX-License-Identifier: MPL-2.0
# The site's library (tools/break_lib.py) makes each break of breaks/breaks.txt exactly as the firmware's bank does
# (tools/prep_breaks.py --list): the same level (a line's own @X, FLEXOR's breaks louder), the same grid (*= / -=),
# the same bytes. FLEXOR, 14 Nov 2026: his breaks from the library at the volume they have in the firmware.
#   tests/lib_same.sh
set -e
cd "$(dirname "$0")/.."
python3 - <<'P'
import argparse, sys
sys.path.insert(0, "tools")
import prep_breaks as pb, break_lib as bl
from pathlib import Path
args = argparse.Namespace(bars=2, slices=16, break_slices=16, rate=22050, transients=True, loudness=-12.0, name_prefix="")
fw = {shown: spec for spec, shown in pb.read_list(Path("breaks/breaks.txt"))}
n = 0
for built, file, bpm, bars, name, grid, loud, hz in bl.entries(Path("breaks/breaks.txt")):
    if not built:
        continue
    if file.startswith("flexor/"):
        assert loud == -12.0, f"{name}: FLEXOR's breaks at -12 LUFS (breaks.txt @X or flexor/'s level)"
    lib = f"breaks/{file}:{bpm}:{bars}:{name}" + (":grid" if grid else "") + (f":loud={loud}" if loud is not None else "") + \
        (f":hz={hz}" if hz else "")
    a = pb.build_bank([pb.prepare(lib, args, file)])
    b = pb.build_bank([pb.prepare(fw[file], args, file)])
    assert a == b, f"{name}: the library's differs from the firmware's"
    n += 1
print(f"lib_same: the library's {n} built-in breaks are the firmware's, byte for byte (level, grid, slices)")
P
