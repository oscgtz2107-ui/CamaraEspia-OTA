#pragma once
#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <DNSServer.h>
#include <Preferences.h>

class CaptivePortal {
public:
    void begin();
    void stop();
    void process();
    bool isActive() const { return _active; }

private:
    AsyncWebServer* _server = nullptr;
    DNSServer* _dns = nullptr;
    bool _active = false;

    // WiFi scan async
    String _wifiCache = "[]";
    bool _wifiScanning = false;
    unsigned long _wifiScanStart = 0;

    void setupRoutes();
    void handleRoot(AsyncWebServerRequest* request);
    void handleSave(AsyncWebServerRequest* request);
    String getSetupHTML();
};
