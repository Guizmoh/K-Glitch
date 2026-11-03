#!/usr/bin/env python3
"""Découpe les GIF de nuit (art/nuit_agent1.gif, art/nuit_agent2.gif) en images PNG
pour l'animation de l'écran « Surveillance en cours »."""
import os
import numpy as np
from PIL import Image, ImageSequence
from scipy import ndimage

# Fond du cadre : noir bleuté (0,0,9) -> dégradé bleu nuit. Les contours du dessin
# sont noir neutre, ils ne sont pas touchés.
TOP, BOTTOM = np.array([38, 58, 128]), np.array([24, 38, 92])


def blue_background(im):
    a = np.asarray(im).astype(int)
    r, g, b, al = a[..., 0], a[..., 1], a[..., 2], a[..., 3]
    mask = (al > 200) & (np.maximum(r, g) <= 6) & (b >= 5) & (b <= 24)
    lab, n = ndimage.label(mask)
    # toutes les grandes zones de fond (le personnage peut couper le fond en plusieurs morceaux)
    sizes = ndimage.sum(np.ones_like(lab), lab, range(1, n + 1))
    keep = np.isin(lab, [i + 1 for i, sz in enumerate(sizes) if sz > 1500])
    keep = ndimage.binary_closing(keep, iterations=1) & mask | keep
    # bande sombre le long du bord intérieur du cadre
    inside = ndimage.distance_transform_edt(al > 30)
    ring = (inside < 30) & (al > 200) & (np.maximum(np.maximum(r, g), b) < 34) & (b >= np.maximum(r, g))
    keep |= ring
    h = a.shape[0]
    t = (np.arange(h) / max(1, h - 1))[:, None, None]
    grad = (TOP * (1 - t) + BOTTOM * t).astype(int)
    out = a.copy()
    out[..., :3] = np.where(keep[..., None], np.broadcast_to(grad, a[..., :3].shape), a[..., :3])
    return Image.fromarray(out.astype(np.uint8), "RGBA")

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT = os.path.join(ROOT, "app/src/main/res/drawable-nodpi")
for agent in (1, 2):
    gif = Image.open(os.path.join(ROOT, f"art/nuit_agent{agent}.gif"))
    frames = [f.convert("RGBA") for f in ImageSequence.Iterator(gif)]
    # cadre commun à toutes les images (zone non transparente de l'ensemble)
    box = None
    for f in frames:
        b = f.getchannel("A").point(lambda a: 255 if a > 10 else 0).getbbox()
        box = b if box is None else (min(box[0], b[0]), min(box[1], b[1]), max(box[2], b[2]), max(box[3], b[3]))
    for i, f in enumerate(frames):
        blue_background(f.crop(box)).save(os.path.join(OUT, f"night_agent{agent}_f{i}.png"), optimize=True)
    print(f"agent {agent}: {len(frames)} images, cadre {box}")
