const CACHE = 'camaraespia-v1';
const ASSETS = ['/', '/index.html', '/manifest.json', '/css/style.css', '/js/app.js', '/js/i18n.js', '/js/discovery.js', '/js/camera.js', '/js/settings.js'];

self.addEventListener('install', e => {
    e.waitUntil(caches.open(CACHE).then(c => c.addAll(ASSETS)).then(() => self.skipWaiting()));
});

self.addEventListener('activate', e => {
    e.waitUntil(
        caches.keys().then(keys => Promise.all(keys.filter(k => k !== CACHE).map(k => caches.delete(k)))).then(() => self.clients.claim())
    );
});

self.addEventListener('fetch', e => {
    if (e.request.url.includes('/api/')) return;
    e.respondWith(caches.match(e.request).then(r => r || fetch(e.request)));
});
