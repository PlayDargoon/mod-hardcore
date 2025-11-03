#ifndef AZEROTHCORE_HARDCORE_H
#define AZEROTHCORE_HARDCORE_H

#include "ScriptMgr.h"
#include "Player.h"
#include "Config.h"
#include "Chat.h"
#include "ScriptedGossip.h"
#include "SpellMgr.h"
#include "GameObjectAI.h"
#include "World.h"
#include "WorldSession.h"
#include <map>
#include <unordered_map>
#include <sstream>
#include <functional>
#include <vector>

enum HardcoreSettings
{
    SETTING_HARDCORE = 0,
    HARDCORE_DEAD    = 1,
    HARDCORE_LAST_DUNGEON_TIME = 2
};

// Типы событий хардкор-режима
enum HardcoreEventType
{
    HARDCORE_EVENT_ACTIVATE = 0,
    HARDCORE_EVENT_DEATH = 1,
    HARDCORE_EVENT_LEVELUP = 2,
    HARDCORE_EVENT_DUNGEON_ENTER = 3,
    HARDCORE_EVENT_REWARD = 4,
    HARDCORE_EVENT_COMPLETE = 5,
    HARDCORE_EVENT_MAP_CHANGED = 6,
    HARDCORE_EVENT_MAIL_SEND = 7,
    HARDCORE_EVENT_MAIL_RECEIVE = 8,
    HARDCORE_EVENT_BG_QUEUE = 9,
    HARDCORE_EVENT_ARENA_QUEUE = 10
};

// Типы callback-функций для хуков модуля
typedef std::function<void(Player*)> HardcorePlayerCallback;
typedef std::function<bool(Player*, uint32)> HardcoreDungeonCallback;
typedef std::function<void(Player*, uint8)> HardcoreLevelCallback;
typedef std::function<void(Player*, uint32, uint32)> HardcoreRewardCallback;
typedef std::function<bool(Player*)> HardcoreMailCallback;
typedef std::function<bool(Player*, uint32)> HardcoreBGCallback;

class Hardcore
{
public:
    static Hardcore* instance();

    bool enabled() const { return hardcoreEnabled; }
    bool hardcoreEnabled;
    uint32 hardcoreAuraSpellId;

    // Награды и настройки
    float hardcoreXPMultiplier;
    uint32 hardcoreItemRewardAmount;
    uint32 hardcoreDisableLevel;
    uint32 hardcoreMaxDeathLevel;
    
    // Ограничения режима
    float hardcoreDamageDealtModifier;      // Множитель наносимого урона (0.9 = -10%)
    float hardcoreDamageTakenModifier;      // Множитель получаемого урона (1.5 = +50%)
    uint32 hardcoreMinDeathAnnounceLvl;     // Минимальный уровень для анонса смерти
    uint32 hardcoreMaxLevelDifference;      // Максимальная разница уровней в группе
    uint32 hardcoreDungeonCooldown;         // Кулдаун подземелий в часах
    bool hardcoreBlockDeathKnight;          // Запретить ДК участвовать
    bool hardcoreBlockAuction;              // Блокировать аукцион
    bool hardcoreBlockMail;                 // Блокировать почту
    bool hardcoreBlockGuildBank;            // Блокировать гильдейский банк
    bool hardcoreBlockBattleground;         // Блокировать поля боя
    bool hardcoreBlockArena;                // Блокировать арену
    bool hardcoreForcePvE;                  // Принудительный PvE
    
    // Карты наград (уровень -> ID)
    std::unordered_map<uint8, uint32> hardcoreTitleRewards;
    std::unordered_map<uint8, uint32> hardcoreTalentRewards;
    std::unordered_map<uint8, uint32> hardcoreItemRewards;
    std::unordered_map<uint8, uint32> hardcoreAchievementReward;

    bool isHardcorePlayer(Player* player) const;
    bool isHardcoreDead(Player* player) const;
    bool canEnterDungeon(Player* player, uint32 mapId);
    void checkDungeonCooldownOnLogin(Player* player);

    // ============================================
    // СИСТЕМА ХУКОВ МОДУЛЯ
    // ============================================
    
