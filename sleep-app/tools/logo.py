#!/usr/bin/env python3
"""Source unique du logo « Police du Dodo ».

Génère :
  - app/src/main/res/drawable/ic_launcher_foreground.xml (icône, yeux ouverts)
  - app/src/main/res/drawable/ic_launcher_background.xml
  - app/src/main/java/com/somnia/app/ui/components/LogoArt.kt (formes pour le logo animé)
  - un aperçu SVG des deux états (option --preview DOSSIER)

Repère : carré de 108 x 108 (format des icônes adaptatives Android).
"""
import os, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# Dégradés linéaires : (x1, y1, x2, y2, couleur début, couleur fin)
G = {
    "uniform": (54, 80, 54, 108, "#FF26337F", "#FF121845"),
    "skin": (54, 40, 54, 76, "#FFFBDDC4", "#FFF0C29E"),
    "crown": (54, 24, 54, 47, "#FF34459F", "#FF161E58"),
    "visor": (54, 48, 54, 56, "#FF262C52", "#FF05070F"),
    "gold": (54, 31, 54, 44, "#FFF8DC8A", "#FFC48F2A"),
}

CAP_DY = -3.5  # la casquette est remontée pour dégager les sourcils
CAP = {"crown", "crown_shine", "band", "band_cord", "visor", "visor_shine", "badge", "badge_moon"}

# Calques dessinés dans l'ordre : (nom, tracé, remplissage, contour, épaisseur)
# remplissage = couleur ARGB, "grad:<nom>" ou None.
LAYERS = [
    ("uniform", "M16,108 L16,101 Q18,87 37,82.5 L46,80 L54,90 L62,80 L71,82.5 Q90,87 92,101 L92,108 Z", "grad:uniform", None, 0),
    ("shirt", "M46,80 L54,90 L62,80 L60.5,76 L47.5,76 Z", "#FFD6E4F7", None, 0),
    ("neck", "M47.5,70 L60.5,70 L60.5,78.5 Q54,82.5 47.5,78.5 Z", "#FFE3B48F", None, 0),
    ("tie_knot", "M51.6,80.6 L56.4,80.6 L55.5,84.6 L52.5,84.6 Z", "#FF0D1230", None, 0),
    ("tie", "M52.4,84.4 L55.6,84.4 L57,96 L54,99.5 L51,96 Z", "#FF0D1230", None, 0),
    ("collar_pin_l", "M41.5,87m-1.4,0a1.4,1.4 0,1 1,2.8 0a1.4,1.4 0,1 1,-2.8 0", "grad:gold", None, 0),
    ("collar_pin_r", "M66.5,87m-1.4,0a1.4,1.4 0,1 1,2.8 0a1.4,1.4 0,1 1,-2.8 0", "grad:gold", None, 0),
    ("ear_l", "M38.5,58m-3,0a3,4.5 0,1 1,6 0a3,4.5 0,1 1,-6 0", "#FFEDBD98", None, 0),
    ("ear_r", "M69.5,58m-3,0a3,4.5 0,1 1,6 0a3,4.5 0,1 1,-6 0", "#FFEDBD98", None, 0),
    ("face", "M39,52 Q39,74.5 54,75.5 Q69,74.5 69,52 Q69,40 54,40 Q39,40 39,52 Z", "grad:skin", None, 0),
    ("hair_l", "M39.2,49 L42.4,49 L42.4,57.5 Q40.2,55.8 39.3,53.5 Z", "#FF4A3426", None, 0),
    ("hair_r", "M68.8,49 L65.6,49 L65.6,57.5 Q67.8,55.8 68.7,53.5 Z", "#FF4A3426", None, 0),
    ("cheek_l", "M44.5,65.2m-2.6,0a2.6,1.6 0,1 1,5.2 0a2.6,1.6 0,1 1,-5.2 0", "#4DFF9E8C", None, 0),
    ("cheek_r", "M63.5,65.2m-2.6,0a2.6,1.6 0,1 1,5.2 0a2.6,1.6 0,1 1,-5.2 0", "#4DFF9E8C", None, 0),
    ("brow_l", "M44.6,55 Q47.5,53.4 50.4,54.8", None, "#FF4A3426", 1.6),
    ("brow_r", "M57.6,54.8 Q60.5,53.4 63.4,55", None, "#FF4A3426", 1.6),
    ("nose", "M54.3,59.6 Q52.6,63.6 54.8,64.3", None, "#FFD49A76", 1.2),
    ("mustache", "M54,66 Q50.2,64 46.6,66.5 Q49.5,68 54,67.1 Q58.5,68 61.4,66.5 Q57.8,64 54,66 Z", "#FF4A3426", None, 0),
    ("mouth", "M51.9,70.1 Q54,71.4 56.1,70.1", None, "#FFB5776A", 1.1),
    ("crown", "M31,38 Q32,27 54,24 Q76,27 77,38 Q74.5,43.5 70,46.5 L38,46.5 Q33.5,43.5 31,38 Z", "grad:crown", None, 0),
    ("crown_shine", "M37,31.5 Q54,25.8 71,31.5 Q54,28.6 37,31.5 Z", "#26FFFFFF", None, 0),
    ("band", "M38,45 L70,45 L70,50.6 L38,50.6 Z", "#FF0D1230", None, 0),
    ("band_cord", "M38.5,46.4 L69.5,46.4", None, "#FFE0B656", 0.9),
    ("visor", "M36.5,50 Q54,47.4 71.5,50 Q72,53.6 66,55.2 Q54,57.6 42,55.2 Q36,53.6 36.5,50 Z", "grad:visor", None, 0),
    ("visor_shine", "M42.5,51.6 Q54,50 65.5,51.6", None, "#40FFFFFF", 0.7),
    ("badge", "M54,30.8 L60.2,33 Q60.2,40.6 54,43.8 Q47.8,40.6 47.8,33 Z", "grad:gold", None, 0),
    ("badge_moon", "M55.3,34.4 a3,3 0,1 0,0.9 5.6 a2.3,2.3 0,1 1,-0.9 -5.6 z", "#FF1C2766", None, 0),
]

