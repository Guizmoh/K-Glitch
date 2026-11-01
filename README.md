# K-GLITCH

**DRUM DESTROYER pour le M-VAVE FM-1** — un firmware de glitch et de déconstruction rythmique (breakcore, jungle, IDM, drill, glitch), construit sur **[SLOOP](https://github.com/isod89/sloop-fm1) 2.8.4** (isod89), lui-même basé sur **Felucca** (Leo Kuroshita / Hügelton Instruments). GPL-3.0-only.

> État : version de développement **0.2**. Testé sur ordinateur (tests hôtes + émulateur web). **Jamais compilé pour le FM-1 ni essayé sur l'appareil** : ne pas l'installer.

## Essayer : l'émulateur

Ouvrir [`emulator/index.html`](emulator/index.html) dans un navigateur (une seule page, fonctionne hors ligne). Le firmware y tourne réellement (son, écran, séquenceur, effets) en WebAssembly. Un break jungle à 170 BPM est chargé : **PLAY** (ou Espace), puis **FX**.

Reconstruire : `cd source && python3 tools/build_emulator.py` (clang avec la cible wasm32 et wasm-ld, Python 3 + Pillow).

## Ce que K-GLITCH ajoute à SLOOP

Tout SLOOP est conservé (4 pistes, 12 moteurs, kits, samples, séquenceur, 27 effets punch-in, MIDI…). Le bouton **FX** ouvre trois nouvelles pages, dessinées en plein écran dans le style « Ribbon classique » de K-WASUB :

| Page | K1 | K2 | K3 | K4 |
| --- | --- | --- | --- | --- |
| **DESTROY** | CHAOS | DENSITY | MOVEMENT | OUT (FM-1 / MPC / BOTH) |
| **FX SEQ** | le pas (1–16) | son effet | SEQ on/off | DICE : mutation aléatoire |
| **BANK** | la banque des touches : DESTROY / SLOOP | INTENSITÉ | HOLD | — |
| **SETUP** | AUTO on/off | CLEAR la séquence | canal MIDI de la MPC | BREAK : charge un break jungle 170 BPM |

- **Banque DRUM DESTROYER** (par défaut sur les 16 touches blanches, FX maintenu) : LOOP DESTROY, STUTTER X, GLITCH, SCRATCH, REVERSE X, PITCH MAD, GATE X, FILTER CHAOS, BEAT REPEAT, BEAT SKIP, MICRO STUTTER, TIME STRETCH, RING MOD, FUZZ, RANDOMIZER, TOTAL CHAOS. **INTENSITÉ** règle chacun (ex. LOOP : de 1/2 à 1/32 de mesure ; STUTTER : 2x, 3x, 4x, 8x ; PITCH : escalier, sauts, plongée). Les touches noires gardent les effets de SLOOP ; BANK sur SLOOP rend les 27 effets d'origine.
- **HOLD** : l'effet reste après avoir lâché la touche (jusqu'à une autre touche ou HOLD off).
- **Live punch-in** : FX maintenu + une touche.
- **Séquenceur d'effets** : un effet (ou rien) sur chacun des 16 pas, calé sur le tempo.
- **AUTO** : à chaque double-croche, DENSITY % de chances de lancer un geste ; CHAOS élargit le choix (des boucles douces jusqu'au tape stop, freeze, wobble…), MOVEMENT règle leur longueur.
- **DICE / MUTATE** : écrit une nouvelle séquence d'effets selon CHAOS et DENSITY.
- **MPC** : les effets partent en MIDI CC — voir [docs/MPC_REMOTE.md](docs/MPC_REMOTE.md).

Priorité : touche > séquenceur > AUTO. Ces réglages sont « live » : ils ne sont pas encore sauvegardés dans les projets.

## Code

- `source/firmware/src/kglitch.c` : le moteur (séquenceur d'effets, AUTO, MUTATE, sortie MPC, break de démo).
- `source/firmware/src/kg_dd.c` : la banque DRUM DESTROYER (16 effets).
- `source/firmware/src/kg_ui.c` + `kg_font5.h` : les écrans K-GLITCH.
- `source/firmware/host/emulator.c`, `source/web/emulator.html`, `source/tools/build_emulator.py` : l'émulateur.
- Tests : `source/tests/kglitch_test.c`, et `source/tests/ui_pages_test.c` (captures des écrans).

Voir aussi [docs/ROADMAP.md](docs/ROADMAP.md). Les crédits et licences de SLOOP / Felucca sont dans `source/LICENSING.md` et `source/LICENSES/` ; les icônes et l'image du panneau de Felucca restent sous leurs propres conditions (voir `source/LICENSING.md`).

Projet non affilié à M-VAVE, Hügelton Instruments ni isod89.
