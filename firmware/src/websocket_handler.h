#ifndef WEBSOCKET_HANDLER_H
#define WEBSOCKET_HANDLER_H

#include <Arduino.h>
#include "config.h"
#include <ESPAsyncWebServer.h>

class WebSocketHandler {
public:
    WebSocketHandler();

    // Configura WebSocket server en /ws
    void begin();

    // Procesa mensajes y eventos pendientes
    void loop();

    // Envia estado del sistema a todos los clientes conectados
    void broadcastStatus(const char* json);

    // Notifica deteccion de movimiento PIR
    void sendPIRAlert();

    // Notifica evento de grabacion (inicio/fin/error)
    void sendRecordingEvent(const char* event);

    // Numero de clientes conectados al WebSocket
    int getClientCount() const;

    // Verifica si un cliente especifico esta autenticado
    bool isAuthenticated(uint32_t clientId) const;

private:
    AsyncWebSocket* ws;
    uint32_t lastPingTime;

    // Callbacks de eventos WebSocket
    void onEvent(AsyncWebSocket* server, AsyncWebSocketClient* client,
                 AwsEventType type, void* arg, uint8_t* data, size_t len);

    // Procesa mensaje entrante de un cliente
    void handleClientMessage(AsyncWebSocketClient* client,
                             const char* message, size_t len);

    // Autentica un cliente con credenciales
    bool authenticateClient(uint32_t clientId, const char* user, const char* pass);

    // Envia ping a todos los clientes
    void sendPings();
};

#endif // WEBSOCKET_HANDLER_H
