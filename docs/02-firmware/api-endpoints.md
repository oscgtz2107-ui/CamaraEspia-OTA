# API Endpoints - CamaraEspia ESP32-S3

## Informacion general

- **Base URL**: `http://192.168.4.1` (WiFi AP mode)
- **Formato**: JSON (excepto `/stream`, `/api/video/{name}`, `/api/thumb/{name}`, `/diag`)
- **Autenticacion**: JWT token en header `Authorization: Bearer <token>` para endpoints `/api/*`
- **Puerto**: 80 (HTTP)

---

## 1. GET /stream

**Descripcion**: Transmision MJPEG en tiempo real via HTTP multipart.

**Headers**:

```
Accept: multipart/x-mixed-replace
```

**Response 200**:

```
Content-Type: multipart/x-mixed-replace; boundary=frame

--frame
Content-Type: image/jpeg
Content-Length: 28453

[JPEG bytes]
--frame
Content-Type: image/jpeg
Content-Length: 29102

[JPEG bytes]
...
```

**Ejemplo curl**:

```bash
curl -o stream.mjpeg http://192.168.4.1/stream
```

**Notas**:
- El stream continua hasta que el cliente se desconecte o expire el timeout de 60 segundos.
- Cada frame es un chunk independiente con su propio Content-Length.
- Compatible con navegadores web y reproductores VLC.
- Si hay una grabacion activa, el stream se ejecuta simultaneamente.

---

## 2. POST /api/servo

**Descripcion**: Mover el servo a un angulo especifico.

**Headers**:

```
Content-Type: application/json
Authorization: Bearer <token>
```

**Body**:

```json
{
  "angle": 90
}
```

| Campo | Tipo | Rango | Descripcion |
|---|---|---|---|
| angle | integer | 0-180 | Angulo en grados |

**Response 200**:

```json
{
  "ok": true,
  "angle": 90
}
```

**Response 400** (angle fuera de rango):

```json
{
  "ok": false,
  "error": "Angle must be between 0 and 180"
}
```

**Response 401** (sin autenticacion):

```json
{
  "ok": false,
  "error": "Unauthorized"
}
```

**Ejemplo curl**:

```bash
curl -X POST http://192.168.4.1/api/servo \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer eyJhbGc..." \
  -d '{"angle": 45}'
```

---

## 3. GET /api/status

**Descripcion**: Obtener el estado completo del sistema.

**Headers**:

```
Authorization: Bearer <token>
```

**Response 200**:

```json
{
  "battery_pct": 85,
  "battery_v": 3.89,
  "angle": 90,
  "mode": "solo_movimiento",
  "resolution": "vga",
  "sd_free_mb": 12400,
  "sd_total_mb": 15200,
  "recording": false,
  "streaming": true,
  "wifi_clients": 1,
  "uptime_s": 3600
}
```

| Campo | Tipo | Descripcion |
|---|---|---|
| battery_pct | integer | Porcentaje de bateria (0-100) |
| battery_v | float | Voltaje de bateria (3.0-4.2V) |
| angle | integer | Angulo actual del servo (0-180) |
| mode | string | "solo_movimiento" o "grabar_todo" |
| resolution | string | "vga", "hd", o "mp3" |
| sd_free_mb | integer | MB libres en SD |
| sd_total_mb | integer | MB totales en SD |
| recording | boolean | Grabando actualmente |
| streaming | boolean | Transmitiendo actualmente |
| wifi_clients | integer | Numero de clientes WiFi conectados |
| uptime_s | integer | Tiempo de actividad en segundos |

**Ejemplo curl**:

```bash
curl -H "Authorization: Bearer eyJhbGc..." \
  http://192.168.4.1/api/status
```

---

## 4. GET /api/videos

**Descripcion**: Listar todos los videos almacenados en la SD.

**Headers**:

```
Authorization: Bearer <token>
```

**Response 200**:

```json
{
  "videos": [
    {
      "name": "2024-01-15_14-30-00.avi",
      "size": 12500000,
      "date": "2024-01-15T14:30:00",
      "duration": 30,
      "important": false
    },
    {
      "name": "2024-01-15_14-45-00.avi",
      "size": 8200000,
      "date": "2024-01-15T14:45:00",
      "duration": 15,
      "important": true
    }
  ]
}
```

| Campo | Tipo | Descripcion |
|---|---|---|
| name | string | Nombre del archivo AVI |
| size | integer | Tamano en bytes |
| date | string | Fecha ISO 8601 de creacion |
| duration | integer | Duracion en segundos |
| important | boolean | Marcado como importante |

**Ejemplo curl**:

