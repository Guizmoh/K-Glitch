// SPDX-License-Identifier: MPL-2.0
// LOAF BREAKER's app (0.9.71): the site's service worker, at its root. The network first, always (a new firmware's
// pages the moment they are up); the copy kept of each page and its files only when the network fails, so the app
// opens offline on what it last saw. Never the firmware's package or the break library's audio (big, and the
// installer must have the current one).
const CACHE = "loaf-app-1";
self.addEventListener("install", () => self.skipWaiting());
self.addEventListener("activate", (e) => e.waitUntil(
  caches.keys().then((ks) => Promise.all(ks.filter((k) => k !== CACHE).map((k) => caches.delete(k)))).then(() => self.clients.claim())));
self.addEventListener("fetch", (e) => {
  const r = e.request, u = new URL(r.url);
  if (r.method !== "GET" || u.origin !== location.origin || /\/(firmware|breaks\/lib)\//.test(u.pathname)) return;
  e.respondWith(fetch(r).then((res) => {
    if (res.ok) { const c = res.clone(); caches.open(CACHE).then((k) => k.put(r, c)).catch(() => {}); }
    return res;
  }).catch(() => caches.match(r).then((m) => m || Response.error())));
});