    // Регистрация обработчиков событий
    void RegisterOnActivate(HardcorePlayerCallback callback) { onActivateCallbacks.push_back(callback); }
    void RegisterOnDeath(HardcorePlayerCallback callback) { onDeathCallbacks.push_back(callback); }
    void RegisterOnLevelUp(HardcoreLevelCallback callback) { onLevelUpCallbacks.push_back(callback); }
    void RegisterOnDungeonEnter(HardcoreDungeonCallback callback) { onDungeonEnterCallbacks.push_back(callback); }
    void RegisterOnReward(HardcoreRewardCallback callback) { onRewardCallbacks.push_back(callback); }
    void RegisterOnComplete(HardcorePlayerCallback callback) { onCompleteCallbacks.push_back(callback); }
    void RegisterOnMapChanged(HardcoreDungeonCallback callback) { onMapChangedCallbacks.push_back(callback); }
    void RegisterOnMailSend(HardcoreMailCallback callback) { onMailSendCallbacks.push_back(callback); }
    void RegisterOnMailReceive(HardcoreMailCallback callback) { onMailReceiveCallbacks.push_back(callback); }
    void RegisterOnBGQueue(HardcoreBGCallback callback) { onBGQueueCallbacks.push_back(callback); }
    void RegisterOnArenaQueue(HardcoreBGCallback callback) { onArenaQueueCallbacks.push_back(callback); }
    
    // Триггеры событий (вызываются внутри модуля)
    void TriggerOnActivate(Player* player)
    {
        for (auto& callback : onActivateCallbacks)
            callback(player);
    }
    
    void TriggerOnDeath(Player* player)
    {
        for (auto& callback : onDeathCallbacks)
            callback(player);
    }
    
    void TriggerOnLevelUp(Player* player, uint8 level)
    {
        for (auto& callback : onLevelUpCallbacks)
            callback(player, level);
    }
    
    bool TriggerOnDungeonEnter(Player* player, uint32 mapId)
    {
        bool canEnter = true;
        for (auto& callback : onDungeonEnterCallbacks)
        {
            if (!callback(player, mapId))
                canEnter = false;
        }
        return canEnter;
    }
    
    void TriggerOnReward(Player* player, uint32 rewardType, uint32 value)
    {
        for (auto& callback : onRewardCallbacks)
            callback(player, rewardType, value);
    }
    
    void TriggerOnComplete(Player* player)
    {
        for (auto& callback : onCompleteCallbacks)
            callback(player);
    }
    
    bool TriggerOnMapChanged(Player* player, uint32 mapId)
    {
        bool canChange = true;
        for (auto& callback : onMapChangedCallbacks)
        {
            if (!callback(player, mapId))
                canChange = false;
        }
        return canChange;
    }
    
    bool TriggerOnMailSend(Player* player)
    {
        bool canSend = true;
        for (auto& callback : onMailSendCallbacks)
        {
            if (!callback(player))
                canSend = false;
        }
        return canSend;
    }
    
    bool TriggerOnMailReceive(Player* player)
    {
        bool canReceive = true;
        for (auto& callback : onMailReceiveCallbacks)
        {
            if (!callback(player))
                canReceive = false;
        }
        return canReceive;
    }
    
    bool TriggerOnBGQueue(Player* player, uint32 bgTypeId)
    {
        bool canJoin = true;
        for (auto& callback : onBGQueueCallbacks)
        {
            if (!callback(player, bgTypeId))
                canJoin = false;
        }
        return canJoin;
    }
    
    bool TriggerOnArenaQueue(Player* player, uint32 arenaType)
    {
        bool canJoin = true;
        for (auto& callback : onArenaQueueCallbacks)
        {
            if (!callback(player, arenaType))
                canJoin = false;
        }
        return canJoin;
    }

private:
    // Векторы callback-функций
    std::vector<HardcorePlayerCallback> onActivateCallbacks;
    std::vector<HardcorePlayerCallback> onDeathCallbacks;
    std::vector<HardcoreLevelCallback> onLevelUpCallbacks;
    std::vector<HardcoreDungeonCallback> onDungeonEnterCallbacks;
    std::vector<HardcoreRewardCallback> onRewardCallbacks;
    std::vector<HardcorePlayerCallback> onCompleteCallbacks;
    std::vector<HardcoreDungeonCallback> onMapChangedCallbacks;
    std::vector<HardcoreMailCallback> onMailSendCallbacks;
    std::vector<HardcoreMailCallback> onMailReceiveCallbacks;
    std::vector<HardcoreBGCallback> onBGQueueCallbacks;
    std::vector<HardcoreBGCallback> onArenaQueueCallbacks;
};

#define sHardcore Hardcore::instance()

#endif
