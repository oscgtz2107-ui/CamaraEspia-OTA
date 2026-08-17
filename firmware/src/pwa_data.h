#ifndef PWA_DATA_H
#define PWA_DATA_H

#include <pgmspace.h>

// ============================================================
// PWA index.html
// ============================================================
const char PWA_INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="es">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1,maximum-scale=1,user-scalable=no">
<meta name="theme-color" content="#4CAF50">
<meta name="apple-mobile-web-app-capable" content="yes">
<meta name="apple-mobile-web-app-status-bar-style" content="black-translucent">
<link rel="manifest" href="/manifest.json">
<link rel="stylesheet" href="/css/style.css">
<title>CamaraEspia</title>
</head>
<body>
<div id="app">
    <nav id="sidebar" class="sidebar">
        <div class="sidebar-header">
            <h2>CamaraEspia</h2>
            <button id="closeSidebar" class="icon-btn">X</button>
        </div>
        <div id="cameraList" class="camera-list">
            <p class="empty-msg" id="emptyMsg">No hay camaras configuradas</p>
        </div>
        <div class="sidebar-footer">
            <button onclick="goTo('/settings')" class="menu-btn" data-i18n="settings">Ajustes</button>
            <button onclick="goTo('/add')" class="menu-btn" data-i18n="add_camera">Agregar camara</button>
        </div>
    </nav>

    <main id="main" class="main">
        <header id="topbar" class="topbar">
            <button id="openSidebar" class="icon-btn">&#9776;</button>
            <h1 id="pageTitle">CamaraEspia</h1>
            <div id="topActions"></div>
        </header>

        <div id="content" class="content">
            <div id="view-home" class="view active"></div>
            <div id="view-camera" class="view"></div>
            <div id="view-settings" class="view"></div>
            <div id="view-add" class="view"></div>
        </div>
    </main>
</div>

<script src="/js/i18n.js"></script>
<script src="/js/discovery.js"></script>
<script src="/js/camera.js"></script>
<script src="/js/settings.js"></script>
<script src="/js/app.js"></script>
</body>
</html>
)rawliteral";

