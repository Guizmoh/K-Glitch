#!/usr/bin/env python3
# SPDX-License-Identifier: MPL-2.0
"""Prepare breaks for CHOPPAH (PRD A-61): WAV in -> whole bars -> slices -> resample -> IMA ADPCM -> a bank image.

    tools/prep_breaks.py amen.wav:136.9:4 think.wav --bars 2 -o build/bank.bin
    tools/prep_breaks.py --dry-run loops/*.wav           # what fits, without writing anything
    tools/prep_breaks.py --header app/bank_data.h ...    # the bank as a C array, built into the firmware
    tools/prep_breaks.py --list breaks/breaks.txt --header app/bank_data.h   # what ./build.sh does

Each input is FILE[:BPM[:BARS[:NAME]]]. Without a BPM, the tempo comes from the length and --bars (the file is taken to be
whole bars). Slices are an equal grid (--slices 16 a bar by default, at most 32 a break) or, with --transients,
the grid's markers moved to the nearest drum hit. Python 3 standard library only.

--list reads a catalogue (breaks/breaks.txt): one WAV a line, "IN FILE BPM BARS OF NAME", FILE relative to the
catalogue; the lines whose IN is * become the bank, sorted by BPM (0.9.43: the slowest first; the same BPM, in order). Paths in the manifest stay relative, so nothing generated
points outside the repo.

The bank layout is app/dsp/brk_bank.h's. Uploading it to the FM-1 arrives in milestone M5 (tools/fm1_breaks.py).
"""
import argparse
import array
import json
import math
import re
import struct
import sys
import wave
import zlib
from pathlib import Path

BANK_MAGIC = 0x31424843          # "CHB1"
BANK_VERSION = 1
BANK_SLOTS = 16
BANK_SLICES = 32
SLOT_FMT = "<12sIIIIIBBH33I"      # brk_bank.h bank_slot_t
HDR_FMT = "<IHHII"
# flash for breaks (PRD 8.2): 0x97000..0xD9FFF and PLAT_XDATA1 0xE5000..0xFBFFF, less a sector of headroom
BANK_CAPACITY = (0xDA000 - 0x97000) + (0xFC000 - 0xE5000) - 0x1000
# a bank built into the firmware (--header) shares fm1-base's app slot (581,564 B) with the code (~155 KB with
# Bluetooth MIDI): 400 KB keeps ~20 KB for the code to grow. The build checks the image against the slot as well.
IMAGE_CAPACITY = 0x64000
# a break decodes into the firmware's PCM buffer (app.h CH_PCM_MAX): no break longer than that
BANK_MAX_LEN = 106496

STEP = [7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31, 34, 37, 41, 45, 50, 55, 60, 66, 73, 80, 88, 97,
        107, 118, 130, 143, 157, 173, 190, 209, 230, 253, 279, 307, 337, 371, 408, 449, 494, 544, 598, 658, 724, 796,
        876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428,
        4871, 5358, 5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899, 15289, 16818, 18500, 20350,
        22385, 24623, 27086, 29794, 32767]
ADJ = [-1, -1, -1, -1, 2, 4, 6, 8]