```bash
curl -H "Authorization: Bearer eyJhbGc..." \
  http://192.168.4.1/api/videos
```

---

## 5. GET /api/video/{name}

**Descripcion**: Descargar un video especifico. Streaming de respuesta.

**Headers**:

```
Authorization: Bearer <token>
```

**Response 200**:

```
Content-Type: video/avi
Content-Length: 12500000
Content-Disposition: attachment; filename="2024-01-15_14-30-00.avi"

[AVI bytes]
```

**Response 404** (archivo no encontrado):

```json
{
  "ok": false,
  "error": "Video not found"
}
```

**Ejemplo curl**:

```bash
curl -H "Authorization: Bearer eyJhbGc..." \
  -o video.avi \
  http://192.168.4.1/api/video/2024-01-15_14-30-00.avi
```

**Notas**:
- La respuesta se envia como stream para archivos grandes.
- Compatible con descarga directa en navegadores.
- El nombre del archivo se codifica en la URL (URL encoding para caracteres especiales).

---

## 6. POST /api/config

**Descripcion**: Configurar parametros de captura.

**Headers**:

```
Content-Type: application/json
Authorization: Bearer <token>
```

**Body**:

```json
{
  "mode": "solo_movimiento",
  "resolution": "vga",
  "quality": 10
}
```

| Campo | Tipo | Rango/Valores | Descripcion |
|---|---|---|---|
| mode | string | "solo_movimiento", "grabar_todo" | Modo de grabacion |
| resolution | string | "vga", "hd", "mp3" | Resolucion de captura |
| quality | integer | 1-63 | Calidad JPEG (1=mejor, 63=peor) |

**Resoluciones disponibles**:

| Nombre | Resolucion | FPS estimado | Tamano frame |
|---|---|---|---|
| vga | 640x480 | 10-15 fps | ~20-40KB |
| hd | 1280x720 | 5-10 fps | ~60-120KB |
| mp3 | 1600x1200 | 2-5 fps | ~100-200KB |

**Response 200**:

```json
{
  "ok": true,
  "config": {
    "mode": "solo_movimiento",
    "resolution": "vga",
    "quality": 10
  }
}
```

**Response 400** (parametro invalido):

```json
{
  "ok": false,
  "error": "Invalid resolution: 'xga'"
}
```

**Ejemplo curl**:

```bash
curl -X POST http://192.168.4.1/api/config \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer eyJhbGc..." \
  -d '{"resolution": "hd", "quality": 8}'
```

---

## 7. GET /api/thumb/{name}

**Descripcion**: Obtener la miniatura (primer frame) de un video.

**Headers**:

```
Authorization: Bearer <token>
```

**Response 200**:

```
Content-Type: image/jpeg
Content-Length: 28453

[JPEG bytes del primer frame]
```

**Response 404** (video no encontrado):

```json
{
  "ok": false,
  "error": "Video not found"
}
```

**Ejemplo curl**:

```bash
curl -H "Authorization: Bearer eyJhbGc..." \
  -o thumb.jpg \
  http://192.168.4.1/api/thumb/2024-01-15_14-30-00.avi
```

**Notas**:
- Extrae el primer frame JPEG del archivo AVI.
- Util para previsualizacion en la interfaz web.
- Respuesta rapida (~50ms) ya que solo lee el inicio del archivo.

---

## 8. DELETE /api/video/{name}

**Descripcion**: Eliminar un video de la SD.

**Headers**:

```
Authorization: Bearer <token>
```

**Response 200**:

```json
{
  "ok": true,
  "deleted": "2024-01-15_14-30-00.avi"
}
```

**Response 404** (video no encontrado):

```json
{
  "ok": false,
  "error": "Video not found"
}
```

**Response 403** (video importante):

```json
{
  "ok": false,
  "error": "Cannot delete important video. Remove importance flag first."
}
```

**Ejemplo curl**:

```bash
curl -X DELETE -H "Authorization: Bearer eyJhbGc..." \
  http://192.168.4.1/api/video/2024-01-15_14-30-00.avi
```

**Notas**:
- No se pueden eliminar clips marcados como "IMPORTANTE" sin antes desmarcarlos.
- La eliminacion es inmediata (no hay papelera).
- Se actualiza el indice de clips en la SD.

---

## 9. POST /api/video/{name}/important

**Descripcion**: Marcar o desmarcar un video como importante.

**Headers**:

```
Content-Type: application/json
Authorization: Bearer <token>
```

**Body**:

```json
{
  "important": true
}
```

**Response 200**:

```json
{
  "ok": true,
  "name": "2024-01-15_14-30-00.avi",
  "important": true
}
```

