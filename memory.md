# Memoria del Proyecto

<!-- OCS-PLAN:START -->
## PLAN ACTUAL

# Plan: CamaraEspia-ESP32S3 — Hoja de Ruta Completa

## Visión del Producto
Cámara de seguridad ESP32-S3 con:
- **Captive portal** para setup inicial (AP mode)
- **PWA** para gestión desde la LAN
- **Telegram bot** para alertas y control remoto
- **23 idiomas** en firmware y app
- **Una cámara = un usuario** (Chat ID vinculado)

---

## FASE 1: Captive Portal (Setup Inicial)
**Objetivo**: Al encender la cámara, el usuario configura todo desde el captive portal.

### 1.1 Captive Portal Engine
- Detectar captive portal (Android/iOS responden a `/generate_204`, `/hotspot-detect.html`, etc.)
- Redirigir TODO a `192.168.4.1`
- WiFi SSID: `Camara-Setup-XXXX` (con los últimos 4 bytes MAC)

### 1.2 Panel 1 — Selección de Idioma
- 23 botones con banderas + nombre del idioma
- Guarda idioma en NVS
- Actualiza todos los textos del portal al instante

### 1.3 Panel 2 — Descargar App
- Botón grande: "Descargar App" (en el idioma seleccionado)
- Descarga el PWA (service worker + manifest + archivos)
- Link directo: `http://192.168.4.1/app/manifest.json`
- Si el navegador soporta PWA: instala automáticamente
- Si no: muestra QR o link para guardar

### 1.4 Panel 3 — Configuración del Dispositivo
- **WiFi**: Escanear redes + contraseña
- **Nombre de cámara**: Texto libre (ej: "Patio", "Sala", "Garage")
- **Contraseña de acceso**: Para proteger la cámara desde la app
- Botón: "Guardar y reiniciar"
- Todo en el idioma seleccionado

### 1.5 Persistencia
- `wifi_ssid` + `wifi_pass` en NVS
- `camera_name` en NVS
- `device_password` en NVS (hash SHA256)
- `language` en NVS
- `setup_complete` flag en NVS (para saber si ya se configuró)

### Archivos
- `docs/captive_portal.html` → 3 paneles, vanilla JS
- `docs/manifest.json` → PWA manifest
- `docs/sw.js` → Service worker básico
- `firmware/src/captive_portal.h` → Motor de captive portal
- `firmware/src/captive_portal.cpp` → Implementación

---

## FASE 2: PWA App (Gestión desde LAN)
**Objetivo**: App web completa para gestionar cámaras desde la red local.

### 2.1 Estructura de la PWA
```
docs/
├── index.html          → App principal (SPA)
├── manifest.json       → PWA manifest
├── sw.js               → Service worker (offline)
├── css/
│   └── app.css         → Estilos (light/dark theme)
├── js/
│   ├── app.js          → Router principal
│   ├── i18n.js         → 23 idiomas (mismos del firmware)
│   ├── discovery.js    → Escaneo LAN de cámaras
│   ├── camera.js       → Panel de cámara (stream, controls)
│   ├── settings.js     → Preferencias de usuario
│   └── api.js          → Comunicación HTTP con cámaras
├── icons/              → Iconos PWA
└── img/                → Assets
```

### 2.2 Panel Bienvenida (Primera vez)
- Seleccionar idioma de la app
- Seleccionar tema (claro/oscuro/auto)
- Nombre del usuario (opcional)
- Guardar en localStorage

### 2.3 Descubrimiento de Cámaras (Discovery)
- Escanear rango de IP de la LAN (192.168.1.1-254)
- Probar `GET /api/camera/info` en cada IP
- Filtrar solo las que responden con el firmware correcto
- Mostrar lista con: nombre, IP, estado (online/offline)
- Guardar cámaras conocidas en localStorage

### 2.4 Panel de Cámara
- **Autenticación**: Pedir contraseña al acceder (guardar en sessionStorage)
- **Video en vivo**: Stream MJPEG `http://IP:81/stream`
- **Foto**: Botón para capturar foto
- **Controles**: Servo, grabación, etc.
- **Status**: Batería, WiFi, SD, cámara
- **Configuración**: 
  - Cambiar nombre
  - Cambiar contraseña
  - Configurar Chat ID (vincular con @Camara_Espia_SAEL_bot)
  - Cambiar WiFi
  - Actualizar firmware (OTA)

### 2.5 Gestión de Múltiples Cámaras
- Sidebar con lista de cámaras guardadas
- Cada cámara es una "sección" en la app
- Navegación rápida entre cámaras
- Badge de alertas por cámara

### 2.6 Preferencias de Usuario
- Idioma de la app
- Tema (claro/oscuro/auto)
- Notificaciones push (futuro)
- Eliminar cámara de la lista

### Archivos
- Todos los archivos PWA en `docs/`
- Vanilla JS (sin frameworks, como pidió el usuario)

