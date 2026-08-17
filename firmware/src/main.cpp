#include <Arduino.h>
#include <Preferences.h>
#include <esp_task_wdt.h>
#include <esp_sleep.h>
#include "driver/gpio.h"

#include "config.h"
#include "battery_manager.h"
#include "servo_manager.h"
#include "pir_manager.h"
#include "sd_manager.h"
#include "camera_manager.h"
#include "wifi_manager.h"
#include "web_server.h"
#include "stream_server.h"
#include "avi_recorder.h"
#include "telegram_bot.h"
#include "captive_portal.h"

// TG1WDT del bootloader: deshabilitar + poner stages a timeout maximo.
#include "soc/timer_group_reg.h"
static void disableBootloaderWDT() {
    REG_WRITE(TIMG_WDTWPROTECT_REG(1), 0xFFFFFFFF);
    REG_WRITE(TIMG_WDTCONFIG0_REG(1), 0);
    REG_WRITE(TIMG_WDTCONFIG2_REG(1), 0xFFFFFFFF);
    REG_WRITE(TIMG_WDTCONFIG3_REG(1), 0xFFFFFFFF);
    REG_WRITE(TIMG_WDTCONFIG4_REG(1), 0xFFFFFFFF);
    REG_WRITE(TIMG_WDTCONFIG5_REG(1), 0xFFFFFFFF);
    REG_WRITE(TIMG_WDTFEED_REG(1), 1);
}

// ===== ENUMS =====
enum SystemState {
    STATE_BOOT,
    STATE_ACTIVO,
    STATE_GRABANDO,
    STATE_DIAGNOSTICO
};

enum RecordingMode {
    REC_CONTINUOUS,
    REC_MOTION
};

// ===== INSTANCIAS GLOBALES =====
BatteryMonitor battery;
ServoManager servo;
PIRManager pir;
SDManager sd;
CameraManager camera;
WiFiManager wifi;
WebServer web;
StreamServer stream;
AVIRecorder recorder;
TelegramBot tgBot;

CaptivePortal captivePortal;
Preferences prefs;

// ===== VARIABLES DE ESTADO =====
SystemState currentState = STATE_BOOT;
SystemState previousState = STATE_BOOT;
RecordingMode currentMode = REC_CONTINUOUS;
unsigned long stateEntryTime = 0;
unsigned long lastActivityTime = 0;
unsigned long lastBatteryRead = 0;
unsigned long lastStatusBroadcast = 0;
unsigned long lastFrameWrite = 0;
unsigned long pirLowTime = 0;
bool pirWasTriggered = false;
bool recordingActive = false;
bool motionDetected = false;

// ===== FORWARD DECLARATIONS =====
void changeState(SystemState newState);
void enterActivo();
void enterGrabando();
void exitGrabando();
void handleStateActivo();
void handleStateGrabando();
void handleStateDiagnostico();
void onPIRTrigger();
void onWiFiClientChange(uint8_t count);
void broadcastStatus();
void loadConfig();
void saveConfig();
void feedWDT();
void handleRecording();
String generateEventFilename();
void checkLowBattery();
void setupGPIO();
void setupWDT();
void forceDisableTG1WDT();

