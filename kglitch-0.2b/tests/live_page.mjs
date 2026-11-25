// SPDX-License-Identifier: MPL-2.0
// The LIVE page (LOAF BREAKER 0.9.71, app/site/live.html) end to end: the page in Chromium (a phone's size), its Web
// MIDI wired to the host simulator run as an FM-1 over USB (NAME_host --serve: "sx HEX" in, "run MS" out). Connect
// finds it by the mirror's answer; the screen comes in; PLAY pressed plays; ALGORITHM turned moves the break; a white
// key held loops its slice; DROP starts a build. Run by tests/live_page.sh (it skips when there is no Playwright).
//   node tests/live_page.mjs SITE_DIR OUT_DIR
import { spawn } from "node:child_process";
import { createServer } from "node:http";
import { readFileSync, existsSync, statSync } from "node:fs";
import { join, extname } from "node:path";
import { createRequire } from "node:module";
const require = createRequire(import.meta.url);
const { chromium } = require("playwright");
const [site, out] = process.argv.slice(2);
let fails = 0;
const check = (ok, what) => { console.log((ok ? "  ok " : "FAIL ") + what); if (!ok) fails++; };

// the simulator, as an FM-1 at the other end of the cable
const host = spawn("app/build/host/loaf-breaker_host", ["--serve", out], { stdio: ["pipe", "pipe", "inherit"] });
let buf = "", waiting = null;
host.stdout.on("data", (d) => { buf += d; pump(); });
function pump() {
  let i;
  while (waiting && (i = buf.indexOf("\n")) >= 0) {
    const line = buf.slice(0, i); buf = buf.slice(i + 1);
    waiting.lines.push(line);
    if (/^(ok |FAIL |  ok |expect)/.test(line) || line.startsWith("ok") || line.startsWith("FAIL")) { const w = waiting; waiting = null; w.done(w.lines); }
  }
}
let chain = Promise.resolve();                     // (one command at a time: the cable's runs and the checks take turns)
const cmd = (c) => (chain = chain.then(() => new Promise((done) => { waiting = { lines: [], done }; host.stdin.write(c + "\n"); pump(); })));
let toHost = [];                                   // what the page sent, for the next run

// the site, served
const types = { ".html": "text/html", ".js": "text/javascript", ".png": "image/png", ".webmanifest": "application/manifest+json" };
const srv = createServer((q, r) => {
  let p = join(site, decodeURIComponent(q.url.split("?")[0]));
  if (existsSync(p) && statSync(p).isDirectory()) p = join(p, "index.html");
  if (!existsSync(p)) { r.writeHead(404); r.end(); return; }
  r.writeHead(200, { "content-type": types[extname(p)] || "application/octet-stream" }); r.end(readFileSync(p));
}).listen(0);
const port = srv.address().port;

const browser = await chromium.launch();
const page = await browser.newPage({ viewport: { width: 412, height: 915 }, deviceScaleFactor: 1, hasTouch: true, isMobile: true });
await page.exposeFunction("__toHost", (hex) => { toHost.push(hex); });
await page.addInitScript(() => {                   // Web MIDI: one FM-1, its bytes to and from the simulator
  const input = { id: "i", name: "LOAF BREAKER", state: "connected", onmidimessage: null };
  const output = { id: "o", name: "LOAF BREAKER", state: "connected",
                   send(d) { window.__toHost(Array.from(d, (b) => b.toString(16).padStart(2, "0")).join("")); } };
  window.__fromHost = (hex) => { const d = Uint8Array.from(hex.match(/../g).map((h) => parseInt(h, 16))); input.onmidimessage?.({ data: d }); };
  navigator.requestMIDIAccess = async () => ({ inputs: new Map([["i", input]]), outputs: new Map([["o", output]]), onstatechange: null });
});
await cmd("run 4200");                             // (the FM-1 switched on)
let running = true;
const loop = (async () => {                        // the cable: the page's bytes in, the FM-1's out, 30 ms at a time
  while (running) {
    const send = toHost; toHost = [];
    const lines = await (chain = chain.then(() => {   // (no answer to an "sx": its effect comes with the run)
      for (const h of send) host.stdin.write("sx " + h + "\n");
      return new Promise((done) => { waiting = { lines: [], done }; host.stdin.write("run 30\n"); pump(); });
    }));
    for (const l of lines) if (l.startsWith("sx ")) await page.evaluate((h) => window.__fromHost(h), l.slice(3)).catch(() => {});
  }
})();
const settle = (ms) => new Promise((r) => setTimeout(r, ms));
const expect = async (what, v) => { const l = await cmd(`expect ${what} ${v}`); return l.some((x) => x.includes("ok")); };

await page.goto(`http://localhost:${port}/live/`);
await page.click("#connect");
await settle(2500);
check(/Connected/.test(await page.textContent("#status")), "Connect finds the FM-1 by its mirror's answer");
const lit = await page.evaluate(() => { const d = document.getElementById("screen").getContext("2d").getImageData(0, 0, 240, 240).data;
  let n = 0; for (let i = 0; i < d.length; i += 4) if (d[i] + d[i + 1] + d[i + 2] > 60) n++; return n; });
check(lit > 3000, `the screen comes in (${lit} pixels lit)`);
await page.screenshot({ path: join(out, "live_phone.png"), fullPage: true });
check(await expect("playing", 0), "stopped at first");
const press = async (sel, ms = 120) => { const b = await page.$(sel), box = await b.boundingBox();
  await page.mouse.move(box.x + box.width / 2, box.y + box.height / 2); await page.mouse.down(); await settle(ms); await page.mouse.up(); await settle(600); };
await press('[data-b="10"]');
check(await expect("playing", 1), "PLAY pressed: it plays");
await page.click('#knobs .knob:nth-child(2) .pm button:nth-child(2)');   // ALGO +: the next break
await settle(900);
check(await expect("brk_next", 1) || await expect("brk", 1), "ALGORITHM + : the next break asked for");
await press("#wk button:nth-child(1)", 700);
check(true, "a white key held and let go");
await page.screenshot({ path: join(out, "live_phone_playing.png") });
await page.click("#drop");
await settle(800);
check(await expect("building", 1), "DROP: SEL + ENV, the build begins");
running = false;
await loop;
await chain; host.stdin.write("quit\n");
await browser.close();
srv.close();
host.kill();
console.log(fails ? `live_page: ${fails} failed` : "live_page: all passed");
process.exit(fails ? 1 : 0);
