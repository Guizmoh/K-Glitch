#!/bin/sh
# SPDX-License-Identifier: MPL-2.0
# The ATELIER (LOAF BREAKER 0.9.75, FLEXOR: a secret page to install TEST and personal builds): the installer as
# make_site writes it (fm1-base's web/installer.html, its libraries inlined, the package's metadata), then
# tools/site_style.py (install, atelier); in Chromium: a LOAF BREAKER .fwsc chosen is taken (Install enabled), a zip
# holding one too, another identity (m-bootloader's) refused; the page is noindex and no other page links it.
# Needs app/build/loaf-breaker.fwsc (./build.sh) and Playwright with its Chromium; without either it says so and passes.
#   tests/atelier_page.sh
set -e
cd "$(dirname "$0")/.."
PKG="$(ls app/build/loaf-breaker*.fwsc 2>/dev/null | head -1)"
if ! node -e "require('playwright')" 2>/dev/null || [ ! -f "${PKG:-/none}" ]; then
    echo "atelier_page: no Playwright or no $PKG here, skipped"
    exit 0
fi
O=app/build/atelier
S="$O/site"
rm -rf "$O"
mkdir -p "$S/install"
python3 - "$S" "$PKG" "$O" <<'P'
import json, sys, zipfile
from pathlib import Path
sys.path.insert(0, "base/tools")
import make_site as m
site, pkg, out = Path(sys.argv[1]), Path(sys.argv[2]), Path(sys.argv[3])
html = (m.SRC / "web" / "installer.html").read_text(encoding="utf-8")
lib = "\n".join(m.strip_module((m.SRC / "web" / f).read_text(encoding="utf-8")) for f in ("fm1ports.js", "fm1pkg.js", "fm1ota.js"))
meta = json.dumps({"version": "0.0.0", "product": m.product_of(pkg.read_bytes()), "pkg": "../firmware/none.fwsc",
                   "backup": False, "res": None, "boot": None})
for k, v in (("RES", "data"), ("RES_JA", "data"), ("NAME", "LOAF BREAKER"), ("REPO", ""), ("REPO_SHORT", ""),
             ("LICENSE", "MPL-2.0"), ("CREDITS", "")):
    html = html.replace("{{" + k + "}}", v)
(site / "install" / "index.html").write_text(html.replace("/*LIB*/", lib).replace("/*META*/", meta).replace("/*APP*/", "null"), encoding="utf-8")
with zipfile.ZipFile(out / "build.zip", "w", zipfile.ZIP_DEFLATED) as z:
    z.writestr("readme.txt", "an artifact")
    z.write(pkg, "app/build/loaf-breaker-9.9.9-perso.fwsc")
P
cp -r app/site/img "$S/img"
python3 tools/site_style.py "$S" > /dev/null
grep -q 'name="robots" content="noindex' "$S/atelier/index.html"
if grep -rlE 'href="(\.\./)?atelier/' "$S" --include=index.html | grep -v "^$S/atelier/"; then echo "atelier_page: linked from a page"; exit 1; fi
timeout -k 5 120 node - "$S" "$PKG" "$O" <<'N'
const { chromium } = require("playwright");
const path = require("path");
const [S, PKG, O] = process.argv.slice(2).map((p) => path.resolve(p));
(async () => {
  const b = await chromium.launch();
  const p = await b.newPage();
  const errs = [];
  p.on("pageerror", (e) => errs.push(String(e)));
  await p.goto("file://" + S + "/atelier/index.html");
  const pick = async (f, want, go) => {
    await p.setInputFiles("#atfile", f);
    await p.waitForFunction((w) => document.getElementById("atstate").textContent.includes(w), want, { timeout: 10000 });
    const dis = await p.$eval("#go", (e) => e.disabled);
    if (dis === go) throw new Error(`${path.basename(f)}: Install ${go ? "disabled" : "enabled"}`);
    console.log(`atelier_page: ${path.basename(f)}: ${await p.$eval("#atstate", (e) => e.textContent)}`);
  };
  await pick(path.resolve("base/web/m-bootloader/fm1-uboot-1.2.fwsc"), "pas un LOAF BREAKER", false);
  await pick(PKG, "Prêt", true);
  await pick(O + "/build.zip", "9.9.9-perso", true);
  /* the app page's secret: five taps on its title show the tile */
  const a = await b.newPage();
  await a.goto("file://" + S + "/app/index.html");
  if (await a.$eval("#atelier", (e) => !e.hidden)) throw new Error("app: the atelier's tile shown at first");
  for (let i = 0; i < 5; i++) await a.click(".ap h1");
  if (await a.$eval("#atelier", (e) => e.hidden || !e.href.endsWith("/atelier/"))) throw new Error("app: five taps, no tile");
  console.log("atelier_page: the app's tile after five taps");
  if (errs.length) throw new Error(errs.join("\n"));
  await b.close();
})().catch((e) => { console.error("atelier_page: FAIL " + e.message); process.exit(1); });
N
echo "atelier_page: ok"
