#!/usr/bin/env python3
# SPDX-License-Identifier: MPL-2.0
"""LOAF BREAKER (0.9.30, FLEXOR): the start-up picture's face, for the VINYL BREAK record's label: a square of the
picture, 16 greys packed two to a byte (high nibble first), 0 outside the circle (the label's colour there).

  tools/make_face.py art/loaf_breaker.webp app/face_data.h
"""
import sys
from PIL import Image, ImageDraw, ImageOps

SIZE, CX, CY, R = 64, 640, 500, 300      # the label's picture; its square in the art (centre, half side)


def main():
    src, out = sys.argv[1], sys.argv[2]
    im = Image.open(src).convert("L").crop((CX - R, CY - R, CX + R, CY + R)).resize((SIZE, SIZE), Image.LANCZOS)
    im = ImageOps.autocontrast(im, cutoff=1)
    m = Image.new("L", (SIZE, SIZE), 0)
    ImageDraw.Draw(m).ellipse((0, 0, SIZE - 1, SIZE - 1), fill=255)
    px, mk = im.load(), m.load()
    data = []
    for y in range(SIZE):
        for x in range(0, SIZE, 2):
            v = [max(1, px[x + k, y] * 15 // 255) if mk[x + k, y] else 0 for k in (0, 1)]
            data.append(v[0] << 4 | v[1])
    with open(out, "w") as f:
        f.write("/* SPDX-License-Identifier: MPL-2.0 */\n")
        f.write("/* LOAF BREAKER: FACE, made by tools/make_face.py from loaf_breaker.webp: %d x %d, 16 greys, two pixels a byte\n"
                " * (0 outside the circle). Regenerate it with: tools/make_face.py art/loaf_breaker.webp app/face_data.h */\n" % (SIZE, SIZE))
        f.write("#pragma once\n#include <stdint.h>\n#define FACE_N %d\n" % SIZE)
        f.write("static const uint8_t FACE_PX[%d] = {\n" % len(data))
        for i in range(0, len(data), 16):
            f.write("    " + ", ".join("0x%02X" % b for b in data[i:i + 16]) + ",\n")
        f.write("};\n")


if __name__ == "__main__":
    main()
