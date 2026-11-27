#!/usr/bin/env python3
"""K-GLITCH 0.2b: the BREAKS page (LOAF BREAKER's, CHOPPAH 1.4's break manager) as one standalone file: your own
loops (WAV) sent to the FM-1 over USB (Web MIDI, Chrome). No library (built with LOAF BREAKER's site only).
   python3 tools/make_breaks_page.py OUT.html"""
import pathlib, re, sys
APP = pathlib.Path(__file__).resolve().parent.parent / "app"
page = (APP / "site" / "breaks.html").read_text(encoding="utf-8")
css = (APP / "site" / "site.css").read_text(encoding="utf-8")
page = page.replace("<style>/*SITE_CSS*/", "<style>" + css, 1).replace("<!--HEADER-->", "", 1)
# the visible text: K-GLITCH (the code's comments left as they are)
page = re.sub(r"(?<![/\w-])LOAF BREAKER( 0\.9\.32)?( or later)?", "K-GLITCH", page)
page = page.replace("/loaf|choppah|fm-?1/i", "/glitch|loaf|choppah|fm-?1/i")
page = page.replace("The library is not here (it is built with the site).",
                    "No library in this page: drop your own WAV loops below.")
pathlib.Path(sys.argv[1]).write_text(page, encoding="utf-8")
print("breaks page:", sys.argv[1], len(page), "B")
