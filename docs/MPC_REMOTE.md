# K-GLITCH — MPC REMOTE / BOTH

Un seul firmware : la page **FX > DESTROY**, **KNOB 4 (OUT)**, choisit où jouent les effets, sans reflasher.

| OUT | Ce qui se passe |
| --- | --- |
| `FM-1` | Autonome : les effets jouent sur le mix du FM-1 (comme SLOOP). Aucun CC n'est envoyé. |
| `MPC` | Le FM-1 reste sec ; chaque effet part en MIDI CC vers la MPC. |
| `BOTH` | Les deux : l'effet joue sur le FM-1 **et** part en CC. |

Les effets envoyés viennent des touches (FX maintenu + touche), du **séquenceur d'effets** (FX SEQ) et d'**AUTO** (SETUP).

## Messages

- USB-MIDI, canal réglable dans **SETUP > CH** (16 par défaut, OFF : rien n'est envoyé).
- **127** quand l'effet commence, **0** quand il s'arrête ; un seul effet à la fois. Quitter MPC/BOTH, changer de canal ou STOP remet à 0 ce qui était levé.
- **CHAOS, DENSITY, MOVEMENT, INTENSITÉ** : CC **85, 86, 87, 89** (0..127) quand ils bougent.
- Un CC par **touche** : avec la banque DRUM DESTROYER, la touche blanche n envoie le même CC que l'effet SLOOP de cette touche (ex. touche 3 = CC 22, que ce soit ALIAS... ou GLITCH). Sur la MPC, chaque touche du FM-1 = une fonction.
- Sur la MPC : MIDI Learn de chaque CC vers ce qu'il doit piloter (filtre, pitch, effet, mute…).

## Table des CC

| Touche (FX maintenu) | Couleur | Effet SLOOP | CC |
| --- | --- | --- | --- |
| 1 | blanche | LOOP 4 | 20 |
| 2 | blanche | LOOP 8 | 21 |
| 3 | blanche | LOOP 16 | 22 |
| 4 | blanche | LOOP 32 | 23 |
| 5 | blanche | STUTTER | 24 |
| 6 | blanche | REVERSE | 25 |
| 7 | blanche | STOP | 26 |
| 8 | blanche | HALF | 27 |
| 9 | blanche | LOW | 28 |
| 10 | blanche | HIGH | 29 |
| 11 | blanche | PHONE | 30 |
| 12 | blanche | CRUSH | 31 |
| 13 | blanche | ALIAS | 102 |
| 14 | blanche | GATE | 103 |
| 15 | blanche | ECHO | 104 |
| 16 | blanche | WOBBLE | 105 |
| 17 | noire | KILL LOW | 106 |
| 18 | noire | KILL MID | 107 |
| 19 | noire | KILL HIGH | 108 |
| 20 | noire | REWIND | 109 |
| 21 | noire | OCT UP | 110 |
| 22 | noire | FLANGER | 111 |
| 23 | noire | AUTO-PAN | 112 |
| 24 | noire | FREEZE | 113 |
| 25 | noire | LOOP 8T | 114 |
| 26 | noire | MONO | 115 |
| 27 | noire | FADE OUT | 116 |

## État

Testé sur le simulateur (`source/tests/kglitch_test.c`) et dans l'émulateur web (journal MIDI OUT, Web MIDI vers une vraie sortie). **Pas encore testé sur un FM-1 ni avec une MPC.**