# Yeux (dessinés à part pour pouvoir les ouvrir / fermer)
EYES = {"left": (47.5, 58.6), "right": (60.5, 58.6), "rx": 2.7, "ry": 2.3, "iris": 1.65}
EYE_WHITE, IRIS, LID = "#FFFFFFFF", "#FF27304F", "#FF4A3426"
ZZZ = ["M71,21 L75.5,21 L71,25.5 L75.5,25.5", "M77.5,13.5 L80.8,13.5 L77.5,16.8 L80.8,16.8"]


def eye_paths(open_):
    out = []
    for cx, cy in (EYES["left"], EYES["right"]):
        rx, ry = EYES["rx"], EYES["ry"]
        if open_:
            out.append((f"M{cx},{cy}m-{rx},0a{rx},{ry} 0,1 1,{2*rx} 0a{rx},{ry} 0,1 1,-{2*rx} 0", EYE_WHITE, None, 0))
            r = EYES["iris"]
            out.append((f"M{cx},{cy+0.2}m-{r},0a{r},{r} 0,1 1,{2*r} 0a{r},{r} 0,1 1,-{2*r} 0", IRIS, None, 0))
            out.append((f"M{cx+0.55},{cy-0.45}m-0.5,0a0.5,0.5 0,1 1,1 0a0.5,0.5 0,1 1,-1 0", EYE_WHITE, None, 0))
            out.append((f"M{cx-rx-0.2},{cy-0.3} Q{cx},{cy-ry-1.3} {cx+rx+0.2},{cy-0.3}", None, LID, 1.1))
        else:
            out.append((f"M{cx-rx},{cy} Q{cx},{cy+2.4} {cx+rx},{cy}", None, LID, 1.5))
    return out