def read_wav(path):
    """mono floats -1..1 and the rate. RIFF read by hand: integer PCM (8, 16, 24, 32 bit), IEEE float (32, 64 bit) and
    WAVE_FORMAT_EXTENSIBLE holding either (the wave module reads integer PCM only)"""
    data = Path(path).read_bytes()
    if data[0:4] != b"RIFF" or data[8:12] != b"WAVE":
        raise SystemExit(f"{path}: not a RIFF WAVE file")
    fmt = raw = None
    i = 12
    while i + 8 <= len(data):
        cid, size = data[i:i + 4], struct.unpack_from("<I", data, i + 4)[0]
        body = data[i + 8:i + 8 + size]
        if cid == b"fmt ":
            fmt = body
        elif cid == b"data":
            raw = body
        i += 8 + size + (size & 1)
    if fmt is None or raw is None:
        raise SystemExit(f"{path}: no fmt or data chunk")
    tag, ch, rate = struct.unpack_from("<HHI", fmt, 0)
    bits = struct.unpack_from("<H", fmt, 14)[0]
    if tag == 0xFFFE and len(fmt) >= 26:     # extensible: the real format is the sub-format GUID's first two bytes
        tag = struct.unpack_from("<H", fmt, 24)[0]
    width = bits // 8
    raw = raw[:len(raw) - len(raw) % (width * ch)]
    if tag == 3 and width == 4:
        a, scale = array.array("f", raw), 1.0
    elif tag == 3 and width == 8:
        a, scale = array.array("d", raw), 1.0
    elif tag == 1 and width == 2:
        a, scale = array.array("h", raw), 32768.0
    elif tag == 1 and width == 3:
        a = [int.from_bytes(raw[k:k + 3], "little", signed=True) for k in range(0, len(raw), 3)]
        scale = 8388608.0
    elif tag == 1 and width == 4:
        a, scale = array.array("i", raw), 2147483648.0
    elif tag == 1 and width == 1:
        a, scale = [b - 128 for b in raw], 128.0
    else:
        raise SystemExit(f"{path}: WAV format {tag}, {bits}-bit, not supported")
    if sys.byteorder == "big" and isinstance(a, array.array):
        a.byteswap()
    mono = [max(-1.0, min(1.0, sum(a[k:k + ch]) / (ch * scale))) for k in range(0, len(a) - ch + 1, ch)]
    return mono, rate


def resample(x, src, dst):
    """windowed-sinc (8 zero crossings each side), with a low-pass at the lower Nyquist"""
    if src == dst:
        return list(x)
    ratio = dst / src
    cut = min(1.0, ratio)
    n_out = int(len(x) * ratio)
    half = 8
    out = []
    for i in range(n_out):
        t = i / ratio
        c = int(t)
        acc = 0.0
        norm = 0.0
        for k in range(c - int(half / cut), c + int(half / cut) + 1):
            if 0 <= k < len(x):
                d = (t - k) * cut
                s = 1.0 if d == 0 else math.sin(math.pi * d) / (math.pi * d)
                win = 0.5 + 0.5 * math.cos(math.pi * d / half) if abs(d) < half else 0.0
                acc += x[k] * s * win
                norm += s * win
        out.append(acc / norm if norm else 0.0)
    return out


def onsets(x, rate):
    """an energy-flux curve, one value per 256 samples"""
    hop = 256
    env = []
    prev = 0.0
    for i in range(0, len(x) - hop, hop):
        e = sum(v * v for v in x[i:i + hop])
        env.append(max(0.0, e - prev))
        prev = e
    return env, hop


