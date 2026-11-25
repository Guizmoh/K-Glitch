/* LOAF BREAKER (0.9.75, FLEXOR: "un onglet secret sur le site pour facilement installer ... les firmware de test ou les
   firmwares perso du git"): the ATELIER, the installer (fm1-base's install/, restyled) with any LOAF BREAKER package
   instead of the site's own. tools/site_style.py `atelier` copies install/index.html to atelier/ and puts this inside
   its module, before the Install button's handler: it sees the installer's image, meta, writing, say, $, productOf,
   logicalImage, checkUfw. A .fwsc, or a zip holding one (a GitHub Actions artifact, the site's zip), dropped or
   chosen; only LOAF BREAKER's identity (meta.product) is taken, so the resources and the confetti stay right. */
const AT_REPO = "https://github.com/FLEXOR-R/loaf-breaker";
const atSay = (txt, bad) => { const el = $("atstate"); el.textContent = txt; el.classList.toggle("bad", !!bad); };

/* a zip's entry ending in .fwsc (stored or deflated: what GitHub's artifacts and release zips use) */
async function atFromZip(buf) {
  const v = new DataView(buf.buffer, buf.byteOffset, buf.byteLength);
  let e = buf.length - 22;
  while (e >= 0 && v.getUint32(e, true) !== 0x06054b50) e--;
  if (e < 0) throw new Error("zip illisible");
  let p = v.getUint32(e + 16, true);
  const n = v.getUint16(e + 10, true);
  for (let i = 0; i < n; i++) {
    if (v.getUint32(p, true) !== 0x02014b50) break;
    const method = v.getUint16(p + 10, true), csize = v.getUint32(p + 20, true);
    const nl = v.getUint16(p + 28, true), xl = v.getUint16(p + 30, true), cl = v.getUint16(p + 32, true);
    const off = v.getUint32(p + 42, true);
    const name = new TextDecoder().decode(buf.subarray(p + 46, p + 46 + nl));
    p += 46 + nl + xl + cl;
    if (!/\.fwsc$/i.test(name) || /m-bootloader|uboot/i.test(name)) continue;
    const start = off + 30 + v.getUint16(off + 26, true) + v.getUint16(off + 28, true);
    const data = buf.subarray(start, start + csize);
    if (method === 0) return { name: name.split("/").pop(), raw: data };
    if (method !== 8) throw new Error(`${name}: compression ${method}`);
    const out = new Response(new Blob([data]).stream().pipeThrough(new DecompressionStream("deflate-raw")));
    return { name: name.split("/").pop(), raw: new Uint8Array(await out.arrayBuffer()) };
  }
  throw new Error("aucun .fwsc dans ce zip");
}

async function atTake(file) {
  if (!file || writing) return;
  image = null;
  $("go").disabled = true;
  try {
    let name = file.name, raw = new Uint8Array(await file.arrayBuffer());
    if (raw[0] === 0x50 && raw[1] === 0x4b) ({ name, raw } = await atFromZip(raw));
    const product = productOf(raw);
    if (product !== meta.product) {
      atSay(`${name} : ce n'est pas un LOAF BREAKER (${product || "pas un paquet FM-1"}). Ici, seulement les LOAF BREAKER ` +
            `(${meta.product}) ; le firmware d'origine se remet sur la page Install.`, true);
      return;
    }
    const img = logicalImage(raw);
    try { checkUfw(img); } catch (_) { atSay(`${name} : paquet abîmé, ne l'installe pas.`, true); return; }
    image = img;
    const v = (name.match(/loaf-breaker-(.+)\.fwsc$/i) || [])[1];
    atSay(`Prêt : ${name}${v ? ` (version ${v})` : ""}, ${Math.round(raw.length / 1024)} Ko. Appuie sur Install.`);
    say(`LOAF BREAKER ${v || name}`);
    $("go").disabled = false;
  } catch (e) { atSay(`${file.name} : ${e && e.message || e}`, true); }
}
$("atfile").addEventListener("change", (e) => atTake(e.target.files && e.target.files[0]));
const atDrop = $("atdrop");
["dragenter", "dragover"].forEach((k) => atDrop.addEventListener(k, (e) => { e.preventDefault(); atDrop.classList.add("over"); }));
["dragleave", "drop"].forEach((k) => atDrop.addEventListener(k, () => atDrop.classList.remove("over")));
atDrop.addEventListener("drop", (e) => { e.preventDefault(); atTake(e.dataTransfer.files && e.dataTransfer.files[0]); });
document.querySelectorAll("a[data-at]").forEach((a) => { a.href = AT_REPO + a.dataset.at; });
