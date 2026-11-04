#!/usr/bin/env python3
"""Icône du policier moustachu : la 3e tête (il rigole) de art/agent1_expressions.webp."""
import os
import numpy as np
from PIL import Image
from scipy import ndimage

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT = os.path.join(ROOT, "app/src/main/res/drawable-nodpi")
X0, X1 = 577, 855  # 3e tête dans la planche

rgb = np.asarray(Image.open(os.path.join(ROOT, "art/agent1_expressions.webp")).convert("RGB")).astype(int)
light = (rgb.min(axis=2) > 200) & (rgb.max(axis=2) - rgb.min(axis=2) < 18)
lab, _ = ndimage.label(light)
border = set(np.unique(np.concatenate([lab[0], lab[-1], lab[:, 0], lab[:, -1]]))) - {0}
bg = ndimage.binary_opening(np.isin(lab, list(border)), iterations=1)
alpha = ndimage.gaussian_filter(np.where(bg, 0, 255).astype(float), 0.6).clip(0, 255).astype(np.uint8)
xx = np.arange(rgb.shape[1])[None, :]
alpha = np.where((xx >= X0) & (xx < X1), alpha, 0)
l2, n = ndimage.label(alpha > 40)
sizes = ndimage.sum(np.ones_like(l2), l2, range(1, n + 1))
keep = ndimage.binary_fill_holes(l2 == int(np.argmax(sizes)) + 1)
alpha = np.where(keep, alpha, 0).astype(np.uint8)
rgba = np.dstack([rgb.astype(np.uint8), alpha])
ys, xs = np.nonzero(alpha > 10)
head = Image.fromarray(rgba[ys.min():ys.max() + 1, xs.min():xs.max() + 1], "RGBA")
head.save(os.path.join(OUT, "agent1_laugh.png"), optimize=True)
# premier plan de l'icône adaptative (tête dans la zone sûre)
fg = Image.new("RGBA", (432, 432), (0, 0, 0, 0))
h = 300; w = int(head.width * h / head.height)
fg.alpha_composite(head.resize((w, h), Image.LANCZOS), ((432 - w) // 2, 70))
fg.save(os.path.join(OUT, "ic_launcher_agent.png"), optimize=True)
print("icône du moustachu générée")
