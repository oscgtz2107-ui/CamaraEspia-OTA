#include "captive_portal.h"
#include "pwa_data.h"
#include <WiFi.h>
#include <Preferences.h>
#include <mbedtls/sha256.h>

static String sha256(const String& input) {
    unsigned char hash[32];
    mbedtls_sha256_context ctx;
    mbedtls_sha256_init(&ctx);
    mbedtls_sha256_starts(&ctx, 0);
    mbedtls_sha256_update(&ctx, (const unsigned char*)input.c_str(), input.length());
    mbedtls_sha256_finish(&ctx, hash);
    mbedtls_sha256_free(&ctx);
    char buf[65];
    for (int i = 0; i < 32; i++) snprintf(buf + i * 2, 3, "%02x", hash[i]);
    return String(buf);
}

void CaptivePortal::begin() {
    _server = new AsyncWebServer(80);
    _dns = new DNSServer();
    _active = true;
    setupRoutes();
    _server->begin();
    // DNS: redirigir TODO dominio a 192.168.4.1
    _dns->start(53, "*", WiFi.softAPIP());
    Serial.println("[CP] Captive portal activo en 192.168.4.1 (DNS wildcard ON)");
}

void CaptivePortal::stop() {
    if (_dns) {
        _dns->stop();
        delete _dns;
        _dns = nullptr;
    }
    if (_server) {
        _server->end();
        delete _server;
        _server = nullptr;
    }
    _active = false;
}

void CaptivePortal::process() {
    if (_dns) _dns->processNextRequest();
    // Poll async WiFi scan
    if (_wifiScanning) {
        int16_t result = WiFi.scanComplete();
        if (result >= 0) {
            String json = "[";
            for (int i = 0; i < result; i++) {
                if (i) json += ",";
                json += "{\"ssid\":\"" + WiFi.SSID(i) + "\",\"rssi\":" + String(WiFi.RSSI(i)) +
                        ",\"enc\":" + String(WiFi.encryptionType(i)) + "}";
            }
            json += "]";
            _wifiCache = json;
            WiFi.scanDelete();
            _wifiScanning = false;
            Serial.printf("[CP] WiFi scan completo: %d redes\n", result);
        }
    }
}

void CaptivePortal::setupRoutes() {
    // === iOS captive portal ===
    // iOS checks this URL and shows popup if response is NOT the success HTML
    _server->on("/hotspot-detect.html", HTTP_GET, [](AsyncWebServerRequest* r) {
        // Return HTML that does NOT contain "Success" → iOS shows captive portal
        r->send(200, "text/html",
            "<!DOCTYPE html><html><head><title>CameraEspia</title></head>"
            "<body><script>window.location='http://192.168.4.1/';</script></body></html>");
    });

    // === Android captive portal ===
    // Android checks /generate_204 — needs 302 redirect
    _server->on("/generate_204", HTTP_GET, [](AsyncWebServerRequest* r) {
        AsyncWebServerResponse* res = r->beginResponse(302, "text/html", "");
        res->addHeader("Location", "http://192.168.4.1/");
        r->send(res);
    });
    _server->on("/gen_204", HTTP_GET, [](AsyncWebServerRequest* r) {
        AsyncWebServerResponse* res = r->beginResponse(302, "text/html", "");
        res->addHeader("Location", "http://192.168.4.1/");
        r->send(res);
    });

    // === Windows captive portal ===
    _server->on("/connecttest.txt", HTTP_GET, [](AsyncWebServerRequest* r) {
        AsyncWebServerResponse* res = r->beginResponse(302, "text/html", "");
        res->addHeader("Location", "http://192.168.4.1/");
        r->send(res);
    });
    _server->on("/ncsi.txt", HTTP_GET, [](AsyncWebServerRequest* r) {
        r->send(200, "text/plain", "NCSI HTTP/400 Bad Request");
    });

    // === Firefox ===
    _server->on("/success.txt", HTTP_GET, [](AsyncWebServerRequest* r) {
        AsyncWebServerResponse* res = r->beginResponse(302, "text/html", "");
        res->addHeader("Location", "http://192.168.4.1/");
        r->send(res);
    });

    // === Kindle ===
    _server->on("/kindle-wifi/wifistub.html", HTTP_GET, [](AsyncWebServerRequest* r) {
        AsyncWebServerResponse* res = r->beginResponse(302, "text/html", "");
        res->addHeader("Location", "http://192.168.4.1/");
        r->send(res);
    });

    // Root - serve setup page
    _server->on("/", HTTP_GET, [this](AsyncWebServerRequest* r) {
        handleRoot(r);
    });

    // WiFi scan — async, no bloquea
    _server->on("/api/wifi/scan", HTTP_GET, [this](AsyncWebServerRequest* r) {
        if (_wifiScanning) {
            // Scan en progreso — devolver cache vacío
            r->send(200, "application/json", "[]");
            return;
        }
        if (_wifiCache.length() > 2) {
            // Cache fresco (< 15s) — devolver directo
            r->send(200, "application/json", _wifiCache);
            _wifiScanning = true;
            _wifiScanStart = millis();
            WiFi.scanNetworks(true); // async
            return;
        }
        // Primera vez — devolver vacío y lanzar scan
        r->send(200, "application/json", "[]");
        _wifiScanning = true;
        _wifiScanStart = millis();
        WiFi.scanNetworks(true);
    });

    // Save config
    _server->on("/api/save", HTTP_POST, [this](AsyncWebServerRequest* r) {
        handleSave(r);
    });

    // Set language (client-side preference)
    _server->on("/api/lang", HTTP_POST, [](AsyncWebServerRequest* r) {
        r->send(200, "application/json", "{\"ok\":true}");
    });

    // === PWA files (for install prompt during captive portal) ===
    _server->on("/manifest.json", HTTP_GET, [](AsyncWebServerRequest* r) {
        AsyncWebServerResponse* resp = r->beginResponse_P(200, "application/json", PWA_MANIFEST_JSON);
        r->send(resp);
    });
    _server->on("/sw.js", HTTP_GET, [](AsyncWebServerRequest* r) {
        AsyncWebServerResponse* resp = r->beginResponse_P(200, "application/javascript", PWA_SW_JS);
        r->send(resp);
    });
    _server->on("/css/style.css", HTTP_GET, [](AsyncWebServerRequest* r) {
        AsyncWebServerResponse* resp = r->beginResponse_P(200, "text/css", PWA_STYLE_CSS);
        r->send(resp);
    });
    _server->on("/js/i18n.js", HTTP_GET, [](AsyncWebServerRequest* r) {
        AsyncWebServerResponse* resp = r->beginResponse_P(200, "application/javascript", PWA_I18N_JS);
        r->send(resp);
    });
    _server->on("/js/discovery.js", HTTP_GET, [](AsyncWebServerRequest* r) {
        AsyncWebServerResponse* resp = r->beginResponse_P(200, "application/javascript", PWA_DISCOVERY_JS);
        r->send(resp);
    });
    _server->on("/js/camera.js", HTTP_GET, [](AsyncWebServerRequest* r) {
        AsyncWebServerResponse* resp = r->beginResponse_P(200, "application/javascript", PWA_CAMERA_JS);
        r->send(resp);
    });
    _server->on("/js/settings.js", HTTP_GET, [](AsyncWebServerRequest* r) {
        AsyncWebServerResponse* resp = r->beginResponse_P(200, "application/javascript", PWA_SETTINGS_JS);
        r->send(resp);
    });
    _server->on("/js/app.js", HTTP_GET, [](AsyncWebServerRequest* r) {
        AsyncWebServerResponse* resp = r->beginResponse_P(200, "application/javascript", PWA_APP_JS);
        r->send(resp);
    });
    _server->on("/icons/icon-192.png", HTTP_GET, [](AsyncWebServerRequest* r) {
        AsyncWebServerResponse* resp = r->beginResponse_P(200, "image/png", PWA_ICON_192, PWA_ICON_192_SIZE);
        r->send(resp);
    });
    _server->on("/icons/icon-512.png", HTTP_GET, [](AsyncWebServerRequest* r) {
        AsyncWebServerResponse* resp = r->beginResponse_P(200, "image/png", PWA_ICON_512, PWA_ICON_512_SIZE);
        r->send(resp);
    });

    // Fallback - redirect unknown paths to root
    _server->onNotFound([](AsyncWebServerRequest* r) {
        AsyncWebServerResponse* res = r->beginResponse(302, "text/html", "");
        res->addHeader("Location", "http://192.168.4.1/");
        r->send(res);
    });
}

