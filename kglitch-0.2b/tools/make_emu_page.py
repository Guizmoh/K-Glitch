#!/usr/bin/env python3
# SPDX-License-Identifier: MPL-2.0
"""K-GLITCH 0.2b: the web emulator (./build.sh --web: app/build/emu) as one HTML file (an artifact, a file opened from
the disk): the wasm in base64, the worklets in one (a blob URL, data: if refused), the app's part inlined, no reader.

  python3 tools/make_emu_page.py app/build/emu OUT.html
"""
import base64, json, re, sys
from pathlib import Path

def main(emu, out):
    emu = Path(emu)
    h = (emu / "index.html").read_text(encoding="utf-8")
    w = base64.b64encode((emu / "app.wasm").read_bytes()).decode()
    wk = "\n".join((emu / f).read_text(encoding="utf-8") for f in ("parts/parts_worklet.js", "parts/resample_worklet.js", "worklet.js"))
    wk = re.sub(r"^\s*import [^\n]*\n", "", wk, flags=re.M)
    parts = re.sub(r"^export ", "", (emu / "parts/parts.js").read_text(encoding="utf-8"), flags=re.M)
    app = (emu / "app.js").read_text(encoding="utf-8").replace('import { compose } from "./parts/parts.js";', "") \
        .replace("export const { attach, message } =", "const { attach, message } =")
    mod = parts + "\n" + app + "\nreturn { attach, message };"
    pre = ('<script>\nconst __WASM_B64="' + w + '";\nconst __WK=' + json.dumps(wk) + ";\nconst __APP=new Function(" +
           json.dumps(mod) + ");\n</script>\n")
    subs = [
        ('fetch("app.wasm").then((r) => { if (!r.ok) throw new Error(`app.wasm: ${r.status}`); return r.arrayBuffer(); })',
         "Promise.resolve(Uint8Array.from(atob(__WASM_B64), (c) => c.charCodeAt(0)).buffer)"),
        ('ac.audioWorklet.addModule("app_worklet.js").catch(() => {})\n                                        .then(() => ac.audioWorklet.addModule("worklet.js"))',
         'ac.audioWorklet.addModule(URL.createObjectURL(new Blob([__WK], { type: "text/javascript" })))\n'
         '                                        .catch(() => ac.audioWorklet.addModule("data:text/javascript;base64," + btoa(unescape(encodeURIComponent(__WK)))))'),
        ('appPart = await import("./app.js").catch(() => null);', "appPart = (() => { try { return __APP(); } catch (e) { return null; } })();"),
        ('const wordsLib = import("./reader/words.js").then((m) => { spoken = m.spoken; return m; }).catch(() => null);',
         "const wordsLib = Promise.resolve(null);"),
        ('ttsReady = import("./reader/tts.js").then((m) => m.createTTS())', 'ttsReady = Promise.reject(new Error("no reader")).then((m) => m.createTTS())'),
    ]
    for a, b in subs:
        if a not in h:
            raise SystemExit(f"index.html changed: {a[:50]!r}")
        h = h.replace(a, b, 1)
    h = re.sub(r"<link[^>]*fonts\.(googleapis|gstatic)[^>]*>\n?", "", h)
    i = h.index("<script>")
    h = h[:i] + pre + h[i:]
    h = re.sub(r"<title>[^<]*</title>", "<title>K-GLITCH 0.2b Emulator</title>", h, count=1)
    Path(out).write_text(h, encoding="utf-8")
    print(f"emulator page: {out} ({len(h)} B)")

if __name__ == "__main__":
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    main(*sys.argv[1:])
