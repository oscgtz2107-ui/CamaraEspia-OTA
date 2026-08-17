/**
 * notifications.js - Notificaciones del navegador
 * Solicita permiso, muestra notificacion cuando PIR dispara
 */
const Notifications = (() => {
    let permission = 'default';

    async function init() { /* solicita permiso Notification API */ }
    function show(title, body, icon) { /* muestra notificacion nativa */ }
    function onPIRAlert(data) { /* callback para mensajes WS pir_alert */ }
    function isSupported() { /* verifica soporte Notification */ }

    return { init, show, onPIRAlert, isSupported };
})();
