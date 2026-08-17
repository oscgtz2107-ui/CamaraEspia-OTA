# Fase 2: Video y Grabacion

## Objetivo
Implementar el streaming MJPEG y el sistema de grabacion a archivo AVI. Verificar calidad y fiabilidad de la grabacion.

## Pre-requisitos
- Fase 1 completada y aprobada
- Camara CSI funcionando en todas las resoluciones
- SD card con velocidad verificada (>2MB/s)
- Servo funcionando

## Checklist

### MJPEG Stream Basico
- [ ] Implementar endpoint GET /stream
- [ ] Stream retorna Content-Type: multipart/x-mixed-replace; boundary=frame
- [ ] Cada frame: --frame\r\nContent-Type: image/jpeg\r\n\r\n + datos JPEG
- [ ] Abrir http://192.168.4.1/stream en navegador: debe mostrar video en vivo
- [ ] Verificar que el stream no se detiene automaticamente

### Verificacion de FPS
- [ ] VGA (640x480): medir fps real, debe ser ~10fps
- [ ] HD (1280x720): medir fps real, debe ser ~5fps
- [ ] 3MP (2048x1536): medir fps real, debe ser ~1fps
- [ ] FPS se mide contando frames recibidos en 10 segundos y dividiendo
- [ ] Si FPS es menor al esperado: verificar que la camara esta en la resolucion correcta

### AVI Writer
- [ ] Implementar estructura RIFF header para AVI
- [ ] Header contiene: riff size, avi header, stream header (MJPG, width, height, fps)
- [ ] Cada frame: chunk "00dc" con datos JPEG crudos
- [ ] Index chunk "idx1" al final del archivo
- [ ] Guardar archivo con extension .avi en /sd/recordings/

### Grabacion Simple - Test
- [ ] Grabar clip de 10 segundos a VGA
- [ ] Archivo generado: clip_test.avi
- [ ] Transferir a PC y abrir en VLC
- [ ] Verificar: imagen visible, sin distorsion, duracion correcta (~10s)
- [ ] Verificar: audio (si aplica) o solo video

### Double Buffer
- [ ] Mientras se escribe a clip.avi, crear clip.tmp
- [ ] Al terminar la grabacion: renombrar clip.tmp a clip.avi
- [ ] Si se corta alimentacion durante grabacion: clip.avi debe estar completo
- [ ] clip.tmp puede estar incompleto (se descarta en proximo boot)

### Buffer Circular PSRAM
- [ ] Reservar buffer circular en PSRAM: 2 segundos a VGA (~200KB por frame * 10fps * 2s = ~4MB)
- [ ] Capturar frames continuamente al buffer circular
- [ ] Verificar que el buffer sobreescribe frames antiguos correctamente
- [ ] Al disparar grabacion: incluir los 2 segundos previos en el archivo
- [ ] Verificar que los primeros frames del AVI son anteriores al trigger

### Modo Solo Movimiento
- [ ] Configurar modo "motion" via API
- [ ] PIR detecta movimiento -> iniciar grabacion automatica
- [ ] Duracion de grabacion: 30 segundos despues del ultimo trigger
- [ ] Cooldown: no re-grabear si el cooldown no ha pasado (5 segundos)
- [ ] PIR re-trigger durante grabacion: extender duracion 30 segundos mas
- [ ] LED se enciende durante grabacion

### Modo Grabar Todo
- [ ] Configurar modo "always" via API
- [ ] Grabar clips de 5 minutos continuos
- [ ] Al completar 5 minutos: crear nuevo clip automaticamente
- [ ] Renombrar clip anterior (double buffer)
- [ ] Verificar que no hay gap entre clips (< 1 segundo)
- [ ] LED encendido continuamente durante grabacion

### Gestion SD
- [ ] Monitorear espacio libre en SD
- [ ] Cuando espacio libre < 500MB: borrar clip mas antiguo (no importante)
- [ ] Clips marcados como "importante" nunca se borran automaticamente
- [ ] Si todos los clips son importantes y espacio < 200MB: notificar via WS
- [ ] Verificar que el borrado no interrumpe grabacion en curso

### Thumbnails
- [ ] Extraer primer frame de un AVI como thumbnail
- [ ] Guardar como /sd/thumbs/{nombre_original}.jpg
- [ ] Thumbnail de calidad reducida (200px de ancho)
- [ ] Verificar que el thumbnail se genera correctamente

### Verificacion Final
- [ ] Todos los AVI generados se reproducen correctamente en VLC
- [ ] Verificar en Windows, Linux y Mac
- [ ] No hay frames corruptos en los archivos
- [ ] La calidad de imagen es aceptable

## Criterio de aprobacion
- Stream MJPEG visible en navegador en todas las resoluciones
- AVI grabado se reproduce en VLC sin errores
- Double buffer funciona (no hay archivos corruptos por corte de alimentacion)
- Modo motion y always funcionan correctamente
- Gestion SD borra archivos antiguos cuando es necesario
