# Fase 1: Modulos Aislados

## Objetivo
Probar cada modulo hardware de forma independiente con firmware minimo. Cada modulo se verifica por separado antes de integrar.

## Pre-requisitos
- Fase 0 completada y aprobada
- PlatformIO configurado con ESP32-S3
- Placa conectada por USB

## Checklist

### Camera Manager
- [ ] Inicializar OV3660 con config minima (formato JPEG, resolucion VGA)
- [ ] Capturar un frame de prueba y verificar que no esta vacio
- [ ] Verificar que el frame es JPEG valido (bytes FF D8 al inicio)
- [ ] Guardar frame en SD y abrir en PC para verificar imagen

### Camera - Resoluciones
- [ ] Probar VGA (640x480): capturar frame, verificar tamanho ~20-40KB
- [ ] Probar HD (1280x720): capturar frame, verificar tamanho ~80-150KB
- [ ] Probar 3MP (2048x1536): capturar frame, verificar tamanho ~200-400KB
- [ ] Verificar que todos los frames son JPEG validos

### SD Manager - Operaciones basicas
- [ ] Montar SD con FATFS (ESP_VFS)
- [ ] Crear directorio /sd/recordings
- [ ] Crear archivo de texto, escribir 1KB de datos
- [ ] Leer archivo y verificar que los datos coinciden
- [ ] Borrar archivo y verificar que ya no existe
- [ ] Listar archivos en directorio (debe mostrar el archivo creado)

### SD Manager - Velocidad
- [ ] Escribir archivo de 1MB secuencialmente
- [ ] Medir tiempo de escritura
- [ ] Calcular velocidad: debe ser > 2MB/s para soportar grabacion VGA a 10fps
- [ ] Si velocidad < 1MB/s: verificar velocidad de tarjeta SD (clase 10 minimo)

### Servo Manager
- [ ] Inicializar canal LEDC en GPIO2
- [ ] Mover servo a 0 grados (pulso ~500us)
- [ ] Mover servo a 90 grados (pulso ~1500us)
- [ ] Mover servo a 180 grados (pulso ~2500us)
- [ ] Verificar posicion con angulometro o visualmente

### Servo Manager - Persistencia NVS
- [ ] Guardar angulo 45 en NVS
- [ ] Reiniciar ESP32
- [ ] Leer angulo de NVS: debe ser 45
- [ ] Guardar angulo 135 en NVS
- [ ] Reiniciar ESP32
- [ ] Leer angulo de NVS: debe ser 135

### PIR Manager
- [ ] Configurar interrupcion en GPIO1 (flanco de subida)
- [ ] Mover mano frente al PIR: debe generar interrupcion
- [ ] Verificar debounce: no debe generar multiples interrupciones con un solo movimiento
- [ ] Tiempo minimo entre interrupciones: 3 segundos

### PIR Manager - Wake desde Light Sleep
- [ ] Configurar ESP32 en light sleep
- [ ] PIR configurado como wake source en GPIO1
- [ ] Mover mano frente al PIR: ESP32 debe despertar
- [ ] Verificar con LED: parpadear 3 veces al despertar
- [ ] Tiempo de wake: debe ser < 50ms

### Battery Monitor
- [ ] Leer valor ADC crudo en GPIO3
- [ ] Calibrar con multímetro: medir voltaje real de bateria simultaneamente
- [ ] Crear tabla de calibracion: ADC raw -> voltaje real
- [ ] Mapear voltaje a porcentaje: 4.2V=100%, 3.7V=50%, 3.3V=0%
- [ ] Verificar que el porcentaje es consistente con la medicion del multímetro
- [ ] Test de variacion: el porcentaje no debe fluctuar mas de 2% en 10 segundos

### WiFi Access Point
- [ ] Crear AP con SSID "CamaraEspia" y password "camara123"
- [ ] Verificar que el AP aparece en lista de WiFi del telefono
- [ ] Conectar desde telefono y verificar acceso a internet (si hay uplink)
- [ ] Medir throughput: transferir archivo de 1MB por HTTP, medir velocidad
- [ ] Throughput esperado: > 2MB/s para streaming VGA

### LED Estado
- [ ] Parpadeo lento (1Hz): estado idle
- [ ] Parpadeo rapido (5Hz): estado conectando
- [ ] ON fijo: grabando
- [ ] OFF: sleep/dormido
- [ ] Verificar todos los patrones visiblemente

## Criterio de aprobacion
Todos los modulos pasan sus tests individuales. Cada modulo funciona de forma aislada sin errores.
