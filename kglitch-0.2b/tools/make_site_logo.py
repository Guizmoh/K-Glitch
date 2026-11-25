#!/usr/bin/env python3
# SPDX-License-Identifier: MPL-2.0
"""LOAF BREAKER: the site's tab icon, LOAF LORD's face without its lettering (art/loaf_lord.webp, the face cropped
square above the words), its contrast pushed so it reads at 16 px, on black:

  tools/make_site_logo.py        -> app/site/img/favicon.png (64 px), app/site/img/apple-touch-icon.png (180 px),
                                    app/site/img/app-192.png, app-512.png (the installable app's, 0.9.71: the face inside
                                    the middle 80 %, so a round or squircle mask keeps it whole)"""
from pathlib import Path
from PIL import Image, ImageOps

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "art" / "loaf_lord.webp"
OUT = ROOT / "app" / "site" / "img"
BOX = (420, 150, 1580, 1310)              # the face, tight and square, above the lettering (from row ~1430 of 2000)


def main():
    face = Image.open(SRC).convert("L").crop(BOX)
    face = ImageOps.autocontrast(face, cutoff=1).point(lambda v: min(255, int(v * 1.25)))
    OUT.mkdir(parents=True, exist_ok=True)
    for old in ("loaf-lord.png",):
        (OUT / old).unlink(missing_ok=True)
    face.resize((64, 64), Image.LANCZOS).save(OUT / "favicon.png", optimize=True)
    face.resize((180, 180), Image.LANCZOS).save(OUT / "apple-touch-icon.png", optimize=True)
    for n in (192, 512):                  # (the app's icons, maskable: a black ground, the face in its safe middle)
        icon = Image.new("L", (n, n), 0)
        m = int(n * 0.8)
        icon.paste(face.resize((m, m), Image.LANCZOS), ((n - m) // 2, (n - m) // 2))
        icon.save(OUT / f"app-{n}.png", optimize=True)
    print(f"make_site_logo: {OUT / 'favicon.png'}, {OUT / 'apple-touch-icon.png'}")


if __name__ == "__main__":
    main()
