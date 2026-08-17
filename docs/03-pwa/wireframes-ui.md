# Wireframes y Diseno de Pantallas - PWA

## Pantalla 1 - Login

### Layout
- Contenedor principal: flex column, centrado horizontal y verticalmente (align-items: center, justify-content: center)
- Ocupa 100vh, padding horizontal 2rem

### Elementos (de arriba a abajo)
1. **Logo/Icono camara**: SVG o icono centrado, margen-bottom 2rem, color acento #0f3460
2. **Input Usuario**: Material-style underline
   - Borde inferior 2px solido #444
   - Focus: borde inferior #0f3460
   - Placeholder "Usuario"
   - Label flotante que sube al escribir
3. **Input Contrasena**: Material-style underline
   - Igual que usuario pero con type="password"
   - Placeholder "Contrasena"
   - Icono ojo para mostrar/ocultar (opcional)
4. **Boton "Conectar"**:
   - Fondo #0f3460, texto blanco, border-radius 8px
   - Full-width dentro del contenedor (max-width 320px)
   - Hover: lighten 10%
   - Disabled: opacity 0.5, sin interaccion
5. **Mensaje de error**: texto #e74c3c, font-size small, oculto por defecto, visible solo si fallo

### Colores
- Fondo: #1a1a2e
- Texto primario: #ffffff
- Texto secundario: #a0a0b0
- Acento: #0f3460
- Error: #e74c3c

---

## Pantalla 2 - Principal (despues de login)

