#!/bin/sh
# SPDX-License-Identifier: MPL-2.0
# The break manager page (app/site/breaks.html) makes a break exactly as tools/prep_breaks.py does: its own code run
# in node on a real break against the Python, step by step. The bank image must be the same byte for byte for the
# same samples; the level within float rounding. Needs node.
#   tests/page_parity.sh
set -e
cd "$(dirname "$0")/.."
O=app/build/parity
mkdir -p "$O"
python3 - "$O" <<'P'
import argparse, json, sys
sys.path.insert(0, "tools")
import prep_breaks as pb
o = sys.argv[1]
x, rate = pb.read_wav("breaks/classics/rotary_connection_life_could.wav")
bpm = 99.81
x = x[:int(round(rate * 240.0 / bpm * 2))]
lv, _, _ = pb.level(x, rate, -12.0)   # (0.9.70: every break at -12 LUFS)
args = argparse.Namespace(bars=2, slices=16, break_slices=16, rate=22050, transients=True, loudness=-12.0, name_prefix="")
s = pb.prepare(f"breaks/classics/rotary_connection_life_could.wav:{bpm}:2:LIFE COULD", args)
img = pb.build_bank([s])
s2 = pb.prepare("breaks/classics/lightnin_rod_sport.wav:120.69:2:SPORT", args)   # (LOAF BREAKER 0.9.32: two, merged)
json.dump({"x": x[::1], "rate": rate, "level": lv, "pcm": s["pcm"], "slices": s["slices"], "bpm": bpm,
           "hex": img.hex(), "hex2": pb.build_bank([s2]).hex(), "both": pb.build_bank([s, s2]).hex()},
          open(f"{o}/py.json", "w"))
P
node - "$O" <<'N'
const fs = require("fs"), o = process.argv[2];
const page = fs.readFileSync("app/site/breaks.html", "utf8");
const code = page.split("the break, as tools/prep_breaks.py makes it */")[1].split("/* ----------------------------------------------------------------")[0];
const api = new Function(code + "; return {level, slicePoints, buildBank, readBank, adpcmDecode, mergeBanks, splitBank, bankBytes, atFix};")();
const py = JSON.parse(fs.readFileSync(o + "/py.json"));
let worst = 0;
const lv = api.level(Float32Array.from(py.x), py.rate);
for (let i = 0; i < lv.length; i++) worst = Math.max(worst, Math.abs(lv[i] - py.level[i]));
if (worst > 2e-4) throw new Error("level differs by " + worst);
const pcm = Float32Array.from(py.pcm);
const sl = api.slicePoints(pcm, 16, true);
if (JSON.stringify(sl) !== JSON.stringify(py.slices)) throw new Error("slices differ:\n" + sl + "\n" + py.slices);
const img = api.buildBank({ name: "LIFE COULD", bpm: py.bpm, rate: 22050, pcm, slices: sl });
const hex = Buffer.from(img).toString("hex");
if (hex !== py.hex) {
  let i = 0; while (hex[i] === py.hex[i]) i++;
  throw new Error(`bank differs at byte ${i >> 1} of ${py.hex.length >> 1}`);
}
const fn = (name) => page.match(new RegExp("function " + name + "\\([^]*?\\n}\\n"))[0];
const { pack7, unpack7 } = new Function(fn("pack7") + fn("unpack7") + "return {pack7, unpack7};")();
const { execSync } = require("child_process");
const probe = Uint8Array.from({ length: 516 }, (_, i) => (i * 37 + 200) & 255);
const pyPacked = execSync(`python3 -c "import sys; sys.path.insert(0,'tools'); import cu_frames as c; print(c.pack7(bytes((i*37+200)&255 for i in range(516))).hex())"`).toString().trim();
if (Buffer.from(pack7(probe)).toString("hex") !== pyPacked) throw new Error("pack7 differs from cu_frames.py's");
if (Buffer.compare(Buffer.from(unpack7(Uint8Array.from(pack7(probe)))), Buffer.from(probe))) throw new Error("unpack7(pack7) is not the same");
const hx = (h) => Uint8Array.from(Buffer.from(h, "hex"));
const both = api.mergeBanks([hx(py.hex), hx(py.hex2)]);
if (Buffer.from(both).toString("hex") !== py.both) throw new Error("two banks merged differ from build_bank's");
const parts = api.splitBank(both);
if (parts.length !== 2 || Buffer.from(parts[0]).toString("hex") !== py.hex || Buffer.from(parts[1]).toString("hex") !== py.hex2)
  throw new Error("split back differs");
if (api.bankBytes([hx(py.hex), hx(py.hex2)]) !== both.length) throw new Error("bankBytes differs from mergeBanks' length");
for (const z of [0, 1]) {                /* an "AT" put at 0xF06 of the bank's first sector (an XDATA window in both
                                          * places) and of its sixth (zone 0's data area: kept), nudged as cu_frames.py */
  const t = Uint8Array.from(both);
  t.set([0x41, 0x54], 0xF06); t.set([0x41, 0x54], 5 * 4096 + 0xF06);
  fs.writeFileSync(o + "/at.bin", t);
  const want = execSync(`python3 -c "import sys; sys.path.insert(0,'tools'); import cu_frames as c; print(c.at_fix(open('${o}/at.bin','rb').read(), ${z}).hex())"`).toString().trim();
  const got = api.atFix(t, z);
  if (Buffer.from(got).toString("hex") !== want) throw new Error("atFix differs from cu_frames.py's, zone " + z);
  if (got[0xF06] !== 0x40 || got[5 * 4096 + 0xF06] !== (z ? 0x40 : 0x41)) throw new Error("atFix in the wrong places, zone " + z);
}
const back = api.readBank(img);
if (!back || back.name !== "LIFE COULD" || back.slices.length !== 17) throw new Error("readBank");
console.log(`page_parity: level within ${worst.toExponential(1)}, slices the same, the bank the same (${img.length} B), read back; two merged as build_bank and split back, their size worked out, an update record's mark nudged as cu_frames.py; the frames packed as cu_frames.py`);
N
