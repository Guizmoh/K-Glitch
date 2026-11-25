#!/usr/bin/env python3
# SPDX-License-Identifier: MPL-2.0
"""CHOPPAH's look for the site fm1-base's tools/make_site.py builds (run it after make_site.py):

  tools/site_style.py SITE_DIR

- install/index.html (fm1-base's installer): site.css and app/site/install.css in place of its style, and the
  site's header above it; its markup and script stay as they are;
- emu/index.html (fm1-base's emulator): site.css and app/site/emu.css in place of its style (its Barlow stays for the
  panel's printed labels), the site's
  header and an introduction in place of its title, and a "where to start" row under the panel; its panel, markup
  and script stay as they are;
- index.html and manual/index.html: app/site/site.css (with its fonts) in place of fm1-base's style, a class on
  <body> for each page, and a "Controls" link in the site's header (to the manual's control map);
- manual/index.html: the control map (app/site/controlmap.html: explorer, buttons, the whole map, who owns the
  knobs) after the heading of the section "The control map", in place of that section's text (the Markdown's text
  is for readers on GitHub); and the contents follow the reading (the section on screen marked).

The pages keep everything else make_site.py gave them: their text, links, images, tables and the contents."""
import re
import sys
from pathlib import Path

APP = Path(__file__).resolve().parents[1] / "app"
FONTS = ('<link rel="preconnect" href="https://fonts.googleapis.com">\n'
         '<link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>\n'
         '<link rel="stylesheet" href="https://fonts.googleapis.com/css2?family=Chakra+Petch:wght@500;600;700'
         '&family=IBM+Plex+Mono:wght@400;500;600&family=IBM+Plex+Sans:wght@400;500;600&display=swap">\n')
SPY = """<script>
(function () {                         /* the contents follow the reading: the section on screen marked */
  var links = {}, nav = document.querySelector("nav.contents");
  if (!nav || !("IntersectionObserver" in window)) return;
  nav.querySelectorAll("a[href^='#']").forEach(function (a) { links[a.getAttribute("href").slice(1)] = a; });
  var seen = new Set();
  var io = new IntersectionObserver(function (es) {
    es.forEach(function (e) { if (e.isIntersecting) seen.add(e.target.id); else seen.delete(e.target.id); });
    var first = Object.keys(links).find(function (id) { return seen.has(id); });
    if (!first) return;
    Object.keys(links).forEach(function (id) {
      if (id === first) links[id].setAttribute("aria-current", "true"); else links[id].removeAttribute("aria-current");
    });
  }, { rootMargin: "-80px 0px -55% 0px" });
  Object.keys(links).forEach(function (id) { var h = document.getElementById(id); if (h) io.observe(h); });
})();
</script>
"""


def restyle(path, cls, controls_href, extra=""):
    s = path.read_text(encoding="utf-8")
    css = (APP / "site" / "site.css").read_text(encoding="utf-8") + extra
    s, n = re.subn(r"<style>.*?</style>", lambda m: FONTS + "<style>" + css + "</style>", s, count=1, flags=re.S)
    if n != 1:
        raise SystemExit(f"{path}: no <style> to replace")
    s = s.replace("<body>", f'<body class="{cls}">', 1)
    nav = re.search(r'(<header class="site">.*?<nav>)(.*?)(</nav>)', s, re.S)
    if nav:                                          # "Controls" after "Manual"
        breaks = "breaks/" if controls_href.startswith("manual/") else "../breaks/"   # (the front page, the manual)
        links = re.sub(r'(<a href="[^"]*manual/"[^>]*>Manual</a>)',
                       r'\1<a href="' + controls_href + '">Controls</a><a href="' + breaks + '">Breaks</a><a href="' + breaks.replace("breaks", "import") + '">Import</a><a href="' + breaks.replace("breaks", "editor") + '">Studio</a><a href="' + breaks.replace("breaks", "vst") + '">VST</a>',
                       nav.group(2), count=1)
        s = s[:nav.start(2)] + links + s[nav.end(2):]
    return s


