const Camera = {
    current: null,

    renderStream(cam) {
        return `<div class="stream-container">
            <img src="http://${cam.ip}:81/stream" alt="Stream" onerror="this.parentElement.innerHTML='<div style=\\'display:flex;align-items:center;justify-content:center;height:100%;color:#888\\'>Stream no disponible</div>'">
            <div class="badge">${cam.name}</div>
        </div>`;
    },

    renderStats(cam) {
        return `<div class="stats-grid" id="camStats">
            <div class="stat-card"><div class="label">${t('wifi')}</div><div class="value" id="statWifi">...</div></div>
            <div class="stat-card"><div class="label">${t('battery')}</div><div class="value" id="statBat">...</div></div>
            <div class="stat-card"><div class="label">${t('camera_name')}</div><div class="value" id="statName">${cam.name}</div></div>
            <div class="stat-card"><div class="label">IP</div><div class="value">${cam.ip}</div></div>
        </div>`;
    },

    renderControls(cam) {
        return `<div style="margin-top:12px">
            <div class="btn-row">
                <button class="btn btn-green" onclick="Camera.takePhoto('${cam.id}')">${t('photo')}</button>
                <button class="btn btn-dark" onclick="Camera.toggleRecord('${cam.id}')">${t('record')}</button>
            </div>
        </div>`;
    },

    renderFull(cam) {
        this.current = cam;
        return `<div>
            ${this.renderStream(cam)}
            ${this.renderStats(cam)}
            ${this.renderControls(cam)}
        </div>`;
    },

    async loadStatus(cam) {
        try {
            const r = await fetch(`http://${cam.ip}/api/camera/status`);
            if (r.ok) {
                const d = await r.json();
                const el = document.getElementById('camStats');
                if (el) {
                    document.getElementById('statWifi').textContent = d.wifi_signal ? d.wifi_signal + 'dB' : 'N/A';
                    document.getElementById('statBat').textContent = d.battery != null ? d.battery + '%' : 'N/A';
                }
            }
        } catch(e) {}
    },

    async takePhoto(id) {
        const cam = Discovery.getCamera(id);
        if (!cam) return;
        try {
            const r = await fetch(`http://${cam.ip}/api/capture`);
            if (r.ok) {
                const blob = await r.blob();
                const url = URL.createObjectURL(blob);
                const a = document.createElement('a');
                a.href = url; a.download = `foto_${cam.name}_${Date.now()}.jpg`; a.click();
                URL.revokeObjectURL(url);
            }
        } catch(e) { alert(t('error')); }
    },

    async toggleRecord(id) {
        const cam = Discovery.getCamera(id);
        if (!cam) return;
        try { await fetch(`http://${cam.ip}/api/record/toggle`, { method: 'POST' }); } catch(e) {}
    }
};
