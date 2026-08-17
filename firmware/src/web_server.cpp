#include "web_server.h"
#include "pwa_data.h"
#include "camera_manager.h"
#include "servo_manager.h"
#include "sd_manager.h"
#include "battery_manager.h"
#include "pir_manager.h"
#include "telegram_bot.h"
#include <WiFi.h>

WebServer::WebServer() : _server(nullptr),
    _camera(nullptr), _servo(nullptr), _sd(nullptr),
    _battery(nullptr), _pir(nullptr), _telegram(nullptr) {
}

bool WebServer::begin(CameraManager* cam, ServoManager* servo, SDManager* sd,
                      BatteryMonitor* bat, PIRManager* pir, TelegramBot* tg) {
    _camera = cam;
    _servo = servo;
    _sd = sd;
    _battery = bat;
    _pir = pir;
    _telegram = tg;

    _server = new AsyncWebServer(80);
    setupRoutes();
    _server->begin();
    Serial.println("[WEB] Servidor web en puerto 80");
    return true;
}

void WebServer::update() {
    // AsyncWebServer no necesita update
}

void WebServer::setupRoutes() {
    // Pagina principal — PWA
    _server->on("/", HTTP_GET, [this](AsyncWebServerRequest* request) {
        handlePWAIndex(request);
    });

    // API existentes
    _server->on("/api/capture", HTTP_GET, [this](AsyncWebServerRequest* request) {
        handleCapture(request);
    });

    _server->on("/api/config", HTTP_GET, [this](AsyncWebServerRequest* request) {
        handleConfigGet(request);
    });

    _server->on("/api/config", HTTP_POST,
        [this](AsyncWebServerRequest* request) {
            handleConfigSet(request);
        }
    );

    // WiFi scan
    _server->on("/api/wifi/scan", HTTP_GET, [this](AsyncWebServerRequest* request) {
        handleWifiScan(request);
    });

    // Telegram config
    _server->on("/api/telegram", HTTP_GET, [this](AsyncWebServerRequest* request) {
        handleTelegramGet(request);
    });

    _server->on("/api/telegram", HTTP_POST,
        [this](AsyncWebServerRequest* request) {
            if (!request->hasParam("body", true)) {
                request->send(400, "application/json", "{\"error\":\"No body\"}");
                return;
            }
            String body = request->getParam("body", true)->value();
            JsonDocument doc;
            if (deserializeJson(doc, body)) {
                request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
                return;
            }

            int64_t chatId = doc["owner_chat_id"] | 0LL;
            String staSsid = doc["sta_ssid"] | "";
            String staPass = doc["sta_pass"] | "";
            String camName = doc["camera_name"] | "";

            Preferences p;
            p.begin("camara", false);
            if (chatId != 0) {
                p.putLong64("owner_chat_id", chatId);
            }
            if (staSsid.length() > 0) {
                p.putString("wifi_ssid", staSsid);
                p.putString("wifi_pass", staPass);
            }
            if (camName.length() > 0) {
                p.putString("camera_name", camName);
            }
            p.end();

            Serial.printf("[WEB] Config guardada: chatId=%lld, ssid=%s\n",
                          chatId, staSsid.c_str());

            JsonDocument resp;
            resp["success"] = true;
            resp["chat_id"] = chatId;
            resp["wifi"] = staSsid.length() > 0;
            resp["reboot"] = true;
            String response;
            serializeJson(resp, response);
            request->send(200, "application/json", response);

            delay(500);
            ESP.restart();
        }
    );

    // Reset
    _server->on("/api/reset", HTTP_POST, [this](AsyncWebServerRequest* request) {
        handleReset(request);
    });

    // Camera info (PWA discovery)
    _server->on("/api/camera/info", HTTP_GET, [this](AsyncWebServerRequest* request) {
        handleCameraInfo(request);
    });

    // Camera status
    _server->on("/api/camera/status", HTTP_GET, [this](AsyncWebServerRequest* request) {
        handleCameraStatus(request);
    });

    // PWA static files
    _server->on("/index.html", HTTP_GET, [this](AsyncWebServerRequest* request) {
        handlePWAIndex(request);
    });

    _server->on("/css/style.css", HTTP_GET, [this](AsyncWebServerRequest* request) {
        handlePWACss(request);
    });

    _server->on("/sw.js", HTTP_GET, [this](AsyncWebServerRequest* request) {
        handlePWASw(request);
    });

    _server->on("/manifest.json", HTTP_GET, [this](AsyncWebServerRequest* request) {
        handlePWAManifest(request);
    });

    _server->on("/js/i18n.js", HTTP_GET, [this](AsyncWebServerRequest* request) {
        handlePWAI18n(request);
    });

    _server->on("/js/discovery.js", HTTP_GET, [this](AsyncWebServerRequest* request) {
        handlePWADiscovery(request);
    });

    _server->on("/js/camera.js", HTTP_GET, [this](AsyncWebServerRequest* request) {
        handlePWACamera(request);
    });

    _server->on("/js/settings.js", HTTP_GET, [this](AsyncWebServerRequest* request) {
        handlePWASettings(request);
    });

    _server->on("/js/app.js", HTTP_GET, [this](AsyncWebServerRequest* request) {
        handlePWAApp(request);
    });

    _server->on("/icons/icon-192.png", HTTP_GET, [this](AsyncWebServerRequest* request) {
        handlePWAIcon192(request);
    });

    _server->on("/icons/icon-512.png", HTTP_GET, [this](AsyncWebServerRequest* request) {
        handlePWAIcon512(request);
    });

    // Fallback: serve PWA index for any unknown path (SPA routing)
    _server->onNotFound([this](AsyncWebServerRequest* request) {
        handlePWAIndex(request);
    });
}

