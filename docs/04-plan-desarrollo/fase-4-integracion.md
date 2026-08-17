# Fase 4: Integracion y Sleep

## Objetivo
Integrar todos los modulos, implementar light sleep para ahorrar bateria, modo diagnostico, OTA updates y pruebas de estres.

## Pre-requisitos
- Fase 3 completada: servidor web y WebSocket funcionando
- Todos los modulos individuales verificados

## Checklist

### Light Sleep - Entrada
- [ ] Entrar en light sleep despues de 30 segundos de inactividad
- [ ] Inactividad: sin movimiento PIR, sin comandos WebSocket, sin interacciones
- [ ] LED se apaga al entrar en sleep
- [ ] WiFi se desconecta al dormir
- [ ] Servo mantiene su posicion por friccion mecanica (sin energia)

### Light Sleep - Wake por PIR
- [ ] PIR configurado como wake source (GPIO1, flanco de subida)
- [ ] Mover mano frente al PIR: ESP32 despierta
- [ ] Tiempo de wake: < 50ms
- [ ] LED parpadea 3 veces al despertar (verificacion visual)
- [ ] WiFi se reconecta automaticamente al despertar
- [ ] WebSocket se reestablece si habia clientes conectados

### Light Sleep - Wake por Boton
- [ ] Boton BOOT configurado como wake source
- [ ] Presionar BOOT: ESP32 despierta
- [ ] LED parpadea 2 veces al despertar por boton
- [ ] WiFi se reconecta

### Light Sleep - Restauracion
- [ ] Al despertar: restaurar angulo del servo desde NVS
- [ ] Reconfigurar canal LEDC para servo
- [ ] Restaurar modo de grabacion (motion/always)
- [ ] Reanudar stream si habia clientes conectados
- [ ] El sistema debe estar funcional en < 1 segundo despues del wake

### Modo Diagnostico
- [ ] Boton BOOT mantenido 5 segundos durante boot activa modo diagnostico
- [ ] Modo diagnostico: servidor escucha en /diag
- [ ] Tests automaticos: SD, Camera, Servo, PIR, WiFi, Battery, OTA
- [ ] Cada test retorna pass/fail con descripcion
- [ ] Log en tiempo real via WebSocket en pagina /diag
- [ ] Modo diagnostico no afecta configuracion normal del sistema

### Modo Seguro (Safe Mode)
- [ ] Si WDT resetea el sistema: guardar flag "unsafe_boot" en NVS
- [ ] En proximo boot: si flag existe, entrar en modo seguro
- [ ] Modo seguro: solo WiFi AP y servidor basico, sin grabacion
- [ ] Modo seguro permite OTA para recuperar el sistema
- [ ] Si boot es exitoso 3 veces seguidas: borrar flag "unsafe_boot"

### Watchdog Timer
- [ ] WDT configurado con timeout de 30 segundos
- [ ] WDT se alimenta periodicamente en loop principal
- [ ] Si loop se cuelga > 30s: WDT resetea el ESP32
- [ ] Verificar que el reset es limpio (boot reason: WDT)
- [ ] Despues de WDT reset: sistema arranca correctamente

### OTA Updates
- [ ] OTA recibe firmware .bin via HTTP POST /api/ota
- [ ] Guardar firmware en particion alternativa (otadata)
- [ ] Verificar SHA256 del firmware recibido
- [ ] Si SHA256 correcto: reiniciar con nuevo firmware
- [ ] Si SHA256 incorrecto: rechazar firmware, mantener actual
- [ ] Verificar que el nuevo firmware arranca correctamente
- [ ] Implementar rollback: si nuevo firmware falla (boot_count < 3), volver al anterior
- [ ] Mostrar progreso de OTA en UI si es posible

### Gestion SD - Limpieza
- [ ] Borrar clips antiguos no importantes cuando espacio < 500MB
- [ ] Orden de borrado: clips mas antiguos primero
- [ ] Clips marcados como "importante" nunca se borran automaticamente
- [ ] Si todos los clips son importantes y espacio < 200MB: notificar via WS
- [ ] Verificar que el borrado no interrumpe grabacion en curso

### Monitor de Bateria
- [ ] Leer bateria cada 30 segundos
- [ ] Enviar actualizacion por WebSocket cada 30 segundos
- [ ] Si bateria < 20%: enviar alerta por WS
- [ ] Si bateria < 10%: activar overlay de bateria critica en PWA
- [ ] Si bateria < 5%: entrar en sleep profundo (deep sleep) para proteger bateria

### Prueba de Estres - 1 Hora
- [ ] Ejecutar grabacion continua por 1 hora
- [ ] Modo always, VGA, clip de 5 minutos
- [ ] Verificar que no hay crash durante la hora
- [ ] Verificar que todos los clips se graban correctamente
- [ ] Verificar que la SD no se llena (limpieza funciona)
- [ ] Medir temperatura de la placa: no debe exceder 50C

### Prueba de Endurance - 24 Horas
- [ ] Dejar sistema en vigilancia pasiva (modo motion) por 24 horas
- [ ] PIR activo, grabacion bajo demanda
- [ ] Verificar que el sistema no se cuelga
- [ ] Verificar nivel de bateria al final
- [ ] Si bateria > 20% despues de 24h: eficiencia de consumo aceptable
- [ ] Verificar que los archivos de grabacion son validos

## Criterio de aprobacion
- Light sleep funciona con wake rapido por PIR y boton
- OTA actualiza el firmware sin perder configuracion
- Watchdog resetea el sistema correctamente en caso de crash
- Prueba de 1 hora pasa sin errores
- Prueba de 24 horas pasa con bateria suficiente
