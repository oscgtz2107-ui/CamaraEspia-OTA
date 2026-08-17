# Fase 3: Servidor Web

## Objetivo
Implementar el servidor HTTP completo con todas las APIs REST y WebSocket. Servir la PWA desde SPIFFS.

## Pre-requisitos
- Fase 2 completada: camara, SD y servo funcionando
- PWA basica desarrollada (login, stream, controles)
- SPIFFS particion configurada en PlatformIO

## Checklist

### HTTP Server Basico
- [ ] Iniciar servidor HTTP en puerto 80
- [ ] Sirve archivos estaticos desde SPIFFS (HTML, CSS, JS)
- [ ] MIME types correctos: .html -> text/html, .js -> application/javascript, .css -> text/css
- [ ] Si no se encuentra archivo: retornar 404
- [ ] Verificar que la pagina carga en navegador al conectar a 192.168.4.1

### GET /api/status
- [ ] Retorna JSON con estado actual del sistema
- [ ] Campos: battery_pct, battery_v, angle, mode, resolution, recording, sd_free_mb, uptime_s
- [ ] Verificar que los valores son correctos y actualizados
- [ ] Responder con HTTP 200 y Content-Type application/json

### POST /api/servo
- [ ] Aceptar body JSON: {"angle": 90}
- [ ] Validar rango: angle debe ser entero entre 0 y 180
- [ ] Si angle invalido: retornar 400 con mensaje de error
- [ ] Mover servo al angulo solicitado
- [ ] Retornar 200 con {"angle": 90, "status": "ok"}
- [ ] Verificar que el servo se mueve fisicamente

### GET /api/videos
- [ ] Listar archivos .avi en /sd/recordings/
- [ ] Retornar JSON array: [{"name": "clip_001.avi", "size": 12345678, "date": "2024-01-15T14:30:00", "important": false}]
- [ ] Ordenar por fecha descendente (mas reciente primero)
- [ ] Incluir informacion de favorito/importante
- [ ] Si no hay videos: retornar array vacio []

### GET /api/video/{name}
- [ ] Descargar archivo de video por nombre
- [ ] Content-Type: video/avi
- [ ] Content-Disposition: attachment; filename="{name}"
- [ ] Transfer-Encoding: chunked para archivos grandes
- [ ] Si archivo no existe: retornar 404
- [ ] Verificar que la descarga completa funciona en navegador

### POST /api/config
- [ ] Aceptar body JSON: {"mode": "motion", "resolution": "hd", "quality": 80}
- [ ] Validar valores: mode in ["always", "motion"], resolution in ["vga", "hd", "3mp"], quality 1-100
- [ ] Si valor invalido: retornar 400
- [ ] Aplicar configuracion al sistema
- [ ] Retornar 200 con configuracion confirmada
- [ ] Verificar que el cambio toma efecto (cambiar resolucion, verificar en stream)

### GET /api/thumb/{name}
- [ ] Retornar thumbnail JPG de un video
- [ ] Content-Type: image/jpeg
- [ ] Si thumbnail no existe: generar desde el primer frame del AVI
- [ ] Si video no existe: retornar 404
- [ ] Redimensionar a 200px de ancho si es necesario

### DELETE /api/video/{name}
- [ ] Eliminar archivo de video por nombre
- [ ] También eliminar thumbnail correspondiente
- [ ] Si archivo no existe: retornar 404
- [ ] Si exitoso: retornar 200 con {"status": "deleted"}
- [ ] Verificar que el archivo ya no aparece en GET /api/videos

### POST /api/video/{name}/favorite
- [ ] Alternar estado de favorito/importante de un video
- [ ] Si es favorito: marcar como no-favorito y viceversa
- [ ] Retornar 200 con nuevo estado: {"important": true}
- [ ] Persistir en metadata del archivo o archivo .meta separado

### WebSocket
- [ ] Implementar upgrade a WebSocket en ruta /ws
- [ ] Recibir primer mensaje auth con token JWT
- [ ] Si token invalido: cerrar conexion con codigo 4001
- [ ] Enviar mensajes status cada 2 segundos a clientes conectados
- [ ] Recibir mensajes del cliente (servo, config, status_request, capture)
- [ ] Enviar respuestas correspondientes (servo_ack, config_ack, pir_alert, recording_event)
- [ ] Keep-alive: ping cada 5 segundos
- [ ] Detectar clientes desconectados y limpiar recursos

### Autenticacion JWT
- [ ] POST /api/auth: recibir {user, pass}
- [ ] Si credenciales correctas: generar JWT con expiracion 24h
- [ ] Retornar {token: "eyJ..."}
- [ ] Todos los endpoints API verificar token en header Authorization: Bearer {token}
- [ ] Si token invalido o expirado: retornar 401
- [ ] WebSocket: primer mensaje auth con token, reject si invalido

### Servir PWA
- [ ] Archivos en SPIFFS: index.html, app.js, styles.css, manifest.json, sw.js
- [ ] GET / retorna index.html
- [ ] Todos los archivos estaticos accesibles via ruta relativa
- [ ] Service Worker registrado correctamente
- [ ] Manifest.json accesible en /manifest.json
- [ ] Icons accesibles en /icons/

### CORS (Desarrollo)
- [ ] Headers Access-Control-Allow-Origin: *
- [ ] Headers Access-Control-Allow-Methods: GET, POST, DELETE, OPTIONS
- [ ] Headers Access-Control-Allow-Headers: Content-Type, Authorization
- [ ] Responder OPTIONS con 200 (preflight)

## Criterio de aprobacion
- Todas las APIs responden correctamente con JSON valido
- Autenticacion JWT funciona (login, proteccion de endpoints)
- WebSocket funciona bidireccionalmente
- PWA se carga completa desde SPIFFS
- Descarga de videos funciona desde navegador
