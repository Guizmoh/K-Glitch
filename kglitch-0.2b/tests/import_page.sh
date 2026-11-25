#!/bin/sh
# SPDX-License-Identifier: MPL-2.0
# The IMPORT page (LOAF BREAKER 0.9.45) against the firmware: the page's own code (app/site/import.html: trim, shape,
# the rate that fits, ADPCM, the kit) run in node on a snare, a hat and a clap; its kit uploaded to the host simulator
# as the page sends it (zone 2) must check there (all three); the page reads its own kit back; three too long for 8 KB:
# the page takes 11025 Hz and shortens them to fit. Needs ./build.sh --host first.
#   tests/import_page.sh
set -e
cd "$(dirname "$0")/.."
O=app/build/import
mkdir -p "$O"
node - "$O" <<'N'
const fs = require("fs"), vm = require("vm"), o = process.argv[2];
const html = fs.readFileSync("app/site/import.html", "utf8");
const js = html.split("<script>")[1].split("/* ---------------------------------------------------------------- the page */")[0];
const c = {}; vm.createContext(c); vm.runInContext(js + "\nthis.K = {trim, shape, fitRate, adpcmEncode, adpcmDecode, buildKit, readKit, KIT_CAP};", c);
const K = c.K, R = 22050;
let seed = 3; const rnd = () => ((seed = (seed * 1103515245 + 12345) >>> 0) / 4294967296) * 2 - 1;
const mk = (sec, f) => Float32Array.from({ length: Math.round(sec * R) }, (_, i) => f(i / R));
function kit(srcs, names) {
  const xs = srcs.map((x) => K.trim(x, R)), fit = K.fitRate(xs.map((x) => ({ n: x.length, rate: R })));
  if (fit.rate !== R) throw new Error("these are made at 22050: the test wants that rate (" + fit.rate + ")");
  const ents = xs.map((x, v) => { const y = K.shape(x.subarray(0, Math.floor(x.length * Math.min(1, fit.keep))), R); return { name: names[v], n: y.length, rate: R, adpcm: K.adpcmEncode(y) }; });
  return { img: K.buildKit(ents), fit };
}
const sn = mk(0.25, (t) => t < 0.01 ? 0 : (Math.sin(2 * Math.PI * 190 * t) * 0.6 + rnd() * 0.5) * Math.exp(-t / 0.07));
const hh = mk(0.12, (t) => rnd() * Math.exp(-t / 0.02)), cp = mk(0.2, (t) => rnd() * Math.exp(-t / 0.05));
const a = kit([sn, hh, cp], ["PAGESNARE", "PAGEHAT", "PAGECLAP"]);
fs.writeFileSync(o + "/page_kit.bin", a.img);
const back = K.readKit(a.img);
if (!back || back.some((e) => !e)) throw new Error("the page can't read its own kit back");
const long = [0, 1, 2].map(() => mk(1.0, (t) => rnd() * Math.exp(-t / 0.5)));
const xs = long.map((x) => K.trim(x, R)), f2 = K.fitRate(xs.map((x) => ({ n: x.length, rate: R })));
if (f2.rate !== 11025 || !(f2.keep < 1)) throw new Error("three seconds: 11025 Hz and shortened, got " + f2.rate + " " + f2.keep);
console.log(`import_page: the page's kit ${a.img.length} B at ${a.fit.rate} Hz; three long ones: ${f2.rate} Hz, ${Math.round(f2.keep * 100)}% kept`);
N
H="$(ls app/build/host/*_host | head -1)"
{
echo "wait 4200"
echo "expect kit 0"
python3 - "$O/page_kit.bin" <<'P'
import sys; sys.path.insert(0, "tools"); import kit_frames as k
img = open(sys.argv[1], "rb").read()
for kind, f in k.frames(img):
    print("sysex " + k.cf.hexs(f)); print("wait 20" if kind == "data" else "wait 200")
P
echo "expect kit 7"
} > "$O/page.txt"
"$H" "$O/page.txt" "$O" | grep -E "FAIL|EXPECT" && exit 1
echo "import_page: the FM-1 (simulator) takes the page's kit: snare, hat and clap"
