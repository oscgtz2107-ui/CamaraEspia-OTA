#ifndef TELEGRAM_BOT_H
#define TELEGRAM_BOT_H

#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include "languages.h"
#include "config.h"

// Forward declarations
class CameraManager;
class ServoManager;
class SDManager;
class BatteryMonitor;

struct MotionNotification {
    uint8_t* jpeg;
    size_t len;
};

class TelegramBot {
public:
    TelegramBot();

    void begin(const char* botToken);
    void stop();
    bool isConnected() const;

    // Core 1 llama esto para notificar movimiento
    void notifyMotion(const uint8_t* jpeg, size_t len);

    // Config — owner
    void setToken(const char* token);
    const String& getToken() const;
    void setOwnerChatId(int64_t chatId);
    int64_t getOwnerChatId() const;
    bool isLinked() const;
    void setCameraName(const char* name);
    const String& getCameraName() const;

    // Referencias a perifericos (para status, foto, grabar)
    void setDevices(CameraManager* cam, ServoManager* servo,
                    SDManager* sd, BatteryMonitor* bat);

private:
    // FreeRTOS task
    static void pollTask(void* param);
    void pollLoop();

    // Telegram API
    bool apiGetUpdates(long offset, int timeout);
    bool apiSendMessage(int64_t chatId, const String& text,
                        const String& replyMarkup = "");
    bool apiSendPhoto(int64_t chatId, const uint8_t* jpeg, size_t len,
                      const String& caption = "");
    bool apiAnswerCallback(const String& callbackId,
                           const String& text = "");

    // Procesar mensajes
    void processMessage(const String& json);
    void handleText(int64_t chatId, const String& text);
    void handleCallback(int64_t chatId, const String& data,
                        const String& callbackId);

    // Menus (envian inline keyboards)
    void showMainMenu(int64_t chatId);
    void showConfigMenu(int64_t chatId);
    void showLanguageMenu(int64_t chatId);
    void showRecordMenu(int64_t chatId);
    void showStatus(int64_t chatId);

    // Acciones
    void actionPhoto(int64_t chatId);
    void actionStream(int64_t chatId);
    void actionRecordStart(int64_t chatId);
    void actionRecordStop(int64_t chatId);

    // Utilidades
    String buildKeyboard(const String& rows) const;

    // Estado
    bool _configured;
    bool _running;
    String _botToken;
    int64_t _ownerChatId;
    String _cameraName;
    long _lastUpdateId;
    unsigned long _lastActivityTime;

    // FreeRTOS
    TaskHandle_t _taskHandle;
    QueueHandle_t _notifyQueue;

    // Perifericos
    CameraManager* _camera;
    ServoManager* _servo;
    SDManager* _sd;
    BatteryMonitor* _battery;
};

#endif // TELEGRAM_BOT_H
