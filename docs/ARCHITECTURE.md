# K-GLITCH — architecture

- **Base** : SLOOP 2.8.4 dans `source/` (son, séquenceur, UI, USB/MIDI, mise à jour), sans changement de structure.
- **Moteur** : `kglitch.c` est inclus par `fx.c` juste après `punch.c`. Une fois par bloc audio, `punch_process` demande à `kg_route()` quel effet jouer : la touche (punch.req), sinon le pas du séquenceur d'effets, sinon AUTO. Les effets ne jouent que sur le mix du FM-1 (la sortie CC vers une MPC a été retirée en 0.9.3).
- **Réglages** : globaux `G_KCHAOS` … `G_KBRK`, après `G_FXFADE`, hors du format de projet (live seulement ; vérifié par `_Static_assert` dans `project.c`).
- **Écrans** : les pages `GR_KG` sont dessinées entières par `kg_ui.c` (deux bandes de 120 lignes, redessinées quand quelque chose change ; l'oscilloscope seul un cadre sur deux).
- **Émulateur** : `firmware/host/emulator.c` compile la vraie UI et le vrai son pour wasm32 (sans libc, `tools/wasm_shim`), avec l'écran, les boutons, les touches et le MIDI fournis par la page.
