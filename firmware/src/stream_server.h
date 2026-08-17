#ifndef STREAM_SERVER_H
#define STREAM_SERVER_H

#include <Arduino.h>
#include <WiFi.h>
#include "camera_manager.h"

class StreamServer {
public:
    StreamServer();

    bool begin(uint16_t port = 81);
    void stop();
    bool isRunning() const;
    bool hasClients() const;
    void setCameraManager(CameraManager* cam);
    void setFrameDelay(uint16_t ms);

    static void streamTask(void* parameter);

private:
    WiFiServer* _server;
    CameraManager* _camera;
    uint16_t _port;
    uint16_t _frameDelay;
    volatile bool _running;
    volatile bool _clientConnected;
    TaskHandle_t _taskHandle;

    static const char* STREAM_HEADER;
};

#endif // STREAM_SERVER_H