// ===== SETUP =====
void setup() {
    Serial.begin(115200);
    while (!Serial && millis() < 3000) { ; }
    delay(500);

    Serial.println("\n========================================");
    Serial.println("  CamaraEspia-ESP32S3 - Boot");
    Serial.println("========================================");

    disableBootloaderWDT();

    // Arrancar guard TG1WDT lo antes posible — protege contra re-habilitacion
    // por WiFi u otros subsistemas durante todo el boot.
    WiFiManager::startTG1Guard();

    // === CAPTIVE PORTAL: Si no esta configurado, solo mostrar setup ===
    {
        Preferences checkPrefs;
        checkPrefs.begin("camara", true);
        bool setupDone = checkPrefs.getBool("setup_complete", false);
        checkPrefs.end();

        if (!setupDone) {
            Serial.println("[BOOT] Setup NO completado — iniciando Captive Portal");

            // Solo AP, sin STA
            WiFi.mode(WIFI_AP);
            WiFi.softAP("Camara-Setup");
            delay(500);
            Serial.printf("[AP] SSID: Camara-Setup  IP: %s\n",
                          WiFi.softAPIP().toString().c_str());

            captivePortal.begin();
            Serial.println("[CP] Abre http://192.168.4.1 en tu navegador");

            // Bloquear — procesar DNS + web hasta que el usuario guarde
            while (true) {
                captivePortal.process();
                delay(10);
            }
        }
        Serial.println("[BOOT] Setup completado — modo normal");
    }

    setupGPIO();
    setupWDT();

    // Detectar modo diagnostico (5s) o factory reset (3s)
    if (digitalRead(PIN_BOOT) == LOW) {
        unsigned long pressStart = millis();
        while (digitalRead(PIN_BOOT) == LOW && millis() - pressStart < 5000) {
            delay(100);
        }
        if (millis() - pressStart >= 5000) {
            Serial.println("[BOOT] Modo DIAGNOSTICO activado (boton 5s)");
            changeState(STATE_DIAGNOSTICO);
            return;
        }
        if (millis() - pressStart >= 3000) {
            Serial.println("[BOOT] FACTORY RESET — borrando NVS...");
            Preferences rstPrefs;
            rstPrefs.begin("camara", false);
            rstPrefs.clear();
            rstPrefs.end();
            delay(500);
            ESP.restart();
        }
    }

    // 1. Camara: config lazy (sin DMA/I2S)
    if (!camera.begin()) {
        Serial.println("[FATAL] Camara no inicializo. Reiniciando en 5s...");
        delay(5000);
        ESP.restart();
    }
    forceDisableTG1WDT();

    // 2. Servo
    if (!servo.begin(PIN_SERVO, SERVO_CHANNEL)) {
        Serial.println("[FATAL] Servo no inicializo. Bloqueando.");
        while (true) { delay(1000); }
    }
    forceDisableTG1WDT();
    prefs.begin("camara", true);
    uint8_t savedAngle = prefs.getUInt("servo_angle", SERVO_DEFAULT_ANGLE);
    prefs.end();
    servo.setAngle(savedAngle);
    Serial.printf("[OK] Servo en %d grados\n", savedAngle);
    forceDisableTG1WDT();

    // 3. SD lazy (SD.begin() bloquea y usa SPI DMA → TG1WDT)
    Serial.println("[SD] Init diferida (lazy) - se monta al grabar");

    // 4. Bateria
    battery.begin(PIN_BATTERY, 2.0f);
    forceDisableTG1WDT();
    if (battery.isPresent()) {
        Serial.printf("[OK] Bateria: %.2fV (%d%%)\n",
                      battery.readVoltage(), battery.getPercentage());
    } else {
        Serial.println("[INFO] Bateria no detectada.");
    }

    // 5. PIR
    pir.begin(PIN_PIR, onPIRTrigger);
    forceDisableTG1WDT();
    if (pir.isPresent()) {
        Serial.println("[OK] PIR detectado.");
    } else {
        Serial.println("[INFO] PIR no detectado. Forzando modo continuo.");
        currentMode = REC_CONTINUOUS;
    }
    forceDisableTG1WDT();

    loadConfig();
    forceDisableTG1WDT();

    // WiFi siempre ON desde el primer momento.
    forceDisableTG1WDT();
    wifi.begin();

    // Conectar STA si hay credenciales guardadas
    prefs.begin("camara", true);
    String savedSsid = prefs.getString("wifi_ssid", "");
    String savedPass = prefs.getString("wifi_pass", "");
    prefs.end();
    if (savedSsid.length() > 0) {
        Serial.printf("[WIFI] Conectando STA a '%s'...\n", savedSsid.c_str());
        wifi.beginDual(savedSsid.c_str(), savedPass.c_str());
    }

    // Puente: deshabilitar TG1WDT repetidamente mientras WiFi event handler
    // puede re-habilitarlo internamente. Despues loop() se encarga.
    for (int i = 0; i < 50; i++) {
        forceDisableTG1WDT();
        delay(100);
    }

    disableBootloaderWDT();
    tgBot.setDevices(&camera, &servo, &sd, &battery);
    web.begin(&camera, &servo, &sd, &battery, &pir, &tgBot);
    stream.setCameraManager(&camera);
    stream.setFrameDelay(100);
    stream.begin(81);

    Serial.println("[BOOT] WiFi + Web + Stream iniciados");

    changeState(STATE_ACTIVO);
}

