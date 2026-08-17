# Fase 5: PWA Completa

## Objetivo
Desarrollar la PWA completa con todas las funcionalidades de UI, incluyendo login, stream, controles, galeria, notificaciones, touch, offline y instalabilidad.

## Pre-requisitos
- Fase 4 completada: servidor y firmware integrados
- Todas las APIs REST y WebSocket funcionando
- Documentacion de wireframes y estructura de componentes revisada

## Checklist

### Login
- [ ] Formulario de login con campos usuario y contrasena
- [ ] Enviar POST /api/auth con credenciales
- [ ] Si exitoso: almacenar JWT en memoria, ocultar login, mostrar pantalla principal
- [ ] Si falla: mostrar mensaje de error debajo del boton
- [ ] Manejar errores de red (servidor no disponible)
- [ ] Spinner en boton durante peticion

### Stream MJPEG
- [ ] Conectar img.src a http://192.168.4.1/stream despues de login exitoso
- [ ] Mostrar imagen en vivo en el contenedor de stream
- [ ] Manejar error de carga (imagen no disponible)
- [ ] Desconectar stream al hacer logout o cerrar sesion
- [ ] Stream ocupa 60% del viewport

### Controles de Servo
- [ ] Slider de rango 0-180 grados
- [ ] Debounce de 150ms: no enviar mensaje WS en cada cambio
- [ ] Valor numerico al lado del slider se actualiza en tiempo real
- [ ] Enviar angulo via WebSocket al soltar slider
- [ ] Sincronizar slider con angulo recibido del ESP32
- [ ] Botones rapidos: 0, 45, 90, 135, 180 grados
- [ ] Boton activo resaltado segun angulo actual
- [ ] Click en boton rapido mueve slider y envia angulo

### Indicador de Bateria
- [ ] Mostrar icono de bateria con color dinamico:
  - Verde (#2ecc71): >= 50%
  - Amarillo (#f39c12): 20-49%
  - Rojo (#e74c3c): < 20%
- [ ] Mostrar porcentaje numerico junto al icono
- [ ] Actualizar cada 30 segundos con datos del WebSocket
- [ ] Overlay de bateria critica cuando < 10%

### Toggle Modo Grabacion
- [ ] Toggle switch: "Grabar Todo" / "Solo Movimiento"
- [ ] Enviar cambio via WebSocket: {tipo: "config", mode: "always"/"motion"}
- [ ] Indicador LED visual debajo del toggle
- [ ] Sincronizar estado con ESP32

### Selector de Resolucion
- [ ] Botones segmentados: VGA, HD, 3MP
- [ ] Enviar cambio via WebSocket: {tipo: "config", resolution: "vga"/"hd"/"3mp"}
- [ ] Feedback visual: boton activo con fondo de acento
- [ ] Sincronizar con estado del ESP32

### Indicador REC
- [ ] Badge "REC" rojo durante grabacion
- [ ] Animacion de parpadeo (opacity 0 y 1 cada 1 segundo)
- [ ] Visible solo cuando recording=true en status del WS
- [ ] Oculto cuando no esta grabando

### Galeria - Lista de Videos
- [ ] Boton galeria (FAB) en esquina inferior derecha
- [ ] Al abrir: fetch GET /api/videos
- [ ] Agrupar videos por fecha: "Hoy", "Ayer", fecha completa
- [ ] Grid de thumbnails con preview, nombre, duracion, tamano
- [ ] Thumbnails cargados desde /api/thumb/{name} con lazy loading

### Galeria - Descarga
- [ ] Boton de descarga en detalle de video
- [ ] Al presionar: abrir /api/video/{name} en nueva tab (download)
- [ ] Mostrar progreso de descarga si es posible

### Galeria - Eliminacion
- [ ] Boton de eliminar en detalle de video
- [ ] Al presionar: mostrar dialogo de confirmacion
- [ ] "Eliminar video clip_001.avi? Esta accion no se puede deshacer."
- [ ] Botones: "Cancelar" y "Eliminar" (rojo)
- [ ] Si confirma: DELETE /api/video/{name}
- [ ] Actualizar lista de videos despues de eliminar

### Galeria - Favorito/Importante
- [ ] Toggle estrella en detalle de video
- [ ] Estrella rellena (#f39c12) si es importante
- [ ] Estrella vacia si no es importante
- [ ] Al presionar: POST /api/video/{name}/favorite
- [ ] Clips importantes no se borran automaticamente

### Notificaciones
- [ ] Solicitar permiso de notificacion del navegador al abrir app
- [ ] Si permiso concedido: escuchar mensajes WS tipo pir_alert
- [ ] Al recibir pir_alert: mostrar notificacion nativa del SO
- [ ] Contenido: "Movimiento detectado" + timestamp
- [ ] Si permiso denegado: no mostrar errores, solo no enviar notificaciones

### Touch en Stream
- [ ] Swipe izquierda/derecha en stream: mover servo (izq = -15grados, der = +15grados)
- [ ] Double-tap en stream: captura manual de foto
- [ ] No interferir con scroll de la pagina
- [ ] Funcional solo en dispositivos tactiles

### Reconexion WebSocket
- [ ] Backoff exponencial: 3s, 6s, 12s, 24s, 30s (maximo)
- [ ] Banner visual "Conexion perdida - Reconectando..." durante desconexion
- [ ] Controles deshabilitados durante desconexion
- [ ] Al reconectar: ocultar banner, re-habilitar controles, refrescar estado
- [ ] Reset de backoff cuando la conexion se restablece

### Service Worker
- [ ] Registrar sw.js en index.html
- [ ] Cachear shell de la app: index.html, app.js, styles.css, manifest.json
- [ ] Estrategia: cache-first para assets estaticos
- [ ] Estrategia: network-first para API calls
- [ ] Offline: mostrar ultima version cached del shell
- [ ] Actualizar SW cuando haya nuevo version (skipWaiting)

### Manifest
- [ ] manifest.json con nombre, descripcion, icons, theme_color, background_color
- [ ] Icons: 192x192 y 512x512
- [ ] display: standalone
- [ ] theme_color: #1a1a2e
- [ ] start_url: /
- [ ] Boton "Instalar app" aparece en Chrome/Edge

### Responsive
- [ ] Funciona en movil (320px-480px): layout vertical, controles apilados
- [ ] Funciona en tablet (768px-1024px): layout adaptado
- [ ] Funciona en desktop (>1024px): layout centrado, max-width 600px
- [ ] Touch targets minimos 44x44px en movil
- [ ] Font sizes legibles en todos los tamanos

### Tema Oscuro
- [ ] Colores base: fondo #1a1a2e, header #16213e, texto #ffffff
- [ ] Colores de acento: #0f3460
- [ ] Bordes e inputs: #444
- [ ] Errores: #e74c3c
- [ ] Exito: #2ecc71
- [ ] Todos los componentes usan la paleta de colores oscura

### Pruebas Finales
- [ ] Instalar PWA en Android via Chrome
- [ ] Verificar que funciona offline (shell cacheado)
- [ ] Verificar en Chrome desktop
- [ ] Verificar en Firefox
- [ ] Verificar en Safari (iOS)
- [ ] Verificar que no hay errores en consola del navegador
- [ ] Verificar rendimiento: carga inicial < 2 segundos

## Criterio de aprobacion
- PWA completa funcionando con todas las funcionalidades
- Instalable como app nativa en Android
- Funciona offline (shell cacheado)
- Sin errores en consola del navegador
- Responsive en movil y desktop
- Tema oscuro consistente
