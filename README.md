# K-GLITCH

**DRUM DESTROYER pour le M-VAVE FM-1** — un firmware de glitch et de déconstruction rythmique (breakcore, jungle, IDM, drill, glitch), construit sur **[SLOOP](https://github.com/isod89/sloop-fm1) 2.8.4** (isod89), lui-même basé sur **Felucca** (Leo Kuroshita / Hügelton Instruments). GPL-3.0-only.

> État : version de développement **0.4**. Testé sur ordinateur (tests hôtes + émulateur web). **Jamais compilé pour le FM-1 ni essayé sur l'appareil** : ne pas l'installer.

## Essayer : l'émulateur

Ouvrir [`emulator/index.html`](emulator/index.html) dans un navigateur (une seule page, fonctionne hors ligne). Le firmware y tourne réellement (son, écran, séquenceur, effets) en WebAssembly. Un break jungle à 170 BPM est chargé : **PLAY** (ou Espace), puis **FX**.

Reconstruire : `cd source && python3 tools/build_emulator.py` (clang avec la cible wasm32 et wasm-ld, Python 3 + Pillow).

## Ce que K-GLITCH ajoute à SLOOP

Tout SLOOP est conservé (4 pistes, 12 moteurs, kits, samples, séquenceur, 27 effets punch-in, MIDI…). Le bouton **FX** ouvre K-GLITCH : dix pages dessinées en plein écran dans le style « Ribbon classique » de K-WASUB (SELECT passe de l'une à l'autre).

| Page | Rôle | K1 | K2 | K3 | K4 |
| --- | --- | --- | --- | --- | --- |
| **DESTROY** | écran principal : les 16 pads | CHAOS | DENSITY | MOVEMENT | CLEAR (retour au son original) |
| **DRUMS** | mode 1 : génération de percussions | STYLE | VARIATION | SWING | GO |
| **CHOP** | mode 2 : découpage / mutation du rythme | DICE | MUTATE | RESTORE | EVERY (auto) |
| **LOOP** | sampler : enregistre ce qui joue | REC | BARS 1 / 2 / 4 | MODE OFF / LOOP / SLICE / GRAIN | SOURCE : pistes MUTE / MIX |
| **SLICE** | les 16 slices de la boucle | la slice | laquelle elle joue | à l'envers | RETRIG x1 … x8 |
| **GRAIN** | la boucle en granulaire | POSITION | SIZE | DENSITY | PITCH |
| **SEQ** | séquenceur / automation d'effets | le pas | son effet | sa probabilité | longueur OFF / 16 / 32 / 64 |
| **BANK** | les pads | banque DESTROY / SLOOP | INTENSITÉ | HOLD | FX : SOLO / MULTI |
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

### Boutons OCT- / OCT+ sur les pages K-GLITCH
- **OCT+ = DICE** : sur SEQ, une séquence d'effets ; sur LOOP / SLICE / GRAIN, les slices réorganisées ; ailleurs, le rythme.
- **OCT- = CLEAR** (voir Automation) ; sur LOOP / SLICE / GRAIN, remet les slices dans l'ordre.

### SOLO / MULTI
Page BANK, K4. **SOLO** : un nouvel effet remplace le précédent. **MULTI** : les effets s'empilent (jusqu'à 3 DRUM DESTROYER + 1 SLOOP en même temps, les pads tenus, la séquence et AUTO ensemble) ; l'automation enregistre alors deux effets par pas.

### Le sampler (LOOP, SLICE, GRAIN)
- **REC** enregistre, à partir de la mesure suivante, 1, 2 ou 4 mesures de **ce qui joue** (effets compris) ; la boucle tourne ensuite calée sur le tempo. SOURCE MUTE coupe les pistes pour n'entendre que la boucle (à détruire avec les pads : elle passe dans tous les effets).
- **Mémoire** : le FM-1 n'a pas de RAM libre pour ça ; la boucle occupe la ligne du **delay tempo** (128 Ko, 2,97 s en mono 22 kHz : 2 mesures jusqu'à 162 BPM, 1 mesure jusqu'à 81). Pendant qu'une boucle est gardée, le delay tempo est muet ; MODE OFF libère la boucle et rend le delay.
- **SLICE** : 16 slices ; pour chacune, laquelle joue (réorganisation), à l'envers, retrigger x2, x3, x4, x8 (stutter). Les touches blanches jouent une slice (et l'écrivent dans la carte pendant REC).
- **GRAIN** : 4 grains de 10 à 250 ms, position (ou la touche tenue), densité, pitch ±12, CHAOS = dispersion.

