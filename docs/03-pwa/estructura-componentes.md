# Estructura de Componentes - PWA

## Arbol DOM Jerarquico

```
<body>
  <div id="app">
    <!-- HEADER -->
    <header id="header">
      <div class="header-left">
        <img id="logo" src="/icons/logo.svg" alt="CamaraEspia">
      </div>
      <div class="header-right">
        <div id="battery-indicator">
          <svg id="battery-icon"></svg>
          <span id="battery-pct">100%</span>
        </div>
        <span id="wifi-ssid">MiWiFi</span>
      </div>
    </header>

    <!-- PANTALLA LOGIN -->
    <div id="login-screen" class="screen active">
      <form id="login-form">
        <img id="login-logo" src="/icons/logo.svg">
        <div class="input-group">
          <input type="text" id="input-user" autocomplete="username">
          <label for="input-user">Usuario</label>
        </div>
        <div class="input-group">
          <input type="password" id="input-pass" autocomplete="current-password">
          <label for="input-pass">Contrasena</label>
        </div>
        <button type="submit" id="btn-login">Conectar</button>
        <p id="login-error" class="error-msg" hidden></p>
      </form>
    </div>

    <!-- PANTALLA PRINCIPAL -->
    <div id="main-screen" class="screen">
      <!-- Stream -->
      <div id="stream-container">
        <img id="stream-img" src="" alt="Stream en vivo">
        <div id="rec-badge" class="rec-indicator" hidden>
          <span>REC</span>
        </div>
      </div>

      <!-- Controles -->
      <div id="controls">
        <!-- Slider Servo -->
        <div class="control-group">
          <label for="servo-slider">Servo</label>
          <div class="slider-row">
            <input type="range" id="servo-slider" min="0" max="180" value="90">
            <span id="servo-value">90</span>
          </div>
        </div>

        <!-- Botones Rapidos -->
        <div id="quick-angles">
          <button class="angle-btn" data-angle="0">0</button>
          <button class="angle-btn" data-angle="45">45</button>
          <button class="angle-btn active" data-angle="90">90</button>
          <button class="angle-btn" data-angle="135">135</button>
          <button class="angle-btn" data-angle="180">180</button>
        </div>

        <!-- Toggle Modo Grabacion -->
        <div class="control-group">
          <div class="toggle-row">
            <span id="mode-label">Solo Movimiento</span>
            <label class="toggle-switch">
              <input type="checkbox" id="toggle-mode">
              <span class="toggle-track"></span>
              <span class="toggle-thumb"></span>
            </label>
          </div>
          <div id="led-indicator" class="led off"></div>
        </div>

        <!-- Selector Resolucion -->
        <div id="resolution-selector" class="btn-group">
          <button class="res-btn active" data-res="vga">VGA</button>
          <button class="res-btn" data-res="hd">HD</button>
          <button class="res-btn" data-res="3mp">3MP</button>
        </div>
      </div>

      <!-- Boton Galeria -->
      <button id="btn-gallery" class="fab" aria-label="Galeria">
        <svg></svg>
      </button>
    </div>

    <!-- MODAL GALERIA -->
    <div id="gallery-modal" class="modal-overlay" hidden>
      <div class="modal-header">
        <h2>Galeria</h2>
        <button id="btn-close-gallery" class="btn-icon">X</button>
      </div>
      <div id="gallery-content">
        <div class="gallery-section">
          <h3 class="gallery-date">Hoy</h3>
          <div class="gallery-grid">
            <div class="gallery-thumb" data-file="clip_001.avi">
              <img src="/api/thumb/clip_001.avi" alt="Preview">
              <span class="thumb-duration">2:30</span>
              <span class="thumb-name">clip_001.avi</span>
              <span class="thumb-size">12.5 MB</span>
            </div>
          </div>
        </div>
      </div>
    </div>

    <!-- MODAL DETALLE VIDEO -->
    <div id="video-detail-modal" class="modal-overlay" hidden>
      <div class="video-detail-header">
        <button id="btn-prev-video" class="btn-nav">&lt;</button>
        <span id="video-detail-name">clip_001.avi</span>
        <button id="btn-next-video" class="btn-nav">&gt;</button>
      </div>
      <div id="video-detail-preview">
        <img id="video-detail-thumb" src="" alt="Preview">
      </div>
      <div class="video-detail-actions">
        <button id="btn-download" class="btn-action">
          <svg></svg> Descargar
        </button>
        <button id="btn-delete" class="btn-action btn-danger">
          <svg></svg> Eliminar
        </button>
        <button id="btn-favorite" class="btn-action">
          <svg></svg> Importante
        </button>
      </div>
    </div>

    <!-- PANTALLA DIAGNOSTICO -->
    <div id="diag-screen" class="screen">
      <h2>Diagnostico del Sistema</h2>
      <div id="diag-list">
        <div class="diag-item" data-module="sd">
          <span class="diag-icon"></span>
          <span class="diag-name">SD Card</span>
          <span class="diag-status">Pendiente</span>
        </div>
        <div class="diag-item" data-module="camera">
          <span class="diag-icon"></span>
          <span class="diag-name">Camera</span>
          <span class="diag-status">Pendiente</span>
        </div>
        <div class="diag-item" data-module="servo">
          <span class="diag-icon"></span>
          <span class="diag-name">Servo</span>
          <span class="diag-status">Pendiente</span>
        </div>
        <div class="diag-item" data-module="pir">
          <span class="diag-icon"></span>
          <span class="diag-name">PIR</span>
          <span class="diag-status">Pendiente</span>
        </div>
        <div class="diag-item" data-module="wifi">
          <span class="diag-icon"></span>
          <span class="diag-name">WiFi</span>
          <span class="diag-status">Pendiente</span>
        </div>
        <div class="diag-item" data-module="battery">
          <span class="diag-icon"></span>
          <span class="diag-name">Battery ADC</span>
          <span class="diag-status">Pendiente</span>
        </div>
        <div class="diag-item" data-module="ota">
          <span class="diag-icon"></span>
          <span class="diag-name">OTA</span>
          <span class="diag-status">Pendiente</span>
        </div>
      </div>
      <button id="btn-test-servo" class="btn-full">Test Servo</button>
      <div id="diag-log" class="log-container"></div>
    </div>

    <!-- OVERLAYS DE ESTADO -->
    <div id="connecting-overlay" class="state-overlay" hidden>
      <div class="spinner"></div>
      <p>Conectando al servidor...</p>
    </div>
    <div id="offline-banner" class="offline-banner" hidden>
      Conexion perdida - Reconectando...
    </div>
    <div id="low-battery-overlay" class="state-overlay critical" hidden>
      <svg class="battery-critical"></svg>
      <p>Bateria critica</p>
      <p id="low-battery-pct"></p>
      <p>El sistema se apagara pronto</p>
      <button id="btn-dismiss-battery">Entendido</button>
    </div>
  </div>
</body>
```