// ============================================================
// CSS
// ============================================================
const char PWA_STYLE_CSS[] PROGMEM = R"rawliteral(
:root{--bg:#0a0a0a;--card:#1a1a1a;--border:#2a2a2a;--text:#e0e0e0;--text2:#888;--accent:#4CAF50;--accent2:#45a049;--danger:#f44336;--radius:12px}
[data-theme="light"]{--bg:#f5f5f5;--card:#fff;--border:#e0e0e0;--text:#1a1a1a;--text2:#666;--accent:#2e7d32;--accent2:#1b5e20}
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',sans-serif;background:var(--bg);color:var(--text);min-height:100vh;overflow:hidden}
#app{display:flex;height:100vh}
.sidebar{width:280px;background:var(--card);border-right:1px solid var(--border);display:flex;flex-direction:column;transform:translateX(-100%);transition:transform .25s;position:fixed;z-index:100;height:100vh}
.sidebar.open{transform:translateX(0)}
.sidebar-header{padding:20px;display:flex;justify-content:space-between;align-items:center;border-bottom:1px solid var(--border)}
.sidebar-header h2{font-size:16px}
.camera-list{flex:1;overflow-y:auto;padding:8px}
.camera-item{padding:12px 16px;border-radius:var(--radius);cursor:pointer;display:flex;align-items:center;gap:12px;margin-bottom:4px;transition:background .15s}
.camera-item:hover{background:var(--border)}
.camera-item.active{background:var(--accent);color:#fff}
.camera-item .dot{width:8px;height:8px;border-radius:50%;flex-shrink:0}
.camera-item .dot.online{background:#4CAF50}
.camera-item .dot.offline{background:#666}
.camera-item .info{flex:1;min-width:0}
.camera-item .info .name{font-size:14px;font-weight:500;white-space:nowrap;overflow:hidden;text-overflow:ellipsis}
.camera-item .info .ip{font-size:11px;color:var(--text2)}
.sidebar-footer{padding:12px;border-top:1px solid var(--border)}
.menu-btn{width:100%;padding:12px;background:none;border:none;color:var(--text);text-align:left;font-size:14px;cursor:pointer;border-radius:8px}
.menu-btn:hover{background:var(--border)}
.main{flex:1;display:flex;flex-direction:column;overflow:hidden}
.topbar{height:56px;padding:0 16px;display:flex;align-items:center;gap:12px;border-bottom:1px solid var(--border);flex-shrink:0}
.topbar h1{font-size:18px;flex:1}
.icon-btn{background:none;border:none;color:var(--text);font-size:20px;cursor:pointer;padding:8px;border-radius:8px}
.icon-btn:hover{background:var(--border)}
.content{flex:1;overflow-y:auto;padding:16px}
.view{display:none}
.view.active{display:block}
.empty-msg{color:var(--text2);text-align:center;padding:40px 16px;font-size:14px}
.card{background:var(--card);border:1px solid var(--border);border-radius:var(--radius);padding:20px;margin-bottom:12px}
.card h3{font-size:15px;margin-bottom:8px}
.card p{font-size:13px;color:var(--text2)}
.field{margin-bottom:14px}
.field label{display:block;font-size:12px;color:var(--text2);margin-bottom:6px;text-transform:uppercase;letter-spacing:.5px}
.field input,.field select{width:100%;padding:12px;border-radius:8px;border:1px solid var(--border);background:var(--bg);color:var(--text);font-size:15px}
.field input:focus,.field select:focus{outline:none;border-color:var(--accent)}
.btn{width:100%;padding:14px;border:none;border-radius:var(--radius);font-size:15px;font-weight:600;cursor:pointer;transition:all .2s}
.btn-green{background:var(--accent);color:#fff}.btn-green:hover{background:var(--accent2)}
.btn-dark{background:var(--border);color:var(--text)}.btn-dark:hover{background:#333}
.btn:disabled{opacity:.4;cursor:not-allowed}
.btn-row{display:flex;gap:8px}.btn-row .btn{flex:1}
.stream-container{width:100%;background:#000;border-radius:var(--radius);overflow:hidden;aspect-ratio:4/3;position:relative}
.stream-container img{width:100%;height:100%;object-fit:contain}
.stream-container .badge{position:absolute;top:8px;right:8px;background:rgba(0,0,0,.7);color:#fff;padding:4px 10px;border-radius:20px;font-size:11px}
.stats-grid{display:grid;grid-template-columns:repeat(2,1fr);gap:8px}
.stat-card{background:var(--card);border:1px solid var(--border);border-radius:var(--radius);padding:14px;text-align:center}
.stat-card .label{font-size:11px;color:var(--text2);text-transform:uppercase;margin-bottom:4px}
.stat-card .value{font-size:18px;font-weight:600}
.status-dot{display:inline-block;width:8px;height:8px;border-radius:50%;margin-right:6px}
.status-dot.on{background:#4CAF50}.status-dot.off{background:#f44336}
.toggle{position:relative;width:44px;height:24px;background:#333;border-radius:12px;cursor:pointer;transition:background .2s}
.toggle.on{background:var(--accent)}
.toggle::after{content:'';position:absolute;top:2px;left:2px;width:20px;height:20px;background:#fff;border-radius:50%;transition:transform .2s}
.toggle.on::after{transform:translateX(20px)}
.overlay{position:fixed;top:0;left:0;width:100%;height:100%;background:rgba(0,0,0,.5);z-index:90;display:none}
.overlay.open{display:block}
@media(min-width:768px){
    .sidebar{transform:translateX(0);position:relative}
    .overlay{display:none!important}
}
)rawliteral";

// ============================================================
// Service Worker
// ============================================================
const char PWA_SW_JS[] PROGMEM = R"rawliteral(
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
)rawliteral";

// ============================================================
// manifest.json
// ============================================================
const char PWA_MANIFEST_JSON[] PROGMEM = R"rawliteral(
{
  "name": "CamaraEspia",
  "short_name": "CamaraEspia",
  "description": "Gestiona tus camaras de seguridad ESP32",
  "start_url": "/",
  "display": "standalone",
  "orientation": "portrait",
  "background_color": "#0a0a0a",
  "theme_color": "#4CAF50",
  "icons": [
    { "src": "/icons/icon-192.png", "sizes": "192x192", "type": "image/png" },
    { "src": "/icons/icon-512.png", "sizes": "512x512", "type": "image/png" }
  ]
}
)rawliteral";

// ============================================================
// js/i18n.js
// ============================================================
const char PWA_I18N_JS[] PROGMEM = R"rawliteral(
const LANGS = {
    es:{name:'Espanol',flag:'ES',welcome:'Bienvenido a CamaraEspia',no_cameras:'No hay camaras configuradas',settings:'Ajustes',add_camera:'Agregar camara',scan:'Buscar camaras',scanning:'Buscando camaras...',connecting:'Conectando...',connected:'Conectado',offline:'Sin conexion',live:'EN VIVO',photo:'Foto',record:'Grabar',stop:'Detener',servo:'Servo',settings_title:'Ajustes',language:'Idioma',theme:'Tema',dark:'Oscuro',light:'Claro',auto:'Auto',camera_name:'Nombre',wifi:'WiFi',battery:'Bateria',password:'Contrasena',enter_password:'Ingresa la contrasena de la camara',save:'Guardar',cancel:'Cancelar',back:'Volver',no_results:'Sin resultados',error:'Error'},
    en:{name:'English',flag:'EN',welcome:'Welcome to CamaraEspia',no_cameras:'No cameras configured',settings:'Settings',add_camera:'Add camera',scan:'Scan cameras',scanning:'Scanning...',connecting:'Connecting...',connected:'Connected',offline:'Offline',live:'LIVE',photo:'Photo',record:'Record',stop:'Stop',servo:'Servo',settings_title:'Settings',language:'Language',theme:'Theme',dark:'Dark',light:'Light',auto:'Auto',camera_name:'Name',wifi:'WiFi',battery:'Battery',password:'Password',enter_password:'Enter camera password',save:'Save',cancel:'Cancel',back:'Back',no_results:'No results',error:'Error'},
    pt:{name:'Portugues',flag:'PT',welcome:'Bem-vindo ao CamaraEspia',no_cameras:'Nenhuma camera configurada',settings:'Configuracoes',add_camera:'Adicionar camera',scan:'Buscar cameras',scanning:'Procurando...',connecting:'Conectando...',connected:'Conectado',offline:'Offline',live:'AO VIVO',photo:'Foto',record:'Gravar',stop:'Parar',servo:'Servo',settings_title:'Configuracoes',language:'Idioma',theme:'Tema',dark:'Escuro',light:'Claro',auto:'Auto',camera_name:'Nome',wifi:'WiFi',battery:'Bateria',password:'Senha',enter_password:'Digite a senha da camera',save:'Salvar',cancel:'Cancelar',back:'Voltar',no_results:'Sem resultados',error:'Erro'},
    fr:{name:'Francais',flag:'FR',welcome:'Bienvenue sur CamaraEspia',no_cameras:'Aucune camera configuree',settings:'Parametres',add_camera:'Ajouter camera',scan:'Rechercher',scanning:'Recherche...',connecting:'Connexion...',connected:'Connecte',offline:'Hors ligne',live:'EN DIRECT',photo:'Photo',record:'Enregistrer',stop:'Arreter',servo:'Servo',settings_title:'Parametres',language:'Langue',theme:'Theme',dark:'Sombre',light:'Clair',auto:'Auto',camera_name:'Nom',wifi:'WiFi',battery:'Batterie',password:'Mot de passe',enter_password:'Entrez le mot de passe',save:'Enregistrer',cancel:'Annuler',back:'Retour',no_results:'Aucun resultat',error:'Erreur'},
    de:{name:'Deutsch',flag:'DE',welcome:'Willkommen bei CamaraEspia',no_cameras:'Keine Kameras konfiguriert',settings:'Einstellungen',add_camera:'Kamera hinzufuegen',scan:'Kameras suchen',scanning:'Suchen...',connecting:'Verbinden...',connected:'Verbunden',offline:'Offline',live:'LIVE',photo:'Foto',record:'Aufnehmen',stop:'Stopp',servo:'Servo',settings_title:'Einstellungen',language:'Sprache',theme:'Design',dark:'Dunkel',light:'Hell',auto:'Auto',camera_name:'Name',wifi:'WiFi',battery:'Akku',password:'Passwort',enter_password:'Kamera-Passwort eingeben',save:'Speichern',cancel:'Abbrechen',back:'Zurueck',no_results:'Keine Ergebnisse',error:'Fehler'},
    it:{name:'Italiano',flag:'IT',welcome:'Benvenuto in CamaraEspia',no_cameras:'Nessuna fotocamera configurata',settings:'Impostazioni',add_camera:'Aggiungi camera',scan:'Cerca camere',scanning:'Ricerca...',connecting:'Connessione...',connected:'Connesso',offline:'Offline',live:'DIRETTA',photo:'Foto',record:'Registra',stop:'Ferma',servo:'Servo',settings_title:'Impostazioni',language:'Lingua',theme:'Tema',dark:'Scuro',light:'Chiaro',auto:'Auto',camera_name:'Nome',wifi:'WiFi',battery:'Batteria',password:'Password',enter_password:'Inserisci password',save:'Salva',cancel:'Annulla',back:'Indietro',no_results:'Nessun risultato',error:'Errore'},
    ru:{name:'Русский',flag:'RU',welcome:'Добро пожаловать в CamaraEspia',no_cameras:'Нет настроенных камер',settings:'Настройки',add_camera:'Добавить камеру',scan:'Найти камеры',scanning:'Поиск...',connecting:'Подключение...',connected:'Подключено',offline:'Оффлайн',live:'ОНЛАЙН',photo:'Фото',record:'Запись',stop:'Стоп',servo:'Серво',settings_title:'Настройки',language:'Язык',theme:'Тема',dark:'Тёмная',light:'Светлая',auto:'Авто',camera_name:'Имя',wifi:'WiFi',battery:'Батарея',password:'Пароль',enter_password:'Введите пароль камеры',save:'Сохранить',cancel:'Отмена',back:'Назад',no_results:'Нет результатов',error:'Ошибка'},
    zh:{name:'中文',flag:'CN',welcome:'欢迎使用 CamaraEspia',no_cameras:'没有配置摄像头',settings:'设置',add_camera:'添加摄像头',scan:'搜索摄像头',scanning:'搜索中...',connecting:'连接中...',connected:'已连接',offline:'离线',live:'直播',photo:'拍照',record:'录像',stop:'停止',servo:'舵机',settings_title:'设置',language:'语言',theme:'主题',dark:'深色',light:'浅色',auto:'自动',camera_name:'名称',wifi:'WiFi',battery:'电池',password:'密码',enter_password:'输入摄像头密码',save:'保存',cancel:'取消',back:'返回',no_results:'无结果',error:'错误'},
    ja:{name:'日本語',flag:'JP',welcome:'CamaraEspiaへようこそ',no_cameras:'カメラが設定されていません',settings:'設定',add_camera:'カメラ追加',scan:'カメラ検索',scanning:'検索中...',connecting:'接続中...',connected:'接続済み',offline:'オフライン',live:'ライブ',photo:'写真',record:'録画',stop:'停止',servo:'サーボ',settings_title:'設定',language:'言語',theme:'テーマ',dark:'ダーク',light:'ライト',auto:'自動',camera_name:'名前',wifi:'WiFi',battery:'バッテリー',password:'パスワード',enter_password:'カメラパスワードを入力',save:'保存',cancel:'キャンセル',back:'戻る',no_results:'結果なし',error:'エラー'},
    ko:{name:'한국어',flag:'KR',welcome:'CamaraEspia에 오신것을 환영합니다',no_cameras:'카메라가 설정되지 않았습니다',settings:'설정',add_camera:'카메라 추가',scan:'카메라 검색',scanning:'검색중...',connecting:'연결중...',connected:'연결됨',offline:'오프라인',live:'실시간',photo:'사진',record:'녹화',stop:'중지',servo:'서보',settings_title:'설정',language:'언어',theme:'테마',dark:'다크',light:'라이트',auto:'자동',camera_name:'이름',wifi:'WiFi',battery:'배터리',password:'비밀번호',enter_password:'카메라 비밀번호 입력',save:'저장',cancel:'취소',back:'뒤로',no_results:'결과 없음',error:'오류'},
    ar:{name:'العربية',flag:'SA',welcome:'مرحبا بكم في CamaraEspia',no_cameras:'لا توجد كاميرات',settings:'الاعدادات',add_camera:'اضافة كاميرا',scan:'بحث',scanning:'بحث...',connecting:'اتصال...',connected:'متصل',offline:'غير متصل',live:'مباشر',photo:'صورة',record:'تسجيل',stop:'ايقاف',servo:'سيرفو',settings_title:'الاعدادات',language:'اللغة',theme:'السمة',dark:'داكن',light:'فاتح',auto:'تلقائي',camera_name:'الاسم',wifi:'WiFi',battery:'البطارية',password:'كلمة المرور',enter_password:'ادخل كلمة مرور الكاميرا',save:'حفظ',cancel:'الغاء',back:'رجوع',no_results:'لا نتائج',error:'خطأ'},
    hi:{name:'हिन्दी',flag:'IN',welcome:'CamaraEspia में आपका स्वागत है',no_cameras:'कोई कैमरा कॉन्फ़िगर नहीं',settings:'सेटिंग्स',add_camera:'कैमरा जोड़ें',scan:'कैमरा खोजें',scanning:'खोज रहा है...',connecting:'कनेक्ट हो रहा है...',connected:'कनेक्टेड',offline:'ऑफलाइन',live:'लाइव',photo:'फ़ोटो',record:'रिकॉर्ड',stop:'रोकें',servo:'सर्वो',settings_title:'सेटिंग्स',language:'भाषा',theme:'थीम',dark:'डार्क',light:'लाइट',auto:'ऑटो',camera_name:'नाम',wifi:'WiFi',battery:'बैटरी',password:'पासवर्ड',enter_password:'कैमरा पासवर्ड दर्ज करें',save:'सहेजें',cancel:'रद्द करें',back:'वापस',no_results:'कोई परिणाम नहीं',error:'त्रुटि'},
    bn:{name:'বাংলা',flag:'BD',welcome:'CamaraEspia-তে স্বাগতম',no_cameras:'কোনো ক্যামেরা কনফিগার করা হয়নি',settings:'সেটিংস',add_camera:'ক্যামেরা যোগ',scan:'ক্যামেরা খুঁজুন',scanning:'খুঁজছে...',connecting:'সংযোগ...',connected:'সংযুক্ত',offline:'অফলাইন',live:'লাইভ',photo:'ছবি',record:'রেকর্ড',stop:'বন্ধ',servo:'সার্ভো',settings_title:'সেটিংস',language:'ভাষা',theme:'থিম',dark:'ডার্ক',light:'লাইট',auto:'অটো',camera_name:'নাম',wifi:'WiFi',battery:'ব্যাটারি',password:'পাসওয়ার্ড',enter_password:'ক্যামেরা পাসওয়ার্ড দিন',save:'সংরক্ষণ',cancel:'বাতিল',back:'পিছনে',no_results:'ফলাফল নেই',error:'ত্রুটি'},
    tr:{name:'Türkçe',flag:'TR',welcome:'CamaraEspia''ya Hos Geldiniz',no_cameras:'Kamera yok',settings:'Ayarlar',add_camera:'Kamera ekle',scan:'Kamera ara',scanning:'Araniyor...',connecting:'Baglaniyor...',connected:'Bagli',offline:'Cevrimdisi',live:'CANLI',photo:'Foto',record:'Kaydet',stop:'Dur',servo:'Servo',settings_title:'Ayarlar',language:'Dil',theme:'Tema',dark:'Karanlik',light:'Acik',auto:'Oto',camera_name:'Ad',wifi:'WiFi',battery:'Pil',password:'Sifre',enter_password:'Kamera sifresini girin',save:'Kaydet',cancel:'Iptal',back:'Geri',no_results:'Sonuc yok',error:'Hata'},
    vi:{name:'Tiếng Việt',flag:'VN',welcome:'Chao mung den voi CamaraEspia',no_cameras:'Chua co camera nao',settings:'Cai dat',add_camera:'Them camera',scan:'Tim camera',scanning:'Dang tim...',connecting:'Dang ket noi...',connected:'Da ket noi',offline:'Mang offline',live:'TRUC TIEP',photo:'Anh',record:'Quay',stop:'Dung',servo:'Servo',settings_title:'Cai dat',language:'Ngon ngu',theme:'Chu de',dark:'Toi',light:'Sang',auto:'Tu dong',camera_name:'Ten',wifi:'WiFi',battery:'Pin',password:'Mat khau',enter_password:'Nhap mat khau camera',save:'Luu',cancel:'Huy',back:'Quay lai',no_results:'Khong co ket qua',error:'Loi'},
    th:{name:'ภาษาไทย',flag:'TH',welcome:'ยินดีต้อนรับสู่ CamaraEspia',no_cameras:'ยังไม่มีการตั้งค่ากล้อง',settings:'การตั้งค่า',add_camera:'เพิ่มกล้อง',scan:'ค้นหากล้อง',scanning:'กำลังค้นหา...',connecting:'กำลังเชื่อมต่อ...',connected:'เชื่อมต่อแล้ว',offline:'ออฟไลน์',live:'ถ่ายทอดสด',photo:'รูป',record:'บันทึก',stop:'หยุด',servo:'เซอร์โว',settings_title:'การตั้งค่า',language:'ภาษา',theme:'ธีม',dark:'มืด',light:'สว่าง',auto:'อัตโนมัติ',camera_name:'ชื่อ',wifi:'WiFi',battery:'แบตเตอรี่',password:'รหัสผ่าน',enter_password:'ป้อนรหัสผ่านกล้อง',save:'บันทึก',cancel:'ยกเลิก',back:'ย้อนกลับ',no_results:'ไม่มีผลลัพธ์',error:'ข้อผิดพลาด'},
    id:{name:'Indonesia',flag:'ID',welcome:'Selamat datang di CamaraEspia',no_cameras:'Belum ada kamera',settings:'Pengaturan',add_camera:'Tambah kamera',scan:'Cari kamera',scanning:'Mencari...',connecting:'Menghubungkan...',connected:'Tersambung',offline:'Offline',live:'LANGSUNG',photo:'Foto',record:'Rekam',stop:'Berhenti',servo:'Servo',settings_title:'Pengaturan',language:'Bahasa',theme:'Tema',dark:'Gelap',light:'Terang',auto:'Otomatis',camera_name:'Nama',wifi:'WiFi',battery:'Baterai',password:'Kata sandi',enter_password:'Masukkan kata sandi kamera',save:'Simpan',cancel:'Batal',back:'Kembali',no_results:'Tidak ada hasil',error:'Kesalahan'},
    pl:{name:'Polski',flag:'PL',welcome:'Witamy w CamaraEspia',no_cameras:'Brak kamer',settings:'Ustawienia',add_camera:'Dodaj kamera',scan:'Szukaj kamer',scanning:'Szukanie...',connecting:'Laczenie...',connected:'Polaczono',offline:'Offline',live:'NA ZYWO',photo:'Zdjecie',record:'Nagrywaj',stop:'Zatrzymaj',servo:'Servo',settings_title:'Ustawienia',language:'Jezyk',theme:'Motyw',dark:'Ciemny',light:'Jasny',auto:'Auto',camera_name:'Nazwa',wifi:'WiFi',battery:'Bateria',password:'Haslo',enter_password:'Wprowadz haslo kamery',save:'Zapisz',cancel:'Anuluj',back:'Wstecz',no_results:'Brak wynikow',error:'Blad'},
    uk:{name:'Українська',flag:'UA',welcome:'Ласкаво просимо до CamaraEspia',no_cameras:'Немає налаштованих камер',settings:'Налаштування',add_camera:'Додати камеру',scan:'Знайти камери',scanning:'Пошук...',connecting:'Підключення...',connected:'Підключено',offline:'Офлайн',live:'ОНЛАЙН',photo:'Фото',record:'Запис',stop:'Стоп',servo:'Серво',settings_title:'Налаштування',language:'Мова',theme:'Тема',dark:'Темна',light:'Світла',auto:'Авто',camera_name:'Iм\'я',wifi:'WiFi',battery:'Батарея',password:'Пароль',enter_password:'Введiть пароль камери',save:'Зберегти',cancel:'Скасувати',back:'Назад',no_results:'Немає результатів',error:'Помилка'},
    ms:{name:'Melayu',flag:'MY',welcome:'Selamat datang ke CamaraEspia',no_cameras:'Tiada kamera dikonfigurasi',settings:'Tetapan',add_camera:'Tambah kamera',scan:'Cari kamera',scanning:'Mencari...',connecting:'Menyambung...',connected:'Bersambung',offline:'Offline',live:'LANGSUNG',photo:'Gambar',record:'Rakam',stop:'Henti',servo:'Servo',settings_title:'Tetapan',language:'Bahasa',theme:'Tema',dark:'Gelap',light:'Terang',auto:'Auto',camera_name:'Nama',wifi:'WiFi',battery:'Bateri',password:'Kata laluan',enter_password:'Masukkan kata laluan kamera',save:'Simpan',cancel:'Batal',back:'Kembali',no_results:'Tiada hasil',error:'Ralat'},
    sw:{name:'Kiswahili',flag:'KE',welcome:'Karibu CamaraEspia',no_cameras:'Hakuna kamera',settings:'Mpangilio',add_camera:'Ongeza kamera',scan:'Tafuta kamera',scanning:'Inatafuta...',connecting:'Inaunganisha...',connected:'Imeunganishwa',offline:'Nje ya mtandao',live:'MOJA KWA MOJA',photo:'Picha',record:'Rekodi',stop:'Simama',servo:'Servo',settings_title:'Mpangilio',language:'Lugha',theme:'Mandhari',dark:'Giza',light:'Nuru',auto:'Otomatiki',camera_name:'Jina',wifi:'WiFi',battery:'Betri',password:'Nenosiri',enter_password:'Weka nenosiri ya kamera',save:'Hifadhi',cancel:'Ghairi',back:'Rudi',no_results:'Hakuna matokeo',error:'Hitilafu'},
    tl:{name:'Filipino',flag:'PH',welcome:'Maligayang pagdating sa CamaraEspia',no_cameras:'Walang camera',settings:'Settings',add_camera:'Dagdag camera',scan:'Hanapin camera',scanning:'Hinahanap...',connecting:'Kumokonekta...',connected:'Nakakonekta',offline:'Offline',live:'LIVE',photo:'Foto',record:'I-record',stop:'Itigil',servo:'Servo',settings_title:'Settings',language:'Wika',theme:'Tema',dark:'Dilim',light:'Liwanag',auto:'Auto',camera_name:'Pangalan',wifi:'WiFi',battery:'Battery',password:'Password',enter_password:'Ilagay ang password ng camera',save:'I-save',cancel:'Kanselahin',back:'Bumalik',no_results:'Walang resulta',error:'Error'},
    nl:{name:'Nederlands',flag:'NL',welcome:'Welkom bij CamaraEspia',no_cameras:'Geen camera''s geconfigureerd',settings:'Instellingen',add_camera:'Camera toevoegen',scan:'Zoek camera''s',scanning:'Zoeken...',connecting:'Verbinden...',connected:'Verbonden',offline:'Offline',live:'LIVE',photo:'Foto',record:'Opnemen',stop:'Stop',servo:'Servo',settings_title:'Instellingen',language:'Taal',theme:'Thema',dark:'Donker',light:'Licht',auto:'Auto',camera_name:'Naam',wifi:'WiFi',battery:'Batterij',password:'Wachtwoord',enter_password:'Voer camera wachtwoord in',save:'Opslaan',cancel:'Annuleren',back:'Terug',no_results:'Geen resultaten',error:'Fout'}
};

let _lang = localStorage.getItem('lang') || 'es';
let T = LANGS[_lang];

function setLang(l) { _lang = l; T = LANGS[l]; localStorage.setItem('lang', l); document.documentElement.lang = l; }
function t(k) { return (T && T[k]) || (LANGS.en[k]) || k; }
function getLang() { return _lang; }
function getAllLangs() { return Object.keys(LANGS).map(k => ({id:k, name:LANGS[k].name, flag:LANGS[k].flag})); }
)rawliteral";

// ============================================================
// js/discovery.js
// ============================================================
const char PWA_DISCOVERY_JS[] PROGMEM = R"rawliteral(
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
)rawliteral";

// ============================================================
// js/camera.js
// ============================================================
const char PWA_CAMERA_JS[] PROGMEM = R"rawliteral(
const Camera = {
    current: null,

    renderStream(cam) {
        return '<div class="stream-container">' +
            '<img src="http://' + cam.ip + ':81/stream" alt="Stream" onerror="this.parentElement.innerHTML=\'<div style=&quot;display:flex;align-items:center;justify-content:center;height:100%;color:#888&quot;>Stream no disponible</div>\'">' +
            '<div class="badge">' + cam.name + '</div>' +
            '</div>';
    },

    renderStats(cam) {
        return '<div class="stats-grid" id="camStats">' +
            '<div class="stat-card"><div class="label">' + t('wifi') + '</div><div class="value" id="statWifi">...</div></div>' +
            '<div class="stat-card"><div class="label">' + t('battery') + '</div><div class="value" id="statBat">...</div></div>' +
            '<div class="stat-card"><div class="label">' + t('camera_name') + '</div><div class="value" id="statName">' + cam.name + '</div></div>' +
            '<div class="stat-card"><div class="label">IP</div><div class="value">' + cam.ip + '</div></div>' +
            '</div>';
    },

    renderControls(cam) {
        return '<div style="margin-top:12px">' +
            '<div class="btn-row">' +
            '<button class="btn btn-green" onclick="Camera.takePhoto(\'' + cam.id + '\')">' + t('photo') + '</button>' +
            '<button class="btn btn-dark" onclick="Camera.toggleRecord(\'' + cam.id + '\')">' + t('record') + '</button>' +
            '</div></div>';
    },

    renderFull(cam) {
        this.current = cam;
        return '<div>' +
            this.renderStream(cam) +
            this.renderStats(cam) +
            this.renderControls(cam) +
            '</div>';
    },

    async loadStatus(cam) {
        try {
            const r = await fetch('http://' + cam.ip + '/api/camera/status');
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
            const r = await fetch('http://' + cam.ip + '/api/capture');
            if (r.ok) {
                const blob = await r.blob();
                const url = URL.createObjectURL(blob);
                const a = document.createElement('a');
                a.href = url; a.download = 'foto_' + cam.name + '_' + Date.now() + '.jpg'; a.click();
                URL.revokeObjectURL(url);
            }
        } catch(e) { alert(t('error')); }
    },

    async toggleRecord(id) {
        const cam = Discovery.getCamera(id);
        if (!cam) return;
        try { await fetch('http://' + cam.ip + '/api/record/toggle', { method: 'POST' }); } catch(e) {}
    }
};
)rawliteral";

// ============================================================
// js/settings.js
// ============================================================
const char PWA_SETTINGS_JS[] PROGMEM = R"rawliteral(
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
        const langOpts = langs.map(function(l) {
            return '<option value="' + l.id + '" ' + (l.id === s.lang ? 'selected' : '') + '>' + l.flag + ' ' + l.name + '</option>';
        }).join('');

        return '<div class="card">' +
            '<h3>' + t('language') + '</h3>' +
            '<div class="field">' +
            '<select onchange="Settings.changeLang(this.value)">' + langOpts + '</select>' +
            '</div></div>' +
            '<div class="card">' +
            '<h3>' + t('theme') + '</h3>' +
            '<div class="btn-row">' +
            '<button class="btn ' + (s.theme === 'dark' ? 'btn-green' : 'btn-dark') + '" onclick="Settings.setTheme(\'dark\'); App.renderSettings()">' + t('dark') + '</button>' +
            '<button class="btn ' + (s.theme === 'light' ? 'btn-green' : 'btn-dark') + '" onclick="Settings.setTheme(\'light\'); App.renderSettings()">' + t('light') + '</button>' +
            '<button class="btn ' + (s.theme === 'auto' ? 'btn-green' : 'btn-dark') + '" onclick="Settings.setTheme(\'auto\'); App.renderSettings()">' + t('auto') + '</button>' +
            '</div></div>' +
            '<div class="card">' +
            '<h3>' + t('add_camera') + '</h3>' +
            '<p style="margin-bottom:12px">IP: <strong>192.168.x.x</strong></p>' +
            '<button class="btn btn-green" onclick="goTo(\'/add\')">' + t('scan') + '</button>' +
            '</div>';
    },

    changeLang(l) {
        setLang(l);
        App.renderAll();
    }
};
)rawliteral";

// ============================================================
// js/app.js
// ============================================================
const char PWA_APP_JS[] PROGMEM = R"rawliteral(
const App = {
    init() {
        Settings.applyTheme(Settings.get().theme);
        if ('serviceWorker' in navigator) navigator.serviceWorker.register('/sw.js').catch(function() {});

        document.getElementById('openSidebar').onclick = function() {
            document.getElementById('sidebar').classList.add('open');
            App._addOverlay();
            document.getElementById('overlay').classList.add('open');
        };
        document.getElementById('closeSidebar').onclick = function() { App.closeSidebar(); };

        this._addOverlay();
        this.renderAll();
        this.updateSidebar();

        window.addEventListener('hashchange', function() { App.route(); });
        this.route();
    },

    _addOverlay() {
        if (!document.getElementById('overlay')) {
            var ov = document.createElement('div');
            ov.id = 'overlay';
            ov.className = 'overlay';
            ov.onclick = function() { App.closeSidebar(); };
            document.getElementById('app').appendChild(ov);
        }
    },

    closeSidebar() {
        document.getElementById('sidebar').classList.remove('open');
        var ov = document.getElementById('overlay');
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
        var list = document.getElementById('cameraList');
        var empty = document.getElementById('emptyMsg');
        if (Discovery.cameras.length === 0) {
            list.innerHTML = '';
            list.appendChild(empty);
            empty.style.display = 'block';
            return;
        }
        empty.style.display = 'none';
        list.innerHTML = Discovery.cameras.map(function(c) {
            return '<div class="camera-item ' + (c.id === (Camera.current && Camera.current.id) ? 'active' : '') + '" onclick="App.openCamera(\'' + c.id + '\')">' +
                '<div class="dot ' + (c.online ? 'online' : 'offline') + '"></div>' +
                '<div class="info">' +
                '<div class="name">' + c.name + '</div>' +
                '<div class="ip">' + c.ip + '</div>' +
                '</div></div>';
        }).join('');
    },

    openCamera(id) {
        var cam = Discovery.getCamera(id);
        if (!cam) return;
        this.closeSidebar();
        window.location.hash = '#/camera/' + id;
    },

    route() {
        var hash = window.location.hash || '#/';
        var parts = hash.replace('#/', '').split('/');
        var views = document.querySelectorAll('.view');
        views.forEach(function(v) { v.classList.remove('active'); });

        if (parts[0] === 'camera' && parts[1]) {
            var cam = Discovery.getCamera(parts[1]);
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
            return '<div style="text-align:center;padding:60px 20px">' +
                '<div style="font-size:48px;margin-bottom:16px">📷</div>' +
                '<h2 style="margin-bottom:8px">' + t('welcome') + '</h2>' +
                '<p style="color:var(--text2);margin-bottom:24px">' + t('no_cameras') + '</p>' +
                '<button class="btn btn-green" onclick="goTo(\'/add\')">' + t('add_camera') + '</button>' +
                '</div>';
        }
        return '<div>' +
            '<div class="btn-row" style="margin-bottom:16px">' +
            '<button class="btn btn-green" onclick="App.scanLAN()">' + t('scan') + '</button>' +
            '</div>' +
            Discovery.cameras.map(function(c) {
                return '<div class="card" onclick="App.openCamera(\'' + c.id + '\')" style="cursor:pointer">' +
                    '<div style="display:flex;align-items:center;gap:12px">' +
                    '<div class="dot ' + (c.online ? 'online' : 'offline') + '"></div>' +
                    '<div><h3>' + c.name + '</h3><p>' + c.ip + '</p></div>' +
                    '</div></div>';
            }).join('') +
            '</div>';
    },

    renderAddCamera() {
        return '<div class="card">' +
            '<h3>' + t('scan') + '</h3>' +
            '<p style="margin-bottom:12px">' + t('scanning') + '</p>' +
            '<button class="btn btn-green" onclick="App.scanLAN()" id="scanBtn">' + t('scan') + '</button>' +
            '<div id="scanResults" style="margin-top:12px"></div>' +
            '</div>' +
            '<div class="card">' +
            '<h3>' + t('add_camera') + ' IP manual</h3>' +
            '<div class="field">' +
            '<input type="text" id="manualIP" placeholder="192.168.1.100">' +
            '</div>' +
            '<div class="field">' +
            '<input type="password" id="manualPass" placeholder="' + t('password') + '">' +
            '</div>' +
            '<button class="btn btn-green" onclick="App.addManual()">' + t('save') + '</button>' +
            '</div>';
    },

    async scanLAN() {
        var btn = document.getElementById('scanBtn');
        var results = document.getElementById('scanResults');
        if (btn) { btn.disabled = true; btn.textContent = t('scanning'); }
        if (results) results.innerHTML = '';

        await Discovery.scanLAN(function(ip, found, count) {
            if (found && results) {
                results.innerHTML += '<div class="card" style="cursor:pointer" onclick="App.addDiscovered(\'' + ip + '\')">' +
                    '<p>OK ' + ip + ' — encontrado</p>' +
                    '</div>';
            }
        });

        if (btn) { btn.disabled = false; btn.textContent = t('scan'); }
        this.updateSidebar();
    },

    async addDiscovered(ip) {
        var cam = await Discovery.addCamera(ip, '');
        if (cam) { this.updateSidebar(); goTo('/'); }
    },

    async addManual() {
        var ip = document.getElementById('manualIP').value.trim();
        var pass = document.getElementById('manualPass').value;
        if (!ip) return;
        var cam = await Discovery.addCamera(ip, pass);
        if (cam) { this.updateSidebar(); goTo('/'); }
        else alert(t('error'));
    }
};

function goTo(path) { window.location.hash = '#' + path; }
function goHome() { window.location.hash = '#/'; }

document.addEventListener('DOMContentLoaded', function() { App.init(); });
)rawliteral";

// ============================================================
// Icon 192x192 (minimal green PNG placeholder)
// ============================================================
const uint8_t PWA_ICON_192[] PROGMEM = {
  0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00, 0x00, 0x0d,
  0x49, 0x48, 0x44, 0x52, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01,
  0x08, 0x02, 0x00, 0x00, 0x00, 0x90, 0x77, 0x53, 0xde, 0x00, 0x00, 0x00,
  0x0c, 0x49, 0x44, 0x41, 0x54, 0x08, 0xd7, 0x63, 0xd8, 0xab, 0xf0, 0x01,
  0x00, 0x00, 0x01, 0x01, 0x00, 0x05, 0x18, 0xd8, 0xf2, 0x00, 0x00, 0x00,
  0x00, 0x49, 0x45, 0x4e, 0x44, 0xae, 0x42, 0x60, 0x82
};

const uint8_t PWA_ICON_512[] PROGMEM = {
  0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00, 0x00, 0x0d,
  0x49, 0x48, 0x44, 0x52, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01,
  0x08, 0x02, 0x00, 0x00, 0x00, 0x90, 0x77, 0x53, 0xde, 0x00, 0x00, 0x00,
  0x0c, 0x49, 0x44, 0x41, 0x54, 0x08, 0xd7, 0x63, 0xd8, 0xab, 0xf0, 0x01,
  0x00, 0x00, 0x01, 0x01, 0x00, 0x05, 0x18, 0xd8, 0xf2, 0x00, 0x00, 0x00,
  0x00, 0x49, 0x45, 0x4e, 0x44, 0xae, 0x42, 0x60, 0x82
};

const size_t PWA_ICON_192_SIZE = sizeof(PWA_ICON_192);
const size_t PWA_ICON_512_SIZE = sizeof(PWA_ICON_512);

#endif // PWA_DATA_H
