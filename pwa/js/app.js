const App = {
    init() {
        Settings.applyTheme(Settings.get().theme);
        if ('serviceWorker' in navigator) navigator.serviceWorker.register('/sw.js').catch(() => {});

        document.getElementById('openSidebar').onclick = () => {
            document.getElementById('sidebar').classList.add('open');
            document.getElementById('overlay').classList.add('open') || this._addOverlay();
        };
        document.getElementById('closeSidebar').onclick = () => this.closeSidebar();

        this._addOverlay();
        this.renderAll();
        this.updateSidebar();

        window.addEventListener('hashchange', () => this.route());
        this.route();
    },

    _addOverlay() {
        if (!document.getElementById('overlay')) {
            const ov = document.createElement('div');
            ov.id = 'overlay';
            ov.className = 'overlay';
            ov.onclick = () => this.closeSidebar();
            document.getElementById('app').appendChild(ov);
        }
    },

    closeSidebar() {
        document.getElementById('sidebar').classList.remove('open');
        const ov = document.getElementById('overlay');
        if (ov) ov.classList.remove('open');
    },

    renderAll() {
        this.renderSettings();
        this.updateSidebar();
        this.route();
    },

    renderSettings() {
        document.getElementById('view-settings').innerHTML = Settings.render();
    },

    updateSidebar() {
        const list = document.getElementById('cameraList');
        const empty = document.getElementById('emptyMsg');
        if (Discovery.cameras.length === 0) {
            list.innerHTML = '';
            list.appendChild(empty);
            empty.style.display = 'block';
            return;
        }
        empty.style.display = 'none';
        list.innerHTML = Discovery.cameras.map(c => `
            <div class="camera-item ${c.id === (Camera.current && Camera.current.id) ? 'active' : ''}"
                 onclick="App.openCamera('${c.id}')">
                <div class="dot ${c.online ? 'online' : 'offline'}"></div>
                <div class="info">
                    <div class="name">${c.name}</div>
                    <div class="ip">${c.ip}</div>
                </div>
            </div>
        `).join('');
    },

    openCamera(id) {
        const cam = Discovery.getCamera(id);
        if (!cam) return;
        this.closeSidebar();
        window.location.hash = '#/camera/' + id;
    },

    route() {
        const hash = window.location.hash || '#/';
        const parts = hash.replace('#/', '').split('/');
        const views = document.querySelectorAll('.view');
        views.forEach(v => v.classList.remove('active'));

        if (parts[0] === 'camera' && parts[1]) {
            const cam = Discovery.getCamera(parts[1]);
            if (cam) {
                document.getElementById('view-camera').innerHTML = Camera.renderFull(cam);
                document.getElementById('view-camera').classList.add('active');
                document.getElementById('pageTitle').textContent = cam.name;
                Camera.loadStatus(cam);
                this.updateSidebar();
                return;
            }
        }

        if (parts[0] === 'settings') {
            document.getElementById('view-settings').innerHTML = Settings.render();
            document.getElementById('view-settings').classList.add('active');
            document.getElementById('pageTitle').textContent = t('settings');
            return;
        }

        if (parts[0] === 'add') {
            document.getElementById('view-add').innerHTML = this.renderAddCamera();
            document.getElementById('view-add').classList.add('active');
            document.getElementById('pageTitle').textContent = t('add_camera');
            return;
        }

        document.getElementById('view-home').innerHTML = this.renderHome();
        document.getElementById('view-home').classList.add('active');
        document.getElementById('pageTitle').textContent = 'CamaraEspia';
    },

    renderHome() {
        if (Discovery.cameras.length === 0) {
            return `<div style="text-align:center;padding:60px 20px">
                <div style="font-size:48px;margin-bottom:16px">📷</div>
                <h2 style="margin-bottom:8px">${t('welcome')}</h2>
                <p style="color:var(--text2);margin-bottom:24px">${t('no_cameras')}</p>
                <button class="btn btn-green" onclick="goTo('/add')">${t('add_camera')}</button>
            </div>`;
        }
        return `<div>
            <div class="btn-row" style="margin-bottom:16px">
                <button class="btn btn-green" onclick="App.scanLAN()">${t('scan')}</button>
            </div>
            ${Discovery.cameras.map(c => `
                <div class="card" onclick="App.openCamera('${c.id}')" style="cursor:pointer">
                    <div style="display:flex;align-items:center;gap:12px">
                        <div class="dot ${c.online ? 'online' : 'offline'}"></div>
                        <div><h3>${c.name}</h3><p>${c.ip}</p></div>
                    </div>
                </div>
            `).join('')}
        </div>`;
    },

    renderAddCamera() {
        return `<div class="card">
            <h3>${t('scan')}</h3>
            <p style="margin-bottom:12px">${t('scanning')}</p>
            <button class="btn btn-green" onclick="App.scanLAN()" id="scanBtn">${t('scan')}</button>
            <div id="scanResults" style="margin-top:12px"></div>
        </div>
        <div class="card">
            <h3>${t('add_camera')} IP manual</h3>
            <div class="field">
                <input type="text" id="manualIP" placeholder="192.168.1.100">
            </div>
            <div class="field">
                <input type="password" id="manualPass" placeholder="${t('password')}">
            </div>
            <button class="btn btn-green" onclick="App.addManual()">${t('save')}</button>
        </div>`;
    },

    async scanLAN() {
        const btn = document.getElementById('scanBtn');
        const results = document.getElementById('scanResults');
        if (btn) { btn.disabled = true; btn.textContent = t('scanning'); }
        if (results) results.innerHTML = '';

        await Discovery.scanLAN((ip, found, count) => {
            if (found && results) {
                results.innerHTML += `<div class="card" style="cursor:pointer" onclick="App.addDiscovered('${ip}')">
                    <p>✅ ${ip} — encontrado</p>
                </div>`;
            }
        });

        if (btn) { btn.disabled = false; btn.textContent = t('scan'); }
        this.updateSidebar();
    },

    async addDiscovered(ip) {
        const cam = await Discovery.addCamera(ip, '');
        if (cam) { this.updateSidebar(); goTo('/'); }
    },

    async addManual() {
        const ip = document.getElementById('manualIP').value.trim();
        const pass = document.getElementById('manualPass').value;
        if (!ip) return;
        const cam = await Discovery.addCamera(ip, pass);
        if (cam) { this.updateSidebar(); goTo('/'); }
        else alert(t('error'));
    }
};

function goTo(path) { window.location.hash = '#' + path; }
function goHome() { window.location.hash = '#/'; }

document.addEventListener('DOMContentLoaded', () => App.init());
