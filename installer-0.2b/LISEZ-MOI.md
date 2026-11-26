# K-GLITCH 0.2b pour le M-VAVE FM-1

K-GLITCH 0.2b est un nouveau firmware, construit sur **LOAF BREAKER** (FLEXOR), lui-même construit sur **CHOPPAH** (Laurens Profittlich) et fm1-base. On y retrouve le découpeur de breaks, le kick, le séquenceur et les effets de LOAF BREAKER, avec en plus les effets rythmiques de K-GLITCH.

## Installer
1. FM-1 branché en USB avec un câble de données, allumé.
2. Ouvrez `K-GLITCH-0.2b-installer.html` dans **Chrome ou Edge** (sur ordinateur).
3. **Installer**, accepter l'accès MIDI, attendre la fin sans débrancher.

Ligne de commande (Python 3, `pip install mido python-rtmidi`) : `python3 fm1_install.py k-glitch-0.2b.fwsc` (l'outil de fm1-base).

## Ce que K-GLITCH ajoute
- **48 effets rythmiques en 3 banques de 16** (ils réécrivent les pas du break : slice, sens, pitch, répétitions, trous) :
  - **CHOP** : STUTTER, REPEAT 1/8, REPEAT 1/4, REVERSE X, SKIP, HALF TIME, DOUBLE, PITCH MAD, SCRATCH, BACKSPIN, SHUFFLE, GATE X, MICRO, TRIPLET, TAPE STOP, CHAOS ;
  - **DESTROY** (le DRUM DESTROYER de K-GLITCH) : LOOP DESTROY, STUTTER X, GLITCH, SCRATCH X, REVERSE XX, PITCH MAD X, GATE XX, FILTER CHAOS, BEAT REPEAT, BEAT SKIP, MICRO STUT, TIME STRETCH, RING MOD, FUZZ, RANDOMIZER, TOTAL CHAOS (RING MOD, FUZZ et FILTER CHAOS passent par les effets ARP RING, DIST et RADIO) ;
  - **BEAT** (des programmes sur la mesure) : RND STUTTER, RND BAR, RND FILL, ROLL UP, SKIP 8TH, STAMMER, GATE 8, GATE 16, GATE 3-3-2, GATE RAMP, REV BEAT, REV OFFBEAT, SCRATCH BAR, SCRATCH CUT, TAPE STOP, HALF TIME.
- **Page GLITCH = la première page du bouton FX** (FX encore : la chaîne d'effets de LOAF BREAKER ; aussi le dernier onglet de HOME). C'est l'éditeur de chop, ses pas en barres (hauteur = intensité, couleur = banque : CHOP orange, DESTROY cyan, BEAT jaune).
  - **OCT− / OCT+** : la banque des touches blanches (CHOP, DESTROY, BEAT), écrite en haut à droite.
  - Touches blanches : les 16 effets de la banque, tant qu'elles sont tenues.
  - REC en lecture : l'effet tenu s'écrit dans le **couloir** (automation de 16 / 32 / 64 pas ; les 3 banques s'y mélangent).
  - K1 le pas, K2 son effet (les 48), K3 l'intensité, K4 la longueur du couloir (OFF / 16 / 32 / 64).
  - **SEL + touche blanche** : cet effet retiré du couloir, à la volée.
- **Sur HOME, OCT− et OCT+ sont des SHIFT** :
  - **OCT− maintenu + touche blanche** : l'effet de la banque choisie sur la page GLITCH ;
  - **OCT+ maintenu + touche blanche** : l'effet ARP de la touche (les 16 effets sur le mix) ;
  - OCT− / OCT+ tapé seul : le pitch du break, comme avant (à la relâche).

## Apparence
- **RIBBON CLASSIC** (le look de K-GLITCH / K-WASUB) est l'apparence de base : fond ardoise, onglets en pastille orange, chaque page sur une grande carte arrondie, blocs et potards en cartes, une couleur par potard, valeurs en cyan ; l'onde en couleurs RIBBON (basses orange = kicks, médiums cyan = caisses claires, aigus blancs = charleys).
- L'ancienne apparence de LOAF BREAKER reste au choix : **SYSTEM › SETUP, K1 SKIN** (ses 24 skins, RIBBON est le dernier). Un FM-1 qui avait déjà un skin enregistré le garde.

## Bon à savoir
- Version de développement, testée dans le simulateur de fm1-base (scénarios et tests du moteur), **pas encore sur un FM-1 réel**.
- Les sons de la caisse claire, du charley et du clap jouent toujours dans les patterns, mais leurs réglages ne sont plus accessibles.
- Le break FOCUZ 175 de FLEXOR (un sample personnel) n'est pas inclus : 7 breaks Amen libres de droits.
- Revenir au firmware officiel : la section prévue en bas de l'installateur, ou M-UPGRADE de M-VAVE.
- Paquet `k-glitch-0.2b.fwsc`, identité FM-1_4519000200, SHA-256 `2514a3910fb0a3a609a1dfad5f7702ba4d7e96c1051b52233f2d3efeb6a68a3c`.
- Page FX › GLITCH : le plan des 16 touches blanches (2 rangées de 8) affiche l'effet de chaque touche pour la banque choisie (OCT-/OCT+), la touche tenue en orange, l'effet joué cerclé de blanc.
- OCT- + OCT+ ensemble (partout) : le son bascule entre LOAF (fort, break à -12 LUFS, 3 dB de marge MAX OUT) et CHOPPAH (plus clair : break 4 dB plus bas dans le limiteur, sans marge). Le choix est sauvegardé ; le pitch revient à 0.