void WebServer::handleRoot(AsyncWebServerRequest* request) {
    String html = R"rawliteral(
<!DOCTYPE html>
<html lang="es">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Camara Espia - Configuracion</title>
<style>
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:-apple-system,sans-serif;background:#0f0f0f;color:#e0e0e0;min-height:100vh;display:flex;justify-content:center;align-items:center}
.card{background:#1a1a1a;border-radius:16px;padding:32px;max-width:400px;width:90%;box-shadow:0 8px 32px rgba(0,0,0,.5)}
h1{text-align:center;font-size:24px;margin-bottom:8px}
.subtitle{text-align:center;color:#888;margin-bottom:24px;font-size:14px}
.field{margin-bottom:20px}
.field label{display:block;font-size:13px;color:#aaa;margin-bottom:6px}
.field input,.field select{width:100%;padding:12px;border-radius:8px;border:1px solid #333;background:#0f0f0f;color:#fff;font-size:15px}
.field input:focus{outline:none;border-color:#4CAF50}
.btn{width:100%;padding:14px;border:none;border-radius:10px;font-size:16px;font-weight:600;cursor:pointer;margin-top:8px}
.btn-primary{background:#4CAF50;color:#fff}
.btn-primary:hover{background:#45a049}
.btn-primary:disabled{background:#333;color:#666;cursor:not-allowed}
.status{margin-top:16px;padding:12px;border-radius:8px;font-size:13px;display:none}
.status.ok{display:block;background:#1b3a1b;color:#4CAF50;border:1px solid #2d5a2d}
.status.err{display:block;background:#3a1b1b;color:#f44336;border:1px solid #5a2d2d}
.spinner{display:inline-block;width:16px;height:16px;border:2px solid #fff;border-top-color:transparent;border-radius:50%;animation:spin .6s linear infinite;margin-right:8px;vertical-align:middle}
@keyframes spin{to{transform:rotate(360deg)}}
#scanBtn{background:#333;color:#aaa;border:1px solid #444;padding:8px 12px;border-radius:6px;cursor:pointer;font-size:13px;margin-top:4px}
#scanBtn:hover{background:#444}
</style>
</head>
<body>
<div class="card">
<h1>📷 Configurar Camara</h1>
<p class="subtitle" id="subtitle">Vincula tu camara con Telegram</p>

<!-- PASO 1: WiFi + Nombre (visible si no hay WiFi configurado) -->
<div id="step1">
<div class="field">
<label>Paso 1: Conectar a tu WiFi</label>
<div style="display:flex;gap:8px;align-items:center">
<select id="wifiList"><option value="">Buscando redes...</option></select>
<button id="scanBtn" onclick="scanWifi()">🔍</button>
</div>
<input type="password" id="wifiPass" placeholder="Contraseña WiFi">
</div>
<div class="field">
<label>Nombre de esta camara</label>
<input type="text" id="camName" placeholder="Ej: Patio, Sala, Garage...">
</div>
<button class="btn btn-primary" id="saveBtn1" onclick="saveStep1()">
💾 Conectar y reiniciar
</button>
</div>

<!-- PASO 2: Chat ID (visible si ya hay WiFi pero no chat_id) -->
<div id="step2" style="display:none">
<div class="field">
<label>Paso 2: Vincular tu Telegram</label>
<p style="font-size:13px;color:#aaa;margin-bottom:10px;line-height:1.5">
Ahora que la camara tiene internet, abre Telegram y escribe <b>/start</b> a <b>@Camara_Espia_SAEL_bot</b>.
El bot te respondera con tu Chat ID. Copialo y pegalo aqui:
</p>
<input type="text" id="chatId" placeholder="Tu Chat ID">
</div>
<button class="btn btn-primary" id="saveBtn2" onclick="saveStep2()">
🔗 Vincular y reiniciar
</button>
</div>

<!-- PASO 3: Ya vinculado -->
<div id="step3" style="display:none">
<div style="text-align:center;padding:20px 0">
<div style="font-size:48px;margin-bottom:12px">✅</div>
<p style="font-size:16px;color:#4CAF50;font-weight:600">Camara vinculada</p>
<p style="font-size:13px;color:#888;margin-top:8px" id="linkedInfo"></p>
</div>
</div>

<div id="status" class="status"></div>
</div>

<script>
var sel=document.getElementById('wifiList');
var pass=document.getElementById('wifiPass');
var chatId=document.getElementById('chatId');
var st=document.getElementById('status');
var camName=document.getElementById('camName');

// Cargar estado actual
fetch('/api/telegram').then(r=>r.json()).then(d=>{
    if(d.camera_name) camName.value=d.camera_name;
    if(d.chat_id) chatId.value=d.chat_id;

    if(d.wifi_ssid && d.linked){
        // Paso 3: todo configurado
        document.getElementById('step1').style.display='none';
        document.getElementById('step2').style.display='none';
        document.getElementById('step3').style.display='block';
        document.getElementById('linkedInfo').textContent=
            'WiFi: '+d.wifi_ssid+' | Chat ID: '+d.chat_id;
        document.getElementById('subtitle').textContent='Todo configurado';
    } else if(d.wifi_ssid){
        // Paso 2: WiFi listo, falta chat_id
        document.getElementById('step1').style.display='none';
        document.getElementById('step2').style.display='block';
        document.getElementById('subtitle').textContent='Paso 2: Vincular Telegram';
        document.getElementById('step2').querySelector('label').textContent=
            'WiFi: '+d.wifi_ssid+' | '+camName.value;
    }
    // Si no hay WiFi, muestra paso 1 (default)
});

function scanWifi(){
    sel.innerHTML='<option value="">Escaneando...</option>';
    document.getElementById('scanBtn').disabled=true;
    fetch('/api/wifi/scan').then(r=>r.json()).then(lists=>{
        sel.innerHTML='<option value="">Selecciona red...</option>';
        lists.forEach(function(n){
            var o=document.createElement('option');
            o.value=n.ssid;
            o.textContent=n.ssid+' ('+n.rssi+'dB)';
            sel.appendChild(o);
        });
        document.getElementById('scanBtn').disabled=false;
    }).catch(function(){
        sel.innerHTML='<option value="">Error al escanear</option>';
        document.getElementById('scanBtn').disabled=false;
    });
}
scanWifi();

function saveStep1(){
    var ssid=sel.value;
    var p=pass.value;
    var name=camName.value.trim();
    if(!ssid){showSt('Selecciona una red WiFi',1);return;}
    if(!name){showSt('Ingresa un nombre para la camara',1);return;}
    var btn=document.getElementById('saveBtn1');
    btn.disabled=true;
    btn.innerHTML='<span class="spinner"></span>Guardando...';
    fetch('/api/telegram',{
        method:'POST',
        headers:{'Content-Type':'application/json'},
        body:JSON.stringify({sta_ssid:ssid,sta_pass:p,camera_name:name})
    }).then(r=>r.json()).then(d=>{
        if(d.success){
            showSt('Guardado! Reiniciando... Ahora la camara se conectara a tu WiFi.',0);
        }else{
            showSt('Error al guardar',1);
            btn.disabled=false;
            btn.innerHTML='💾 Conectar y reiniciar';
        }
    }).catch(function(){
        showSt('Error de conexion',1);
        btn.disabled=false;
        btn.innerHTML='💾 Conectar y reiniciar';
    });
}

function saveStep2(){
    var cid=parseInt(chatId.value);
    if(!cid){showSt('Ingresa tu Chat ID',1);return;}
    var btn=document.getElementById('saveBtn2');
    btn.disabled=true;
    btn.innerHTML='<span class="spinner"></span>Vinculando...';
    fetch('/api/telegram',{
        method:'POST',
        headers:{'Content-Type':'application/json'},
        body:JSON.stringify({owner_chat_id:cid})
    }).then(r=>r.json()).then(d=>{
        if(d.success){
            showSt('Vinculada! Reiniciando...',0);
        }else{
            showSt('Error al vincular',1);
            btn.disabled=false;
            btn.innerHTML='🔗 Vincular y reiniciar';
        }
    }).catch(function(){
        showSt('Error de conexion',1);
        btn.disabled=false;
        btn.innerHTML='🔗 Vincular y reiniciar';
    });
}

function showSt(msg,err){
    st.className='status '+(err?'err':'ok');
    st.textContent=msg;
    st.style.display='block';
}
</script>
</body>
</html>
)rawliteral";
    request->send(200, "text/html", html);
}

void WebServer::handleCapture(AsyncWebServerRequest* request) {
    if (!_camera) {
        request->send(503, "application/json", "{\"error\":\"Camera not available\"}");
        return;
    }

    bool wasInit = _camera->isInitialized();
    if (!wasInit) _camera->reinit();
    if (_camera->isStandby()) _camera->wake();

    if (!_camera->isInitialized()) {
        request->send(500, "application/json", "{\"error\":\"Camera init failed\"}");
        if (!wasInit) _camera->deinit();
        return;
    }

    size_t len = 0;
    uint8_t* buf = _camera->captureJPEG(&len);

    if (!wasInit && _camera->isInitialized()) _camera->deinit();

    if (buf && len > 0) {
        // Copiar a buffer malloc para que AsyncWebServer pueda liberar
        uint8_t* copy = (uint8_t*)malloc(len);
        if (copy) {
            memcpy(copy, buf, len);
            free(buf);
            AsyncWebServerResponse* response = request->beginResponse(
                "image/jpeg", len,
                [copy, len](uint8_t* buffer, size_t maxLen, size_t index) -> size_t {
                    size_t toWrite = min(len - index, maxLen);
                    memcpy(buffer, copy + index, toWrite);
                    if (index + toWrite >= len) free(copy);
                    return toWrite;
                });
            response->addHeader("Access-Control-Allow-Origin", "*");
            request->send(response);
        } else {
            request->send(500, "application/json", "{\"error\":\"No memory\"}");
            free(buf);
        }
    } else {
        request->send(500, "application/json", "{\"error\":\"Capture failed\"}");
    }
}

void WebServer::handleConfigGet(AsyncWebServerRequest* request) {
    JsonDocument doc;
    doc["rec_mode"] = "unknown";

    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
}

void WebServer::handleConfigSet(AsyncWebServerRequest* request) {
    request->send(200, "application/json", "{\"success\":true}");
}

void WebServer::handleWifiScan(AsyncWebServerRequest* request) {
    int n = WiFi.scanNetworks();
    JsonDocument doc;
    JsonArray arr = doc.to<JsonArray>();

    for (int i = 0; i < n; i++) {
        JsonObject net = arr.add<JsonObject>();
        net["ssid"] = WiFi.SSID(i);
        net["rssi"] = WiFi.RSSI(i);
        net["enc"] = WiFi.encryptionType(i) != WIFI_AUTH_OPEN;
    }
    WiFi.scanDelete();

    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
}

void WebServer::handleTelegramGet(AsyncWebServerRequest* request) {
    if (!_telegram) {
        request->send(503, "application/json", "{\"error\":\"Telegram not available\"}");
        return;
    }

    Preferences p;
    p.begin("camara", true);
    int64_t ownerChatId = p.getLong64("owner_chat_id", 0);
    String staSsid = p.getString("wifi_ssid", "");
    String camName = p.getString("camera_name", "");
    p.end();

    JsonDocument doc;
    doc["configured"] = true;
    doc["linked"] = _telegram->isLinked();
    doc["chat_id"] = ownerChatId;
    doc["wifi_ssid"] = staSsid;
    doc["camera_name"] = camName;

    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
}

void WebServer::handleReset(AsyncWebServerRequest* request) {
    request->send(200, "application/json", "{\"success\":true,\"reboot\":true}");
    delay(500);
    ESP.restart();
}

// ===== PWA STATIC FILES =====

void WebServer::handlePWAIndex(AsyncWebServerRequest* request) {
    AsyncWebServerResponse* response = request->beginResponse_P(200, "text/html", PWA_INDEX_HTML);
    response->addHeader("Cache-Control", "no-cache");
    request->send(response);
}

void WebServer::handlePWACss(AsyncWebServerRequest* request) {
    AsyncWebServerResponse* response = request->beginResponse_P(200, "text/css", PWA_STYLE_CSS);
    response->addHeader("Cache-Control", "public, max-age=86400");
    request->send(response);
}

void WebServer::handlePWASw(AsyncWebServerRequest* request) {
    AsyncWebServerResponse* response = request->beginResponse_P(200, "application/javascript", PWA_SW_JS);
    response->addHeader("Cache-Control", "no-cache");
    request->send(response);
}

void WebServer::handlePWAManifest(AsyncWebServerRequest* request) {
    AsyncWebServerResponse* response = request->beginResponse_P(200, "application/json", PWA_MANIFEST_JSON);
    response->addHeader("Cache-Control", "public, max-age=86400");
    request->send(response);
}

void WebServer::handlePWAI18n(AsyncWebServerRequest* request) {
    AsyncWebServerResponse* response = request->beginResponse_P(200, "application/javascript", PWA_I18N_JS);
    response->addHeader("Cache-Control", "public, max-age=86400");
    request->send(response);
}

void WebServer::handlePWADiscovery(AsyncWebServerRequest* request) {
    AsyncWebServerResponse* response = request->beginResponse_P(200, "application/javascript", PWA_DISCOVERY_JS);
    response->addHeader("Cache-Control", "public, max-age=86400");
    request->send(response);
}

void WebServer::handlePWACamera(AsyncWebServerRequest* request) {
    AsyncWebServerResponse* response = request->beginResponse_P(200, "application/javascript", PWA_CAMERA_JS);
    response->addHeader("Cache-Control", "public, max-age=86400");
    request->send(response);
}

void WebServer::handlePWASettings(AsyncWebServerRequest* request) {
    AsyncWebServerResponse* response = request->beginResponse_P(200, "application/javascript", PWA_SETTINGS_JS);
    response->addHeader("Cache-Control", "public, max-age=86400");
    request->send(response);
}

void WebServer::handlePWAApp(AsyncWebServerRequest* request) {
    AsyncWebServerResponse* response = request->beginResponse_P(200, "application/javascript", PWA_APP_JS);
    response->addHeader("Cache-Control", "public, max-age=86400");
    request->send(response);
}

void WebServer::handlePWAIcon192(AsyncWebServerRequest* request) {
    AsyncWebServerResponse* response = request->beginResponse_P(200, "image/png", PWA_ICON_192, PWA_ICON_192_SIZE);
    response->addHeader("Cache-Control", "public, max-age=604800");
    request->send(response);
}

void WebServer::handlePWAIcon512(AsyncWebServerRequest* request) {
    AsyncWebServerResponse* response = request->beginResponse_P(200, "image/png", PWA_ICON_512, PWA_ICON_512_SIZE);
    response->addHeader("Cache-Control", "public, max-age=604800");
    request->send(response);
}

// ===== CAMERA INFO / STATUS (PWA) =====

void WebServer::handleCameraInfo(AsyncWebServerRequest* request) {
    Preferences p;
    p.begin("camara", true);
    String camName = p.getString("camera_name", "CamaraEspia");
    p.end();

    JsonDocument doc;
    doc["name"] = camName;
    doc["model"] = "ESP32-S3";
    doc["firmware"] = "CamaraEspia v1.0";
    doc["mac"] = WiFi.macAddress();
    doc["ip"] = WiFi.localIP().toString();
    doc["uptime_ms"] = millis();

    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
}

void WebServer::handleCameraStatus(AsyncWebServerRequest* request) {
    JsonDocument doc;

    // WiFi
    doc["wifi_ssid"] = WiFi.SSID();
    doc["wifi_signal"] = WiFi.RSSI();
    doc["wifi_ip"] = WiFi.localIP().toString();

    // Battery
    if (_battery) {
        doc["battery"] = _battery->getPercentage();
        doc["battery_voltage"] = _battery->readVoltage();
    } else {
        doc["battery"] = nullptr;
    }

    // Camera
    doc["camera_init"] = _camera ? _camera->isInitialized() : false;
    doc["camera_standby"] = _camera ? _camera->isStandby() : false;

    // SD
    doc["sd_present"] = _sd ? _sd->isMounted() : false;

    // PIR
    doc["pir_present"] = _pir ? _pir->isPresent() : false;

    // Telegram
    doc["telegram_linked"] = _telegram ? _telegram->isLinked() : false;

    // Uptime
    doc["uptime_ms"] = millis();

    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
}