### Instruments GLITCH
Nouveau moteur **GLITCH** pour les pistes synthé (PRESETS, famille GLCH) : 6 modes (BYTE = bytebeat, BITS, BLIP, ZAP, DATA, BEND), retrigger interne, sauts de pitch aléatoires reproductibles (SEED), crush, tone. 10 presets : BYTEBEAT, BYTE CRUSH, BIT SNAP, BLIP RAIN, ZAP GUN, DATA DUST, DATA LOOP, BENT SAW, ROBOT, CRASH BUG.

### Police
Écran plus lisible : police lissée (d'après IBM Plex Mono SemiBold, licence OFL) sur les mêmes grilles que SLOOP. Elle prend environ 35 Ko de flash au lieu de 5 Ko : à surveiller sur le FM-1 (`KG_FONT=terminus` remet la police d'origine).

### Les trois modes
- **DRUMS** : écrit 2 mesures dans un style (JUNGLE 170, BREAKCORE 190, DNB 174, DRILL 142, IDM 128, GLITCH 120, HALFTIME 160, FOOTWORK 160) avec son tempo et un kit adapté ; VARIATION le varie aussitôt. Ce motif devient l'**original**.
- **CHOP** : **DICE** tire une recette au hasard (2 à 5 transformations parmi couper, décaler, accentuer, répéter, roll, ghost, longueur, chacune à une force aléatoire) et l'affiche ; MUTATE transforme le rythme à partir de l'original (jamais d'une mutation précédente) : **décale** des coups (pas voisin ou micro-timing), en **supprime**, **répète** des fragments, ajoute des **rolls** et des **ghost notes**, change des **accents** et, avec TARGET ALL, les **longueurs de notes** des synthés. CHAOS dose la destruction, DENSITY les répétitions et découpes. EVERY remute seul toutes les 1, 2, 4 ou 8 mesures. La grille montre en orange ce qui a changé.
- **CHAOS** : AUTO + LFO + un DICE du rythme (et parfois de la séquence d'effets) à chaque mesure, avec une probabilité de CHAOS %.

### Séquenceur d'effets, AUTO, MPC
- **SEQ** : un effet (ou rien) par pas, sur 16, 32 ou 64 pas (K4) ; une **probabilité** par pas (K3 : une barre blanche en haut de la case) ; une touche blanche choisit le pas (sur la page de 16 affichée) ; PRESETS règle l'intensité du pas ; OCT+ (DICE) écrit une séquence selon CHAOS / DENSITY.
- **AUTO** : à chaque 16e, DENSITY % de chances de lancer un effet choisi selon CHAOS.
- **MPC** : les effets partent en MIDI CC (voir [docs/MPC_REMOTE.md](docs/MPC_REMOTE.md)). Avec GLO > SYSTEM > MIDI OUT sur SEQ, la MPC reçoit aussi le rythme muté en notes.

Priorité : pad > séquenceur > AUTO. Ces réglages sont « live » : pas encore sauvegardés dans les projets.

## Code

- `source/firmware/src/kglitch.c` : le moteur (séquenceur d'effets, AUTO, MUTATE, sortie MPC, break de démo).
- `source/firmware/src/kg_dd.c` : la banque DRUM DESTROYER (16 effets).
- `source/firmware/src/kg_rhythm.c` : DRUMS (générateur par style) et CHOP (mutations du rythme).
- `source/firmware/src/kg_loop.c` : le sampler (LOOP, SLICE, GRAIN).
- `source/firmware/src/eng_glitch.c` : le moteur GLITCH.
- `source/firmware/src/kg_ui.c` + `kg_font5.h` : les écrans K-GLITCH.
- `source/firmware/host/emulator.c`, `source/web/emulator.html`, `source/tools/build_emulator.py` : l'émulateur.
- Tests : `source/tests/kglitch_test.c`, et `source/tests/ui_pages_test.c` (captures des écrans).

Voir aussi [docs/ROADMAP.md](docs/ROADMAP.md). Les crédits et licences de SLOOP / Felucca sont dans `source/LICENSING.md` et `source/LICENSES/` ; les icônes et l'image du panneau de Felucca restent sous leurs propres conditions (voir `source/LICENSING.md`).

Projet non affilié à M-VAVE, Hügelton Instruments ni isod89.
