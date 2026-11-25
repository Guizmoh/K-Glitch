#!/usr/bin/env python3
# SPDX-License-Identifier: MPL-2.0
"""LOAF BREAKER: what takes the room in the firmware, from the linked ELF's section headers and symbol table.

  tools/size_report.py SECTIONS.txt SYMBOLS.txt [IMAGE_BYTES]
    SECTIONS.txt: objdump -h loaf-breaker.elf    SYMBOLS.txt: objdump -t loaf-breaker.elf

Prints the sections, the symbols by family (the break bank, pictures, fonts, the engine, the screens, the platform,
Bluetooth...) and the biggest symbols. Symbol sizes are what the linker placed: code and constant data in flash; the
RAM images (.data, .ram_text) are stored LZMA-packed in the package (boot_pack), so they cost the flash less than
their size here.
"""
import re
import sys
from collections import defaultdict

FAMILIES = [   # (family, regex on the symbol's name), first match wins
    ("breaks (the bank)", r"^CH_BANK$|bank_data"),
    ("pictures (splash, title, LOAF LORD)", r"^(SPLASH|TITLE|EGG|LOGO)_"),
    ("fonts", r"(?i)font|glyph|barlow"),
    ("words (the screens' text tables)", r"(?i)^words?_|_words$|^WORD"),
    ("Bluetooth (MIDI, audio)", r"(?i)ble|bt_|bta|a2dp|sbc|l2cap|att_|gatt|smp_|hci|rfcomm|avdtp|sdp"),
    ("break engine (slices, pads, generator)", r"^(brk|voice|grain|hermite|on_step|roll|gen_|pad_?set|key_?set|slx)"),
    ("FX chain (filter, reverb, delay, MB comp...)", r"^(brk_fx|fx_|dl_|svf|eq_|mb_|verb|rev_|comb|allpass|knee|tape|clip|flang|ring|trem|punch|crunch|drive)"),
    ("drums (kick, snare, hat)", r"^(kick|perc|kv_|kc_|layer_fx|drum)"),
    ("overlays (ARP's 16)", r"^ovl"),
    ("break decoder (ADPCM)", r"(?i)adpcm"),
    ("maths (exp2, sin...)", r"^brk_(exp2|log2|sin|tanh|pow|math|clamp)|exp2f|log2f|sinf|cosf|powf|sqrtf"),
    ("screens and controls (UI)", r"^(draw_|ui$|ui_|cv_|seq_|lane_|sl_|pat_|global_|mix_|tap|tp_|intro|title|mark|wave|"
                                  r"skin|egg|grey|logo|splash|app_|say|text_|int_text|tenths|knob|page|tab|lobo|meter|"
                                  r"version|keys_|pads?_|sub_drive|ovl_pad|kick_(bar|knob|part)|perc_knob|reset|break_|"
                                  r"settings|changed|set$|pats|undo|rec_|midi_|ext|clock|transport|leds|input|frame)"),
    ("platform: screen and drawing", r"(?i)lcd|gfx|panel|spi|dma|blit|canvas|cv_|pal$|palette"),
    ("platform: USB (MIDI, audio, install)", r"(?i)usb|uac|ota|hid|uart|midi"),
    ("platform: storage and flash", r"(?i)store|flash|sfc|norflash|spiflash|xip|crc"),
    ("platform: audio and timers", r"(?i)audio|i2s|dac|adc|codec|timer|tick|irq|isr"),
    ("platform: boot, crash, system", r"(?i)boot|crash|sys_|safe|watchdog|stack|panic|reset|start|main|os_|task|lzma"),
    ("C library", r"^(mem|str|__|_?[a-z]*printf|abs|div|mod)"),
]


def sections(path):
    out = []
    for ln in open(path, errors="ignore"):
        m = re.match(r"\s*\d+\s+(\S+)\s+([0-9a-fA-F]+)\s+([0-9a-fA-F]+)\s+([0-9a-fA-F]+)", ln)
        if m:
            out.append((m.group(1), int(m.group(2), 16), int(m.group(3), 16)))
    return out


def symbols(path):
    out = []
    for ln in open(path, errors="ignore"):
        m = re.match(r"^([0-9a-fA-F]+)\s.{7}\s*(\S+)\s+([0-9a-fA-F]+)\s+(.+)$", ln.rstrip("\n"))
        if not m or m.group(2) in ("*ABS*", "*UND*", "*COM*"):
            continue
        size = int(m.group(3), 16)
        if size:
            out.append((m.group(4).split()[-1], m.group(2), size))
    return out


def family(name):
    base = re.sub(r"\.(lto_priv|constprop|isra|part|cold)\.\d+.*$", "", name)
    for fam, rx in FAMILIES:
        if re.search(rx, base):
            return fam
    return "other"


def main():
    secs, syms = sections(sys.argv[1]), symbols(sys.argv[2])
    image = int(sys.argv[3]) if len(sys.argv) > 3 else 0
    print("SECTIONS (name, size, address)")
    for n, sz, vma in secs:
        print(f"  {n:24} {sz:9,d} B  {vma:#010x}")
    seen = set()
    syms = [s for s in syms if not (s[0], s[2]) in seen and not seen.add((s[0], s[2]))]
    fams = defaultdict(lambda: [0, 0])
    for n, sec, sz in syms:
        f = fams[family(n)]
        f[0] += sz
        f[1] += 1
    total = sum(v[0] for v in fams.values())
    print(f"\nFAMILIES (symbols' sizes, {total:,d} B in all" + (f"; the image is {image:,d} B" if image else "") + ")")
    for fam, (sz, n) in sorted(fams.items(), key=lambda kv: -kv[1][0]):
        print(f"  {sz:9,d} B  {100 * sz / total:5.1f} %  {n:5d} symbols  {fam}")
    print("\nTHE 70 BIGGEST SYMBOLS")
    for n, sec, sz in sorted(syms, key=lambda s: -s[2])[:70]:
        print(f"  {sz:9,d} B  {sec:12} {family(n):42} {n}")
    print("\nTHE BIGGEST 'other' (to sort into families)")
    for n, sec, sz in sorted((s for s in syms if family(s[0]) == "other"), key=lambda s: -s[2])[:40]:
        print(f"  {sz:9,d} B  {sec:12} {n}")


if __name__ == "__main__":
    main()
