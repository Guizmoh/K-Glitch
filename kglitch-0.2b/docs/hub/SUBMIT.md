# LOAF BREAKER on the FM-1 Firmware Hub

LOAF BREAKER is FLEXOR's fork of [CHOPPAH](https://github.com/profikid/fm1-break-choppah) (Laurens Profittlich). It
has its own name, files and package identity, and credits CHOPPAH everywhere it is shown (README, site, card text).
The hub's rules: <https://thefirmwarehub.com/fmhub.md> and <https://thefirmwarehub.com/kb/>. It takes a firmware through
its **form**, <https://thefirmwarehub.com/submit/> (zven, 12 Nov 2026: not by mail), once the six points below hold.
zven confirmed on 12 Nov 2026 that LOAF BREAKER is welcome on the hub.

## Identity

- **Package identity:** `FM-1_2623` (fm1.toml `id_prefix`): dev builds `FM-1_262300`, releases `FM-1_2623` + the
  package number as two digits each (0.220 was package 1.0.21: `FM-1_2623010021`; since 0.9.1 (12 Nov 2026) A.B.C is
  A+2 . B . C: 0.9.1 is `FM-1_2623020901`, 1.0.0 will be 3.0.0: `FM-1_2623030000`, always above the last: `scripts/loaf_version.sh`). Checked free on the hub's catalogue on
  9 Nov 2026: no identity there starts with 2623, and 2623 starts none of theirs. CHOPPAH keeps its own, `FM-1_86`.
  Nothing is reserved on the hub: an identity is chosen, used in the releases, and given in the mail.
- **Files:** `loaf-breaker-A.B.C.fwsc` (fm1.toml `name`), the site's installer and the releases.
- **USB:** a MIDI device named "LOAF BREAKER FM-1", maker "LOAF BREAKER".
- **Sounds** (13 Nov 2026, the hub's ask, Sven): `fmhub.json`'s `sounds` line says where `breaks/` comes from: Cyberworm's
  Amen loops (Rhythm Lab, royalty free), the funk and soul classics and the hardcore tracks of `breaks/flexor/` (its
  artists named: rights with their owners); plain text, at most 300 characters, no web address (the hub's check); see
  `breaks/README.md`, `breaks/flexor/README.md`. FLEXOR (14 Nov 2026): keep it current as he adds breaks (a new
  artist of `breaks/flexor/` named there; shorten the rest to fit). The card's `what`: at most 300 characters (more is
  cut at a word), the editor's breaks last (FLEXOR). FLEXOR answers for them. Then "Check again" on the hub's status page (it reads `loaf`).
- **Card:** `fmhub.json` (root): LOAF BREAKER's text, FLEXOR as author, the site's emulator, its BREAKS page as the `editor` (0.9.37, FLEXOR) and the manual, the picture
  `docs/img/loaf_breaker_card.png` (480 × 480 PNG), the screen's red.

## The six points

| # | The hub asks | LOAF BREAKER |
| --- | --- | --- |
| 1 | Open source, the whole source public, a LICENSE | MPL-2.0, LICENSE in the root. Public since 11 Nov 2026. Its history is clean: the GLOUPI BREAKS photo is in no branch and no tag since 10 Nov 2026 (branches rewritten, then the 219 tags with it by the PURGE START-UP PHOTO action, since removed; GitHub's cached views of the old commits: a Support request) |
| 2 | A GitHub release with the .fwsc | Every push to `loaf`: release `loaf-vA.B.C` with `loaf-breaker-A.B.C.fwsc` |
| 3 | Its own `FM-1_` identity | `FM-1_2623` (above) |
| 4 | The update loader in the package | felucca-bsp's, shipped in every package |
| 5 | Tested on a real FM-1: from V15, from another firmware, and another installed from it | Done by FLEXOR with 0.9.1 (12 Nov 2026), and the form sent; his V15's instrument list was already broken (its factory data overwritten, M-VAVE's to restore), said in the form. Before (before leaving LOAF BREAKER: SYSTEM › RESET › WIPE MEMORY, 0.263, its saves erased so the next firmware finds that memory blank) |
| 6 | `fmhub.json` in the root of the default branch | Done (`loaf`, the default branch) |

The site has no install code any more (0.261, public): the firmware installs from it, from the releases and from the hub.

## The form, once 5 is done (what to put in it)

> Hello, here is LOAF BREAKER, a firmware for the M-VAVE FM-1, by FLEXOR. It is a fork of CHOPPAH by Laurens
> Profittlich, credited as such.
> Repository: https://github.com/FLEXOR-R/loaf-breaker
> Identity: FM-1_2623 (releases FM-1_2623XXYYZZ) · Licence: MPL-2.0
> Tested on a real FM-1: installed from V15 and from another firmware, and another firmware installs again from it.
> It keeps its own settings and patterns and does not read other firmwares' projects or samples.
