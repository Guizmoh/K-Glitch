#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""K-GLITCH: the installer in one file, the firmware inside it (no server, no internet).

  tools/make_installer.py build/felucca-X.Y.fwsc X.Y OUT.html

web/index_pkg.html (SLOOP's web installer: Web MIDI, the same update as tools/fm1_install.py) with fm1pkg.js and
fm1ota.js inlined and the package itself inlined as a data: URL (its SHA-256 checked by the page before it is
offered). Opened from the disk in Chrome or Edge (file:// is a secure context: Web MIDI asks for permission), it
installs K-GLITCH on an FM-1 plugged in by USB; it can also put the official M-VAVE firmware back.
"""
import base64
import hashlib
import json
import re
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
WEB = HERE.parent / "web"
sys.path.insert(0, str(WEB))
from make_site import product_of, strip_module   # noqa: E402  (the site's own helpers: one way to read a package)


FR = """  fr: {
    intro: "Branchez le FM-1 en USB, puis cliquez sur Installer. (Chrome ou Edge, sur ordinateur.)",
    warn: "Ne débranchez pas le câble USB pendant l'écriture (environ une minute).",
    install: "Installer", other: "English",
    editor: "", edOffline: "", appInstall: "",
    direct: "Branchez le câble USB directement sur l'ordinateur (un câble qui transporte les données). Aucun autre matériel n'est nécessaire.",
    brick: "Si une installation échoue et que le FM-1 ne démarre plus, il faut un Transporter pour le récupérer :",
    beta: "K-GLITCH est une version de développement : il peut être instable. Son auteur décline toute responsabilité si le FM-1 est endommagé ou se comporte mal à cause de ce firmware. À vos risques.",
    nomidi: "Ce navigateur ne peut pas le faire. Ouvrez ce fichier dans Chrome ou Edge.", loadfail: "Lecture impossible : ",
    start: "Démarrage", verify: "Vérification", loader: "Passage en mode mise à jour", write: "Écriture",
    reboot: "Redémarrage", done: "Terminé : K-GLITCH est installé", stopped: "Arrêté. Rebranchez le câble USB et cliquez à nouveau sur Installer.",
    error: "Erreur : ",
    denied: "L'accès MIDI (SysEx) n'a pas été autorisé. Autorisez MIDI dans les paramètres du site (barre d'adresse) et cliquez à nouveau sur Installer.",
    notfound: "FM-1 introuvable. Vérifiez le câble USB (un câble de données), fermez les autres applications qui utilisent le MIDI, puis cliquez à nouveau sur Installer.",
    model: "L'appareil branché n'est pas un FM-1.",
    lost: "La connexion USB a été perdue. Rebranchez-la et cliquez à nouveau sur Installer.",
    noloader: "Le FM-1 en mode mise à jour est introuvable. Rebranchez le câble USB et cliquez à nouveau sur Installer (il reste en mode mise à jour tant que ce n'est pas fini).",
    noreturn: "Le FM-1 n'est pas revenu. Éteignez-le et rallumez-le.",
    mismatch: "Écrit, mais le FM-1 annonce une autre version : ",
    foreign: "Le FM-1 est en mode mise à jour d'un autre firmware. Terminez cette mise à jour avec son propre outil (M-UPGRADE), puis cliquez à nouveau sur Installer : ",
    badpkg: "Le fichier à installer est abîmé. Téléchargez à nouveau l'installateur.",
    leave: "Écriture en cours. Ne fermez pas la page.",
    license: "GPL-3.0, sans garantie. Sources :",
    stockTitle: "Revenir au firmware officiel (V15)",
    stockHelp: "Téléchargez le FM-1 V15 officiel sur le site de M-VAVE et choisissez le fichier FM-1.fwsc qu'il contient, non modifié. Le fichier est vérifié dans ce navigateur ; il n'est envoyé nulle part.",
    stockBackup: "Le firmware officiel ne peut pas ouvrir les morceaux, projets ou samples de SLOOP / K-GLITCH. Faites-en une sauvegarde avant si vous y tenez.",
    stockEditor: "", stockSaved: "J'ai fait une sauvegarde (ou je n'en ai pas besoin)",
    stockFile: "Le FM-1.fwsc officiel", stockGo: "Installer le V15 officiel",
    stockValid: "V15 officiel vérifié", stockBad: "Ce n'est pas le V15 officiel : ",
    stockConfirm: "Installer le V15 officiel sur le FM-1 ? Le firmware officiel ne peut pas ouvrir la musique de K-GLITCH. Avez-vous fait une sauvegarde ?",
    stockFinish: "Le FM-1 est en mode mise à jour de K-GLITCH. Cliquez d'abord sur Installer pour terminer, puis revenez au V15 officiel.",
    back: "Pour revenir au firmware officiel : la section ci-dessous (avec le FM-1.fwsc officiel), ou l'outil de M-VAVE, M-UPGRADE (Downloads → PC Software → M-UPGRADE), avec le firmware officiel (PC Firmware → FM-1).",
  },
"""
PAGE = [
    ("<h1>Firmware for the M-VAVE FM-1</h1>", "<h1>Firmware glitch pour le M-VAVE FM-1</h1>"),
    ('<p class="lead">A groovebox you play live: four tracks — three synths and a drum machine — 12 synth engines (with six-operator FM and DX7 patches), 153 sounds, 37 drum kits from 808 to techno, amapiano and ambient, your own samples and drum kits, 27 punch-in effects. House, techno, hip-hop, drum &amp; bass, synthwave, lo-fi, ambient: make what you want. No factory patterns, nothing to load: everything you hear, you play.</p>',
     '<p class="lead">K-GLITCH : un destructeur de rythmes, construit sur SLOOP 2.8.4. 16 pads d\'effets en trois banques (DESTROY, BEAT, SLOOP), leur séquence en automation, un sampler de boucle (slices, grains), 11 filtres créatifs, des batteries FM et C64 éditables CHAOS et DICE. Tout SLOOP reste là.</p>'),
    ('<div class="tile" style="--c: var(--blue)"><b>153</b><span>sounds, 12 engines, your samples</span></div>',
     '<div class="tile" style="--c: var(--blue)"><b>48</b><span>effets : DESTROY, BEAT, SLOOP</span></div>'),
    ('<div class="tile" style="--c: var(--green)"><b>37</b><span>drum kits, 909 to afro, + your own</span></div>',
     '<div class="tile" style="--c: var(--green)"><b>FM C64</b><span>batteries éditables, SAT, DLY, REV</span></div>'),
    ('<div class="tile" style="--c: var(--yellow)"><b>27</b><span>punch-in fx: hold fx + key</span></div>',
     '<div class="tile" style="--c: var(--yellow)"><b>11</b><span>filtres : peigne, métal, voyelle…</span></div>'),
    ('<div class="tile" style="--c: var(--orange)"><b>rec</b><span>no click: the loop follows you</span></div>',
     '<div class="tile" style="--c: var(--orange)"><b>loop</b><span>sampler : slices, grains, scratch</span></div>'),
    ('  <a class="ed" href="../editor/" data-t="editor"></a>\n', ''),
    ('  <p class="small"><a href="../editor/sloop-editor.html" download="sloop-editor.html" data-t="edOffline"></a></p>\n', ''),
    ('<p class="small">SLOOP is based on Felucca', '<p class="small">K-GLITCH (GPL-3.0) is based on SLOOP 2.8.4 by isod89 (github.com/isod89/sloop-fm1); its font after Carlito (SIL OFL 1.1). SLOOP is based on Felucca'),
    ("const TEXT = {\n", "const TEXT = {\n" + FR),
    ('install: "インストール", other: "English",', 'install: "インストール", other: "Français",'),
    ('let lang = (navigator.language || "en").toLowerCase().startsWith("ja") ? "ja" : "en";',
     'let lang = (navigator.language || "fr").toLowerCase().startsWith("ja") ? "ja" : (navigator.language || "fr").toLowerCase().startsWith("en") ? "en" : "fr";'),
    ('$("lang").addEventListener("click", () => { lang = lang === "ja" ? "en" : "ja"; applyLang(); });',
     '$("lang").addEventListener("click", () => { lang = lang === "fr" ? "en" : lang === "en" ? "ja" : "fr"; applyLang(); });'),
]


def main(pkg, version, out):
    raw = Path(pkg).read_bytes()
    product = product_of(raw)
    if not re.fullmatch(r"FM-1_9\d\d", product):
        raise SystemExit(f"{pkg}: identity {product!r} is not a Felucca package (FM-1_9xx)")
    if b"FELUCCA-LOADER-1" not in raw:
        raise SystemExit(f"{pkg}: no update loader in it")
    html = (WEB / "index_pkg.html").read_text(encoding="utf-8")
    lib = strip_module((WEB / "fm1pkg.js").read_text(encoding="utf-8")) + "\n" + \
        strip_module((WEB / "fm1ota.js").read_text(encoding="utf-8"))
    meta = json.dumps({"version": version, "product": product,
                       "pkg": "data:application/octet-stream;base64," + base64.b64encode(raw).decode(),
                       "sha256": hashlib.sha256(raw).hexdigest()})
    for mark in ("/*LIB*/", "/*META*/", "<!--LOGO-->"):
        if html.count(mark) != 1:
            raise SystemExit(f"index_pkg.html must contain {mark} once; update make_installer.py")
    html = html.replace("/*LIB*/", lib).replace("/*META*/", meta)
    html = html.replace("<!--LOGO-->", '<b style="font-size:28px;letter-spacing:.02em">K-<span style="color:#ff8838">'
                                       'GLITCH</span></b>')
    html = html.replace("<!--STUDIO_GUIDE-->", "").replace("<!--BANNER-->", "")
    for old, new in PAGE:                          # K-GLITCH's words in SLOOP's page
        if html.count(old) != 1:
            raise SystemExit(f"index_pkg.html: {old[:40]!r} not found once; update make_installer.py")
        html = html.replace(old, new)
    html = re.sub(r"<title>[^<]*</title>", f"<title>K-GLITCH {version} installer</title>", html, count=1)
    n = html.count("say(`SLOOP ${meta.version}`);")
    if n != 1:
        raise SystemExit("index_pkg.html: the ready line changed; update make_installer.py")
    html = html.replace("say(`SLOOP ${meta.version}`);", "say(`K-GLITCH ${meta.version} (SLOOP 2.8.4)`);")
    Path(out).write_text(html, encoding="utf-8")
    print(f"installer: {out} ({len(html)} B), K-GLITCH {version}, {product}, sha256 {hashlib.sha256(raw).hexdigest()[:16]}")


if __name__ == "__main__":
    if len(sys.argv) != 4:
        sys.exit(__doc__)
    main(*sys.argv[1:])