def argb_to_css(c):
    c = c.lstrip("#")
    a = int(c[:2], 16) / 255
    return f"rgba({int(c[2:4],16)},{int(c[4:6],16)},{int(c[6:],16)},{a:.3f})"


def svg(open_, path):
    p = ['<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 108 108" width="432" height="432"><defs>']
    for k, (x1, y1, x2, y2, a, b) in G.items():
        p.append(f'<linearGradient id="{k}" gradientUnits="userSpaceOnUse" x1="{x1}" y1="{y1}" x2="{x2}" y2="{y2}"><stop offset="0" stop-color="{argb_to_css(a)}"/><stop offset="1" stop-color="{argb_to_css(b)}"/></linearGradient>')
    p.append('<radialGradient id="bg" cx="0.5" cy="0.4" r="0.7"><stop offset="0" stop-color="#3A4DB0"/><stop offset="1" stop-color="#0C1135"/></radialGradient>')
    p.append('<clipPath id="c"><circle cx="54" cy="54" r="36"/></clipPath></defs><g clip-path="url(#c)">')
    p.append('<rect width="108" height="108" fill="url(#bg)"/>')
    p.append('<circle cx="54" cy="54" r="34.5" fill="none" stroke="rgba(224,182,86,0.75)" stroke-width="1.1"/>')
    items = [(d, f, s, w, CAP_DY if n in CAP else 0) for n, d, f, s, w in LAYERS] + [(*e, 0) for e in eye_paths(open_)]
    if not open_:
        items += [(z, None, "#E6FFFFFF", 1.6, 0) for z in ZZZ]
    for d, f, s, w, dy in items:
        fill = "none" if f is None else (f"url(#{f[5:]})" if f.startswith("grad:") else argb_to_css(f))
        stroke = "none" if s is None else argb_to_css(s)
        p.append(f'<path transform="translate(0,{dy})" d="{d}" fill="{fill}" stroke="{stroke}" stroke-width="{w}" stroke-linecap="round" stroke-linejoin="round"/>')
    p.append("</g></svg>")
    open(path, "w").write("\n".join(p))


def vector_xml():
    out = ['<?xml version="1.0" encoding="utf-8"?>',
           '<!-- Généré par tools/logo.py : ne pas modifier à la main. -->',
           '<vector xmlns:android="http://schemas.android.com/apk/res/android"',
           '    xmlns:aapt="http://schemas.android.com/aapt"',
           '    android:width="108dp" android:height="108dp"',
           '    android:viewportWidth="108" android:viewportHeight="108">']
    items = [(d, f, s, w, n in CAP) for n, d, f, s, w in LAYERS] + [(*e, False) for e in eye_paths(True)]
    group_open = False
    for d, f, s, w, in_cap in items:
        if in_cap and not group_open:
            out.append(f'    <group android:translateY="{CAP_DY}">'); group_open = True
        if not in_cap and group_open:
            out.append('    </group>'); group_open = False
        attrs = [f'android:pathData="{d}"']
        grad = None
        if f is None:
            attrs.append('android:fillColor="#00000000"')
        elif f.startswith("grad:"):
            grad = G[f[5:]]
        else:
            attrs.append(f'android:fillColor="{f}"')
        if s is not None:
            attrs += [f'android:strokeColor="{s}"', f'android:strokeWidth="{w}"',
                      'android:strokeLineCap="round"', 'android:strokeLineJoin="round"']
        if grad:
            x1, y1, x2, y2, a, b = grad
            out.append(f'    <path {" ".join(attrs)}>')
            out.append('        <aapt:attr name="android:fillColor">')
            out.append(f'            <gradient android:type="linear" android:startX="{x1}" android:startY="{y1}" android:endX="{x2}" android:endY="{y2}" android:startColor="{a}" android:endColor="{b}" />')
            out.append('        </aapt:attr>')
            out.append('    </path>')
        else:
            out.append(f'    <path {" ".join(attrs)} />')
    if group_open:
        out.append('    </group>')
    out.append('</vector>')
    return "\n".join(out) + "\n"


