# Memoria del Proyecto

<!-- OCS-PLAN:START -->
## PLAN ACTUAL

# PLAN: Chatbot Telegram Multi-idioma con Botones

## Resumen
Reescribir `telegram_manager` como un chatbot completo: UN solo bot token para todas las cámaras, cada ESP32 guarda el chat_id de su dueño vía dashboard web. El bot hace polling directo a Telegram (sin servidor) usando una FreeRTOS task dedicada en Core 0 para no bloquear camera/PIR/stream. Inline keyboards multi-idioma (23 idiomas), notificación automática por PIR.

---

## Arquitectura corregida

```
┌─────────────────────────────────────────┐
│            ESP32-S3 (cada board)         │
│                                          │
│  Core 0: TelegramBot task                │
│    ├── getUpdates (polling cada 5s)      │
│    ├── Procesar callbacks de botones     │
│    ├── Enviar fotos/status/responses     │
│    └── Comunica con Core 1 via colas     │
│                                          │
│  Core 1: loop() principal                │
│    ├── Camera stream                     │
│    ├── PIR detection                     │
│    ├── Servo / Battery / SD              │
│    ├── Web server (PWA local)            │
│    └── Notifica a TelegramBot task       │
│         cuando PIR detecta               │
│                                          │
│  WiFi: AP+STA simultáneo                │
│    AP → control local vía PWA            │
│    STA → Telegram polling + alerts       │
└─────────────────────────────────────────┘
```

**¿Por qué FreeRTOS task?**
- `getUpdates` bloquea 1.5-3.5s (incluso con `timeout=1`)
- Si corre en loop(), bloquea camera/PIR/stream/WDT
- En Core 0, el polling no afecta al Core 1 (camera+PIR)
- Comunicación entre cores: `xQueueSend` / `xQueueReceive`

---

## FASE 1: Sistema de Idiomas (`languages.h`)

**Archivo nuevo**: `src/languages.h`

- Enum `Language` con 23 idiomas: ES, EN, PT, FR, DE, IT, RU, ZH, JA, KO, AR, HI, BN, TR, VI, TH, ID, PL, UK, MS, SW, TL, NL
- Struct `LangStrings` con ~30 campos `const char*` en PROGMEM
- Array `const LangStrings LANGS[]` indexado por el enum
- Macros de conveniencia: `TXT(lang, KEY)` → `const char*`
- Defaults: ES, fallback EN

**~30 strings × 23 idiomas ≈ 35KB Flash** (cabe en 1MB libre del app slot de 2MB)

Strings necesarios por idioma:
`WELCOME`, `MENU_PHOTO`, `MENU_STREAM`, `MENU_STATUS`, `MENU_RECORD`, `MENU_CONFIG`, `MENU_LANGUAGE`, `MENU_ALERTS`, `MENU_BACK`, `MENU_USERS`, `ALERTS_ON`, `ALERTS_OFF`, `PHOTO_TAKEN`, `STREAM_URL`, `STATUS_BATTERY`, `STATUS_SD`, `STATUS_WIFI`, `STATUS_CAMERA`, `STATUS_SERVO`, `STATUS_PIR`, `RECORD_START`, `RECORD_STOP`, `RECORDING`, `MOTION_DETECTED`, `SELECT_LANG`, `NOT_AUTHORIZED`, `USER_ADDED`, `USER_REMOVED`, `USERS_LIST`, `ERROR`

---

## FASE 2: Base de Datos de Usuarios (`user_db.h/cpp`)

**Archivos nuevos**: `src/user_db.h`, `src/user_db.cpp`

```cpp
struct TelegramUser {
    int64_t chatId;
    Language language;
    bool motionAlerts;
    bool authorized;
};
```

- Persistencia en NVS (namespace `"tg_users"`)
- Hasta 8 usuarios (~80 bytes en NVS)
- Claves: `u0_id`, `u0_lang`, `u0_alerts`, `u1_id`, etc.
- **Auto-registro**: si no hay usuarios, el primero que escriba `/start` se auto-registra como admin
- `isAuthorized(int64_t chatId)` — solo usuarios registrados pueden usar el bot
- `addUser(int64_t chatId)` — default: lang=ES, alerts=true, authorized=true
- `removeUser(int64_t chatId)` — admin puede revocar
- `setLanguage(int64_t chatId, Language lang)`
- `setMotionAlerts(int64_t chatId, bool enabled)
- `getAlertEnabledUsers()` — retorna lista de chatIds con alerts ON (para PIR notify)

---

## FASE 3: Bot Telegram (`telegram_bot.h/cpp`)

**Reemplaza**: `telegram_manager.h/cpp` (se eliminan)

### Clase TelegramBot

```cpp
class TelegramBot {
public:
    void begin(const char* botToken, const char* staSsid, const char* staPass);
    void startPolling();   // Crea FreeRTOS task en Core 0
    void stopPolling();

