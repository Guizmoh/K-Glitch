#!/usr/bin/env python3
# SPDX-License-Identifier: MPL-2.0
"""LOAF BREAKER (0.9.32, FLEXOR): the BREAKS page's library. Every break of breaks/breaks.txt (in the firmware or not)
made exactly as the firmware's bank makes them (tools/prep_breaks.py: levelled to -12 LUFS (0.9.70, FLEXOR: every break alike; -16 before), sixteen slices on the
hits, IMA ADPCM), each a bank of one slot the page plays and sends as it is; a slow break at a lower rate so its bars
fit the FM-1's buffer (the page's own rule for a sample). And index.json, the list the page shows.

  tools/break_lib.py OUTDIR            (tools/site_style.py: site/breaks/lib)
"""
import argparse
import json
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
import prep_breaks as pb  # noqa: E402

ROOT = Path(__file__).resolve().parent.parent
FOLDER = {"cw": "CYBERWORM AMENS", "huge": "HUGE AMEN PACK", "classics": "CLASSICS", "flexor": "FLEXOR"}


def entries(path):
    """every line of the catalogue: (built in, file, bpm, bars, name, on an even grid)"""
    out = []
    for line in path.read_text(encoding="utf-8").splitlines():
        f = line.split(None, 5)
        if len(f) == 6 and f[0] in ("*", "-", "*=", "-="):
            name, loud, hz = pb.name_opts(f[5])
            out.append((f[0].startswith("*"), f[1], float(f[2]), f[3] if "+" in f[3] else int(f[3]), name, f[0].endswith("="), pb.line_loud(f[1], loud), hz))
    return out


def titles(readme):
    """(0.9.40, FLEXOR) a folder's README table: | `FILE.wav` | Artist, "Track" (kind) | ... -> {FILE: "Artist - Track (kind)"},
    the library's full name for a break whose name on the FM-1 is short"""
    out = {}
    if not readme.exists():
        return out
    for line in readme.read_text(encoding="utf-8").splitlines():
        m = re.match(r'\|\s*`([^`]+)`\s*\|\s*([^|"]+?),\s*"([^"]+)"\s*(\([^)]*\))?\s*\|', line)
        if m:
            out[m.group(1)] = f"{m.group(2).strip()} \u2013 {m.group(3).strip()}" + (f" {m.group(4)}" if m.group(4) else "")
    return out


def main(outdir):
    out = Path(outdir)
    out.mkdir(parents=True, exist_ok=True)
    index = []
    full = {f"flexor/{k}": v for k, v in titles(ROOT / "breaks" / "flexor" / "README.md").items()}
    for built, file, bpm, bars, name, grid, loud, hz in entries(ROOT / "breaks" / "breaks.txt"):
        nb = len(bars.split("+")) if isinstance(bars, str) else bars   # (bars "3+4": two, picked; CHOPPAH 1.5.3's)
        rate = hz or min(22050, int(pb.BANK_MAX_LEN // (nb * 240.0 / bpm)))   # (its own, or slow ones lower, as the page does)
        args = argparse.Namespace(bars=nb, slices=16, break_slices=16, rate=rate, transients=True, loudness=-12.0,
                                  name_prefix="")
        try:
            s = pb.prepare(f"{ROOT / 'breaks' / file}:{bpm}:{bars}:{name}" + (":grid" if grid else "") +
                           (f":loud={loud}" if loud is not None else "") + (f":hz={hz}" if hz else ""), args, file)
        except SystemExit as e:                     # (one that won't make: left out, said)
            print(f"break_lib: {file}: {e}", file=sys.stderr)
            continue
        img = pb.build_bank([s])
        bid = re.sub(r"[^a-z0-9]+", "-", Path(file).stem.lower()).strip("-")
        (out / f"{bid}.bin").write_bytes(img)
        folder = file.split("/")[0]
        index.append({"id": bid, "name": s["name"], "file": file, "folder": FOLDER.get(folder, folder.upper()),
                      "bpm": s["bpm"], "bars": nb, "size": len(img), "built": built})
        if file in full:                         # (the track's whole name, on the page only: the FM-1 keeps "name")
            index[-1]["title"] = full[file]
    (out / "index.json").write_text(json.dumps(index, separators=(",", ":")), encoding="utf-8")
    total = sum(e["size"] for e in index)
    print(f"break_lib: {len(index)} breaks, {total / 1024:.0f} KB, in {out}")


if __name__ == "__main__":
    if len(sys.argv) != 2:
        raise SystemExit(__doc__)
    main(sys.argv[1])