BACKGROUND = '''<?xml version="1.0" encoding="utf-8"?>
<!-- Généré par tools/logo.py : ne pas modifier à la main. -->
<vector xmlns:android="http://schemas.android.com/apk/res/android"
    xmlns:aapt="http://schemas.android.com/aapt"
    android:width="108dp" android:height="108dp"
    android:viewportWidth="108" android:viewportHeight="108">
    <path android:pathData="M0,0h108v108h-108z">
        <aapt:attr name="android:fillColor">
            <gradient android:type="radial" android:centerX="54" android:centerY="43" android:gradientRadius="76"
                android:startColor="#FF3A4DB0" android:endColor="#FF0C1135" />
        </aapt:attr>
    </path>
    <path android:pathData="M54,54m-34.5,0a34.5,34.5 0,1 1,69 0a34.5,34.5 0,1 1,-69 0"
        android:fillColor="#00000000" android:strokeColor="#BFE0B656" android:strokeWidth="1.1" />
</vector>
'''


def kotlin():
    def kc(c):
        return f"Color(0x{c.lstrip('#')})"
    lines = ['package com.somnia.app.ui.components', '',
             '// Généré par tools/logo.py : ne pas modifier à la main.', '',
             'import androidx.compose.ui.graphics.Color', '',
             '/** Formes du logo « Police du Dodo » (repère 108 x 108). */',
             'internal object LogoArt {',
             '    class Grad(val x1: Float, val y1: Float, val x2: Float, val y2: Float, val from: Color, val to: Color)',
             '    class Layer(val path: String, val fill: Color? = null, val grad: Grad? = null, val stroke: Color? = null, val width: Float = 0f, val dy: Float = 0f)', '']
    for k, (x1, y1, x2, y2, a, b) in G.items():
        lines.append(f'    private val G_{k.upper()} = Grad({x1}f, {y1}f, {x2}f, {y2}f, {kc(a)}, {kc(b)})')
    lines += ['', '    val layers = listOf(']
    for name, d, f, s, w in LAYERS:
        args = [f'"{d}"']
        if f is not None:
            args.append(f'grad = G_{f[5:].upper()}' if f.startswith("grad:") else f'fill = {kc(f)}')
        if s is not None:
            args += [f'stroke = {kc(s)}', f'width = {w}f']
        if name in CAP:
            args.append(f'dy = {CAP_DY}f')
        lines.append(f'        Layer({", ".join(args)}), // {name}')
    lines += ['    )', '']
    (lx, ly), (rx_, ry_) = EYES["left"], EYES["right"]
    lines += [f'    val eyeCenters = listOf({lx}f to {ly}f, {rx_}f to {ry_}f)',
              f'    const val EYE_RX = {EYES["rx"]}f', f'    const val EYE_RY = {EYES["ry"]}f', f'    const val IRIS_R = {EYES["iris"]}f',
              f'    val eyeWhite = {kc(EYE_WHITE)}', f'    val iris = {kc(IRIS)}', f'    val lid = {kc(LID)}',
              '    val zzz = listOf(' + ", ".join(f'"{z}"' for z in ZZZ) + ')', '}']
    return "\n".join(lines) + "\n"


if __name__ == "__main__":
    res = os.path.join(ROOT, "app/src/main/res/drawable")
    open(os.path.join(res, "ic_launcher_foreground.xml"), "w").write(vector_xml())
    open(os.path.join(res, "ic_launcher_background.xml"), "w").write(BACKGROUND)
    open(os.path.join(ROOT, "app/src/main/java/com/somnia/app/ui/components/LogoArt.kt"), "w").write(kotlin())
    if "--preview" in sys.argv:
        d = sys.argv[sys.argv.index("--preview") + 1]
        svg(True, os.path.join(d, "logo_awake.svg"))
        svg(False, os.path.join(d, "logo_asleep.svg"))
    print("logo généré")
