#include "telegram_bot.h"
#include "camera_manager.h"
#include "servo_manager.h"
#include "sd_manager.h"
#include "battery_manager.h"
#include <WiFi.h>

TelegramBot::TelegramBot()
    : _configured(false), _running(false),
      _ownerChatId(0), _lastUpdateId(0), _lastActivityTime(0),
      _taskHandle(nullptr), _notifyQueue(nullptr),
      _camera(nullptr), _servo(nullptr), _sd(nullptr), _battery(nullptr) {
}

void TelegramBot::begin(const char* botToken) {
    if (!botToken || strlen(botToken) == 0) {
        Serial.println("[TGBOT] Sin token, no se inicia polling");
        return;
    }
    _botToken = botToken;
    _configured = true;

    _notifyQueue = xQueueCreate(4, sizeof(MotionNotification));

    xTaskCreatePinnedToCore(
        pollTask, "TgBot", 8192, this,
        2, &_taskHandle, 0
    );

    Serial.printf("[TGBOT] Iniciado (token=%s..., owner=%lld)\n",
                  _botToken.substring(0, 8).c_str(), _ownerChatId);
}

void TelegramBot::stop() {
    _running = false;
    if (_taskHandle) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        vTaskDelete(_taskHandle);
        _taskHandle = nullptr;
    }
    if (_notifyQueue) {
        vQueueDelete(_notifyQueue);
        _notifyQueue = nullptr;
    }
    Serial.println("[TGBOT] Detenido");
}

bool TelegramBot::isConnected() const {
    return _configured && (WiFi.status() == WL_CONNECTED);
}

bool TelegramBot::isLinked() const {
    return _ownerChatId != 0;
}

void TelegramBot::notifyMotion(const uint8_t* jpeg, size_t len) {
    if (!jpeg || len == 0 || !_notifyQueue || !isLinked()) return;

    uint8_t* copy = (uint8_t*)ps_malloc(len);
    if (!copy) copy = (uint8_t*)malloc(len);
    if (!copy) {
        Serial.println("[TGBOT] Sin memoria para notificacion");
        return;
    }
    memcpy(copy, jpeg, len);

    MotionNotification notif;
    notif.jpeg = copy;
    notif.len = len;

    if (xQueueSend(_notifyQueue, &notif, pdMS_TO_TICKS(100)) != pdTRUE) {
        Serial.println("[TGBOT] Cola llena, descartando");
        free(copy);
    }
}

void TelegramBot::setToken(const char* token) {
    _botToken = token ? token : "";
    _configured = (_botToken.length() > 0);
}

const String& TelegramBot::getToken() const { return _botToken; }

void TelegramBot::setOwnerChatId(int64_t chatId) {
    _ownerChatId = chatId;
    Serial.printf("[TGBOT] Owner chat_id: %lld\n", chatId);
}

int64_t TelegramBot::getOwnerChatId() const { return _ownerChatId; }

void TelegramBot::setCameraName(const char* name) {
    _cameraName = name ? name : "";
}

const String& TelegramBot::getCameraName() const { return _cameraName; }

void TelegramBot::setDevices(CameraManager* cam, ServoManager* servo,
                              SDManager* sd, BatteryMonitor* bat) {
    _camera = cam;
    _servo = servo;
    _sd = sd;
    _battery = bat;
}

// ===== FreeRTOS Task =====

void TelegramBot::pollTask(void* param) {
    TelegramBot* self = (TelegramBot*)param;
    self->_running = true;
    Serial.println("[TGBOT] Polling task Core 0 OK");

    while (self->_running) {
        self->pollLoop();
    }
    Serial.println("[TGBOT] Polling task terminado");
    vTaskDelete(NULL);
}

