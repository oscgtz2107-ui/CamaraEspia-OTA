/**
 * gallery.js - Galeria de videos
 * Fetch desde /api/videos, agrupar por fecha, render thumbnails
 * Descarga y eliminacion
 */
const Gallery = (() => {
    let modal = null;
    let grid = null;

    function init(modalSelector, gridSelector) { /* obtiene elementos */ }
    async function load() { /* fetch videos, agrupar por fecha, render */ }
    function render(videos) { /* genera HTML del grid con thumbnails */ }
    function groupByDate(videos) { /* agrupa por fecha */ }
    async function download(name) { /* abre URL de descarga */ }
    async function remove(name) { /* confirmacion, DELETE, recargar */ }
    async function toggleImportant(name) { /* POST importance */ }
    function show() { /* modal.classList.remove('hidden') */ }
    function hide() { /* modal.classList.add('hidden') */ }

    return { init, load, show, hide };
})();
