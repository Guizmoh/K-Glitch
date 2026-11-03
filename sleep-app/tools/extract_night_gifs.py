#!/usr/bin/env python3
"""Découpe les GIF de nuit (art/nuit_agent1.gif, art/nuit_agent2.gif) en images PNG
pour l'animation de l'écran « Surveillance en cours »."""
import os
from PIL import Image, ImageSequence

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
        f.crop(box).save(os.path.join(OUT, f"night_agent{agent}_f{i}.png"), optimize=True)
    print(f"agent {agent}: {len(frames)} images, cadre {box}")
