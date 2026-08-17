# Maquina de Estados - CamaraEspia ESP32-S3

## Descripcion general

El firmware implementa una maquina de estados finite (FSM) con 7 estados. Cada estado define que perifericos estan activos, que tareas se ejecutan, y bajo que condiciones se transita a otro estado.

---

## Estados definidos

### 1. BOOT_DIAG

**Descripcion**: Estado inicial al alimentar o resetear el dispositivo.

**Perifericos activos**: WiFi AP, LED (parpadeo rapido), Serial (debug).

**Comportamiento**:
- Al iniciar, el sistema verifica si el boton BOOT (GPIO0) esta presionado.
- Si el boton esta presionado y se mantiene 5 segundos: permanece en este estado. Se levanta el AP WiFi en 192.168.4.1 y se sirve la pagina `/diag` con tests de hardware.
- Si el boton no esta presionado: transiciona inmediatamente a `AWAKE_IDLE`.
- Si hay un flag de WDT reset previo en NVS: transiciona a `SAFE_MODE`.

**Condicion de salida**:
- Boton BOOT liberado sin 5s presionado → `AWAKE_IDLE`
- Boton BOOT mantenido 5s → `BOOT_DIAG` (permanece)
- Flag WDT previo detectado → `SAFE_MODE`

### 2. SLEEP

**Descripcion**: Light Sleep con consumo minimo. Solo el dominio RTC permanece activo.

**Perifericos activos**: RTC domain, interrupciones GPIO (GPIO1 PIR, GPIO0 BOOT).

**Perifericos apagados**: WiFi, UART, LEDC (servo), SPI (SD), Camara, LED, I2S, Bluetooth.

**Comportamiento**:
- El sistema esta dormido, consumiendo ~12-18mA.
- Las interrupciones GPIO estan habilitadas como fuentes de wake.
- Al detectar PIR (GPIO1 HIGH) o BOTON (GPIO0 LOW), el sistema despierta en ~10ms.
- Al despertar, se restaura el estado desde NVS y se re-inicializan los perifericos.

**Condicion de salida**:
- PIR detecta movimiento (GPIO1 HIGH) → `AWAKE_IDLE`
- Boton BOOT presionado (GPIO0 LOW) → `AWAKE_IDLE`

### 3. AWAKE_IDLE

**Descripcion**: Sistema despierto y operativo, pero sin estar grabando ni transmitiendo.

**Perifericos activos**: WiFi AP, Camara (en modo bajo consumo o standby), SD (montada), LED (parpadeo lento), servo posicionado.

**Comportamiento**:
- WiFi AP activo, aceptando conexiones de clientes.
- Camara en modo standby o capturando a baja tasa (1fps) para pre-buffer en PSRAM.
- Timer interno de inactividad corriendo (30 segundos).
- Procesando peticiones HTTP/WebSocket entrantes.
- Esperando eventos PIR para iniciar grabacion segun modo configurado.

**Condicion de salida**:
- Sin actividad por 30 segundos → `SLEEP`
- Cliente conecta `/stream` → `STREAMING`
- PIR trigger + modo "Solo Movimiento" → `RECORDING`
- PIR trigger + modo "Grabar Todo" → `RECORDING`

### 4. STREAMING

**Descripcion**: Transmision MJPEG activa por HTTP/WebSocket a un cliente conectado.

**Perifericos activos**: WiFi AP, Camara (resolucion completa, framerate alto), SD (montada pero no escribiendo), LED (encendido fijo).

**Comportamiento**:
- Camara configurada para frames continuos a la resolucion seleccionada.
- Frames JPEG enviados via multipart stream o WebSocket.
- Timer de timeout activo (60 segundos sin frames enviados).
- El pre-buffer en PSRAM se mantiene activo para posibles grabaciones simultaneas.

**Condicion de salida**:
- Cliente se desconecta → `AWAKE_IDLE`
- Timeout 60 segundos sin frames → `AWAKE_IDLE`
- PIR trigger + modo "Grabar Todo" → `RECORDING_STREAMING`
- WDT timeout → `SAFE_MODE`

### 5. RECORDING

**Descripcion**: Grabando clips AVI a tarjeta SD.

**Perifericos activos**: WiFi AP (puede estar conectado o no), Camara (resolucion de grabacion), SD (escritura activa), LED (parpadeo rapido durante escritura).

**Comportamiento**:
- Frames JPEG escritos a archivo .tmp en SD.
- Pre-buffer de 2 segundos (capturado antes del trigger) se vacia al inicio del clip.
- Grabacion continua mientras PIR detecte movimiento (o hasta maximo 5 minutos en "Grabar Todo").
- Cooldown de 3 segundos despues de que PIR se pone LOW antes de cerrar clip.
- Auto-rotacion de espacio: si SD < 500MB libres, borrar clips antiguos no marcados como importantes.

