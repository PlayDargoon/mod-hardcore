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

enum HardcoreSettings
{
    SETTING_HARDCORE = 0,
    HARDCORE_DEAD    = 1
};

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
};

#define sHardcore Hardcore::instance()

#endif