void TelegramBot::pollLoop() {
    if (!isConnected()) {
        vTaskDelay(pdMS_TO_TICKS(5000));
        return;
    }

    MotionNotification notif;
    while (xQueueReceive(_notifyQueue, &notif, 0) == pdTRUE) {
        if (isLinked()) {
            String msg = "\xF0\x9F\x9A\xA8 " + String(TXT(LANG_ES, MOTION_DETECTED));
            if (_cameraName.length() > 0) {
                msg += "\n📍 " + _cameraName;
            }
            apiSendPhoto(_ownerChatId, notif.jpeg, notif.len, msg);
        }
        free(notif.jpeg);
    }

    apiGetUpdates(_lastUpdateId + 1, 5);

    unsigned long idleMs = millis() - _lastActivityTime;
    if (idleMs > 120000) {
        vTaskDelay(pdMS_TO_TICKS(15000));
    } else if (idleMs > 30000) {
        vTaskDelay(pdMS_TO_TICKS(8000));
    } else {
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

// ===== Telegram API =====

bool TelegramBot::apiGetUpdates(long offset, int timeout) {
    WiFiClientSecure client;
    client.setInsecure();
    client.setTimeout(20000);

    String url = "/bot" + _botToken + "/getUpdates?offset=" +
                 String(offset) + "&limit=10&timeout=" + String(timeout);

    if (!client.connect("api.telegram.org", 443)) {
        return false;
    }

    client.print("GET " + url + " HTTP/1.1\r\n");
    client.print("Host: api.telegram.org\r\n");
    client.print("Connection: close\r\n");
    client.print("\r\n");

    unsigned long t = millis();
    while (client.connected() && millis() - t < 10000) {
        String line = client.readStringUntil('\n');
        if (line == "\r\n") break;
    }

    String body;
    body.reserve(4096);
    t = millis();
    while (client.connected() && millis() - t < 20000) {
        if (client.available()) body += (char)client.read();
    }
    client.stop();

    if (body.length() == 0) return false;

    JsonDocument doc;
    if (deserializeJson(doc, body)) return false;
    if (!doc["ok"]) return false;

    JsonArray results = doc["result"];
    for (JsonObject r : results) {
        long uid = r["update_id"] | 0L;
        if (uid > _lastUpdateId) _lastUpdateId = uid;
    }

    if (results.size() > 0) {
        processMessage(body);
    }

    return results.size() > 0;
}

bool TelegramBot::apiSendMessage(int64_t chatId, const String& text,
                                  const String& replyMarkup) {
    WiFiClientSecure client;
    client.setInsecure();
    client.setTimeout(10000);

    if (!client.connect("api.telegram.org", 443)) return false;

    JsonDocument body;
    body["chat_id"] = chatId;
    body["text"] = text;
    if (replyMarkup.length() > 0) {
        JsonDocument kb;
        deserializeJson(kb, replyMarkup);
        body["reply_markup"] = kb;
    }

    String payload;
    serializeJson(body, payload);

    client.print("POST /bot" + _botToken + "/sendMessage HTTP/1.1\r\n");
    client.print("Host: api.telegram.org\r\n");
    client.print("Content-Type: application/json\r\n");
    client.print("Content-Length: " + String(payload.length()) + "\r\n");
    client.print("Connection: close\r\n\r\n");
    client.print(payload);

    bool ok = false;
    unsigned long t = millis();
    while (client.connected() && millis() - t < 10000) {
        String line = client.readStringUntil('\n');
        if (line.indexOf("200 OK") > 0) ok = true;
        if (line == "\r\n") break;
    }
    client.stop();
    return ok;
}

bool TelegramBot::apiSendPhoto(int64_t chatId, const uint8_t* jpeg, size_t len,
                                const String& caption) {
    WiFiClientSecure client;
    client.setInsecure();
    client.setTimeout(15000);

    if (!client.connect("api.telegram.org", 443)) return false;

    String boundary = "CamBot" + String(millis());
    String startBody = "--" + boundary + "\r\n"
        "Content-Disposition: form-data; name=\"chat_id\"\r\n\r\n"
        + String(chatId) + "\r\n--" + boundary + "\r\n"
        "Content-Disposition: form-data; name=\"caption\"\r\n\r\n"
        + caption + "\r\n--" + boundary + "\r\n"
        "Content-Disposition: form-data; name=\"photo\"; filename=\"capture.jpg\"\r\n"
        "Content-Type: image/jpeg\r\n\r\n";
    String endBody = "\r\n--" + boundary + "--\r\n";
    int contentLen = startBody.length() + len + endBody.length();

    client.print("POST /bot" + _botToken + "/sendPhoto HTTP/1.1\r\n");
    client.print("Host: api.telegram.org\r\n");
    client.print("Content-Type: multipart/form-data; boundary=" + boundary + "\r\n");
    client.print("Content-Length: " + String(contentLen) + "\r\n");
    client.print("Connection: close\r\n\r\n");
    client.print(startBody);
    client.write(jpeg, len);
    client.print(endBody);

    bool ok = false;
    unsigned long t = millis();
    while (client.connected() && millis() - t < 15000) {
        String line = client.readStringUntil('\n');
        if (line.indexOf("200 OK") > 0) ok = true;
        if (line == "\r\n") break;
    }
    client.stop();
    Serial.printf("[TGBOT] sendPhoto chatId=%lld len=%u %s\n",
                  chatId, len, ok ? "OK" : "FAIL");
    return ok;
}

bool TelegramBot::apiAnswerCallback(const String& callbackId,
                                     const String& text) {
    WiFiClientSecure client;
    client.setInsecure();
    client.setTimeout(10000);

    if (!client.connect("api.telegram.org", 443)) return false;

    JsonDocument body;
    body["callback_query_id"] = callbackId;
    if (text.length() > 0) body["text"] = text;
    String payload;
    serializeJson(body, payload);

    client.print("POST /bot" + _botToken + "/answerCallbackQuery HTTP/1.1\r\n");
    client.print("Host: api.telegram.org\r\n");
    client.print("Content-Type: application/json\r\n");
    client.print("Content-Length: " + String(payload.length()) + "\r\n");
    client.print("Connection: close\r\n\r\n");
    client.print(payload);

    unsigned long t = millis();
    while (client.connected() && millis() - t < 10000) {
        String line = client.readStringUntil('\n');
        if (line == "\r\n") break;
    }
    client.stop();
    return true;
}

// ===== Procesar mensajes =====

void TelegramBot::processMessage(const String& json) {
    JsonDocument doc;
    if (deserializeJson(doc, json)) return;

    JsonObject msg = doc["result"][0];
    if (msg.isNull()) return;

    if (msg.containsKey("callback_query")) {
        JsonObject cb = msg["callback_query"];
        int64_t chatId = cb["from"]["id"] | 0LL;
        String data = cb["data"] | "";
        String cbId = cb["id"] | "";

        // Solo owner puede usar botones
        if (chatId != _ownerChatId) {
            apiAnswerCallback(cbId, "No autorizado");
            return;
        }
        _lastActivityTime = millis();
        handleCallback(chatId, data, cbId);
        return;
    }

    if (msg.containsKey("message")) {
        JsonObject m = msg["message"];
        int64_t chatId = m["from"]["id"] | 0LL;
        String text = m["text"] | "";

        // /start: responder con chat_id a CUALQUIERA (flujo de vinculacion)
        if (text == "/start" || text == "/help") {
            String response = "\xF0\x9F\x91\x8B Tu Chat ID es:\n\n";
            response += "`" + String(chatId) + "`\n\n";
            if (_ownerChatId == 0) {
                response += "\xE2\x9C\x85 Camera vinculada correctamente.";
            } else if (chatId == _ownerChatId) {
                response += "\xE2\x9C\x85 Esta camera ya esta vinculada a ti.";
            } else {
                response += "\xE2\x9A\xA0\xEF\xB8\x8F Esta camera ya tiene propietario.";
            }
            apiSendMessage(chatId, response);
            _lastActivityTime = millis();
            return;
        }

        // Todos los demas comandos: solo owner
        if (chatId != _ownerChatId) {
            if (_ownerChatId != 0) {
                apiSendMessage(chatId, "\xE2\x9D\x8C No autorizado. Esta camera pertenece a otro usuario.");
            }
            return;
        }

        _lastActivityTime = millis();
        handleText(chatId, text);
    }
}

void TelegramBot::handleText(int64_t chatId, const String& text) {
    showMainMenu(chatId);
}

void TelegramBot::handleCallback(int64_t chatId, const String& data,
                                  const String& cbId) {
    apiAnswerCallback(cbId);

    if (data == "photo")            { actionPhoto(chatId); }
    else if (data == "stream")      { actionStream(chatId); }
    else if (data == "status")      { showStatus(chatId); }
    else if (data == "record")      { showRecordMenu(chatId); }
    else if (data == "config")      { showConfigMenu(chatId); }
    else if (data == "back")        { showMainMenu(chatId); }
    else if (data == "rec_start")   { actionRecordStart(chatId); }
    else if (data == "rec_stop")    { actionRecordStop(chatId); }
    else if (data == "lang_menu")   { showLanguageMenu(chatId); }
    else if (data == "alerts_on")   {
        apiSendMessage(chatId, "\xF0\x9F\x94\x94 " + String(TXT(LANG_ES, ALERTS_ON)));
        showConfigMenu(chatId);
    }
    else if (data == "alerts_off")  {
        apiSendMessage(chatId, "\xF0\x9F\x94\x95 " + String(TXT(LANG_ES, ALERTS_OFF)));
        showConfigMenu(chatId);
    }
}

// ===== Menus =====

void TelegramBot::showMainMenu(int64_t chatId) {
    Lang lang = LANG_ES;
    String welcome = String(TXT(lang, WELCOME));
    if (_cameraName.length() > 0) {
        welcome += "\n\n📍 " + _cameraName;
    }

    String kb = "[["
        "{\"text\":\"" + String(TXT(lang, MENU_PHOTO)) + "\",\"callback_data\":\"photo\"},"
        "{\"text\":\"" + String(TXT(lang, MENU_STREAM)) + "\",\"callback_data\":\"stream\"}],"
        "[{\"text\":\"" + String(TXT(lang, MENU_STATUS)) + "\",\"callback_data\":\"status\"},"
        "{\"text\":\"" + String(TXT(lang, MENU_RECORD)) + "\",\"callback_data\":\"record\"}],"
        "[{\"text\":\"" + String(TXT(lang, MENU_CONFIG)) + "\",\"callback_data\":\"config\"}]"
        "]]";

    apiSendMessage(chatId, welcome, kb);
}

void TelegramBot::showConfigMenu(int64_t chatId) {
    Lang lang = LANG_ES;

    String kb = "[["
        "{\"text\":\"" + String(TXT(lang, MENU_LANGUAGE)) + "\",\"callback_data\":\"lang_menu\"},"
        "{\"text\":\"" + String(TXT(lang, MENU_ALERTS)) + "\",\"callback_data\":\"alerts_on\"}],"
        "[{\"text\":\"" + String(TXT(lang, MENU_BACK)) + "\",\"callback_data\":\"back\"}]"
        "]]";

    apiSendMessage(chatId, String(TXT(lang, MENU_CONFIG)), kb);
}

void TelegramBot::showLanguageMenu(int64_t chatId) {
    Lang lang = LANG_ES;

    String kb = "[["
        "{\"text\":\"ES\",\"callback_data\":\"lang_es\"},"
        "{\"text\":\"EN\",\"callback_data\":\"lang_en\"},"
        "{\"text\":\"PT\",\"callback_data\":\"lang_pt\"},"
        "{\"text\":\"FR\",\"callback_data\":\"lang_fr\"},"
        "{\"text\":\"DE\",\"callback_data\":\"lang_de\"}],"
        "[{\"text\":\"IT\",\"callback_data\":\"lang_it\"},"
        "{\"text\":\"RU\",\"callback_data\":\"lang_ru\"},"
        "{\"text\":\"ZH\",\"callback_data\":\"lang_zh\"},"
        "{\"text\":\"JA\",\"callback_data\":\"lang_ja\"},"
        "{\"text\":\"KO\",\"callback_data\":\"lang_ko\"}],"
        "[{\"text\":\"AR\",\"callback_data\":\"lang_ar\"},"
        "{\"text\":\"HI\",\"callback_data\":\"lang_hi\"},"
        "{\"text\":\"BN\",\"callback_data\":\"lang_bn\"},"
        "{\"text\":\"TR\",\"callback_data\":\"lang_tr\"},"
        "{\"text\":\"VI\",\"callback_data\":\"lang_vi\"}],"
        "[{\"text\":\"TH\",\"callback_data\":\"lang_th\"},"
        "{\"text\":\"ID\",\"callback_data\":\"lang_id\"},"
        "{\"text\":\"PL\",\"callback_data\":\"lang_pl\"},"
        "{\"text\":\"UK\",\"callback_data\":\"lang_uk\"},"
        "{\"text\":\"MS\",\"callback_data\":\"lang_ms\"}],"
        "[{\"text\":\"SW\",\"callback_data\":\"lang_sw\"},"
        "{\"text\":\"TL\",\"callback_data\":\"lang_tl\"},"
        "{\"text\":\"NL\",\"callback_data\":\"lang_nl\"}],"
        "[{\"text\":\"" + String(TXT(lang, MENU_BACK)) + "\",\"callback_data\":\"back\"}]"
        "]]";

    apiSendMessage(chatId, String(TXT(lang, SELECT_LANG)), kb);
}

void TelegramBot::showRecordMenu(int64_t chatId) {
    Lang lang = LANG_ES;

    String kb = "[["
        "{\"text\":\"" + String(TXT(lang, RECORD_START)) + "\",\"callback_data\":\"rec_start\"},"
        "{\"text\":\"" + String(TXT(lang, RECORD_STOP)) + "\",\"callback_data\":\"rec_stop\"}],"
        "[{\"text\":\"" + String(TXT(lang, MENU_BACK)) + "\",\"callback_data\":\"back\"}]"
        "]]";

    apiSendMessage(chatId, String(TXT(lang, MENU_RECORD)), kb);
}

void TelegramBot::showStatus(int64_t chatId) {
    Lang lang = LANG_ES;

    String s = String(TXT(lang, STATUS_TITLE)) + "\n\n";

    s += "\xF0\x9F\x94\x8B " + String(TXT(lang, STATUS_BATTERY)) + ": ";
    s += (_battery && _battery->isPresent()) ?
         (String(_battery->getPercentage()) + "% (" + String(_battery->readVoltage(), 1) + "V)") :
         "N/A";
    s += "\n\xF0\x9F\x92\xBE " + String(TXT(lang, STATUS_SD)) + ": ";
    s += (_sd && _sd->isMounted()) ? (String(_sd->getFreeSpaceMB()) + " MB") : "N/A";
    s += "\n\xF0\x9F\x93\x86 " + String(TXT(lang, STATUS_WIFI)) + ": " + String(WiFi.softAPgetStationNum()) + " clients";
    s += "\n\xF0\x9F\x93\xB7 " + String(TXT(lang, STATUS_CAMERA)) + ": ";
    s += (_camera && _camera->isInitialized()) ? "ON" : "OFF";
    s += "\n\xF0\x9F\xA6\xBE " + String(TXT(lang, STATUS_SERVO)) + ": ";
    s += _servo ? (String(_servo->getAngle()) + "\u00B0") : "N/A";

    apiSendMessage(chatId, s);
}

// ===== Acciones =====

void TelegramBot::actionPhoto(int64_t chatId) {
    if (!_camera) { apiSendMessage(chatId, "\xE2\x9D\x8C Camera not available"); return; }

    bool wasInit = _camera->isInitialized();
    if (!wasInit) _camera->reinit();
    if (_camera->isStandby()) _camera->wake();

    if (!_camera->isInitialized()) {
        apiSendMessage(chatId, String(TXT(LANG_ES, ERROR)));
        return;
    }

    size_t len = 0;
    uint8_t* buf = _camera->captureJPEG(&len);
    if (buf && len > 0) {
        apiSendPhoto(chatId, buf, len, String(TXT(LANG_ES, PHOTO_TAKEN)));
        free(buf);
    } else {
        apiSendMessage(chatId, String(TXT(LANG_ES, ERROR)));
    }

    if (!wasInit && _camera->isInitialized()) _camera->deinit();
}

void TelegramBot::actionStream(int64_t chatId) {
    String ip = WiFi.localIP().toString();
    if (ip == "0.0.0.0") ip = "192.168.4.1";
    apiSendMessage(chatId, "\xF0\x9F\x93\xB9 " + String(TXT(LANG_ES, STREAM_URL)) +
                   "\nhttp://" + ip + ":81/stream");
}

void TelegramBot::actionRecordStart(int64_t chatId) {
    apiSendMessage(chatId, String(TXT(LANG_ES, RECORDING)));
}

void TelegramBot::actionRecordStop(int64_t chatId) {
    apiSendMessage(chatId, String(TXT(LANG_ES, RECORD_STOP)));
}