**Condicion de salida**:
- Clip completo (5min o fin de movimiento + cooldown) → `AWAKE_IDLE`
- Cliente conecta stream → `RECORDING_STREAMING`
- WDT timeout → `SAFE_MODE`

### 6. RECORDING_STREAMING

**Descripcion**: Grabacion y streaming simultaneos. La camara alimenta tanto al archivo SD como al stream de red.

**Perifericos activos**: Todos los de RECORDING + STREAMING.

**Comportamiento**:
- Frames capturados por la camara se duplican: uno va al archivo .tmp en SD, otro al stream de red.
- La SD y el WiFi comparten tiempo de CPU. Prioridad: SD write > WiFi send.
- Si la SD esta lenta (latencia alta), se reduce la calidad del stream para mantener la grabacion.
- Si WiFi se degrada, la grabacion continua sin interrupcion.

**Condicion de salida**:
- Cliente se desconecta → `RECORDING`
- Grabacion completa → `AWAKE_IDLE`
- WDT timeout → `SAFE_MODE`

### 7. SAFE_MODE

**Descripcion**: Modo seguro tras un reset por watchdog timer (WDT). Solo diagnostico basico.

**Perifericos activos**: WiFi AP, LED (parpadeo muy rapido), Serial (debug extenso).

**Comportamiento**:
- AP WiFi activo en 192.168.4.1 con pagina de diagnostico.
- Camara, SD y servo deshabilitados.
- Permite lectura de logs de crash, actualizacion OTA forzada, o reinicio manual.
- Se activa automaticamente si el firmware se colgo (WDT reset).

**Condicion de salida**:
- OTA exitoso → reinicio → `BOOT_DIAG` → `AWAKE_IDLE`
- Reinicio manual via `/api/restart` → `BOOT_DIAG`

---

## Diagrama de estados

```
                    ┌──────────────────┐
                    │                  │
        ┌──────────│    BOOT_DIAG     │◄──────────┐
        │          │                  │            │
        │          └────────┬─────────┘            │
        │                   │                      │
        │        ┌──────────┴──────────┐           │
        │        │  GPIO0 sin 5s       │           │
        │        │  + sin flag WDT     │           │
        │        ▼                     │           │
        │  ┌─────────────┐     Flag WDT│    OTA exitoso
        │  │             │        │    │    o restart
        │  │ AWAKE_IDLE  │        ▼    │           │
        │  │             │ ┌──────────┐│           │
        │  │  WiFi ON    │ │SAFE_MODE ││           │
        │  │  Camara     │ │  WiFi ON ││           │
        │  │  SD montada │ │  Solo    ││           │
        │  └──┬──┬──┬──┬─┘ │  diag   ││           │
        │     │  │  │  │   └────┬────┘│           │
        │     │  │  │  │        │     │           │
        │     │  │  │  │  WDT   │     │           │
        │     │  │  │  │  timeout│     │           │
        │     │  │  │  │        │     │           │
        │     │  │  │  └──────┐ │     │           │
        │     │  │  │         │ │     │           │
        │     │  │  │  30s    │ │     │           │
        │     │  │  │  inactiv│ │     │           │
        │     │  │  │         ▼ │     │           │
        │     │  │  │   ┌────────┐   │           │
        │     │  │  │   │ SLEEP  │   │           │
        │     │  │  │   │ ~15mA  │   │           │
        │     │  │  │   └───┬────┘   │           │
        │     │  │  │       │        │           │
        │     │  │  │  PIR  │  BOOT  │           │
        │     │  │  │  HIGH │  LOW   │           │
        │     │  │  │       ▼        │           │
        │     │  │  └──►AWAKE_IDLE   │           │
        │     │  │                   │           │
        │     │  │  /stream ──────┐  │           │
        │     │  │               │  │           │
        │     │  │               ▼  │           │
        │     │  │        ┌─────────────┐        │
        │     │  │        │  STREAMING  │        │
        │     │  │        │  MJPEG out  │        │
        │     │  │        └──┬──────┬───┘        │
        │     │  │           │      │            │
        │     │  │  disconnect│  PIR │            │
        │     │  │           │  +   │            │
        │     │  │           │  grabar│           │
        │     │  │           ▼       ▼           │
        │     │  │  ◄──AWAKE_IDLE   ┌─────────────────┐
        │     │  │                   │RECORDING_       │
        │     │  │  PIR + grabar     │STREAMING        │
        │     │  │  ────────────►    │  SD + WiFi      │
        │     │  │                   └──┬──────────┬───┘
        │     │  │                      │          │
        │     │  │  ┌───────────────┐   │  disconnect│
        │     │  │  │  RECORDING    │◄──┘  stream   │
        │     │  │  │  SD write     │              │
        │     │  │  └───────┬───────┘              │
        │     │  │          │                      │
        │     │  │   clip   │  /stream             │
        │     │  │   finish │  conecta             │
        │     │  │          ▼                      │
        │     │  └──►AWAKE_IDLE                   │
        │     │                                    │
        │     │     Cualquier estado:              │
        │     │     WDT timeout ──────► SAFE_MODE  │
        │     │                                    │
        └─────┘                                    │
                                                   │
                    Reinicio manual ───────────────┘
```

