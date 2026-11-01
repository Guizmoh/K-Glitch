# K-GLITCH

**DRUM DESTROYER pour le M-VAVE FM-1** — un firmware de glitch et de déconstruction rythmique (breakcore, jungle, IDM, drill, glitch), construit sur **[SLOOP](https://github.com/isod89/sloop-fm1) 2.8.4** (isod89), lui-même basé sur **Felucca** (Leo Kuroshita / Hügelton Instruments). GPL-3.0-only.

> État : version de développement **0.4**. Testé sur ordinateur (tests hôtes + émulateur web). **Jamais compilé pour le FM-1 ni essayé sur l'appareil** : ne pas l'installer.

## Essayer : l'émulateur

Ouvrir [`emulator/index.html`](emulator/index.html) dans un navigateur (une seule page, fonctionne hors ligne). Le firmware y tourne réellement (son, écran, séquenceur, effets) en WebAssembly. Un break jungle à 170 BPM est chargé : **PLAY** (ou Espace), puis **FX**.

Reconstruire : `cd source && python3 tools/build_emulator.py` (clang avec la cible wasm32 et wasm-ld, Python 3 + Pillow).

## Ce que K-GLITCH ajoute à SLOOP

Tout SLOOP est conservé (4 pistes, 12 moteurs, kits, samples, séquenceur, 27 effets punch-in, MIDI…). Le bouton **FX** ouvre K-GLITCH : sept pages dessinées en plein écran dans le style « Ribbon classique » de K-WASUB (SELECT passe de l'une à l'autre).

| Page | Rôle | K1 | K2 | K3 | K4 |
| --- | --- | --- | --- | --- | --- |
| **DESTROY** | écran principal : les 16 pads | CHAOS | DENSITY | MOVEMENT | CLEAR (retour au son original) |
| **DRUMS** | mode 1 : génération de percussions | STYLE | VARIATION | SWING | GO |
| **CHOP** | mode 2 : découpage / mutation du rythme | DICE | MUTATE | RESTORE | EVERY (auto) |
| **SEQ** | séquenceur / automation d'effets | le pas | son effet | longueur OFF / 16 / 32 / 64 | DICE |
| **BANK** | les pads | banque DESTROY / SLOOP | INTENSITÉ | HOLD | LFO |
| **CHAOS** | mode 3 : performance imprévisible | MODE | AUTO | LFO | CHAOS |
| **SETUP** | MPC | OUT : FM-1 / MPC / BOTH | canal MIDI | effacer la séquence | CHOP : DRUMS / ALL |

### Jouer en direct
- Sur les pages K-GLITCH, **les 16 touches blanches sont des pads** : maintenir = l'effet agit, relâcher = le son revient (punch-in). Ailleurs : FX maintenu + touche.
- **ALGO** (encodeur A) choisit l'effet, **PRESETS** (encodeur B) règle son **intensité** — chaque effet a la sienne. Un pad tenu + PRESETS : l'intensité de ce pad.
- **Automation** : pendant l'enregistrement (**REC** + PLAY), chaque effet joué sur un pad — les 16 DRUM DESTROYER comme les 27 de SLOOP — s'écrit sur ses 16es dans la séquence d'effets, **avec son intensité**, et se rejoue ensuite. Sur la page SEQ, pendant REC, les touches redeviennent des pads (on voit la piste se remplir).
- **CLEAR (OCT- sur les pages K-GLITCH)** : maintenu pendant la lecture ou l'enregistrement, il efface les effets des pas qui passent (comme sur le PO-33) ; maintenu 1 s à l'arrêt, il efface toute l'automation.
- **HOLD** verrouille un effet ; **CLEAR** coupe tout (effets, AUTO, CHAOS, séquence) et rend le rythme original.
- **LFO** : module l'intensité des effets ; **MOVEMENT** règle sa vitesse (4 mesures à 1 temps) et sa profondeur.

### La banque DRUM DESTROYER (16 pads)
LOOP DESTROY, STUTTER X, GLITCH, SCRATCH, REVERSE X, PITCH MAD, GATE X, FILTER CHAOS, BEAT REPEAT, BEAT SKIP, MICRO STUTTER, TIME STRETCH, RING MOD, FUZZ, RANDOMIZER, TOTAL CHAOS. L'intensité change leur comportement (ex. LOOP : de 1/2 à 1/32 de mesure ; STUTTER : 2x, 3x, 4x, 8x ; PITCH : escalier, sauts, plongée). Les touches noires gardent les effets de SLOOP.

### Les trois modes
- **DRUMS** : écrit 2 mesures dans un style (JUNGLE 170, BREAKCORE 190, DNB 174, DRILL 142, IDM 128, GLITCH 120, HALFTIME 160, FOOTWORK 160) avec son tempo et un kit adapté ; VARIATION le varie aussitôt. Ce motif devient l'**original**.
- **CHOP** : **DICE** tire une recette au hasard (2 à 5 transformations parmi couper, décaler, accentuer, répéter, roll, ghost, longueur, chacune à une force aléatoire) et l'affiche ; MUTATE transforme le rythme à partir de l'original (jamais d'une mutation précédente) : **décale** des coups (pas voisin ou micro-timing), en **supprime**, **répète** des fragments, ajoute des **rolls** et des **ghost notes**, change des **accents** et, avec TARGET ALL, les **longueurs de notes** des synthés. CHAOS dose la destruction, DENSITY les répétitions et découpes. EVERY remute seul toutes les 1, 2, 4 ou 8 mesures. La grille montre en orange ce qui a changé.
- **CHAOS** : AUTO + LFO + un DICE du rythme (et parfois de la séquence d'effets) à chaque mesure, avec une probabilité de CHAOS %.

### Séquenceur d'effets, AUTO, MPC
- **SEQ** : un effet (ou rien) par pas, sur 16, 32 ou 64 pas ; une touche blanche choisit le pas (sur la page de 16 affichée) ; PRESETS règle l'intensité du pas ; DICE écrit une séquence selon CHAOS / DENSITY.
- **AUTO** : à chaque 16e, DENSITY % de chances de lancer un effet choisi selon CHAOS.
- **MPC** : les effets partent en MIDI CC (voir [docs/MPC_REMOTE.md](docs/MPC_REMOTE.md)). Avec GLO > SYSTEM > MIDI OUT sur SEQ, la MPC reçoit aussi le rythme muté en notes.

Priorité : pad > séquenceur > AUTO. Ces réglages sont « live » : pas encore sauvegardés dans les projets.

## Code

- `source/firmware/src/kglitch.c` : le moteur (séquenceur d'effets, AUTO, MUTATE, sortie MPC, break de démo).
- `source/firmware/src/kg_dd.c` : la banque DRUM DESTROYER (16 effets).
- `source/firmware/src/kg_rhythm.c` : DRUMS (générateur par style) et CHOP (mutations du rythme).
- `source/firmware/src/kg_ui.c` + `kg_font5.h` : les écrans K-GLITCH.
- `source/firmware/host/emulator.c`, `source/web/emulator.html`, `source/tools/build_emulator.py` : l'émulateur.
- Tests : `source/tests/kglitch_test.c`, et `source/tests/ui_pages_test.c` (captures des écrans).

Voir aussi [docs/ROADMAP.md](docs/ROADMAP.md). Les crédits et licences de SLOOP / Felucca sont dans `source/LICENSING.md` et `source/LICENSES/` ; les icônes et l'image du panneau de Felucca restent sous leurs propres conditions (voir `source/LICENSING.md`).

Projet non affilié à M-VAVE, Hügelton Instruments ni isod89.