// ===== LOOP =====
void loop() {
    static bool wdtTaskAdded = false;
    if (!wdtTaskAdded) {
        esp_task_wdt_add(NULL);
        wdtTaskAdded = true;
        Serial.println("[WDT] Tarea agregada al watchdog");
    }

    feedWDT();

    switch (currentState) {
        case STATE_ACTIVO:
            handleStateActivo();
            break;
        case STATE_GRABANDO:
            handleStateGrabando();
            break;
        case STATE_DIAGNOSTICO:
            handleStateDiagnostico();
            break;
        default:
            break;
    }

    if (currentState != STATE_DIAGNOSTICO) {
        pir.update();
        wifi.update();

        // Camera lazy: deinit si no hay clientes y no esta grabando
        if (camera.isInitialized() && !recordingActive && !stream.hasClients()) {
            Serial.println("[CAM] Sin clientes ni grabacion. Deinit lazy.");
            camera.deinit();
        }

        if (battery.isPresent() && millis() - lastBatteryRead > BATTERY_INTERVAL_MS) {
            lastBatteryRead = millis();
            battery.readVoltage();
            checkLowBattery();
        }

        if (millis() - lastStatusBroadcast > STATUS_INTERVAL_MS) {
            lastStatusBroadcast = millis();
            broadcastStatus();
        }
    }

    delay(10);
}

// ===== CAMBIO DE ESTADO =====
void changeState(SystemState newState) {
    if (newState == currentState) return;

    Serial.printf("[STATE] %d -> %d\n", currentState, newState);
    previousState = currentState;
    currentState = newState;
    stateEntryTime = millis();
    lastActivityTime = millis();

    switch (newState) {
        case STATE_ACTIVO:
            enterActivo();
            break;
        case STATE_GRABANDO:
            enterGrabando();
            break;
        case STATE_DIAGNOSTICO:
            handleStateDiagnostico();
            break;
    }
}

// ===== ACTIVO =====
// Estado permanente: WiFi + Web + Stream siempre ON.
// Camara solo ON cuando hay clientes conectados o grabando.
void enterActivo() {
    Serial.println("[ACTIVO] Modo activo (WiFi siempre ON)");

    if (recordingActive) {
        exitGrabando();
    }

    lastActivityTime = millis();
}

void handleStateActivo() {
    if (currentMode == REC_CONTINUOUS && sd.isMounted() && !recordingActive) {
        handleRecording();
    }

    if (currentMode == REC_MOTION && motionDetected && sd.isMounted()) {
        changeState(STATE_GRABANDO);
        return;
    }

    if (stream.hasClients()) {
        lastActivityTime = millis();
    }
}

// ===== GRABANDO =====
void enterGrabando() {
    Serial.println("[GRABANDO] Iniciando grabacion");

    if (!sd.isMounted()) {
        sd.tryMount(PIN_SD_CS, PIN_SD_MOSI, PIN_SD_MISO, PIN_SD_CLK);
        if (!sd.isMounted()) {
            Serial.println("[ERROR] SD no disponible. Volviendo a ACTIVO.");
            changeState(STATE_ACTIVO);
            return;
        }
    }

    if (!camera.isInitialized()) {
        camera.reinit();
        disableBootloaderWDT();
    }

    String filename = generateEventFilename();

    if (recorder.startRecording(filename.c_str())) {
        recordingActive = true;
        lastFrameWrite = millis();

        if (currentMode == REC_MOTION) {
            pirWasTriggered = true;
            Serial.printf("[GRABANDO] Modo MOTION: %s\n", filename.c_str());
        } else {
            Serial.printf("[GRABANDO] Modo CONTINUO: %s\n", filename.c_str());
        }
    } else {
        Serial.println("[ERROR] No se pudo iniciar grabacion");
        changeState(STATE_ACTIVO);
    }
}

