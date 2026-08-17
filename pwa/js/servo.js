/**
 * servo.js - Control del servo 0-180 grados
 * Slider con debounce + botones rapidos
 */
const Servo = (() => {
    let slider = null;
    let valueDisplay = null;
    let debounceTimer = null;
    let currentAngle = 90;

    function init(sliderSelector, displaySelector, quickBtnsSelector) {
        /* obtiene elementos, bind events, debounce 200ms */
    }
    function setAngle(angle) { /* actualiza slider visual + envia via API o WS */ }
    function onSliderChange(e) { /* debounce 200ms, enviar */ }
    function onQuickButtonClick(angle) { /* establecer angulo */ }
    function updateUI(angle) { /* actualiza valor visual */ }

    return { init, setAngle, updateUI };
})();