**Response 404**:

```json
{
  "ok": false,
  "error": "Video not found"
}
```

**Ejemplo curl**:

```bash
curl -X POST http://192.168.4.1/api/video/2024-01-15_14-30-00.avi/important \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer eyJhbGc..." \
  -d '{"important": true}'
```

**Notas**:
- Los clips importantes no se eliminan automaticamente cuando la SD se llena.
- Se pueden marcar multiples clips como importantes.

---

## 10. POST /api/auth/login

**Descripcion**: Autenticar usuario y obtener token JWT.

**Headers**:

```
Content-Type: application/json
```

**Body**:

```json
{
  "user": "admin",
  "pass": "secreto123"
}
```

| Campo | Tipo | Descripcion |
|---|---|---|
| user | string | Nombre de usuario |
| pass | string | Contrasena |

**Response 200**:

```json
{
  "ok": true,
  "token": "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9...",
  "expires_in": 86400
}
```

**Response 401** (credenciales invalidas):

```json
{
  "ok": false,
  "error": "Invalid credentials"
}
```

**Ejemplo curl**:

```bash
curl -X POST http://192.168.4.1/api/auth/login \
  -H "Content-Type: application/json" \
  -d '{"user": "admin", "pass": "secreto123"}'
```

**Notas**:
- El token JWT tiene un tiempo de expiracion de 24 horas.
- Las credenciales se almacenan en NVS (cifradas).
- Despues del login, usar el token en todos los endpoints `/api/*`.
- El token se invalida si se cambia la contrasena.

---

## 11. POST /api/ota

**Descripcion**: Actualizar firmware via OTA (Over-The-Air).

**Headers**:

```
Content-Type: application/octet-stream
X-SHA256: <hash_del_firmware>
Authorization: Bearer <token>
```

**Body**:

```
[firmware.bin bytes]
```

**Response 200** (iniciado):

```json
{
  "ok": true,
  "message": "OTA update started",
  "progress_url": "/api/ota/progress"
}
```

**Response 200** (completado):

```json
{
  "ok": true,
  "message": "OTA update successful. Rebooting in 5 seconds.",
  "new_version": "1.2.0"
}
```

**Response 400** (SHA256 no coincide):

```json
{
  "ok": false,
  "error": "SHA256 mismatch. Expected: abc123..., Got: def456..."
}
```

**Response 500** (error de escritura):

```json
{
  "ok": false,
  "error": "Failed to write to partition app1"
}
```

**Ejemplo curl**:

```bash
sha256sum firmware.bin

curl -X POST http://192.168.4.1/api/ota \
  -H "Content-Type: application/octet-stream" \
  -H "X-SHA256: abc123def456..." \
  -H "Authorization: Bearer eyJhbGc..." \
  --data-binary @firmware.bin
```

**Proceso OTA**:
1. Se recibe el firmware completo y se escribe a la particion alternativa (app1).
2. Se verifica SHA256 del firmware recibido contra el header.
3. Si la verificacion es exitosa, se marca app1 como boot activa.
4. El sistema se reinicia automaticamente en 5 segundos.
5. Si falla, el sistema mantiene la particion actual (app0) como boot activa.

**Notas**:
- El firmware se escribe a la particion inactiva (app1 si actual esta en app0).
- Se verifica SHA256 antes de marcar como boot activa.
- Si la verificacion falla, la particion alternativa se descarta.
- El reinicio es automatico tras 5 segundos de confirmacion.

---

## 12. GET /diag

**Descripcion**: Pagina HTML de diagnostico con tests de hardware.

**Headers**: Ninguno (acceso sin autenticacion).

**Response 200**:

```
Content-Type: text/html

[HTML con pagina de diagnostico]
```

**Contenido de la pagina**:
- Test de camara (inicializar, capturar frame, verificar resolucion)
- Test de SD (montar, escribir, leer, eliminar archivo temporal)
- Test de servo (mover a 0, 90, 180 grados)
- Test de PIR (leer estado actual del GPIO1)
- Test de WiFi (info de clientes conectados)
- Test de bateria (lectura ADC del GPIO3)
- Estado de particiones flash (uso de NVS, app0, app1, SPIFFS)
- Log de errores recientes (ultimos 10 eventos)
- Boton de reinicio a modo normal

**Ejemplo curl**:

```bash
curl http://192.168.4.1/diag
```

**Notas**:
- Solo accesible cuando el dispositivo esta en modo `BOOT_DIAG` o `SAFE_MODE`.
- En modo normal, redirige a la interfaz web principal.
- Los tests de hardware son no destructivos (no borran datos).
