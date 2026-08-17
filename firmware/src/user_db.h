#ifndef USER_DB_H
#define USER_DB_H

#include <Arduino.h>
#include <Preferences.h>
#include "languages.h"

#define TG_MAX_USERS 8

struct TelegramUser {
    int64_t chatId;
    Lang language;
    bool motionAlerts;
    bool authorized;
};

class UserDB {
public:
    UserDB();

    void begin();
    void load();

    bool addUser(int64_t chatId);
    bool removeUser(int64_t chatId);
    bool isAuthorized(int64_t chatId) const;
    bool isEmpty() const;
    int getUserCount() const;

    TelegramUser* getUser(int64_t chatId);
    const TelegramUser* getUser(int64_t chatId) const;

    void setLanguage(int64_t chatId, Lang lang);
    void setMotionAlerts(int64_t chatId, bool enabled);

    // Retorna cantidad de usuarios con alerts ON, llena el array outIds
    int getAlertEnabledUsers(int64_t* outIds, int maxOut) const;

    // Retorna cantidad total de usuarios, llena el array outIds
    int getAllUserIds(int64_t* outIds, int maxOut) const;

    // Admin: primer usuario registrado
    int64_t getAdminId() const;

    // Debug
    void printAll() const;

private:
    TelegramUser _users[TG_MAX_USERS];
    int _count;
    Preferences _prefs;

    int findSlot(int64_t chatId) const;
    int findFreeSlot() const;
    void saveUser(int index);
    void saveCount();
    void removeSlot(int index);
};

#endif // USER_DB_H
