#!/usr/bin/env python3
"""LOAF BREAKER: an endurance scenario for the host simulator: MINUTES of random play (pages, knobs, keys, pads, drops,
saves, SEL's combinations), the same for a SEED. The run fails if the simulator crashes or stops before the end, or if
the FM-1 is silent once it has been put back to play at the end. CI runs it at every release (.github/workflows/loaf.yml):
    python3 tools/soak.py --minutes 30 --seed 7 > app/build/soak.txt
    app/build/host/loaf-breaker_host app/build/soak.txt app/build/soak"""
import argparse
import random

BUTTONS = ["FX", "ENV", "LFO", "EDIT", "GLO", "HOME", "ARP", "SEQ", "REC", "OCT-", "OCT+"]
KNOBS = ["SELECT", "ALGO", "PRESET", "K1", "K2", "K3", "K4"]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--minutes", type=float, default=30.0)
    ap.add_argument("--seed", type=int, default=1)
    a = ap.parse_args()
    r = random.Random(a.seed)
    out = [f"# LOAF BREAKER: soak, {a.minutes:g} minutes, seed {a.seed} (tools/soak.py)", "wait 4200", "tap PLAY"]
    t, end = 0, int(a.minutes * 60000)
    while t < end:
        x = r.random()
        if x < 0.22:                                  # a page, or its next part
            out.append(f"tap {r.choice(BUTTONS)}")
        elif x < 0.50:                                # a knob, a few detents either way
            out.append(f"turn {r.choice(KNOBS)} {r.choice([-1, 1]) * r.randint(1, 6)} 10")
        elif x < 0.62:                                # a white key: a slice, a loop held
            k, h = r.randint(0, 15), r.choice([30, 30, 300, 1200])
            out += [f"key w{k} down", f"wait {h}", f"key w{k} up"]
            t += h
        elif x < 0.72:                                # a pad held, its knobs turned
            k, h = r.randint(0, 10), r.choice([150, 600, 2000])
            out += [f"key b{k} down", f"wait 80", f"turn K{r.randint(1, 4)} {r.choice([-1, 1]) * r.randint(1, 5)} 10",
                    f"wait {h}", f"key b{k} up"]
            t += h + 80
        elif x < 0.80:                                # SEL + a knob (the live functions)
            out += ["press SEL", f"turn K{r.randint(1, 4)} {r.choice([-1, 1]) * r.randint(1, 8)} 10", "release SEL"]
        elif x < 0.83:                                # a drop
            out += ["press SEL", "tap ENV", "release SEL"]
        elif x < 0.85:                                # LFO + a knob: the levels
            out += ["press LFO", f"turn K{r.randint(1, 4)} {r.choice([-1, 1]) * r.randint(1, 4)} 10", "release LFO"]
        elif x < 0.87:                                # PLAY: stop or play
            out.append("tap PLAY")
        elif x < 0.88:                                # SAVE
            out.append("tap SAVE")
        elif x < 0.90:                                # SEL alone: the card
            out.append("tap SEL")
        w = r.choice([40, 80, 150, 300, 600, 1200])
        out.append(f"wait {w}")
        t += w + 30
    # the end: back to HOME, the levels up, playing; it must sound
    out += ["tap HOME", "wait 300", "ensure_play"]
    for k in ("K1", "K2", "K3", "K4"):
        out += ["press LFO", f"turn {k} 40 10", "release LFO"]
    out += ["wait 300", "peakreset", "wait 3000", "expect playing 1", "expect peak_db_min -60"]
    print("\n".join(out))


if __name__ == "__main__":
    main()
