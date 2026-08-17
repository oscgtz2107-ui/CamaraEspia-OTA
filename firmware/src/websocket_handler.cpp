#include "websocket_handler.h"

WebSocketHandler::WebSocketHandler()
    : ws(nullptr)
    , lastPingTime(0) {
    // TODO: inicializar miembros
}

void WebSocketHandler::begin() {
    // TODO: ws = new AsyncWebSocket("/ws)
    // TODO: registrar callback onEvent
}

void WebSocketHandler::loop() {
    // TODO: ws->cleanupClients()
    // TODO: enviar ping si WS_PING_INTERVAL_MS ha pasado
}

void WebSocketHandler::broadcastStatus(const char* json) {
    // TODO: ws->textAll(json)
}

void WebSocketHandler::sendPIRAlert() {
    // TODO: construir JSON con alerta PIR
    // TODO: ws->textAll(json)
}

void WebSocketHandler::sendRecordingEvent(const char* event) {
    // TODO: construir JSON con evento
    // TODO: ws->textAll(json)
}

int WebSocketHandler::getClientCount() const {
    // TODO: return ws->count()
    return 0;
}

bool WebSocketHandler::isAuthenticated(uint32_t clientId) const {
    // TODO: verificar si clientId esta en lista de autenticados
    return false;
}

void WebSocketHandler::onEvent(AsyncWebSocket* server, AsyncWebSocketClient* client,
                                AwsEventType type, void* arg, uint8_t* data, size_t len) {
    // TODO: manejar WS_EVT_CONNECT, WS_EVT_DISCONNECT, WS_EVT_DATA, WS_EVT_ERROR
}

void WebSocketHandler::handleClientMessage(AsyncWebSocketClient* client,
                                            const char* message, size_t len) {
    // TODO: parsear JSON, despachar comandos (servo, grabar, etc.)
}

bool WebSocketHandler::authenticateClient(uint32_t clientId, const char* user, const char* pass) {
    // TODO: validar contra AUTH_USER_DEFAULT / AUTH_PASS_DEFAULT
    return false;
}

void WebSocketHandler::sendPings() {
    // TODO: ws->pingAll()
}
