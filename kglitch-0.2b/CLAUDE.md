# CHOPPAH: notes for Claude sessions

Firmware for the M-VAVE FM-1 (JieLi AC791N): an Amen break chopper. Spec: `docs/PRD.md` (feature IDs A-*, B-*,
E-*, N-*; milestones M0–M8). Keep the PRD in step when scope changes.

## LOAF BREAKER (FLEXOR's fork)

- The repository is FLEXOR-R/loaf-breaker (public from 11 Nov 2026, FLEXOR). **Work goes to `dev`** (12 Nov 2026, FLEXOR:
  not twenty public versions a day): each push there replaces the one **draft** release "LOAF BREAKER TEST" (loaf-breaker-A.B.C-testN.fwsc; a draft,
  seen by FLEXOR only: the hub looks at the newest release, pre-releases too, and a TEST pre-release made its check fail;
  the splash "A.B.C TEST N"; no site, no soak, no tag). **A release only when FLEXOR asks** ("release", "publie"): dev
  pushed to `loaf` (`git push origin dev:loaf`, a fast-forward), whose every push is a release `loaf-vA.B.C` (0.9.1 on; 0.02-0.267 were X.YY)
  (`.github/workflows/loaf.yml`; CLAUDE.md alone releases nothing) and updates the site, https://flexor-r.github.io/loaf-breaker/ (0.261: the firmware on it
  open, no install code; tools/site_lock.py and the secret LOAF_INSTALL_CODE no longer used). Never write that old code anywhere.
