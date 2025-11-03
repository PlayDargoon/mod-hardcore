/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license: https://github.com/azerothcore/azerothcore-wotlk/blob/master/LICENSE-AGPL3
 */

#include "Hardcore.h"
#include "WorldSessionMgr.h"
#include "WorldSession.h"
#include "Spell.h"
#include "BattlegroundMgr.h"

Hardcore* Hardcore::instance()
{
    static Hardcore instance;
    return &instance;
}

bool Hardcore::isHardcorePlayer(Player* player) const
{
    if (!enabled())
    {
        return false;
    }
    return player->GetPlayerSetting("mod-hardcore", SETTING_HARDCORE).value;
}

bool Hardcore::isHardcoreDead(Player* player) const
{
    if (!enabled())
    {
        return false;
    }
    return player->GetPlayerSetting("mod-hardcore", HARDCORE_DEAD).value;
}

bool Hardcore::canEnterDungeon(Player* player, uint32 /*mapId*/)
{
    if (!enabled() || !isHardcorePlayer(player) || hardcoreDungeonCooldown == 0)
    {
        return true;
    }

    uint32 lastDungeonTime = player->GetPlayerSetting("mod-hardcore", HARDCORE_LAST_DUNGEON_TIME).value;
    uint32 currentTime = uint32(time(nullptr));
    uint32 cooldownSeconds = hardcoreDungeonCooldown * HOUR;

    if (lastDungeonTime > 0 && (currentTime - lastDungeonTime) < cooldownSeconds)
    {
        uint32 remainingTime = cooldownSeconds - (currentTime - lastDungeonTime);
        uint32 hoursLeft = remainingTime / HOUR;
        uint32 minutesLeft = (remainingTime % HOUR) / MINUTE;
        
        ChatHandler(player->GetSession()).SendSysMessage("|cffFF0000[Хардкор] Вход в подземелье заблокирован!|r");
        std::string timeMsg = "|cffFFFF00Осталось: " + std::to_string(hoursLeft) + " ч. " + std::to_string(minutesLeft) + " мин.|r";
        ChatHandler(player->GetSession()).SendSysMessage(timeMsg.c_str());
        return false;
    }

    return true;
}

// Альтернативная проверка кулдауна через OnLogin (работает при входе в подземелье после релога)
void Hardcore::checkDungeonCooldownOnLogin(Player* player)
{
    if (!enabled() || !isHardcorePlayer(player) || hardcoreDungeonCooldown == 0)
        return;

    Map* map = player->GetMap();
    if (!map || !map->IsDungeon())
        return;

    if (!canEnterDungeon(player, map->GetId()))
    {
        // Телепортируем в точку привязки
        player->RepopAtGraveyard();
        ChatHandler(player->GetSession()).PSendSysMessage("|cffFF0000[Хардкор] Вы были телепортированы из подземелья из-за кулдауна.|r");
    }
    else
    {
        // Триггер события входа в подземелье
        sHardcore->TriggerOnDungeonEnter(player, map->GetId());
        
        // Обновляем время входа
        player->UpdatePlayerSetting("mod-hardcore", HARDCORE_LAST_DUNGEON_TIME, uint32(time(nullptr)));
        std::string cooldownMsg = "|cffFFFF00[Хардкор] Подземелье: следующий вход через " + std::to_string(hardcoreDungeonCooldown) + " часов.|r";
        ChatHandler(player->GetSession()).SendSysMessage(cooldownMsg.c_str());
    }
}

class Hardcore_WorldScript : public WorldScript
{
public:
    Hardcore_WorldScript() : WorldScript("Hardcore_WorldScript") {}

