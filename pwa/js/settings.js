const Settings = {
    get() {
        return {
            theme: localStorage.getItem('theme') || 'dark',
            lang: getLang()
        };
    },

    setTheme(theme) {
        localStorage.setItem('theme', theme);
        this.applyTheme(theme);
    },

    applyTheme(theme) {
        if (theme === 'auto') {
            const dark = window.matchMedia('(prefers-color-scheme: dark)').matches;
            document.documentElement.setAttribute('data-theme', dark ? 'dark' : 'light');
        } else {
            document.documentElement.setAttribute('data-theme', theme);
        }
    },

    render() {
        const s = this.get();
        const langs = getAllLangs();
        const langOpts = langs.map(l =>
            `<option value="${l.id}" ${l.id === s.lang ? 'selected' : ''}>${l.flag} ${l.name}</option>`
        ).join('');

        return `<div class="card">
            <h3>${t('language')}</h3>
            <div class="field">
                <select onchange="Settings.changeLang(this.value)">${langOpts}</select>
            </div>
        </div>
        <div class="card">
            <h3>${t('theme')}</h3>
            <div class="btn-row">
                <button class="btn ${s.theme === 'dark' ? 'btn-green' : 'btn-dark'}" onclick="Settings.setTheme('dark'); App.renderSettings()">🌙 ${t('dark')}</button>
                <button class="btn ${s.theme === 'light' ? 'btn-green' : 'btn-dark'}" onclick="Settings.setTheme('light'); App.renderSettings()">☀️ ${t('light')}</button>
                <button class="btn ${s.theme === 'auto' ? 'btn-green' : 'btn-dark'}" onclick="Settings.setTheme('auto'); App.renderSettings()">🔄 ${t('auto')}</button>
            </div>
        </div>
        <div class="card">
            <h3>${t('add_camera')}</h3>
            <p style="margin-bottom:12px">IP: <strong>192.168.x.x</strong></p>
            <button class="btn btn-green" onclick="goTo('/add')">${t('scan')}</button>
        </div>`;
    },

    changeLang(l) {
        setLang(l);
        App.renderAll();
    }
};