void handleStateGrabando() {
    if (!recordingActive) {
        changeState(STATE_ACTIVO);
        return;
    }

    if (millis() - lastFrameWrite >= 100) {
        lastFrameWrite = millis();
        recorder.writeFrame();
    }

    if (currentMode == REC_CONTINUOUS) {
        recorder.checkRotation();
    }

    if (currentMode == REC_MOTION) {
        if (!pir.isTriggered() && pirWasTriggered) {
            if (pirLowTime == 0) {
                pirLowTime = millis();
            } else if (millis() - pirLowTime > PIR_COOLDOWN_MS) {
                Serial.println("[GRABANDO] PIR LOW + cooldown. Deteniendo.");
                changeState(STATE_ACTIVO);
                return;
            }
        } else if (pir.isTriggered()) {
            pirLowTime = 0;
        }
    }
}

void exitGrabando() {
    if (recordingActive) {
        recorder.stopRecording();
        recordingActive = false;
    }
    if (camera.isInitialized()) {
        camera.deinit();
    }
    pirWasTriggered = false;
    pirLowTime = 0;
    motionDetected = false;
}

void handleRecording() {
    if (!recordingActive) {
        enterGrabando();
        return;
    }
}

// ===== DIAGNOSTICO =====
void handleStateDiagnostico() {
    if (!wifi.isActive()) {
        wifi.begin();
        disableBootloaderWDT();
    }
    if (!web.isRunning()) {
        web.begin(&camera, &servo, &sd, &battery, &pir, &tgBot);
    }
    if (!stream.isRunning()) {
        stream.setCameraManager(&camera);
        stream.begin(81);
    }

    camera.wake();
    servo.wake();

    static unsigned long lastDiagPrint = 0;
    if (millis() - lastDiagPrint > 5000) {
        lastDiagPrint = millis();
        Serial.println("=== DIAGNOSTICO ===");
        Serial.printf("Camara: %s\n", camera.isInitialized() ? "OK" : "FAIL");
        Serial.printf("Servo: %d grados\n", servo.getAngle());
        Serial.printf("SD: %s", sd.isMounted() ? "OK" : "NO");
        if (sd.isMounted()) {
            Serial.printf(" (%llu MB libres)", sd.getFreeSpaceMB());
        }
        Serial.println();
        Serial.printf("Bateria: %s", battery.isPresent() ? "OK" : "NO");
        if (battery.isPresent()) {
            Serial.printf(" (%.2fV %d%%)", battery.readVoltage(), battery.getPercentage());
        }
        Serial.println();
        Serial.printf("PIR: %s\n", pir.isPresent() ? "OK" : "NO");
        Serial.printf("WiFi: %d clientes\n", wifi.getClientCount());
        Serial.println("===================");
    }

    pir.update();
    wifi.update();
    delay(100);
}

// ===== CALLBACKS =====
void onPIRTrigger() {
    static unsigned long lastPirTime = 0;
    unsigned long now = millis();
    if ((now - lastPirTime) < PIR_COOLDOWN_MS) return;
    lastPirTime = now;

    Serial.println("[EVENT] PIR disparado!");
    motionDetected = true;
    lastActivityTime = millis();

    // Enviar foto a Telegram si esta configurado y conectado
    if (TG_SEND_ON_MOTION && tgBot.isConnected()) {
        // Reinit camara lazy si no esta activa
        bool wasInit = camera.isInitialized();
        if (!wasInit) {
            camera.reinit();
        }
        if (camera.isInitialized()) {
            if (camera.isStandby()) camera.wake();
            size_t len = 0;
            uint8_t* buf = camera.captureJPEG(&len);
            if (buf && len > 0) {
                tgBot.notifyMotion(buf, len);
                free(buf);
            }
        }
        // Deinit si nadie mas la necesita
        if (!wasInit && !stream.hasClients() && !recordingActive) {
            camera.deinit();
        }
    }
}

void onWiFiClientChange(uint8_t count) {
    Serial.printf("[WIFI] Clientes conectados: %d\n", count);
    if (count > 0) {
        lastActivityTime = millis();
    }
}