    // Interfaz con Core 1 (llamadas desde loop())
    void notifyMotion(const uint8_t* jpeg, size_t len);  // foto a todos con alerts ON
    bool isConnected() const;

    // Config
    void setToken(const char* token);
    const String& getToken() const;

private:
    // Polling task (Core 0)
    static void pollTask(void* param);
    void processUpdate(/* telegram json */);
    void handleCommand(int64_t chatId, const String& text);
    void handleCallback(int64_t chatId, const String& data, const String& callbackId);

    // Envío
    bool sendPhoto(int64_t chatId, const uint8_t* jpeg, size_t len, const char* caption);
    bool sendMessage(int64_t chatId, const String& text, const char* replyMarkup = nullptr);
    bool answerCallback(const char* callbackId, const char* text = nullptr);

    // Menús
    void showMainMenu(int64_t chatId);
    void showConfigMenu(int64_t chatId);
    void showLanguageMenu(int64_t chatId);
    void showRecordMenu(int64_t chatId);
    void showStatus(int64_t chatId);

    // Cola de mensajes (Core 1 → Core 0)
    QueueHandle_t _notifyQueue;
    struct MotionNotification { uint8_t* jpeg; size_t len; };
};
```

### Polling strategy (en pollTask):

```
while (running) {
    // 1. Checkear si hay notificaciones pendientes de Core 1
    if (xQueueReceive(_notifyQueue, &notif, 0) == pdTRUE) {
        // Enviar foto a TODOS los usuarios con alerts ON
        for (auto& user : userDB.getAlertEnabledUsers()) {
            sendPhoto(user.chatId, notif.jpeg, notif.len, "🚨 Movimiento detectado!");
        }
        free(notif.jpeg);
    }

    // 2. Poll Telegram (bloquea ~1-3s, pero es OK en Core 0)
    int numNew = bot.getUpdates(bot.last_message_received + 1);
    while (numNew) {
        for (int i = 0; i < numNew; i++) {
            processMessage(bot.messages[i]);
        }
        numNew = bot.getUpdates(bot.last_message_received + 1);
    }

    // 3. Adaptive delay
    delay(isUserActive ? 2000 : 10000);  // 2s si activo, 10s si idle
}
```

### Callbacks de botones (inline keyboard data):

| Callback | Acción |
|----------|--------|
| `photo` | Capturar JPEG, enviar foto al usuario |
| `stream` | Enviar URL del stream como link clickable |
| `status` | Leer battery/sd/wifi/pir/servo, enviar texto multi-idioma |
| `config` | Mostrar menú config |
| `lang_xx` | Cambiar idioma (xx = código ISO 639-1) |
| `alerts_on` | Activar alertas de movimiento |
| `alerts_off` | Desactivar alertas de movimiento |
| `rec_start` | Iniciar grabación (pide confirmación si SD no montada) |
| `rec_stop` | Detener grabación |
| `back` | Volver al menú principal |
| `usr_add` | (Admin) Auto-registrar usuario actual |
| `usr_del` | (Admin) Revocar acceso |

### Menús inline keyboards:

**Menú Principal** (`/start` o texto libre):
```
📸 Foto      📹 Stream
📊 Estado    🔴 Grabar
⚙️ Config
```

**Menú Config**:
```
🌍 Idioma    🔔 Alertas: ON/OFF
🔙 Volver
```

**Menú Idioma** (botones por fila):
```
🇪🇸 Español    🇬🇧 English    🇵🇹 Português
🇫🇷 Français   🇩🇪 Deutsch    🇮🇹 Italiano
🇷🇺 Русский    🇨🇳 中文       🇯🇵 日本語
🇰🇷 한국어      🇸🇦 عربي      🇮🇳 हिन्दी
🇧🇩 বাংলা       🇹🇷 Türkçe    🇻🇳 Tiếng Việt
🇹🇭 ไทย       🇮🇩 Bahasa    🇵🇱 Polski
🇺🇦 Українська  🇲🇾 Melayu    🇰🇪 Swahili
🇵🇭 Filipino   🇳🇱 Nederlands
```

**Menú Grabar**:
```
⏺ Iniciar    ⏹ Detener
🔙 Volver
```

---

## FASE 4: Integración con Firmware

### main.cpp:
- Reemplazar `#include "telegram_manager.h"` → `#include "telegram_bot.h"`
- Reemplazar `TelegramManager telegram` → `TelegramBot tgBot`
- `setup()`: `tgBot.begin(token, staSsid, staPass); tgBot.startPolling();`
- `loop()`: eliminar `telegram.update()` (la task se maneja sola)
- `onPIRTrigger()`: reemplazar `telegram.sendPhoto()` → `tgBot.notifyMotion(buf, len)` (envía por cola a Core 0)
- `loadConfig()`: cargar token/ssid/pass desde NVS, llamar `tgBot.begin()`

