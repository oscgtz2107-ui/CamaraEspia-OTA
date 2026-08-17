#include "user_db.h"

UserDB::UserDB() : _count(0) {
    memset(_users, 0, sizeof(_users));
}

void UserDB::begin() {
    load();
    Serial.printf("[USERDB] %d usuarios cargados\n", _count);
    printAll();
}

void UserDB::load() {
    _prefs.begin("tg_users", true);
    _count = _prefs.getInt("count", 0);
    if (_count < 0 || _count > TG_MAX_USERS) _count = 0;

    for (int i = 0; i < _count; i++) {
        char key[16];
        snprintf(key, sizeof(key), "u%d_id", i);
        _users[i].chatId = _prefs.getLong64(key, 0);

        snprintf(key, sizeof(key), "u%d_lang", i);
        _users[i].language = (Lang)_prefs.getUChar(key, LANG_ES);

        snprintf(key, sizeof(key), "u%d_alerts", i);
        _users[i].motionAlerts = _prefs.getBool(key, true);

        snprintf(key, sizeof(key), "u%d_auth", i);
        _users[i].authorized = _prefs.getBool(key, true);
    }
    _prefs.end();
}

bool UserDB::addUser(int64_t chatId) {
    if (isAuthorized(chatId)) return true;
    if (_count >= TG_MAX_USERS) {
        Serial.println("[USERDB] Limite de usuarios alcanzado");
        return false;
    }

    int slot = findFreeSlot();
    if (slot < 0) return false;

    _users[slot].chatId = chatId;
    _users[slot].language = LANG_ES;
    _users[slot].motionAlerts = true;
    _users[slot].authorized = true;
    _count++;

    saveCount();
    saveUser(slot);

    Serial.printf("[USERDB] Usuario anadido: %lld (slot %d)\n", chatId, slot);
    return true;
}

bool UserDB::removeUser(int64_t chatId) {
    int slot = findSlot(chatId);
    if (slot < 0) return false;

    removeSlot(slot);
    _count--;

    // Reorganizar: mover todos hacia arriba
    for (int i = slot; i < _count; i++) {
        _users[i] = _users[i + 1];
        saveUser(i);
    }
    // Limpiar ultimo slot
    memset(&_users[_count], 0, sizeof(TelegramUser));
    saveCount();

    Serial.printf("[USERDB] Usuario eliminado: %lld\n", chatId);
    return true;
}

bool UserDB::isAuthorized(int64_t chatId) const {
    return findSlot(chatId) >= 0;
}

bool UserDB::isEmpty() const {
    return _count == 0;
}

int UserDB::getUserCount() const {
    return _count;
}

TelegramUser* UserDB::getUser(int64_t chatId) {
    int slot = findSlot(chatId);
    if (slot < 0) return nullptr;
    return &_users[slot];
}

const TelegramUser* UserDB::getUser(int64_t chatId) const {
    int slot = findSlot(chatId);
    if (slot < 0) return nullptr;
    return &_users[slot];
}

void UserDB::setLanguage(int64_t chatId, Lang lang) {
    int slot = findSlot(chatId);
    if (slot < 0) return;
    _users[slot].language = lang;
    saveUser(slot);
}

void UserDB::setMotionAlerts(int64_t chatId, bool enabled) {
    int slot = findSlot(chatId);
    if (slot < 0) return;
    _users[slot].motionAlerts = enabled;
    saveUser(slot);
}

int UserDB::getAlertEnabledUsers(int64_t* outIds, int maxOut) const {
    int count = 0;
    for (int i = 0; i < _count && count < maxOut; i++) {
        if (_users[i].authorized && _users[i].motionAlerts) {
            outIds[count++] = _users[i].chatId;
        }
    }
    return count;
}

int UserDB::getAllUserIds(int64_t* outIds, int maxOut) const {
    int count = 0;
    for (int i = 0; i < _count && count < maxOut; i++) {
        outIds[count++] = _users[i].chatId;
    }
    return count;
}

int64_t UserDB::getAdminId() const {
    if (_count == 0) return 0;
    return _users[0].chatId;
}

void UserDB::printAll() const {
    for (int i = 0; i < _count; i++) {
        Serial.printf("[USERDB] #%d: chatId=%lld lang=%d alerts=%s auth=%s\n",
                      i, _users[i].chatId, _users[i].language,
                      _users[i].motionAlerts ? "ON" : "OFF",
                      _users[i].authorized ? "SI" : "NO");
    }
}

int UserDB::findSlot(int64_t chatId) const {
    for (int i = 0; i < _count; i++) {
        if (_users[i].chatId == chatId) return i;
    }
    return -1;
}

int UserDB::findFreeSlot() const {
    for (int i = 0; i < TG_MAX_USERS; i++) {
        bool used = false;
        for (int j = 0; j < _count; j++) {
            if (_users[j].chatId != 0) { used = true; break; }
        }
        // Buscar slot con chatId == 0
        if (_users[i].chatId == 0) return i;
    }
    return -1;
}

void UserDB::saveUser(int index) {
    if (index < 0 || index >= TG_MAX_USERS) return;

    _prefs.begin("tg_users", false);
    char key[16];

    snprintf(key, sizeof(key), "u%d_id", index);
    _prefs.putLong64(key, _users[index].chatId);

    snprintf(key, sizeof(key), "u%d_lang", index);
    _prefs.putUChar(key, (uint8_t)_users[index].language);

    snprintf(key, sizeof(key), "u%d_alerts", index);
    _prefs.putBool(key, _users[index].motionAlerts);

    snprintf(key, sizeof(key), "u%d_auth", index);
    _prefs.putBool(key, _users[index].authorized);

    _prefs.end();
}

void UserDB::saveCount() {
    _prefs.begin("tg_users", false);
    _prefs.putInt("count", _count);
    _prefs.end();
}

void UserDB::removeSlot(int index) {
    memset(&_users[index], 0, sizeof(TelegramUser));
    char key[16];

    _prefs.begin("tg_users", false);
    snprintf(key, sizeof(key), "u%d_id", index);
    _prefs.putLong64(key, (int64_t)0);
    snprintf(key, sizeof(key), "u%d_lang", index);
    _prefs.putUChar(key, LANG_ES);
    snprintf(key, sizeof(key), "u%d_alerts", index);
    _prefs.putBool(key, true);
    snprintf(key, sizeof(key), "u%d_auth", index);
    _prefs.putBool(key, false);
    _prefs.end();
}