def header(current):
    conf = (APP / "fm1.toml").read_text(encoding="utf-8")
    name = re.search(r'^title\s*=\s*"([^"]+)"', conf, re.M) or re.search(r'^name\s*=\s*"([^"]+)"', conf, re.M)
    repo = re.search(r'^repo\s*=\s*"([^"]+)"', conf, re.M)
    links = [("../vst/", "VST"), ("../emu/", "Emulator"), ("../install/", "Install"), ("../manual/", "Manual"), ("../manual/#controls", "Controls"),
             ("../breaks/", "Breaks"), ("../import/", "Import"), ("../editor/", "Studio"), ("../live/", "Live"),
             ("../app/", "App")]
    if repo:
        links.append((repo.group(1), "Source"))
    return ('<div class="page"><header class="site"><a class="home" href="../">' + (name.group(1) if name else "CHOPPAH") +
            "</a><nav>" + "".join(f'<a href="{h}"' + (' aria-current="page"' if t == current else "") + f">{t}</a>"
                                  for h, t in links) + "</nav></header></div>\n")


EMU_START = """<div class="start-here" aria-label="Where to start">
  <div><b>Switch on, then PLAY</b><p>The break plays at 170 BPM, the mix scrolling by as on a CDJ. The white keys are its sixteen slices: press one and it lands on the next snap point; hold it and it loops.</p></div>
  <div><b>Throw the pads</b><p>The black keys are momentary effects: rolls on the break or the kick, reverse, filter, half speed, vinyl break, gate, random, SUB DRIVE. Hold one and turn K1–K4 to tune it. EDIT opens the kick.</p></div>
  <div><b>Find your way</b><p>Each button opens a page; press it again for its next part. <span class="k">SEL</span> held a second lists every combination. All of it is in the <a href="../manual/#controls">control map</a>.</p></div>
</div>
"""


def emu(site):
    """the emulator: CHOPPAH's style and fonts, the site's header, an introduction, a row of where to start"""
    page = site / "emu" / "index.html"
    if not page.exists():
        return
    s = restyle(page, "emu", "", (APP / "site" / "emu.css").read_text(encoding="utf-8"))
    m = re.search(r"<header>\s*<h1>([^<]*)</h1>\s*</header>\s*<main>", s)
    if not m:
        raise SystemExit(f"{page}: no <header><h1> then <main> to replace")
    s = (s[:m.start()] + header("Emulator") + '<main>\n<div class="intro"><h1>' + m.group(1) + " in the browser</h1><p>The "
         "same code the FM-1 runs, with sound. Play it with the mouse, a touch screen or the keyboard.</p></div>" + s[m.end():])
    s = s.replace('<details class="speak" id="speak">', EMU_START + '<details class="speak" id="speak">', 1)
    page.write_text(s, encoding="utf-8")


def install(site):
    """the installer: CHOPPAH's style, and the site's header (the same pages as the manual's) above its main"""
    page = site / "install" / "index.html"
    if not page.exists():
        return
    s = restyle(page, "install", "", (APP / "site" / "install.css").read_text(encoding="utf-8"))
    s = s.replace('<body class="install">', '<body class="install">\n' + header("Install"), 1)
    page.write_text(s, encoding="utf-8")


ATELIER_HTML = """<h1>Atelier</h1>
  <p class="atlead">Installe n'importe quel LOAF BREAKER : la TEST du moment, une ancienne version, un firmware fait sur une
  branche. Page secrète : dans aucun menu, ni dans les moteurs de recherche.</p>
  <div id="atdrop" class="atdrop">
    <p><strong>Glisse ici un .fwsc</strong> (ou le .zip d'une build GitHub)</p>
    <p><label for="atfile" class="sr">Fichier du firmware</label><input type="file" id="atfile" accept=".fwsc,.zip"></p>
    <p id="atstate" role="status"></p>
  </div>
  <ul class="small atwhere">
    <li><strong>TEST du moment</strong> : <a data-at="/releases" target="_blank" rel="noopener">Releases</a>, le brouillon
      « LOAF BREAKER TEST » (connecté à GitHub : toi seul le vois), fichier loaf-breaker-A.B.C-testN.fwsc.</li>
    <li><strong>Une branche à toi</strong> : pousse-la en <code>perso/un-nom</code> (ou Actions › LOAF BREAKER › Run
      workflow sur ta branche) : un brouillon « LOAF BREAKER PERSO un-nom » arrive dans
      <a data-at="/releases" target="_blank" rel="noopener">Releases</a>.</li>
    <li><strong>Une ancienne version</strong> : <a data-at="/releases" target="_blank" rel="noopener">Releases</a>,
      loaf-breaker-A.B.C.fwsc.</li>
  </ul>"""

