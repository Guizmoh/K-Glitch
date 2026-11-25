# K-GLITCH 0.2b pour le M-VAVE FM-1

K-GLITCH 0.2b est un nouveau firmware, construit sur **LOAF BREAKER** (FLEXOR), lui-même construit sur **CHOPPAH** (Laurens Profittlich) et fm1-base. On y retrouve le découpeur de breaks, le kick, le séquenceur et les effets de LOAF BREAKER, avec en plus les effets rythmiques de K-GLITCH.

## Installer
1. FM-1 branché en USB avec un câble de données, allumé.
2. Ouvrez `K-GLITCH-0.2b-installer.html` dans **Chrome ou Edge** (sur ordinateur).
3. **Installer**, accepter l'accès MIDI, attendre la fin sans débrancher.

Ligne de commande (Python 3, `pip install mido python-rtmidi`) : `python3 fm1_install.py k-glitch-0.2b.fwsc` (l'outil de fm1-base).

## Ce que K-GLITCH ajoute
- **16 effets CHOP** (les destructeurs rythmiques de K-GLITCH, appliqués aux pas du break) : STUTTER, REPEAT 1/8, REPEAT 1/4, REVERSE X, SKIP, HALF TIME, DOUBLE, PITCH MAD, SCRATCH, BACKSPIN, SHUFFLE, GATE X, MICRO, TRIPLET, TAPE STOP, CHAOS.
- **Page GLITCH** (EDIT, après le KICK : à la place des réglages snare / hat / clap) : l'éditeur de chop.
  - Touches blanches : les 16 effets, tant qu'elles sont tenues.
  - REC en lecture : l'effet tenu s'écrit dans le **couloir CHOP** (automation, 16 / 32 / 64 pas).
  - K1 le pas, K2 son effet, K3 l'intensité, K4 la longueur du couloir (OFF / 16 / 32 / 64).
  - **SEL + touche blanche** : cet effet retiré du couloir, à la volée.
- **Sur HOME, OCT− et OCT+ sont des SHIFT** :
  - **OCT− maintenu + touche blanche** : l'effet CHOP de la touche ;
  - **OCT+ maintenu + touche blanche** : l'effet ARP de la touche (les 16 effets sur le mix) ;
  - OCT− / OCT+ tapé seul : le pitch du break, comme avant (à la relâche).

## Bon à savoir
- Version de développement, testée dans le simulateur de fm1-base (scénarios et tests du moteur), **pas encore sur un FM-1 réel**.
- Les sons de la caisse claire, du charley et du clap jouent toujours dans les patterns, mais leurs réglages ne sont plus accessibles.
- Le break FOCUZ 175 de FLEXOR (un sample personnel) n'est pas inclus : 7 breaks Amen libres de droits.
- Revenir au firmware officiel : la section prévue en bas de l'installateur, ou M-UPGRADE de M-VAVE.
- Paquet `k-glitch-0.2b.fwsc`, identité FM-1_4519000200, SHA-256 `2e782202133e443dd3189ed97dcc6c01fe8babf4d82ec7263b5d5072eb20a49c`.
