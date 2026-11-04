# K-GLITCH — feuille de route

## Fait (0.1, hors matériel)

- [x] Base SLOOP 2.8.4 (isod89/sloop-fm1 `91f3018`), licences conservées.
- [x] Séquenceur d'effets 16 pas, AUTO, DICE/MUTATE, macros CHAOS / DENSITY / MOVEMENT.
- [x] ~~OUT FM-1 / MPC / BOTH (effets en MIDI CC)~~ : retiré en 0.9.3, seule la synchro de tempo MIDI reste.
- [x] Écrans DESTROY / FX SEQ / SETUP (style Ribbon classique de K-WASUB), écran de démarrage K-GLITCH.
- [x] Break jungle de démo (170 BPM).
- [x] Émulateur web (WebAssembly) avec journal MIDI et Web MIDI.

## Prochaines étapes

- [x] La banque DRUM DESTROYER de 16 effets, INTENSITÉ, HOLD (0.2, testés sur l'hôte et dans l'émulateur).
- [x] Une intensité par effet (ALGO choisit, PRESETS règle), LFO piloté par MOVEMENT (0.3).
- [x] Les 16 touches comme pads directs sur les pages K-GLITCH ; CLEAR (retour au son original).
- [x] Les 3 modes : DRUMS (8 styles), CHOP (mutation du rythme : décaler, supprimer, répéter, rolls, ghosts, accents, longueurs ; RESTORE ; EVERY), CHAOS (0.3).
- [x] SCRATCH réaliste (gestes de main, crossfader, son qui suit la vitesse) (0.8).
- [ ] Comb filter dans FILTER CHAOS ; stutter 3x / 8x en triolets.
- [x] Automation de tous les effets (REC), intensité comprise, sur 16/32/64 pas ; CLEAR maintenu (OCT-) qui l'efface ; DICE de mutation du rythme (0.4).
- [x] Sampler : enregistrer une boucle de ce qui joue, SLICE (réorganisation, reverse, retrigger, DICE), GRAIN (0.4).
- [x] SOLO / MULTI, probabilité par pas, longueur 16/32/64 lisible, moteur GLITCH, police lissée, correctif écrans noirs de l'émulateur (0.4).
- [x] Filtre créatif : LP, HP, BP, NOTCH, DRIVE, COMB±, METAL, VOWEL, PHASER, RESONATOR, accordable au clavier, LFO (0.6).
- [x] Automation du filtre (FREQ, TYPE, RESO par pas), effacée par CLEAR, sauvegardée (0.7).
- [x] Troisième banque d'effets BEAT : 16 programmes sur la mesure (randomiseurs, gates, reverses, SCRATCH BAR, TAPE STOP…) (0.8).
- [x] Kits FM DRUM et C64 DRUM éditables sur l'appareil, saturation (3 types) / delay / reverb par instrument (0.8).
- [ ] Éditeur web : les kits FM / C64 (aujourd'hui seulement sur l'appareil).
- [ ] Sauvegarder une boucle dans un slot de sample utilisateur (flash) et la recharger.
- [ ] Charger de vrais breaks (samples utilisateur) dans le sampler.
- [ ] Éditeur web : vérifier la compatibilité (le moteur GLITCH ajoute un numéro de moteur).
- [ ] Plus de styles DRUMS, et des fills générés.
- [x] Sauvegarde automatique de l'état K-GLITCH en flash, rechargé au démarrage (0.5).
- [x] Le thème Ribbon classique (palette RIBBON) sur tout le logiciel, par défaut (0.8).
- [ ] Les bandeaux à onglets du Ribbon sur les pages de SLOOP.

## Avant toute installation sur un FM-1

- [ ] Identité de paquet propre à K-GLITCH (ne pas réutiliser celle de SLOOP).
- [x] Compiler avec la chaîne JieLi + SDK AC79, vérifier les budgets image / RAM / CPU audio (0.9 : 567 Ko / 581, RAM 73 Ko / 96) ; installateur en un fichier (installer/).
- [ ] Tester sur l'appareil : son, écran (temps de rafraîchissement du plein écran), MIDI, mise à jour et retour au firmware d'origine.