    void OnBeforeConfigLoad(bool /*reload*/) override
    {
        sHardcore->hardcoreEnabled = sConfigMgr->GetOption<bool>("Hardcore.Enable", true);
        sHardcore->hardcoreAuraSpellId = sConfigMgr->GetOption<uint32>("Hardcore.AuraSpellId", 36573);
        
        // Загрузка настроек наград
        sHardcore->hardcoreXPMultiplier = sConfigMgr->GetOption<float>("Hardcore.XPMultiplier", 1.0f);
        sHardcore->hardcoreItemRewardAmount = sConfigMgr->GetOption<uint32>("Hardcore.ItemRewardAmount", 1);
        sHardcore->hardcoreDisableLevel = sConfigMgr->GetOption<uint32>("Hardcore.DisableLevel", 0);
        sHardcore->hardcoreMaxDeathLevel = sConfigMgr->GetOption<uint32>("Hardcore.MaxDeathLevel", 0);
        
        // Загрузка ограничений режима
        sHardcore->hardcoreDamageDealtModifier = sConfigMgr->GetOption<float>("Hardcore.DamageDealtModifier", 0.9f);
        sHardcore->hardcoreDamageTakenModifier = sConfigMgr->GetOption<float>("Hardcore.DamageTakenModifier", 1.5f);
        sHardcore->hardcoreMinDeathAnnounceLvl = sConfigMgr->GetOption<uint32>("Hardcore.MinDeathAnnounceLevel", 20);
        sHardcore->hardcoreMaxLevelDifference = sConfigMgr->GetOption<uint32>("Hardcore.MaxLevelDifference", 5);
        sHardcore->hardcoreDungeonCooldown = sConfigMgr->GetOption<uint32>("Hardcore.DungeonCooldown", 24);
        sHardcore->hardcoreBlockDeathKnight = sConfigMgr->GetOption<bool>("Hardcore.BlockDeathKnight", true);
        sHardcore->hardcoreBlockAuction = sConfigMgr->GetOption<bool>("Hardcore.BlockAuction", true);
        sHardcore->hardcoreBlockMail = sConfigMgr->GetOption<bool>("Hardcore.BlockMail", true);
        sHardcore->hardcoreBlockGuildBank = sConfigMgr->GetOption<bool>("Hardcore.BlockGuildBank", true);
        sHardcore->hardcoreBlockBattleground = sConfigMgr->GetOption<bool>("Hardcore.BlockBattleground", true);
        sHardcore->hardcoreBlockArena = sConfigMgr->GetOption<bool>("Hardcore.BlockArena", true);
        sHardcore->hardcoreForcePvE = sConfigMgr->GetOption<bool>("Hardcore.ForcePvE", true);
        
        // Загрузка карт наград
        LoadStringToMap(sHardcore->hardcoreTitleRewards, sConfigMgr->GetOption<std::string>("Hardcore.TitleRewards", ""));
        LoadStringToMap(sHardcore->hardcoreTalentRewards, sConfigMgr->GetOption<std::string>("Hardcore.TalentRewards", ""));
        LoadStringToMap(sHardcore->hardcoreItemRewards, sConfigMgr->GetOption<std::string>("Hardcore.ItemRewards", ""));
        LoadStringToMap(sHardcore->hardcoreAchievementReward, sConfigMgr->GetOption<std::string>("Hardcore.AchievementReward", ""));
    }

private:
    static void LoadStringToMap(std::unordered_map<uint8, uint32>& mapToLoad, const std::string& configString)
    {
        mapToLoad.clear();
        if (configString.empty())
        {
            return;
        }
        
        std::string delimitedValue;
        std::stringstream configIdStream;

        configIdStream.str(configString);
        // Обрабатываем каждый ID конфигурации в строке, разделенный запятой - "," и затем пробелом " "
        while (std::getline(configIdStream, delimitedValue, ','))
        {
            std::string pairOne, pairTwo;
            std::stringstream configPairStream(delimitedValue);
            configPairStream >> pairOne >> pairTwo;
            auto configLevel = atoi(pairOne.c_str());
            auto rewardValue = atoi(pairTwo.c_str());
            mapToLoad[configLevel] = rewardValue;
        }
    }
};

class Hardcore_PlayerScript : public PlayerScript
{
public:
    Hardcore_PlayerScript() : PlayerScript("Hardcore_PlayerScript") {}

    // Хук при создании персонажа
    void OnCreatePlayer(Player* player)
    {
        // Выдаем спелл для вызова меню хардкора
        if (!player->HasSpell(38057))
        {
            player->learnSpell(38057);
        }
        
        // Показываем приглашение сразу при создании
        if (player->GetLevel() == 1)
        {
            ChatHandler chatHandle = ChatHandler(player->GetSession());
            
            chatHandle.SendSysMessage(" ");
            chatHandle.SendSysMessage("|cffFF0000==========================================|r");
            chatHandle.SendSysMessage("|cffFFD700     ДОБРО ПОЖАЛОВАТЬ!|r");
            chatHandle.SendSysMessage("|cffFF0000==========================================|r");
            chatHandle.SendSysMessage(" ");
            chatHandle.SendSysMessage("|cffFFFFFFВам доступен режим |cffFF0000ХАРДКОР|r!|r");
            chatHandle.SendSysMessage(" ");
            chatHandle.SendSysMessage("|cff00FF00Для доступа к меню используйте:|r");
            chatHandle.SendSysMessage("|cffFFFF00  Команда: .menu|r");
            chatHandle.SendSysMessage("|cffFFFF00  Или найдите спелл в книге заклинаний|r");
            chatHandle.SendSysMessage(" ");
            chatHandle.SendSysMessage("|cffFF0000==========================================|r");
        }
    }