- **The VST** (24 Nov 2026, FLEXOR: "synchroniser la VST et loaf breaker pour la fm1 ... toujours d'abord s'occuper de la
  fm1, me demander si il faut publier, et ensuite faire les modifs et possiblement la publication de la vst"): this
  firmware also runs as FLEXOR's plugin, in his private repository FLEXOR-R/loaf-breaker-vst (this repository as its
  `firmware` submodule, run through fm1-base's host core, host/rfm1.c with PLAT_RFM1). The order, every time: the
  FM-1 first (dev, its TEST), then ask FLEXOR whether to publish; then the VST (its submodule moved to the firmware's
  commit, the VST's changes, its TEST), published only if he wants (its version is the firmware's). Keep the host
  build (`./build.sh --host`) working: the VST is built from it. LB_BIG (the VST only): SELECT never the tempo (HOME's tabs there,
  nothing elsewhere; the VST's header and lb_tempo have it, FLEXOR: "certaines commandes ... montent le BPM").
- The changelog, in French, is FLEXOR's Claude Doc (project 6f32699a-06f8-4eaa-a41a-6f03a8b20dc9, node 1fc2737b-3a82):
  after each release (not a TEST) add a row (everything since the last one) (version, date, what changed) above the top one and put the version in the intro's
  "LOAF BREAKER 0.9.x". Keep its first part current whenever it is relevant: the summary paragraph, "Les grandes
  étapes", and "Les modifs importantes" (where things are, what was removed or replaced, shortcuts, "À savoir").
- Identity (9 Nov 2026): its own, `FM-1_2623` (fm1.toml `id_prefix`; CHOPPAH's is FM-1_86), files `loaf-breaker*`
  (`name`: loaf-breaker.fwsc, app/build/host/loaf-breaker_host), `fmhub.json` LOAF BREAKER's (author FLEXOR). CHOPPAH
  stays credited (README, site credits, the card's text, fm1.toml). The hub checklist: docs/hub/SUBMIT.md.
- Menus (9 Nov 2026, 0.177): ENV is the song (TRACKS, PLAY, DROP, BUILD); RESET is GLO's sixth part (`RST_ON`, ENV's
  reset code in track.c); the knobs' functions are EDIT KICK › KNOBS only. SYSTEM › SETUP K3 MODE (`set.novice`): BEGINNER keeps
  the essentials, as FLEXOR chose them on a checklist (0.229; https://claude.ai/artifact/75VV4U2fH5iouT8789feZN, its
  store `choix/beginner`): the KICK three pages on OUT's part (`kick_easy`, `KEASY[]`, `keasy_pg`: KICK, SHAPE, MIX; the foot's tabs, the
  header's KICK SN HH CP with SELECT round them, 0.230), the
  drums PRESET / FX / LEVEL / DECAY, SEQ as EXPERT's but the break's two halves always on top (0.233), no step menu, no GROOVE in its lanes' round (0.231) and K4 LENGTH the kick's alone (16/32/64, a drum
  lane 16; BEGINNER turned on sets every drum 32 (0.9.32, FLEXOR; the kicks 16 in 0.246-0.9.31), `novice_lengths`; a new pattern's every drum 32 too, whatever the mode, `PAT_ALL32`), the FX chain two slots (`fx_nslots`, the others
  muted in `fx_apply`) and a short list (`FX_EASY`) then MAGIC EQ, MAGIC MBC; SONG whole since 0.9.22 (FLEXOR; `trk_knob_ok` always 1), `glo_knob_ok`, the
  held white keys (no knobs), the black keys' knobs as EXPERT's (0.232); blank knobs say EXPERT MODE (`easy_no`; the DST and SUB keys' restriction gone in 0.244); `KP_ADV` no
  longer reached. Gone in 0.229: the FX scenes, SEQ's BEAT presets, MB KICK and MB OTT. A new FM-1 asks its mode
  (tutor.c `TUT_ASK`, set.tut 0xFD; the simulator with LOAF_ASK=1), and so does RESET ALL, as a first start (0.236:
  after its screen, `ui` cleared and `app_ui_init`: the start screen, then the question). The host simulator starts EXPERT (scenarios, the
  manual's screens), the FM-1 and the web emulator BEGINNER. The manual opens with "Start in five minutes" (the hub card's `cheat` link).
- Screen and knobs (0.178): the screen's rows are remembered as hashes (`rows_send` in common.c): a full-width strip
  sends only its changed rows. Anything drawing on the screen other than through `cv_blit` must forget its rows
  (`rows_forget`; `lcd_fill` is wrapped). `changed()` applies once a pass (`apply_flush`); a kick-only or FX-only change
  uses `changed_kick()` / `changed_fx()`. SYSTEM › MIDI K3 METER (EXPERT only, 0.213) shows CPU / IMG / FR on the device. CI's 30-minute
  random soak (`tools/soak.py`, seed = run number) at every release is off since 16 Nov 2026 (FLEXOR: "pas besoin des soak test, juste publie"; loaf.yml `if: false`).
- Names (10 Nov 2026, 0.191): what the code calls TERROR, TERROR 2, TERROR + the screen and the docs call DISTO,
  SCREAM, DISTO + (D+ TYPE: FOLD CLIP CRUSH RECT TUBE, 0.255 HALF SINE WRAP FUZZ, `KICK_T_TYPES`; D+ SCREECH ..); the black key DST. Keep newcomers' names on the screen; TERROR stays the
  genre (README's tagline: HARDCORE - GABBER - BREAKCORE - TERROR - SPEEDCORE, FLEXOR 11 Nov 2026) and a STYLE.
- Breaks (10 Nov 2026, 0.193; 0.9.43, FLEXOR: the bank always sorted by BPM, slowest first, by prep_breaks.py whatever breaks.txt's order, the same BPM in its order: a new FM-1 starts on the slowest; a saved one on the break it was left on, scenario brkorder): FLEXOR chose the bank on a listening page of all 70 breaks as the FM-1 holds them
  (https://claude.ai/artifact/MCakoDjuGTXjMLFiobM1k3; the choice in its store, `tri/choix`): 9 breaks, NUSKOOL first (FLEXOR's order; LO-FI out in 0.200 for room; AMEN15 174 out in 0.9.30, FLEXOR, for the drums' 64 steps: 8 breaks, 298 KB). 0.9.34 (14 Nov 2026, FLEXOR's Break Lab https://claude.ai/artifact/RqYzxYRDr9nMWw98PXnesW, store `lab/choix`: names, order, the ones out): AMEN20 164, AMEN19 172, UPDOWN 165, DETEST 175, LOUDER 175, SATAN 200, PANTHER 170, SHUFFLE 190 (0.9.38; OBEDIENC165 in 0.9.37, now the library's, `-=`), FOCUZ 175 and TIMES 175 (0.9.43, FLEXOR's Break Lab 2 https://claude.ai/artifact/PgZpqM7PwiTPx4qCi6AsPD, store `lab2/choix`: the library's 98 as the FM-1 holds them, the flash left worked out live; AKIRA 190 out in 0.9.40, THINK 114 out in 0.9.43, both the library's: ten; 0.9.50, FLEXOR: DETEST 175 out, TIMES 175 out for Dolphin's BANGPHS 155, the slowest so a new FM-1's first: nine, 548-552 KB image; 0.9.67, FLEXOR: SHUFFLE 190 out (the library's) for DJ Hidden's NIGHT 175: 272.6 KB, image 567,356 B), his seven of `breaks/flexor/` on an even grid (`*=`) louder than the others: every break of `breaks/flexor/` at -12 LUFS (0.9.42, FLEXOR "tout mieux égaliser": the level as heard, BS.1770 K-weighted and gated, prep_breaks.py `loudness_db` and the page's `loudnessDb` alike; -12 dBFS RMS before, up to 5 dB apart heard; prep_breaks.py `FOLDER_LOUD`, FLEXOR: his DJ library, mastered alike; a line's `@X` overrides; `@0` in 0.9.35, their files' level, too loud; tests/lib_same.sh holds the site's library to the firmware's); THEWORM, NUSKOOL, JUNGLE, APACHE out; 282.3 KB. `breaks/flexor/` (14 Nov 2026): FLEXOR's own breaks, sampled from artists' records (not royalty free), sliced on an even grid (breaks.txt `*=` / `-=`, FLEXOR: EVEN GRID); his first three were in a TEST, then taken out for him to make them again; his choosing page https://claude.ai/artifact/JqvNy6DuTPBk6WBZVDZHww (store `choix/breaks`; he wants more options on it). The bank is not final (FLEXOR, 14 Nov 2026): PANTHER, AKIRA and others will go as he samples more; the aim is the best set later (keep the out ones in the library, `-=`). Every break added to `breaks/flexor/` (in the firmware or the library): a row in breaks/flexor/README.md's table (`Artist, "Track" (kind)`: the site library's full name, 0.9.40, FLEXOR; break_lib.py `titles`, index.json `title`; the FM-1 keeps the short name), its artist named in `fmhub.json`'s `sounds` line too, at most 300 characters (FLEXOR, 14 Nov 2026; docs/hub/SUBMIT.md).
- CHOPPAH's breaks followed (24 Nov 2026, FLEXOR: "les breaks mis à jour par profikid dans CHOPPAH doivent être à jour
  sur LOAF BREAKER"; "tous les breaks ... ON GRID et pas ON HIT"): CHOPPAH 1.5.3's AMEN (bars 3+4 of the record, cut at
  the drummer's own tempo, 144.72 BPM: prep_breaks.py `pick_bars` / `downbeat`, BARS "3+4" in breaks.txt; break_lib.py
  reads it) in the library as AMEN 145, on an even grid (CHOPPAH's are on the hits: `played_slices` not taken); THINK
  114 at 22 kHz again (16 kHz: never). Every line of breaks.txt `*=` / `-=`: the firmware's, the site's library and the
  VST's bank all on the grid.
- Bank and strip (20 Nov 2026, 0.9.70, FLEXOR's Break Lab 3, https://claude.ai/artifact/XTFcM7rNcos7EK8c2hDHQw, store `lab3/choix`; supersedes the bank above): 8 breaks, by BPM: AMEN10 135 (a new FM-1's first), AMEN05 158, AMEN20 164, UPDOWN 165, AMEN06 169, AMEN19 172, AMEN13 173, FOCUZ 175 (LOUDER, SATAN, PANTHER, BANGPHS, NIGHT to the library); every break of breaks.txt on an even grid (`*=` / `-=`, FLEXOR: "absolument tous") and at -12 LUFS (prep_breaks.py / break_lib.py default, -16 before; the BREAKS page's level and EVEN GRID defaults too); never 16 kHz (FLEXOR: "terriblement naze"). Image 550,628 B. The strip over the master's wave (screen.c draw_wave): SYSTEM › DISPLAY's PRESETS, RGB (the start), 3 BAND, BLUE, SERATO, GREY (as before), common.c `strip_style` in set.bta_codec (0xC0 | it, untagged RGB; RESET SCREEN), coloured from app.c `wave_b` (pool: each column's level as heard^0.8 and its three bands, wave_build) over the beats' grid; WAVE TYPE (set.wstyle) RGB, SERATO, 3 BAND, BLUE (`WS_*`, common.c `wave_col`: the CDJ view, the kick's and the drums' pictures, the strip; the Studio's editor.js `colOf` / drawRGB alike). Scenario strip.
- Patterns (10 Nov 2026, 0.197): 64, four groups of 16 (`NGRP`, `NPAT_ALL`); the group in RAM is pats.p/kn/hits and
  steps.b (`set.pgrp`; `set.pattern` its slot; `pat_cur()` 0..63; `pat_go()` brings one in). Each group has its own
  objects (OBJ_PG0/KG0/SG0 + g; NG0's first is KICK 11-20 since 0.9.2, the others unused); SAVE (`store_all`) writes 11 (with OBJ_KLAB, OBJ_KMEM2, OBJ_KFXM, OBJ_DX, OBJ_KX2 since 0.9.45). Every drum 16, 32 or 64 steps (0.9.30, FLEXOR): the snare's, hat's, clap's bars 3-4 of all four groups in `dx` (OBJ_DX = OBJ_NG0 + 2, read through `dx_of`; `dx_tp` for a drop's pattern from another group), 64 = PAT_LEN bits 5-7 (`LEN64`), SEQ lanes S3 S4 H3 H4 C3 C4; their steps 33-64 share 1-32's stepb. Leaving a group with unsaved changes while playing is
  refused ("SAVE OR STOP", FLEXOR's choice: never a cut in the sound). Track items, `drop_after`, `ui.drop_pat` and
  `pat_next` are 0..63 (or 1..64); `drop_to` stays a slot (0xFF: another group's, read in at the drop).
- MELODY and KEYS (0.198-0.208) removed in 0.209 (FLEXOR: "kicks and drums only"): perc.c's PV_STAB is an empty place
  (perc_t's layout in the saves), DR_STAB gone (NDRUMS 4; `NDRUMS_SAVED` 5 for pats.hits), OBJ_NG0 + g no longer
  saved or read (SAVE wrote 7 objects; 8 since 0.9.2, OBJ_NG0 holding KICK 11-20), settings' pstab/smem/sslot/mel_vol/mel_ver unused, st[k][2] bits 3-7 unused.
  Don't bring a melody back without FLEXOR asking.
- Domains (10 Nov 2026, 0.201; 0.200 is the layout before, FLEXOR may want it back): the header and the tabs name the
  domain (`DOM[]`, `dom_name()` in main.c): LIVE, SOUNDS, PATTERN, SONG, MIX, EFFECTS, SYSTEM. SOUNDS' sources: KICK,
  DRUMS (einst 1-3), BREAK (PG_GLOBAL `GP_BREAK`, GLO's old PITCH code, K4 LEVEL); SEL + EDIT the
  next; EDIT again: the KICK's sections only (0.227, FLEXOR: the 0.226 round to the drums taken back), a drum's the next drum. PATTERN's GROOVE is PG_GLOBAL `GP_FILL` (SWING alone since 0.244: FILL gone), reached from SEQ's ALGORITHM and SEL + ARP;
  ARP itself is the OVERLAY page again (0.244, FLEXOR: ovl.c, PG_OVL; its 16 effects since 0.9.55, FLEXOR: DIVE RISE RADIO CRUSH LO-FI DIST RING CHOP GATE DUCK PAN BOOM JET, then KICK X2 X4 TRI (OV_KX2..: the render's `kick_ovl`, as the black keys; no sound in dsp/ovl.c), WAH WOBBLE REESE gone; an older save remapped by app.c `ovl_order_fix` (bk[23] ARPR_TAG 0x7D, 0x7C the order before); 0.9.27: its effect touched last is the ARP key's, `ovl_last` = set.ovl_pad & 15, and its keys of the effects on lit);
  LEDs light EDIT / SEQ for those two. EXPERT's SEQ (0.234): BREAK above the first lane (`ui.seq_brk`, `SEQ_BRK`), K4
  the break's rows shown (`brk_view()`, settings' `brkv`: the old mel_ver byte, tagged 0xB0, since settings_t is at
  3840 bytes: no field can be appended any more without a new magic or freeing one; `rsv2`'s 8 bytes hold the ARP keys since 0.9.45). SYSTEM's parts `GLO_ORD` (MIDI, SETUP, SETTINGS, EFFECTS, FX SET, LIGHTS, RESET; their tabs narrower, `tab_glo`, so the seven fit); RESET ALL is
  RESET's last item, EVERYTHING. RESET (0.239, FLEXOR): K4 picks, K2 five clicks, then GLO within 5 s (`ui.rst_ask`,
  `rst_confirm`); ALGORITHM (and SELECT, 0.9.2) there goes to SYSTEM's parts. MIX's second part is SENDS (FX, DRUMS, K3 DUCKS since 0.9.64 (FLEXOR: BREAK / PERC / BOTH, `set.duck_what` 0 / 2 / 1, DYNAMICS' K4 and the knob function DUCK's K4 the same; PERC the drums alone, the break untouched); nothing in 0.9.51-0.9.63 (VINYL BREAK's THRU there in a 0.9.54 TEST: FLEXOR took it back; VINYL's THRU stays its K1 held) (0.9.29-0.9.50 the ARP's THRU for all; now each ARP effect its own, the OVERLAY page's K2: app.c `arp_route` / `arp_route_set`, bk[19..22] two bits an effect, bk[23] ARPR_TAG, untagged the old ovl_pad bits 6-7 for all; the render's `ovl_process(..., mask)` three passes: the break's, the kick's, the whole mix's),  K4 ROLL, the three rolls' THRU BREAK KICK BOTH, `roll_thru_turn`, set.pads.rsv[0], 0.9.40, FLEXOR: the rolls' K2 is HOME's break filter, to roll and filter at once; LEVELS' K4 too until 0.9.30. Its BREAK / KICK route the kick around the chain: the limiter then after it joins, `brk_fx_late_limit` + `brk_fx_add_dry`, 0.9.31, FLEXOR: it sounded fatter, unlimited).
- Tab names (18 Nov 2026, 0.9.68, FLEXOR, on https://claude.ai/artifact/PDp6N5gHaMuANxmqZaihad, store `choix/menus`; "on touche pas au menu des kicks": the KICK menu's names stay): what these notes (and the code) call OVERLAY, SEQ's SETTINGS, SONG's BREAK and DROPS, MIX's SENDS and DYN, SYSTEM's SETTINGS (GLO_HSET), EFFECTS and FX SET, a drum's MORE, the screen and the docs call ARP, GROOVE, BUILD BRK, DROP BANK, ROUTING, DYNAMICS, DISPLAY, SCREEN FX, SCREEN SET, SPACE (main.c DOM, GPART, tab_glo (pad 1 now, the seven fit), draw_tabs (SONG's tabs pad 3 gap 2 since 0.9.69, DROP BANK in full), page_say (ARP's first tab: EFFECTS in the header, 0.9.69); track.c TPART; perc.c PPAGE_NAME). FLEXOR wants other pages' tabs reorganized later; the FILTERS page (MIX: break and drum filter kinds, resonance, KICK SWITCH) stored on that page, not confirmed yet.
- Knob presets (10 Nov 2026, 0.203): STYLE's PRESETS goes on past the eight styles to PRESET 1-4 (track.c, `KPRESETS`):
  `steps.kpre[4][9]` (appended to steps_t: four codes, four values 0..200, stored), the same for every kick; an empty
  one gives back `kcust` (the knobs before PRESETS turned); SAVE on STYLE with one picked stores them (`kpre_store`).
- Knob style FLEXOR (16 Nov 2026, 0.9.62, FLEXOR): ASSIGN's ninth, last before PRESET 1-4 (track.c KSTYLE, KSTYLES 9): DRIVE, D+ TYPE, SUB DRIVE, DISTO; SUB DRIVE a knob function of its own (SK 21, code 22: the SUB layer's DRIVE gain `fa`; 0.9.66, FLEXOR: the gain alone, never the switch: SDR turns it on and off). On KNOBS and ASSIGN PRESETS the knobs' presets (styles, PRESET 1-4), on K1-K4 SET the knob's function, SEL + PRESETS the kick there (0.9.66, FLEXOR).
- KICK LAB (10 Nov 2026, 0.204): the KICK menu's sixth section LAB (TAIL, TAIL +, STACK, TOOLS; kick.c klab_*),
  `kick_lab_t` (kick.h) per kick in its own object `klab` (OBJ_KLAB 0xB6000: cur + mem[10], swapped with the kick in
  kick_mem_go, copied with COPY K n, off with a built-in PRESET / RESET); engine `kick_set_lab`, `kick_set_stack`
  (a second kick voice, kv2, until STACK's hand-over), `kick_set_solo`. All 0 renders bit for bit as 0.203: keep it so.
  The tabs drop the "KICK" label on the KICK menu (six sections).
- END (19 Nov 2026, 0.9.70, FLEXOR: "1000 ms ne suffit pas"): KICK › TAIL K4, kick_lab_t.end 1..49 40 .. 1000 ms by 20 (as saved before), 50..KICK_END_MAX (89) on to 3 s by 50 (kick.c kick_lab_ms KLM_END); scenario kickend.
- The app (21 Nov 2026, 0.9.71, FLEXOR: "une application pour Android"; his choices: the site installable (a PWA, not a native app), all of it, Live first): app/site/app.html (site app/, the start page; its Update tile the installer, install/, 0.9.74, FLEXOR: "pas possible de mettre a jour le firmware"), app/site/live.html (site live/: fm1-base's mirror decoder copied, its remote cmd 0x54 for every button, key, knob, MASTER; DROP = SEL + ENV; nothing in the firmware: mirror = true already), app/site/manifest.webmanifest and sw.js (the site's root; network first; never firmware/ or breaks/lib/), img/app-192.png, app-512.png (tools/make_site_logo.py); tools/site_style.py `page_of`, `pwa` (every page: the manifest, theme-color, the worker) and the nav's Live, App. Over USB only (Chrome Android's Web MIDI), never Bluetooth. Test tests/live_page.sh (tests/live_page.mjs: Chromium at a phone's size against `loaf-breaker_host --serve`; skipped without Playwright).
- The site's VST page (24 Nov 2026, FLEXOR: "faire un onglet VST sur le site, et mettre en avant le côté VST"):
  app/site/vst.html at vst/ (site_style.py page_of, its pictures app/site/vst_img/ copied there), first in the nav
  (`header`, `restyle`), and the front page's first link. Its Download points at the VST's releases (FLEXOR-R/loaf-
  breaker-vst). Public docs checked against these notes the same day (README, landing, MANUAL, QUICKSTART, controlmap,
  breaks/ READMEs, SUBMIT, fmhub.json): keep them in step when a control moves.
- The split (21 Nov 2026, 0.9.71, FLEXOR, from CHOPPAH 1.5 / 1.5.1, with profikid's go-ahead (taken out a few hours, the branch `split`, until he agreed); his choices: HOME's third tab, EXPERT only, the white keys the split's own): PG_SPLIT (common.c, after PG_HSET; `HOME_PG`), global.c `split_value` / `split_turn` / `draw_split`, main.c `split_key` (a white key latched its half: brk_half) and `break_turn` (ALGORITHM: B with bar 2 latched) until 0.9.73 (FLEXOR: "je reste bloqué sur un seul loop", "aucun verrouillage": both gone, the loop always A + B, ALGORITHM A's break, B K2's; brk.c's half kept, never set); brk.c CHOPPAH's two key lanes (`kl[]`, `lv`, `lane_for`, `fold`, `half_on`; LOAF's DECAY and BREAK TIME's `bsl` kept), brk_break_t.split; app.c `split_on`, `split_b_of`, `bar_decode`, `split_slices`, `split_load` (A's bar, B's repitched, both in pcm_a), each break A its own split since 0.9.72 (FLEXOR: "chaque break a un split indépendant en mémoire", OFF by hand only): `kx2.sp[24]` (SP_TAG2 0x5C, in the KITS' old bytes; bit 7 on, bits 0-4 B + 1, bits 5-6 the bars; `sp_get` / `sp_put` / `split_on_of` / `split_bars_of`; 0.9.71's one split, SP_TAG 0x5B and split / split_b / split_bars, read once into the break it was saved on) and its own keys `kx2.wks[24][16]` (appended; `wk_row`, rows 25..48; wk's row 24 unused since); a break change clears the latched half (break_ask); screen.c `hdr_brk_name` "A+B"; probes split split_at half half_ask layer split_b; engine test test_split; scenario split. RAM 97,900 of 98,304 after it; then (0.9.71, FLEXOR: "comment libérer de la ram", his choice) the dub line 0.55 s (CH_DUB_LEN 24256: 3/8 down to 123 BPM, 0.70 s before), the drums' room (perc.c rm_c / rm_a, brk_math.h `BRK_POOL`) and the kicks' FX sections (`kfxm`) in the pool: RAM 88,012 of 98,304, pool 336,724 of 344,064.
- CHAOS (24 Nov 2026, 0.9.75, FLEXOR: "un mathcore lite ... pour rajouter du chaos", the break, not the drums): HOME's last tab (PG_CHAOS, after SPLIT; BEGINNER too; HOME passes over it while the TUTORIAL runs: `tut_on`), global.c `chaos_value` / `chaos_turn` / `draw_chaos`; brk.c `brk_auto_roles` (CHOPPAH's MATHCORE Slice Map, `mc_auto_roles`, MPL: credited) into brk_break_t.roles at every break_load (the split's too), `brk_chaos` (packed: amount, MATH N, rolls, seed) applied in on_step after the generator: `ch_swap` (another slice of the same role), reverse from 50, ratchet ×3 snares / ×2 hats, MATH's q (each bar's first N 16ths), seeded by the loop step (`ch_hash`: the same chaos every loop); kx2.ch_tag / chaos[4] (CH_TAG 0xC4, in rsv4), `chaos_push`; RESET ALL off. Probes chaos cmath crolls cseed rkick rsnare rhat; test_chaos; scenario chaos. Image 555,440 B, RAM 90,172.
- CHS (24 Nov 2026, 0.9.76, FLEXOR: "une nouvelle assignation de touche noire avec un on / off pour le chaos"): `BKF_CHS`, last in BKF_* (a press: HOME › CHAOS off, its settings kept, then on; on with nothing set, CHAOS 40; lit while CHAOS is heard), CUSTOM / SEL BLACK / REC only; kx2.ch_sw (CH_SW 0xE0 | 1 off, rsv4's first byte; anything else on), app.c `chaos_off` in chaos_push; a CHAOS knob turned turns it on; RESET ALL on. Probe chsw; scenario chskey (and the scenarios turning K3 left of R8 one more). Image 555,856 B.
- BACKUP (24 Nov 2026, FLEXOR: "de quoi envoyer ce que j'ai fait sur le VST sur la machine ... un système de traduction
  et de synchronisation"): fm1-base's backup and restore over USB, fm1.toml `backup = true` (device/backup.c; the site's
  backup/ page, tools/fm1_backup.py; the VST sends and fetches through it). app.h APP_BACKUP_OBJS (every saved object
  but OBJ_PANEL and OBJ_BLE: settings, patterns, steps, the four groups, NG0..NG0+3, KLAB), magic "LBRK" version 1,
  APP_BACKUP_BUF `lb_bu_buf` (3840 B of RAM: the pool has no room), app.c `app_backup_*` (playing: refused, STOP FIRST;
  flush: store_all; hold: `bu_held` stops store_all; restored: plat_restart, APP_RESTART). Not the player's breaks nor
  the drum samples (their own pages). Image 559,572 B, RAM 94,716. Scenario backup.
- ATELIER (24 Nov 2026, 0.9.75, FLEXOR: "un onglet secret sur le site pour ... installer les firmware de test ou les firmwares perso du git"): the site's atelier/ (in no menu, noindex; the app page shows its tile after five taps on its title, kept in localStorage `loaf-atelier`): tools/site_style.py `atelier` copies the restyled install/ and puts app/site/atelier.js in its module: a .fwsc or a zip holding one (an Actions artifact) dropped or chosen, LOAF BREAKER's identity only (meta.product), then fm1-base's own install. PERSO builds: loaf.yml on any branch but loaf / dev (pushed as `perso/NAME`, or Run workflow on it; before, a hand run on another branch made a loaf-v release): a draft "LOAF BREAKER PERSO NAME" (tag loaf-perso-NAME), its splash "A.B.C PERSO". Test tests/atelier_page.sh.
- DROP BREAK NEXT (20 Nov 2026, 0.9.71, FLEXOR): SONG › DROP K3 SAME, NEXT, then each break; `DBRK_NEXT` 0xFF in set.drop_brk (an older firmware reads it as SAME), track.c `trk_drop_brk` the break after set.brk, round; probe dbrk; scenario dropnext.
- White keys per break; SEL WHITE and KITS gone (20 Nov 2026, 0.9.71, FLEXOR: "assigner des réglages de length de white key pour chaque break afin de mieux chopper ... tu peux supprimer la fonction d'assignation des white keys"; KITS too, his choice): set.keys is the break playing's, every break's in kx2.wk (24 x 16, packed 2 bytes: LENGTH, DECAY, MODE, PITCH, bit 15 set; appended to OBJ_KX2 after `selrec`), app.c `wk_follow` (break_ask), `wk_store` (store_all), `wk_fill` (an older save, RESET WHITE KEYS, RESET ALL); slice effects (set.slx) still shared. ARP's tabs ARP, BLACK, SEL BLACK (`ui.opage` 0..2); SETUP's PRESETS the skin again; RESET's KITS gone; kx2.kit and kx2.rsv[0..47] unused now (kept for the layout), set.rsv6[2] bit 3 free; the notes on SEL WHITE, KITS and the TRACK shortcut below are history. Scenario wkbreak.
- TAIL MODE SPLIT, DIRTY (20 Nov 2026, 0.9.71, FLEXOR: "rajoute les deux"; the Disto Lab's five waveshapers, https://claude.ai/artifact/8JWJMGtWgdtF8HrVJj854a, "rien ne rajoute réellement quelque chose": none added): KICK › TAIL K1 CLASSIC, CLEAN, SPLIT, DIRTY (kick.h `KTM_*`, ui/kick.c `ktm_turn`, `KTM_NAME`); SPLIT the body as CLASSIC then a clean sine on the SUB's phase (kick.c kv_tick: `sp_n`, `sp_env`, `sp_g`), DIRTY the drive 0.35x on the hit rising to 3x (`dr_k`, `dirt`, DISTO's `h` too); their setting TAIL + K4 SPLIT AT / DIRT RISE (`kick_lab_t.amt`, its old `rsv`; KLM_SPLIT, KLM_RISE; 50 when picked); host `kset tmode 0..3`, `kset tamt`; probe kl_amt; scenario tailmode.
- KICK menu by anatomy (10 Nov 2026, 0.205, FLEXOR's choice): KSECT ATTACK, BODY, TAIL, DIRT, FX1, FX2, MORE (kick.c;
  0.9.29, FLEXOR: FX1 and FX2 sections of one part each, ALGORITHM there the effect's parts (`fx_part_turn`, the FX page's chips), no ksect_row on them; MORE (KNOBS until 0.9.28) 21 27 22-25 31 11 12: KNOBS, ASSIGN, K1-K4 SET, TOOLS, EQ B, EQ S, nine parts (KSECT rows of 9; `ksect_row` narrows its margins to fit); KNOBS' STYLE named ASSIGN in 0.9.2);
  app_ui_init opens it on ATTACK's PRESET (0.253, FLEXOR: KP_PRESET, part KP_PART_PRESET, K1 the built-in kicks alone; ui.ksect 0; 0.205-0.252 on BODY's MIX); the parts in a row at the
  picture's foot (0.264, FLEXOR: SELECT the tabs at the top, its sections, kick_sect_turn; ALGORITHM those at the
  bottom, its parts; EXPERT's drums: SELECT round SN HH CP, the KICK by SEL + EDIT; 0.266: EXPERT's EDIT goes round the sounds, KICK SN HH CP) (kick_chip; 0.207: a full-width bar of tabs, FONT_T if it fits, the wave up: its middle y 45, 29
  px each way). Views (ui.kview, EDIT held): RGB (kick_rgb, 500 ms), WAVE, then CLASSIC (ui.kclassic); 0.231 the drums'
  big view too (perc_rgb into kv_rgb, shared: kv_sig / pw_sig say whose), one state for all.
- HOME (0.9.2, FLEXOR: the main page): PERFORM, GENERATOR (SETTINGS its third tab until 0.9.55; 0.9.56, FLEXOR: SYSTEM's part after SETUP, global.c `GLO_HSET` in GLO_ORD, PG_HSET still its page, SYSTEM's tabs drawn on it, no SYSTEM label so the six fit; the TUTORIAL's HOME lesson GENERATOR, PERFORM, HOME held twice: 30 steps) (PG_HSET, global.c hset_*: K1 VIEW `ui.blocks` kept in
  `bkeys.bk[BK_VIEW]`, K2 WAVE TYPE `wstyle`, K3 WAVE ZOOM `wzoom` (SETUP's K2 before), K4 BREAK TIME (MIDI's K4 before)).
  Navigation (0.9.2): on pages with tabs SELECT the top ones, ALGORITHM those below (EFFECTS: the slot / MAIN MORE MOD;
  SONG, MIX, SYSTEM, OVERLAY: their tabs; PATTERN: SELECT the lanes as ALGORITHM, 0.9.3); where those two move everywhere,
  the page's button again goes HOME (FX, ENV, LFO, GLO, ARP, SEQ (held: the clear, as before); not EDIT, not the kick's FX; GLO still confirms RESET and quits the TUTORIAL). The drums go AROUND the
  FX by default (`drums_chain` 0 in the defaults; a saved THRU stays). On HOME, SEL + PRESETS the kick (0.9.26, FLEXOR;
  elsewhere the pattern).
- Release notes (24 Nov 2026, FLEXOR: "mettre à jour aussi le github"; this session can't edit releases): docs/NEXT.md
  lists, for the players, what the next release adds; loaf.yml puts it at the top of the release's notes (its
  comment stripped). Add a line there with each change worth telling; empty it (the comment kept) after a release.
- **A saved setting never changes at a restart** (FLEXOR, 12 Nov 2026): defaults are for a new FM-1 and resets only;
  never force a value at power-on once it has been saved. The one exception, his own (0.9.73): the breaks' PITCH (`set.pitch`, OCT- / OCT+, SOUNDS ›
  BREAK K1) is 0 at every start (app_init; probe bpitch, scenario pitchboot).
- KICK 1-20 (0.9.2, FLEXOR): `KICK_MEMS` 20, `KMEM_A` 10 (the first ten's arrays as saved before); 11-20 in `km2`
  (OBJ_KMEM2 = OBJ_NG0, pool RAM); always reach a memory through `kmem_at`, `ksmem_at`, `kx_at`, `kxh_at`, `klmem_at`.
  Each kick's PRESET name: `kick_lab_t.pre` (1 + OUT K4's place; 0 the memory's own built-in, `kpre_mem`).
  Each kick's FX section (EQ B/S, FX1, FX2: set.keq, kfx, kfxx, kmbc) is its own since 0.9.26 (FLEXOR): `kfxm`, 16-bit
  packed, OBJ_KFXM = OBJ_NG0 + 1 (0xBA000), `kfx_put` / `kfx_get` in kick_mem_go and COPY; SAVE writes 9 objects. In the
  KICK menu PRESETS is the kick; SEL + PRESETS K1-K4 SET's function, ASSIGN's style; in its FX section (0.9.29, FLEXOR) PRESETS the effect (FX1, FX2; nothing on EQ B, EQ S), SEL + PRESETS the kick. SEL + ARP there (and on a drum's page) copies, then on another kick pastes (`kick_clip`, `perc_clip` in main.c; `kick_copy_from`), GROOVE elsewhere. SEL + ALGORITHM where ALGORITHM is the break: BREAK TIME (`sel_bt_page`); SEL + PRESETS the kick on HOME, PATTERN, BREAK, MIX, OVERLAY, SYSTEM but SETUP / EFFECTS; SETUP's PRESETS SEL WHITE on / off (0.9.70, FLEXOR: off for every mode, EXPERT only; app.c `swk_on`, set.rsv6[2] bit 3, untagged OFF; off, ARP's SEL WHITE tab is gone, SELECT passes over it; the skin before); SEQ's ALGORITHM every lane top to bottom, round (`seq_lane_step`), SELECT the views FULL, DRUMS, SETTINGS (`seq_view_turn`; SETTINGS is GP_FILL: SWING, BREAK ROWS, FOLLOW in bk[17] `BK_SEQ`; 0.9.31, FLEXOR). SONG › DROP K2 AFTER: SAME (0.9.55, FLEXOR: `DROP_SAME` 0xFF, the pattern playing again), NEXT, P1-64. TRACKS: a DROP item's K1 its build's kick, K3 its STUTTER (`b_stut`, bit 0 of its `trk_brk` byte), K4 GAP KEEPS (`b_gapm`, bits 1-2: NOTHING, BREAK, DRUMS, ECHOES); FLEXOR wants more drop settings later. SONG › DROPS (0.9.58, FLEXOR, tpage 7, last): DROP 1-8, each a whole drop (`steps.dm[8][12]`, dm_put's order: DROP's, BUILD's, BREAK's bk[18], [11] its GAP KEEPS, BREAK K3), the one selected in set (`steps.dsel`, `drop_go` swaps as kick_mem_go; store_all puts it back first; DM_TAG, an older save every drop as its settings, `drops_init`, RESET ALL too); keys 1-8 / K1 pick, K2-K4 BUILD, STYLE, AFTER; SEL + ARP copy / paste (`drop_clip`); a track's DROP item: K2 END, EMPTY, DROP 1-8, DROP, P1.. (its drop in bits 3-6 of its byte, `trk_dropn_of`, selected as its build begins; `trk_dbars` its bars); JET's line OVL_LINE 256 (it reads 208 at most) freed 3 KB of pool for it. SONG › BREAK (0.9.50, FLEXOR, tpage 6 after BUILD, TORDER): K1 the break in a build STILL / ROLL (main.c `build_stut`: the STUTTER style's R8 R16 R32 roll, over any style), K2 ROLL FROM (start, half way, last quarter); bk[18] `BK_BROLL` 0xE0 | bits.
- TERROR and SPINE, the seventh and eighth built-in kicks (0.9.32, FLEXOR, chosen by ear: PRD A-54i; SPINE TERROR with DRIVE 50,
  D+ TYPE RECT, SUB 766 ms): `KICK_PRESET_TERROR`, `KICK_PRESET_SPINE`, their kick_t part in
  kick.c PRESET / kick_preset, the rest (SYNTH, TAIL + DRIVE, KICK EQ LOW; TERROR's TAIL MODE CLEAN, 0.9.35) in app.c `kt_extra` (ui/kick.c `kpre_extra`); the kick memories
  begin as the eight in order, round (0.9.33, FLEXOR: KICK 7 TERROR, KICK 8 SPINE; 0.9.39: the eleven, `KMEM_ORD`; `kmem_extra` / `kmem_extras` their rest and
  their PRESET stored, on a new FM-1, RESET ALL, ALL KICKS; a saved memory never changed: a PRESET never stored is read as
  the first six, round, `KICK_PRESETS_MEM`). Kick listening pages: the host's
  `kset NAME VALUE` (ui/main.c) sets a kick exactly as its knobs show it.
- Built-in kicks remade (14 Nov 2026, 0.9.39, FLEXOR, by ear on https://claude.ai/artifact/GrjudTKC9s5nDRimcGXYEx, store `choix`;
  PRD A-54k): K1's round `KICK_PRESET_ORD` 909, 909 DRIVE, TECHNO, EARLY HC, ROTTERDAM, INDUSTRIAL, TERROR, SPINE, GABBER,
  MILLENNIUM, DOOMCORE; DISTORTO out of it (table index 5 kept, `KICK_PRESET_DISTORTO`: a kick saved from it keeps its name);
  the table's indices never move (`kick_lab_t.pre` stores them): append new kicks at the end. Each preset: kick.c PRESET +
  PRESET_X, app.c `KT_X` (SYNTH, LAB, KICK EQ: flat since 0.9.47 but TERROR's and SPINE's LOW +5 dB, FLEXOR: never a band above +5 dB on a built-in, now or later, `KT_EQ_MAX`; `on` 0: ROTTERDAM, DISTORTO, the EQ left as it was). The memories `KMEM_ORD`
  (ROTTERDAM KICK 4, the start's `kslot` 3; EARLY HC KICK 5). His taste for new ones: drive and distortion, SUB and tails
  medium to long, TAIL MODE CLEAN, tails not shrill (a page measures above 3 kHz in the tail: TERROR's -40 dB the mark).
  A built-in picked (K1 / OUT K4) turns DST back on when it has DISTO (selk_hi bit 4, 0.9.40, FLEXOR), DRV with a DRIVE,
  SUB with a SUB FX (bit 6, 0.9.43); SUB DRIVE is the SUB layer's effect, so the preset's own.
  The host's `kset LAYER.level|pitch|decay|tone|fx|fa|fb|wet`, `syn.*`, `lab.*`, `t.*`, `t2.*`, `csub` set any of a kick raw.
- WAVE TYPE (`set.wstyle`: REKORDBOX / SERATO; HOME › SETTINGS K2 alone since 0.9.25, GLO EFFECTS' K3 before; SYSTEM shows
  none of HOME's SETTINGS any more, FLEXOR; common.c serato_col; the kick's RGB and cdj.c) and EDIT
  VIEW (KICK › KNOBS › TOOLS K3 since 0.9.29, SYSTEM › EFFECTS K4 before; `set.kfocus`: the whole kick / the part edited: kick.c kv_ms, kv_kick) (10 Nov 2026, 0.206).
- TUTORIAL (0.211, FLEXOR: "a sober name"): ui/tutor.c (included before screen.c), TUT[] steps (lesson, condition TC_*,
  keys and buttons to blink, text), `set.tut` (appended: 0 off, 1..TUT_N the step, 0xFE done, 0xFD the mode asked (0.229); a new FM-1 asks,
  an older save not), the counters tut_* in common.c; the header draws the step (screen.c); SETUP K3 left of
  BEGINNER; GLO quits, SEL + GLO skips. 0.212 (FLEXOR): HOME first (on PERFORM already, HOME stays: tut_home_stay),
  OUT's K4 the built-in kicks (0.223: BEGINNER's KICK page, K1). 0.235: 23 steps in eight lessons (0.244: 25, the OVERLAY page; 0.246: 24, the ARP key's step gone, FLEXOR: not relevant; 0.252: 26, THE PATTERN before THE KICK, then the kick and the snare heard stopped on keys 1-4: `tut_heard`, a key after the last knob turn; 0.9.3: 31, lesson HOME after the break's: GENERATOR, SETTINGS, KNOB 1 the slices, PERFORM, HOME held, TC_HGEN..TC_HHOLD), BEGINNER as FLEXOR
  chose it (ALGORITHM a break, ARP + K1, K2 DISTO, EDIT to SHAPE and K3 TAIL, SELECT to the SNARE and its PRESET / FX,
  the FX page's PRESETS). 0.213: a step done waits 5 s (`TUT_WAIT`, the header counting) only where there is
  something to hear (TUT[].wait: slices, effects, kicks, DISTO, steps, the drop); a button step goes on at once. Off a step's page the header says the way back (0.248, `tut_away`); the page buttons a step doesn't ask for are held back (0.249, `tut_btn_ok`); the steps say KNOB 1, not K1. AUTO SAVE (0.250, FLEXOR): off while the tutorial runs, on at its end or quit; the first start's BEGINNER turns it on. Keep
  its steps true when a control they teach moves.
- EDIT ZOOM (0.216, FLEXOR: "a wider view"): `set.ezoom` (appended after `tut`; 0 AUTO), KICK › KNOBS › TOOLS K4 "DRUM ZOOM" (0.9.29; SYSTEM › EFFECTS PRESETS before),
  EZ_MS[] in common.c (250, 500, 750 (0.257), 1000, 1500, 2000 ms; read with `ez_idx()`: EZ_TAG | index, an older save's 3.. one on): kick.c kv_ms() and perc.c draw_perc_big's pw_ms; the time drawn under the HZ.
  The picture's cost grows with its time (kick_rgb ticks the kick: 2 s = 88,200 samples a redraw, main loop only).
- Skins (0.237, FLEXOR's for 1.0): 24 in common.c (SKIN_NAME, SKIN_RGB; picked and named on the artifacts
  https://claude.ai/artifact/QpXSifxqofMdzMw1AM23yr, .../KuYXSJbwFsLJJUTTZRCUU6, .../PZo1tMVjmi2HcoLLJtY3tV), RED
  (SKIN_DEFAULT 10; CHOPPAH 0.237-0.239) to start; names in full (0.239: a foot value wider than 54 px is drawn in FONT_T, 2 px left;
  BREADCRUMBZ, 11 letters, is the widest that fits); set.skin
  carries SKIN_NEW (bit 7), an older save's 0..17 mapped by SKIN_OLD; read it with `skin_idx()`. SETUP shows two rows
  of twelve swatches.
- SETUP (0.213, FLEXOR): the knobs in the screen's order, K1 SKIN, K2 KICK EQ (0.9.57, FLEXOR, EXPERT: OFF leaves every kick's EQ out, flat, the EQs untouched; app.c `keq_flat`, never saved, ON at every start: app_init; WAVE ZOOM until 0.9.1: HOME SETTINGS), K3 MODE, K4 AUTO SAVE
  (0.9.26, FLEXOR, asked several times: never while PLAY runs, once PLAY has been stopped 20 s, `AS_REST`; touching the
  FM-1 stopped holds nothing, his choice; never make it sooner);
  the METER moved to MIDI's K3, EXPERT only (BEGINNER turns it off).
- LIGHTS (16 Nov 2026, 0.9.57, FLEXOR: brightness, levels with a meaning, "pour les pulsations il faut que ce soit
  réglable"): SYSTEM › LIGHTS (`GP_LIGHTS` 7, global.c `lights_*`), main.c `leds()`: four levels, off, the glow (every
  button and key at rest; K1 GLOW OFF / 1-7, `plat_dim_ns` with app.h `APP_DIM_LEVELS`, 6 = 3,200 ns, plat_leds' glow
  before), mid (`plat_lights_mid`: the step playing on a lane's page, seq.c `lit_head`; the SEL functions while SEL is
  held or locked), lit (the FM-1 has no other strength for lit LEDs). K2 PULSE (LP_*: OFF, KICK, 1/8, 1/4, 1/2, BAR,
  while playing, not in a build or a drop), K3 PULSE ON (LO_*), K4 STYLE (LS_*: FLASH FADE BREATHE BLINK CHASE). Stored
  in `set.leds[4]` (the STAB's sslot and rsv8: LED_TAG 0xD0 | GLOW, untagged the defaults; app.c `led_cfg` / `led_put`);
  RESET SCREEN puts them back. PLAY lights green through plat_lights (LED_PLAY_GREEN), as plat_leds did. The page draws
  the lights live (`lt_*`, 20 a second). Probes lglow lpulse lon lstyle llitk lmidk ldimk llitb lmidb ldimb; scenario
  lights.
- Drum presets (11 Nov 2026, in progress): FLEXOR picks styled presets by ear on listening pages (rendered on the
  host); his picks, the prototype engine and the plan in `docs/sounds/PERC_PRESETS.md`. His aim: BEGINNER's drum pages
  only a PRESET and an FX, EXPERT every setting (accessibility: surf the machine first, the depth after).
  0.225: the engine's extras (perc.h PX_*: DROP, DTIME, DIRT, LIFT, ROOM; `perc_set_x`), per memory in `smem` (the
  MELODY's old place, `perc_x()`, `set.pxv` = PX_OK), FLEXOR's presets PP_SN / PP_CP in ui/perc.c with their FX,
  pages SOUND / SHAPE (K4 DIRT) / MORE, BEGINNER's PRESET / FX / LEVEL / TUNE.
- BLACK KEYS (11 Nov 2026, 0.244, FLEXOR): each black key a function (app.c `BKF_*`: PAD_* 0-10, FREEZE and DUB
  back from CHOPPAH, then BKF_ARP, KIL (0.9.50, FLEXOR: the kicks back, the drums' filter `kick_dj` OFF at once, `kil_back_flt`; 0.9.64, FLEXOR: SYSTEM › EFFECTS K4 KICK SWITCH, app.c `ksw_mode` in set.rsv 0x50 | it: SMART (the start) keeps it when turned while the kicks were quiet, pads.c `kdj_quiet`; FLT OFF always; FLT KEEP never), DST, SUB (0.9.43, FLEXOR: a switch, the kick's SUB FX off / on whichever effect, selk_hi bit 6, `subfx_off`, its name SUB FX; never SUB DRIVE itself any more), DRV: 0.9.43, FLEXOR, the kick's DRIVE switch, selk_hi bit 5, `drive_off`, as DST; X2: 0.9.48, FLEXOR, the kicks doubled, a kick on every odd step of a bar with kicks, `kick_x2` bit 0, its LATCH on K1, on at first, half_mt_off bit 3 = LATCH OFF; X4 and TRI: 0.9.55, FLEXOR, a kick on every step (bit 1, LATCH OFF bit 4) and the kicks in 8th-note triplets from the next beat (bit 2, the render's KICK ROLL with krp 4 at 4/3 of a 16th, the pattern's kicks out; LATCH OFF bit 5; 0.9.57-0.9.66 KIL turned TRI off and refused it; 0.9.67, FLEXOR: TRI exactly as X2 and X4, KIL only quiets its roll in the render while held), main.c's loop over the three; REC KEY: 0.9.58, FLEXOR, the REC button a twelfth black key (`REC_KEY`, bit 27 among the keys, injected in input(); `bk_fn(11)` = `rec_fn`, kx2.rsv[50] 0x80 | fn (untagged KILL BREAK), rsv[51] 0xA0 | assigned (`rec_key_on`); any function but ARP; ARP › BLACK its last key, right, `nbk()` 12 always); 0.9.70, FLEXOR: no SYSTEM › MIDI K4 REC KEY any more: NONE on BLACK (the start) REC records, a function there REC is that key whatever the layout (no CUSTOM copy); a SEL function alone (`kx2.selrec`), SEL + REC plays it, routed as REC goes down (main.c `rk_key`), REC alone records; KBR: 0.9.58, FLEXOR, KILL BREAK, the break out while held (LATCH: a press each), K1 MODE FADE (K2 TIME) / CUT / END FADE (0.9.60, FLEXOR: to the break's loop end, st.step % 32 round, then the fade, `kend`) / BAR END (its last 8th into the reverb, K3 REVERB, and (0.9.60, FLEXOR) into an echo, K2 ECHO OFF / 1/8 / 3/16 / 1/4 / 3/8 in set.rsv6 (0xE5 tag, `kb_echo`), the dub delay set by `brk_fx_kill_echo` while it rings; no REVERB in the chain: brk_fx.c's plate alone, `brk_fx_tail`, q.tail, 8 s), the render's gain `kbrk_g`, kx2.rsv[48..49] (`KB_TAG`, `kb_*`); SDR: 0.9.57, FLEXOR, SUB DRIVE on / off again as before 0.9.43, the SUB layer's effect DRIVE (its mix up from nothing, the SUB FX switch on) then OFF, `BKF_SDR`, last in BKF_*; DRV, SDR, KBR in no preset layout, CUSTOM only (X2, X4, TRI on FLEXOR LAYOUT since 0.9.58)); `bk_fn(j)`, `bkf_mask(f)` (= `PAD_BIT`) follow the layout in use:
  presets CHOPPAH (a new FM-1's and RESET ALL's; BEGINNER keeps the layout in use since 0.9.23, FLEXOR: `bk_preset` no longer forces it; DST's
  switch (`terror_off`: selk_hi bit 4 and a DST key in the layout, `bk_has`; 0.9.71, FLEXOR: a SEL BLACK function or the REC key's counts too, DRV and SUB alike, scenario seldst) and SUB's are each kick's own, FLEXOR 0.9.24), FLEXOR LAYOUT (FLEXOR's, 0.9.26:
  R8 R16 R32 ARP ARP TRI X4 X2 SUB DST KIL since 0.9.61 (FLEXOR: KIL and SUB swapped; 0.9.58-0.9.60 KIL DST SUB) (FLEXOR: key 5's ARP effect RISE, ovl.c `flexor_lay_fix`, once (kx2.rsv[52] FLAY_TAG) and whenever the preset is picked; REC KILL BREAK; scenarios written on the one before start with the host's `layout_0953`; VNL GAT FLT ARP ARP from key 4 in 0.9.53-0.9.57, FLEXOR's page https://claude.ai/artifact/7m1HPizdCWv5PoPb7JjbgR, store `layout/choix`; RND HLF FLT ARP VNL from key 4 in 0.9.26-0.9.52); keep it exactly; on BLACK KEYS ALGORITHM moves along the keys as K2), CUSTOM (0.9.45, FLEXOR: several ARP keys, each its own effect and LATCH, together: pads.c `arpk_*` in settings' `rsv2`, tagged 0xA7; BLACK KEYS K4 ARP FX; the knobs and the OVERLAY page the one pressed last, `arpk_focus`); stored in `set.bkeys.bk` (the STAB's old 24 bytes: BK_TAG, preset, eleven functions; bk[15]
  BK_PLAT 0xA0 | the K4 LATCH bits of REVERSE, FREEZE, GATE, RANDOM, bk[16] BK_FTHRU 0xC0 | FILTER's K4 THRU BREAK / DRUMS /
  BOTH, 0.9.26, FLEXOR: no LATCH on FILTER; bk[18] BK_BROLL 0xE0 (0.9.50), bk[19..23] the ARP effects' THRU (0.9.51), none free now: older saves hold the STAB's leftovers there). ARP's
  tabs in EXPERT (`ui.opage`, ovl.c; SELECT both ways, 0.9.52): ARP EFFECTS (ALGORITHM the 16 effects in order, `ovl_step`, 0.9.52, FLEXOR; SEL + ALGORITHM the break there), BLACK KEYS (bk_turn, draw_bk), SEL BLK (0.9.49, FLEXOR, SEL KEYS at first: each black key a second function pressed with SEL held or locked, `kx2.selk` (1 + BKF_*, 0 its own) appended to OBJ_KX2; 0.9.70, FLEXOR: the REC key's too, `kx2.selrec` appended after `kit`, any function but ARP, bk_sel_dn bit 11), SEL WHITE (0.9.50, FLEXOR: each white key a shortcut with SEL, any kind on any key: NONE, KICK, PATTERN, BREAK, EFFECT, DROP, KIT, TRACK (0.9.63, FLEXOR: the whole track, stopped at once, playing at the break's loop end: main.c `trk_want`, track.c `trk_seam`, no restart; on the KICK and drum pages, stopped, keys 1-4 hear the drums first, SEL or not: `drum_hear_key`) and its value, `kx2.rsv` two bytes a key, ovl.c `sw_*`, main.c `sw_do` / `sw_held`; the tabs' names short, FLEXOR: ARP, BLK, SEL BLK, SEL WHITE (BLACK and SEL BLACK in full since 0.9.53, FLEXOR: there is room), KITS (0.9.54, FLEXOR: sixteen kits, a kick, a break, a tempo, a pattern each, 0 KEEP, `kx2.kit[16]` (kick, break, pattern) appended to OBJ_KX2 and their tempo in `kx2.rsv[32..47]` (`kit_bpm`: BPM - 49; for the pool, whose margin is then 3,076 B of the 3,072 the build wants; a shorter save: all KEEP), ovl.c `kit_*` / `draw_kits`, main.c `kit_apply`; SEL WHITE's kind SW_KIT loads one; RESET's KITS) (a fifth, SETTINGS, every white key at once, 0.9.50-0.9.51: gone, FLEXOR: the white keys set held, as ever, RESET's WHITE KEYS for all), no OVERLAY label; a white key held: K1 LENGTH, K2 PITCH, K3 MODE, K4 DECAY (0.9.50, FLEXOR: START gone, brk_keyset_t.start bits 4-7, each repeat fading, `brk_key_decay_ms`); 0.9.56, FLEXOR: SEL + a key with nothing assigned (white or black) plays as without SEL, SEL let go after it no tap; SEL + a roll no longer the kick alone; `bk_sel_dn` holds a key's SEL function until let go, app.c `bk_fn`; `bk_fn_shown` for the lights and the strip; 0.9.57, FLEXOR: SEL held or locked and a black key with knobs held, its knobs win over SEL + K1-K4, foot and knobs, main.c `bk_knobs_held`); MORE gone for now (0.259, FLEXOR; its idea: new black-key
  settings, e.g. the ARP effects one a key). SYSTEM › MIDI shows USB and TRS only (no BT box, 0.259). The simulator starts on FLEXOR LAYOUT (its scenarios).
- VINYL BREAK and the drums (17 Nov 2026, 0.9.67, FLEXOR: "le vinyl stop coupe le kick"): held with THRU BOTH or KICK,
  the drums' steps come from the render's record clock (app.c `v_pos`, from the pattern's last step `v_idx` + `v_since`,
  advancing by the tape's speed `spd`): kicks and drums further apart down to the stop, slowed by kick_speed/perc_speed.
- DRUM FILTER (17 Nov 2026, 0.9.67, FLEXOR: "moins agressifs, comme sur une DJM-A9"): SYSTEM › EFFECTS K3, the drums'
  filter's kind (brk_fx.c `brk_fx_drum_type` / `brk_fx_drum_hz`, BRK_DFT_*): CLASSIC (as always; untagged), SMOOTH
  (Butterworth, a mixer's taper, narrower), SOFT (6 dB one pole), RESO (a peak); app.c `dft_kind` in set.click_gone
  (0x60 | it: the metronome's old byte), pushed each block before `brk_fx_drums_dj`; the screen's Hz through
  `brk_fx_drum_hz` (pads.c kick_flt_text). SMOOTH: a detent two steps (0.9.68, FLEXOR: "pas assez réactif", kick_flt_turn). Free bytes left: the BT bytes (rsv6[2] bit 3 SEL WHITE's since 0.9.70) `bta_on` / `bta_codec`.
- NOTES IN (16 Nov 2026, 0.9.64, FLEXOR, a TR-8S cutting the break: its crash 49, high tom 50, ride 51 held slices): SYSTEM ›
  MIDI's PRESETS (the four knobs taken), ALL / DRUMS / OFF, app.c `notes_in` (set.rsv6[2]: 0xA0 | bits 0-1 it, bit 2
  VINYL BREAK's LATCH, `vnl_lat`; untagged ALL, no latch); app_loop's `mid_held`: a slice a note holds is let go by its
  note-off whatever NOTES IN says since. VINYL BREAK's K2 its LATCH since 0.9.64 (FLEXOR: HOME's break filter before):
  `lat_bit(PAD_TAPESTOP)` 4, `bk_lat` bit 4, write through `bk_lat_put`.
- BREAK TIME (11 Nov 2026, 0.256, FLEXOR): HOME › SETTINGS K4 (SYSTEM › MIDI K4 until 0.9.1) NORMAL / HALF, `set.bkeys.bk[BK_BHALF]` (the STAB's old
  bytes again: bk[14] BK_VIEW since 0.9.2, bk[15], bk[16] the latches and FILTER's THRU (0.9.26), bk[17..23] still free); brk.c `p_bhalf` (`brk_half_time`): the break a step every other 16th at half
  ratio, the drums' step events, the swing and the clock at the tempo (they count `steps_total - pat_org`, not `step`).
- RESET (0.209; its list since 0.9.52, FLEXOR: DROP, BUILD (with SONG › BREAK), KNOB, KICK, KICK TONE, ALL KICKS, DRUMS, MIX, FX CHAIN, STEPS, BLACK KEYS (was PADS: padset, latches, VINYL and FILTER THRU; the layout kept), ARP (amounts, each effect's THRU, RATE, SWEEP, LATCH, the ARP keys' effects), WHITE KEYS (was KEYS), WHITE LENGTH (0.9.71, FLEXOR: the break playing's keys one slice each, the rest kept), SPLITS (0.9.73, FLEXOR: every break's split OFF, B and bars kept; `RST_SPLITS`), SEL KEYS (`selk_clear`: SEL BLK and SEL WHITE), KITS (0.9.54, `kits_clear`), TRACK, SCREEN, WIPE MEMORY, EVERYTHING: track.c RST_*; add an item when a new setting is worth putting back): after a reset its page's knobs do nothing for 3 s (`RST_COOL`, `ui.rst_done`, rst_cool()), DONE shown.
  WIPE MEMORY (0.263, FLEXOR: V15 odd after LOAF BREAKER): RESET's item before EVERYTHING, erases every sector of the
  store map but the panel's (track.c `wipe_step`, a sector a frame, plat_data_erase), settings unreadable, `wiped`
  blocks saves until a restart; on the host the store is kept apart, so only the FM-1 really erases.
- Screen mirror (0.9.43, FLEXOR: a computer interface, then a bigger sequencer and kick editor on it later): fm1-base's
  `mirror = true` (fm1.toml; its web page at the site's mirror/, tools/fm1_mirror.py), `APP_REDRAW` (main.c app_redraw);
  for the RAM: app.h MR_SEGW 240 (whole rows), MRQ 2048 (4096 again since 0.9.74, FLEXOR: artefacts on the mirror; RAM 90,060), the CDJ `scope` in the pool: RAM 97,116 of 98,304, pool 340,712 (fm1.toml `pool_spare` 3072: the 8 KB margin spent on it, 3,352 B left; more in the pool frees room first). Image 566,192 B with the Studio: ~15.4 KB left.
- Studio (0.9.43, FLEXOR: "une interface efficace et complète", then "gérer séquenceur, banque de patterns et tracks ...
  comme si j'utilisais un DAW", the RGB waveforms as on the machine): the site's editor/ page (app/site/editor.html, its
  script app/site/editor.js inlined by tools/site_style.py `editor` at SCRIPT; the nav's "Studio"). SysEx (ui/main.c):
  ED_INFO 7 / ED_KNOB 8 (the kick's parts, FX1/FX2 with a fifth byte the part, knob 4 the effect, 5 on/off; the drums'
  pages) / ED_ACT 9 / ED_WAVE 10 (`ed_sysex`, headless: `ed_enter`/`ed_leave`, EXPERT forced); ED_SEQ 11 / ED_STEP 12
  (`ed_seq`, `ed_step`: SEQ's own lane_toggle / lane_knob / seq_turn, patterns as PRESETS, undo); ED_X 13 (`edx_sysex`):
  1 STATUS (early in app_sysex, no canvas: the transport, the track, `scope` columns since a count for the page's live
  deck; `ed_live_t` keeps `scope_on`), 2 any page's knobs through `knob_turn` / `knob_value` / `knob_name` (draw_foot's
  names, its own function since) / `foot_norm` (MIX, SONG's DROP and BUILD, SYSTEM but RESET, ARP and its black keys,
  the FX chain's slots, HOME, GENERATOR, SETTINGS), 3/4/5 a track read, written whole (each item checked), played, 6 a
  group's 16 patterns' pictures (another group read into the canvas, `dx_tp` for its bars 3-4), 7 RGB waveforms
  (kick_rgb, perc_rgb, the break from pcm_a with its slices), 8 transport (tempo, break, PLAY as the button, the breaks'
  names). 0.9.65 (FLEXOR: "très laggy"): knobs and faders move at once in the page (`Panel.loc`, `paint`, `learn`: a
  click's worth learned), the FM-1's value back `SETTLE` 450 ms after the hand stops; status, view reads and waves wait
  while it turns (`busyNow`); `ready` and `ask1` (an older request fails, never hangs) keep connecting clean. tests/editor_link.sh in CI. A local test bench (not in the repository): the host simulator reading its
  scenario from a FIFO (sysex, wait, sysex_log, said_none as a marker) drives the page live in Playwright.
- Drum samples (15 Nov 2026, 0.9.45, FLEXOR: "importer des samples type snare, hi-hat, clap", the site's IMPORT tab): a kit
  of three in OBJ_BLE's two sectors (0xDE000, 8 KB: no Bluetooth, nothing else writes there; app.c `KIT_AT`, `kit_scan`,
  `kit_ent`, `kit_name`, `kit_has`), IMA ADPCM read from flash as it plays (perc.c `perc_set_sample`, `sample_tick`;
  adpcm.c `adpcm_at`); a drum plays its sample while its perc_x PX_RSV is PX_SAMPLE (the last PRESET, ui/perc.c
  `pp_nb` / `pp_n`, `perc_smp`); SysEx zone 2 (`kit_sysex`: 1 INFO, 2 BEGIN, 3 DATA, 4 COMMIT, 5 READ, 6 DELETE);
  app/site/import.html (site_style.py `imp`, the nav's Import), tools/kit_frames.py the same kit in Python; tests
  drum_kit.sh, import_page.sh. WIPE MEMORY erases it (the store map's), RESET ALL keeps it.
- Kick Lab 2 (14 Nov 2026, 0.9.45, FLEXOR: "une refonte ou des additions ... qui changent vraiment la donne", by ear on
  https://claude.ai/artifact/DZpTr7uf1rQ1DB1kyH4Dww, store `lab2k/choix`: 11 ideas and 8 whole kicks prototyped in numpy
  over the built-in kicks recorded from the host; he liked HARD 909's dry attack and ROLL's chopped tail, turned down
  vowels, FM, metal, room, width, ZAAG "too acid"; the inventory (keep / merge / remove) not answered yet): ATTACK's TOK
  (part 33, KP_TOK) and TAIL's GATE (part 34, KP_GATE), each last in its section (the parts before kept where they were), kick.h `kick_x2_t` / `kick_set_x2` (kc_make reads p_x2; STACK's
  attack voice has none), each kick's in `kx2` (pool, OBJ_KX2 = OBJ_NG0 + 3, its own object so the KICK LAB's sizes and
  every save stay; a built-in PRESET, RESET KICK, ALL KICKS, RESET ALL clear it; COPY copies it); scenario kicklab2.
- No Bluetooth, ever (FLEXOR's rule). The GLOUPI BREAKS start-up photo must never come back into the repository. FLEXOR's
  sleeping photo (art/egg_sleep.png, commit afbbd5c, 6 Nov) stays in the history: FLEXOR, 12 Nov 2026, no purge.
- Fonts: `font_trim` is on (fm1.toml, 9 Nov 2026). LOAF BREAKER draws in FONT_T, FONT_XS and FONT_B only (cv_text2 for
  bigger: those doubled). fm1-base still generates S, M (~5 KB trimmed) and L (~9 KB trimmed, 31 px), left out of the
  image as never read. FLEXOR asked to be reminded of them whenever bigger or other text would help (a big tempo, a
  title): say what one would cost then.

## Layout

Porting to other hardware (FLEXOR's question, 10 Nov 2026): `docs/PORTING.md` (the 31 `plat_*` the app calls, 18
without Bluetooth; the screen, the controls, the order of the work). Keep it current when the app calls a new one.


`app/app.c` is the core (settings, patterns, MIDI, the render); it ends by including `app/ui/*.c` in order (common,
fx, pads, keys, bta, global, ovl, seq, kick, perc, mixer, track, tutor, screen, demo, cdj, main): one unity build, so a page file sees what the
files before it define. `app/dsp/*.c` are separate `o2` units (brk, brk_fx, kick, perc, ovl, …).

## Hard rules

- `app/dsp/` stays platform-free: no `plat_*`, no fm1-base headers, no libc beyond `<stdint.h>`. It must build with
  plain `cc` (tests/engine) and as fm1-base `o2` units.
- **No double precision** anywhere in `app/` (the device build rejects it): `f` suffix on every float literal,
  no `math.h` in dsp (use `brk_math.h`). `tests/engine/run.sh` builds with `-Wdouble-promotion -Werror`.
- No 64-bit division on the device (no libgcc `__udivdi3`): keep sample maths in 32 bits.
- The render (`app_render`, `brk_render`) runs in the audio interrupt: no flash writes, no blocking, no
  allocation. Main loop → render through `brk_*` setters (single words) and the key queue.
- Pool RAM budget (PRD 8.2): canvas 240×60 (28 KB) + PCM buffers; the build wants 8 KB of the 344 KB spare.
- Flash writes stall audio: only on SAVE or while stopped.
- Settings (`settings_t`, magic CHP7) only grow at the end: append new fields after the last one (`fxx`, each FX slot's extras) and
  give them defaults in `settings_defaults()`; a shorter (older) save loads into the front (`SET_SIZE_MIN`: the
  offset of the first appended field, `rsv2`, the dropped fills' bytes; keep it there).
  Anything else needs a new magic (saves lost). Patterns are their own object (`OBJ_PAT`, 0xDA000, magic PAT1).
- No Bluetooth in LOAF BREAKER (FLEXOR's rule): app/fm1.toml has `ble = false` and `bt_audio = false` (CHOPPAH's
  were true: ~40 KB and ~37 KB of flash). Their app hooks (`APP_BLE_AT_BOOT`, `BP_MQ_ATTR` in app.h) stay as
  CHOPPAH's, inert; never turn either back on.
- Flash: the app slot is 581,564 B (`APP_SLOT` 0x8DFBC in base/tools/fm1pkg_make.py: not 581 KiB); 577,288 B used at
  0.192 with a 357 KB bank; 561,424 B at 0.220 (9 breaks); 568,492 B at 0.225 (the drums' extras): **~13 KB left**. **580,660 B at 0.9.26 (the latches and FILTER's THRU): ~0.9 KB left; 581,312 B at 0.9.30 (GLO EFFECTS' cards): 252 B left; then AMEN15 174 out (FLEXOR): 554,144 B with the drums' 64 steps and VINYL BREAK's platter (its face 2 KB, app/face_data.h from tools/make_face.py): ~27 KB left; RAM 96,396 of 98,304; .ram_text 17,200 of 32,768. 556,928 B at 0.9.32 (the player's breaks in two zones, TERROR and SPINE): ~24.6 KB left; 557,160 B with KICK 7-8 TERROR, SPINE; 540,896 B at 0.9.34 with FLEXOR's five breaks (9, 282.3 KB): ~40 KB left; 573,120 B with his sixth, OBEDIENC165 (10, 313.8 KB): ~8.2 KB left; 560,368 B at 0.9.37 with THINK at 16 kHz (breaks.txt NAME `16000Hz`, a line's own rate; FLEXOR): ~20.7 KB left; 556,144 B at 0.9.38 with SHUFFLE 190 for OBEDIENC165: ~24.8 KB left; RAM 96,508; .ram_text 17,400. 557,312 B at 0.9.39 (the eleven built-in kicks): ~24.2 KB left; 529,044 B at 0.9.40 with AKIRA 190 out (9 breaks, 269.8 KB): ~51.3 KB left; 555,956 B at 0.9.43 (10 breaks, 296.1 KB: THINK out, FOCUZ and TIMES in): ~25.6 KB left; 573,148 B at 0.9.45 (TOK, TAIL GATE, the ARP keys, the drum samples): ~8.4 KB left; RAM 97,372 of 98,304; 573,832 B at 0.9.48 (X2): ~7.7 KB left; 574,520 B at 0.9.49 (SEL KEYS): ~7 KB left; 576,524 B at 0.9.50 (SEL WHITE): ~5 KB left; 578,052 B with ARP › SETTINGS and the keys' DECAY: ~3.5 KB left; RAM 97,436; 578,736 B with SONG › BREAK: ~2.8 KB left; 552,240 B with DETEST and TIMES out, BANGPHS 155 in (9 breaks): ~28.6 KB left. LFO held + K4: the FX fader (0.9.50, FLEXOR), the FX page's tabs FX MIX nn%.**
  RAM .data+.bss 93,292 of 98,304 B at 0.225 (the drums' room, 5.8 KB of 16-bit lines, is in RAM): ~5 KB left; the
  pool has ~10.8 KB (it could take the room's lines). The break bank (`CH_BANK`) is most of it.
  New features are welcome before 1.0 (FLEXOR, 11 Nov 2026: "not that close to 1.0, I want to perfect the firmware");
  anything big needs something taken out first (ask FLEXOR what).
  Block-rate engine code is `BRK_COLD` (built for size) and calls the shared maths (`brk_exp2f_c` and the like,
  brk_math.c; the UI too) instead of an inline copy; the FX chain's per-sample loop (`fx_samples`) is `BRK_RAM`:
  .ram_text on the FM-1 (fast, packed in flash), so nothing it reaches may be a call (`BRK_INL` in brk_fx.c forces
  inlining; the build fails on "ram_text contains calls"; flash code reaches it through a pointer). CPU: the render
  must stay under its 5.8 ms block, or audio glitches and the watchdog resets the FM-1 (0.9.26: brk_fx.c's slot_run works out
  one side while the two are the same, `mono`; an effect that makes them differ must be in `FX_STEREO`); `APP_OVERLOAD` sheds FX slots
  as a last resort (PRD E-7). Compare an engine change's cost on the host against the last release before shipping (`APP_SYS_TEXT` in app.h draws the platform's screens in CHOPPAH's faces,
  saving fm1-base's fonts): check `./build.sh`'s image line;
  the canvas is 240×60 (`APP_CANVAS_PX`), so draw in strips of ≤ 60 rows (draw_fx / draw_key draw two bands).
- Breaks: `breaks/breaks.txt` (paths relative to `breaks/`) marks with `*` the WAVs built into the firmware;
  `./build.sh` regenerates `app/bank_data.h` (never commit it, never edit it). The bank has 400 KB (`IMAGE_CAPACITY`
  in `tools/prep_breaks.py`) and 16 slots, a break at most `CH_PCM_MAX` 106,496 samples (pool ~326 of 344 KB). The breaks are the bank's only: brk_synth is the no-bank fallback (and
  the tests'). The player's own breaks (0.9.32, FLEXOR, from CHOPPAH 1.4's custom break): two banks (zones, `CU_ZB`, `CU_MAX` 4 each, FLEXOR 14 Nov 2026): zone 0 0x93000 (XDATA0 + the data area up to OBJ_KLAB, 35 sectors, 140 KB), zone 1 0xE5000 (XDATA1, 23 sectors, 92 KB), five to seven breaks; in XDATA never "AT" at a sector's 0xF06 (the update loader's record: the page's `atFix`, the FM-1's `cu_at_ok`); a 0.9.32 TEST's bank at 0x97000 (`CU_OLD`) read until zone 0 is written, written whole over USB SysEx from the site's BREAKS page (`app/site/breaks.html` -> breaks/, its library breaks/lib/ made by `tools/break_lib.py` from breaks.txt's 70 at site build; `app_sysex` in ui/main.c, the canvas as scratch); present, they come AFTER the bank's, zone 0's then zone 1's (`slot_of`, `n_user`, `n_uz`, `n_bank`: saved break numbers keep theirs); tests/custom_break.sh and tests/page_parity.sh in CI. FLEXOR: no hiding of the built-in ones. The breaks stay 2 bars (32 steps; a one-bar break plays twice, LO-FI 165 was one in 0.196-0.199).
- Releases are automatic: every push to `loaf` is a release. Numbers A.B.C since 12 Nov 2026 (FLEXOR: "on passe en
  0.9.1"): the last `loaf-vA.B.C` tag's patch + 1 (`scripts/loaf_version.sh next`), or `./LOAF_FROM` if higher
  (0.9.21 now, FLEXOR: "on va dire 0.9.21"; then 0.9.22 ...; write 0.10.0 or 1.0.0 there for a new minor or major). Before: X.YY, 0.02 to 0.267. The package's own
  number (fm1-base's X.Y.Z, two digits each) is `scripts/loaf_version.sh pkg`: 0.267 is 1.0.68, A.B.C is A+2 . B . C
  (0.9.1 is 2.9.1, 1.0.0 is 3.0.0), always above the one before (`check` in CI). Don't tag by hand. (`scripts/next_version.sh` and
  `VERSION` are CHOPPAH's main branch's.)
- **Always push to GitHub after changes** (Laurens' standing rule, 4 Nov 2026): commit and push as soon as a change is
  done, before moving on: to `dev` (a TEST build for FLEXOR), never straight to `loaf` (FLEXOR, 12 Nov 2026); `loaf`
  takes dev when FLEXOR asks for a release. Work that doesn't build or fit yet goes to a branch of its own.

## The update screen

The screen the FM-1 shows while an update is written ("UPDATING", "DO NOT UNPLUG", the bar) is the update loader's,
built from felucca-bsp (`base/bsp/loader/`), which has no hook: `build.sh` copies `app/loader/ldr_screen.c` over
`base/bsp/loader/ldr_screen.c` for a device build and puts the original back when it ends (never commit the
submodule changed). Keep its function names, the panel's start and RAM-only buffers as felucca-bsp's: the loader runs
while the app area is rewritten. It ships in the package, so it shows when that package is installed.

## Site

`base/tools/make_site.py` builds the GitHub Pages site; `tools/site_style.py` then gives it CHOPPAH's look
(`app/site/site.css`; the installer and the emulator, fm1-base's pages, also `app/site/install.css` and
`app/site/emu.css`: only their style, never their markup, ids or script) and puts the interactive control map (`app/site/controlmap.html`) into the manual's section
"The control map". When a page's controls change, update that file's `PAGES` list with the manual. Manual screenshots
come from the host scenarios (`app/build/scenarios/*.png`, copied into `app/docs/img`). `tools/site_shots.sh` (its MAP: the manual's
name for each shot) does it, at every release too; `tools/readme_shots.py` then makes the README's grids (docs/img: screens,
pads, kick, live, menus: the newest menus), by hand when the menus change (0.9.57).

## Commands

```
./build.sh test        # engine tests (fast, run after every dsp change)
./build.sh             # firmware (needs scripts/bootstrap.sh, Docker/OrbStack on macOS)
./build.sh --host      # host simulator
./build.sh flash       # macOS Terminal only: the sandboxed shell has no USB access
```

## Platform facts (fm1-base 2d8ed74, Nov 2026)

44.1 kHz stereo, 256-frame blocks; 581,564 B app flash; 96 KB RAM + 336 KB pool; 240×240 screen; 27 keys
(16 white / 11 black, `PLAT_WHITE_KEYS` / `PLAT_BLACK_KEYS`); 14 buttons (`B_*`), 7 encoders (`EN_*`), MASTER pot.
Store objects ≤ 3840 B; data area 0x97000–0xDFFFF, XDATA0 0x93000–0x96FFF, XDATA1 0xE5000–0xFBFFF (keep bytes
0xF00–0xF07 of each XDATA sector erased).
