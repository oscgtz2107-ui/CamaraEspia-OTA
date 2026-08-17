const Discovery = {
    cameras: JSON.parse(localStorage.getItem('cameras') || '[]'),
    _scanning: false,

    save() { localStorage.setItem('cameras', JSON.stringify(this.cameras)); },

    async addCamera(ip, password) {
        try {
            const info = await this._fetchInfo(ip, password);
            if (info && info.name) {
                const cam = { id: Date.now().toString(), ip, name: info.name, password: password || '', online: true, lastSeen: Date.now() };
                const exist = this.cameras.find(c => c.ip === ip);
                if (exist) { Object.assign(exist, cam); }
                else { this.cameras.push(cam); }
                this.save();
                return cam;
            }
        } catch(e) { console.error('addCamera error:', e); }
        return null;
    },

    removeCamera(id) {
        this.cameras = this.cameras.filter(c => c.id !== id);
        this.save();
    },

    getCamera(id) { return this.cameras.find(c => c.id === id); },
    getCameraByIP(ip) { return this.cameras.find(c => c.ip === ip); },

    async scanLAN(onProgress) {
        if (this._scanning) return;
        this._scanning = true;
        const found = [];
        const ips = this._generateIPs();
        const batchSize = 20;

        for (let i = 0; i < ips.length; i += batchSize) {
            const batch = ips.slice(i, i + batchSize);
            const results = await Promise.allSettled(batch.map(ip => this._pingCamera(ip)));
            results.forEach((r, idx) => {
                if (r.status === 'fulfilled' && r.value) {
                    found.push({ ip: batch[idx], ...r.value });
                    if (onProgress) onProgress(batch[idx], true);
                }
            });
            if (onProgress) onProgress(null, false, found.length);
        }

        found.forEach(f => {
            const exist = this.cameras.find(c => c.ip === f.ip);
            if (exist) { exist.online = true; exist.name = f.name; exist.lastSeen = Date.now(); }
            else { this.cameras.push({ id: Date.now().toString() + Math.random(), ip: f.ip, name: f.name, password: '', online: true, lastSeen: Date.now() }); }
        });

        this.cameras.forEach(c => {
            if (!found.find(f => f.ip === c.ip)) c.online = false;
        });

        this.save();
        this._scanning = false;
        return found;
    },

    _generateIPs() {
        const ips = [];
        const base = this.cameras.length > 0 ? this.cameras[0].ip.split('.').slice(0, 3).join('.') : '192.168.1';
        for (let i = 1; i <= 254; i++) ips.push(base + '.' + i);
        return ips;
    },

    async _pingCamera(ip) {
        const ctrl = new AbortController();
        const timer = setTimeout(() => ctrl.abort(), 1500);
        try {
            const r = await fetch('http://' + ip + '/api/camera/info', { signal: ctrl.signal });
            clearTimeout(timer);
            if (r.ok) {
                const d = await r.json();
                if (d.firmware && d.firmware.includes('CamaraEspia')) return d;
            }
        } catch(e) { clearTimeout(timer); }
        return null;
    },

    async _fetchInfo(ip, password) {
        const ctrl = new AbortController();
        const timer = setTimeout(() => ctrl.abort(), 2000);
        try {
            const r = await fetch('http://' + ip + '/api/camera/info', { signal: ctrl.signal });
            clearTimeout(timer);
            if (r.ok) return await r.json();
        } catch(e) { clearTimeout(timer); }
        return null;
    }
};