---

## FASE 3: Firmware — Endpoints para la PWA
**Objetivo**: API completa para que la PWA pueda gestionar las cámaras.

### 3.1 Endpoints Nuevos
```
GET  /api/camera/info      → Nombre, modelo, firmware, MAC, estado
GET  /api/camera/status    → Batería, WiFi, SD, cámara, servo, PIR
POST /api/auth             → {password} → {token} para requests autenticados
GET  /api/stream           → Stream MJPEG (requiere auth)
GET  /api/capture          → Foto JPEG (requiere auth)
POST /api/config/wifi      → Cambiar WiFi
POST /api/config/name      → Cambiar nombre
POST /api/config/password  → Cambiar contraseña
POST /api/config/telegram  → Configurar Chat ID
POST /api/firmware/update  → OTA update
GET  /api/firmware/version → Versión actual
```

### 3.2 Autenticación por Token
- POST /api/auth con device_password → retorna JWT o token simple
- Todos los endpoints (excepto info) requieren token en header
- Token expira en 24h
- Guardar token en la PWA (localStorage)

### 3.3 OTA Updates
- Endpoint para recibir firmware binario
- Verificar hash SHA256
- Flashear con esp_ota
- Reboot

### Archivos
- `firmware/src/api_auth.h/cpp` → Autenticación
- `firmware/src/api_ota.h/cpp` → OTA updates

---

## FASE 4: Telegram Bot — Integración Completa
**Objetivo**: Bot funcional con vinculación desde la PWA.

### 4.1 Flujo de Vinculación (desde la PWA)
1. Usuario abre @Camara_Espia_SAEL_bot en Telegram
2. Envía `/start` → bot responde: "Tu Chat ID es: XXXXX"
3. En la PWA, sección "Telegram" de la cámara
4. Usuario pega su Chat ID
5. PWA envía `POST /api/config/telegram {chat_id: XXXXX}`
6. Cámara guarda en NVS y confirma

### 4.2 Mensajes del Bot
- Alertas de movimiento: "🚨 Movimiento en: Patio"
- Comandos: /foto, /stream, /status, /grabar
- Solo responde al owner (Chat ID guardado)
- Inline keyboards con 23 idiomas

### 4.3 Multi-Cámara
- Cada cámara envía alertas a su owner
- Si un usuario tiene 3 cámaras, recibe 3 alertas
- Las alertas incluyen el nombre de la cámara
- La app agrupa alertas por cámara

### Archivos
- `firmware/src/telegram_bot.h/cpp` → Ya implementado, ajustar
- `docs/app/js/telegram.js` → Panel de Telegram en la PWA

---

## FASE 5: Seguridad
**Objetivo**: Protección contra acceso no autorizado.

### 5.1 Seguridad Local
- Contraseña de cámara (SHA256 en NVS)
- Token de sesión por cámara (expira 24h)
- Rate limiting en endpoints sensibles

### 5.2 Seguridad Telegram
- Solo Chat ID owner puede comandar
- Mensajes de extraños: ignorados silenciosamente
- Reset físico requerido para cambiar owner

### 5.3 Seguridad WiFi
- AP de setup solo activo durante configuración
- Después del setup, AP se desactiva (o queda como fallback)
- WPA2 para WiFi STA

### 5.4 OTA Security
- Verificar hash SHA256 antes de flashear
- Solo aceptar firmware firmado (futuro)

---

## FASE 6: Producción
**Objetivo**: Preparar para fabricación.

### 6.1 Factory Reset
- Botón físico (GPIO) para resetear a fábrica
- Mantener 5 segundos → borra NVS → reinicia

### 6.2 Firmware Updates OTA
- Push de firmware nuevo desde servidor
- Verificar compatibilidad de hardware

### 6.3 Monitoreo
- Conteo de cámaras activas (sin datos personales)
- Reporte de errores

---

## Recursos Actuales
| Componente | RAM | Flash | Estado |
|---|---|---|---|
| Base firmware | 15.9% | 50.6% | ✅ |
| Telegram bot | +0.5% | +2% | ✅ |
| Captive portal | +1% | +1% | Pendiente |
| PWA (en ESP32) | +2% | +5% | Pendiente |
| OTA | +0.5% | +1% | Pendiente |
| **Estimado total** | **~20%** | **~60%** | Dentro del presupuesto |

## Orden de Desarrollo Recomendado
1. **FASE 1**: Captive Portal → El usuario puede configurar la cámara
2. **FASE 3.1**: Endpoints básicos → La PWA puede comunicarse
3. **FASE 2**: PWA completa → App funcional con discovery
4. **FASE 3.2-3.3**: Auth + OTA → Seguridad y updates
5. **FASE 4**: Telegram completo → Vinculación desde PWA
6. **FASE 5**: Seguridad → Hardening
7. **FASE 6**: Producción → Factory reset, monitoreo
<!-- OCS-PLAN:END -->