### config.h:
- Mantener `TG_COOLDOWN_MS`, `TG_SEND_ON_MOTION`
- Agregar `TG_MAX_USERS 8`
- Agregar `TG_DEFAULT_LANG LANG_ES`
- Agregar `TG_POLL_INTERVAL_ACTIVE_MS 2000`
- Agregar `TG_POLL_INTERVAL_IDLE_MS 10000`

### web_server.h/cpp:
- Endpoint `POST /api/telegram` mantiene config de token/ssid/pass
- Nuevo endpoint `GET /api/telegram/users` → lista de usuarios registrados
- Nuevo endpoint `POST /api/telegram/users/remove` → revocar usuario por chatId

### platformio.ini:
- Agregar `UniversalTelegramBot` a `lib_deps`

---

## FASE 5: Compilación y Flash

1. `pio run` → verificar compila sin errores
2. Flash manual a COM13 con esptool
3. Verificar por serial:
   - `[TG] Polling task iniciado en Core 0`
   - `[TG] STA conectado: x.x.x.x`
   - `[TG] Long poll timeout=5`
4. Probar desde Telegram real:
   - Enviar `/start` → recibir menú de botones
   - Presionar 📸 → recibir foto
   - Presionar 📊 → recibir estado
   - Presionar 🌍 → recibir menú de idiomas
   - Cambiar idioma → verificar textos en nuevo idioma
   - Activar/desactivar alertas
   - PIR trigger → verificar foto enviada a usuarios con alerts ON
5. Verificar que camera stream no se congela durante polling

---

## Archivos a crear/modificar

| Archivo | Acción |
|---------|--------|
| `src/languages.h` | **NUEVO** — 23 idiomas, ~690 strings PROGMEM |
| `src/user_db.h` | **NUEVO** — struct TelegramUser + clase UserDB |
| `src/user_db.cpp` | **NUEVO** — persistencia en NVS |
| `src/telegram_bot.h` | **NUEVO** — reemplaza telegram_manager.h |
| `src/telegram_bot.cpp` | **NUEVO** — FreeRTOS task, polling, callbacks, keyboards |
| `src/telegram_manager.h` | **ELIMINAR** |
| `src/telegram_manager.cpp` | **ELIMINAR** |
| `src/config.h` | **MODIFICAR** — TG_MAX_USERS, TG_DEFAULT_LANG, poll intervals |
| `src/main.cpp` | **MODIFICAR** — reemplazar TelegramManager→TelegramBot |
| `src/web_server.h` | **MODIFICAR** — incluir telegram_bot.h, endpoint usuarios |
| `src/web_server.cpp` | **MODIFICAR** — handlers de usuarios |
| `platformio.ini` | **MODIFICAR** — agregar UniversalTelegramBot a lib_deps |

---

## Estimación de recursos

| Recurso | Impacto |
|---------|---------|
| Flash (app) | +~150KB (librería + textos 23 idiomas + código bot) → ~58% del slot 2MB |
| RAM interna | +~4KB (UserDB, colas FreeRTOS, buffers JSON) |
| PSRAM | +~8KB (buffer getUpdates response + JPEG notificaciones) |
| CPU Core 0 | Polling HTTPS cada 2-10s + procesar callbacks. Bajo impacto. |
| CPU Core 1 | Sin cambio — camera/PIR/stream operan normalmente |
| WiFi STA | ~100-200mA adicionales (necesario para Telegram) |

**Cabe sin problema en 2MB app partition con 1MB libre.**

---

## Criterio de "hecho"

1. ✅ Compila sin errores
2. ✅ Flashea a COM13 y arranca
3. ✅ `[TG] Polling task iniciado en Core 0` aparece en serial
4. ✅ Bot responde a `/start` con menú de botones
5. ✅ Cada botón ejecuta la acción correcta
6. ✅ Cambio de idioma funciona (23 idiomas)
7. ✅ PIR → foto enviada a todos los usuarios con alerts ON
8. ✅ Camera stream NO se congela durante polling de Telegram
9. ✅ Config vía web: token/ssid/pass se guardan y persisten
10. ✅ Primer usuario se auto-registra como admin
<!-- OCS-PLAN:END -->