ATELIER_CSS = """<style>
.atdrop { border: 2px dashed currentColor; border-radius: 10px; padding: 14px 16px; margin: 18px 0; opacity: .95; }
.atdrop.over { border-style: solid; background: rgba(255, 255, 255, .06); }
.atdrop p { margin: 6px 0; }
#atstate { font-weight: 600; min-height: 1.4em; }
#atstate.bad { color: #ff6b6b; }
.atwhere li { margin: 4px 0; }
</style>
"""


def atelier(site):
    """LOAF BREAKER (0.9.75, FLEXOR: "un onglet secret ... pour installer les firmware de test ou les firmwares perso du
    git"): atelier/, the installer (install/, already restyled) with any LOAF BREAKER package dropped on it in place of
    the site's own (app/site/atelier.js inside its module); in no menu, noindex"""
    src = site / "install" / "index.html"
    if not src.exists():
        return
    s = src.read_text(encoding="utf-8")
    js = (APP / "site" / "atelier.js").read_text(encoding="utf-8")
    load = "    const r = await fetch(meta.pkg);\n"
    click = '$("go").addEventListener("click", () => run(image, meta.product, false));'
    for mark in (load, click, "</head>", "<title>"):
        if s.count(mark) != 1:
            raise SystemExit(f"install/index.html: {mark.strip()!r} not found once (atelier)")
    s, n = re.subn(r"<h1>.*?</h1>", ATELIER_HTML, s, count=1, flags=re.S)
    if n != 1:
        raise SystemExit("install/index.html: no <h1> (atelier)")
    s = s.replace(load, '    say("Choisis un firmware ci-dessus."); return;   /* (the atelier: no package of its own) */\n' + load, 1)
    s = s.replace(click, js.replace("</script", "<\\/script") + "\n" + click, 1)
    s = re.sub(r"<title>[^<]*</title>", "<title>Atelier · LOAF BREAKER</title>", s, count=1)
    s = s.replace("</head>", '<meta name="robots" content="noindex, nofollow">\n' + ATELIER_CSS + "</head>", 1)
    (site / "atelier").mkdir(parents=True, exist_ok=True)
    (site / "atelier" / "index.html").write_text(s, encoding="utf-8")


def site_name():
    conf = (APP / "fm1.toml").read_text(encoding="utf-8")
    m = re.search(r'^title\s*=\s*"([^"]+)"', conf, re.M)
    n = re.search(r'^name\s*=\s*"([^"]+)"', conf, re.M)
    return (m.group(1) if m else None), (n.group(1) if n else None)


def rebrand(site):
    """LOAF BREAKER: a fork's site in its own name (fm1.toml title, the build keeping CHOPPAH's): the pages'
    titles and words, the package's file (choppah-X.fwsc -> loaf-breaker-X.fwsc); the credits to CHOPPAH kept"""
    new, old = site_name()
    if not new or not old or new == old:
        return
    stem = new.lower().replace(" ", "-")
    renames = {}
    for f in (site / "firmware").glob(old.lower() + "-*"):
        to = f.with_name(stem + f.name[len(old):])
        renames[f.name] = to.name
        f.rename(to)
    keep = re.compile(r'(?<=fm1-break-choppah">)' + re.escape(old) + "|" + re.escape(old) +
                      r"(?=</strong>, Laurens| by Laurens|'s author)")   # (the credits: the link, the sentences)
    for f in site.rglob("*"):
        if f.suffix not in (".html", ".js", ".json", ".webmanifest") or not f.is_file():
            continue
        t = f.read_text(encoding="utf-8")
        u = t
        for a, b in renames.items():
            u = u.replace(a, b)
        u = keep.sub("\0KEEP\0", u)
        u = u.replace(old, new)                     # (upper case: the name; its URLs are lower case)
        u = u.replace("<title>" + new + ": " + new, "<title>" + new + ": the manual")
        u = u.replace("\0KEEP\0", old)
        if u != t:
            f.write_text(u, encoding="utf-8")


