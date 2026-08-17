#include "wifi_manager.h"
#include "config.h"
#include "soc/timer_group_reg.h"

// Tarea continua que mantiene TG1WDT deshabilitado.
// Corre en Core 1 (donde esta loop()) para NO competir con WiFi en Core 0.
static TaskHandle_t tg1wdt_guard_task = NULL;

static void tg1wdt_guard_loop(void* arg) {
    for (;;) {
        REG_WRITE(TIMG_WDTWPROTECT_REG(1), 0xFFFFFFFF);
        REG_WRITE(TIMG_WDTCONFIG0_REG(1), 0);
        REG_WRITE(TIMG_WDTFEED_REG(1), 1);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

WiFiManager::WiFiManager()
    : _active(false), _staActive(false), _clientCb(nullptr), _lastClientCount(0) {
}

void WiFiManager::startTG1Guard() {
    if (tg1wdt_guard_task) return;
    xTaskCreatePinnedToCore(tg1wdt_guard_loop, "tg1_gd", 2048, NULL,
                            configMAX_PRIORITIES - 1, &tg1wdt_guard_task, 1);
}

void WiFiManager::stopTG1Guard() {
    if (!tg1wdt_guard_task) return;
    vTaskDelete(tg1wdt_guard_task);
    tg1wdt_guard_task = NULL;
}

bool WiFiManager::begin() {
    // Guard ya esta corriendo desde setup() — no necesita iniciarse aqui.

    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(
        IPAddress(192, 168, 4, 1),
        IPAddress(192, 168, 4, 1),
        IPAddress(255, 255, 255, 0)
    );
    WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASS, WIFI_AP_CHANNEL, 0, WIFI_AP_MAX_CONN);

    // NO detener guard — corre permanentemente. WiFi re-habilita TG1WDT
    // asincronamente y necesitamos deshabilitarlo continuamente.

    _active = true;
    _lastClientCount = 0;

    Serial.printf("[WIFI] AP '%s' activo en %s\n", WIFI_AP_SSID,
                  WiFi.softAPIP().toString().c_str());
    return true;
}

void WiFiManager::stop() {
    WiFi.softAPdisconnect(true);
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    _active = false;
    _staActive = false;
    _lastClientCount = 0;
    Serial.println("[WIFI] Detenido");
}

bool WiFiManager::beginDual(const char* staSsid, const char* staPass) {
    if (!staSsid || strlen(staSsid) == 0) return false;

    Serial.printf("[WIFI] STA conectando a '%s'...\n", staSsid);

    WiFi.mode(WIFI_AP_STA);

    // Primero STA
    WiFi.begin(staSsid, staPass);
    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) {
        delay(500);
        Serial.print(".");
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
        _staActive = true;
        Serial.printf("[WIFI] STA OK: %s\n", WiFi.localIP().toString().c_str());
    } else {
        Serial.println("[WIFI] STA fallo, continuando solo AP");
        WiFi.disconnect();
    }

    // Luego AP
    WiFi.softAPConfig(
        IPAddress(192, 168, 4, 1),
        IPAddress(192, 168, 4, 1),
        IPAddress(255, 255, 255, 0)
    );
    WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASS, WIFI_AP_CHANNEL, 0, WIFI_AP_MAX_CONN);

    _active = true;
    _lastClientCount = 0;

    Serial.printf("[WIFI] AP '%s' activo en %s\n", WIFI_AP_SSID,
                  WiFi.softAPIP().toString().c_str());
    return true;
}

bool WiFiManager::isSTAConnected() const {
    return _staActive && (WiFi.status() == WL_CONNECTED);
}

IPAddress WiFiManager::getSTAIP() const {
    return WiFi.localIP();
}

bool WiFiManager::isActive() const {
    return _active;
}

uint8_t WiFiManager::getClientCount() const {
    return WiFi.softAPgetStationNum();
}

IPAddress WiFiManager::getAPIP() const {
    return WiFi.softAPIP();
}

void WiFiManager::update() {
    if (_active) {
        checkClientChanges();
    }
}

void WiFiManager::onClientChange(ClientEventCallback cb) {
    _clientCb = cb;
}

void WiFiManager::checkClientChanges() {
    uint8_t count = WiFi.softAPgetStationNum();
    if (count != _lastClientCount) {
        _lastClientCount = count;
        if (_clientCb) {
            _clientCb(count);
        }
    }
}
