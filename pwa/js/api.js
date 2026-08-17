/**
 * api.js - Comunicacion HTTP con el ESP32
 * Maneja fetch, autenticacion JWT, errores
 */
const API = (() => {
    const BASE = '';
    let token = null;

    function setToken(t) { token = t; }
    function getToken() { return token; }

    async function request(method, url, body = null) { /* ... */ }
    async function login(user, pass) { /* POST /api/auth/login */ }
    async function getStatus() { /* GET /api/status */ }
    async function setServo(angle) { /* POST /api/servo */ }
    async function setConfig(config) { /* POST /api/config */ }
    async function getVideos() { /* GET /api/videos */ }
    async function getVideoUrl(name) { /* retorna URL de descarga */ }
    async function getThumbUrl(name) { /* retorna URL de thumbnail */ }
    async function deleteVideo(name) { /* DELETE /api/video */ }
    async function setImportant(name, flag) { /* POST /api/video/{name}/important */ }
    async function uploadOTA(file) { /* POST /api/ota */ }

    return {
        setToken, getToken, login, getStatus, setServo, setConfig,
        getVideos, getVideoUrl, getThumbUrl, deleteVideo, setImportant, uploadOTA
    };
})();
