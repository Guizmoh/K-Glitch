#!/usr/bin/env python3
"""Découpe les 6 expressions du gros agent (art/agent2_expressions.webp).

1 colère, 2 neutre, 3 sourire, 4 dort, 5 dort + petite bave, 6 dort + grosse bave.
Les « Zzz » dessinés sur 4-5-6 sont effacés (inpainting) : l'appli les anime elle-même.
"""
import os
import cv2
import numpy as np
from PIL import Image
from scipy import ndimage

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT = os.path.join(ROOT, "app/src/main/res/drawable-nodpi")
NAMES = ["agent2_angry", "agent2_neutral", "agent2_smile", "agent2_sleep1", "agent2_sleep2", "agent2_sleep3"]
# Bords gauche / droit de chaque tête dans la planche (pixels)
SPANS = [(18, 352), (352, 674), (674, 1000), (996, 1325), (1321, 1649), (1645, 1990)]

img = cv2.cvtColor(np.asarray(Image.open(os.path.join(ROOT, "art/agent2_expressions.webp")).convert("RGB")), cv2.COLOR_RGB2BGR)
H, W = img.shape[:2]
yy, xx = np.mgrid[0:H, 0:W]

# 1) Effacer les Zzz des têtes endormies : on recopie le même morceau de casquette
#    depuis la tête souriante (casquette identique), alignée par corrélation.
gray = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)
SMILE = SPANS[2]
for left, right in SPANS[3:]:
    tpl = gray[140:265, left + 20:left + 210]
    search = gray[100:320, SMILE[0] - 40:SMILE[1]]
    res = cv2.matchTemplate(search, tpl, cv2.TM_CCOEFF_NORMED)
    _, _, _, (mx, my) = cv2.minMaxLoc(res)
    dx = (SMILE[0] - 40 + mx) - (left + 20)
    dy = (100 + my) - 140
    y0, y1, x0, x1 = 100, 252, left + 205, min(right, W)
    img[y0:y1, x0:x1] = img[y0 + dy:y1 + dy, x0 + dx:x1 + dx]
    # Les Z qui dépassent à droite de la casquette : fond blanc
    zone = (xx >= x1 - 0) & (xx < right + 30) & (yy < 252)
    img[zone & (yy >= 100)] = 255
rgb = cv2.cvtColor(img, cv2.COLOR_BGR2RGB).astype(int)

# 2) Fond (blanc / damier) -> transparent
light = (rgb.min(axis=2) > 200) & (rgb.max(axis=2) - rgb.min(axis=2) < 18)
lab, _ = ndimage.label(light)
border = set(np.unique(np.concatenate([lab[0], lab[-1], lab[:, 0], lab[:, -1]]))) - {0}
bg = ndimage.binary_opening(np.isin(lab, list(border)), iterations=1)
alpha = ndimage.gaussian_filter(np.where(bg, 0, 255).astype(float), 0.6).clip(0, 255).astype(np.uint8)
rgba = np.dstack([rgb.astype(np.uint8), alpha])


def save(rgba_head, path, size=512):
    lab2, n = ndimage.label(rgba_head[..., 3] > 40)
    sizes = ndimage.sum(np.ones_like(lab2), lab2, range(1, n + 1))
    keep = ndimage.binary_fill_holes(lab2 == int(np.argmax(sizes)) + 1)
    rgba_head[..., 3] = np.where(keep, rgba_head[..., 3], 0)
    # Cadre commun à toutes les têtes (même taille, même position) pour que
    # l'animation ne « saute » pas : la bave qui dépasse en bas n'agrandit pas le cadre.
    al = rgba_head[..., 3] > 10
    ys, xs = np.nonzero(al[:505])
    side = 400
    x0 = int((xs.min() + xs.max()) / 2 - side / 2)
    y0 = ys.min() - 4
    sq = Image.fromarray(rgba_head, "RGBA").crop((x0, y0, x0 + side, y0 + side))
    sq.resize((size, size), Image.LANCZOS).save(path, optimize=True)


for name, (x0, x1) in zip(NAMES, SPANS):
    head = rgba.copy()
    head[..., 3] = np.where((xx >= x0) & (xx < x1), head[..., 3], 0)
    save(head, os.path.join(OUT, name + ".png"))

# Icône de l'appli pour ce personnage : la tête souriante
smile = Image.open(os.path.join(OUT, "agent2_smile.png"))
fg = Image.new("RGBA", (432, 432), (0, 0, 0, 0))
fg.alpha_composite(smile.resize((300, 300), Image.LANCZOS), (66, 70))
fg.save(os.path.join(OUT, "ic_launcher_agent2.png"), optimize=True)
print("expressions générées")