def slice_points(x, rate, nslices, transients):
    n = len(x)
    pts = [n * i // nslices for i in range(nslices + 1)]
    if transients:
        env, hop = onsets(x, rate)
        reach = n // nslices // 3            # a marker moves at most a third of a slice
        for i in range(1, nslices):
            lo, hi = max(0, (pts[i] - reach) // hop), min(len(env) - 1, (pts[i] + reach) // hop)
            if hi > lo:
                best = max(range(lo, hi + 1), key=lambda j: env[j])
                pts[i] = best * hop
        pts = sorted(set(pts))
        if len(pts) != nslices + 1:          # two markers met: back to the grid
            pts = [n * i // nslices for i in range(nslices + 1)]
    return pts


def adpcm_encode(x):
    pred, idx = 0, 0
    out = bytearray((len(x) + 1) // 2)
    for i, v in enumerate(x):
        s = max(-32768, min(32767, int(round(v * 32767.0))))
        d, step, code = s - pred, STEP[idx], 0
        diff = step >> 3
        if d < 0:
            code, d = 8, -d
        if d >= step:
            code |= 4; d -= step; diff += step
        if d >= step >> 1:
            code |= 2; d -= step >> 1; diff += step >> 1
        if d >= step >> 2:
            code |= 1; diff += step >> 2
        pred = max(-32768, min(32767, pred - diff if code & 8 else pred + diff))
        idx = max(0, min(88, idx + ADJ[code & 7]))
        out[i >> 1] |= code << (4 * (i & 1))
    return bytes(out)


def adpcm_decode(data, n):
    pred, idx, out = 0, 0, []
    for i in range(n):
        code = (data[i >> 1] >> (4 * (i & 1))) & 15
        step = STEP[idx]
        diff = step >> 3
        if code & 4: diff += step
        if code & 2: diff += step >> 1
        if code & 1: diff += step >> 2
        pred = max(-32768, min(32767, pred - diff if code & 8 else pred + diff))
        idx = max(0, min(88, idx + ADJ[code & 7]))
        out.append(pred)
    return out


def _biquad(x, b0, b1, b2, a1, a2):
    out, x1, x2, y1, y2 = [0.0] * len(x), 0.0, 0.0, 0.0, 0.0
    for i, v in enumerate(x):
        o = b0 * v + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2
        x2, x1, y2, y1 = x1, v, y1, o
        out[i] = o
    return out


def k_weight(x, rate):
    """ITU-R BS.1770's K-weighting at any rate (the RBJ forms of its two stages): a high shelf of +4 dB above ~1.7 kHz
    (the head's), then a high-pass at ~38 Hz: what the ear hears of a level"""
    A, w = 10.0 ** (3.99984385397 / 40.0), 2.0 * math.pi * 1681.97445095 / rate
    c, al = math.cos(w), math.sin(w) / (2.0 * 0.7071752369554193)
    a0 = (A + 1) - (A - 1) * c + 2 * math.sqrt(A) * al
    y = _biquad(x, A * ((A + 1) + (A - 1) * c + 2 * math.sqrt(A) * al) / a0, -2 * A * ((A - 1) + (A + 1) * c) / a0,
                A * ((A + 1) + (A - 1) * c - 2 * math.sqrt(A) * al) / a0, 2 * ((A - 1) - (A + 1) * c) / a0,
                ((A + 1) - (A - 1) * c - 2 * math.sqrt(A) * al) / a0)
    w = 2.0 * math.pi * 38.13547087613982 / rate
    c, al = math.cos(w), math.sin(w) / (2.0 * 0.5003270373253953)
    a0 = 1 + al
    return _biquad(y, (1 + c) / 2 / a0, -(1 + c) / a0, (1 + c) / 2 / a0, -2 * c / a0, (1 - al) / a0)


def loudness_db(x, rate):
    """the break's level as it is heard (LOAF BREAKER 0.9.42, FLEXOR: "tout mieux égaliser"; the RMS before): K-weighted
    (k_weight), 400 ms blocks every 100 ms, those under -70 and then 10 dB under the mean left out (BS.1770's gates),
    in LUFS"""
    y, n, hop = k_weight(x, rate), max(1, int(rate * 0.4)), max(1, int(rate * 0.1))
    z = []
    for i in range(0, max(1, len(y) - n + 1), hop):
        seg = y[i:i + n]
        z.append(sum(v * v for v in seg) / len(seg))
    lk = lambda e: -0.691 + 10.0 * math.log10(max(1e-12, e))
    z = [e for e in z if lk(e) > -70.0] or z
    rel = lk(sum(z) / len(z)) - 10.0
    z = [e for e in z if lk(e) > rel] or z
    return lk(sum(z) / len(z))


def limit(y, rate, ceiling):
    """a look-ahead limiter (2 ms ahead, 40 ms release, no clipping): -> the samples, its most gain off (a factor)"""
    n, la = len(y), max(1, int(rate * 0.002))
    req = [min(1.0, ceiling / abs(v)) if v else 1.0 for v in y]
    ahead, q, h = [1.0] * n, [], 0              # the least gain any sample in the next la wants (a sliding minimum)
    for i in range(n - 1, -1, -1):
        while len(q) > h and req[q[-1]] >= req[i]:
            q.pop()
        q.append(i)
        while q[h] > i + la - 1:
            h += 1
        ahead[i] = req[q[h]]
    rel = 1.0 - math.exp(-1.0 / (rate * 0.04))
    env, held = 1.0, [1.0] * n                  # down at once (it looks ahead), back up slowly
    for i in range(n):
        env = ahead[i] if ahead[i] < env else env + (1.0 - env) * rel
        held[i] = env
    out, acc, most = [0.0] * n, float(la), 1.0   # a la-long average: the gain glides down onto each peak (as if
    for i in range(n):                           # la samples at 1 came before)
        acc += held[i] - (held[i - la] if i >= la else 1.0)
        gi = min(acc / la, req[i])               # (never over the ceiling)
        most = min(most, gi)
        out[i] = y[i] * gi
    return out, most


def level(x, rate, target_db, ceiling=0.98):
    """brings the break to target_db (loudness_db) and holds its peaks under the ceiling (limit): every break as loud
    as the others, whatever its few loudest hits. The limiter takes some level too, so the gain is found again until
    the result is within 0.2 dB. -> the samples, the gain (dB) and the most the limiter took off (dB)"""
    g = 10.0 ** ((target_db - loudness_db(x, rate)) / 20.0)
    for _ in range(5):
        out, most = limit([v * g for v in x], rate, ceiling)
        err = target_db - loudness_db(out, rate)
        if abs(err) < 0.2:
            break
        g *= 10.0 ** (err / 20.0)
    return out, 20.0 * math.log10(g), -20.0 * math.log10(most)


def name_opts(field):
    """a catalogue line's NAME and its options after it, in any order: @X its own loudness, NNNNNHz its own sample
    rate (LOAF BREAKER 0.9.37, FLEXOR: THINK at 16 kHz for room): "THINK 114 16000Hz" -> ("THINK 114", None, 16000)"""
    name, loud, hz = field.strip(), None, None
    while True:
        m = re.match(r"^(.*?)\s+(@[+-]?\d+(?:\.\d+)?|\d{4,5}Hz)$", name)
        if not m:
            return name, loud, hz
        name, opt = m.group(1).strip(), m.group(2)
        if opt.startswith("@"):
            loud = float(opt[1:])
        else:
            hz = int(opt[:-2])


def name_loud(field):
    """a catalogue line's NAME and its own loudness: "AKIRA 190 @0" -> ("AKIRA 190", 0.0); none -> (NAME, None)"""
    name, loud, _ = name_opts(field)
    return name, loud


FOLDER_LOUD = {"flexor": -12.0}         # (LOAF BREAKER, 14 Nov 2026, FLEXOR: his breaks, from his DJ library, all
                                         # mastered alike: every one at -12 LUFS unless its line says @X)


def line_loud(file, loud):
    """a catalogue line's level: its own @X, or its folder's (flexor/: -12), or None (the default, -12 since 0.9.70)"""
    return loud if loud is not None else FOLDER_LOUD.get(file.split("/")[0])


def read_list(path, every=False):
    """the catalogue's lines marked *, as specs FILE:BPM:BARS:NAME (FILE resolved) and the FILE as written"""
    specs = []
    for n, line in enumerate(Path(path).read_text().splitlines(), 1):
        if not line.strip() or line.lstrip().startswith("#"):
            continue
        f = line.split(None, 5)
        if len(f) < 6 or f[0] not in ("*", "-", "*=", "-="):
            raise SystemExit(f"{path}:{n}: expected IN FILE BPM BARS OF NAME, IN * or - (*= or -=: slices on an even grid)")
        if f[0].startswith("*") or every:   # (--all, LOAF BREAKER's VST: the library's lines too)
            name, loud, hz = name_opts(f[5])
            loud = line_loud(f[1], loud)
            specs.append((f"{Path(path).parent / f[1]}:{f[2]}:{f[3]}:{name}" + (":grid" if f[0].endswith("=") else "") +
                          (f":loud={loud}" if loud is not None else "") + (f":hz={hz}" if hz else ""), f[1]))
    if not specs:
        raise SystemExit(f"{path}: no line marked *")
    return specs


# (LOAF BREAKER, 24 Nov 2026, FLEXOR: "les breaks mis à jour par profikid dans CHOPPAH doivent être à jour sur LOAF
# BREAKER"): CHOPPAH 1.5.3's bars picked from a live recording (BARS "3+4"), cut at the drummer's own tempo; its slices
# stay on an even grid here (FLEXOR: "tous les breaks ... ON GRID et pas ON HIT"), not on the hits as CHOPPAH's
def downbeat(x, rate, at, win):
    """where the drummer played the bar line near sample at: the strongest attack within win samples of it, 3 ms
    before it so the attack is whole; at itself when nothing there stands out (a bar that starts without a hit)"""
    h = 64
    lo, hi = max(h, at - win), min(len(x) - 2 * h, at + win)
    rms = lambda i, n: (sum(v * v for v in x[i:i + n]) / max(1, n)) ** 0.5
    best, bi = 0.0, None
    for i in range(lo, hi, h):
        before, after = rms(i - h, h), rms(i, h)
        if after > 4.0 * before + 1e-6:              # a rise of 6 dB+ in 1.5 ms: an attack
            loud = rms(i, int(rate * 0.03))
            if loud > best:
                best, bi = loud, i
    if bi is None or best < rms(0, len(x)):           # no hit louder than the break on average
        return at
    peak = max(abs(v) for v in x[bi:bi + int(rate * 0.01)])
    k = bi - h
    while k < bi + h and abs(x[k]) < 0.2 * peak:
        k += 1
    return max(0, k - int(rate * 0.003))


def pick_bars(x, rate, bar_len, pick):
    """the file's bars pick (from 1), in that order, each run of bars that follow in the file in one piece, from where
    the drummer played its first bar line (downbeat) to where he played the bar line after it: a live recording drifts
    off an even grid (the Amen's drummer speeds up, bars 3 and 4 at ~144.7 BPM in a file at 141.82), and a run cut to
    the file's average tempo drags at its end. A run at the file's end ends where its sound does (the Amen's WAV goes
    silent 71 ms before its end: where the next downbeat would come). Where a run follows one that isn't before it in
    the file, its first 1.5 ms fade in from what followed that one, so the join doesn't click. Returns the samples and
    the tempo they make (the bars picked, end to end)"""
    runs = []
    for b in pick:
        if runs and runs[-1][-1] + 1 == b:
            runs[-1].append(b)
        else:
            runs.append([b])
    win = int(bar_len / 32)                          # half a 16th
    sound = len(x)
    while sound > 1 and abs(x[sound - 1]) < 1e-5:
        sound -= 1
    out = []
    for j, r in enumerate(runs):
        s = downbeat(x, rate, int(round((r[0] - 1) * bar_len)), win)
        nxt = int(round(r[-1] * bar_len))
        e = sound if nxt >= sound - rate * 0.1 else downbeat(x, rate, nxt, win)
        seg = list(x[s:e])
        if j:                                        # a join
            n = int(rate * 0.0015)
            for i in range(min(n, len(x) - prev_end, len(seg))):
                w = (i + 0.5) / n
                seg[i] = x[prev_end + i] * (1.0 - w) + seg[i] * w
        prev_end = e
        out += seg
    return out, 240.0 * len(pick) * rate / len(out)


def prepare(spec, args, shown=None):
    parts = spec.split(":")
    path = Path(parts[0])
    bpm = float(parts[1]) if len(parts) > 1 and parts[1] else None
    pick = [int(b) for b in parts[2].split("+")] if len(parts) > 2 and "+" in parts[2] else None
    bars = len(pick) if pick else int(parts[2]) if len(parts) > 2 and parts[2] else args.bars
    given = parts[3] if len(parts) > 3 else None
    extra = parts[4:]                        # (LOAF BREAKER 0.9.34: "grid", a break marked *= or -= in the catalogue: an even
    transients = args.transients and "grid" not in extra   # grid, the BREAKS page's EVEN GRID; 0.9.35 "loud=X": its own
    loud = next((float(e[5:]) for e in extra if e.startswith("loud=")), args.loudness)   # loudness, NAME @X there; @0 its
                                             # file's own level, the peaks to full scale; 0.9.37 "hz=N": its own rate)
    out_rate = next((int(e[3:]) for e in extra if e.startswith("hz=")), args.rate)
    x, rate = read_wav(path)
    if bpm is None:                          # the file is whole bars
        if pick:
            raise SystemExit(f"{path}: bars {parts[2]} need the loop's BPM")
        bpm = 240.0 * bars / (len(x) / rate)
    bar_len = rate * 240.0 / bpm
    if pick:                                 # the bars asked for, in that order, at the drummer's own tempo (CHOPPAH's)
        if max(pick) * bar_len > len(x) + 2 or min(pick) < 1:
            raise SystemExit(f"{path}: has no bar {max(pick)} at {bpm:.2f} BPM")
        x, bpm = pick_bars(x, rate, bar_len, pick)
        bar_len = rate * 240.0 / bpm
    else:
        x = x[:int(round(bar_len * bars))]   # exactly the bars asked for
    if len(x) < int(bar_len * bars) - 2:
        raise SystemExit(f"{path}: shorter than {bars} bar(s) at {bpm:.2f} BPM")
    if loud:                                 # every break as loud as the others (its peaks limited)
        x, gain_db, lim_db = level(x, rate, loud)
    else:                                    # the loudest sample at full scale
        peak = max(1e-9, max(abs(v) for v in x))
        gain_db, lim_db = 20.0 * math.log10(0.98 / peak), 0.0
        x = [v * (0.98 / peak) for v in x]
    x = resample(x, rate, out_rate)
    max_len = getattr(args, "max_len", None) or BANK_MAX_LEN   # (the VST's longer; the pages' callers have none)
    if len(x) > max_len:
        raise SystemExit(f"{shown or path}: {bars} bar(s) at {bpm:.2f} BPM are {len(x)} samples, the firmware's buffer "
                         f"holds {max_len} (fewer bars)")
    nslices = min(BANK_SLICES, args.break_slices or args.slices * bars)
    pts = slice_points(x, out_rate, nslices, transients)
    if getattr(args, "pcm16", False):  # (LOAF BREAKER's VST: 16-bit PCM, as recorded; the slot's rsv bit 0 says so)
        data = struct.pack(f"<{len(x)}h", *(max(-32768, min(32767, int(round(v * 32767.0)))) for v in x))
    else:
        data = adpcm_encode(x)
    name = (given or (args.name_prefix + path.stem)).upper()[:11]
    return {"name": name, "file": shown or str(path), "bpm": round(bpm, 2), "bars": bars, "rate": out_rate,
            "len": len(x), "nslices": nslices, "slices": pts, "data": data, "pcm": x,
            "gain_db": round(gain_db, 1), "limit_db": round(lim_db, 1)}


def build_bank(slots, pcm16=False):
    table = bytearray()
    data = bytearray()
    off = 16 + 168 * len(slots)
    for s in slots:
        pad = (-len(data)) % 4
        data += b"\0" * pad
        sl = s["slices"] + [s["len"]] * (BANK_SLICES + 1 - len(s["slices"]))
        table += struct.pack(SLOT_FMT, s["name"].encode("ascii", "replace"), int(round(s["bpm"] * 100)), s["rate"],
                             s["len"], off + len(data), len(s["data"]), s["bars"], s["nslices"], 1 if pcm16 else 0, *sl)
        data += s["data"]
    body = bytes(table + data)
    return struct.pack(HDR_FMT, BANK_MAGIC, BANK_VERSION, len(slots), len(body), zlib.crc32(body)) + body


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("inputs", nargs="*", help="FILE[:BPM[:BARS[:NAME]]]")
    ap.add_argument("--list", type=Path, help="a catalogue (breaks/breaks.txt): its lines marked * instead of inputs")
    ap.add_argument("--bars", type=int, default=1, help="bars per break when not given (default 1)")
    ap.add_argument("--slices", type=int, default=16, help="slices a bar (default 16)")
    ap.add_argument("--break-slices", type=int, default=0,
                    help="slices a break whatever its bars (16: one per white key) instead of --slices a bar")
    ap.add_argument("--rate", type=int, default=22050, help="sample rate in the bank (default 22050, PRD Q4)")
    ap.add_argument("--transients", action="store_true", help="move markers to the nearest hit")
    ap.add_argument("--loudness", type=float, default=-12.0,
                    help="every break to this level, LUFS (as heard: K-weighted, gated), its peaks limited (default -12 since 0.9.70, FLEXOR: every break at the same level; -16 before; "
                         "0: peaks to full scale instead)")
    ap.add_argument("--name-prefix", default="", help="put before each name")
    ap.add_argument("-o", "--out", type=Path, default=Path("build/bank.bin"))
    ap.add_argument("--dry-run", action="store_true", help="report what fits, write nothing")
    ap.add_argument("--header", type=Path, help="also write the bank as a C header (CH_BANK[]) for the firmware")
    ap.add_argument("--verify", action="store_true", help="decode again and report the ADPCM's SNR")
    ap.add_argument("--all", action="store_true", help="--list: every line, * and - (LOAF BREAKER's VST: the whole library)")
    ap.add_argument("--pcm16", action="store_true", help="16-bit PCM instead of IMA ADPCM (the VST's bank: no FM-1 reads it)")
    ap.add_argument("--slots", type=int, default=0, help="at most this many breaks (default the FM-1's 16)")
    ap.add_argument("--capacity", type=int, default=0, help="bytes the bank may take (default the FM-1's)")
    ap.add_argument("--max-len", type=int, default=0, help="samples a break may have (default the FM-1's buffer)")
    args = ap.parse_args()
    specs = read_list(args.list, args.all) if args.list else [(i, None) for i in args.inputs]
    if not specs:
        ap.error("inputs or --list")
    if len(specs) > (args.slots or BANK_SLOTS):
        raise SystemExit(f"{len(specs)} breaks: at most {args.slots or BANK_SLOTS}")
    slots = [prepare(s, args, shown) for s, shown in specs]
    if args.list:                        # (LOAF BREAKER 0.9.43, FLEXOR: the bank always slowest to fastest, whatever the
        slots.sort(key=lambda s: s["bpm"])   # catalogue's order (the same BPM: its order kept): a new FM-1 starts on the
                                         # slowest, the bank's first)
    capacity = args.capacity or (IMAGE_CAPACITY if args.header else BANK_CAPACITY)
    bank = build_bank(slots, args.pcm16)
    for s in slots:
        line = (f"{s['name']:<12} {s['bpm']:7.2f} BPM  {s['bars']} bar(s)  {s['nslices']:2} slices  "
                f"{s['len'] / s['rate']:5.2f} s  {len(s['data']) / 1024:6.1f} KB  {s['gain_db']:+5.1f} dB"
                f"{'  peaks -%.1f dB' % s['limit_db'] if s['limit_db'] >= 0.1 else ''}")
        if args.verify and not args.pcm16:
            d = adpcm_decode(s["data"], s["len"])
            sig = sum((v * 32767.0) ** 2 for v in s["pcm"])
            err = sum((v * 32767.0 - q) ** 2 for v, q in zip(s["pcm"], d)) + 1.0
            line += f"  SNR {10 * math.log10(sig / err):5.1f} dB"
        print(line)
    print(f"bank: {len(bank) / 1024:.1f} KB of {capacity / 1024:.0f} KB "
          f"({100 * len(bank) / capacity:.0f} %){' (built into the firmware)' if args.header else ''}")
    if len(bank) > capacity:
        raise SystemExit("does not fit: fewer bars, a lower --rate, or fewer breaks")
    if args.dry_run:
        return
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_bytes(bank)
    manifest = [{k: v for k, v in s.items() if k not in ("data", "pcm")} for s in slots]
    args.out.with_suffix(".json").write_text(json.dumps(manifest, indent=2))
    print(f"wrote {args.out} and {args.out.with_suffix('.json')}")
    if args.header:
        rows = ",\n".join("    " + ",".join(f"0x{b:02x}" for b in bank[i:i + 24]) for i in range(0, len(bank), 24))
        names = ", ".join(s["name"] for s in slots)
        args.header.write_text(
            "/* Generated by tools/prep_breaks.py: don't edit (./build.sh makes it again from breaks/breaks.txt, and\n"
            f" * git ignores it). Slots: {names} */\n"
            "#pragma once\n#include <stdint.h>\n"
            f"#define CH_BANK_SIZE {len(bank)}u\n"
            "static const uint8_t CH_BANK[CH_BANK_SIZE] __attribute__((aligned(4))) = {\n" + rows + "\n};\n")
        print(f"wrote {args.header} ({len(bank)} bytes)")


if __name__ == "__main__":
    main()