## Relacion JavaScript - DOM

| Modulo JS | Elementos DOM controlados |
|-----------|--------------------------|
| `app.js` | `#app`, todas las screens, overlays de estado, inicializacion global |
| `api.js` | (sin DOM directo) - maneja fetch, retorna datos a otros modulos |
| `stream.js` | `#stream-img`, `#stream-container`, eventos touch en stream |
| `servo.js` | `#servo-slider`, `#servo-value`, `.angle-btn`, `#quick-angles` |
| `gallery.js` | `#gallery-modal`, `#gallery-content`, `.gallery-thumb`, `#video-detail-modal` |
| `notifications.js` | permiso de notificacion, `#pir-alert-template` |

## Flujo de Datos

### 1. ESP32 envia estado por WebSocket

```
ESP32 --WS JSON--> app.js:onMessage()
  -> parsea JSON
  -> actualiza state global
  -> servo.js:updateSlider(angle)
  -> app.js:updateBattery(pct)
  -> app.js:updateRecording(recording)
  -> gallery.js:refreshIfOpen()
```

### 2. Usuario interactua con controles

```
Usuario mueve slider
  -> servo.js:onSliderChange(angle)
  -> debounce 150ms
  -> api.js:sendServo(angle) via WS
  -> ESP32 mueve servo fisicamente
  -> ESP32 envia servo_ack por WS
  -> servo.js:confirmAngle(angle)
```

### 3. Stream MJPEG

```
app.js:connectStream()
  -> stream-img.src = "http://192.168.4.1/stream"
  -> navegador fetch chunked JPEG
  -> renderiza cada frame en el <img>
  -> (sin JS adicional, MJPEG nativo del navegador)
```

### 4. Autenticacion

```
Usuario envia login form
  -> api.js:login(user, pass)
  -> POST /api/auth {user, pass}
  -> ESP32 retorna {token: "jwt..."}
  -> api.js:storeToken(token)
  -> api.js:connectWS(token)
  -> WS primer mensaje: {tipo: "auth", token: "jwt..."}
  -> ESP32 valida, responde status
```

## Modulos JavaScript

### Patrón: Modulo Revelado (IIFE)

Cada archivo JS usa el patron de modulo revelado:

```javascript
const NombreModulo = (function() {
  // Variables privadas
  let _state = {};

  // Funciones privadas
  function _helper() {}

  // API publica
  return {
    init: function() {},
    update: function() {}
  };
})();
```

### app.js - Inicializacion y Orquestacion
- Estado global de la aplicacion
- Manejo de screens (login, main, diag)
- Coordinacion entre modulos
- Deteccion de offline/online
- Inicializacion de todos los modulos al cargar DOM

### api.js - Comunicacion HTTP y WebSocket
- `login(user, pass)` - POST /api/auth
- `getStatus()` - GET /api/status
- `sendServo(angle)` - via WebSocket
- `sendConfig(config)` - via WebSocket
- `getVideos()` - GET /api/videos
- `downloadVideo(name)` - GET /api/video/{name}
- `deleteVideo(name)` - DELETE /api/video/{name}
- `toggleFavorite(name)` - POST /api/video/{name}/favorite
- Gestion de token JWT en memoria
- Retry automatico en errores de red

### stream.js - MJPEG y Touch
- Conexion del stream: img.src = URL
- Manejo de errores de carga
- Touch events: swipe para servo, double-tap para captura
- Reconexion automatica si stream falla
- Pausa/reanudacion del stream

### servo.js - Slider y Botones
- Event listener en slider input
- Debounce de 150ms para no saturar WS
- Sync del slider con estado recibido del ESP32
- Botones de angulos predefinidos
- Highlight del boton activo segun angulo actual

### gallery.js - Galeria de Videos
- Fetch de lista de videos desde API
- Renderizado de grid agrupado por fecha
- Lazy loading de thumbnails
- Navegacion entre videos (swipe o flechas)
- Descarga de archivos
- Eliminacion con dialogo de confirmacion
- Toggle de favorito/importante

### notifications.js - Notificaciones Navegador
- Solicitar permiso de notificacion
- Escuchar mensajes WS tipo pir_alert
- Mostrar notificacion nativa del SO
- Sound de alerta (opcional)
- Manejo de permiso denegado
