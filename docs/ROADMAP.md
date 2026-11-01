# K-GLITCH — feuille de route

## Fait (0.1, hors matériel)

- [x] Base SLOOP 2.8.4 (isod89/sloop-fm1 `91f3018`), licences conservées.
- [x] Séquenceur d'effets 16 pas, AUTO, DICE/MUTATE, macros CHAOS / DENSITY / MOVEMENT.
- [x] OUT FM-1 / MPC / BOTH : effets en MIDI CC, macros en CC 85–87, aucun CC bloqué.
- [x] Écrans DESTROY / FX SEQ / SETUP (style Ribbon classique de K-WASUB), écran de démarrage K-GLITCH.
- [x] Break jungle de démo (170 BPM).
- [x] Émulateur web (WebAssembly) avec journal MIDI et Web MIDI.

## Prochaines étapes

- [ ] La banque DRUM DESTROYER de 16 effets (LOOP DESTROYER, STUTTER 2/3/4/8x, GLITCH, SCRATCH, PITCH, GATE, FILTER CHAOS, BEAT REPEAT, BEAT SKIP, MICRO STUTTER, TIME STRETCH, RING MOD, FUZZ, RANDOMIZER, TOTAL CHAOS) en plus des 27 de SLOOP.
- [ ] Intensité par effet (encodeur B), HOLD (verrouiller un effet), LFO sur les effets.
- [ ] MUTATE sur le rythme lui-même (décaler, supprimer, répéter des pas de batterie), pas seulement sur les effets.
- [ ] Plus de breaks préchargés ; découpe (CHOP) dédiée aux breaks.
- [ ] Sauvegarder la séquence d'effets et les macros avec le projet.
- [ ] MPC : CC continus depuis les potards, notes de slices, profils de mapping.
- [ ] Étendre le style Ribbon classique aux autres pages.

## Avant toute installation sur un FM-1

- [ ] Identité de paquet propre à K-GLITCH (ne pas réutiliser celle de SLOOP).
- [ ] Compiler avec la chaîne JieLi + SDK AC79, vérifier les budgets image / RAM / CPU audio.
- [ ] Tester sur l'appareil : son, écran (temps de rafraîchissement du plein écran), MIDI, mise à jour et retour au firmware d'origine.
