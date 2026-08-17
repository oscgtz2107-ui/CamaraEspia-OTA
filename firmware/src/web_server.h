#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <AsyncJson.h>
#include <ArduinoJson.h>
#include <Preferences.h>

// Forward declarations
class CameraManager;
class ServoManager;
class SDManager;
class BatteryMonitor;
class PIRManager;
class TelegramBot;

class WebServer {
public:
    WebServer();

    bool begin(CameraManager* cam, ServoManager* servo, SDManager* sd,
               BatteryMonitor* bat, PIRManager* pir, TelegramBot* tg);
    void update();
    bool isRunning() const { return _server != nullptr; }
    void broadcastWS(const String& json) {} // stub — WebSocket eliminado

private:
    void setupRoutes();

    // API handlers
    void handleRoot(AsyncWebServerRequest* request);
    void handleCapture(AsyncWebServerRequest* request);
    void handleConfigGet(AsyncWebServerRequest* request);
    void handleConfigSet(AsyncWebServerRequest* request);
    void handleTelegramGet(AsyncWebServerRequest* request);
    void handleTelegramSet(AsyncWebServerRequest* request);
    void handleWifiScan(AsyncWebServerRequest* request);
    void handleReset(AsyncWebServerRequest* request);
    void handleCameraInfo(AsyncWebServerRequest* request);
    void handleCameraStatus(AsyncWebServerRequest* request);

    // PWA static files
    void handlePWAIndex(AsyncWebServerRequest* request);
    void handlePWACss(AsyncWebServerRequest* request);
    void handlePWASw(AsyncWebServerRequest* request);
    void handlePWAManifest(AsyncWebServerRequest* request);
    void handlePWAI18n(AsyncWebServerRequest* request);
    void handlePWADiscovery(AsyncWebServerRequest* request);
    void handlePWACamera(AsyncWebServerRequest* request);
    void handlePWASettings(AsyncWebServerRequest* request);
    void handlePWAApp(AsyncWebServerRequest* request);
    void handlePWAIcon192(AsyncWebServerRequest* request);
    void handlePWAIcon512(AsyncWebServerRequest* request);

    AsyncWebServer* _server;
    CameraManager* _camera;
    ServoManager* _servo;
    SDManager* _sd;
    BatteryMonitor* _battery;
    PIRManager* _pir;
    TelegramBot* _telegram;
};

#endif // WEB_SERVER_H