---

## Transiciones detalladas

| Desde | Hasta | Condicion | Accion |
|---|---|---|---|
| BOOT_DIAG | AWAKE_IDLE | GPIO0 liberado sin 5s, sin flag WDT | Iniciar WiFi AP, montar SD |
| BOOT_DIAG | SAFE_MODE | Flag WDT previo en NVS | Modo diagnostico |
| SLEEP | AWAKE_IDLE | GPIO1 HIGH (PIR) o GPIO0 LOW (BOOT) | Restaurar perifericos desde NVS |
| AWAKE_IDLE | SLEEP | Timer 30s sin actividad | Guardar estado, destruir perifericos, sleep |
| AWAKE_IDLE | STREAMING | Cliente GET /stream | Iniciar camara, enviar frames |
| AWAKE_IDLE | RECORDING | PIR trigger + modo grabar | Iniciar clip AVI |
| STREAMING | AWAKE_IDLE | Cliente desconectado o timeout 60s | Detener stream |
| STREAMING | RECORDING_STREAMING | PIR trigger + modo grabar | Iniciar grabacion + mantener stream |
| RECORDING | AWAKE_IDLE | Clip completo (5min o cooldown 3s) | Cerrar AVI, renombrar .tmp |
| RECORDING | RECORDING_STREAMING | Cliente conecta /stream | Agregar stream a grabacion activa |
| RECORDING_STREAMING | RECORDING | Cliente se desconecta | Mantener grabacion |
| RECORDING_STREAMING | AWAKE_IDLE | Clip completo | Cerrar AVI, detener ambos |
| CUALQUIER | SAFE_MODE | WDT timeout | Log de crash, modo seguro |
| SAFE_MODE | BOOT_DIAG | OTA exitoso o restart manual | Reinicio limpio |

---

## Flags de estado

El sistema utiliza una estructura de flags en NVS para persistir el estado entre ciclos de sleep:

```cpp
struct SystemState {
    uint8_t current_state;      // Estado actual (enum State)
    uint8_t capture_mode;       // 0=solo_movimiento, 1=grabar_todo
    uint8_t resolution;         // 0=VGA, 1=HD, 2=MP3
    uint8_t servo_angle;        // 0-180 grados
    uint8_t quality;            // 1-63 (JPEG quality)
    bool is_recording;          // Grabando actualmente
    bool is_streaming;          // Streaming actualmente
    uint32_t last_activity_ts;  // Timestamp de ultima actividad
    uint8_t wdt_flag;           // Flag de WDT reset previo
    char auth_token[64];        // Token JWT para autenticacion
};
```

---

## Timer de inactividad

Cada estado activo tiene un timer asociado:

| Estado | Timer | Duracion | Accion al expirar |
|---|---|---|---|
| AWAKE_IDLE | sleep_timer | 30 segundos | Transicion a SLEEP |
| STREAMING | stream_timeout | 60 segundos | Transicion a AWAKE_IDLE |
| RECORDING | clip_max_duration | 5 minutos | Cerrar clip, transicion a AWAKE_IDLE |
| RECORDING | pir_cooldown | 3 segundos | Cerrar clip si PIR LOW |
| BOOT_DIAG | boot_timeout | 5 segundos | Transicion a AWAKE_IDLE |

---

## Watchdog Timer (WDT)

El firmware configura el Task WDT (TWDT) con timeout de 10 segundos:

- Si el loop principal no se ejecuta en 10 segundos, el TWDT genera un reset.
- Antes del reset, se escribe el flag WDT en NVS con la informacion del crash.
- Al reiniciar, `BOOT_DIAG` detecta el flag y entra en `SAFE_MODE`.
- El usuario puede recuperar el dispositivo via OTA o reinicio manual.

```cpp
// Configuracion del WDT
esp_task_wdt_config_t wdt_config = {
    .timeout_ms = 10000,          // 10 segundos
    .idle_core_mask = 0,          // No vigilar tareas idle
    .trigger_panic = true         // Reset en vez de panic
};
esp_task_wdt_reconfigure(&wdt_config);
```