def logo(site):
    """LOAF BREAKER: every page's tab icon, LOAF LORD's face (app/site/img, tools/make_site_logo.py); ?v= the icon's
    size, so a browser that kept an older one takes the new"""
    ico = site / "img" / "favicon.png"
    if not ico.exists():
        return
    v = ico.stat().st_size
    for f in site.rglob("index.html"):
        up = "../" * (len(f.relative_to(site).parts) - 1)
        s = f.read_text(encoding="utf-8")
        s = s.replace("</head>", f'<link rel="icon" type="image/png" sizes="64x64" href="{up}img/favicon.png?v={v}">\n'
                                 f'<link rel="apple-touch-icon" href="{up}img/apple-touch-icon.png?v={v}">\n</head>', 1)
        f.write_text(s, encoding="utf-8")


def breaks(site):
    """the BREAKS page (app/site/breaks.html, CHOPPAH 1.4's break manager: the library to hear, your own breaks over
    Web MIDI): the site's style and header, at breaks/, and its library (tools/break_lib.py) at breaks/lib/"""
    page = (APP / "site" / "breaks.html").read_text(encoding="utf-8")
    css = (APP / "site" / "site.css").read_text(encoding="utf-8")
    if "/*SITE_CSS*/" not in page or "<!--HEADER-->" not in page:
        raise SystemExit("app/site/breaks.html: no /*SITE_CSS*/ or <!--HEADER--> to fill")
    page = page.replace("<style>/*SITE_CSS*/", FONTS + "<style>" + css, 1).replace("<!--HEADER-->", header("Breaks"), 1)
    (site / "breaks").mkdir(parents=True, exist_ok=True)
    (site / "breaks" / "index.html").write_text(page, encoding="utf-8")
    import break_lib                               # (0.9.32, FLEXOR: the library the page plays and sends: lib/)
    break_lib.main(site / "breaks" / "lib")


def imp(site):
    """the IMPORT page (app/site/import.html, LOAF BREAKER 0.9.45, FLEXOR: your own snare, hat and clap on the FM-1 over
    Web MIDI): the site's style and header, at import/"""
    page = (APP / "site" / "import.html").read_text(encoding="utf-8")
    css = (APP / "site" / "site.css").read_text(encoding="utf-8")
    if "/*SITE_CSS*/" not in page or "<!--HEADER-->" not in page:
        raise SystemExit("app/site/import.html: no /*SITE_CSS*/ or <!--HEADER--> to fill")
    page = page.replace("<style>/*SITE_CSS*/", FONTS + "<style>" + css, 1).replace("<!--HEADER-->", header("Import"), 1)
    (site / "import").mkdir(parents=True, exist_ok=True)
    (site / "import" / "index.html").write_text(page, encoding="utf-8")


def editor(site):
    """the Studio (app/site/editor.html and editor.js, LOAF BREAKER 0.9.43: the whole FM-1 from a computer over Web MIDI,
    as a DAW): the site's style, header and the script inlined, at editor/"""
    page = (APP / "site" / "editor.html").read_text(encoding="utf-8")
    css = (APP / "site" / "site.css").read_text(encoding="utf-8")
    js = (APP / "site" / "editor.js").read_text(encoding="utf-8")   # (0.9.43: the Studio's script, its own file)
    if "/*SITE_CSS*/" not in page or "<!--HEADER-->" not in page or "/*SCRIPT*/" not in page:
        raise SystemExit("app/site/editor.html: no /*SITE_CSS*/, <!--HEADER--> or /*SCRIPT*/ to fill")
    page = page.replace("<style>/*SITE_CSS*/", FONTS + "<style>" + css, 1).replace("<!--HEADER-->", header("Studio"), 1)
    page = page.replace("/*SCRIPT*/", js.replace("</script", "<\\/script"), 1)
    (site / "editor").mkdir(parents=True, exist_ok=True)
    (site / "editor" / "index.html").write_text(page, encoding="utf-8")


