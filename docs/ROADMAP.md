# K-GLITCH — feuille de route

## Fait (0.1, hors matériel)

- [x] Base SLOOP 2.8.4 (isod89/sloop-fm1 `91f3018`), licences conservées.
- [x] Séquenceur d'effets 16 pas, AUTO, DICE/MUTATE, macros CHAOS / DENSITY / MOVEMENT.
- [x] OUT FM-1 / MPC / BOTH : effets en MIDI CC, macros en CC 85–87, aucun CC bloqué.
- [x] Écrans DESTROY / FX SEQ / SETUP (style Ribbon classique de K-WASUB), écran de démarrage K-GLITCH.
- [x] Break jungle de démo (170 BPM).
- [x] Émulateur web (WebAssembly) avec journal MIDI et Web MIDI.

## Prochaines étapes

- [x] La banque DRUM DESTROYER de 16 effets, INTENSITÉ, HOLD (0.2, testés sur l'hôte et dans l'émulateur).
- [x] Une intensité par effet (ALGO choisit, PRESETS règle), LFO piloté par MOVEMENT (0.3).
- [x] Les 16 touches comme pads directs sur les pages K-GLITCH ; CLEAR (retour au son original).
- [x] Les 3 modes : DRUMS (8 styles), CHOP (mutation du rythme : décaler, supprimer, répéter, rolls, ghosts, accents, longueurs ; RESTORE ; EVERY), CHAOS (0.3).
- [ ] Comb filter dans FILTER CHAOS ; scratch avant/arrière plus « vinyle » ; stutter 3x / 8x en triolets.
- [x] Automation de tous les effets (REC), intensité comprise, sur 16/32/64 pas ; CLEAR maintenu (OCT-) qui l'efface ; DICE de mutation du rythme (0.4).
- [ ] Découpe de vrais breaks échantillonnés (moteur SLICE de SLOOP + samples utilisateur) pilotée par CHOP.
- [ ] Plus de styles DRUMS, et des fills générés.
- [ ] Sauvegarder la séquence d'effets et les macros avec le projet.
- [ ] MPC : CC continus depuis les potards, notes de slices, profils de mapping.
- [ ] Étendre le style Ribbon classique aux autres pages.

## Avant toute installation sur un FM-1

- [ ] Identité de paquet propre à K-GLITCH (ne pas réutiliser celle de SLOOP).
- [ ] Compiler avec la chaîne JieLi + SDK AC79, vérifier les budgets image / RAM / CPU audio.
- [ ] Tester sur l'appareil : son, écran (temps de rafraîchissement du plein écran), MIDI, mise à jour et retour au firmware d'origine.