void CaptivePortal::handleRoot(AsyncWebServerRequest* request) {
    String html = getSetupHTML();
    AsyncWebServerResponse* response = request->beginResponse(200, "text/html", html);
    response->addHeader("Cache-Control", "no-cache");
    request->send(response);
}

void CaptivePortal::handleSave(AsyncWebServerRequest* request) {
    if (!request->hasParam("ssid", true) || !request->hasParam("password", true) ||
        !request->hasParam("camera_name", true) || !request->hasParam("device_password", true) ||
        !request->hasParam("language", true)) {
        request->send(400, "application/json", "{\"error\":\"missing params\"}");
        return;
    }

    String ssid = request->getParam("ssid", true)->value();
    String pass = request->getParam("password", true)->value();
    String name = request->getParam("camera_name", true)->value();
    String devPass = request->getParam("device_password", true)->value();
    String lang = request->getParam("language", true)->value();

    Preferences prefs;
    prefs.begin("camara", false);
    prefs.putString("wifi_ssid", ssid);
    prefs.putString("wifi_pass", pass);
    prefs.putString("camera_name", name);
    prefs.putString("device_password", sha256(devPass));
    prefs.putString("language", lang);
    prefs.putBool("setup_complete", true);
    prefs.end();

    request->send(200, "application/json", "{\"ok\":true}");

    delay(1000);
    ESP.restart();
}

