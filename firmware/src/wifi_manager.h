#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <functional>

class WiFiManager {
public:
    WiFiManager();

    static void startTG1Guard();
    static void stopTG1Guard();

    bool begin();
    bool beginDual(const char* staSsid, const char* staPass);
    void stop();
    bool isActive() const;
    bool isSTAConnected() const;
    IPAddress getAPIP() const;
    IPAddress getSTAIP() const;
    uint8_t getClientCount() const;
    void update();

    using ClientEventCallback = std::function<void(uint8_t count)>;
    void onClientChange(ClientEventCallback cb);

private:
    bool _active;
    bool _staActive;
    ClientEventCallback _clientCb;
    uint8_t _lastClientCount;

    void checkClientChanges();
};

#endif // WIFI_MANAGER_H
