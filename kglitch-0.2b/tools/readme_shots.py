#!/usr/bin/env python3
# LOAF BREAKER: the README's pictures (docs/img/screens.png, pads.png, kick.png, live.png, menus.png), each a grid of
# the host simulator's screens. Run after ./build.sh --host and sh tools/site_shots.sh (which runs every scenario into
# app/build/scenarios):
#   python3 tools/readme_shots.py
import sys
from pathlib import Path
from PIL import Image

ROOT = Path(__file__).resolve().parent.parent
SHOTS = ROOT / "app/build/scenarios"
OUT = ROOT / "docs/img"
BG, TILE, GAP = (40, 40, 46), 240, 10

GRIDS = {  # the picture: (columns, the scenarios' shots in order)
    "screens": (3, "u0_splash u3_intro_full site_cdj site_slices site_held km_big"),
    "pads": (4, "p_roll8 p_gate a_freeze p_filter a_flt p_arp tri_on a_blackkeys"),
    "kick": (3, "km_preset kl2_tok km_dirt kl_tail km_snare k_fx1"),
    "live": (3, "t_tracks t_play t_build h_step m_maxout q_seq"),
    "menus": (4, "a_thru_split sk_page tailsplit wkbreak_len t_break t_dropsame dm_sends fx_mix hset rs_reset kl2_gate lights_pulse"),
}

miss = []
for name, (cols, shots) in GRIDS.items():
    shots = shots.split()
    rows = (len(shots) + cols - 1) // cols
    pic = Image.new("RGB", (GAP + cols * (TILE + GAP), GAP + rows * (TILE + GAP)), BG)
    for i, s in enumerate(shots):
        p = SHOTS / (s + ".png")
        if not p.exists():
            miss.append(s)
            continue
        pic.paste(Image.open(p).convert("RGB").resize((TILE, TILE)),
                  (GAP + (i % cols) * (TILE + GAP), GAP + (i // cols) * (TILE + GAP)))
    pic.save(OUT / (name + ".png"), optimize=True)
print(f"readme_shots: {len(GRIDS)} pictures in {OUT}" + (f"; missing: {' '.join(miss)}" if miss else ""))
sys.exit(1 if miss else 0)