    // Применить красную ауру хардкора
    void ApplyHardcoreAura(Player* player)
    {
        uint32 spellId = sHardcore->hardcoreAuraSpellId;
        if (spellId && !player->HasAura(spellId))
        {
            if (SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId))
            {
                player->AddAura(spellId, player);
                if (Aura* aura = player->GetAura(spellId))
                {
                    aura->SetDuration(-1);
                    aura->SetMaxDuration(-1);
                }
            }
        }
    }

    // Убрать ауру хардкора
    void RemoveHardcoreAura(Player* player)
    {
        uint32 spellId = sHardcore->hardcoreAuraSpellId;
        if (spellId && player->HasAura(spellId))
        {
            player->RemoveAura(spellId);
        }
    }

    void OnLogin(Player* player)
    {
        // Показать приглашение к режиму хардкор новым персонажам 1 уровня
        if (!sHardcore->isHardcorePlayer(player) && player->GetLevel() == 1)
        {
            // Проверяем, не отказался ли игрок от испытания
            uint32 declined = player->GetPlayerSetting("mod-hardcore", 10).value; // 10 = HARDCORE_DECLINED
            
            if (declined == 0)
            {
                // Отправляем красивое приглашение
                ChatHandler chatHandle = ChatHandler(player->GetSession());
                
                chatHandle.SendSysMessage("|cffFF0000==========================================|r");
                chatHandle.SendSysMessage("|cffFFD700     РЕЖИМ ХАРДКОР ДОСТУПЕН!|r");
                chatHandle.SendSysMessage("|cffFF0000==========================================|r");
                chatHandle.SendSysMessage(" ");
                chatHandle.SendSysMessage("|cffFFFFFFДобро пожаловать, искатель приключений!|r");
                chatHandle.SendSysMessage("|cffFF8800Вам доступен легендарный режим ХАРДКОР:|r");
                chatHandle.SendSysMessage(" ");
                chatHandle.SendSysMessage("|cffFF6347 * ОДНА ЖИЗНЬ - одна смерть = конец|r");
                chatHandle.SendSysMessage("|cffFF6347 * БЕЗ ТОРГОВЛИ - запрещен аукцион|r");
                chatHandle.SendSysMessage("|cffFF6347 * БЕЗ ГРУППЫ - только соло игра|r");
                chatHandle.SendSysMessage("|cffFF6347 * БЕЗ ГИЛЬДИИ - путь одиночки|r");
                chatHandle.SendSysMessage(" ");
                chatHandle.SendSysMessage("|cff00FF00Для активации используйте:|r");
                chatHandle.SendSysMessage("|cffFFFF00  .hardcore start|r - Начать испытание");
                chatHandle.SendSysMessage("|cffFFFF00  .hardcore info|r  - Подробности");
                chatHandle.SendSysMessage("|cffFFFF00  .hardcore decline|r - Отказаться");
                chatHandle.SendSysMessage(" ");
                chatHandle.SendSysMessage("|cffFF0000ВНИМАНИЕ: После активации отменить нельзя!|r");
                chatHandle.SendSysMessage("|cffFF0000==========================================|r");
            }
        }
        
        // Применяем красную ауру, если хардкор активен
        if (sHardcore->isHardcorePlayer(player))
        {
            ApplyHardcoreAura(player);
            
            // Принудительный PvE режим
            if (sHardcore->hardcoreForcePvE && player->HasFlag(PLAYER_FLAGS, PLAYER_FLAGS_IN_PVP))
            {
                player->RemoveFlag(PLAYER_FLAGS, PLAYER_FLAGS_IN_PVP);
                player->UpdatePvPState();
            }
            
            // Проверка кулдауна подземелий (альтернативная реализация)
            sHardcore->checkDungeonCooldownOnLogin(player);
        }
        
        // Если персонаж мертв в режиме хардкор
        if (sHardcore->isHardcorePlayer(player) && sHardcore->isHardcoreDead(player))
        {
            ChatHandler(player->GetSession()).PSendSysMessage("|cffFF0000Ваш хардкор-персонаж погиб. Воскрешение невозможно.|r");
            ChatHandler(player->GetSession()).PSendSysMessage("|cffFFFF00Создайте нового персонажа для продолжения игры.|r");
            
            if (player->IsAlive())
            {
                player->KillPlayer();
            }
        }
    }
    
    /* HOOK NOT AVAILABLE IN THIS AC VERSION
    void OnMapChanged(Player* player) override
    {
        if (!sHardcore->isHardcorePlayer(player))
            return;

        Map* map = player->GetMap();
        if (!map || !map->IsDungeon())
            return;

        // Проверяем кулдаун при входе в подземелье
        if (!sHardcore->canEnterDungeon(player, map->GetId()))
        {
            // Телепортируем обратно в точку входа или хоумбинд
            player->RepopAtGraveyard();
            return;
        }

        // Обновляем время последнего входа в подземелье
        player->UpdatePlayerSetting("mod-hardcore", HARDCORE_LAST_DUNGEON_TIME, uint32(time(nullptr)));
        ChatHandler(player->GetSession()).PSendSysMessage("|cffFFFF00[Хардкор] Подземелье: следующий вход через %u часов.|r", sHardcore->hardcoreDungeonCooldown);
    }
    */
    
    /* HOOK NOT AVAILABLE IN THIS AC VERSION
    // Блокировка почты (отправка)
    void OnBeforeSendMail(Player* player, ObjectGuid receiverGuid, uint32& mailTemplateId, uint32& deliver_delay, uint32& custom_expire_time, std::string& subject, std::string& body) override
    {
        if (sHardcore->hardcoreBlockMail && sHardcore->isHardcorePlayer(player))
        {
            ChatHandler(player->GetSession()).SendSysMessage("|cffFF0000[Хардкор] Отправка почты недоступна в режиме «Без права на ошибку»!|r");
        }
    }
    */
    
    /* HOOK NOT AVAILABLE IN THIS AC VERSION
    // Блокировка почты (получение)
    void OnMailReceive(Player* player, Mail* mail) override
    {
        if (sHardcore->hardcoreBlockMail && sHardcore->isHardcorePlayer(player))
        {
            ChatHandler(player->GetSession()).SendSysMessage("|cffFF0000[Хардкор] Получение почты недоступно в режиме «Без права на ошибку»!|r");
        }
    }
    */
    
    /* HOOK NOT AVAILABLE IN THIS AC VERSION
    // Блокировка полей боя
    bool CanJoinInBattlegroundQueue(Player* player, ObjectGuid BattlemasterGuid, BattlegroundTypeId BGTypeID, uint8 joinAsGroup) override
    {
        if (sHardcore->hardcoreBlockBattleground && sHardcore->isHardcorePlayer(player))
        {
            ChatHandler(player->GetSession()).SendSysMessage("|cffFF0000[Хардкор] Поля боя недоступны в режиме «Без права на ошибку»!|r");
            return false;
        }
        return true;
    }
    */
    
    /* HOOK NOT AVAILABLE IN THIS AC VERSION
    // Блокировка арены
    bool CanJoinInArenaQueue(Player* player, ObjectGuid BattlemasterGuid, uint8 arenaslot, BattlegroundTypeId BGTypeID, uint8 joinAsGroup, uint8 IsRated, uint32 ArenaRating) override
    {
        if (sHardcore->hardcoreBlockArena && sHardcore->isHardcorePlayer(player))
        {
            ChatHandler(player->GetSession()).SendSysMessage("|cffFF0000[Хардкор] Арена недоступна в режиме «Без права на ошибку»!|r");
            return false;
        }
        return true;
    }
    */
    
    // Блокировка обмена между хардкор и обычными игроками
    bool OnBeforePlayerTrade(Player* player, Player* target)
    {
        bool playerIsHardcore = sHardcore->isHardcorePlayer(player);
        bool targetIsHardcore = sHardcore->isHardcorePlayer(target);
        
        if (playerIsHardcore != targetIsHardcore)
        {
            ChatHandler(player->GetSession()).SendSysMessage("|cffFF0000[Хардкор] Обмен возможен только с игроками того же режима!|r");
            return false;
        }
        
        return true;
    }
    
    // Блокировка дуэлей (кроме смертельных)
    bool OnBeforeDuel(Player* player, Player* target)
    {
        bool playerIsHardcore = sHardcore->isHardcorePlayer(player);
        bool targetIsHardcore = sHardcore->isHardcorePlayer(target);
        
        if (playerIsHardcore != targetIsHardcore)
        {
            ChatHandler(player->GetSession()).SendSysMessage("|cffFF0000[Хардкор] Дуэли возможны только с игроками того же режима!|r");
            return false;
        }
        
        return true;
    }
    
    /* HOOK NOT AVAILABLE - SPELL POINTER ACCESS ISSUE
    // Блокировка баффов от не-хардкор игроков
    bool OnBeforeCastSpell(Player* caster, Spell* spell, bool triggered)
    {
        if (!sHardcore->isHardcorePlayer(caster))
            return true;
            
        Unit* target = spell->m_targets.GetUnitTarget();
        if (!target || !target->IsPlayer())
            return true;
            
        Player* targetPlayer = target->ToPlayer();
        bool targetIsHardcore = sHardcore->isHardcorePlayer(targetPlayer);
        
        // Хардкор игрок не может баффать не-хардкор игрока
        if (!targetIsHardcore)
        {
            SpellInfo const* spellInfo = spell->GetSpellInfo();
            if (spellInfo && spellInfo->IsPositive())
            {
                ChatHandler(caster->GetSession()).SendSysMessage("|cffFF0000[Хардкор] Вы можете баффать только игроков режима «Без права на ошибку»!|r");
                return false;
            }
        }
        
        return true;
    }
    */

    void OnPlayerReleasedGhost(Player* player) override
    {
        if (!sHardcore->isHardcorePlayer(player))
        {
            return;
        }
        
        player->UpdatePlayerSetting("mod-hardcore", HARDCORE_DEAD, 1);
        ChatHandler(player->GetSession()).PSendSysMessage("|cffFF0000Ваш хардкор-персонаж погиб навсегда. Воскрешение невозможно.|r");
    }

    void OnPVPKill(Player* killer, Player* killed)
    {
        if (!sHardcore->isHardcorePlayer(killed))
        {
            return;
        }
        
        // Проверка: если уровень отключения достигнут - не применяем окончательную смерть
        if (sHardcore->hardcoreDisableLevel > 0 && killed->GetLevel() >= sHardcore->hardcoreDisableLevel)
        {
            std::string msg = "|cffFFFF00[Хардкор]|r Режим отключен на " + std::to_string(sHardcore->hardcoreDisableLevel) + " уровне - смерть не окончательна.";
            ChatHandler(killed->GetSession()).SendSysMessage(msg.c_str());
            return;
        }
        
        // Проверка: если уровень игрока выше максимального для окончательной смерти
        if (sHardcore->hardcoreMaxDeathLevel > 0 && killed->GetLevel() > sHardcore->hardcoreMaxDeathLevel)
        {
            std::string msg = "|cffFFFF00[Хардкор]|r Ваш уровень выше " + std::to_string(sHardcore->hardcoreMaxDeathLevel) + " - смерть не окончательна.";
            ChatHandler(killed->GetSession()).SendSysMessage(msg.c_str());
            return; // Не применяем окончательную смерть
        }
        
        killed->UpdatePlayerSetting("mod-hardcore", HARDCORE_DEAD, 1);
        killed->BuildPlayerRepop();
        killed->RepopAtGraveyard();
        
        // ТРИГГЕР ХУКА: Смерть хардкор-игрока (PvP)
        sHardcore->TriggerOnDeath(killed);
        
        // Сообщение погибшему игроку
        ChatHandler(killed->GetSession()).PSendSysMessage("|cffFF0000╔══════════════════════════════════════╗|r");
        ChatHandler(killed->GetSession()).PSendSysMessage("|cffFF0000║        ВЫ ПОГИБЛИ НАВСЕГДА!         ║|r");
        ChatHandler(killed->GetSession()).PSendSysMessage("|cffFF0000╚══════════════════════════════════════╝|r");
        ChatHandler(killed->GetSession()).PSendSysMessage("|cffFFFF00Хардкор-режим: Воскрешение невозможно.|r");
        ChatHandler(killed->GetSession()).PSendSysMessage("|cffFFFF00Создайте нового персонажа для продолжения.|r");
        
        // Глобальное оповещение о смерти (только для персонажей минимального уровня и выше)
        if (killed->GetLevel() >= sHardcore->hardcoreMinDeathAnnounceLvl)
        {
            std::string killerName = killer ? killer->GetName() : "неизвестный враг";
            std::string deathAnnouncement = "|cffFFFF00==========================================|r\n"
                                           "|cffFFFF00[Сервер]|r |cffFF0000СМЕРТЬ В ХАРДКОРЕ|r\n"
                                           "|cffFF0000" + killed->GetName() + "|r (ур. " + std::to_string(killed->GetLevel()) + ") пал в бою с |cff00FFFF" + killerName + "|r!\n"
                                           "|cffFF8800Режим Без права на ошибку: последнее путешествие завершено.|r\n"
                                           "|cffFFFF00==========================================|r";
            ChatHandler(nullptr).SendWorldText(deathAnnouncement.c_str());
            
            // Глобальное оповещение на экране
            const std::string screenNotification = killed->GetName() + " (ур. " + std::to_string(killed->GetLevel()) + ") погиб в режиме ХАРДКОР!";
            sWorldSessionMgr->DoForAllOnlinePlayers([&screenNotification](Player* onlinePlayer)
            {
                if (WorldSession* session = onlinePlayer->GetSession())
                {
                    session->SendAreaTriggerMessage(screenNotification);
                }
            });
        }
    }

    void OnPlayerKilledByCreature(Creature* killer, Player* killed) override
    {
        if (!sHardcore->isHardcorePlayer(killed))
        {
            return;
        }
        
        // Проверка: если уровень отключения достигнут - не применяем окончательную смерть
        if (sHardcore->hardcoreDisableLevel > 0 && killed->GetLevel() >= sHardcore->hardcoreDisableLevel)
        {
            std::string msg = "|cffFFFF00[Хардкор]|r Режим отключен на " + std::to_string(sHardcore->hardcoreDisableLevel) + " уровне - смерть не окончательна.";
            ChatHandler(killed->GetSession()).SendSysMessage(msg.c_str());
            return;
        }
        
        // Проверка: если уровень игрока выше максимального для окончательной смерти
        if (sHardcore->hardcoreMaxDeathLevel > 0 && killed->GetLevel() > sHardcore->hardcoreMaxDeathLevel)
        {
            std::string msg = "|cffFFFF00[Хардкор]|r Ваш уровень выше " + std::to_string(sHardcore->hardcoreMaxDeathLevel) + " - смерть не окончательна.";
            ChatHandler(killed->GetSession()).SendSysMessage(msg.c_str());
            return; // Не применяем окончательную смерть
        }
        
        killed->UpdatePlayerSetting("mod-hardcore", HARDCORE_DEAD, 1);
        killed->BuildPlayerRepop();
        killed->RepopAtGraveyard();
        
        // ТРИГГЕР ХУКА: Смерть хардкор-игрока (PvE)
        sHardcore->TriggerOnDeath(killed);
        
        // Сообщение погибшему игроку
        ChatHandler(killed->GetSession()).PSendSysMessage("|cffFF0000╔══════════════════════════════════════╗|r");
        ChatHandler(killed->GetSession()).PSendSysMessage("|cffFF0000║        ВЫ ПОГИБЛИ НАВСЕГДА!         ║|r");
        ChatHandler(killed->GetSession()).PSendSysMessage("|cffFF0000╚══════════════════════════════════════╝|r");
        ChatHandler(killed->GetSession()).PSendSysMessage("|cffFFFF00Хардкор-режим: Воскрешение невозможно.|r");
        ChatHandler(killed->GetSession()).PSendSysMessage("|cffFFFF00Создайте нового персонажа для продолжения.|r");
        
        // Глобальное оповещение о смерти (только для персонажей минимального уровня и выше)
        if (killed->GetLevel() >= sHardcore->hardcoreMinDeathAnnounceLvl)
        {
            std::string killerName = killer ? killer->GetName() : "неизвестное существо";
            std::string deathAnnouncement = "|cffFFFF00==========================================|r\n"
                                           "|cffFFFF00[Сервер]|r |cffFF0000СМЕРТЬ В ХАРДКОРЕ|r\n"
                                           "|cffFF0000" + killed->GetName() + "|r (ур. " + std::to_string(killed->GetLevel()) + ") пал от |cffFF8800" + killerName + "|r!\n"
                                           "|cffFF8800Режим Без права на ошибку: приключение окончено.|r\n"
                                           "|cffFFFF00==========================================|r";
            ChatHandler(nullptr).SendWorldText(deathAnnouncement.c_str());
            
            // Глобальное оповещение на экране
            const std::string screenNotification = killed->GetName() + " (ур. " + std::to_string(killed->GetLevel()) + ") погиб в режиме ХАРДКОР!";
            sWorldSessionMgr->DoForAllOnlinePlayers([&screenNotification](Player* onlinePlayer)
            {
                if (WorldSession* session = onlinePlayer->GetSession())
                {
                    session->SendAreaTriggerMessage(screenNotification);
                }
            });
        }
    }

    bool CanRepopAtGraveyard(Player* player)
    {
        // Блокируем воскрешение через Целителя Душ
        if (sHardcore->isHardcorePlayer(player) && sHardcore->isHardcoreDead(player))
        {
            ChatHandler(player->GetSession()).PSendSysMessage("|cffFF0000Воскрешение запрещено для мертвых хардкор-персонажей!|r");
            return false;
        }
        return true;
    }

    void OnPlayerResurrect(Player* player, float /*restore_percent*/, bool /*applySickness*/) override
    {
        if (!sHardcore->isHardcorePlayer(player))
        {
            return;
        }
        
        ChatHandler(player->GetSession()).PSendSysMessage("|cffFF0000Воскрешение запрещено для хардкор-персонажей!|r");
        player->UpdatePlayerSetting("mod-hardcore", HARDCORE_DEAD, 1);
        player->KillPlayer();
    }

    void OnGiveXP(Player* player, uint32& amount, Unit* /*victim*/, uint8 /*xpSource*/)
    {
        if (!sHardcore->isHardcorePlayer(player))
        {
            return;
        }
        
        // Сохраняем текущий уровень для проверки после получения опыта
        uint8 oldLevel = player->GetLevel();
        
        // Применяем множитель опыта
        amount *= sHardcore->hardcoreXPMultiplier;
        
        // ВАЖНО: Проверка уровня будет в OnLevelChanged, который должен сработать автоматически
        // Этот код нужен только для отладки, если OnLevelChanged не работает
    }

    /* HOOK NOT AVAILABLE IN THIS AC VERSION
    // Модификация урона - наносимого
    void OnDamageDealt(Player* player, Unit* victim, uint32& damage, DamageEffectType damageType)
    {
        if (!sHardcore->isHardcorePlayer(player))
        {
            return;
        }
        
        // Применяем ослабление урона (-10% по умолчанию)
        damage *= sHardcore->hardcoreDamageDealtModifier;
    }
    
    // Модификация урона - получаемого
    void OnTakeDamage(Player* player, Unit* attacker, uint32& damage, DamageEffectType damageType)
    {
        if (!sHardcore->isHardcorePlayer(player))
        {
            return;
        }
        
        // Применяем усиление получаемого урона (+50% по умолчанию)
        damage *= sHardcore->hardcoreDamageTakenModifier;
    }
    */

    void OnLevelChanged(Player* player, uint8 /*oldLevel*/)
    {
        if (!sHardcore->isHardcorePlayer(player))
        {
            return;
        }

        uint8 level = player->GetLevel();
        
        // ТРИГГЕР ХУКА: Повышение уровня
        sHardcore->TriggerOnLevelUp(player, level);
        
        // Глобальное объявление каждые 5 уровней
        if (level % 5 == 0 && level >= 5)
        {
            // Сообщение в чат (глобальное)
            std::string levelAnnouncement = "|cffFFFF00==========================================|r\n"
                                           "|cffFFFF00[Сервер]|r |cff00FF00ПРОГРЕСС В ХАРДКОРЕ|r\n"
                                           "|cff00FF00" + player->GetName() + "|r достиг |cffFFFF00" + std::to_string(level) + " уровня|r\n"
                                           "в режиме |cffFF0000Без права на ошибку|r!\n"
                                           "|cffFF8800Путь продолжается...|r\n"
                                           "|cffFFFF00==========================================|r";
            ChatHandler(nullptr).SendWorldText(levelAnnouncement.c_str());
            
            // Глобальное оповещение на экране
            std::string screenNotification = player->GetName() + " достиг " + std::to_string(level) + " уровня в режиме ХАРДКОР!";
            sWorldSessionMgr->DoForAllOnlinePlayers([&screenNotification](Player* onlinePlayer)
            {
                if (WorldSession* session = onlinePlayer->GetSession())
                {
                    session->SendAreaTriggerMessage(screenNotification);
                }
            });
        }
        
        // Уведомление при достижении максимального уровня окончательной смерти
        if (sHardcore->hardcoreMaxDeathLevel > 0 && level == sHardcore->hardcoreMaxDeathLevel + 1)
        {
            // Убираем красную ауру (окончательная смерть отключена)
            uint32 spellId = sHardcore->hardcoreAuraSpellId;
            if (spellId && player->HasAura(spellId))
            {
                player->RemoveAura(spellId);
            }
            
            ChatHandler(player->GetSession()).SendSysMessage("|cff00FF00╔══════════════════════════════════════╗|r");
            ChatHandler(player->GetSession()).SendSysMessage("|cff00FF00║  ОКОНЧАТЕЛЬНАЯ СМЕРТЬ ОТКЛЮЧЕНА!    ║|r");
            ChatHandler(player->GetSession()).SendSysMessage("|cff00FF00╚══════════════════════════════════════╝|r");
            std::string levelMsg = "|cffFFFF00Вы достигли " + std::to_string(level) + " уровня!|r";
            ChatHandler(player->GetSession()).SendSysMessage(levelMsg.c_str());
            ChatHandler(player->GetSession()).SendSysMessage("|cffFFFF00Смерть больше не окончательна.|r");
            ChatHandler(player->GetSession()).SendSysMessage("|cff00FF00Хардкор-режим продолжает давать бонусы.|r");
            ChatHandler(player->GetSession()).SendSysMessage("|cffFF8800Красная аура снята.|r");
            
            // Глобальное объявление о достижении безопасного уровня
            std::string safetyAnnouncement = "|cff00FF00==========================================|r\n"
                                            "|cffFFFF00[Сервер]|r |cff00FF00РУБЕЖ ПРОЙДЕН|r\n"
                                            "|cff00FF00" + player->GetName() + "|r достиг |cffFFFF00" + std::to_string(level) + " уровня|r\n"
                                            "в режиме |cffFF0000Без права на ошибку|r!\n"
                                            "|cff00FF00Окончательная смерть больше не действует!|r\n"
                                            "|cff00FF00==========================================|r";
            ChatHandler(nullptr).SendWorldText(safetyAnnouncement.c_str());
            
            // Экранное уведомление
            std::string safetyScreenNotification = player->GetName() + " выжил до " + std::to_string(level) + " уровня в ХАРДКОРЕ!";
            sWorldSessionMgr->DoForAllOnlinePlayers([&safetyScreenNotification](Player* onlinePlayer)
            {
                if (WorldSession* session = onlinePlayer->GetSession())
                {
                    session->SendAreaTriggerMessage(safetyScreenNotification);
                }
            });
        }

        // Награды титулами
        if (sHardcore->hardcoreTitleRewards.find(level) != sHardcore->hardcoreTitleRewards.end())
        {
            uint32 titleId = sHardcore->hardcoreTitleRewards[level];
            CharTitlesEntry const* titleInfo = sCharTitlesStore.LookupEntry(titleId);
            if (titleInfo)
            {
                ChatHandler handler(player->GetSession());
                std::string titleNameStr = Acore::StringFormat(player->getGender() == GENDER_MALE ? titleInfo->nameMale[handler.GetSessionDbcLocale()] : titleInfo->nameFemale[handler.GetSessionDbcLocale()], player->GetName());
                player->SetTitle(titleInfo);
                handler.PSendSysMessage("|cffFFFF00[Хардкор]|r Получен титул: %s", titleNameStr.c_str());
                sHardcore->TriggerOnReward(player, HARDCORE_EVENT_REWARD, titleId); // ТРИГГЕР ХУКА
            }
        }

        // Награды талантами
        if (sHardcore->hardcoreTalentRewards.find(level) != sHardcore->hardcoreTalentRewards.end())
        {
            uint32 talentPoints = sHardcore->hardcoreTalentRewards[level];
            player->RewardExtraBonusTalentPoints(talentPoints);
            std::string talentMsg = "|cffFFFF00[Хардкор]|r Получено очков талантов: " + std::to_string(talentPoints);
            ChatHandler(player->GetSession()).SendSysMessage(talentMsg.c_str());
            sHardcore->TriggerOnReward(player, HARDCORE_EVENT_REWARD, talentPoints); // ТРИГГЕР ХУКА
        }

        // Награды достижениями
        if (sHardcore->hardcoreAchievementReward.find(level) != sHardcore->hardcoreAchievementReward.end())
        {
            uint32 achievementId = sHardcore->hardcoreAchievementReward[level];
            AchievementEntry const* achievementInfo = sAchievementStore.LookupEntry(achievementId);
            if (achievementInfo)
            {
                player->CompletedAchievement(achievementInfo);
                ChatHandler(player->GetSession()).PSendSysMessage("|cffFFFF00[Хардкор]|r Получено достижение!");
                sHardcore->TriggerOnReward(player, HARDCORE_EVENT_REWARD, achievementId); // ТРИГГЕР ХУКА
            }
        }

        // Награды предметами
        if (sHardcore->hardcoreItemRewards.find(level) != sHardcore->hardcoreItemRewards.end())
        {
            uint32 itemEntry = sHardcore->hardcoreItemRewards[level];
            uint32 itemAmount = sHardcore->hardcoreItemRewardAmount;
            player->SendItemRetrievalMail({ { itemEntry, itemAmount } });
            ChatHandler(player->GetSession()).PSendSysMessage("|cffFFFF00[Хардкор]|r Награда отправлена по почте!");
            sHardcore->TriggerOnReward(player, HARDCORE_EVENT_REWARD, itemEntry); // ТРИГГЕР ХУКА
        }

        // Автоматическое отключение режима при достижении уровня
        if (sHardcore->hardcoreDisableLevel > 0 && level == sHardcore->hardcoreDisableLevel)
        {
            player->UpdatePlayerSetting("mod-hardcore", SETTING_HARDCORE, 0);
            
            // Убираем красную ауру
            uint32 spellId = sHardcore->hardcoreAuraSpellId;
            if (spellId && player->HasAura(spellId))
            {
                player->RemoveAura(spellId);
            }
            
            // Триггер события успешного завершения хардкор-режима
            sHardcore->TriggerOnComplete(player);
            
            // Эпичное уведомление об отключении режима
            ChatHandler(player->GetSession()).SendSysMessage("|cff00FF00╔══════════════════════════════════════╗|r");
            ChatHandler(player->GetSession()).SendSysMessage("|cff00FF00║    ХАРДКОР-РЕЖИМ ЗАВЕРШЕН!          ║|r");
            ChatHandler(player->GetSession()).SendSysMessage("|cff00FF00╚══════════════════════════════════════╝|r");
            std::string completeLevelMsg = "|cffFFFF00Вы достигли " + std::to_string(level) + " уровня!|r";
            ChatHandler(player->GetSession()).SendSysMessage(completeLevelMsg.c_str());
            ChatHandler(player->GetSession()).SendSysMessage("|cff00FF00Режим хардкор автоматически отключен.|r");
            ChatHandler(player->GetSession()).SendSysMessage("|cffFFFF00Все ограничения сняты! Поздравляем!|r");
            
            // Глобальное оповещение о завершении испытания
            std::string completionAnnouncement = "|cffFFD700==============================================|r\n"
                                                "|cffFFD700                                              |r\n"
                                                "|cffFFD700        |cff00FF00ЭПИЧЕСКАЯ ПОБЕДА|r        |r\n"
                                                "|cffFFD700                                              |r\n"
                                                "|cffFFD700==============================================|r\n"
                                                "|cffFFFF00Игрок|r |cff00FF00" + player->GetName() + "|r\n"
                                                "достиг |cffFFD700" + std::to_string(level) + " уровня|r и успешно завершил\n"
                                                "режим |cffFF0000БЕЗ ПРАВА НА ОШИБКУ|r!\n"
                                                "|cff00FF00Ни одной смерти!|r\n"
                                                "|cffFF8800Легенда сервера!|r\n"
                                                "|cffFFD700==============================================|r";
            ChatHandler(nullptr).SendWorldText(completionAnnouncement.c_str());
            
            // Глобальное оповещение на экране
            const std::string screenNotification = player->GetName() + " ЗАВЕРШИЛ ХАРДКОР-РЕЖИМ НА " + std::to_string(level) + " УРОВНЕ! ЛЕГЕНДА!";
            sWorldSessionMgr->DoForAllOnlinePlayers([&screenNotification](Player* onlinePlayer)
            {
                if (WorldSession* session = onlinePlayer->GetSession())
                {
                    session->SendAreaTriggerMessage(screenNotification);
                }
            });
        }
    }
};

