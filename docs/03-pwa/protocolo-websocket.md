# Protocolo WebSocket - CamaraEspia-ESP32S3

## Conexion

- **URL**: `ws://192.168.4.1/ws`
- **Transporte**: WebSocket nativo del navegador
- **Primer mensaje**: El cliente DEBE enviar un mensaje `auth` dentro de los primeros 3 segundos
- **Autenticacion**: Token JWT en el campo `token` del mensaje auth
- **Si no se autentica en 3s**: el servidor cierra la conexion con codigo 4001

## Formato de Mensaje

Todos los mensajes son JSON plano (sin anidamiento). Cada mensaje tiene un campo `tipo` que identifica el tipo de mensaje.

```json
{
  "tipo": "nombre_tipo",
  "campo1": "valor",
  "campo2": 123
}
```

Los campos listados para cada tipo son **obligatorios**. Enviar campos faltantes causa error del servidor.

---

## Mensajes Cliente -> Servidor

### auth
Autenticacion inicial. Debe ser el primer mensaje enviado.

```json
{
  "tipo": "auth",
  "token": "eyJhbGciOiJIUzI1NiIs..."
}
```

### servo
Mover servo a un angulo especifico.

```json
{
  "tipo": "servo",
  "angle": 90
}
```

- `angle`: entero, rango 0-180

### config
Cambiar configuracion del sistema.

```json
{
  "tipo": "config",
  "mode": "motion",
  "resolution": "hd",
  "quality": 80
}
```

- `mode`: string, valores posibles: `"always"`, `"motion"`
- `resolution`: string, valores posibles: `"vga"`, `"hd"`, `"3mp"`
- `quality`: entero, rango 1-100 (compresion JPEG)

### status_request
Solicitar estado actual del ESP32.

```json
{
  "tipo": "status_request"
}
```

Campos vacios, solo el tipo.

### capture
Captura manual de una foto.

```json
{
  "tipo": "capture"
}
```

### stream_start
Iniciar streaming MJPEG (notificacion al ESP32, el stream es HTTP GET).

```json
{
  "tipo": "stream_start"
}
```

### stream_stop
Detener streaming.

```json
{
  "tipo": "stream_stop"
}
```

---

## Mensajes Servidor -> Cliente

### status
Estado completo del sistema. Enviado periodicamente y en respuesta a `status_request`.

```json
{
  "tipo": "status",
  "battery_pct": 85,
  "battery_v": 3.95,
  "angle": 90,
  "mode": "motion",
  "resolution": "hd",
  "recording": false,
  "sd_free_mb": 12400,
  "uptime_s": 3600
}
```

- `battery_pct`: entero 0-100
- `battery_v`: float, voltaje actual
- `angle`: entero 0-180, angulo actual del servo
- `mode`: string `"always"` o `"motion"`
- `resolution`: string `"vga"`, `"hd"`, `"3mp"`
- `recording`: boolean, true si esta grabando
- `sd_free_mb`: enterto, espacio libre en MB
- `uptime_s`: entero, segundos desde ultimo boot

### pir_alert
El sensor PIR detecto movimiento.

```json
{
  "tipo": "pir_alert",
  "timestamp": 1700000000,
  "angle_servo": 45
}
```

- `timestamp`: entero, Unix timestamp (segundos)
- `angle_servo`: entero 0-180, angulo del servo al momento de deteccion

### recording_event
Evento de grabacion iniciada, detenida o con error.

```json
{
  "tipo": "recording_event",
  "event": "start",
  "filename": "clip_20240115_143022.avi"
}
```

- `event`: string, valores: `"start"`, `"stop"`, `"error"`
- `filename`: string, nombre del archivo de grabacion

### config_ack
Confirmacion de cambio de configuracion.

```json
{
  "tipo": "config_ack",
  "mode": "motion",
  "resolution": "hd",
  "quality": 80
}
```

### error
Error del servidor.

```json
{
  "tipo": "error",
  "message": "SD card no detectada",
  "code": 1001
}
```

- `message`: string, descripcion legible del error
- `code`: entero, codigo de error

### servo_ack
Confirmacion de movimiento de servo.

```json
{
  "tipo": "servo_ack",
  "angle": 90
}
```

---

## Keep-Alive

- El servidor envia un **ping** cada **5 segundos**
- El cliente debe responder con un **pong** automaticamente (handled by WebSocket protocol)
- Si el servidor envia 3 pings consecutivos sin recibir pong:
  - Cierra la conexion con codigo 4002
  - El cliente debe iniciar reconexion

## Reconexion

### Estrategia de Backoff Exponencial

| Intento | Espera antes de reintentar |
|---------|---------------------------|
| 1       | 3 segundos                |
| 2       | 6 segundos                |
| 3       | 12 segundos               |
| 4       | 24 segundos               |
| 5+      | 30 segundos (maximo)      |

### Proceso de reconexion

1. WebSocket se cierra (onclose o.onerror)
2. Esperar tiempo de backoff
3. Mostrar en UI: banner "Conexion perdida - Reconectando..."
4. Intentar conectar nuevamente a `ws://192.168.4.1/ws`
5. Enviar mensaje `auth` con token almacenado
6. Si auth exitoso: ocultar banner, reanudar operaciones
7. Si auth falla (4001): redirigir a login
8. Si conexion falla: incrementar backoff, volver al paso 2
9. Reset de backoff a 3s cuando la conexion se restablece

### Estados de UI durante desconexion

- **Banner offline**: visible debajo del header, fondo rojo
- **Controles**: deshabilitados (opacity 0.5)
- **Stream**: se mantiene ultima imagen con overlay oscuro
- **Al reconectar**: ocultar banner, re-habilitar controles, refrescar estado
