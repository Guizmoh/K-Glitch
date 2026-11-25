#!/bin/sh
# LOAF BREAKER: the manual's and the site's screenshots, from the host simulator's scenarios (and the update loader's
# preview), copied into app/docs/img under the names the manual uses. Run after ./build.sh --host:
#   sh tools/site_shots.sh
set -e
cd "$(dirname "$0")/.."
H=app/build/host/loaf-breaker_host
OUT=app/build/scenarios
IMG=app/docs/img
[ -x "$H" ] || { echo "site_shots: build the host first (./build.sh --host)"; exit 1; }
mkdir -p "$OUT"
for sc in tests/scenarios/*.txt; do      # (all of them: the pictures come from many; one failing is no matter here)
    "$H" "$sc" "$OUT" >/dev/null 2>&1 || true
done
L=app/build/ldr_shots
mkdir -p "$L"
cc -std=c99 -I tests/loader/src -I tests/loader/boards/fm1 -I app/loader tests/loader/src/preview.c -o "$L/ldr_preview"
(cd "$L" && ./ldr_preview)
python3 - "$OUT" "$IMG" "$L" <<'PY'
import sys
from pathlib import Path
from PIL import Image
out, img, ldr = map(Path, sys.argv[1:4])
MAP = {  # the manual's name: the scenario's shot
    "screen-boot": "u0_splash", "screen-slicer": "site_cdj", "screen-slices": "site_slices", "screen-start": "u3_intro_full",
    "screen-selhelp": "site_selhelp", "screen-held": "site_held", "screen-keyloop": "u6_held",
    "screen-slicefx": "x_key_fx", "pad-roll8": "p_roll8", "pad-stop": "p_stop", "pad-dub": "p_dub",
    "screen-seq": "q_seq", "screen-step": "h_step", "screen-fxchain": "n_fx",
    "screen-fx-tape": "v_tape", "screen-fx-more": "n_fx2", "screen-mbcomp": "c_multiband",
    "screen-mbcomp-band": "c_mb_low", "screen-mbcomp-solo": "c_mb_solo", "screen-global": "n_glo",
    "screen-glomore": "n_glo3", "screen-lobotomized": "site_lobo", "screen-kick": "km_big",
    "screen-kick-classic": "km_classic", "screen-kick-synth": "km_synth", "screen-perc": "km_snare",
    "screen-mixer": "x_levels", "screen-master": "m_maxout", "c_cdj_gen": "c_cdj_gen", "t_tracks": "t_tracks",
    "t_play": "t_play", "t_build": "t_build", "screen-drum-big": "d_big_snare",
    "screen-klab-tail": "kl_tail", "screen-klab-stack": "kl_stack", "screen-kick-wave": "km_wave", "screen-tutorial": "tu_1",
    "screen-blackkeys": "a_blackkeys", "screen-split": "split_page",
    # (0.9.57: the menus of 0.9.45-0.9.56)
    "screen-arp": "a_thru_split", "screen-selblack": "sk_page",
    "screen-song-break": "t_break", "screen-drop-same": "t_dropsame", "screen-sends": "dm_sends",
    "screen-settings": "hset", "screen-reset": "rs_reset", "screen-fx-mix": "fx_mix", "screen-kick-tok": "kl2_tok",
    "screen-kick-gate": "kl2_gate", "screen-kick-fx": "k_fx1", "screen-kick-preset": "km_preset", "screen-lights": "lights_pulse",
}
miss = []
for dst, src in MAP.items():
    p = out / (src + ".png")
    if p.exists():
        Image.open(p).convert("RGB").save(img / (dst + ".png"), optimize=True)
    else:
        miss.append(src)
Image.open(ldr / "ldr_50.ppm").convert("RGB").save(img / "screen-updating.png", optimize=True)
print(f"site_shots: {len(MAP) + 1 - len(miss)} pictures in {img}" + (f"; missing: {' '.join(miss)}" if miss else ""))
if miss:
    sys.exit(1)
PY