// Дополнительные ограничения режима хардкор
class Hardcore_AllScript : public AllCreatureScript
{
public:
    Hardcore_AllScript() : AllCreatureScript("Hardcore_AllScript") {}

    // Блокировка взаимодействия между хардкор и обычными игроками
    bool CanCreatureSendListInventory(Player* player, Creature* creature, uint32 /*vendorEntry*/)
    {
        // Блокировка аукциона
        if (sHardcore->hardcoreBlockAuction && sHardcore->isHardcorePlayer(player))
        {
            if (creature->IsAuctioner())
            {
                ChatHandler(player->GetSession()).SendSysMessage("|cffFF0000[Хардкор] Аукцион недоступен в режиме «Без права на ошибку»!|r");
                return false;
            }
        }
        
        // Блокировка гильдейского банка
        if (sHardcore->hardcoreBlockGuildBank && sHardcore->isHardcorePlayer(player))
        {
            if (creature->IsGuildMaster())
            {
                ChatHandler(player->GetSession()).SendSysMessage("|cffFF0000[Хардкор] Гильдейский банк недоступен в режиме «Без права на ошибку»!|r");
                return false;
            }
        }
        
        return true;
    }
};

class Hardcore_GroupScript : public GroupScript
{
public:
    Hardcore_GroupScript() : GroupScript("Hardcore_GroupScript") {}

