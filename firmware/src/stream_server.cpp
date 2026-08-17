#include "stream_server.h"

const char* StreamServer::STREAM_HEADER =
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: multipart/x-mixed-replace; boundary=frame\r\n"
    "Access-Control-Allow-Origin: *\r\n"
    "\r\n";

StreamServer::StreamServer()
    : _server(nullptr), _camera(nullptr), _port(81),
      _frameDelay(100), _running(false), _clientConnected(false), _taskHandle(nullptr) {
}

bool StreamServer::begin(uint16_t port) {
    _port = port;
    _server = new WiFiServer(_port);
    _server->begin();
    _running = true;

    xTaskCreatePinnedToCore(
        streamTask,
        "StreamTask",
        4096,
        this,
        1,
        &_taskHandle,
        1
    );

    Serial.printf("[STREAM] Servidor de streaming en puerto %d\n", _port);
    return true;
}

void StreamServer::stop() {
    _running = false;
    if (_taskHandle) {
        vTaskDelay(pdMS_TO_TICKS(200));
        vTaskDelete(_taskHandle);
        _taskHandle = nullptr;
    }
    if (_server) {
        _server->close();
        delete _server;
        _server = nullptr;
    }
    Serial.println("[STREAM] Servidor detenido");
}

bool StreamServer::hasClients() const {
    return _running && _clientConnected;
}

bool StreamServer::isRunning() const {
    return _running;
}

void StreamServer::setCameraManager(CameraManager* cam) {
    _camera = cam;
}

void StreamServer::setFrameDelay(uint16_t ms) {
    _frameDelay = ms;
}

void StreamServer::streamTask(void* parameter) {
    StreamServer* self = (StreamServer*)parameter;

    while (self->_running) {
        WiFiClient client = self->_server->available();

        if (client) {
            self->_clientConnected = true;
            Serial.println("[STREAM] Cliente conectado");

            // Activar camara lazy: reinit I2S DMA solo cuando hay cliente real
            if (self->_camera && !self->_camera->isInitialized()) {
                self->_camera->reinit();
            }
            if (self->_camera && self->_camera->isStandby()) {
                self->_camera->wake();
            }

            client.write((const uint8_t*)self->STREAM_HEADER, strlen(self->STREAM_HEADER));

            while (client.connected() && self->_running) {
                if (self->_camera && self->_camera->isInitialized()) {
                    size_t len = 0;
                    uint8_t* buf = self->_camera->captureJPEG(&len);

                    if (buf && len > 0) {
                        client.print("--frame\r\n");
                        client.print("Content-Type: image/jpeg\r\n");
                        client.printf("Content-Length: %u\r\n\r\n", len);
                        client.write(buf, len);
                        client.print("\r\n");
                        free(buf);
                    }

                    delay(self->_frameDelay);
                } else {
                    delay(500);
                }

                if (!client.connected()) break;
            }

            client.stop();
            self->_clientConnected = false;
            Serial.println("[STREAM] Cliente desconectado");

            // Desinicializar camara para liberar I2S DMA (evitar TG1WDT)
            if (self->_camera && self->_camera->isInitialized()) {
                self->_camera->deinit();
                Serial.println("[STREAM] Camara deiniciada (sin clientes)");
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
