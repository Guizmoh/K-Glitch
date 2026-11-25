#!/usr/bin/env python3
# SPDX-License-Identifier: MPL-2.0
"""K-GLITCH 0.2b: fm1-base's web installer as one file, the firmware inside it (no server, no internet).

  python3 tools/make_installer.py app/build/k-glitch-0.2.fwsc 0.2b OUT.html

fm1-base's web/installer.html with its scripts inlined and the package as a data: URL (as base/tools/make_site.py
builds install/, the firmware and m-bootloader beside it). Opened from the disk in Chrome or Edge, it installs
K-GLITCH on an FM-1 plugged in by USB.
"""
import base64, hashlib, json, re, sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
BASE = ROOT / "base"
sys.path.insert(0, str(BASE / "tools"))
from make_site import strip_module, product_of, MBOOT_VERSION   # noqa: E402

def data_url(p):
    return "data:application/octet-stream;base64," + base64.b64encode(p.read_bytes()).decode()

def main(pkg, version, out):
    pkg = Path(pkg)
    raw = pkg.read_bytes()
    product = product_of(raw)
    if not product.startswith("FM-1_4519"):
        raise SystemExit(f"{pkg}: identity {product!r} is not K-GLITCH's")
    html = (BASE / "web" / "installer.html").read_text(encoding="utf-8")
    lib = "\n".join(strip_module((BASE / "web" / f).read_text(encoding="utf-8")) for f in ("fm1ports.js", "fm1pkg.js", "fm1ota.js"))
    boot = BASE / "web" / "m-bootloader" / f"fm1-uboot-{MBOOT_VERSION}.fwsc"
    meta = json.dumps({"version": version, "product": product, "pkg": data_url(pkg), "backup": True, "res": None,
                       "boot": {"pkg": data_url(boot), "version": MBOOT_VERSION, "sha256": hashlib.sha256(boot.read_bytes()).hexdigest()}})
    repo = "https://github.com/guizmoh/k-glitch"
    credits = ("K-GLITCH 0.2b by Guizmoh, built on LOAF BREAKER by FLEXOR, on CHOPPAH by Laurens Profittlich, on fm1-base "
               "and felucca-bsp, from Felucca by Leo Kuroshita (Hügelton Instruments).")
    for key, val in (("RES", "K-GLITCH's data"), ("RES_JA", "K-GLITCH のデータ"), ("NAME", "K-GLITCH"), ("REPO", repo),
                     ("REPO_SHORT", re.sub(r"^https?://", "", repo)), ("LICENSE", "MPL-2.0"), ("CREDITS", " " + credits)):
        html = html.replace("{{" + key + "}}", val)
    app_js = (ROOT / "app" / "site" / "install_app.js").read_text(encoding="utf-8")
    html = html.replace("/*LIB*/", lib).replace("/*META*/", meta).replace("/*APP*/", app_js)
    html = re.sub(r"<title>[^<]*</title>", f"<title>K-GLITCH {version} installer</title>", html, count=1)
    Path(out).write_text(html, encoding="utf-8")
    print(f"installer: {out} ({len(html)} B), K-GLITCH {version}, {product}, sha256 {hashlib.sha256(raw).hexdigest()[:16]}")

if __name__ == "__main__":
    if len(sys.argv) != 4:
        sys.exit(__doc__)
    main(*sys.argv[1:])
