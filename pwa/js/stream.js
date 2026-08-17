/**
 * stream.js - MJPEG streaming y touch events
 * Conecta al stream MJPEG del ESP32
 * Maneja swipe (servo) y double-tap (captura)
 */
const Stream = (() => {
    let imgElement = null;
    let isConnected = false;
    let touchStartX = 0;
    let touchStartTime = 0;

    function init(imgSelector) { /* obtiene elemento img, bind touch events */ }
    function connect() { /* img.src = '/stream' */ }
    function disconnect() { /* img.src = '' */ }
    function handleTouchStart(e) { /* registra posicion y tiempo */ }
    function handleTouchEnd(e) { /* detecta swipe o double-tap */ }
    function onSwipeLeft() { /* mueve servo -15 grados */ }
    function onSwipeRight() { /* mueve servo +15 grados */ }
    function onDoubleTap() { /* captura manual */ }

    return { init, connect, disconnect };
})();