### HEADER FIJO
- Position fixed, top 0, full width, height 56px
- Fondo: #16213e, z-index 100
- Contenido (flex row, space-between, align-center):
  - **Izquierda**: Logo/icono camara (24x24)
  - **Centro**: (vacio o titulo)
  - **Derecha**:
    - Indicador batería: icono bateria + porcentaje
      - Verde (#2ecc71) si >= 50%
      - Amarillo (#f39c12) si 20-49%
      - Rojo (#e74c3c) si < 20%
    - Separador vertical 8px
    - Nombre WiFi (SSID conectado), font-size small

### STREAM (60% del viewport)
- Contenedor con position relative
- Elemento `<img>` con src="/stream"
- width: 100%, height: 60vh (minimo 300px)
- object-fit: contain
- border-radius: 12px
- background-color: #000000
- Sombra sutil: box-shadow 0 4px 20px rgba(0,0,0,0.5)

### CONTROLES (debajo del stream)
- Padding: 1rem
- Flex column, gap 1rem

#### Slider Servo Horizontal
- Input range: min=0, max=180, step=1
- width: 100%
- Valor numerico al lado (font-size 1.2rem, font-weight bold, min-width 45px)
- Track color: #0f3460
- Thumb: circulo blanco con borde #0f3460

#### Botones Rapidos de Angulo
- Fila de 5 botones circulares: 0, 45, 90, 135, 180
- Cada boton: width 48px, height 48px, border-radius 50%
- Fondo: #16213e, texto blanco, borde 2px #0f3460
- Activo (angulo actual): fondo #0f3460
- Hover: lighten 10%
- Texto dentro: "0", "45", "90", "135", "180" (grados omitido)

#### Toggle "Grabar Todo" / "Solo Movimiento"
- Contenedor flex row, justify-content space-between
- Label izquierda: texto del modo actual
- Toggle switch personalizado:
  - Track: 52px x 28px, border-radius 14px
  - Off (Solo Movimiento): #444
  - On (Grabar Todo): #2ecc71
  - Thumb: circulo blanco 24x24, translateX al moverse
- Indicador LED debajo del toggle:
  - Circulo 8px, verde si activo, rojo si inactivo
  - Animacion parpadeo suave cuando grabando

#### Selector de Resolucion
- Grupo de 3 botones segmentados (btn-group)
- Botones: VGA | HD | 3MP
- Activo: fondo #0f3460, texto blanco
- Inactivo: fondo transparente, borde #444, texto #a0a0b0
- width: 100%, cada boton 33.33%

#### Indicador de Grabacion
- Posicion absolute, top-right del stream
- Badge rojo con texto "REC"
- Animacion: opacity 0 y 1 cada 1s (parpadeo)
- Oculto cuando no esta grabando

### GALLERY BUTTON
- Posicion fixed, bottom-right (20px, 20px)
- Boton circular 56x56px
- Fondo #0f3460, icono galeria (grid o images) en blanco
- Sombra: 0 4px 12px rgba(0,0,0,0.4)
- Hover: scale 1.05

---

## Pantalla 3 - Galeria (modal overlay)

### Overlay
- Position fixed, inset 0
- background: rgba(0,0,0,0.85)
- z-index: 200
- overflow-y: auto

### Header del modal
- Position sticky, top 0
- background: #16213e
- padding: 1rem
- Titulo "Galeria" a la izquierda
- Boton cerrar (X) a la derecha, 40x40px

### Grid de Thumbnails
- Padding: 1rem
- Agrupados por fecha con headers de seccion:
  - "Hoy", "Ayer", "15 Ene 2024", etc.
  - Font-size: 0.9rem, color: #a0a0b0, margin-bottom: 0.5rem
- CSS Grid: grid-template-columns repeat(auto-fill, minmax(150px, 1fr))
- Gap: 12px

### Cada Thumbnail
- Border-radius: 8px
- overflow: hidden
- background: #1a1a2e
- Contenido:
  1. **Imagen preview**: aspect-ratio 16/9, object-fit cover
  2. **Nombre archivo**: font-size small, padding 8px
  3. **Duracion**: texto "2:30" badge sobre la imagen (bottom-left)
  4. **Tamano**: font-size 0.75rem, color #666
- Hover: borde 2px #0f3460, transform scale 1.02

### Detalle de Video (al seleccionar)
- Modal secundario o panel lateral
- Video preview grande
- Acciones:
  - **Swipe izq/derecha**: siguiente/anterior video
  - **Boton descarga**: icono download, tooltip "Descargar"
  - **Boton eliminar**: icono trash, color rojo, con confirmacion
  - **Toggle "Importante"**: icono estrella, filled si activo, outline si no
  - Color estrella activa: #f39c12

### Boton Cerrar X
- Position fixed (dentro del overlay)
- Top-right: 20px desde esquina
- Background: rgba(255,255,255,0.1)
- Border-radius: 50%
- Size: 40x40px
- Hover: background rgba(255,255,255,0.2)

---

## Pantalla 4 - Diagnostico (/diag)

### Layout
- Padding: 1rem
- Overflow-y: auto
- background: #1a1a2e

### Header
- Titulo "Diagnostico del Sistema"
- Subtitulo "Estado actual de los modulos"

### Lista de Tests
- Cada test es un item con:
  - Icono indicador: circulo verde con check si OK, circulo rojo con X si fallo
  - Nombre del modulo: "SD Card", "Camera", "Servo", "PIR", "WiFi", "Battery ADC", "OTA"
  - Estado texto: "OK", "Error: no detectada", "Pendiente"
- Separator line entre cada test (1px #333)

### Test Servo
- Boton "Test Servo" (full width, padding 12px)
- Fondo #0f3460, texto blanco
- Al presionar: servo mueve 0 -> 90 -> 180 -> 90 -> 0 (con delay de 500ms entre cada posicion)
- Durante el test: boton deshabilitado, texto "Probando..."

### Log en Tiempo Real
- Contenedor con background #0d0d0d, border-radius 8px
- height: 300px, overflow-y: auto
- Font: monospace, font-size 12px, color #2ecc71 (verde terminal)
- Cada linea: timestamp + mensaje
- Auto-scroll al fondo

---

## Estados de UI

### Estado: Conectando
- Overlay semitransparente sobre todo el contenido
- Spinner circular (CSS animation, borde 3px transparente, borde-top #0f3460)
- Texto "Conectando al servidor..." debajo del spinner
- Font-size 1rem, color blanco

### Estado: Offline (WebSocket perdido)
- Banner fijo debajo del header:
  - Background: #e74c3c, color texto: blanco
  - Texto: "Conexion perdida - Reconectando..."
  - Padding: 8px
  - Animacion: slide-down desde arriba
- Controles deshabilitados (opacity 0.5, pointer-events none)
- Stream: imagen con overlay oscuro y texto "Sin conexion"

### Estado: Low Battery Overlay
- Se muestra cuando bateria < 10%
- Overlay fijo sobre todo el contenido (z-index 300)
- Background: rgba(231, 76, 60, 0.9)
- Contenido centrado:
  - Icono bateria vacia (SVG grande)
  - Texto "Bateria critica"
  - Porcentaje actual
  - Texto "El sistema se apagara pronto"
- Se puede cerrar con boton "Entendido" pero reaparece cada 60s
