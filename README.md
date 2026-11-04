# K-GLITCH

**DRUM DESTROYER pour le M-VAVE FM-1** — un firmware de glitch et de déconstruction rythmique (breakcore, jungle, IDM, drill, glitch), construit sur **[SLOOP](https://github.com/isod89/sloop-fm1) 2.8.4** (isod89), lui-même basé sur **Felucca** (Leo Kuroshita / Hügelton Instruments). GPL-3.0-only.

> État : version de développement **0.9**. Compilé pour le FM-1 avec la chaîne officielle de JieLi (image 567 Ko sur 581, RAM 73 Ko sur 96, toutes les vérifications de la compilation passent), testé sur ordinateur (tests hôtes, émulateur web). **Pas encore essayé sur un FM-1 réel** : à vos risques.

## Installer sur le FM-1

Dossier [`installer/`](installer/) : un installateur **en un seul fichier, le firmware dedans, sans internet** — mode d'emploi dans [`installer/LISEZ-MOI.md`](installer/LISEZ-MOI.md).

1. FM-1 branché en USB (câble de données, directement sur l'ordinateur).
2. Windows : `INSTALLER-WINDOWS.bat` ; Mac : `INSTALLER-MAC.command` ; sinon ouvrir `K-GLITCH-0.9.1-installer.html` avec **Chrome ou Edge**.
3. **Installer**, autoriser le MIDI, attendre « Terminé » (environ une minute).

Aussi en ligne de commande : `python3 installer/fm1_install.py installer/k-glitch-0.9.1.fwsc` (mido + python-rtmidi). Secours : OCT− maintenu à l'allumage.

Construire le paquet et l'installateur : `cd source && sh tools/get_toolchain.sh` (une fois), les trois fichiers du SDK AC79 (voir `source/BUILDING.md`), puis `sh build.sh --release 0.9` et `python3 tools/make_installer.py build/felucca-0.9.fwsc 0.9 ../installer/K-GLITCH-0.9.1-installer.html`. Pour tenir dans la flash, quatre jeux de samples de SLOOP (PIANO, BASS, VIBES, HORNS) gardent un son sur deux (chacun couvre deux fois plus de touches) : 66 Ko rendus à K-GLITCH.

## Essayer : l'émulateur

Ouvrir [`emulator/index.html`](emulator/index.html) dans un navigateur (une seule page, fonctionne hors ligne). Le firmware y tourne réellement (son, écran, séquenceur, effets) en WebAssembly. Un break jungle est chargé, au tempo de base de 98 BPM : **PLAY** (ou Espace), puis **FX**.

**Aide-mémoire** : [`docs/cheatsheet.html`](docs/cheatsheet.html) (boutons, pages, effets, filtres, CC MPC, clavier).

Reconstruire : `cd source && python3 tools/build_emulator.py` (clang avec la cible wasm32 et wasm-ld, Python 3 + Pillow).

## Ce que K-GLITCH ajoute à SLOOP

Tout SLOOP est conservé (4 pistes, 12 moteurs, kits, samples, séquenceur, 27 effets punch-in, MIDI…). Le bouton **FX** ouvre K-GLITCH : onze pages dessinées en plein écran dans le style « Ribbon classique » de K-WASUB (SELECT passe de l'une à l'autre). Ce thème est aussi celui de **tout le logiciel** : avec la palette **RIBBON** (par défaut), les pages de SLOOP ont les mêmes onglets (les pages de la famille, celle affichée en pastille orange, le temps et le tempo à droite), les valeurs en cartes, le graphe sur un panneau, les filets orange ; les écrans TRACKS, DRUMS et REC ont les onglets TRACKS / DRUMS / SONG et des cartes pour les potards. MENU > COLOR garde les autres palettes, avec la mise en page d'origine de SLOOP.

Les pages K-GLITCH sont rangées par bouton (0.9) :

- **FX** (un appui : les **16 PADS**, un autre appui : la page suivante) : PADS (banque + macros), FX SEQ, puis le SLICER et l'INSERT de SLOOP.
- **LFO** : … puis les envois **SENDS** (DIST, CHO, DLY, REV de la piste), **DLY** et **REV/CHO**.
- **ARP** : … puis **MODES** (mode CHAOS, AUTO, LFO, HOLD), **MODES 2** (SOLO / MULTI, INTENSITÉ, CLEAR), **GEN** (générateur DRUMS) et **CHOP** (mutation).
- **SEL** : le **FILTER** créatif et le sampler **LOOP**, **SLICE**, **GRAIN** (la gamme se règle avec SEL maintenu : CHORD, SCALE, KEYS, TRANSPOSE).
- **EDIT** : … et, en dernier, **DJ FILTER** : le filtre DJ de la piste sélectionnée (K1 : passe-bas à gauche, passe-haut à droite), sa **résonance** (K2, propre à chaque piste) et sa **courbe** dessinée avec la fréquence de coupure.
- **GLO** : … et **MPC** (sortie MIDI vers la MPC).
- **SAVE** (un appui, même depuis HOME) : les pages SAVE ; l'écran SONG : SAVE maintenu + 16e touche blanche.
- **Sélecteur de pages** : bouton d'une section maintenu (FX, EDIT, ARP, SEQ, SEL, GLO, SAVE) + **ALGO** : la liste de ses pages, **les plus visitées en premier** (étoile) ; on relâche le bouton : la page choisie s'ouvre.
- **Bandeau du haut** : les pages de la section en onglets, la page affichée en pastille orange au milieu de ses voisines (un triangle quand il y en a d'autres à gauche ou à droite) ; à droite le tempo et la batterie en petit chiffre (USB : alimenté).
- **Automation des sons** : en **REC + PLAY**, tourner un potard d'une page de son de la piste sélectionnée (EDIT : DCW, CUT…, ENV, LFO, les envois, le FILTER de piste, l'INSERT, le SLICER) l'écrit sur le pas qui joue ; il est rejoué à chaque tour (verrous de paramètres de SLOOP, 24 par piste).
- **Outils de séquence** : **SEQ maintenu + PRESETS** choisit un outil, **OCT+** l'applique au motif de la piste sélectionnée, **OCT-** l'annule : RANDOM (pas aléatoires : notes dans la gamme de la piste, ou coups du son choisi sur la batterie), MUTATE (un quart des pas retirés au sort), COPY, PASTE (sur une piste du même genre), CLEAR, DOUBLE (longueur x2, la 2e moitié recopiée), HALF, SHIFT < / > (décale d'un pas), REVERSE, THIN OUT (un pas sur trois vidé).
- **Annuler / rétablir** : **SAVE (ou EDIT) + OCT-** annule la dernière modification des motifs (enregistrement, effacement, pas, outils…), autant de fois que l'historique en contient ; **+ OCT+** rétablit.

| Page | Rôle | K1 | K2 | K3 | K4 |
| --- | --- | --- | --- | --- | --- |
| **PADS** (FX) | écran principal : les 16 pads | BANK : SLOOP / DESTROY / BEAT | CHAOS | DENSITY | MOVEMENT |
| **FILTER** (SEL) | filtre créatif sur tout le mix | TYPE | FREQ (une note) | RESO | MOVE (LFO) |
| **GEN** (ARP) | mode 1 : génération de percussions | STYLE | VARIATION | SWING | GO |
| **CHOP** (ARP) | mode 2 : découpage / mutation du rythme | DICE | MUTATE | RESTORE | EVERY (auto) |
| **LOOP** (SEL) | sampler : enregistre ce qui joue | REC | BARS 1 / 2 / 4 | MODE OFF / LOOP / SLICE / GRAIN | SOURCE : pistes MUTE / MIX |
| **SLICE** (SEL) | les 16 slices de la boucle | la slice | laquelle elle joue | à l'envers | RETRIG x1 … x8 |
| **GRAIN** (SEL) | la boucle en granulaire | POSITION | SIZE | DENSITY | PITCH |
| **FX SEQ** (FX) | séquenceur / automation d'effets | le pas | son effet | sa probabilité | longueur OFF / 16 / 32 / 64 |
| **MODES 2** (ARP) | les pads | FX : SOLO / MULTI | INTENSITÉ | CLEAR (retour au son original) | |
| **MODES** (ARP) | mode 3 : performance imprévisible | MODE CHAOS | AUTO | LFO | HOLD |
| **MPC** (GLO) | sortie vers la MPC | OUT : FM-1 / MPC / BOTH | canal MIDI | effacer la séquence | CHOP : DRUMS / ALL |

### Jouer en direct
- Sur les pages K-GLITCH, **les 16 touches blanches sont des pads** : maintenir = l'effet agit, relâcher = le son revient (punch-in). Ailleurs : FX maintenu + touche.
- **ALGO** (encodeur A) choisit l'effet, **PRESETS** (encodeur B) règle son **intensité** — chaque effet a la sienne. Un pad tenu + PRESETS : l'intensité de ce pad.
- **Automation** : pendant l'enregistrement (**REC** + PLAY), chaque effet joué sur un pad — les 16 DRUM DESTROYER comme les 27 de SLOOP — s'écrit sur ses 16es dans la séquence d'effets, **avec son intensité**, et se rejoue ensuite. Sur la page SEQ, pendant REC, les touches redeviennent des pads (on voit la piste se remplir). La séquence d'effets prend la **longueur de la boucle** (sa piste la plus longue : 16, 32 ou 64 pas) dès qu'on enregistre : une boucle de 2 ou 4 mesures enregistre ses effets sur toutes ses mesures ; si la séquence était plus courte, ce qu'elle contenait est recopié sur les nouvelles mesures, rien n'est perdu.
- **CLEAR (OCT- sur les pages K-GLITCH)** : maintenu pendant la lecture ou l'enregistrement, il efface les effets des pas qui passent (comme sur le PO-33) ; maintenu 1 s à l'arrêt, il efface toute l'automation.
- **HOLD** verrouille un effet ; **CLEAR** coupe tout (effets, AUTO, CHAOS, séquence) et rend le rythme original.
- **LFO** : module l'intensité des effets ; **MOVEMENT** règle sa vitesse (4 mesures à 1 temps) et sa profondeur.

### Trois banques d'effets (FX > BANK, K1)
Les 16 touches blanches jouent l'une des trois banques ; les touches noires gardent toujours les effets de SLOOP.

- **SLOOP** : les effets d'origine (LOOP 1/4, 1/8, 1/16, 1/32, STUTTER, REVERSE, STOP, HALF, filtres, CRUSH, GATE, ECHO…).
- **DESTROY** (DRUM DESTROYER) : LOOP DESTROY, STUTTER X, GLITCH, SCRATCH, REVERSE X, PITCH MAD, GATE X, FILTER CHAOS, BEAT REPEAT, BEAT SKIP, MICRO STUTTER, TIME STRETCH, RING MOD, FUZZ, RANDOMIZER, TOTAL CHAOS. L'intensité change leur comportement (ex. LOOP : de 1/2 à 1/32 de mesure ; STUTTER : 2x, 3x, 4x, 8x ; PITCH : escalier, sauts, plongée).
- **BEAT** (inspirée des effets de déstructuration sur une mesure entière, type Flex Beat) : chaque effet est un **programme sur la mesure**, calé sur le transport, tiré au hasard à chaque mesure pour les randomiseurs :
  - **RND STUTTER** : stutters (32es, triolets, 64es) sur des croches au hasard ; **RND BAR** : chaque double-croche répétée, rejouée, à l'envers, coupée, ralentie ou pitchée ; **RND FILL** : la fin de la mesure devient un break au hasard ;
  - **ROLL UP** : le premier temps répété en noires, croches, doubles, triples (+ pitch qui monte) ;
  - **SKIP 8TH** (contretemps sautés ou doublés), **STAMMER** (3-3-2 : les coups gardés, le reste bégaie) ;
  - **GATE 8**, **GATE 16**, **GATE 3-3-2**, **GATE RAMP** (de plus en plus rapide) : un gate différent par touche ;
  - **REV BEAT** (des temps entiers à l'envers), **REV OFFBEAT** (les contretemps à l'envers) ;
  - **SCRATCH BAR** : un temps de silence, trois scratchs courts du premier temps, puis le premier temps à l'envers ; **SCRATCH CUT** : temps 2 et 4 scratchés au crossfader ;
  - **TAPE STOP** (la fin de la mesure ralentit jusqu'à l'arrêt), **HALF TIME**.

Toutes s'enregistrent en automation, partent en CC MIDI (le CC de la touche) et sont tirées par AUTO / DICE.

### SCRATCH réaliste
Le SCRATCH de DESTROY (et ceux de BEAT) imitent une main sur un disque : des gestes qui accélèrent, s'arrêtent et repartent en arrière (baby scratch, chirp, tear, transformer, scribble, drag, scratch arrière selon l'intensité), sous un crossfader, avec une lecture interpolée cubique. Le son suit la vitesse du disque comme une cellule : plus sourd et plus faible quand il ralentit, muet à l'arrêt. Le « disque » est la dernière attaque trouvée dans ce qui vient de jouer (ou le premier temps pour SCRATCH BAR).

### Batteries FM et C64 (piste batterie > EDIT)
Deux kits synthétisés propres à K-GLITCH, choisis dans la liste des kits (**FM DRUM**, **C64 DRUM**) ou sur la page **KIT**. Piste batterie sélectionnée, **EDIT** ouvre l'édition du son joué en dernier (ou choisi avec SND) :

| Page | K1 | K2 | K3 | K4 |
| --- | --- | --- | --- | --- |
| **KIT** | KIT (FM / C64) | SND (le son) | LEVEL | INIT (son d'usine) |
| **FM TONE** | PITCH | RATIO (0.5 … 11.2) | INDEX | I.DEC (chute de l'index) |
| **FM BODY** | FDBK | BEND | BTIM | DEC |
| **FM NOISE** | NOIS | N.DEC | TONE (sombre … brillant) | SND |
| **C64 WAVE** | ATTK (onde d'attaque) | FRMS (sa durée en trames) | BODY (onde du corps) | PITCH |
| **C64 MOVE** | SWP (pitch par trame) | PW | PWM | DEC (taux du SID) |
| **C64 FILT** | FILT (LP / BP / HP) | CUT | RES | SND |
| **DRUM FX** | SAT : OFF / SOFT / HARD / FOLD | DRIVE | DLY (envoi delay) | REV (envoi reverb) |

- **FM** : une sinusoïde modulée en phase par une autre à RATIO (avec feedback), un index qui retombe (attaque brillante, corps rond), une chute de pitch, du bruit coloré.
- **C64** : une voix façon SID en trames de 50 Hz, comme les routines de batterie du C64 : une onde d'attaque (bruit, pulse…) pendant quelques trames, puis l'onde du corps qui glisse en pitch et en largeur d'impulsion, les taux de decay du SID, son filtre, des ondes 12 bits et son bruit 23 bits.
- **Par instrument** : saturation (3 types) + drive, envoi delay et envoi reverb propres à chaque son (ils remplacent les envois globaux de la piste pour ces kits).
- Chaque réglage s'entend aussitôt (à l'arrêt) ; les deux kits sont sauvegardés en flash avec le reste de K-GLITCH.

### Le filtre (FILTER)
Un filtre sur tout le mix (boucle du sampler comprise, avant son enregistrement), 11 types :
- **LP, HP, BP, NOTCH** : filtre à variable d'état résonant ;
- **DRIVE** : saturation puis passe-bas résonant (acide) ;
- **COMB+ / COMB−** : filtre en peigne accordé sur la note (résonance = réinjection) : anneau aigu ou son creux et nasal ;
- **METAL** : quatre peignes à rapports inharmoniques, en stéréo : cloches, gongs, tôle ;
- **VOWEL** : deux formants qui passent de A à E, I, O, U quand la fréquence monte : un filtre qui parle ;
- **PHASER** : quatre étages déphaseurs avec réinjection, à faire bouger avec MOVE ;
- **RESONATOR** : trois passe-bandes très étroits sur la note, son octave et sa quinte : le mix sonne en accord.

FREQ se règle en notes (les types accordés restent justes) ; sur la page FILTER, **les touches accordent le filtre** sur leur note (OCT± décale). MOVE fait bouger la fréquence jusqu'à ±3 octaves au rythme du LFO (vitesse = MOVEMENT). L'écran dessine la forme du filtre. Mémoire : 4 Ko pour les peignes.

**Automation du filtre** : avec REC + PLAY, toucher le filtre (un potard ou une touche) écrit FREQ, TYPE et RESO sur le pas en cours ; un potard tourné pendant la lecture dessine une courbe. À la relecture, un pas enregistré impose ses valeurs jusqu'au suivant ; toucher le filtre reprend la main ; CLEAR (OCT- tenu) efface aussi cette automation au passage. La page FILTER montre la bande d'automation (une barre par pas : hauteur = la note, couleur = le type) ; sur SEQ, un petit carré violet marque les pas qui portent le filtre. Sauvegardé avec le reste.

### Boutons OCT- / OCT+ sur les pages K-GLITCH
- **OCT+ = DICE** : sur SEQ, une séquence d'effets ; sur LOOP / SLICE / GRAIN, les slices réorganisées ; ailleurs, le rythme.
- **OCT- = CLEAR** (voir Automation) ; sur LOOP / SLICE / GRAIN, remet les slices dans l'ordre.

### SOLO / MULTI
Page BANK, K4. **SOLO** : un nouvel effet remplace le précédent. **MULTI** : les effets s'empilent (jusqu'à 3 DRUM DESTROYER + 1 SLOOP en même temps, les pads tenus, la séquence et AUTO ensemble) ; l'automation enregistre alors deux effets par pas.

### Le sampler (LOOP, SLICE, GRAIN)
- **REC** enregistre, à partir de la mesure suivante, 1, 2 ou 4 mesures de **ce qui joue** (effets compris) ; la boucle tourne ensuite calée sur le tempo. SOURCE MUTE coupe les pistes pour n'entendre que la boucle (à détruire avec les pads : elle passe dans tous les effets).
- **Mémoire et son** : le FM-1 n'a pas de RAM libre pour ça ; la boucle occupe la ligne du **delay tempo** (128 Ko), en échantillons 8 bits par blocs de 16 qui partagent une échelle (« bloc flottant ») : **pleine bande à 44,1 kHz** (les aigus, les charlestons) sur 2,88 s, soit 1 mesure jusqu'à 84 BPM ; quand les mesures demandées ne tiennent pas, elle passe en 22 kHz (filtre demi-bande) sur 5,76 s, soit 2 mesures jusqu'à 84 BPM. Relue avec une interpolation cubique, au même niveau que la source. Pendant qu'une boucle est gardée, le delay tempo est muet ; MODE OFF libère la boucle et rend le delay.
- **SLICE** : 16 slices ; pour chacune, laquelle joue (réorganisation), à l'envers, retrigger x2, x3, x4, x8 (stutter). Les touches blanches jouent une slice (et l'écrivent dans la carte pendant REC).
- **GRAIN** : 4 grains de 10 à 250 ms, position (ou la touche tenue), densité, pitch ±12, CHAOS = dispersion.

### Instruments GLITCH
Nouveau moteur **GLITCH** pour les pistes synthé (PRESETS, famille GLCH) : 6 modes (BYTE = bytebeat, BITS, BLIP, ZAP, DATA, BEND), retrigger interne, sauts de pitch aléatoires reproductibles (SEED), crush, tone. 10 presets : BYTEBEAT, BYTE CRUSH, BIT SNAP, BLIP RAIN, ZAP GUN, DATA DUST, DATA LOOP, BENT SAW, ROBOT, CRASH BUG.

### Batterie sur une piste synthé (moteur KDRUM)
N'importe laquelle des 3 pistes synthé peut devenir une batterie : sur la piste, PRESETS > **FM KIT** ou **C64 KIT** (famille FX), ou EDIT > moteur **KDRUM**. Chaque touche joue un des 16 sons de la batterie FM / C64 (Do3 = grosse caisse, puis un son par demi-ton, et ça recommence tous les 16 demi-tons) : ce sont les mêmes sons que ceux édités sur la piste batterie (EDIT > FM TONE…, SAT comprise). EDIT : KIT (FM / C64) et TUNE (±24 demi-tons). Le DLY et la REV sont les envois de la piste. Coût : comme un son de la piste batterie, par note jouée.

### Police et écran
Police lissée dérivée de **Carlito Bold** (licence OFL), proportionnelle avec des chiffres de largeur fixe, sur les mêmes lignes que SLOOP. Économe : seuls les caractères affichés (tout est en capitales) et 4 niveaux de lissage (2 bits par pixel) : environ **15 Ko** de flash pour les trois tailles (SLOOP : 5 Ko ; `KG_FONT=terminus` remet sa police). Dans l'émulateur, l'écran est agrandi en lissé ; le bouton **PIXELS** montre les vrais pixels du FM-1.

### Les trois modes
- **DRUMS** : écrit 2 mesures dans un style (JUNGLE 170, BREAKCORE 190, DNB 174, DRILL 142, IDM 128, GLITCH 120, HALFTIME 160, FOOTWORK 160) avec son tempo et un kit adapté ; VARIATION le varie aussitôt. Ce motif devient l'**original**.
- **CHOP** : **DICE** tire une recette au hasard (2 à 5 transformations parmi couper, décaler, accentuer, répéter, roll, ghost, longueur, chacune à une force aléatoire) et l'affiche ; MUTATE transforme le rythme à partir de l'original (jamais d'une mutation précédente) : **décale** des coups (pas voisin ou micro-timing), en **supprime**, **répète** des fragments, ajoute des **rolls** et des **ghost notes**, change des **accents** et, avec TARGET ALL, les **longueurs de notes** des synthés. CHAOS dose la destruction, DENSITY les répétitions et découpes. EVERY remute seul toutes les 1, 2, 4 ou 8 mesures. La grille montre en orange ce qui a changé.
- **CHAOS** : AUTO + LFO + un DICE du rythme (et parfois de la séquence d'effets) à chaque mesure, avec une probabilité de CHAOS %.

### Séquenceur d'effets, AUTO, MPC
- **SEQ** : un effet (ou rien) par pas, sur 16, 32 ou 64 pas (K4) ; une **probabilité** par pas (K3 : une barre blanche en haut de la case) ; une touche blanche choisit le pas (sur la page de 16 affichée) ; PRESETS règle l'intensité du pas ; OCT+ (DICE) écrit une séquence selon CHAOS / DENSITY.
- **AUTO** : à chaque 16e, DENSITY % de chances de lancer un effet choisi selon CHAOS.
- **MPC** : les effets partent en MIDI CC (voir [docs/MPC_REMOTE.md](docs/MPC_REMOTE.md)). Avec GLO > SYSTEM > MIDI OUT sur SEQ, la MPC reçoit aussi le rythme muté en notes.

Priorité : pad > séquenceur > AUTO.

### Sauvegarde
L'état K-GLITCH (la séquence d'effets avec ses deux couches, intensités et probabilités, l'intensité de chaque effet, la carte des slices, les kits FM et C64 et tous les réglages K-GLITCH) est **enregistré automatiquement** dans la flash, 3 s après un changement, transport arrêté (jamais pendant la lecture : la flash bloque le code), et **revient au démarrage**. Il est rangé à la fin du secteur des réglages de SLOOP (copies A / B : une coupure pendant l'écriture ne perd rien ; un SLOOP d'origine l'ignore). Le son de la boucle du sampler, lui, reste en RAM. Dans l'émulateur, l'état est gardé dans le navigateur.

## Code

- `source/firmware/src/kglitch.c` : le moteur (séquenceur d'effets, AUTO, MUTATE, sortie MPC, break de démo).
- `source/firmware/src/kg_dd.c` : la banque DRUM DESTROYER (16 effets) ; `kg_beat.c` : la banque BEAT et le SCRATCH.
- `source/firmware/src/kg_drum.c` : les kits FM DRUM et C64 DRUM.
- `source/firmware/src/kg_rhythm.c` : DRUMS (générateur par style) et CHOP (mutations du rythme).
- `source/firmware/src/kg_loop.c` : le sampler (LOOP, SLICE, GRAIN).
- `source/firmware/src/kg_filter.c` : le filtre créatif.
- `source/firmware/src/eng_glitch.c` : le moteur GLITCH.
- `source/firmware/src/kg_ui.c` + `kg_font5.h` : les écrans K-GLITCH.
- `source/firmware/host/emulator.c`, `source/web/emulator.html`, `source/tools/build_emulator.py` : l'émulateur.
- Tests : `source/tests/kglitch_test.c`, `source/tests/kgdrum_test.c`, et `source/tests/ui_pages_test.c` (captures des écrans).

Voir aussi [docs/ROADMAP.md](docs/ROADMAP.md). Les crédits et licences de SLOOP / Felucca sont dans `source/LICENSING.md` et `source/LICENSES/` ; les icônes et l'image du panneau de Felucca restent sous leurs propres conditions (voir `source/LICENSING.md`).

Projet non affilié à M-VAVE, Hügelton Instruments ni isod89.
