#!/usr/bin/env python3
"""Découpe les têtes de l'agent dans art/agents.webp et prépare l'affiche.

- tête de gauche (yeux ouverts)  -> res/drawable-nodpi/agent_awake.png
- deuxième tête (endormie), sans ses « Zzz » dessinés (ils sont animés dans l'app)
                                 -> res/drawable-nodpi/agent_asleep.png
- art/affiche.webp               -> res/drawable-nodpi/splash_poster.webp
"""
import os
import numpy as np
from PIL import Image
from scipy import ndimage

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT = os.path.join(ROOT, "app/src/main/res/drawable-nodpi")


def remove_background(img):
    a = np.asarray(img.convert("RGB")).astype(int)
    light = (a.min(axis=2) > 200) & (a.max(axis=2) - a.min(axis=2) < 18)  # blanc / damier gris
    lab, _ = ndimage.label(light)
    border = set(np.unique(np.concatenate([lab[0], lab[-1], lab[:, 0], lab[:, -1]]))) - {0}
    bg = np.isin(lab, list(border))
    bg = ndimage.binary_opening(bg, iterations=1)
    alpha = np.where(bg, 0, 255).astype(np.uint8)
    alpha = ndimage.gaussian_filter(alpha.astype(float), 0.6).clip(0, 255).astype(np.uint8)
    rgba = np.dstack([a.astype(np.uint8), alpha])
    return rgba


def keep_largest(rgba):
    lab, n = ndimage.label(rgba[..., 3] > 40)
    if n == 0:
        return rgba
    sizes = ndimage.sum(np.ones_like(lab), lab, range(1, n + 1))
    keep = lab == (int(np.argmax(sizes)) + 1)
    keep = ndimage.binary_fill_holes(keep)
    rgba[..., 3] = np.where(keep, rgba[..., 3], 0)
    return rgba


def save_square(rgba, path, size=512):
    ys, xs = np.nonzero(rgba[..., 3] > 10)
    crop = rgba[ys.min():ys.max() + 1, xs.min():xs.max() + 1]
    im = Image.fromarray(crop, "RGBA")
    side = max(im.size)
    sq = Image.new("RGBA", (side, side), (0, 0, 0, 0))
    sq.paste(im, ((side - im.width) // 2, (side - im.height) // 2))
    sq.resize((size, size), Image.LANCZOS).save(path, optimize=True)


src = Image.open(os.path.join(ROOT, "art/agents.webp"))
rgba = remove_background(src)
H, W = rgba.shape[:2]
yy, xx = np.mgrid[0:H, 0:W]

# Tête 1 : coupe verticale entre les deux têtes (casquette plus large en haut)
awake = rgba.copy()
cut1 = np.where(yy < 290, 288, 256)
awake[..., 3] = np.where(xx < cut1, awake[..., 3], 0)
save_square(keep_largest(awake), os.path.join(OUT, "agent_awake.png"))

# Tête 2 : entre la tête 1 et la tête 3, on efface la zone des « Zzz »
asleep = rgba.copy()
left = np.where(yy < 310, 288, 262)
z_zone = (xx > 412) & (yy < 215 + (xx - 412) * 0.6)
asleep[..., 3] = np.where((xx >= left) & (xx < 518) & ~z_zone, asleep[..., 3], 0)
save_square(keep_largest(asleep), os.path.join(OUT, "agent_asleep.png"))

Image.open(os.path.join(ROOT, "art/affiche.webp")).convert("RGB").save(
    os.path.join(OUT, "splash_poster.webp"), "WEBP", quality=82)
print("images générées")

# Icône de l'appli (premier plan adaptatif 432 px, tête dans la zone sûre)
head = Image.open(os.path.join(OUT, "agent_awake.png"))
fg = Image.new("RGBA", (432, 432), (0, 0, 0, 0))
h = head.resize((300, 300), Image.LANCZOS)
fg.alpha_composite(h, (66, 70))
fg.save(os.path.join(OUT, "ic_launcher_agent.png"), optimize=True)
# (icône du gros agent : voir tools/extract_agent2.py)