String CaptivePortal::getSetupHTML() {
    return R"rawliteral(
<!DOCTYPE html>
<html lang="es">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1,maximum-scale=1,user-scalable=no">
<title>CameraEspia</title>
<style>
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',sans-serif;background:#0a0a0a;color:#e0e0e0;min-height:100vh;display:flex;justify-content:center;align-items:center;padding:16px}
.card{background:#1a1a1a;border-radius:16px;padding:28px 24px;max-width:420px;width:100%;box-shadow:0 8px 32px rgba(0,0,0,.6)}
h1{text-align:center;font-size:22px;margin-bottom:4px}
.sub{text-align:center;color:#888;margin-bottom:24px;font-size:13px}
.panel{display:none}
.panel.active{display:block}
.field{margin-bottom:18px}
.field label{display:block;font-size:12px;color:#aaa;margin-bottom:6px;text-transform:uppercase;letter-spacing:.5px}
.field input,.field select{width:100%;padding:12px;border-radius:8px;border:1px solid #333;background:#0f0f0f;color:#fff;font-size:15px}
.field input:focus{outline:none;border-color:#4CAF50}
.btn{width:100%;padding:14px;border:none;border-radius:10px;font-size:16px;font-weight:600;cursor:pointer;margin-top:8px;transition:all .2s}
.btn-green{background:#4CAF50;color:#fff}.btn-green:hover{background:#45a049}
.btn-dark{background:#2a2a2a;color:#ccc;border:1px solid #444}.btn-dark:hover{background:#333}
.btn:disabled{background:#222;color:#555;cursor:not-allowed}
.scan-row{display:flex;gap:8px;align-items:center;margin-bottom:8px}
.scan-row select{flex:1}
.scan-row button{padding:12px;border-radius:8px;border:1px solid #444;background:#222;color:#aaa;font-size:14px;cursor:pointer;white-space:nowrap}
.scan-row button:hover{background:#333}
.lang-grid{display:grid;grid-template-columns:repeat(3,1fr);gap:8px;margin-top:16px}
.lang-btn{padding:12px 8px;border-radius:8px;border:1px solid #333;background:#111;color:#ddd;font-size:13px;cursor:pointer;text-align:center;transition:all .2s}
.lang-btn:hover{border-color:#4CAF50;background:#1a2a1a}
.lang-btn.selected{border-color:#4CAF50;background:#1b3a1b;color:#4CAF50}
.lang-btn .flag{font-size:20px;display:block;margin-bottom:4px}
.step-num{display:inline-block;width:28px;height:28px;line-height:28px;text-align:center;border-radius:50%;background:#4CAF50;color:#fff;font-size:13px;font-weight:700;margin-right:8px}
.app-icon{width:80px;height:80px;border-radius:20px;background:linear-gradient(135deg,#1b5e20,#4CAF50);margin:0 auto 16px;display:flex;align-items:center;justify-content:center;font-size:36px}
.hint{font-size:11px;color:#666;margin-top:4px}
.status{margin-top:12px;padding:10px;border-radius:8px;font-size:13px;display:none}
.status.ok{display:block;background:#1b3a1b;color:#4CAF50}
.status.err{display:block;background:#3a1b1b;color:#f44336}
.spinner{display:inline-block;width:16px;height:16px;border:2px solid #fff;border-top-color:transparent;border-radius:50%;animation:spin .6s linear infinite;margin-right:8px;vertical-align:middle}
@keyframes spin{to{transform:rotate(360deg)}}
.arrow{font-size:20px;vertical-align:middle;margin-left:4px}
</style>
</head>
<body>
<div class="card">
<h1 id="title">📷 CameraEspia</h1>
<p class="sub" id="subtitle"></p>

<!-- PANEL 1: LANGUAGE -->
<div class="panel active" id="p1">
<p id="p1text" style="text-align:center;margin-bottom:8px"></p>
<div class="lang-grid" id="langGrid"></div>
</div>

<!-- PANEL 2: DOWNLOAD APP -->
<div class="panel" id="p2">
<div class="app-icon">📷</div>
<p id="p2text" style="text-align:center;margin-bottom:20px;line-height:1.5;font-size:14px"></p>
<button class="btn btn-green" id="dlBtn" onclick="installPWA()">
<span id="dlBtnText"></span>
</button>
</div>

<!-- PANEL 3: CONFIG -->
<div class="panel" id="p3">
<div class="field">
<label><span class="step-num">1</span><span id="l3wifi"></span></label>
<div class="scan-row">
<select id="wifiList"><option value="">...</option></select>
<button onclick="scanWifi()" id="scanBtn">🔍</button>
</div>
<input type="password" id="wifiPass" placeholder="">
</div>
<div class="field">
<label><span class="step-num">2</span><span id="l3name"></span></label>
<input type="text" id="camName" placeholder="">
</div>
<div class="field">
<label><span class="step-num">3</span><span id="l3pass"></span></label>
<input type="password" id="devPass" placeholder="">
<p class="hint" id="l3hint"></p>
</div>
<button class="btn btn-green" onclick="saveAll()" id="saveBtn"></button>
<div id="status" class="status"></div>
</div>

</div>

<script>
var S={};
var curLang='es';
var langs=[
{l:'es',f:'🇪🇸',n:'Español'},
{l:'en',f:'🇺🇸',n:'English'},
{l:'pt',f:'🇧🇷',n:'Português'},
{l:'fr',f:'🇫🇷',n:'Français'},
{l:'de',f:'🇩🇪',n:'Deutsch'},
{l:'it',f:'🇮🇹',n:'Italiano'},
{l:'ru',f:'🇷🇺',n:'Русский'},
{l:'zh',f:'🇨🇳',n:'中文'},
{l:'ja',f:'🇯🇵',n:'日本語'},
{l:'ko',f:'🇰🇷',n:'한국어'},
{l:'ar',f:'🇸🇦',n:'العربية'},
{l:'hi',f:'🇮🇳',n:'हिन्दी'},
{l:'bn',f:'🇧🇩',n:'বাংলা'},
{l:'tr',f:'🇹🇷',n:'Türkçe'},
{l:'vi',f:'🇻🇳',n:'Tiếng Việt'},
{l:'th',f:'🇹🇭',n:'ภาษาไทย'},
{l:'id',f:'🇮🇩',n:'Indonesia'},
{l:'pl',f:'🇵🇱',n:'Polski'},
{l:'uk',f:'🇺🇦',n:'Українська'},
{l:'ms',f:'🇲🇾',n:'Melayu'},
{l:'sw',f:'🇰🇪',n:'Kiswahili'},
{l:'tl',f:'🇵🇭',n:'Filipino'},
{l:'nl',f:'🇳🇱',n:'Nederlands'}
];

var T={
es:{welcome:'Bienvenido',sub:'Configura tu cámara en 3 pasos',p1text:'¿En qué idioma quieres continuar?',
p2text:'Descarga la app para gestionar tus cámaras desde el celular. Accede al video en vivo, recibe alertas y configura todo.',
dlBtn:'Descargar App',l3wifi:'Conectar a tu WiFi',l3name:'Nombre de la cámara',
l3pass:'Contraseña de acceso',l3hint:'Protege el acceso a esta cámara desde la app',
scanBtn:'🔍 Buscar',saveBtn:'💾 Guardar y reiniciar',saving:'Guardando...',saved:'¡Guardado! Reiniciando...',
errWifi:'Selecciona una red WiFi',errName:'Ingresa un nombre para la cámara',errPass:'Ingresa una contraseña',
errSave:'Error al guardar',errConn:'Error de conexión',wifiPass:'Contraseña WiFi',phName:'Ej: Patio, Sala, Garage',phPass:'Mínimo 6 caracteres',
instStep1:'Para instalar la app:',instIOS:'Toca el botón <b>Compartir</b> □↑ y selecciona <b>"Agregar a pantalla de inicio"</b>. La app aparecerá en tu pantalla de inicio.',
instAndroid:'Toca los <b>3 puntos</b> ⋮ y selecciona <b>"Agregar a pantalla de inicio"</b> o <b>"Instalar app"</b>. La app aparecerá en tu pantalla de inicio.',
instDesktop:'Haz clic en el <b>ícono de instalar</b> en la barra de direcciones, o presiona <b>Ctrl+Shift+I</b> → "Install CamaraEspia".'},
en:{welcome:'Welcome',sub:'Set up your camera in 3 steps',p1text:'Which language do you want to continue in?',
p2text:'Download the app to manage your cameras from your phone. Access live video, receive alerts and configure everything.',
dlBtn:'Download App',l3wifi:'Connect to your WiFi',l3name:'Camera name',
l3pass:'Device password',l3hint:'Protect access to this camera from the app',
scanBtn:'🔍 Scan',saveBtn:'💾 Save and restart',saving:'Saving...',saved:'Saved! Restarting...',
errWifi:'Select a WiFi network',errName:'Enter a camera name',errPass:'Enter a password',
errSave:'Error saving',errConn:'Connection error',wifiPass:'WiFi Password',phName:'Ex: Patio, Living room, Garage',phPass:'Minimum 6 characters',
instStep1:'To install the app:',instIOS:'Tap the <b>Share</b> button □↑ and select <b>"Add to Home Screen"</b>. The app will appear on your home screen.',
instAndroid:'Tap the <b>3 dots</b> ⋮ and select <b>"Add to Home Screen"</b> or <b>"Install app"</b>. The app will appear on your home screen.',
instDesktop:'Click the <b>install icon</b> in the address bar, or press <b>Ctrl+Shift+I</b> → "Install CamaraEspia".'},
pt:{welcome:'Bem-vindo',sub:'Configure sua câmera em 3 passos',p1text:'Em qual idioma deseja continuar?',
p2text:'Baixe o app para gerenciar suas câmeras pelo celular. Acesse o vídeo ao vivo, receba alertas e configure tudo.',
dlBtn:'Baixar App',l3wifi:'Conectar ao seu WiFi',l3name:'Nome da câmera',
l3pass:'Senha do dispositivo',l3hint:'Proteja o acesso a esta câmera pelo app',
scanBtn:'🔍 Buscar',saveBtn:'💾 Salvar e reiniciar',saving:'Salvando...',saved:'Salvo! Reiniciando...',
errWifi:'Selecione uma rede WiFi',errName:'Digite um nome para a câmera',errPass:'Digite uma senha',
errSave:'Erro ao salvar',errConn:'Erro de conexão',wifiPass:'Senha WiFi',phName:'Ex: Pátio, Sala, Garagem',phPass:'Mínimo 6 caracteres',
instStep1:'Para instalar o app:',instIOS:'Toque no botão <b>Compartilhar</b> □↑ e selecione <b>"Adicionar à Tela de Início"</b>. O app aparecerá na sua tela de início.',
instAndroid:'Toque nos <b>3 pontos</b> ⋮ e selecione <b>"Adicionar à Tela de Início"</b> ou <b>"Instalar app"</b>. O app aparecerá na sua tela de início.',
instDesktop:'Clique no <b>ícone de instalar</b> na barra de endereços, ou pressione <b>Ctrl+Shift+I</b> → "Install CamaraEspia".'},
fr:{welcome:'Bienvenue',sub:'Configurez votre caméra en 3 étapes',p1text:'Dans quelle langue souhaitez-vous continuer ?',
p2text:'Téléchargez l\'app pour gérer vos caméras depuis votre téléphone. Accédez à la vidéo en direct, recevez des alertes et configurez tout.',
dlBtn:'Télécharger App',l3wifi:'Connecter à votre WiFi',l3name:'Nom de la caméra',
l3pass:'Mot de passe de l\'appareil',l3hint:'Protégez l\'accès à cette caméra depuis l\'app',
scanBtn:'🔍 Chercher',saveBtn:'💾 Enregistrer et redémarrer',saving:'Enregistrement...',saved:'Enregistré ! Redémarrage...',
errWifi:'Sélectionnez un réseau WiFi',errName:'Entrez un nom pour la caméra',errPass:'Entrez un mot de passe',
errSave:'Erreur lors de l\'enregistrement',errConn:'Erreur de connexion',wifiPass:'Mot de passe WiFi',phName:'Ex: Patio, Salon, Garage',phPass:'6 caractères minimum',
instStep1:'Pour installer l\'app :',instIOS:'Appuyez sur le bouton <b>Partager</b> □↑ et sélectionnez <b>"Ajouter à l\'écran d\'accueil"</b>. L\'app apparaîtra sur votre écran d\'accueil.',
instAndroid:'Appuyez sur les <b>3 points</b> ⋮ et sélectionnez <b>"Ajouter à l\'écran d\'accueil"</b> ou <b>"Installer l\'app"</b>. L\'app apparaîtra sur votre écran d\'accueil.',
instDesktop:'Cliquez sur l\'<b>icône d\'installation</b> dans la barre d\'adresse, ou appuyez sur <b>Ctrl+Shift+I</b> → "Install CamaraEspia".'},
de:{welcome:'Willkommen',sub:'Richten Sie Ihre Kamera in 3 Schritten ein',p1text:'In welcher Sprache möchten Sie fortfahren?',
p2text:'Laden Sie die App herunter, um Ihre Kameras vom Handy aus zu verwalten. Zugriff auf Live-Video, Benachrichtigungen und Konfiguration.',
dlBtn:'App herunterladen',l3wifi:'Mit Ihrem WiFi verbinden',l3name:'Kameraname',
l3pass:'Gerätepasswort',l3hint:'Schützen Sie den Zugriff auf diese Kamera über die App',
scanBtn:'🔍 Suchen',saveBtn:'💾 Speichern und neu starten',saving:'Speichern...',saved:'Gespeichert! Neustart...',
errWifi:'Wählen Sie ein WiFi-Netzwerk',errName:'Geben Sie einen Kameranamen ein',errPass:'Geben Sie ein Passwort ein',
errSave:'Fehler beim Speichern',errConn:'Verbindungsfehler',wifiPass:'WiFi-Passwort',phName:'z.B. Terrasse, Wohnzimmer, Garage',phPass:'Mindestens 6 Zeichen',
instStep1:'App installieren:',instIOS:'Tippen Sie auf <b>Teilen</b> □↑ und wählen Sie <b>"Zum Home-Bildschirm"</b>. Die App erscheint auf Ihrem Home-Bildschirm.',
instAndroid:'Tippen Sie auf die <b>3 Punkte</b> ⋮ und wählen Sie <b>"Zum Home-Bildschirm"</b> oder <b>"App installieren"</b>. Die App erscheint auf Ihrem Home-Bildschirm.',
instDesktop:'Klicken Sie auf das <b>Installations-Symbol</b> in der Adressleiste oder drücken Sie <b>Ctrl+Shift+I</b> → "Install CamaraEspia".'},
it:{welcome:'Benvenuto',sub:'Configura la fotocamera in 3 passaggi',p1text:'In quale lingua vuoi continuare?',
p2text:'Scarica l\'app per gestire le tue fotocamere dal telefono. Accedi al video in vivo, ricevi avvisi e configura tutto.',
dlBtn:'Scarica App',l3wifi:'Connetti al tuo WiFi',l3name:'Nome della fotocamera',
l3pass:'Password del dispositivo',l3hint:'Proteggi l\'accesso a questa fotocamera dall\'app',
scanBtn:'🔍 Cerca',saveBtn:'💾 Salva e riavvia',saving:'Salvataggio...',saved:'Salvato! Riavvio...',
errWifi:'Seleziona una rete WiFi',errName:'Inserisci un nome per la fotocamera',errPass:'Inserisci una password',
errSave:'Errore nel salvataggio',errConn:'Errore di connessione',wifiPass:'Password WiFi',phName:'Es: Patio, Soggiorno, Garage',phPass:'Almeno 6 caratteri',
instStep1:'Per installare l\'app:',instIOS:'Tocca il pulsante <b>Condividi</b> □↑ e seleziona <b>"Aggiungi a schermata iniziale"</b>. L\'app apparirà sulla schermata iniziale.',
instAndroid:'Tocca i <b>3 punti</b> ⋮ e seleziona <b>"Aggiungi a schermata iniziale"</b> o <b>"Installa app"</b>. L\'app apparirà sulla schermata iniziale.',
instDesktop:'Clicca l\'<b>icona di installazione</b> nella barra degli indirizzi, o premi <b>Ctrl+Shift+I</b> → "Install CamaraEspia".'},
ru:{welcome:'Добро пожаловать',sub:'Настройте камеру за 3 шага',p1text:'На каком языке вы хотите продолжить?',
p2text:'Скачайте приложение для управления камерами с телефона. Просмотр видео, уведомления и настройки.',
dlBtn:'Скачать App',l3wifi:'Подключить к WiFi',l3name:'Имя камеры',
l3pass:'Пароль устройства',l3hint:'Защитите доступ к камере через приложение',
scanBtn:'🔍 Найти',saveBtn:'💾 Сохранить и перезагрузить',saving:'Сохранение...',saved:'Сохранено! Перезагрузка...',
errWifi:'Выберите сеть WiFi',errName:'Введите имя камеры',errPass:'Введите пароль',
errSave:'Ошибка сохранения',errConn:'Ошибка соединения',wifiPass:'Пароль WiFi',phName:'Напр: Двор, Гараж, Комната',phPass:'Минимум 6 символов',
instStep1:'Для установки приложения:',instIOS:'Нажмите <b>Поделиться</b> □↑ и выберите <b>"На экран Домой"</b>. Приложение появится на главном экране.',
instAndroid:'Нажмите <b>3 точки</b> ⋮ и выберите <b>"На экран Домой"</b> или <b>"Установить приложение"</b>. Приложение появится на главном экране.',
instDesktop:'Нажмите <b>иконку установки</b> в адресной строке или <b>Ctrl+Shift+I</b> → "Install CamaraEspia".'},
zh:{welcome:'欢迎',sub:'3步设置您的摄像头',p1text:'您想用哪种语言继续？',
p2text:'下载应用程序，从手机管理摄像头。实时视频、警报通知和全部配置。',
dlBtn:'下载应用',l3wifi:'连接WiFi',l3name:'摄像头名称',
l3pass:'设备密码',l3hint:'保护从App访问此摄像头',
scanBtn:'🔍 搜索',saveBtn:'💾 保存并重启',saving:'保存中...',saved:'已保存！重启中...',
errWifi:'选择WiFi网络',errName:'输入摄像头名称',errPass:'输入密码',
errSave:'保存失败',errConn:'连接错误',wifiPass:'WiFi密码',phName:'如: 院子、客厅、车库',phPass:'至少6个字符',
instStep1:'安装应用：',instIOS:'点击 <b>分享</b> 按钮 □↑，选择 <b>"添加到主屏幕"</b>。应用将出现在主屏幕上。',
instAndroid:'点击 <b>3个点</b> ⋮，选择 <b>"添加到主屏幕"</b> 或 <b>"安装应用"</b>。应用将出现在主屏幕上。',
instDesktop:'点击地址栏中的 <b>安装图标</b>，或按 <b>Ctrl+Shift+I</b> → "Install CamaraEspia"。'},
ja:{welcome:'ようこそ',sub:'3ステップでカメラをセットアップ',p1text:'どの言語で続行しますか？',
p2text:'アプリをダウンロードして、スマホからカメラを管理。ライブ映像、アラート、設定ができます。',
dlBtn:'アプリをダウンロード',l3wifi:'WiFiに接続',l3name:'カメラ名',
l3pass:'デバイスパスワード',l3hint:'アプリからのアクセスを保護',
scanBtn:'🔍 検索',saveBtn:'💾 保存して再起動',saving:'保存中...',saved:'保存完了！再起動中...',
errWifi:'WiFiネットワークを選択',errName:'カメラ名を入力',errPass:'パスワードを入力',
errSave:'保存エラー',errConn:'接続エラー',wifiPass:'WiFiパスワード',phName:'例: 庭、リビング、ガレージ',phPass:'6文字以上',
instStep1:'アプリのインストール：',instIOS:'<b>共有</b>ボタン □↑ をタップし、<b>"ホーム画面に追加"</b>を選択。アプリがホーム画面に表示されます。',
instAndroid:'<b>3つの点</b> ⋮ をタップし、<b>"ホーム画面に追加"</b>または<b>"アプリをインストール"</b>を選択。アプリがホーム画面に表示されます。',
instDesktop:'アドレスバーの <b>インストールアイコン</b> をクリックするか、<b>Ctrl+Shift+I</b> → "Install CamaraEspia"。'},
ko:{welcome:'환영합니다',sub:'3단계로 카메라 설정',p1text:'어떤 언어로 계속하시겠습니까?',
p2text:'앱을 다운로드하여 스마트폰에서 카메라를 관리하세요. 라이브 영상, 알림 및 전체 설정이 가능합니다.',
dlBtn:'앱 다운로드',l3wifi:'WiFi 연결',l3name:'카메라 이름',
l3pass:'장치 비밀번호',l3hint:'앱에서 이 카메라 접근 보호',
scanBtn:'🔍 검색',saveBtn:'💾 저장 후 재시작',saving:'저장 중...',saved:'저장 완료! 재시작 중...',
errWifi:'WiFi 네트워크 선택',errName:'카메라 이름 입력',errPass:'비밀번호 입력',
errSave:'저장 오류',errConn:'연결 오류',wifiPass:'WiFi 비밀번호',phName:'예: 마당, 거실, 차고',phPass:'최소 6자',
instStep1:'앱 설치 방법:',instIOS:'<b>공유</b> 버튼 □↑을 탭하고 <b>"홈 화면에 추가"</b>를 선택하세요. 앱이 홈 화면에 나타납니다.',
instAndroid:'<b>3개 점</b> ⋮ 을 탭하고 <b>"홈 화면에 추가"</b> 또는 <b>"앱 설치"</b>를 선택하세요. 앱이 홈 화면에 나타납니다.',
instDesktop:'주소 표시줄의 <b>설치 아이콘</b>을 클릭하거나 <b>Ctrl+Shift+I</b> → "Install CamaraEspia".'},
ar:{welcome:'مرحباً',sub:'إعداد الكاميرا في 3 خطوات',p1text:'بأي لغة تريد المتابعة؟',
p2text:'قم بتنزيل التطبيق لإدارة كاميراتك من هاتفك. مشاهدة الفيديو المباشر والتنبيهات والإعدادات.',
dlBtn:'تنزيل التطبيق',l3wifi:'الاتصال بالـ WiFi',l3name:'اسم الكاميرا',
l3pass:'كلمة مرور الجهاز',l3hint:'حماية الوصول للكاميرا من التطبيق',
scanBtn:'🔍 بحث',saveBtn:'💾 حفظ وإعادة تشغيل',saving:'جارٍ الحفظ...',saved:'تم الحفظ! إعادة التشغيل...',
errWifi:'اختر شبكة WiFi',errName:'أدخل اسم الكاميرا',errPass:'أدخل كلمة المرور',
errSave:'خطأ في الحفظ',errConn:'خطأ في الاتصال',wifiPass:'كلمة مرور WiFi',phName:'مثال: فناء، غرفة، مرآب',phPass:'6 أحرف على الأقل',
instStep1:'لتثبيت التطبيق:',instIOS:'اضغط زر <b>مشاركة</b> □↑ واختر <b>"إضافة إلى الشاشة الرئيسية"</b>. سيظهر التطبيق على شاشتك الرئيسية.',
instAndroid:'اضغط <b>3 نقاط</b> ⋮ واختر <b>"إضافة إلى الشاشة الرئيسية"</b> أو <b>"تثبيت التطبيق"</b>. سيظهر التطبيق على شاشتك الرئيسية.',
instDesktop:'انقر على <b>أيقونة التثبيت</b> في شريط العنوان، أو اضغط <b>Ctrl+Shift+I</b> → "Install CamaraEspia".'},
hi:{welcome:'स्वागत है',sub:'3 चरणों में अपना कैमरा सेट करें',p1text:'आप किस भाषा में जारी रखना चाहते हैं?',
p2text:'ऐप डाउनलोड करें और फोन से कैमरे प्रबंधित करें। लाइव वीडियो, अलर्ट और सभी सेटिंग्स।',
dlBtn:'ऐप डाउनलोड करें',l3wifi:'WiFi से कनेक्ट करें',l3name:'कैमरे का नाम',
l3pass:'डिवास पासवर्ड',l3hint:'ऐप से इस कैमरे की पहुंच की सुरक्षा करें',
scanBtn:'🔍 खोजें',saveBtn:'💾 सहेजें और पुनरारंभ करें',saving:'सहेज रहा है...',saved:'सहेजा गया! पुनरारंभ...',
errWifi:'WiFi नेटवर्क चुनें',errName:'कैमरे का नाम दर्ज करें',errPass:'पासवर्ड दर्ज करें',
errSave:'सहेजने में त्रुटि',errConn:'कनेक्शन त्रुटि',wifiPass:'WiFi पासवर्ड',phName:'उदा: आंगन, लिविंग रूम, गैरेज',phPass:'कम से कम 6 अक्षर',
instStep1:'ऐप इंस्टॉल करने के लिए:',instIOS:'<b>शेयर</b> बटन □↑ दबाएं और <b>"होम स्क्रीन पर जोड़ें"</b> चुनें। ऐप आपकी होम स्क्रीन पर दिखाई देगा।',
instAndroid:'<b>3 डॉट्स</b> ⋮ दबाएं और <b>"होम स्क्रीन पर जोड़ें"</b> या <b>"ऐप इंस्टॉल करें"</b> चुनें। ऐप आपकी होम स्क्रीन पर दिखाई देगा।',
instDesktop:'एड्रेस बार में <b>इंस्टॉल आइकन</b> पर क्लिक करें, या <b>Ctrl+Shift+I</b> दबाएं → "Install CamaraEspia"।'},
bn:{welcome:'স্বাগতম',sub:'৩ ধাপে আপনার ক্যামেরা সেট করুন',p1text:'কোন ভাষায় চালিয়ে যেতে চান?',
p2text:'অ্যাপ ডাউনলোড করুন এবং ফোন থেকে ক্যামেরা পরিচালনা করুন। লাইভ ভিডিও, সতর্কতা এবং সব সেটিংস।',
dlBtn:'অ্যাপ ডাউনলোড করুন',l3wifi:'WiFi এ সংযোগ করুন',l3name:'ক্যামেরার নাম',
l3pass:'ডিভাইস পাসওয়ার্ড',l3hint:'অ্যাপ থেকে এই ক্যামেরার অ্যাক্সেস সুরক্ষিত করুন',
scanBtn:'🔍 খুঁজুন',saveBtn:'💾 সংরক্ষণ করুন এবং পুনরায় চালু করুন',saving:'সংরক্ষণ হচ্ছে...',saved:'সংরক্ষিত! পুনরায় চালু হচ্ছে...',
errWifi:'একটি WiFi নেটওয়ার্ক নির্বাচন করুন',errName:'ক্যামেরার নাম লিখুন',errPass:'পাসওয়ার্ড লিখুন',
errSave:'সংরক্ষণে ত্রুটি',errConn:'সংযোগের ত্রুটি',wifiPass:'WiFi পাসওয়ার্ড',phName:'যেমন: প্রাঙ্গন, লিভিং রুম, গ্যারাজ',phPass:'কমপক্ষে ৬ অক্ষর',
instStep1:'অ্যাপ ইনস্টল করতে:',instIOS:'<b>শেয়ার</b> বাটন □↑ ট্যাপ করুন এবং <b>"হোম স্ক্রিনে যোগ করুন"</b> নির্বাচন করুন। অ্যাপ আপনার হোম স্ক্রিনে দেখা যাবে।',
instAndroid:'<b>৩ ডট</b> ⋮ ট্যাপ করুন এবং <b>"হোম স্ক্রিনে যোগ করুন"</b> বা <b>"অ্যাপ ইনস্টল করুন"</b> নির্বাচন করুন। অ্যাপ আপনার হোম স্ক্রিনে দেখা যাবে।',
instDesktop:'অ্যাড্রেস বারে <b>ইনস্টল আইকন</b> ক্লিক করুন, অথবা <b>Ctrl+Shift+I</b> চাপুন → "Install CamaraEspia"।'},
tr:{welcome:'Hoş geldiniz',sub:'Kameranızı 3 adımda kurun',p1text:'Hangi dilde devam etmek istiyorsunuz?',
p2text:'Uygulamayı indirin ve telefonunuzdan kameralarınızı yönetin. Canlı video, uyarılar ve tüm ayarlar.',
dlBtn:'Uygulamayı İndir',l3wifi:'WiFi\'ye Bağlan',l3name:'Kamera Adı',
l3pass:'Cihaz Şifresi',l3hint:'Uygulamadan bu kameraya erişimi koruyun',
scanBtn:'🔍 Tara',saveBtn:'💾 Kaydet ve yeniden başlat',saving:'Kaydediliyor...',saved:'Kaydedildi! Yeniden başlatılıyor...',
errWifi:'Bir WiFi ağı seçin',errName:'Bir kamera adı girin',errPass:'Bir şifre girin',
errSave:'Kaydetme hatası',errConn:'Bağlantı hatası',wifiPass:'WiFi Şifresi',phName:'Örn: Avlu, Oturma odası, Garaj',phPass:'En az 6 karakter',
instStep1:'Uygulamayı yüklemek için:',instIOS:'<b>Paylaş</b> düğmesine □↑ dokunun ve <b>"Ana Ekrana Ekle"</b> seçin. Uygulama ana ekranınızda görünecektir.',
instAndroid:'<b>3 nokta</b> ⋮ a dokunun ve <b>"Ana Ekrana Ekle"</b> veya <b>"Uygulamayı yükle"</b> seçin. Uygulama ana ekranınızda görünecektir.',
instDesktop:'Adres çubuğundaki <b>yükleme simgesine</b> tıklayın veya <b>Ctrl+Shift+I</b> → "Install CamaraEspia".'},
vi:{welcome:'Chào mừng',sub:'Thiết lập camera trong 3 bước',p1text:'Bạn muốn tiếp tục bằng ngôn ngữ nào?',
p2text:'Tải ứng dụng để quản lý camera từ điện thoại. Xem video trực tiếp, nhận cảnh báo và cấu hình tất cả.',
dlBtn:'Tải Ứng Dụng',l3wifi:'Kết nối WiFi',l3name:'Tên camera',
l3pass:'Mật khẩu thiết bị',l3hint:'Bảo vệ quyền truy cập camera từ ứng dụng',
scanBtn:'🔍 Tìm',saveBtn:'💾 Lưu và khởi động lại',saving:'Đang lưu...',saved:'Đã lưu! Đang khởi động lại...',
errWifi:'Chọn mạng WiFi',errName:'Nhập tên camera',errPass:'Nhập mật khẩu',
errSave:'Lỗi lưu',errConn:'Lỗi kết nối',wifiPass:'Mật khẩu WiFi',phName:'VD: Sân, Phòng khách, Nhà xe',phPass:'Ít nhất 6 ký tự',
instStep1:'Cài đặt ứng dụng:',instIOS:'Nhấn nút <b>Chia sẻ</b> □↑ và chọn <b>"Thêm vào màn hình chính"</b>. Ứng dụng sẽ xuất hiện trên màn hình chính.',
instAndroid:'Nhấn <b>3 chấm</b> ⋮ và chọn <b>"Thêm vào màn hình chính"</b> hoặc <b>"Cài đặt ứng dụng"</b>. Ứng dụng sẽ xuất hiện trên màn hình chính.',
instDesktop:'Nhấn vào <b>biểu tượng cài đặt</b> trên thanh địa chỉ, hoặc nhấn <b>Ctrl+Shift+I</b> → "Install CamaraEspia"。'},
th:{welcome:'ยินดีต้อนรับ',sub:'ตั้งค่ากล้องใน 3 ขั้นตอน',p1text:'คุณต้องการใช้ภาษาใด?',
p2text:'ดาวน์โหลดแอปเพื่อจัดการกล้องจากมือถือ ดูวิดีโอสด รับการแจ้งเตือน และตั้งค่าทั้งหมด',
dlBtn:'ดาวน์โหลดแอป',l3wifi:'เชื่อมต่อ WiFi',l3name:'ชื่อกล้อง',
l3pass:'รหัสผ่านอุปกรณ์',l3hint:'ป้องกันการเข้าถึงกล้องจากแอป',
scanBtn:'🔍 ค้นหา',saveBtn:'💾 บันทึกและรีสตาร์ท',saving:'กำลังบันทึก...',saved:'บันทึกแล้ว! กำลังรีสตาร์ท...',
errWifi:'เลือกเครือข่าย WiFi',errName:'ป้อนชื่อกล้อง',errPass:'ป้อนรหัสผ่าน',
errSave:'เกิดข้อผิดพลาดในการบันทึก',errConn:'เกิดข้อผิดพลาดในการเชื่อมต่อ',wifiPass:'รหัสผ่าน WiFi',phName:'เช่น: สนาม, ห้องนั่งเล่น, โรงรถ',phPass:'อย่างน้อย 6 ตัวอักษร',
instStep1:'ติดตั้งแอป:',instIOS:'แตะปุ่ม <b>แชร์</b> □↑ แล้วเลือก <b>"เพิ่มลงหน้าจอหลัก"</b> แอปจะปรากฏบนหน้าจอหลักของคุณ',
instAndroid:'แตะ <b>3 จุด</b> ⋮ แล้วเลือก <b>"เพิ่มลงหน้าจอหลัก"</b> หรือ <b>"ติดตั้งแอป"</b> แอปจะปรากฏบนหน้าจอหลักของคุณ',
instDesktop:'คลิก <b>ไอคอนติดตั้ง</b> ในแถบ地址 หรือกด <b>Ctrl+Shift+I</b> → "Install CamaraEspia".'},
id:{welcome:'Selamat datang',sub:'Siapkan kamera Anda dalam 3 langkah',p1text:'Bahasa apa yang ingin Anda gunakan?',
p2text:'Unduh aplikasi untuk mengelola kamera dari ponsel. Akses video langsung, terima peringatan, dan konfigurasi semuanya.',
dlBtn:'Unduh Aplikasi',l3wifi:'Hubungkan ke WiFi',l3name:'Nama Kamera',
l3pass:'Kata Sandi Perangkat',l3hint:'Lindungi akses ke kamera ini dari aplikasi',
scanBtn:'🔍 Cari',saveBtn:'💾 Simpan dan mulai ulang',saving:'Menyimpan...',saved:'Tersimpan! Memulai ulang...',
errWifi:'Pilih jaringan WiFi',errName:'Masukkan nama kamera',errPass:'Masukkan kata sandi',
errSave:'Gagal menyimpan',errConn:'Kesalahan koneksi',wifiPass:'Kata Sandi WiFi',phName:'Contoh: Teras, Ruang tamu, Garasi',phPass:'Minimal 6 karakter',
instStep1:'Untuk menginstal app:',instIOS:'Ketuk tombol <b>Bagikan</b> □↑ dan pilih <b>"Tambah ke Layar Utama"</b>. App akan muncul di layar utama Anda.',
instAndroid:'Ketuk <b>3 titik</b> ⋮ dan pilih <b>"Tambah ke Layar Utama"</b> atau <b>"Instal app"</b>. App akan muncul di layar utama Anda.',
instDesktop:'Klik <b>ikon instal</b> di bilah alamat, atau tekan <b>Ctrl+Shift+I</b> → "Install CamaraEspia".'},
pl:{welcome:'Witamy',sub:'Skonfiguruj kamerę w 3 krokach',p1text:'W jakim języku chcesz kontynuować?',
p2text:'Pobierz aplikację, aby zarządzać kamerami z telefonu. Podgląd na żywo, alerty i wszystkie ustawienia.',
dlBtn:'Pobierz Aplikację',l3wifi:'Połącz z WiFi',l3name:'Nazwa kamery',
l3pass:'Hasło urządzenia',l3hint:'Chroń dostęp do kamery z aplikacji',
scanBtn:'🔍 Szukaj',saveBtn:'💾 Zapisz i uruchom ponownie',saving:'Zapisywanie...',saved:'Zapisano! Ponowne uruchamianie...',
errWifi:'Wybierz sieć WiFi',errName:'Wprowadź nazwę kamery',errPass:'Wprowadź hasło',
errSave:'Błąd zapisu',errConn:'Błąd połączenia',wifiPass:'Hasło WiFi',phName:'Np: Podwórko, Salon, Garaż',phPass:'Co najmniej 6 znaków',
instStep1:'Aby zainstalować aplikację:',instIOS:'Stuknij przycisk <b>Udostępnij</b> □↑ i wybierz <b>"Dodaj do ekranu głównego"</b>. Aplikacja pojawi się na ekranie głównym.',
instAndroid:'Stuknij <b>3 kropki</b> ⋮ i wybierz <b>"Dodaj do ekranu głównego"</b> lub <b>"Zainstaluj aplikację"</b>. Aplikacja pojawi się na ekranie głównym.',
instDesktop:'Kliknij <b>ikonę instalacji</b> na pasku adresu lub naciśnij <b>Ctrl+Shift+I</b> → "Install CamaraEspia".'},
uk:{welcome:'Ласкаво просимо',sub:'Налаштуйте камеру за 3 кроки',p1text:'Якою мовою ви хочете продовжити?',
p2text:'Завантажте додаток для керування камерами з телефону. Відео в реальному часі, сповіщення та налаштування.',
dlBtn:'Завантажити App',l3wifi:'Підключитись до WiFi',l3name:'Назва камери',
l3pass:'Пароль пристрою',l3hint:'Захистіть доступ до камери з додатку',
scanBtn:'🔍 Пошук',saveBtn:'💾 Зберегти та перезавантажити',saving:'Збереження...',saved:'Збережено! Перезавантаження...',
errWifi:'Виберіть мережу WiFi',errName:'Введіть назву камери',errPass:'Введіть пароль',
errSave:'Помилка збереження',errConn:'Помилка з\'єднання',wifiPass:'Пароль WiFi',phName:'Напр: Двір, Кімната, Гараж',phPass:'Мінімум 6 символів',
instStep1:'Для встановлення додатку:',instIOS:'Натисніть <b>Поділитися</b> □↑ та оберіть <b>"На екран Додому"</b>. Додаток з\'явиться на головному екрані.',
instAndroid:'Натисніть <b>3 крапки</b> ⋮ та оберіть <b>"На екран Додому"</b> або <b>"Встановити додаток"</b>. Додаток з\'явиться на головному екрані.',
instDesktop:'Натисніть <b>іконку встановлення</b> в адресному рядку або <b>Ctrl+Shift+I</b> → "Install CamaraEspia".'},
ms:{welcome:'Selamat datang',sub:'Sediakan kamera anda dalam 3 langkah',p1text:'Bahasa apa yang anda ingin teruskan?',
p2text:'Muat turun aplikasi untuk mengurus kamera dari telefon. Akses video langsung, terima amaran dan konfigurasikan semuanya.',
dlBtn:'Muat Turun App',l3wifi:'Sambung ke WiFi',l3name:'Nama Kamera',
l3pass:'Kata Laluan Peranti',l3hint:'Lindungi akses ke kamera ini dari aplikasi',
scanBtn:'🔍 Cari',saveBtn:'💾 Simpan dan mulakan semula',saving:'Menyimpan...',saved:'Disimpan! Memulakan semula...',
errWifi:'Pilih rangkaian WiFi',errName:'Masukkan nama kamera',errPass:'Masukkan kata laluan',
errSave:'Ralat menyimpan',errConn:'Ralat sambungan',wifiPass:'Kata Laluan WiFi',phName:'Contoh: Halaman, Ruang tamu, Garage',phPass:'Minimal 6 aksara',
instStep1:'Untuk memasang app:',instIOS:'Ketik butang <b>Kongsi</b> □↑ dan pilih <b>"Tambah ke Skrin Utama"</b>. App akan muncul di skrin utama anda.',
instAndroid:'Ketik <b>3 titik</b> ⋮ dan pilih <b>"Tambah ke Skrin Utama"</b> atau <b>"Pasang app"</b>. App akan muncul di skrin utama anda.',
instDesktop:'Klik <b>ikon pasang</b> pada bar alamat, atau tekan <b>Ctrl+Shift+I</b> → "Install CamaraEspia".'},
sw:{welcome:'Karibu',sub:'Weka kamera yako hatua 3',p1text:'Lugha gani unataka kuendelea nayo?',
p2text:'Pakua programu ili kusimamia kamera yako kutoka simu. Ufikiaji wa video moja kwa moja, arifa na usanidi wote.',
dlBtn:'Pakua App',l3wifi:'Unganisha na WiFi',l3name:'Jina la Kamera',
l3pass:'Nenosiri la Kifaa',l3hint:'Linda upatikanaji wa kamera hii kutoka programu',
scanBtn:'🔍 Tafuta',saveBtn:'💾 Hifadhi na upya upya',saving:'Inahifadhiwa...',saved:'Imehifadhiwa! Inapya upya...',
errWifi:'Chagua mtandao wa WiFi',errName:'Weka jina la kamera',errPass:'Weka nenosiri',
errSave:'Hitilafu ya kuhifadhi',errConn:'Hitilafu ya muunganisho',wifiPass:'Nenosiri ya WiFi',phName:'Mf: ua, sebule, garage',phPass:'Herufi 6 angalavo',
instStep1:'Kwa kusakinisha app:',instIOS:'Gusa kitufe cha <b>Kushiriki</b> □↑ na uchague <b>"Ongeza kwenye Skrini ya Nyumbani"</b>. App itaonekana kwenye skrini yako ya nyumbani.',
instAndroid:'Gusa <b>pointi 3</b> ⋮ na uchague <b>"Ongeza kwenye Skrini ya Nyumbani"</b> au <b>"Sakinisha app"</b>. App itaonekana kwenye skrini yako ya nyumbani.',
instDesktop:'Bonyeza <b>ikoni ya kusakinisha</b> kwenye upau wa anwani, au bonyeza <b>Ctrl+Shift+I</b> → "Install CamaraEspia".'},
tl:{welcome:'Maligayahan',sub:'I-setup ang camera sa 3 hakbang',p1text:'Anong wika mo gustong gamitin?',
p2text:'I-download ang app para pamahalaan ang mga camera mula sa phone. Live video, alerto at lahat ng settings.',
dlBtn:'I-download ang App',l3wifi:'Kumonekta sa WiFi',l3name:'Pangalan ng Camera',
l3pass:'Password ng Device',l3hint:'Protektahan ang access sa camera mula sa app',
scanBtn:'🔍 Maghanap',saveBtn:'💾 I-save at i-restart',saving:'Sinasave...',saved:'Na-save! Nire-restart...',
errWifi:'Pumili ng WiFi network',errName:'Ilagay ang pangalan ng camera',errPass:'Ilagay ang password',
errSave:'Error sa pag-save',errConn:'Error sa koneksyon',wifiPass:'WiFi Password',phName:'Hal: Patio, Living room, Garage',phPass:'Minimal 6 karakter',
instStep1:'Upang i-install ang app:',instIOS:'I-tap ang <b>Share</b> button □↑ at piliin ang <b>"Add to Home Screen"</b>. Ang app ay magpapakita sa iyong home screen.',
instAndroid:'I-tap ang <b>3 dots</b> ⋮ at piliin ang <b>"Add to Home Screen"</b> o <b>"Install app"</b>. Ang app ay magpapakita sa iyong home screen.',
instDesktop:'I-click ang <b>install icon</b> sa address bar, o pindutin ang <b>Ctrl+Shift+I</b> → "Install CamaraEspia".'},
nl:{welcome:'Welkom',sub:'Stel uw camera in 3 stappen in',p1text:'In welke taal wilt u doorgaan?',
p2text:'Download de app om uw camera\'s te beheren vanaf uw telefoon. Live video, meldingen en alle instellingen.',
dlBtn:'Download App',l3wifi:'Verbinden met WiFi',l3name:'Camera Naam',
l3pass:'Apparaat Wachtwoord',l3hint:'Bescherm toegang tot deze camera vanuit de app',
scanBtn:'🔍 Zoeken',saveBtn:'💾 Opslaan en herstarten',saving:'Opslaan...',saved:'Opgeslagen! Herstarten...',
errWifi:'Selecteer een WiFi-netwerk',errName:'Voer een cameranaam in',errPass:'Voer een wachtwoord in',
errSave:'Fout bij opslaan',errConn:'Verbindingsfout',wifiPass:'WiFi Wachtwoord',phName:'Bv: Tuin, Woonkamer, Garage',phPass:'Minimaal 6 tekens',
instStep1:'Om de app te installeren:',instIOS:'Tik op de <b>Delen</b>-knop □↑ en selecteer <b>"Toevoegen aan beginscherm"</b>. De app verschijnt op je beginscherm.',
instAndroid:'Tik op de <b>3 puntjes</b> ⋮ en selecteer <b>"Toevoegen aan beginscherm"</b> of <b>"App installeren"</b>. De app verschijnt op je beginscherm.',
instDesktop:'Klik op het <b>installatie-icoon</b> in de adresbalk, of druk op <b>Ctrl+Shift+I</b> → "Install CamaraEspia".'}
};

// Build language grid
var grid=document.getElementById('langGrid');
langs.forEach(function(item){
    var b=document.createElement('div');
    b.className='lang-btn';
    b.dataset.lang=item.l;
    b.innerHTML='<span class="flag">'+item.f+'</span>'+item.n;
    b.onclick=function(){selectLang(item.l)};
    grid.appendChild(b);
});

function selectLang(l){
    curLang=l;
    S=T[l];
    document.querySelectorAll('.lang-btn').forEach(function(b){
        b.classList.toggle('selected',b.dataset.lang===l);
    });
    document.getElementById('title').textContent='📷 CameraEspia';
    document.getElementById('subtitle').textContent=S.sub;
    document.getElementById('p1text').textContent=S.p1text;
    document.getElementById('p2text').textContent=S.p2text;
    document.getElementById('dlBtnText').textContent=S.dlBtn;
    document.getElementById('l3wifi').textContent=S.l3wifi;
    document.getElementById('l3name').textContent=S.l3name;
    document.getElementById('l3pass').textContent=S.l3pass;
    document.getElementById('l3hint').textContent=S.l3hint;
    document.getElementById('wifiPass').placeholder=S.wifiPass;
    document.getElementById('camName').placeholder=S.phName;
    document.getElementById('devPass').placeholder=S.phPass;
    document.getElementById('saveBtn').textContent=S.saveBtn;
    document.getElementById('scanBtn').textContent=S.scanBtn;
    document.getElementById('wifiPass').previousElementSibling; // noop
    // Go to step 2
    document.getElementById('p1').classList.remove('active');
    document.getElementById('p2').classList.add('active');
}

function goStep3(){
    document.getElementById('p2').classList.remove('active');
    document.getElementById('p3').classList.add('active');
    scanWifi();
}

function installPWA(){
    if(window.deferredPrompt){
        window.deferredPrompt.prompt();
        window.deferredPrompt.userChoice.then(function(){
            window.deferredPrompt=null;
            goStep3();
        });
    }else{
        goStep3();
    }
}

function scanWifi(){
    var sel=document.getElementById('wifiList');
    sel.innerHTML='<option value="">'+(S.scanBtn||'Scan')+'...</option>';
    document.getElementById('scanBtn').disabled=true;
    fetch('/api/wifi/scan').then(function(r){return r.json()}).then(function(list){
        if(!list.length){
            // Scan async en progreso — reintentar en 3s
            setTimeout(function(){scanWifi()},3000);
            return;
        }
        sel.innerHTML='<option value="">'+(S.l3wifi||'WiFi')+'</option>';
        list.forEach(function(n){
            var o=document.createElement('option');
            o.value=n.ssid;
            o.textContent=n.ssid+' ('+n.rssi+'dB)';
            sel.appendChild(o);
        });
        document.getElementById('scanBtn').disabled=false;
    }).catch(function(){
        sel.innerHTML='<option value="">Error</option>';
        document.getElementById('scanBtn').disabled=false;
    });
}

function saveAll(){
    var ssid=document.getElementById('wifiList').value;
    var pass=document.getElementById('wifiPass').value;
    var name=document.getElementById('camName').value.trim();
    var devp=document.getElementById('devPass').value;
    if(!ssid){showSt(S.errWifi,1);return;}
    if(!name){showSt(S.errName,1);return;}
    if(devp.length<6){showSt(S.errPass,1);return;}
    var btn=document.getElementById('saveBtn');
    btn.disabled=true;
    btn.innerHTML='<span class="spinner"></span>'+S.saving;
    var fd=new FormData();
    fd.append('ssid',ssid);
    fd.append('password',pass);
    fd.append('camera_name',name);
    fd.append('device_password',devp);
    fd.append('language',curLang);
    fetch('/api/save',{method:'POST',body:fd}).then(function(r){return r.json()}).then(function(d){
        if(d.ok){showSt(S.saved,0);}
        else{showSt(S.errSave,1);btn.disabled=false;btn.textContent=S.saveBtn;}
    }).catch(function(){showSt(S.errConn,1);btn.disabled=false;btn.textContent=S.saveBtn;});
}

function showSt(msg,err){
    var st=document.getElementById('status');
    st.className='status '+(err?'err':'ok');
    st.textContent=msg;
    st.style.display='block';
}

// Capture PWA install prompt
window.deferredPrompt=null;
window.addEventListener('beforeinstallprompt',function(e){
    e.preventDefault();
    window.deferredPrompt=e;
});
</script>
</body>
</html>
)rawliteral";
}