def page_of(site, name, current):
    """LOAF BREAKER (0.9.71): a page of app/site/NAME.html at NAME/ (app.html: app/), the site's style and header"""
    page = (APP / "site" / f"{name}.html").read_text(encoding="utf-8")
    css = (APP / "site" / "site.css").read_text(encoding="utf-8")
    if "/*SITE_CSS*/" not in page or "<!--HEADER-->" not in page:
        raise SystemExit(f"app/site/{name}.html: no /*SITE_CSS*/ or <!--HEADER--> to fill")
    page = page.replace("<style>/*SITE_CSS*/", FONTS + "<style>" + css, 1).replace("<!--HEADER-->", header(current), 1)
    (site / name).mkdir(parents=True, exist_ok=True)
    (site / name / "index.html").write_text(page, encoding="utf-8")


def pwa(site):
    """LOAF BREAKER (0.9.71, FLEXOR: an Android app): the site installable as an app: its manifest and service worker at
    the root (their scope the whole site), every page linking the manifest, the theme's colour, the worker registered"""
    for f in ("manifest.webmanifest", "sw.js"):
        (site / f).write_text((APP / "site" / f).read_text(encoding="utf-8"), encoding="utf-8")
    for f in site.rglob("index.html"):
        up = "../" * (len(f.relative_to(site).parts) - 1)
        s = f.read_text(encoding="utf-8")
        if 'rel="manifest"' in s:
            continue
        reg = "" if "serviceWorker" in s else (f'<script>if ("serviceWorker" in navigator) navigator.serviceWorker'
                                               f'.register("{up}sw.js").catch(function () {{}});</script>\n')
        s = s.replace("</head>", f'<link rel="manifest" href="{up}manifest.webmanifest">\n'
                                 f'<meta name="theme-color" content="#0e0d0b">\n'
                                 f'<meta name="mobile-web-app-capable" content="yes">\n' + reg + "</head>", 1)
        f.write_text(s, encoding="utf-8")


def main(site):
    site = Path(site)
    rebrand(site)
    install(site)
    atelier(site)
    emu(site)
    breaks(site)
    imp(site)
    editor(site)
    page_of(site, "live", "Live")        # (0.9.71: the FM-1 in the hand, the app's)
    page_of(site, "app", "App")
    page_of(site, "vst", "VST")          # (24 Nov 2026, FLEXOR: "un onglet VST sur le site": the plugin, its pictures)
    for f in (APP / "site" / "vst_img").glob("*.png"):
        (site / "vst" / f.name).write_bytes(f.read_bytes())
    home = site / "index.html"
    if home.exists():
        home.write_text(restyle(home, "home", "manual/#controls"), encoding="utf-8")
    man = site / "manual" / "index.html"
    if not man.exists():
        pwa(site)
        print(f"site_style: {site}: front page restyled (no manual)")
        return
    s = restyle(man, "manual", "#controls")
    h = re.search(r'<h2 id="(s\d+)"><span class="num">\d+</span>The control map</h2>', s)
    if not h:
        raise SystemExit(f"{man}: no section \"The control map\" (app/docs/MANUAL.md)")
    end = re.compile(r"<h2 |</main>").search(s, h.end())
    widget = (APP / "site" / "controlmap.html").read_text(encoding="utf-8")
    widget = re.sub(r"<!--.*?-->\s*", "", widget, count=1, flags=re.S)
    s = (s[:h.start()] + '<span id="controls"></span>' + s[h.start():h.end()] + "\n" + widget + "\n" + s[end.start():])
    s = s.replace("</body>", SPY + "</body>", 1)
    man.write_text(s, encoding="utf-8")
    logo(site)
    pwa(site)
    print(f"site_style: {site}: the front page and the manual restyled, the control map in section {h.group(1)}")


if __name__ == "__main__":
    if len(sys.argv) != 2:
        raise SystemExit(__doc__)
    main(sys.argv[1])