// ===== HELPERS =====
void broadcastStatus() {
    if (!web.isRunning()) return;

    JsonDocument doc;
    doc["type"] = "status";
    doc["state"] = (int)currentState;
    doc["battery_present"] = battery.isPresent();
    doc["battery_pct"] = battery.isPresent() ? battery.getPercentage() : 0;
    doc["sd_present"] = sd.isMounted();
    doc["sd_free_mb"] = sd.isMounted() ? sd.getFreeSpaceMB() : 0;
    doc["pir_present"] = pir.isPresent();
    doc["recording"] = recordingActive;
    doc["recording_mode"] = (currentMode == REC_CONTINUOUS) ? "continuous" : "motion";
    doc["angle"] = servo.getAngle();
    doc["wifi_clients"] = wifi.getClientCount();
    doc["camera_active"] = camera.isInitialized();
    doc["stream_url"] = "http://192.168.4.1:81/stream";
    doc["tg_configured"] = tgBot.getToken().length() > 0;
    doc["tg_connected"] = tgBot.isConnected();

    String msg;
    serializeJson(doc, msg);
    web.broadcastWS(msg);
}

String generateEventFilename() {
    static uint16_t clipCounter = 0;

    prefs.begin("camara", false);
    clipCounter = prefs.getUInt("clip_counter", 0);
    prefs.putUInt("clip_counter", clipCounter + 1);
    prefs.end();

    char filename[64];
    snprintf(filename, sizeof(filename), "/DCIM/CLIP_%04d.avi", clipCounter);
    return String(filename);
}

void loadConfig() {
    prefs.begin("camara", true);
    uint8_t mode = prefs.getUInt("rec_mode", 0);
    currentMode = (mode == 0) ? REC_CONTINUOUS : REC_MOTION;

    // Telegram config
    int64_t ownerChatId = prefs.getLong64("owner_chat_id", 0);
    String camName = prefs.getString("camera_name", "");
    prefs.end();

    if (!pir.isPresent()) {
        currentMode = REC_CONTINUOUS;
    }

    Serial.printf("[CONFIG] Modo: %s, Owner: %lld\n",
                  (currentMode == REC_CONTINUOUS) ? "CONTINUO" : "MOTION",
                  ownerChatId);

    // Inicializar Telegram con token hardcodeado
    tgBot.setOwnerChatId(ownerChatId);
    tgBot.setCameraName(camName.c_str());
    tgBot.begin(TG_BOT_TOKEN);
    tgBot.setDevices(&camera, &servo, &sd, &battery);
}

void saveConfig() {
    prefs.begin("camara", false);
    prefs.putUInt("rec_mode", (currentMode == REC_CONTINUOUS) ? 0 : 1);
    prefs.end();
}

void checkLowBattery() {
    if (!battery.isPresent()) return;
    uint8_t pct = battery.getPercentage();
    if (pct < 10) {
        Serial.printf("[ALERTA] Bateria baja: %d%%\n", pct);
    }
}

void setupGPIO() {
    pinMode(PIN_BOOT, INPUT_PULLUP);
    pinMode(PIN_PIR, INPUT);
    pinMode(PIN_SERVO, OUTPUT);
    pinMode(PIN_LED, OUTPUT);
    digitalWrite(PIN_LED, LOW);
}

void setupWDT() {
    esp_task_wdt_init(WDT_TIMEOUT_S, true);
    Serial.printf("[WDT] Configurado: %ds (tarea se agrega en loop)\n", WDT_TIMEOUT_S);
}

void forceDisableTG1WDT() {
    REG_WRITE(TIMG_WDTWPROTECT_REG(1), 0xFFFFFFFF);
    REG_WRITE(TIMG_WDTCONFIG0_REG(1), 0);
    REG_WRITE(TIMG_WDTCONFIG2_REG(1), 0xFFFFFFFF);
    REG_WRITE(TIMG_WDTCONFIG3_REG(1), 0xFFFFFFFF);
    REG_WRITE(TIMG_WDTCONFIG4_REG(1), 0xFFFFFFFF);
    REG_WRITE(TIMG_WDTCONFIG5_REG(1), 0xFFFFFFFF);
    REG_WRITE(TIMG_WDTFEED_REG(1), 1);
}

void feedWDT() {
    esp_task_wdt_reset();
    forceDisableTG1WDT();
}
