#!/usr/bin/env python3
# SPDX-License-Identifier: MPL-2.0
"""GLOUPI BREAKS' grey pictures (LOAF LORD's egg, LOAF BREAKER's splash and title): a picture -> a C header.

  tools/make_egg.py art/loaf_lord.webp app/egg_data.h                       # the egg: 240 x 240, centred
  tools/make_egg.py art/loaf_breaker.webp app/splash_data.h --name SPLASH --trim --fit 240x226
  tools/make_egg.py art/loaf_breaker.webp app/title_data.h --name TITLE --rows 890:1254 --trim --fit 232x42

--rows A:B keeps the source's rows A..B-1 (before anything else); --trim cuts the transparent margins (alpha over 40);
--fit WxH scales it to fit W x H, aspect kept, and the picture is that size (without --fit: centred in SIZE x SIZE).
Transparent parts are --bg's grey (black at first). Greys reduced to LEVELS (2..16, black to white, dithered), run-length coded a byte a
run: bits 0-3 the grey, bits 4-7 the run's length less one (1..16 pixels), rows one after another, no run crossing a
row. The header defines NAME_W, NAME_H, NAME_PAL[], NAME_RLE[]. Needs Pillow.
"""
import argparse
from pathlib import Path

from PIL import Image


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("src", type=Path)
    ap.add_argument("header", type=Path)
    ap.add_argument("--name", default="EGG", help="the C names' prefix (default EGG)")
    ap.add_argument("--size", type=int, default=240, help="without --fit: the square it is centred in")
    ap.add_argument("--fit", help="WxH: scaled to fit, the picture that size")
    ap.add_argument("--rows", help="A:B: only the source's rows A..B-1")
    ap.add_argument("--trim", action="store_true", help="cut the transparent margins")
    ap.add_argument("--levels", type=int, default=16)
    ap.add_argument("--bg", type=int, default=0, help="the grey under its transparent parts and in the margins (0 black)")
    ap.add_argument("--preview", type=Path)
    a = ap.parse_args()
    n = max(2, min(16, a.levels))
    src = Image.open(a.src).convert("RGBA")
    if a.rows:
        r0, r1 = (int(v) for v in a.rows.split(":"))
        src = src.crop((0, r0, src.width, min(r1, src.height)))
    if a.trim:
        box = src.split()[3].point(lambda v: 255 if v > 40 else 0).getbbox()
        if box:
            src = src.crop(box)
    if a.fit:
        fw, fh = (int(v) for v in a.fit.lower().split("x"))
        k = min(fw / src.width, fh / src.height)
        src = src.resize((max(1, round(src.width * k)), max(1, round(src.height * k))), Image.LANCZOS)
        w, h = src.width, src.height
        img = Image.new("RGBA", (w, h), (a.bg, a.bg, a.bg, 255))
        img.alpha_composite(src)
    else:
        src.thumbnail((a.size, a.size), Image.LANCZOS)
        w = h = a.size
        img = Image.new("RGBA", (w, h), (a.bg, a.bg, a.bg, 255))   # its transparent parts, and the margins
        img.alpha_composite(src, ((w - src.width) // 2, (h - src.height) // 2))
    img = img.convert("L")
    pal = Image.new("P", (1, 1))
    greys = [round(i * 255 / (n - 1)) for i in range(n)]
    pal.putpalette([v for g in greys for v in (g, g, g)] + [0, 0, 0] * (256 - n))
    q = img.convert("RGB").quantize(palette=pal, dither=Image.Dither.FLOYDSTEINBERG)
    q = q.point(lambda i: i if i < n else 0)
    px = q.load()
    data = bytearray()
    for y in range(h):
        x = 0
        while x < w:
            c, r = px[x, y], 1
            while x + r < w and px[x + r, y] == c and r < 16:
                r += 1
            data.append((r - 1) << 4 | c)
            x += r
    N = a.name.upper()
    pal565 = ", ".join(f"0x{((g >> 3) << 11) | ((g >> 2) << 5) | (g >> 3):04X}u" for g in greys)
    body = ",".join(("\n    " if i % 24 == 0 else " ") + f"0x{b:02X}" for i, b in enumerate(data))
    a.header.write_text(
        "/* SPDX-License-Identifier: MPL-2.0 */\n"
        f"/* GLOUPI BREAKS: {N}, made by tools/make_egg.py from {a.src.name}: {w} x {h}, {n} greys, "
        f"{len(data)} bytes. Regenerate it, don't edit it. */\n"
        "#pragma once\n#include <stdint.h>\n"
        f"#define {N}_W {w}\n#define {N}_H {h}\n"
        f"static const uint16_t {N}_PAL[{n}] = {{{pal565}}};\n"
        f"static const uint8_t {N}_RLE[{len(data)}] = {{{body}\n}};\n")
    if a.preview:
        prev = Image.new("L", (w, h))
        prev.putdata([greys[px[x, y]] for y in range(h) for x in range(w)])
        prev.save(a.preview)
    print(f"{a.header}: {N} {w} x {h}, {n} greys, {len(data)} bytes")


if __name__ == "__main__":
    main()
