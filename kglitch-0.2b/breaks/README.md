# Breaks

The source of the breaks built into LOAF BREAKER, and which of them go in: [`breaks.txt`](breaks.txt) lists every WAV here
with its tempo, its bars and a name, and marks with `*` the ones in the firmware; ALGORITHM steps through them by
tempo, slowest first (the slowest is the break at power-on). All paths are relative to this folder. The firmware has
eight: AMEN10 135, AMEN05 158, AMEN20 164, UPDOWN 165, AMEN06 169, AMEN19 172, AMEN13 173 (seven of Cyberworm's Amens,
from `cw/` and `huge/`) and FOCUZ 175 (from `flexor/`); every other line is in the site's library.

## classics/: the records

Ten breaks from the records, 4 bars each, 32-bit float WAV, 44.1 kHz (from a shared "10 breaks" folder; the tempos
here come from their lengths, a hair off the folder's labels; the Amen was labelled at half its tempo, 71). None is
built into the firmware now: all ten are in the site's library, two bars each (the AMEN, from CHOPPAH 1.5.3, bars 3 and
4 of the record, cut at the drummer's own tempo, 144.72 BPM: AMEN 145):

| Name | Record | BPM | Where |
| --- | --- | --- | --- |
| AMEN | The Winstons, "Amen, Brother" | 141.82 | the library (AMEN 145, bars 3+4) |
| DRUMMER | James Brown, "Funky Drummer" | 99.81 | the library |
| THE WORM | Jimmy McGriff, "The Worm" | 109.79 | the library |
| THINK | Lyn Collins, "Think (About It)" | 113.78 | the library |
| COLD SWEAT | James Brown, "Cold Sweat" | 114.00 | the library |
| HOT PANTS | Bobby Byrd, "Hot Pants" | 114.78 | the library |
| APACHE | Incredible Bongo Band, "Apache" | 114.78 | the library |
| SPORT | Lightnin' Rod, "Sport" | 120.69 | the library |
| LIFE COULD | Rotary Connection, "Life Could" | 99.81 | the library |
| NEW DAY | Skull Snaps, "It's a New Day" | 94.81 | the library |

These are commercial recordings: their rights belong to the labels and artists. They are here because the project's
owner chose to publish them; anyone building on this should make their own call.

## flexor/: FLEXOR's breaks

Breaks FLEXOR picked (14 Nov 2026), sampled from artists' records: not royalty free, their rights with the artists
and labels, published by FLEXOR's choice. Their list, the records they come from and how to add one:
[`flexor/README.md`](flexor/README.md).

## cw/, huge/: Cyberworm's Amens

60 Amen loops by Dmitriy "Cyberworm" Vasilyev (rhythm-lab.com), who offers them royalty free (`huge/info.txt`): 20 in
`cw/` (tempo in the name), 40 from the "Huge Amen Break Loops Collection" in `huge/`. 16-bit 44.1 kHz WAV. Seven of
the firmware's eight are theirs (0.9.70): AMEN10, AMEN05, AMEN20, AMEN06, AMEN19 and AMEN13 from `cw/`, UPDOWN from
`huge/`.

## How the bank is made

`./build.sh` turns the marked lines into `app/bank_data.h` (generated, git-ignored) with `tools/prep_breaks.py`:
mono, 22.05 kHz, IMA ADPCM, 16 slices a break on an even grid: every line is marked `*=` or `-=` (the BREAKS page's
EVEN GRID; a line without `=` would put the markers on the drum hits). The firmware has 400 KB for breaks and a break
may be at most 106,496 samples (2 bars at 99.4 BPM and up); `./build.sh bank` builds the bank on its own and says
whether it fits. The tool reads integer PCM (8–32 bit) and float WAVs.

Every break is brought to the same loudness as heard, −12 LUFS (0.9.70, FLEXOR: every break at the same level; −16
before, and since 0.9.42 the level as heard: some sounded louder than others at the same RMS; ITU-R BS.1770's
K-weighting and its gates, `loudness_db`), and a
look-ahead limiter (2 ms ahead, 40 ms release) holds its peaks under −0.2 dBFS, so a spiky break is as loud as a dense
one.
A line of breaks.txt may give a break its own sample rate after its name (`NNNNNHz`; THINK 114 was at 16 kHz in
0.9.37 for room: it is at 22.05 kHz again, and no break is at 16 kHz any more, FLEXOR), and its own level: `@-14`
(LUFS), or `@0`, the file's own level, its peaks to full scale. The build prints each break's gain and how much its peaks were held down; `--loudness 0` puts the peaks at full scale
instead (the old way, which left THINK about 8 dB quieter than DRUMMER).

Your own breaks go to the FM-1 without rebuilding (0.9.32): the site's BREAKS page (`app/site/breaks.html`) plays this
folder's breaks (its library, made by `tools/break_lib.py`) or a sample of yours as the FM-1 will, and sends five to
seven of them over USB, after the eight built in.
