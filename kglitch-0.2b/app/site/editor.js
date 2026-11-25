/* SPDX-License-Identifier: MPL-2.0 */
/* LOAF BREAKER Studio's script (app/site/editor.html; tools/site_style.py inlines it at SCRIPT). The FM-1 over Web MIDI
 * SysEx, one request at a time; every knob turned by the FM-1's own code (ui/main.c ed_sysex, ed_seq, ed_step,
 * edx_sysex). */
const $ = (id) => document.getElementById(id);
const esc = (s) => String(s).replace(/[&<>"]/g, (c) => ({ "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;" })[c]);
const str = (d, a, n) => { let s = ""; for (let i = a; i < a + n && i < d.length && d[i]; i++) s += String.fromCharCode(d[i]); return s; };
const h = (tag, cls, html) => { const e = document.createElement(tag); if (cls) e.className = cls; if (html !== undefined) e.innerHTML = html; return e; };
const clamp = (v, a, b) => Math.max(a, Math.min(b, v));
const PG = { HOME: 0, GEN: 1, FX: 2, GLOBAL: 3, KICK: 4, MIX: 5, SEQ: 6, PERC: 7, TRACK: 8, OVL: 9, KEQ: 10, HSET: 11 };
const LANE = ["KICK", "SNARE", "HAT", "CLAP"], LANEC = ["var(--k)", "var(--s)", "var(--h)", "var(--c)"];
const LANEHEX = ["#ff6b57", "#ffc94a", "#4fd1e8", "#b48cff"];
const DRUM = ["SNARE", "HAT", "CLAP"], DPAGE = ["SOUND", "SHAPE", "MORE"];

/* ================================================================== the link (as the BREAKS page's) */
function pack7(b) {
  const out = [];
  for (let i = 0; i < b.length; i += 7) {
    const g = b.slice(i, i + 7);
    out.push(g.reduce((a, v, j) => a | ((v >> 7) & 1) << j, 0));
    for (const v of g) out.push(v & 0x7F);
  }
  return out;
}
function unpack7(p) {
  const u = [];
  for (let i = 0; i < p.length; i += 8)
    for (let j = 1; j < 8 && i + j < p.length; j++) u.push(p[i + j] | ((p[i] >> (j - 1)) & 1) << 7);
  return Uint8Array.from(u);
}
let ready = false, midi = null, out = null, waiting = null, chain = Promise.resolve(), inflight = 0, lost = false;
function onMsg(e) {
  const d = e.data;
  if (d[0] !== 0xF0 || d[1] !== 0x7D || d[2] !== 0x43 || d[3] !== 0x48) return;
  if (waiting && (d[4] & 0x3F) === waiting.cmd) { const w = waiting; waiting = null; w.done = true; w.res({ code: d[5], data: unpack7(d.subarray(6, d.length - 1)) }); }
}
const REPLY = { 9: 7, 12: 11 };              /* (ED_ACT answers as ED_INFO, ED_STEP as ED_SEQ) */
function ask1(cmd, data, ms = 2500, port = out) {
  return new Promise((res, rej) => {
    const me = { cmd: REPLY[cmd] || cmd, res, done: false };
    if (waiting && !waiting.done) { waiting.done = true; waiting.rej(new Error("superseded")); }   /* (0.9.65: never a
                                             * request left hanging: one waits at a time, an older one fails) */
    me.rej = rej;
    waiting = me;
    port.send([0xF0, 0x7D, 0x43, 0x48, cmd, ...pack7(Array.from(data)), 0xF7]);
    setTimeout(() => { if (waiting === me) waiting = null; if (!me.done) { me.done = true; rej(new Error("no answer")); } }, ms);
  });
}
let fails = 0;                              /* (answers missed in a row: the toast at the third, a hiccup is no loss) */
function ask(cmd, data = [], ms = 2500) {    /* (one at a time: each waits for the one before) */
  inflight++;
  const p = chain.then(() => ask1(cmd, data, ms));
  chain = p.catch(() => {});
  p.then(() => { inflight--; fails = 0; if (lost) { lost = false; $("dot").className = "dot on"; } },
         () => { inflight--; if (++fails >= 3) lostLink(); });
  return p;
}
function lostLink() {
  if (!out || lost) return;
  lost = true;
  $("dot").className = "dot bad";
  toast("The FM-1 isn't answering. Check the cable, then Connect again.");
}
let toastT = 0;
function toast(t, ok) {
  const e = $("toast");
  e.textContent = t; e.className = "toast" + (ok ? " ok" : "");
  clearTimeout(toastT); toastT = setTimeout(() => e.classList.add("hidden"), ok ? 2200 : 5000);
}

/* ================================================================== colours: the FM-1's own (common.c serato_col, cdj.c) */
let accent = "#ff8470";                    /* (the design's own, for the canvases) */
let wstyle = 0;                             /* 0 RGB, 1 SERATO, 2 3 BAND, 3 BLUE (the FM-1's WAVE TYPE; 0.9.70 the last two) */
const WS_NAMES = ["RGB", "Serato", "3 Band", "Blue"];
function colOf(v, bright) {                 /* v: lows, mids, highs, peak (0..255) -> [r, g, b] */
  const m = Math.max(v[0], v[1], v[2]);
  if (!m) return [0, 0, 0];
  const br = bright === undefined ? 110 + v[3] * 145 / 255 : bright;
  if (wstyle === 3) {                       /* (BLUE: dark blue, pale with the highs: common.c wave_col) */
    const s = v[0] + v[1] + v[2], q = Math.min(255, s ? v[2] * 560 / s : 0);
    return [(20 + 150 * q / 255) * br / 255, (70 + 155 * q / 255) * br / 255, (210 + 45 * q / 255) * br / 255];
  }
  if (wstyle === 2) {                       /* (3 BAND as one colour, where a column is a single rectangle: the strongest band's) */
    const C = [[36, 104, 255], [255, 150, 40], [245, 245, 245]], k = [v[0] * 1.5, v[1] * 0.95, v[2] * 0.45];
    const j = k[0] >= k[1] && k[0] >= k[2] ? 0 : k[1] >= k[2] ? 1 : 2;
    return C[j].map((c) => c * br / 255);
  }
  if (wstyle === 1) {
    const a = 3 * v[0], b = v[1], c = v[2], s = a + b + c, hh = (b * 120 + c * 240) / s;
    let r, g, bl;
    if (hh < 60) { r = 255; g = hh * 255 / 60; bl = 0; } else if (hh < 120) { r = (120 - hh) * 255 / 60; g = 255; bl = 0; }
    else if (hh < 180) { r = 0; g = 255; bl = (hh - 120) * 255 / 60; } else { r = 0; g = (240 - hh) * 255 / 60; bl = 255; }
    return [r * br / 255, g * br / 255, bl * br / 255];
  }
  let r = v[0] * 255 / m, g = v[1] * 255 / m, b = v[2] * 255 / m;
  r = r * r / 255; g = g * g / 255; b = b * b / 255;
  return [r * br / 255, g * br / 255, b * br / 255];
}
/* an RGB waveform (cols: [l, m, h, peak] each) into a canvas: each column as tall as its peak, both ways */
function drawRGB(cv, cols, opt = {}) {
  const dpr = window.devicePixelRatio || 1, W = Math.round(cv.clientWidth * dpr), H = Math.round(cv.clientHeight * dpr);
  if (!W || !H) return;
  if (cv.width !== W || cv.height !== H) { cv.width = W; cv.height = H; }
  const g = cv.getContext("2d");
  g.fillStyle = "#070706"; g.fillRect(0, 0, W, H);
  const n = cols.length, mid = H / 2;
  let pk = 1;
  if (opt.norm !== false) for (const c of cols) pk = Math.max(pk, c[3]);
  else pk = 255;
  g.fillStyle = "#211e17"; g.fillRect(0, Math.floor(mid), W, Math.max(1, dpr));
  for (let x = 0; x < W; x++) {
    const c = cols[Math.min(n - 1, Math.floor(x * n / W))];
    if (!c || !c[3]) continue;
    const hh = c[3] / pk * (mid - 3 * dpr), br = 120 + c[3] * 135 / pk;
    if (wstyle === 2) {                     /* (3 BAND: the lows blue behind, the mids orange, the highs white in front) */
      const m = Math.max(c[0], c[1], c[2], 1), C = [[36, 104, 255], [255, 150, 40], [245, 245, 245]], K = [1.5, 0.95, 0.45];
      for (let j = 0; j < 3; j++) {
        const h3 = Math.min(hh, hh * c[j] * K[j] / m);
        g.fillStyle = `rgb(${C[j][0] * br / 255 | 0},${C[j][1] * br / 255 | 0},${C[j][2] * br / 255 | 0})`;
        g.fillRect(x, mid - h3, 1, 2 * h3 + 1);
      }
      continue;
    }
    const [r, gg, b] = colOf(c, br);
    g.fillStyle = `rgb(${r | 0},${gg | 0},${b | 0})`;
    g.fillRect(x, mid - hh, 1, 2 * hh + 1);
  }
  if (opt.after) opt.after(g, W, H, dpr);
}
function colsOf(d, at, n) { const c = []; for (let i = 0; i < n; i++) c.push([d[at + 4 * i], d[at + 4 * i + 1], d[at + 4 * i + 2], d[at + 4 * i + 3]]); return c; }
const patHue = (n) => (n * 47 + 8) % 360;
const patCol = (n, l = 58) => `hsl(${patHue(n)} 62% ${l}%)`;

/* ================================================================== knobs: any of the FM-1's, by its address */
/* addr: {t: "k", snd, part, fxp} the kick's (snd 0) or a drum's (1..3) part (ED_KNOB); {t: "p", pg, a, b} a page's
 * (ED_X 2). The FM-1 answers with the four knobs' names, texts and amounts (255: none, a choice) */
function parseKnobs(d, at, fxAt) {
  const ks = [];
  for (let i = 0; i < 4; i++) { const o = at + 29 * i, b = d[o + 28]; ks.push({ name: str(d, o, 12), text: str(d, o + 12, 16), bar: b === 255 ? null : b / 200 }); }
  if (d.length > fxAt + 14) {
    const n = d[fxAt + 14];
    ks.fx = { name: str(d, fxAt, 12), on: !!d[fxAt + 12], page: d[fxAt + 13], pages: Array.from({ length: n }, (_, m) => str(d, fxAt + 15 + 8 * m, 8)) };
  }
  return ks;
}
async function knobReq(a, knob, d) {
  if (a.t === "k") {
    const r = await ask(8, [a.snd, a.part, knob, d & 255, a.fxp || 0]);
    return { code: r.code, ks: parseKnobs(r.data, 2, 118) };
  }
  const r = await ask(13, [2, a.pg, a.a || 0, a.b || 0, knob, d & 255]);
  return { code: r.code, ks: parseKnobs(r.data, 4, 120) };
}
const ARC0 = -135, ARC1 = 135;
function arcPath(cx, cy, r, a0, a1) {
  const p = (a) => [cx + r * Math.sin(a * Math.PI / 180), cy - r * Math.cos(a * Math.PI / 180)];
  const [x0, y0] = p(a0), [x1, y1] = p(a1);
  return `M${x0.toFixed(2)} ${y0.toFixed(2)}A${r} ${r} 0 ${a1 - a0 > 180 ? 1 : 0} 1 ${x1.toFixed(2)} ${y1.toFixed(2)}`;
}
let lastTurn = 0, turnedAny = false, turnedAll = false, lastPollT = 0, polling = false;
const pending = new Map();                  /* panel -> [clicks per knob] */
let flushing = false;
class Panel {
  constructor(addr, title, opt = {}) {
    this.addr = addr; this.title = title; this.opt = opt;
    this.el = h("div", "panel" + (opt.big ? " big" : ""));
    if (title) this.el.appendChild(h("h3", "", esc(title)));
    if (opt.fx) this.fxEl = this.fxHead();
    const row = h("div", "knobs");
    this.knobs = [0, 1, 2, 3].map((i) => this.knob(i));
    this.knobs.forEach((k) => row.appendChild(k));
    this.el.appendChild(row);
    if (opt.color) this.el.style.setProperty("--kc", opt.color);
    this.ks = null;
    this.loc = [null, null, null, null];    /* (0.9.65: what the knob shows while it turns, ahead of the FM-1's answer) */
    this.step = [0.02, 0.02, 0.02, 0.02];   /* (a click's worth, learned from the answers) */
    this.held = [0, 0, 0, 0];               /* (the last local move: the FM-1's own value back once quiet) */
  }
  knob(i) {
    const el = h("div", "kn blank", `<svg viewBox="0 0 46 38" aria-hidden="true"><path class="tr" d="${arcPath(23, 21, 16, ARC0, ARC1)}"/>` +
      `<path class="vl" d=""/><line class="pt" x1="23" y1="21" x2="23" y2="9"/></svg><div class="v"></div><div class="n"></div>`);
    el.tabIndex = 0; el.setAttribute("role", "slider");
    let y0 = null, acc = 0, moved = false;
    el.addEventListener("pointerdown", (e) => { if (e.button) return; y0 = e.clientY; acc = 0; moved = false; el.setPointerCapture(e.pointerId); });
    el.addEventListener("pointermove", (e) => {
      if (y0 === null) return;
      acc += (y0 - e.clientY) / (e.shiftKey ? 14 : 5); y0 = e.clientY;
      const n = Math.trunc(acc);
      if (n) { acc -= n; moved = true; this.turn(i, n); }
    });
    el.addEventListener("pointerup", (e) => {
      if (y0 !== null && !moved && el.classList.contains("choice")) this.turn(i, e.shiftKey ? -1 : 1);   /* (a choice: a click the next) */
      y0 = null;
    });
    el.addEventListener("contextmenu", (e) => { if (el.classList.contains("choice")) { e.preventDefault(); this.turn(i, -1); } });
    el.addEventListener("wheel", (e) => { e.preventDefault(); this.turn(i, (e.deltaY < 0 ? 1 : -1) * (e.shiftKey ? 5 : 1)); }, { passive: false });
    el.addEventListener("keydown", (e) => {
      const m = e.shiftKey ? 5 : 1;
      if (e.key === "ArrowUp" || e.key === "ArrowRight") this.turn(i, m);
      else if (e.key === "ArrowDown" || e.key === "ArrowLeft") this.turn(i, -m);
      else if (e.key === "PageUp") this.turn(i, 10);
      else if (e.key === "PageDown") this.turn(i, -10);
      else if (e.key === "Enter" && el.classList.contains("choice")) this.turn(i, 1);
      else return;
      e.preventDefault(); e.stopPropagation();
    });
    return el;
  }
  fxHead() {
    const e = h("div", "fxhead", `<button data-d="-1" aria-label="Previous effect">◀</button><span class="fxn">—</span>` +
      `<button data-d="1" aria-label="Next effect">▶</button><button class="pw" aria-pressed="false">On</button><div class="tabs"></div>`);
    e.querySelectorAll("button[data-d]").forEach((b) => { b.onclick = () => this.turn(4, +b.dataset.d); });
    e.querySelector(".pw").onclick = (ev) => this.turn(5, ev.currentTarget.getAttribute("aria-pressed") === "true" ? -1 : 1);
    this.el.appendChild(e);
    return e;
  }
  show(ks) {
    this.ks = ks;
    this.paint();
  }
  local(i) {                                /* (a knob still turning, or its clicks not all answered: its local value) */
    const p = pending.get(this);
    return this.loc[i] !== null && ((p && p[i]) || performance.now() - this.held[i] < SETTLE);
  }
  paint() {
    if (!this.ks) return;
    const ks = this.ks.map((k, i) => (k.bar !== null && this.local(i) ? { ...k, bar: this.loc[i] } : k));
    ks.fx = this.ks.fx;
    for (let i = 0; i < 4; i++) if (!this.local(i)) this.loc[i] = null;
    ks.forEach((k, i) => {
      const el = this.knobs[i];
      const hide = !k.name || !!(this.opt.hide && this.opt.hide(k, i));
      el.classList.toggle("blank", hide);
      el.tabIndex = hide ? -1 : 0;
      el.classList.toggle("choice", k.bar === null);
      el.querySelector(".v").textContent = k.text;
      el.querySelector(".n").textContent = k.name;
      if (k.bar !== null) {
        const a = ARC0 + (ARC1 - ARC0) * clamp(k.bar, 0, 1);
        el.querySelector(".vl").setAttribute("d", k.bar > 0.004 ? arcPath(23, 21, 16, ARC0, a) : "");
        const r = a * Math.PI / 180;
        el.querySelector(".pt").setAttribute("x2", (23 + 11 * Math.sin(r)).toFixed(2));
        el.querySelector(".pt").setAttribute("y2", (21 - 11 * Math.cos(r)).toFixed(2));
        el.setAttribute("aria-valuenow", String(Math.round(k.bar * 100)));
      } else el.removeAttribute("aria-valuenow");
      el.setAttribute("aria-valuemin", "0"); el.setAttribute("aria-valuemax", "100");
      el.setAttribute("aria-label", (this.opt.label || this.title || "") + " " + k.name);
      el.setAttribute("aria-valuetext", k.text);
    });
    if (this.fxEl && ks.fx) {
      const f = ks.fx;
      this.fxEl.querySelector(".fxn").textContent = f.name;
      const pw = this.fxEl.querySelector(".pw"), empty = f.name === "EMPTY";
      pw.setAttribute("aria-pressed", String(f.on && !empty)); pw.textContent = empty ? "Empty" : f.on ? "On" : "Bypassed"; pw.disabled = empty;
      this.el.closest(".dev") && this.el.closest(".dev").classList.toggle("off", !f.on || empty);
      this.addr.fxp = this.addr.t === "k" ? f.page : undefined;
      if (this.addr.t === "p") this.addr.b = f.page;
      const tabs = this.fxEl.querySelector(".tabs"), sig = f.pages.join(",");
      if (tabs.dataset.sig !== sig) {
        tabs.dataset.sig = sig;
        tabs.innerHTML = f.pages.length > 1 ? f.pages.map((n, m) => `<button data-m="${m}">${esc(n)}</button>`).join("") : "";
        tabs.querySelectorAll("button").forEach((b) => { b.onclick = () => { if (this.addr.t === "k") this.addr.fxp = +b.dataset.m; else this.addr.b = +b.dataset.m; this.read(); }; });
      }
      tabs.querySelectorAll("button").forEach((b, m) => b.setAttribute("aria-pressed", String(m === f.page)));
    }
    if (this.opt.onShow) this.opt.onShow(ks);
  }
  async read() {
    if (!out) return;
    try { const r = await knobReq(this.addr, 0, 0); if (!r.code) this.show(r.ks); else this.el.classList.add("hidden"); } catch (e) { /* lostLink says it */ }
  }
  turn(i, n) {
    if (!out) return;
    const p = pending.get(this) || [0, 0, 0, 0, 0, 0];
    p[i] += n; pending.set(this, p);
    lastTurn = Date.now(); turnedAny = true;
    if (this.knobs[i]) this.knobs[i].classList.add("busy");
    const k = i < 4 && this.ks && this.ks[i];
    if (k && k.bar !== null) {              /* (0.9.65, FLEXOR "fluide": the knob moves now; the FM-1 follows, and once
                                             * quiet its own value is shown again: what it didn't take, undone) */
      this.loc[i] = clamp((this.loc[i] !== null ? this.loc[i] : k.bar) + n * this.step[i], 0, 1);
      this.held[i] = performance.now();
      this.paint();
      clearTimeout(this.settleT);
      this.settleT = setTimeout(() => this.paint(), SETTLE + 30);
    }
    flush();
  }
  learn(i, before, d) {                     /* (a click's worth from what the FM-1 did with d clicks) */
    const k = this.ks && this.ks[i];
    if (!k || k.bar === null || before === null || !d) return;
    const s = Math.abs(k.bar - before) / Math.abs(d);
    if (s > 0.0005 && k.bar > 0.001 && k.bar < 0.999) this.step[i] = clamp(s, 0.002, 0.5);
  }
}
const SETTLE = 450;                         /* (ms after the last move: the FM-1's own value shown again) */
const busyNow = () => Date.now() - lastTurn < 600 || [...pending.values()].some((a) => a.some((x) => x));
async function flush() {
  if (flushing) return;
  flushing = true;
  try {
    for (;;) {
      let job = null;
      for (const [panel, p] of pending) { const i = p.findIndex((v) => v); if (i >= 0) { job = [panel, p, i]; break; } }
      if (!job) break;
      const [panel, p, i] = job, d = clamp(p[i], -60, 60);
      p[i] -= d;
      const before = panel.ks && panel.ks[i] ? panel.ks[i].bar : null;
      const r = await knobReq(panel.addr, i, d);
      if (!r.code) { panel.ks = r.ks; panel.learn(i, before, d); panel.paint(); }
      if (!p[i] && panel.knobs[i]) panel.knobs[i].classList.remove("busy");
      if (panel.opt.onTurn) panel.opt.onTurn(i, d);
      if (panel.opt.all) turnedAll = true;
      if (performance.now() - lastPollT > 700) await poll(true);   /* (the live deck fed while a knob turns: now and
                                             * then, 0.9.65: not between every click) */
    }
  } catch (e) {                             /* (no answer: what moved here goes back to the FM-1's value) */
    for (const pn of pending.keys()) { pn.loc = [null, null, null, null]; pn.paint(); }
    pending.clear(); document.querySelectorAll(".kn.busy").forEach((k) => k.classList.remove("busy"));
  }
  flushing = false;
}

/* ================================================================== the FM-1's state (STATUS) and the transport */
const S = { connected: false, playing: false, tempo: 0, ext: 0, brk: 0, nbrk: 0, bstep: 255, bar64: 255, pat: 0, next: 255,
  trk: 0, trkOn: 0, trkPos: 0, done: 0, total: 0, dropIn: -1, kslot: 0, novice: 0, dirty: 0, bhalf: 0, wstyle: 0, w: 0, bar: 0, t: 0 };
let breaks = [], scopeSince = 0;
const SCOPE = 4096, ring = new Uint8Array(SCOPE * 4);
let ringW = 0, anchor = null;               /* the columns we hold up to ringW; anchor: the column a step begins at */
function readStatus(d) {
  const o = { playing: !!d[2], tempo: d[3] | d[4] << 8, ext: d[5], brk: d[6], nbrk: d[7], bstep: d[8], bar64: d[9], pat: d[10], next: d[11],
    trk: d[12], trkOn: d[13], trkPos: d[14], done: d[15] | d[16] << 8, total: d[17] | d[18] << 8, dropIn: d[19] << 24 >> 24,
    kslot: d[20], novice: d[21], dirty: d[22], bhalf: d[23], wstyle: d[24], w: (d[25] | d[26] << 8 | d[27] << 16 | d[28] << 24) >>> 0,
    bar: d[29] | d[30] << 8, t: performance.now() };
  const n = d[31];
  for (let i = 0; i < n; i++) {
    const c = (o.w - n + i) >>> 0;
    ring.set(d.subarray(32 + 4 * i, 36 + 4 * i), (c % SCOPE) * 4);
  }
  const start = (o.w - n) >>> 0;
  if (n && ringW && start > ringW)            /* (columns never received: black, not an old lap's) */
    for (let c = Math.max(ringW, start - SCOPE); c < start; c++) ring.fill(0, (c % SCOPE) * 4, (c % SCOPE) * 4 + 4);
  if (n) ringW = o.w;
  if (o.playing && o.bar64 !== 255) {         /* (the grid under the scope: 15 columns a step, the bar's first lit) */
    const abs = o.bar * 16 + (o.bar64 % 16), base = o.w - abs * 15;
    if (!anchor || !S.playing || o.bar < S.bar || base < anchor.base - 30 || base > anchor.base + 30) anchor = { base };
    else anchor.base = Math.min(anchor.base, base);
  }
  return o;
}
async function poll(force) {               /* (force: between a view's own requests, which come first otherwise) */
  if (!out || !ready || polling || (!force && (inflight || flushing || busyNow()))) return;
  polling = true; lastPollT = performance.now();
  try {
    const since = scopeSince >>> 0;
    const r = await ask(13, [1, 1, since & 255, since >> 8 & 255, since >> 16 & 255, since >>> 24], 1000);
    const was = { ...S };
    Object.assign(S, readStatus(r.data));
    scopeSince = S.w;
    showStatus(was);
  } catch (e) { /* lostLink */ }
  polling = false;
}
function fmtTempo(t) { return (t / 10).toFixed(1); }
function showStatus(was) {
  $("play").textContent = S.playing ? "■" : "▶"; $("play").classList.toggle("on", S.playing);
  if (document.activeElement !== $("tempo") || !tempoDrag) $("tempo").textContent = fmtTempo(S.tempo) + (S.ext ? " EXT" : "");
  $("patNow").textContent = "P" + (S.pat + 1);
  $("patNext").textContent = S.next !== 255 ? "→ P" + (S.next + 1) : "";
  $("trkNow").textContent = String(S.trk + 1);
  $("trkProg").style.width = (S.trkOn && S.total ? Math.min(100, S.done / S.total * 100) : 0) + "%";
  $("dropIn").textContent = S.trkOn && S.dropIn > 0 ? "DROP IN " + S.dropIn : S.trkOn ? "TRACK ▶" : "";
  $("barNow").textContent = S.playing ? (S.bar + 1) + "." + (S.bar64 !== 255 ? ((S.bar64 % 16) >> 2) + 1 : 1) : "—";
  $("kickNow").textContent = String(S.kslot + 1);
  $("save").classList.toggle("dirty", !!S.dirty);
  $("save").title = S.dirty ? "Unsaved changes: Save (Ctrl+S)" : "Save (Ctrl+S)";
  if (S.wstyle !== wstyle) { wstyle = S.wstyle; $("wstyle").textContent = WS_NAMES[wstyle] || "RGB"; if (view && view.redrawWaves) view.redrawWaves(); }
  if ($("brkSel").value !== String(S.brk) && document.activeElement !== $("brkSel")) $("brkSel").value = String(S.brk);
  if (view && view.status) view.status(was);
}
/* the tempo: drag up and down, the wheel, the arrows (Shift: ten times), a double-click to type it */
let tempoDrag = false;
function tempoBy(d) {
  if (!out || S.ext) return;
  S.tempo = clamp(S.tempo + d, 400, 3000);
  $("tempo").textContent = fmtTempo(S.tempo);
  clearTimeout(tempoBy.t);
  tempoBy.t = setTimeout(() => ask(13, [8, 1, S.tempo & 255, S.tempo >> 8]).catch(() => {}), 60);
}
(() => {
  const t = $("tempo");
  let y0 = null, acc = 0;
  t.addEventListener("pointerdown", (e) => { y0 = e.clientY; acc = 0; tempoDrag = true; t.setPointerCapture(e.pointerId); });
  t.addEventListener("pointermove", (e) => { if (y0 === null) return; acc += (y0 - e.clientY) / 4; y0 = e.clientY; const n = Math.trunc(acc); if (n) { acc -= n; tempoBy(n * (e.shiftKey ? 1 : 10)); } });
  t.addEventListener("pointerup", () => { y0 = null; tempoDrag = false; });
  t.addEventListener("wheel", (e) => { e.preventDefault(); tempoBy((e.deltaY < 0 ? 1 : -1) * (e.shiftKey ? 1 : 10)); }, { passive: false });
  t.addEventListener("keydown", (e) => {
    if (e.key === "ArrowUp" || e.key === "ArrowRight") tempoBy(e.shiftKey ? 100 : 10);
    else if (e.key === "ArrowDown" || e.key === "ArrowLeft") tempoBy(e.shiftKey ? -100 : -10);
    else return;
    e.preventDefault(); e.stopPropagation();
  });
  t.addEventListener("dblclick", () => {
    const v = prompt("Tempo (40 – 300 BPM)", fmtTempo(S.tempo));
    if (v && !isNaN(+v)) tempoBy(Math.round(+v * 10) - S.tempo);
  });
})();
async function transport(op, v = 0) {
  if (!out) return;
  try {
    const r = await ask(13, [8, op, v & 255, v >> 8]);
    const was = { ...S };
    const n = r.data;
    if (n[0] === 1) { Object.assign(S, readStatus(n)); showStatus(was); }
  } catch (e) { /* lostLink */ }
}
async function act(a, arg) {                /* ED_ACT: 1 KICK n, 2 a hit, 3 SAVE, 4 PLAY / STOP */
  if (!out) return null;
  try {
    const r = await ask(9, [a, arg]);
    if (a === 3) toast("Saved on the FM-1.", true);
    return r;
  } catch (e) { return null; }
}
$("play").onclick = () => transport(3);
$("save").onclick = () => act(3, 0);
$("brkSel").onchange = () => transport(2, +$("brkSel").value);
$("brkPrev").onclick = () => transport(2, (S.brk + S.nbrk - 1) % Math.max(1, S.nbrk));
$("brkNext").onclick = () => transport(2, (S.brk + 1) % Math.max(1, S.nbrk));
$("wstyle").onclick = async () => {                /* (HOME › SETTINGS K2, WAVE TYPE) */
  await knobReq({ t: "p", pg: PG.HSET }, 1, wstyle >= 3 ? -3 : 1).catch(() => {});   /* (round: RGB, SERATO, 3 BAND, BLUE) */
  poll();
};

/* ================================================================== the deck: the mix scrolling by, live, as the FM-1's CDJ view */
let deckSig = "";
function drawDeck() {
  const cv = $("deck"), dpr = window.devicePixelRatio || 1, W = Math.round(cv.clientWidth * dpr), H = Math.round(cv.clientHeight * dpr);
  if (!W || !H) return;
  const sig = S.playing ? "" : [W, H, ringW, S.tempo, wstyle, !!out].join();   /* (0.9.65: stopped, drawn again only
                                             * when something changed: the page's hand free for the knobs) */
  if (sig && sig === deckSig) return;
  deckSig = sig;
  if (cv.width !== W || cv.height !== H) { cv.width = W; cv.height = H; }
  const g = cv.getContext("2d");
  g.fillStyle = "#070706"; g.fillRect(0, 0, W, H);
  const px = 2 * dpr;                        /* (a column's pixels: 15 a step, about 4 s across a wide screen) */
  const cps = S.tempo ? S.tempo / 10 * 4 * 15 / 60000 : 0;   /* columns a millisecond */
  const lag = 160;                           /* (drawn this far behind: smooth between two answers) */
  const est = S.playing ? ringW + Math.min(260, performance.now() - S.t) * cps - lag * cps : ringW;
  const right = W - 6 * dpr, mid = Math.round(H * 0.56);
  const n = Math.ceil(right / px);
  let pk = 70;                               /* (the scale: the loudest column in sight, as a CDJ's auto gain) */
  for (let k = 0; k < n; k++) { const c = Math.floor(est) - k; if (c >= 0 && c < ringW && ringW - c < SCOPE) pk = Math.max(pk, ring[(c % SCOPE) * 4 + 3]); }
  const amp = H * 0.42 * 255 / pk / 255;
  for (let k = 0; k < n; k++) {
    const c = Math.floor(est) - k;
    if (c < 0 || c >= ringW || ringW - c >= SCOPE) continue;
    const o = (c % SCOPE) * 4, v = [ring[o], ring[o + 1], ring[o + 2], ring[o + 3]];
    if (!v[3]) continue;
    const hh = v[3] * amp, x = right - (k + (est - Math.floor(est))) * px;
    const [r, gg, b] = colOf(v);
    g.fillStyle = `rgb(${r | 0},${gg | 0},${b | 0})`;
    g.fillRect(x, mid - hh, Math.ceil(px), 2 * hh + 1);
  }
  if (anchor && S.playing) {                 /* the beats, the bar's first lit */
    for (let k = 0; k < 64; k++) {
      const beatCol = Math.floor((est - anchor.base) / 60) * 60 - k * 60 + anchor.base;
      const x = right - (est - beatCol) * px;
      if (x < 0) break;
      const bar = Math.round((beatCol - anchor.base) / 60) % 4 === 0;
      g.fillStyle = bar ? accent : "#4a4336";
      g.fillRect(x, 0, Math.max(1, dpr), bar ? 9 * dpr : 6 * dpr);
      g.fillRect(x, H - (bar ? 9 : 6) * dpr, Math.max(1, dpr), (bar ? 9 : 6) * dpr);
    }
  }
  g.fillStyle = "#fff"; g.fillRect(right, 4 * dpr, 2 * dpr, H - 8 * dpr);
  $("deckBar").textContent = S.playing ? `${S.bar + 1}.${S.bar64 !== 255 ? ((S.bar64 % 16) >> 2) + 1 : 1}` : out ? "STOPPED" : "";
}
function frame() {
  drawDeck();
  if (view && view.anim) view.anim();
  requestAnimationFrame(frame);
}
requestAnimationFrame(frame);

/* ================================================================== the views */
const ICONS = {
  live: '<path d="M2 8h2l2-5 3 10 2-6 1.5 3H14"/>',
  kick: '<circle cx="8" cy="8" r="5.5"/><circle cx="8" cy="8" r="2"/>',
  drums: '<path d="M2 6c0-1.7 2.7-3 6-3s6 1.3 6 3v4c0 1.7-2.7 3-6 3s-6-1.3-6-3z"/><path d="M2 6c0 1.7 2.7 3 6 3s6-1.3 6-3"/>',
  seq: '<rect x="2" y="3" width="3" height="3"/><rect x="6.5" y="3" width="3" height="3"/><rect x="11" y="10" width="3" height="3"/><rect x="2" y="10" width="3" height="3"/>',
  pat: '<rect x="2" y="2" width="5" height="5" rx="1"/><rect x="9" y="2" width="5" height="5" rx="1"/><rect x="2" y="9" width="5" height="5" rx="1"/><rect x="9" y="9" width="5" height="5" rx="1"/>',
  trk: '<rect x="1.5" y="4" width="5" height="3" rx="1"/><rect x="7.5" y="4" width="7" height="3" rx="1"/><rect x="3.5" y="9.5" width="8" height="3" rx="1"/>',
  fx: '<path d="M3 13V8M3 5V3M8 13v-2M8 8V3M13 13V9M13 6V3"/><path d="M1.5 6.5h3M6.5 9.5h3M11.5 7.5h3"/>',
  mix: '<path d="M4 2v12M8 2v12M12 2v12"/><rect x="2.5" y="9" width="3" height="2" rx=".5"/><rect x="6.5" y="4" width="3" height="2" rx=".5"/><rect x="10.5" y="7" width="3" height="2" rx=".5"/>',
  arp: '<path d="M2 12l3-4 3 2 3-5 3 3"/><circle cx="14" cy="8" r="0.5"/>',
  sys: '<circle cx="8" cy="8" r="2.2"/><path d="M8 1.5v2M8 12.5v2M1.5 8h2M12.5 8h2M3.4 3.4l1.4 1.4M11.2 11.2l1.4 1.4M3.4 12.6l1.4-1.4M11.2 4.8l1.4-1.4"/>',
};
const VIEWS = [];
let view = null, viewTimer = 0;
function defView(v) { VIEWS.push(v); }
function buildNav() {
  const nav = $("nav");
  nav.innerHTML = "";
  VIEWS.forEach((v, i) => {
    const b = h("button", "", `<svg viewBox="0 0 16 16" aria-hidden="true">${ICONS[v.icon]}</svg>${v.title}<span class="k">⌥${(i + 1) % 10}</span>`);
    b.onclick = () => openView(v.id);
    b.dataset.id = v.id;
    nav.appendChild(b);
  });
}
function openView(id) {
  const v = VIEWS.find((x) => x.id === id) || VIEWS[0];
  if (view && view.leave) view.leave();
  view = v;
  document.querySelectorAll("#nav button").forEach((b) => b.setAttribute("aria-current", String(b.dataset.id === id)));
  const root = $("view");
  root.innerHTML = "";
  root.scrollTop = 0;
  pending.clear();
  v.panels = [];
  if (!out) { welcome(root); return; }
  try { localStorage.setItem("lb.view", id); } catch (e) { /* no storage */ }
  v.render(root);
  refreshView(true);
}
function addPanel(parent, p) { parent.appendChild(p.el); view.panels.push(p); return p; }
async function refreshView(first) {
  if (!out || !ready || !view) return;
  const v = view;
  for (const p of v.panels) {
    if (view !== v || busyNow()) return;   /* (0.9.65: a knob in the hand first) */
    if (performance.now() - lastPollT > 150) await poll(true);   /* (the status between, the deck never starved) */
    await p.read();
  }
  if (view === v && v.refresh && !busyNow()) await v.refresh(first);
}
setInterval(async () => {                    /* the status ~8 times a second; a turn quiet: the view again; idle: now and then */
  if (!out || !ready || document.hidden) return;
  await poll();
  if (!view || flushing) return;
  const quiet = Date.now() - lastTurn;
  if (turnedAny && quiet > 700) {           /* (after a turn: the whole view again only where one knob moves others) */
    turnedAny = false;
    if (turnedAll) { turnedAll = false; refreshView(false); }
    return;
  }
  const every = Math.max(8000, view.every || 8000);   /* (idle: what the FM-1's own knobs did, now and then) */
  if (quiet > every && Date.now() - viewTimer > every && !inflight) { viewTimer = Date.now(); refreshView(false); }
}, 125);
function vh(root, title, sub, right) {
  const e = h("div", "vh", `<div><h1>${title}</h1><p>${sub}</p></div><div class="r"></div>`);
  if (right) right(e.querySelector(".r"));
  root.appendChild(e);
  return e;
}
function welcome(root) {
  root.innerHTML = `<div class="welcome"><h1>LOAF BREAKER Studio</h1>
  <p>Your FM-1 on the computer, as a DAW: the kick and the drums with every setting at once, the sequencer, the 64
  patterns, the tracks of your live set, the effects and the mix, the waveforms in RGB as on its screen. Plug the FM-1 in
  by USB (LOAF BREAKER 0.9.43 or later), then <b>Connect</b> above. Chrome or Edge, on a computer.</p>
  <div class="feat"><div><b>KICK</b><span>Every part of the KICK menu, FX1 and FX2, the EQ, its RGB waveform redrawn as you turn.</span></div>
  <div><b>SEQUENCER</b><span>The break's 32 steps over its own waveform, the kick, snare, hat and clap over four bars.</span></div>
  <div><b>PATTERNS</b><span>The 64 patterns as cards, each with its picture: click to play it next.</span></div>
  <div><b>TRACKS</b><span>Your set as blocks on a timeline: drag patterns, drops and gaps, loops, kicks and breaks.</span></div>
  <div><b>LIVE</b><span>The break, the launcher, the track and its next drop, the macros: ready to play.</span></div>
  <div><b>FX · MIX</b><span>The effects chain as a rack, the levels as faders, the master and the sends.</span></div></div>
  <p class="hint">Every change is the FM-1's own: you hear it at once and its screen stays where it is. SAVE keeps it, as on the FM-1.
  <kbd>Space</kbd> play / stop · <kbd>Ctrl</kbd>+<kbd>S</kbd> save · <kbd>H</kbd> the kick · <kbd>Alt</kbd>+<kbd>1</kbd>…<kbd>0</kbd> the views.</p></div>`;
}

/* ================================================================== shared reads: the kick menu's map, the patterns, the tracks */
let info = null;                             /* ED_INFO: the KICK menu's sections and parts */
function readInfo(d) {
  const r = { kslot: d[1], playing: !!d[2], sects: [] };
  let k = 4;
  for (let s = 0; s < d[3]; s++) {
    const sect = { name: str(d, k, 8), parts: [] }, n = d[k + 8];
    k += 9;
    for (let j = 0; j < n; j++, k += 14) sect.parts.push({ p: d[k], name: str(d, k + 1, 12), ok: !!d[k + 13] });
    r.sects.push(sect);
  }
  return r;
}
const bank = [null, null, null, null];       /* each group's 16 patterns: {bars[4], mask[4][4], brk} */
async function readBank(g) {
  const r = await ask(13, [6, g]), d = r.data, ps = [];
  for (let i = 0; i < 16; i++) {
    const o = 2 + 40 * i, mask = [];
    for (let dr = 0; dr < 4; dr++) mask.push([0, 1, 2, 3].map((b) => d[o + 4 + 8 * dr + 2 * b] | d[o + 5 + 8 * dr + 2 * b] << 8));
    const brk = (d[o + 36] | d[o + 37] << 8 | d[o + 38] << 16 | d[o + 39] << 24) >>> 0;
    ps.push({ bars: [d[o], d[o + 1], d[o + 2], d[o + 3]], mask, brk, empty: !mask.some((m) => m.some((x) => x)) });
  }
  bank[g] = ps;
  return ps;
}
const patOf = (n) => bank[n >> 4] && bank[n >> 4][n & 15];
/* a pattern's picture: its four drums over four bars (each its colour), the break's steps under them */
function drawPat(cv, p, opt = {}) {
  const dpr = window.devicePixelRatio || 1, W = Math.round(cv.clientWidth * dpr) || 140, H = Math.round(cv.clientHeight * dpr) || 44;
  if (cv.width !== W || cv.height !== H) { cv.width = W; cv.height = H; }
  const g = cv.getContext("2d");
  g.clearRect(0, 0, W, H);
  if (!p) return;
  const rows = 5, rh = H / rows, cw = W / 64;
  for (let dr = 0; dr < 4; dr++) {
    const bars = p.bars[dr] || 1;
    for (let i = 0; i < 64; i++) {
      const src = i % (16 * bars), on = p.mask[dr][src >> 4] >> (src & 15) & 1;
      if (!on) continue;
      g.fillStyle = LANEHEX[dr];
      g.globalAlpha = i >= 16 * bars ? 0.35 : 1;
      g.fillRect(i * cw + 0.5, dr * rh + rh * 0.18, Math.max(1, cw - 1), rh * 0.64);
    }
  }
  g.globalAlpha = opt.dark ? 0.9 : 0.7;
  g.fillStyle = opt.dark ? "#1a1712" : "#8fb3c9";
  for (let i = 0; i < 64; i++) if (p.brk >> (i & 31) & 1) g.fillRect(i * cw + 0.5, 4 * rh + rh * 0.35, Math.max(1, cw - 1), rh * 0.3);
  g.globalAlpha = 1;
}
const tracks = [];                           /* each track: {end, from, it[32], rep[32], tb[32], tk[32]} */
let loopBars = 2, dropBars = 4;
function readTrack(d) {
  const t = { n: d[1], end: d[5], from: d[6], it: [...d.subarray(7, 39)], rep: [...d.subarray(39, 71)], tb: [...d.subarray(71, 103)], tk: [...d.subarray(103, 135)] };
  loopBars = d[135] || 2; dropBars = d[136] || 4;
  t.len = t.it.indexOf(0); if (t.len < 0) t.len = 32;
  tracks[t.n] = t;
  return t;
}
async function loadTrack(n) { const r = await ask(13, [3, n]); return readTrack(r.data); }
const itemBars = (t, i) => t.it[i] === 0xFE ? dropBars : t.it[i] === 0xFD ? 0 : t.rep[i] * loopBars;
const trackBars = (t) => { let b = 0; for (let i = 0; i < t.len; i++) b += itemBars(t, i); return b; };
const breakName = (i) => breaks[i] || "BREAK " + (i + 1);
async function patGo(n) {                     /* (as PRESETS: playing, at the loop's end; another group: SAVE OR STOP) */
  try {
    const r = await ask(12, [5, n, 0, 0, 0]);
    if (r.code) toast("Pattern " + (n + 1) + " is in another group of 16: SAVE (or STOP) first, this group's changes would be lost.");
    return r;
  } catch (e) { return null; }
}

/* ================================================================== LIVE */
let brkCols = null, brkSlices = [], brkBars = 2, brkFor = -1;
async function readBreakWave() {
  const r = await ask(13, [7, 4, 0, 0]), d = r.data;
  brkCols = colsOf(d, 2, 240);
  brkBars = d[962] || 2;
  brkSlices = [...d.subarray(964, 964 + d[963])];
  brkFor = S.brk;
}
function stepPos() {                           /* the break's step now, with its fraction (from the last status) */
  if (!S.playing || S.bstep === 255) return -1;
  const ms = 60000 / (S.tempo / 10 * 4) * (S.bhalf ? 2 : 1);
  return S.bstep + Math.min(0.999, (performance.now() - S.t) / ms);
}
defView({ id: "live", title: "Live", icon: "live", every: 4000,
  render(root) {
    vh(root, "Live", "The break, the launcher, the track and its next drop, the macros: your set at a glance.");
    const L = h("div", "live");
    const left = h("div"), right = h("div");
    L.append(left, right);
    root.appendChild(L);
    const bc = h("div", "card", `<header><h2>Break</h2><span class="mono" id="lvBrk"></span><div class="r"><span class="lab" id="lvSl"></span></div></header>
      <canvas class="wave" id="lvWave" style="height:150px"></canvas>`);
    left.appendChild(bc);
    const mc = h("div", "card", "<h2>Macros</h2>");
    mc.style.marginTop = "12px";
    left.appendChild(mc);
    addPanel(mc, new Panel({ t: "p", pg: PG.HOME }, "Home", { big: true }));
    addPanel(mc, new Panel({ t: "p", pg: PG.MIX, a: 0 }, "Levels", { big: true }));
    addPanel(mc, new Panel({ t: "p", pg: PG.MIX, a: 1 }, "Sends", { big: true }));
    const tc = h("div", "card", `<header><h2>Track</h2><div class="r"><button id="lvPlay">Play track</button><button id="lvStop">Stop</button></div></header>
      <div class="tabs" id="lvTrk" role="group" aria-label="Track"></div><div class="minitrk" id="lvMini" style="margin:10px 0"></div>
      <div class="row" style="justify-content:space-between"><span class="lab">Next drop</span><span class="count" id="lvDrop">—</span></div>`);
    right.appendChild(tc);
    const lc = h("div", "card", `<header><h2>Launcher</h2><div class="r"><span class="lab">Click: next pattern</span></div></header><div class="launch" id="lvLaunch"></div>`);
    lc.style.marginTop = "12px";
    right.appendChild(lc);
    for (let t = 0; t < 8; t++) { const b = h("button", "", String(t + 1)); b.onclick = () => ask(13, [5, 3, t]).then(() => { poll(); this.drawTrack(); }).catch(() => {}); $("lvTrk").appendChild(b); }
    $("lvPlay").onclick = () => ask(13, [5, 1, S.trk]).then(poll).catch(() => {});
    $("lvStop").onclick = () => ask(13, [5, 2, S.trk]).then(poll).catch(() => {});
    const ln = $("lvLaunch");
    for (let g = 0; g < 4; g++) {
      ln.appendChild(h("span", "lab gl", `P${g * 16 + 1}–${g * 16 + 16}`));
      for (let j = 0; j < 16; j++) {
        const n = g * 16 + j, b = h("button", "", String(n + 1));
        b.dataset.n = n; b.setAttribute("aria-label", "Pattern " + (n + 1));
        b.onclick = () => patGo(n).then(poll);
        ln.appendChild(b);
      }
    }
  },
  async refresh(first) {
    if (first || brkFor !== S.brk || !brkCols) await readBreakWave();
    if (first) { for (let g = 0; g < 4; g++) await readBank(g); for (let t = 0; t < 8; t++) await loadTrack(t); }
    else { await readBank(S.pat >> 4); await loadTrack(S.trk); }
    this.drawLaunch(); this.drawTrack(); this.drawBreak();
  },
  redrawWaves() { this.drawBreak(); },
  status(was) {
    if (was.brk !== S.brk) setTimeout(() => readBreakWave().then(() => this.drawBreak()).catch(() => {}), S.playing ? 2500 : 300);
    if (was.pat !== S.pat || was.next !== S.next) this.drawLaunch();
    if (was.trk !== S.trk) loadTrack(S.trk).then(() => this.drawTrack()).catch(() => {});
    $("lvDrop").textContent = S.trkOn && S.dropIn > 0 ? S.dropIn + (S.dropIn > 1 ? " bars" : " bar") : "—";
  },
  anim() { this.drawBreak(); this.trkHead(); },
  drawBreak() {
    const cv = $("lvWave");
    if (!cv || !brkCols) return;
    $("lvBrk").textContent = breakName(S.brk);
    $("lvSl").textContent = `${brkSlices.length} slices · ${brkBars} bars`;
    const pos = stepPos();
    drawRGB(cv, brkCols, { after: (g, W, H, dpr) => {
      g.font = `600 ${10 * dpr}px "IBM Plex Mono", monospace`;
      brkSlices.forEach((c, i) => {
        const x = c / 240 * W;
        g.fillStyle = "rgba(255,255,255,.28)"; g.fillRect(x, 0, Math.max(1, dpr), H);
        g.fillStyle = "#e8dfcc"; g.fillText(String(i + 1), x + 3 * dpr, 12 * dpr);
      });
      for (let b = 1; b < brkBars; b++) { g.fillStyle = accent; g.fillRect(b / brkBars * W, H - 6 * dpr, 2 * dpr, 6 * dpr); }
      if (pos >= 0) { const x = pos / 32 * W; g.fillStyle = "#fff"; g.fillRect(x - dpr, 0, 2 * dpr, H); }
    } });
  },
  drawLaunch() {
    document.querySelectorAll("#lvLaunch button").forEach((b) => {
      const n = +b.dataset.n, p = patOf(n);
      b.classList.toggle("has", !!p && !p.empty);
      b.setAttribute("aria-pressed", String(n === S.pat));
      b.classList.toggle("next", n === S.next);
      b.style.borderBottom = p && !p.empty ? `3px solid ${patCol(n)}` : "";
    });
  },
  drawTrack() {
    const t = tracks[S.trk], m = $("lvMini");
    document.querySelectorAll("#lvTrk button").forEach((b, i) => b.setAttribute("aria-pressed", String(i === S.trk)));
    if (!t || !m) return;
    const tot = Math.max(1, trackBars(t));
    m.innerHTML = "";
    for (let i = 0; i < t.len; i++) {
      const v = t.it[i], w = itemBars(t, i) / tot * 100;
      if (!w) continue;
      const d = h("div", "", v === 0xFE ? "DROP" : "P" + v + (t.rep[i] > 1 ? "×" + t.rep[i] : ""));
      d.style.width = w + "%";
      d.style.background = v === 0xFE ? "repeating-linear-gradient(135deg,#ff5a4a 0 6px,#c9392b 6px 12px)" : patCol(v - 1);
      if (v === 0xFE) d.style.color = "#fff";
      m.appendChild(d);
    }
    if (!t.len) m.appendChild(h("div", "", "EMPTY: build it on TRACKS")).style.cssText = "width:100%;color:var(--faint)";
    m.appendChild(h("i", "ph")).id = "lvPh";
  },
  trkHead() {
    const p = $("lvPh");
    if (!p) return;
    p.style.display = S.trkOn && S.total ? "" : "none";
    if (S.trkOn && S.total) p.style.left = Math.min(100, S.done / S.total * 100) + "%";
  },
});

/* ================================================================== KICK */
let kickZoom = 500, kickMode = "rgb";
defView({ id: "kick", title: "Kick", icon: "kick", every: 6000,
  render(root) {
    vh(root, "Kick", "Every part of the KICK menu at once, FX1 and FX2 too. Drag a knob, roll the wheel, or the arrows (Shift: five).", (r) => {
      r.innerHTML = `<span class="lab">Kick</span><div class="tabs" id="kSlots"></div><button id="kHit" class="go">▶ Hit <kbd>H</kbd></button>`;
    });
    for (let n = 0; n < 20; n++) {
      const b = h("button", "", String(n + 1));
      b.setAttribute("aria-label", "KICK " + (n + 1));
      b.onclick = () => act(1, n).then(() => { poll(); refreshView(true); });
      $("kSlots").appendChild(b);
    }
    $("kHit").onclick = () => act(2, 0);
    const top = h("div", "grid2");
    top.style.gridTemplateColumns = "minmax(0, 1fr) 270px";
    top.innerHTML = `<div class="card"><header><h2>The hit</h2><div class="r"><div class="tabs"><button data-m="rgb">RGB</button><button data-m="db">Level (dB)</button></div>
      <select id="kZoom" aria-label="Zoom">${[250, 500, 750, 1000, 1500, 2000].map((v) => `<option value="${v}">${v} ms</option>`).join("")}</select></div></header>
      <canvas class="wave" id="kWave" style="height:200px"></canvas></div><div class="card" id="kPre"><h2>Built-in kick</h2></div>`;
    root.appendChild(top);
    $("kZoom").value = String(kickZoom);
    $("kZoom").onchange = () => { kickZoom = +$("kZoom").value; this.wave(); };
    top.querySelectorAll("[data-m]").forEach((b) => { b.onclick = () => { kickMode = b.dataset.m; this.wave(); }; });
    const onTurn = () => { clearTimeout(this.wt); this.wt = setTimeout(() => this.wave(), SETTLE); };
    addPanel($("kPre"), new Panel({ t: "k", snd: 0, part: 32 }, "", { big: true, onTurn, label: "Built-in kick", all: true }));
    $("kPre").insertAdjacentHTML("beforeend", '<p class="hint" style="margin:6px 0 0">Turn it to load a built-in kick on this memory.</p>');
    const main = h("div", "grid3");
    main.style.marginTop = "12px";
    main.style.gridTemplateColumns = "repeat(auto-fit, minmax(310px, 1fr))";
    root.appendChild(main);
    const fx = h("div", "rack");
    const more = h("details", "card", "<summary class=\"st\" style=\"margin:0;cursor:pointer\">MORE · the SEL + knobs, their functions, tools, the EQ</summary>");
    more.style.marginTop = "12px";
    const moreIn = h("div", "grid3"); moreIn.style.marginTop = "10px"; more.appendChild(moreIn);
    for (const s of info.sects) {
      if (s.name === "FX1" || s.name === "FX2") {
        for (const p of s.parts) {
          const dev = h("div", "dev", `<div class="slot">${esc(s.name)} · THE KICK'S OWN</div>`);
          addPanel(dev, new Panel({ t: "k", snd: 0, part: p.p, fxp: 0 }, "", { fx: true, onTurn, label: s.name }));
          fx.appendChild(dev);
        }
        continue;
      }
      const card = h("div", "card", `<h2>${esc(s.name)}</h2>`);
      for (const p of s.parts) {
        if (s.name === "ATTACK" && p.p === 32) continue;
        addPanel(card, new Panel({ t: "k", snd: 0, part: p.p }, p.name, { onTurn, hide: (k) => k.name === "PRESET", all: s.name === "MORE" }));
      }
      (s.name === "MORE" ? moreIn : main).appendChild(card);
    }
    root.appendChild(h("div", "st", "FX1 · FX2"));
    root.appendChild(fx);
    root.appendChild(more);
  },
  async refresh() {
    document.querySelectorAll("#kSlots button").forEach((b, n) => b.setAttribute("aria-pressed", String(n === S.kslot)));
    await this.wave();
  },
  status(was) { if (was.kslot !== S.kslot) refreshView(true); },
  redrawWaves() { this.paint(); },
  async wave() {
    if (!out) return;
    if (busyNow()) { clearTimeout(this.wt); this.wt = setTimeout(() => this.wave(), SETTLE); return; }   /* (0.9.65: after
                                             * the hand, never in its way: the picture costs the FM-1 the most) */
    document.querySelectorAll(".vh + .grid2 [data-m]").forEach((b) => b.setAttribute("aria-pressed", String(b.dataset.m === kickMode)));
    try {
      if (kickMode === "rgb") { const r = await ask(13, [7, 0, kickZoom & 255, kickZoom >> 8]); this.cols = colsOf(r.data, 2, 240); this.db = null; }
      else { const r = await ask(10, [kickZoom & 255, kickZoom >> 8]); this.db = [...r.data]; }
    } catch (e) { return; }
    this.paint();
  },
  paint() {
    const cv = $("kWave");
    if (!cv) return;
    const ruler = (g, W, H, dpr) => {
      g.fillStyle = "#a39a85"; g.font = `600 ${10 * dpr}px "IBM Plex Mono", monospace`;
      for (let t = 0; t < kickZoom; t += kickZoom / 5) { const x = t / kickZoom * W; g.fillRect(x, H - 5 * dpr, dpr, 5 * dpr); g.fillText(Math.round(t) + " ms", x + 4 * dpr, H - 7 * dpr); }
    };
    if (kickMode === "rgb" && this.cols) drawRGB(cv, this.cols, { after: ruler });
    else if (this.db) {
      const dpr = window.devicePixelRatio || 1, W = Math.round(cv.clientWidth * dpr), H = Math.round(cv.clientHeight * dpr);
      if (cv.width !== W || cv.height !== H) { cv.width = W; cv.height = H; }
      const g = cv.getContext("2d"), n = this.db.length;
      g.fillStyle = "#070706"; g.fillRect(0, 0, W, H);
      g.fillStyle = accent;
      for (let i = 0; i < n; i++) {
        const a = Math.abs((this.db[i] << 24 >> 24) / 60), v = a > 0 ? Math.max(0, 1 + Math.log10(a) * 20 / 36) : 0, hh = v * (H / 2 - 8 * dpr);
        g.fillRect(i * W / n, H / 2 - hh, Math.ceil(W / n), Math.max(1, 2 * hh));
      }
      ruler(g, W, H, dpr);
    }
  },
});

/* ================================================================== DRUMS */
defView({ id: "drums", title: "Drums", icon: "drums", every: 6000,
  render(root) {
    vh(root, "Drums", "The snare, the hat and the clap: every page of each, side by side. Keys 1 2 3 play them.");
    const g = h("div", "grid3");
    root.appendChild(g);
    this.waves = [];
    DRUM.forEach((name, v) => {
      const c = h("div", "card", `<header><h2 style="color:${LANEC[v + 1]}">${name}</h2><div class="r"><button>▶ Hit <kbd>${v + 1}</kbd></button></div></header>
        <canvas class="wave" style="height:96px;margin-bottom:8px"></canvas>`);
      c.querySelector("button").onclick = () => act(2, v + 1);
      const cv = c.querySelector("canvas");
      this.waves.push({ cv, cols: null });
      const onTurn = () => { clearTimeout(this.wt); this.wt = setTimeout(() => this.wave(v), SETTLE); };
      DPAGE.forEach((pg, k) => addPanel(c, new Panel({ t: "k", snd: v + 1, part: k }, pg, { color: LANEC[v + 1], onTurn, label: name + " " + pg })));
      g.appendChild(c);
    });
  },
  async refresh() { for (let v = 0; v < 3; v++) await this.wave(v); },
  redrawWaves() { this.waves.forEach((w) => w.cols && drawRGB(w.cv, w.cols)); },
  async wave(v) {
    if (busyNow()) { clearTimeout(this.wt); this.wt = setTimeout(() => this.wave(v), SETTLE); return; }
    try { const r = await ask(13, [7, v + 1, 400 & 255, 400 >> 8]); this.waves[v].cols = colsOf(r.data, 2, 240); drawRGB(this.waves[v].cv, this.waves[v].cols); } catch (e) { /* lost */ }
  },
});

/* ================================================================== SEQUENCER (ED_SEQ 11, ED_STEP 12) */
const COND = ["ALWAYS", "75%", "50%", "25%", "1:2", "2:2", "ALWAYS", "ALWAYS"];
let sq = null, sel = { row: 0, col: 0 };       /* row 0..3 a drum, 4 the break; col 0..63 */
function readSeq(d) {
  const r = { pat: d[1], playing: !!d[2], bar64: d[3], bstep: d[4], slices: d[5], blen: d[6], bars: [...d.subarray(7, 11)],
    kslot: d[11], pslot: [...d.subarray(12, 15)], novice: d[15], changed: d[16], next: d[17], mask: [], sb: [], note: [], brk: [] };
  for (let r0 = 0; r0 < 4; r0++) r.mask.push([0, 1, 2, 3].map((c) => d[18 + 8 * r0 + 2 * c] | d[19 + 8 * r0 + 2 * c] << 8));
  let k = 50;
  for (let r0 = 0; r0 < 4; r0++) { const n = r0 ? 32 : 64; r.sb.push([...d.subarray(k, k + n)]); k += n; }
  r.note = [...d.subarray(210, 274)].map((v) => v << 24 >> 24);
  for (let i = 0; i < 32; i++) r.brk.push({ slice: d[274 + 3 * i], pitch: d[275 + 3 * i] << 24 >> 24, fl: d[276 + 3 * i] });
  return r;
}
const sqOn = (r, i) => !!(sq.mask[r][i >> 4] >> (i & 15) & 1);
const sbOf = (r, i) => sq.sb[r][r ? i & 31 : i & 63];
const sliceText = (b) => (b.slice === 0 ? "OFF" : b.slice === 1 ? "LOOP" : String(b.slice - 1));
const signed = (v, u) => (v > 0 ? "+" : "") + v + u;
async function seqOp(op, a, b, c, d) {
  if (!out) return;
  try {
    const r = await ask(12, [op, a, b, c, d & 255]);
    sq = readSeq(r.data); lastTurn = Date.now();
    if (r.code) toast(op === 5 ? "That pattern is in another group of 16: SAVE (or STOP) first." : "Nothing to undo.");
    if (view && view.id === "seq") view.draw();
  } catch (e) { /* lost */ }
}
defView({ id: "seq", title: "Sequencer", icon: "seq", every: 2500,
  render(root) {
    vh(root, "Sequencer", "Click a step: on or off, then its settings on the right. Right click: pick it without turning it. ← → ↑ ↓ move, Enter on / off, + − its HITS.", (r) => {
      r.innerHTML = `<button id="sqUndo">Undo</button>`;
    });
    $("sqUndo").onclick = () => seqOp(6, 0, 0, 0, 0);
    const ps = h("div", "pstrip", "");
    ps.id = "sqPats";
    for (let g = 0; g < 4; g++) {
      const box = h("div", "", `<span class="lab">P${g * 16 + 1}–${g * 16 + 16}</span><div class="g"></div>`);
      for (let j = 0; j < 16; j++) {
        const n = g * 16 + j, b = h("button", "", String(n + 1));
        b.setAttribute("aria-label", "Pattern " + (n + 1)); b.onclick = () => seqOp(5, n, 0, 0, 0);
        box.querySelector(".g").appendChild(b);
      }
      ps.appendChild(box);
    }
    root.appendChild(ps);
    root.appendChild(h("div", "seqwrap", '<div class="grid" id="grid" role="grid" aria-label="The pattern\'s steps" tabindex="0"></div>'));
    root.appendChild(h("div", "seqside", `<div class="card"><header><h2 id="stepT">Step</h2></header><div id="stepCtl"></div></div>
      <div class="card" id="barBox"><header><h2 id="barT">Bar</h2></header><div id="barCtl"></div></div>
      <div class="card"><h2>Groove</h2><div id="sqGroove"></div></div>`));
    addPanel($("sqGroove"), new Panel({ t: "p", pg: PG.GLOBAL, a: 6 }, ""));
    const grid = $("grid");
    grid.addEventListener("click", (e) => {
      const btn = e.target.closest("button");
      if (btn) { const r = +btn.dataset.r, d = +btn.dataset.d; seqOp(3, r, 0, btn.dataset.op === "snd" ? 1 : 3, d); return; }
      const c = e.target.closest(".cell");
      if (!c || c.classList.contains("echo")) return;
      sel = { row: +c.dataset.r, col: +c.dataset.i };
      if (sel.row < 4) seqOp(1, sel.row, sel.col, 0, 0); else this.draw();
    });
    grid.addEventListener("contextmenu", (e) => {
      const c = e.target.closest(".cell");
      if (!c || c.classList.contains("echo")) return;
      e.preventDefault(); sel = { row: +c.dataset.r, col: +c.dataset.i }; this.draw();
    });
    grid.addEventListener("keydown", (e) => {
      if (!sq || e.target.closest("button")) return;
      const rows = [4, 0, 1, 2, 3], ri = rows.indexOf(sel.row), maxc = (r) => (r === 4 ? 32 : 16 * sq.bars[r]) - 1;
      let { row, col } = sel;
      if (e.key === "ArrowRight") col = Math.min(maxc(row), col + 1);
      else if (e.key === "ArrowLeft") col = Math.max(0, col - 1);
      else if (e.key === "ArrowDown") { row = rows[Math.min(4, ri + 1)]; col = Math.min(col, maxc(row)); }
      else if (e.key === "ArrowUp") { row = rows[Math.max(0, ri - 1)]; col = Math.min(col, maxc(row)); }
      else if (e.key === "Enter") { if (row < 4) seqOp(1, row, col, 0, 0); e.preventDefault(); return; }
      else if (e.key === "+" || e.key === "=" || e.key === "-") { const d = e.key === "-" ? -1 : 1; row === 4 ? seqOp(4, 0, col & 31, 0, d) : seqOp(2, row, col, 1, d); e.preventDefault(); return; }
      else return;
      e.preventDefault(); e.stopPropagation();
      sel = { row, col }; this.draw(); grid.focus();
    });
    for (const id of ["stepCtl", "barCtl"]) {
      $(id).addEventListener("click", (e) => { const b = e.target.closest("button"); if (b) this.insp(b, e.shiftKey ? 5 : 1); });
      $(id).addEventListener("wheel", (e) => { const row = e.target.closest(".ctl"); if (!row) return; e.preventDefault(); this.insp(row.querySelectorAll("button")[e.deltaY < 0 ? 1 : 0], e.shiftKey ? 5 : 1); }, { passive: false });
    }
  },
  async refresh(first) {
    const r = await ask(11);
    sq = readSeq(r.data);
    if (first || brkFor !== S.brk || !brkCols) await readBreakWave();
    this.draw();
  },
  insp(btn, m) {
    const op = btn.dataset.op, k = +btn.dataset.k, d = +btn.dataset.d * m;
    if (op === "step") seqOp(2, sel.row, sel.col, k, d);
    else if (op === "brk") seqOp(4, 0, sel.col & 31, k, d);
    else if (op === "bar") seqOp(3, sel.row, sel.col >> 4, k, d);
  },
  status(was) {
    if (!sq) return;
    if (was.pat !== S.pat || was.next !== S.next) { ask(11).then((r) => { sq = readSeq(r.data); this.draw(); }).catch(() => {}); return; }
    if (was.bar64 !== S.bar64 || was.bstep !== S.bstep || was.playing !== S.playing) this.head();
    if (was.brk !== S.brk) setTimeout(() => readBreakWave().then(() => this.breakRow()).catch(() => {}), S.playing ? 2500 : 300);
  },
  redrawWaves() { this.breakRow(); },
  head() {                                   /* the steps playing, lit */
    document.querySelectorAll("#grid .cell.now").forEach((c) => c.classList.remove("now"));
    if (!S.playing) return;
    if (S.bstep !== 255) { const c = document.querySelector(`#grid .cell[data-r="4"][data-i="${S.bstep}"]`); if (c) c.classList.add("now"); }
    if (S.bar64 !== 255) for (let r = 0; r < 4; r++) {
      const src = S.bar64 % (16 * sq.bars[r]), c = document.querySelector(`#grid .cell[data-r="${r}"][data-i="${src}"]`);
      if (c) c.classList.add("now");
    }
  },
  cell(r, i) {
    const echo = r < 4 ? (i >> 4) >= sq.bars[r] : i >= 32, src = r < 4 && echo ? i % (16 * sq.bars[r]) : i;
    let cls = "cell" + (i % 16 === 0 && i ? " b16" : "") + (i % 4 === 0 ? " q" : "") + (echo ? " echo" : ""), inner = "", lab;
    if (r === 4) {
      const b = sq.brk[i & 31];
      cls += " brk" + (b.slice ? " on" : " off") + (b.fl & 1 ? " rev" : "");
      inner = `<span class="t">${b.slice === 0 ? "" : b.slice === 1 ? "" : b.slice - 1}</span>` +
        `<span class="pi">${b.pitch ? signed(b.pitch, "") : ""}${(b.fl >> 1 & 3) ? " x" + ((b.fl >> 1 & 3) + 1) : ""}${b.fl & 1 ? " ◀" : ""}</span>`;
      lab = `Break step ${(i & 31) + 1}: ${sliceText(b)}`;
    } else {
      const o = sqOn(r, src), b = sbOf(r, src);
      if (o) {
        cls += (b >> 2 & 7) && (b >> 2 & 7) < 6 ? " on cond" : " on";
        if (b & 3) inner += '<span class="h">' + "<i></i>".repeat((b & 3) + 1) + "</span>";
        if (b >> 5) inner += `<span class="nd" style="left:${Math.round((b >> 5) / 16 * 100 + 12)}%"></span>`;
        if (!r && sq.note[src]) inner += `<span class="t">${signed(sq.note[src], "")}</span>`;
      }
      lab = `${LANE[r]} step ${i + 1}: ${o ? "on" : "off"}`;
    }
    if (sel.row === r && sel.col === i) cls += " sel";
    return `<div class="${cls}" style="--lc:${r < 4 ? LANEC[r] : "var(--b)"}${r === 4 ? `;grid-row:2;grid-column:${i + 2}` : ""}" role="gridcell" data-r="${r}" data-i="${i}" aria-label="${esc(lab)}" aria-selected="${sel.row === r && sel.col === i}">${inner}</div>`;
  },
  draw() {
    if (!sq || !$("grid")) return;
    document.querySelectorAll("#sqPats button").forEach((b, n) => { b.setAttribute("aria-pressed", String(n === sq.pat)); b.classList.toggle("next", n === sq.next); });
    let hh = '<div class="hd">SOUND · STEPS</div>';
    for (let i = 0; i < 64; i++) hh += `<div class="hd${i % 16 ? "" : " bar"}">${i % 16 ? (i % 4 ? "" : i + 1) : "BAR " + (i / 16 + 1)}</div>`;
    hh += `<div class="lh"><b style="color:var(--b)">BREAK</b><div class="mini">${esc(breakName(S.brk))}</div><div class="mini">${sq.slices} slices</div></div>`;
    hh += '<canvas class="brkwave" id="sqWave"></canvas>';
    for (let i = 0; i < 64; i++) hh += this.cell(4, i);
    for (const r of [0, 1, 2, 3]) {
      const snd = "#" + ((r ? sq.pslot[r - 1] : sq.kslot) + 1);
      hh += `<div class="lh"><b style="color:${LANEC[r]}">${LANE[r]}</b><div class="mini">` +
        `<button data-op="snd" data-r="${r}" data-d="-1" aria-label="${LANE[r]}: the sound before">◀</button><span title="Its sound">${snd}</span>` +
        `<button data-op="snd" data-r="${r}" data-d="1" aria-label="${LANE[r]}: the next sound">▶</button>` +
        `<button data-op="len" data-r="${r}" data-d="-1" aria-label="${LANE[r]} shorter">◀</button><span title="Its steps">${16 * sq.bars[r]}</span>` +
        `<button data-op="len" data-r="${r}" data-d="1" aria-label="${LANE[r]} longer">▶</button></div></div>`;
      for (let i = 0; i < 64; i++) hh += this.cell(r, i);
    }
    $("grid").innerHTML = hh;                /* (row 2: the break's waveform, its cells over it) */
    this.breakRow();
    this.head();
    this.inspector();
  },
  breakRow() {                               /* the break's waveform under its 32 steps, each step's own columns, twice */
    const cv = $("sqWave");
    if (!cv || !brkCols || !$("grid")) return;
    const cells = [...document.querySelectorAll('#grid .cell[data-r="4"]')], box = cv.getBoundingClientRect();
    const dpr = window.devicePixelRatio || 1, W = Math.round(box.width * dpr), H = Math.round(box.height * dpr);
    if (!W) return;
    cv.width = W; cv.height = H;
    const g = cv.getContext("2d"), mid = H / 2;
    let pk = 1; for (const c of brkCols) pk = Math.max(pk, c[3]);
    cells.forEach((c, i) => {
      const r = c.getBoundingClientRect(), x0 = (r.left - box.left) * dpr, w = r.width * dpr, s = i & 31;
      g.globalAlpha = i >= 32 ? 0.35 : 1;
      const a = s * 240 / 32, n = 240 / 32;
      for (let x = 0; x < w; x++) {
        const col = brkCols[Math.min(239, Math.floor(a + x / w * n))];
        if (!col[3]) continue;
        const hh = col[3] / pk * (mid - 3 * dpr), [rr, gg, bb] = colOf(col, 120 + col[3] * 135 / pk);
        g.fillStyle = `rgb(${rr | 0},${gg | 0},${bb | 0})`;
        g.fillRect(x0 + x, mid - hh, 1, 2 * hh);
      }
    });
    g.globalAlpha = 1;
  },
  ctl(name, val, op, k) {
    return `<div class="ctl"><span class="n">${name}</span><button data-op="${op}" data-k="${k}" data-d="-1" aria-label="${name} down">−</button>` +
      `<span class="v" aria-live="polite">${esc(val)}</span><button data-op="${op}" data-k="${k}" data-d="1" aria-label="${name} up">+</button></div>`;
  },
  inspector() {
    const r = sel.row, i = sel.col;
    if (r === 4) {
      const b = sq.brk[i & 31];
      $("stepT").textContent = `Break step ${(i & 31) + 1}`;
      $("stepCtl").innerHTML = this.ctl("SLICE", sliceText(b), "brk", 0) + this.ctl("PITCH", signed(b.pitch, " ST"), "brk", 1) +
        this.ctl("REPEAT", "X" + ((b.fl >> 1 & 3) + 1), "brk", 2) + this.ctl("REVERSE", b.fl & 1 ? "REVERSE" : "FORWARD", "brk", 3);
      $("barBox").classList.add("hidden");
      return;
    }
    const b = sbOf(r, i);
    $("stepT").innerHTML = `<span style="color:${LANEC[r]}">${LANE[r]}</span> step ${i + 1}${sqOn(r, i) ? "" : " (off)"}`;
    $("stepCtl").innerHTML = (r ? "" : this.ctl("NOTE", signed(sq.note[i], " ST"), "step", 0)) +
      this.ctl("HITS", (b & 3) + 1 + ((b & 3) ? " HITS" : " HIT"), "step", 1) + this.ctl("COND", COND[b >> 2 & 7], "step", 2) +
      this.ctl("NUDGE", b >> 5 ? "+" + (b >> 5) + "/16" : "ON TIME", "step", 3);
    $("barBox").classList.remove("hidden");
    const bar = i >> 4;
    let n = 0; for (let x = sq.mask[r][bar]; x; x >>= 1) n += x & 1;
    $("barT").innerHTML = `<span style="color:${LANEC[r]}">${LANE[r]}</span> bar ${bar + 1}`;
    $("barCtl").innerHTML = this.ctl("FILL", n + " HITS", "bar", 0) +
      `<div class="row" style="margin-top:8px"><button data-op="bar" data-k="2" data-d="-1">Clear bar</button><button data-op="bar" data-k="2" data-d="1">4 × 4</button></div>`;
  },
});

/* ================================================================== PATTERNS */
defView({ id: "pat", title: "Patterns", icon: "pat", every: 3000,
  render(root) {
    vh(root, "Patterns", "The 64 patterns, each with its picture: the kick, snare, hat and clap over four bars, the break's steps under them. Click: it plays next (at the loop's end). Double click: open it in the sequencer.");
    const b = h("div", "bank");
    b.id = "bank";
    root.appendChild(b);
    for (let g = 0; g < 4; g++) {
      b.appendChild(h("div", "st", `P${g * 16 + 1} – ${g * 16 + 16}<span class="lab" id="bkg${g}" style="letter-spacing:.06em"></span>`));
      const row = h("div", "grow");
      for (let j = 0; j < 16; j++) {
        const n = g * 16 + j, c = h("button", "pc", `<div class="top">P${n + 1}<small></small></div><canvas></canvas>`);
        c.dataset.n = n;
        c.onclick = () => patGo(n).then(poll);
        c.ondblclick = () => patGo(n).then(() => openView("seq"));
        row.appendChild(c);
      }
      b.appendChild(row);
    }
  },
  async refresh(first) {
    if (first) for (let g = 0; g < 4; g++) await readBank(g);
    else await readBank(S.pat >> 4);
    this.draw();
  },
  status(was) { if (was.pat !== S.pat || was.next !== S.next || was.dirty !== S.dirty) this.draw(); },
  draw() {
    document.querySelectorAll("#bank .pc").forEach((c) => {
      const n = +c.dataset.n, p = patOf(n);
      c.classList.toggle("cur", n === S.pat); c.classList.toggle("next", n === S.next); c.classList.toggle("empty", !p || p.empty);
      c.querySelector("small").textContent = p ? (p.empty ? "empty" : LANE.map((l, i) => l[0] + 16 * p.bars[i]).join(" ")) : "";
      drawPat(c.querySelector("canvas"), p);
      c.setAttribute("aria-label", `Pattern ${n + 1}${n === S.pat ? ", playing" : ""}${p && p.empty ? ", empty" : ""}`);
    });
    for (let g = 0; g < 4; g++) { const e = $("bkg" + g); if (e) e.textContent = g === S.pat >> 4 ? (S.dirty & 2 ? "  · IN THE FM-1'S MEMORY, NOT SAVED" : "  · IN THE FM-1'S MEMORY") : ""; }
  },
});

/* ================================================================== TRACKS: the set as blocks on a timeline */
const GAPK = ["NOTHING", "BREAK", "DRUMS", "ECHOES"], ENDM = ["LOOP", "STOP", "HOLD"];
const tv = { t: -1, sel: -1, ppb: 26, wt: 0 };
function trkSave() {                          /* (the page's whole track to the FM-1, a moment after the last change) */
  clearTimeout(tv.wt);
  tv.wt = setTimeout(async () => {
    const t = tracks[tv.t];
    if (!t) return;
    try {
      const r = await ask(13, [4, tv.t, t.end, t.from, ...t.it, ...t.rep, ...t.tb, ...t.tk]);
      readTrack(r.data);
      if (view && view.id === "trk") view.draw();
    } catch (e) { /* lost */ }
  }, 220);
}
function trkEdit(fn) {                        /* a change to the track shown: its items rebuilt, drawn, sent */
  const t = tracks[tv.t];
  if (!t) return;
  const items = [];
  for (let i = 0; i < t.len; i++) items.push({ it: t.it[i], rep: t.rep[i], tb: t.tb[i], tk: t.tk[i] });
  const r = fn(items, t);
  if (r === false) return;
  if (items.length > 32) { toast("A track holds 32 items at most."); return; }
  for (let i = 0; i < 32; i++) {
    const x = items[i];
    t.it[i] = x ? x.it : 0; t.rep[i] = x ? x.rep || 1 : 1; t.tb[i] = x ? x.tb || 0 : 0; t.tk[i] = x ? x.tk || 0 : 0;
  }
  t.len = items.length;
  if (t.from >= t.len) t.from = 0;
  if (view && view.id === "trk") view.draw();
  trkSave();
}
const newItem = (kind) => kind === "drop" ? { it: 0xFE, rep: 1, tb: 0, tk: 0 } : kind === "gap" ? { it: 0xFD, rep: 1, tb: 0, tk: 0 } : { it: +kind + 1, rep: 2, tb: 0, tk: 0 };
defView({ id: "trk", title: "Tracks", icon: "trk", every: 3000,
  render(root) {
    if (tv.t < 0) tv.t = S.trk;
    vh(root, "Tracks", "Your set as blocks: drag patterns, DROPs and gaps in from the right, drag a block to move it. Click one to set its loops, kick and break. Delete removes it, Ctrl+D doubles it.");
    root.appendChild(h("div", "tlayout", `<div><div class="tlist" id="tList"></div></div>
      <div class="arr"><header class="row" style="margin-bottom:8px"><b class="mono" id="tTitle" style="font-size:15px"></b><span class="lab" id="tLen"></span>
        <span style="flex:1"></span><button id="tPlay" class="go">▶ Play track</button><button id="tStop">■ Stop</button>
        <span class="lab">At the end</span><div class="tabs" id="tEnd"></div><button id="tZo" aria-label="Zoom out">−</button><button id="tZi" aria-label="Zoom in">+</button></header>
        <div class="tl" id="tTl"><div class="inner" id="tIn"><div class="ruler" id="tRuler"></div><div class="lane" id="tLane"></div></div></div>
        <p class="hint" style="margin:6px 0 0">Played left to right, each block its loops; a DROP is the build, then the drop into the pattern after it; a gap is played past.</p></div>
      <div><div class="card insp" id="tInsp"></div><div class="card" style="margin-top:12px"><h2>Add</h2><div class="pal" id="tPal"></div></div></div>`));
    for (let t = 0; t < 8; t++) {
      const b = h("button", "", `TRACK ${t + 1}<small></small>`);
      b.onclick = () => { tv.t = t; tv.sel = -1; ask(13, [5, 3, t]).then(() => { poll(); this.draw(); }).catch(() => {}); };
      $("tList").appendChild(b);
    }
    ENDM.forEach((n, m) => { const b = h("button", "", n); b.onclick = () => { const t = tracks[tv.t]; if (t) { t.end = m; this.draw(); trkSave(); } }; $("tEnd").appendChild(b); });
    $("tPlay").onclick = () => ask(13, [5, 1, tv.t]).then(poll).catch(() => {});
    $("tStop").onclick = () => ask(13, [5, 2, tv.t]).then(poll).catch(() => {});
    $("tZo").onclick = () => { tv.ppb = Math.max(8, tv.ppb / 1.4); this.draw(); };
    $("tZi").onclick = () => { tv.ppb = Math.min(90, tv.ppb * 1.4); this.draw(); };
    const pal = $("tPal");
    const addBtn = (label, kind, cls, n) => {
      const b = h("button", cls || "", label);
      b.draggable = true;
      b.addEventListener("dragstart", (e) => { e.dataTransfer.setData("text/lb", "new:" + kind); e.dataTransfer.effectAllowed = "copy"; });
      b.onclick = () => trkEdit((items) => { items.splice(tv.sel >= 0 ? tv.sel + 1 : items.length, 0, newItem(kind)); tv.sel = tv.sel >= 0 ? tv.sel + 1 : items.length - 1; });
      if (n !== undefined) { b.dataset.n = n; b.setAttribute("aria-label", "Add pattern " + (n + 1)); }
      pal.appendChild(b);
    };
    addBtn("DROP", "drop", "wide");
    addBtn("GAP", "gap", "wide");
    for (let n = 0; n < 64; n++) addBtn(String(n + 1), String(n), "", n);
    const lane = $("tLane");
    lane.addEventListener("dragover", (e) => {
      e.preventDefault();
      const at = this.insAt(e.clientX), ins = $("tIns") || lane.parentNode.appendChild(Object.assign(h("div", "ins"), { id: "tIns" }));
      ins.style.left = (this.xs[at] !== undefined ? this.xs[at] : this.xEnd) - 2 + "px";
    });
    lane.addEventListener("dragleave", () => { const i = $("tIns"); if (i) i.remove(); });
    lane.addEventListener("drop", (e) => {
      e.preventDefault();
      const i = $("tIns"); if (i) i.remove();
      const what = e.dataTransfer.getData("text/lb"), at = this.insAt(e.clientX);
      if (what.startsWith("new:")) trkEdit((items) => { items.splice(at, 0, newItem(what.slice(4))); tv.sel = at; });
      else if (what.startsWith("blk:")) {
        const from = +what.slice(4);
        trkEdit((items) => { const [x] = items.splice(from, 1); const to = at > from ? at - 1 : at; items.splice(to, 0, x); tv.sel = to; });
      }
    });
    root.addEventListener("keydown", (e) => {
      if (e.target.closest("input, select") || tv.sel < 0) return;
      if (e.key === "Delete" || e.key === "Backspace") { trkEdit((items) => { items.splice(tv.sel, 1); tv.sel = Math.min(tv.sel, items.length - 1); }); e.preventDefault(); }
      else if ((e.ctrlKey || e.metaKey) && e.key.toLowerCase() === "d") { trkEdit((items) => { items.splice(tv.sel + 1, 0, { ...items[tv.sel] }); tv.sel++; }); e.preventDefault(); }
      else if (e.altKey && (e.key === "ArrowLeft" || e.key === "ArrowRight")) {
        const d = e.key === "ArrowLeft" ? -1 : 1;
        trkEdit((items) => { const j = tv.sel + d; if (j < 0 || j >= items.length) return false; [items[tv.sel], items[j]] = [items[j], items[tv.sel]]; tv.sel = j; });
        e.preventDefault();
      } else if (e.key === "ArrowLeft" || e.key === "ArrowRight") {
        const t = tracks[tv.t]; if (!t) return;
        tv.sel = clamp(tv.sel + (e.key === "ArrowLeft" ? -1 : 1), 0, t.len - 1); this.draw(); e.preventDefault();
      }
    });
  },
  async refresh(first) {
    if (first) { for (let g = 0; g < 4; g++) await readBank(g); for (let t = 0; t < 8; t++) await loadTrack(t); }
    else if (!tv.wt || Date.now() - lastTurn > 1500) await loadTrack(tv.t);
    this.draw();
  },
  status(was) { if (was.trkOn !== S.trkOn || was.trkPos !== S.trkPos || was.trk !== S.trk) this.draw(); },
  anim() {
    const ph = $("tPh"), t = tracks[tv.t];
    if (!ph || !t) return;
    const on = S.trkOn && S.trk === tv.t && S.total;
    ph.style.display = on ? "" : "none";
    if (on) ph.style.left = this.barX(S.done) + "px";
  },
  insAt(clientX) {
    const lane = $("tLane"), x = clientX - lane.getBoundingClientRect().left;
    const t = tracks[tv.t];
    for (let i = 0; i < (t ? t.len : 0); i++) if (x < this.xs[i] + this.ws[i] / 2) return i;
    return t ? t.len : 0;
  },
  barX(b) {                                  /* a bar's place on the timeline (a gap takes room but no time) */
    const t = tracks[tv.t];
    let x = 0, acc = 0;
    for (let i = 0; t && i < t.len; i++) {
      const n = itemBars(t, i);
      if (n && b < acc + n) return this.xs[i] + (b - acc) / n * this.ws[i];
      acc += n;
    }
    x = this.xEnd;
    return x;
  },
  draw() {
    const t = tracks[tv.t];
    document.querySelectorAll("#tList button").forEach((b, i) => {
      b.setAttribute("aria-pressed", String(i === tv.t));
      const tt = tracks[i];
      b.querySelector("small").innerHTML = (S.trkOn && S.trk === i ? '<span class="pl">▶ </span>' : "") + (tt ? (tt.len ? trackBars(tt) + " bars" : "empty") : "");
    });
    if (!t || !$("tLane")) return;
    $("tTitle").textContent = "TRACK " + (tv.t + 1);
    $("tLen").textContent = `${t.len} items · ${trackBars(t)} bars · a loop ${loopBars} bars, a drop's build ${dropBars}`;
    document.querySelectorAll("#tEnd button").forEach((b, m) => b.setAttribute("aria-pressed", String(m === t.end)));
    const lane = $("tLane"), ruler = $("tRuler");
    lane.innerHTML = ""; ruler.innerHTML = "";
    this.xs = []; this.ws = [];
    let x = 0;
    for (let i = 0; i < t.len; i++) {
      const v = t.it[i], bars = itemBars(t, i), w = Math.max(v === 0xFD ? 30 : 44, bars * tv.ppb);
      this.xs.push(x); this.ws.push(w);
      const b = h("div", "blk");
      b.style.left = x + "px"; b.style.width = (w - 3) + "px";
      b.tabIndex = 0; b.draggable = true;
      if (v === 0xFE) {
        b.classList.add("drop");
        b.innerHTML = `<b>DROP</b><small>build ${bars} bars${t.tb[i] & 1 ? " · STUTTER" : ""}</small><small>gap: ${GAPK[t.tb[i] >> 1 & 3]}${t.tk[i] ? " · K" + t.tk[i] : ""}</small>`;
      } else if (v === 0xFD) {
        b.classList.add("gap");
        b.innerHTML = "<b>GAP</b><small>fill me</small>";
      } else {
        b.style.background = patCol(v - 1);
        const extra = [t.tk[i] ? "K" + t.tk[i] : "", t.tb[i] ? breakName(t.tb[i] - 1) : ""].filter(Boolean).join(" · ");
        b.innerHTML = `<b>P${v}</b><small>×${t.rep[i]} · ${bars} bars${extra ? " · " + esc(extra) : ""}</small><canvas></canvas>`;
      }
      b.classList.toggle("sel", i === tv.sel);
      b.classList.toggle("playing", S.trkOn && S.trk === tv.t && S.trkPos === i);
      b.setAttribute("aria-label", `Item ${i + 1}: ${v === 0xFE ? "DROP" : v === 0xFD ? "GAP" : "P" + v + " ×" + t.rep[i]}`);
      b.onclick = () => { tv.sel = i; this.draw(); b.focus(); };
      b.addEventListener("dragstart", (e) => { e.dataTransfer.setData("text/lb", "blk:" + i); e.dataTransfer.effectAllowed = "move"; b.classList.add("drag"); });
      b.addEventListener("dragend", () => b.classList.remove("drag"));
      lane.appendChild(b);
      const cv = b.querySelector("canvas");
      if (cv) drawPat(cv, patOf(v - 1), { dark: true });
      x += w;
    }
    this.xEnd = x;
    const end = h("div", "endm", `<span class="lab">${t.len ? "END · " + ENDM[t.end] : "Drag a pattern here, or click one on the right"}</span>`);
    end.style.left = x + "px";
    lane.parentNode.querySelectorAll(".endm, .from, .ph").forEach((e) => e.remove());
    lane.parentNode.appendChild(end);
    $("tIn").style.width = Math.max(x + 240, $("tTl").clientWidth) + "px";
    const total = trackBars(t);
    for (let b = 0; b <= total; b += 4) { const s = h("span", "", String(b + 1)); s.style.left = this.barX(b) + "px"; ruler.appendChild(s); }
    if (t.len) { const f = h("div", "from", "▶ START"); f.style.left = (this.xs[t.from] || 0) + 2 + "px"; ruler.appendChild(f); }
    const ph = h("div", "ph"); ph.id = "tPh"; lane.parentNode.appendChild(ph);
    this.anim();
    this.inspector();
    document.querySelectorAll("#tPal button[data-n]").forEach((b) => { const p = patOf(+b.dataset.n); b.style.boxShadow = p && !p.empty ? `inset 0 -3px 0 ${patCol(+b.dataset.n)}` : ""; });
  },
  ctl(name, val, k) {
    return `<div class="ctl"><span class="n">${name}</span><button data-k="${k}" data-d="-1" aria-label="${name} down">−</button>` +
      `<span class="v">${esc(val)}</span><button data-k="${k}" data-d="1" aria-label="${name} up">+</button></div>`;
  },
  inspector() {
    const box = $("tInsp"), t = tracks[tv.t];
    if (!box || !t) return;
    if (tv.sel < 0 || tv.sel >= t.len) {
      box.innerHTML = `<h2>Item</h2><p class="hint" style="margin:0 0 10px">Click a block to set it. The track's DROPs and builds:</p>`;
    } else {
      const i = tv.sel, v = t.it[i];
      let c = "";
      if (v === 0xFE) c = this.ctl("BUILD KICK", t.tk[i] ? "KICK " + t.tk[i] : "SAME", "tk") + this.ctl("STUTTER", t.tb[i] & 1 ? "ON" : "OFF", "st") + this.ctl("GAP KEEPS", GAPK[t.tb[i] >> 1 & 3], "gk");
      else if (v === 0xFD) c = `<p class="hint" style="margin:0">A gap: played past. Drag a pattern or a DROP onto it, or delete it.</p>` + this.ctl("FILL WITH", "P1", "fill");
      else c = this.ctl("PATTERN", "P" + v, "pat") + this.ctl("LOOPS", "×" + t.rep[i], "rep") + this.ctl("KICK", t.tk[i] ? "KICK " + t.tk[i] : "SAME", "tk") +
        this.ctl("BREAK", t.tb[i] ? breakName(t.tb[i] - 1) : "SAME", "tb");
      box.innerHTML = `<h2>Item ${i + 1} · ${v === 0xFE ? "DROP" : v === 0xFD ? "GAP" : "PATTERN"}</h2>${c}
        <div class="row" style="margin:10px 0 12px"><button data-a="from">▶ Start here</button><button data-a="dup">Double</button><button data-a="del">Delete</button></div>`;
    }
    const drop = h("div"); box.appendChild(drop);
    if (!this.dp || !this.dp.el.isConnected || this.dp.el.parentNode !== drop) {
      view.panels = view.panels.filter((p) => p !== this.dp && p !== this.bp);
      this.dp = addPanel(drop, new Panel({ t: "p", pg: PG.TRACK, a: 2 }, "Drop"));
      this.bp = addPanel(drop, new Panel({ t: "p", pg: PG.TRACK, a: 4 }, "Build"));
      if (this.dpKs) this.dp.show(this.dpKs); else this.dp.read();
      if (this.bpKs) this.bp.show(this.bpKs); else this.bp.read();
      this.dp.opt.onShow = (ks) => { this.dpKs = ks; }; this.bp.opt.onShow = (ks) => { this.bpKs = ks; };
    }
    box.querySelectorAll(".ctl button").forEach((b) => { b.onclick = (e) => this.change(b.dataset.k, +b.dataset.d * (e.shiftKey ? 5 : 1)); });
    box.querySelectorAll("button[data-a]").forEach((b) => {
      b.onclick = () => {
        if (b.dataset.a === "from") { t.from = tv.sel; this.draw(); trkSave(); }
        else if (b.dataset.a === "dup") trkEdit((items) => { items.splice(tv.sel + 1, 0, { ...items[tv.sel] }); tv.sel++; });
        else trkEdit((items) => { items.splice(tv.sel, 1); tv.sel = Math.min(tv.sel, items.length - 1); });
      };
    });
  },
  change(k, d) {
    trkEdit((items) => {
      const x = items[tv.sel];
      if (!x) return false;
      if (k === "pat") x.it = clamp(x.it + d, 1, 64);
      else if (k === "rep") x.rep = clamp(x.rep + d, 1, 16);
      else if (k === "tk") x.tk = clamp(x.tk + d, 0, 20);
      else if (k === "tb") x.tb = clamp(x.tb + d, 0, Math.max(0, S.nbrk));
      else if (k === "st") x.tb = (x.tb & ~1) | (d > 0 ? 1 : 0);
      else if (k === "gk") x.tb = (x.tb & 1) | clamp((x.tb >> 1 & 3) + d, 0, 3) << 1;
      else if (k === "fill") { x.it = 1; x.rep = 2; }
    });
  },
});

/* ================================================================== FX: the chain as a rack */
defView({ id: "fx", title: "FX", icon: "fx", every: 5000,
  render(root) {
    vh(root, "FX", "The break's effects chain, slot by slot as on the FM-1's FX page: ◀ ▶ picks the effect, On bypasses it, its tabs are its parts.");
    const rack = h("div", "rack");
    root.appendChild(rack);
    for (let i = 0; i < 4; i++) {
      const dev = h("div", "dev", `<div class="slot">SLOT ${i + 1}</div>`);
      addPanel(dev, new Panel({ t: "p", pg: PG.FX, a: i, b: 0 }, "", { fx: true, label: "Slot " + (i + 1) }));
      rack.appendChild(dev);
    }
    root.appendChild(h("div", "st", "SENDS · SETS"));
    const g = h("div", "grid3");
    root.appendChild(g);
    const c1 = h("div", "card", "<h2>Sends</h2>"), c2 = h("div", "card", "<h2>FX set</h2>"), c3 = h("div", "card", "<h2>Slice effects</h2>");
    g.append(c1, c2, c3);
    addPanel(c1, new Panel({ t: "p", pg: PG.MIX, a: 1 }, ""));
    addPanel(c2, new Panel({ t: "p", pg: PG.GLOBAL, a: 4 }, ""));
    addPanel(c3, new Panel({ t: "p", pg: PG.GLOBAL, a: 3 }, ""));
  },
});

/* ================================================================== MIX */
defView({ id: "mix", title: "Mix", icon: "mix", every: 4000,
  render(root) {
    vh(root, "Mix", "The levels as faders (drag them, or the wheel), the sends, the master and its dynamics.");
    const row = h("div", "row");
    row.style.alignItems = "flex-start";
    root.appendChild(row);
    const strips = h("div", "strips");
    row.appendChild(strips);
    const lv = new Panel({ t: "p", pg: PG.MIX, a: 0 }, "Levels");
    view.panels.push(lv);
    const C = ["var(--b)", "var(--k)", "var(--s)"];
    this.f = ["BREAK", "KICK", "DRUMS"].map((n, i) => {
      const s = h("div", "strip", `<b style="color:${C[i]}">${n}</b><div class="fader" style="--lc:${C[i]}" tabindex="0" role="slider" aria-label="${n} level"><i></i><em></em></div><span class="v">—</span>`);
      const f = s.querySelector(".fader");
      let y0 = null, acc = 0;
      f.addEventListener("pointerdown", (e) => { y0 = e.clientY; acc = 0; f.setPointerCapture(e.pointerId); });
      f.addEventListener("pointermove", (e) => { if (y0 === null) return; acc += (y0 - e.clientY) / 3.6; y0 = e.clientY; const n = Math.trunc(acc); if (n) { acc -= n; lv.turn(i, n); } });
      f.addEventListener("pointerup", () => { y0 = null; });
      f.addEventListener("wheel", (e) => { e.preventDefault(); lv.turn(i, e.deltaY < 0 ? 1 : -1); }, { passive: false });
      f.addEventListener("keydown", (e) => { if (e.key === "ArrowUp") lv.turn(i, 1); else if (e.key === "ArrowDown") lv.turn(i, -1); else return; e.preventDefault(); });
      strips.appendChild(s);
      return s;
    });
    lv.opt.onShow = (ks) => this.f.forEach((s, i) => {
      const k = ks[i], v = k.bar || 0;
      s.querySelector(".fader i").style.height = v * 100 + "%";
      s.querySelector(".fader em").style.bottom = v * 100 + "%";
      s.querySelector(".v").textContent = k.text;
      s.querySelector(".fader").setAttribute("aria-valuetext", k.text);
    });
    const g = h("div", "grid3");
    g.style.flex = "1"; g.style.minWidth = "320px";
    row.appendChild(g);
    const add = (title, a) => { const c = h("div", "card", `<h2>${title}</h2>`); addPanel(c, new Panel({ t: "p", pg: PG.MIX, a }, "", { big: true })); g.appendChild(c); };
    add("Sends", 1); add("Master", 2); add("Dynamics", 3);
    const c = h("div", "card", "<h2>Home</h2>"); addPanel(c, new Panel({ t: "p", pg: PG.HOME }, "", { big: true })); g.appendChild(c);
  },
});

/* ================================================================== ARP and the black keys */
defView({ id: "arp", title: "ARP · Keys", icon: "arp", every: 6000,
  render(root) {
    vh(root, "ARP · Black keys", "The ARP key's effects, and what each of the eleven black keys does: click a key, then turn its function.");
    const c = h("div", "card", "<h2>ARP effects</h2>");
    addPanel(c, new Panel({ t: "p", pg: PG.OVL, a: 0 }, "", { big: true }));
    root.appendChild(c);
    const k = h("div", "card", `<h2>Black keys</h2><div class="bkeys" id="bk"></div><div id="bkP" style="margin-top:12px;max-width:620px"></div>`);
    k.style.marginTop = "12px";
    root.appendChild(k);
    this.key = this.key || 0;
    for (let i = 0; i < 11; i++) {
      const b = h("button", "", `<span>${i + 1}</span><small>…</small>`);
      b.onclick = () => { this.key = i; this.pick(); };
      $("bk").appendChild(b);
    }
    this.pick();
  },
  pick() {
    document.querySelectorAll("#bk button").forEach((b, i) => b.setAttribute("aria-pressed", String(i === this.key)));
    const box = $("bkP");
    view.panels = view.panels.filter((p) => p !== this.kp);
    box.innerHTML = "";
    this.kp = addPanel(box, new Panel({ t: "p", pg: PG.OVL, a: 1, b: this.key }, "Key " + (this.key + 1), { big: true, onTurn: () => setTimeout(() => this.keys(), 300) }));
    this.kp.read();
  },
  async refresh() { await this.keys(); },
  async keys() {
    for (let i = 0; i < 11; i++) {
      try {
        const r = await knobReq({ t: "p", pg: PG.OVL, a: 1, b: i }, 0, 0);
        const b = document.querySelectorAll("#bk button")[i];
        if (b) b.querySelector("small").textContent = r.ks[2].text;
        if (i === 0) this.layout = r.ks[0].text;
      } catch (e) { return; }
    }
  },
});

/* ================================================================== SYSTEM */
defView({ id: "sys", title: "System", icon: "sys", every: 6000,
  render(root) {
    vh(root, "System", "The FM-1's settings: setup, MIDI, HOME's settings, the lights, the generator, the groove, the break's own. RESET stays on the FM-1.");
    const g = h("div", "grid3");
    root.appendChild(g);
    const add = (title, addr) => { const c = h("div", "card", `<h2>${title}</h2>`); addPanel(c, new Panel(addr, "")); g.appendChild(c); };
    add("Setup", { t: "p", pg: PG.GLOBAL, a: 2 });
    add("MIDI", { t: "p", pg: PG.GLOBAL, a: 0 });
    add("Home settings", { t: "p", pg: PG.HSET });
    add("Lights", { t: "p", pg: PG.GLOBAL, a: 7 });
    add("Groove", { t: "p", pg: PG.GLOBAL, a: 6 });
    add("Break", { t: "p", pg: PG.GLOBAL, a: 1 });
    add("Generator", { t: "p", pg: PG.GEN });
    const c = h("div", "card", `<h2>Save</h2><p class="hint" style="margin:0 0 10px">Every setting, the kicks, the patterns and the tracks into the FM-1's memory, as its SAVE.</p><button class="go" id="sysSave">Save now</button>`);
    g.appendChild(c);
    $("sysSave").onclick = () => act(3, 0);
  },
});

/* ================================================================== connecting, the keys */
async function readBreaks() {
  const r = await ask(13, [8, 4, 0, 0]), d = r.data;
  breaks = Array.from({ length: d[1] }, (_, i) => str(d, 2 + 12 * i, 12));
  $("brkSel").innerHTML = breaks.map((n, i) => `<option value="${i}">${esc(n)}</option>`).join("");
}
$("connect").onclick = async () => {
  try { midi = midi || await navigator.requestMIDIAccess({ sysex: true }); }
  catch (e) { toast("No MIDI with SysEx here: use Chrome or Edge, and allow it."); return; }
  $("connect").textContent = "Looking…";
  ready = false;                             /* (0.9.65: no status asked while it looks: one request at a time) */
  await chain;
  midi.inputs.forEach((i) => { i.onmidimessage = onMsg; });
  const outs = [...midi.outputs.values()].filter((o) => o.state !== "disconnected");
  const likely = outs.filter((o) => /loaf|choppah|fm-?1/i.test(o.name));
  out = null;
  for (const o of likely.concat(outs.filter((o) => !likely.includes(o)))) {
    try { const r = await ask1(7, [], 800, o); if (r.data[0] >= 1) { out = o; info = readInfo(r.data); break; } } catch (e) { /* not it */ }
  }
  if (out) {                                 /* (0.9.43 or later: ED_X answers) */
    try { await ask1(13, [1, 0, 0, 0, 0, 0], 800); } catch (e) { out = null; toast("This LOAF BREAKER is too old for the Studio: update it (the site's installer) to 0.9.43 or later."); }
  }
  $("connect").textContent = out ? "Refresh" : "Connect";
  if (!out) { $("dot").className = "dot bad"; if (!$("toast").textContent.includes("old")) toast("No LOAF BREAKER found. Plug it in by USB, update it if needed, and connect again."); return; }
  $("dot").className = "dot on"; lost = false; ready = true;
  ["play", "brkPrev", "brkNext", "brkSel", "wstyle", "save"].forEach((id) => { $(id).disabled = false; });
  await readBreaks().catch(() => {});
  await poll();
  let v = "live"; try { v = localStorage.getItem("lb.view") || "live"; } catch (e) { /* no storage */ }
  openView(v);
};
document.addEventListener("keydown", (e) => {
  if (e.target.closest && e.target.closest("input, select, textarea")) return;
  if (e.altKey && /^[0-9]$/.test(e.key)) { const i = (+e.key + 9) % 10; if (VIEWS[i]) { openView(VIEWS[i].id); e.preventDefault(); } return; }
  if (e.altKey && /^Digit[0-9]$/.test(e.code)) { const i = (+e.code.slice(5) + 9) % 10; if (VIEWS[i]) { openView(VIEWS[i].id); e.preventDefault(); } return; }
  if (!out) return;
  if ((e.ctrlKey || e.metaKey) && e.key.toLowerCase() === "s") { e.preventDefault(); act(3, 0); return; }
  if (e.ctrlKey || e.metaKey || e.altKey) return;
  if (e.key === " " && !(e.target.closest && e.target.closest("button, [role=slider], [role=grid]"))) { e.preventDefault(); transport(3); }
  else if (e.key === "h" || e.key === "H") act(2, 0);
  else if (["1", "2", "3"].includes(e.key) && !(e.target.closest && e.target.closest("[role=grid]"))) act(2, +e.key);
});
window.addEventListener("resize", () => { if (view && view.redrawWaves) view.redrawWaves(); if (view && view.id === "trk") view.draw(); });
buildNav();
(() => {                                     /* the design, as left */
  let t = ""; try { t = localStorage.getItem("lb.theme") || ""; } catch (e) { /* no storage */ }
  const set = (v) => {
    if (v) document.body.dataset.theme = v; else delete document.body.dataset.theme;
    accent = getComputedStyle(document.body).getPropertyValue("--amber").trim() || "#ff8470";
    try { localStorage.setItem("lb.theme", v); } catch (e) { /* no storage */ }
  };
  $("theme").value = t; set(t);
  $("theme").onchange = () => { set($("theme").value); if (view && view.redrawWaves) view.redrawWaves(); };
})();
openView("live");
if (!navigator.requestMIDIAccess) toast("This browser has no Web MIDI: use Chrome or Edge on a computer.");
