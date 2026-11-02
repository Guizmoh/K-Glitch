#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""K-GLITCH in a browser: the firmware's UI and sound (firmware/host/emulator.c) built for wasm32 with clang and
wasm-ld (no Emscripten, no libc: tools/wasm_shim), put into web/emulator.html as one page that works offline.

python3 tools/build_emulator.py   -> build/emu/k-glitch.wasm, ../emulator/index.html
Needs Python 3 with Pillow (the generated headers), clang with the wasm32 target and wasm-ld (lld)."""
import base64
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
GEN, OUT = ROOT / "build/gen", ROOT / "build/emu"
EXPORTS = ["init", "home", "screen", "screen_gen", "audio", "rate", "block", "render", "midi_buf", "midi_take", "frame",
           "button", "key", "keys", "enc", "master", "playing", "bank", "midi_in", "state_get", "state_size", "state_sum",
           "state_buf", "state_put"]


def main():
    GEN.mkdir(parents=True, exist_ok=True)
    OUT.mkdir(parents=True, exist_ok=True)
    for script, header in [("gen_tables.py", "felucca_tables.h"), ("gen_samples.py", "felucca_samples.h"),
                           ("gen_drumkits.py", "felucca_drumkits.h"), ("gen_fm6_patches.py", "felucca_fm6.h"),
                           ("gen_font.py", "felucca_font.h"), ("gen_icons.py", "felucca_icons.h"),
                           ("gen_logo.py", "sloop_logo.h")]:
        if not (GEN / header).exists():
            subprocess.run([sys.executable, str(ROOT / "tools" / script), str(GEN / header)], cwd=ROOT, check=True)
    mem = OUT / "mem.o"
    subprocess.run(["clang", "--target=wasm32", "-O2", "-fno-builtin", "-ffreestanding", "-c",
                    str(ROOT / "tools/wasm_shim/mem.c"), "-o", str(mem)], cwd=ROOT, check=True)
    wasm = OUT / "k-glitch.wasm"
    subprocess.run(["clang", "--target=wasm32", "-O2", "-fwrapv", "-fno-strict-aliasing", "-ffreestanding", "-nostdlib",
                    "-w", "-Itools/wasm_shim", "-Ibuild/gen", "-Ifirmware/src", "-Ifirmware/hal",
                    "firmware/host/emulator.c", str(mem), "-Wl,--no-entry", "-Wl,--export-memory",
                    "-Wl,--initial-memory=33554432", "-Wl,-z,stack-size=262144"]
                   + ["-Wl,--export=emu_" + n for n in EXPORTS] + ["-o", str(wasm)], cwd=ROOT, check=True)
    page = (ROOT / "web/emulator.html").read_text().replace("__KG_WASM__", base64.b64encode(wasm.read_bytes()).decode())
    dest = ROOT.parent / "emulator/index.html"
    dest.parent.mkdir(exist_ok=True)
    dest.write_text(page)
    print("wasm", wasm.stat().st_size, "B; page", len(page), "B ->", dest)


if __name__ == "__main__":
    main()