    // Проверка при добавлении игрока в группу
    void OnAddMember(Group* group, ObjectGuid guid) override
    {
        Player* newPlayer = ObjectAccessor::FindPlayer(guid);
        if (!newPlayer)
            return;

        bool newPlayerIsHardcore = sHardcore->isHardcorePlayer(newPlayer);
        
        // Проверяем каждого члена группы
        for (GroupReference* itr = group->GetFirstMember(); itr != nullptr; itr = itr->next())
        {
            Player* groupMember = itr->GetSource();
            if (!groupMember || groupMember == newPlayer)
                continue;

            bool groupMemberIsHardcore = sHardcore->isHardcorePlayer(groupMember);

            // Хардкор игроки могут группироваться только с хардкор игроками
            if (newPlayerIsHardcore != groupMemberIsHardcore)
            {
                ChatHandler(newPlayer->GetSession()).SendSysMessage("|cffFF0000[Хардкор] Вы можете группироваться только с игроками того же режима!|r");
                group->RemoveMember(guid);
                return;
            }

            // Проверка разницы уровней для хардкор игроков
            if (newPlayerIsHardcore && groupMemberIsHardcore)
            {
                uint8 levelDiff = abs(int(newPlayer->GetLevel()) - int(groupMember->GetLevel()));
                if (levelDiff > sHardcore->hardcoreMaxLevelDifference)
                {
                    std::string levelDiffMsg = "|cffFF0000[Хардкор] Разница уровней не должна превышать " + 
                        std::to_string(sHardcore->hardcoreMaxLevelDifference) + "! (Разница: " + 
                        std::to_string(levelDiff) + ")|r";
                    ChatHandler(newPlayer->GetSession()).SendSysMessage(levelDiffMsg.c_str());
                    group->RemoveMember(guid);
                    return;
                }
            }
        }
    }
};

class Hardcore_BattlegroundScript : public AllBattlegroundScript
{
public:
    Hardcore_BattlegroundScript() : AllBattlegroundScript("Hardcore_BattlegroundScript") {}

    // Блокировка полей боя
    bool CanFillPlayersToBG(BattlegroundQueue* /*queue*/, Battleground* /*bg*/, BattlegroundBracketId /*bracket_id*/) override
    {
        return true;
    }
};

// Объявление функций из других файлов
void AddSC_HardcoreCommandScript();
void AddSC_item_hardcore_scroll();

void AddSC_mod_hardcore()
{
    new Hardcore_WorldScript();
    new Hardcore_PlayerScript();
    new Hardcore_AllScript();
    new Hardcore_GroupScript();
    new Hardcore_BattlegroundScript();
    AddSC_HardcoreCommandScript();
    AddSC_item_hardcore_scroll();
}
