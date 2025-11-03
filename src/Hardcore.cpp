/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license: https://github.com/azerothcore/azerothcore-wotlk/blob/master/LICENSE-AGPL3
 */

#include "Hardcore.h"
#include "WorldSessionMgr.h"
#include "WorldSession.h"

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

    void OnLogin(Player* player) override
    {
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
    
    // Блокировка почты
    void OnMailReceive(Player* player, Mail* /*mail*/) override
    {
        if (sHardcore->hardcoreBlockMail && sHardcore->isHardcorePlayer(player))
        {
            ChatHandler(player->GetSession()).SendSysMessage("|cffFF0000[Хардкор] Почта недоступна в режиме «Без права на ошибку»!|r");
        }
    }
    
    // Блокировка полей боя
    bool OnBeforeBuyItemFromVendor(Player* player, ObjectGuid /*vendorguid*/, uint32 /*vendorslot*/, uint32& /*item*/, uint8 /*count*/, uint8 /*bag*/, uint8 /*slot*/) override
    {
        return true;
    }
    
    // Блокировка обмена между хардкор и обычными игроками
    bool OnBeforePlayerTrade(Player* player, Player* target) override
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
    bool OnBeforeDuel(Player* player, Player* target) override
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
    
    // Блокировка баффов от не-хардкор игроков
    bool OnBeforeCastSpell(Player* caster, Spell* spell, bool /*triggered*/) override
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

    void OnPlayerReleasedGhost(Player* player) override
    {
        if (!sHardcore->isHardcorePlayer(player))
        {
            return;
        }
        
        player->UpdatePlayerSetting("mod-hardcore", HARDCORE_DEAD, 1);
        ChatHandler(player->GetSession()).PSendSysMessage("|cffFF0000Ваш хардкор-персонаж погиб навсегда. Воскрешение невозможно.|r");
    }

    void OnPVPKill(Player* killer, Player* killed) override
    {
        if (!sHardcore->isHardcorePlayer(killed))
        {
            return;
        }
        
        // Проверка: если уровень отключения достигнут - не применяем окончательную смерть
        if (sHardcore->hardcoreDisableLevel > 0 && killed->GetLevel() >= sHardcore->hardcoreDisableLevel)
        {
            ChatHandler(killed->GetSession()).PSendSysMessage("|cffFFFF00[Хардкор]|r Режим отключен на %u уровне - смерть не окончательна.", sHardcore->hardcoreDisableLevel);
            return;
        }
        
        // Проверка: если уровень игрока выше максимального для окончательной смерти
        if (sHardcore->hardcoreMaxDeathLevel > 0 && killed->GetLevel() > sHardcore->hardcoreMaxDeathLevel)
        {
            ChatHandler(killed->GetSession()).PSendSysMessage("|cffFFFF00[Хардкор]|r Ваш уровень выше %u - смерть не окончательна.", sHardcore->hardcoreMaxDeathLevel);
            return; // Не применяем окончательную смерть
        }
        
        killed->UpdatePlayerSetting("mod-hardcore", HARDCORE_DEAD, 1);
        killed->BuildPlayerRepop();
        killed->RepopAtGraveyard();
        
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
            std::string deathAnnouncement = "|cffFFFF00━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━|r\n"
                                           "|cffFFFF00[Сервер]|r |cffFF0000☠ СМЕРТЬ ☠|r\n"
                                           "|cffFF0000" + killed->GetName() + "|r (ур. " + std::to_string(killed->GetLevel()) + ") пал в бою с |cff00FFFF" + killerName + "|r!\n"
                                           "|cffFF8800Режим «Без права на ошибку»: последнее путешествие завершено.|r\n"
                                           "|cffFFFF00━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━|r";
            ChatHandler(nullptr).SendWorldText(deathAnnouncement.c_str());
            
            // Глобальное оповещение на экране
            const std::string screenNotification = "☠ " + killed->GetName() + " (ур. " + std::to_string(killed->GetLevel()) + ") погиб в режиме ХАРДКОР! ☠";
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
            ChatHandler(killed->GetSession()).PSendSysMessage("|cffFFFF00[Хардкор]|r Режим отключен на %u уровне - смерть не окончательна.", sHardcore->hardcoreDisableLevel);
            return;
        }
        
        // Проверка: если уровень игрока выше максимального для окончательной смерти
        if (sHardcore->hardcoreMaxDeathLevel > 0 && killed->GetLevel() > sHardcore->hardcoreMaxDeathLevel)
        {
            ChatHandler(killed->GetSession()).PSendSysMessage("|cffFFFF00[Хардкор]|r Ваш уровень выше %u - смерть не окончательна.", sHardcore->hardcoreMaxDeathLevel);
            return; // Не применяем окончательную смерть
        }
        
        killed->UpdatePlayerSetting("mod-hardcore", HARDCORE_DEAD, 1);
        killed->BuildPlayerRepop();
        killed->RepopAtGraveyard();
        
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
            std::string deathAnnouncement = "|cffFFFF00━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━|r\n"
                                           "|cffFFFF00[Сервер]|r |cffFF0000☠ СМЕРТЬ ☠|r\n"
                                           "|cffFF0000" + killed->GetName() + "|r (ур. " + std::to_string(killed->GetLevel()) + ") пал от |cffFF8800" + killerName + "|r!\n"
                                           "|cffFF8800Режим «Без права на ошибку»: приключение окончено.|r\n"
                                           "|cffFFFF00━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━|r";
            ChatHandler(nullptr).SendWorldText(deathAnnouncement.c_str());
            
            // Глобальное оповещение на экране
            const std::string screenNotification = "☠ " + killed->GetName() + " (ур. " + std::to_string(killed->GetLevel()) + ") погиб в режиме ХАРДКОР! ☠";
            sWorldSessionMgr->DoForAllOnlinePlayers([&screenNotification](Player* onlinePlayer)
            {
                if (WorldSession* session = onlinePlayer->GetSession())
                {
                    session->SendAreaTriggerMessage(screenNotification);
                }
            });
        }
    }

    bool CanRepopAtGraveyard(Player* player) override
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

    void OnGiveXP(Player* player, uint32& amount, Unit* /*victim*/, uint8 /*xpSource*/) override
    {
        if (!sHardcore->isHardcorePlayer(player))
        {
            return;
        }
        
        // Применяем множитель опыта
        amount *= sHardcore->hardcoreXPMultiplier;
    }

    // Модификация урона - наносимого
    void OnDamageDealt(Player* player, Unit* /*victim*/, uint32& damage, DamageEffectType /*damageType*/) override
    {
        if (!sHardcore->isHardcorePlayer(player))
        {
            return;
        }
        
        // Применяем ослабление урона (-10% по умолчанию)
        damage *= sHardcore->hardcoreDamageDealtModifier;
    }
    
    // Модификация урона - получаемого
    void OnTakeDamage(Player* player, Unit* /*attacker*/, uint32& damage, DamageEffectType /*damageType*/) override
    {
        if (!sHardcore->isHardcorePlayer(player))
        {
            return;
        }
        
        // Применяем усиление получаемого урона (+50% по умолчанию)
        damage *= sHardcore->hardcoreDamageTakenModifier;
    }

    void OnLevelChanged(Player* player, uint8 /*oldLevel*/) override
    {
        if (!sHardcore->isHardcorePlayer(player))
        {
            return;
        }

        uint8 level = player->GetLevel();
        
        // Уведомление при достижении максимального уровня окончательной смерти
        if (sHardcore->hardcoreMaxDeathLevel > 0 && level == sHardcore->hardcoreMaxDeathLevel + 1)
        {
            ChatHandler(player->GetSession()).PSendSysMessage("|cff00FF00╔══════════════════════════════════════╗|r");
            ChatHandler(player->GetSession()).PSendSysMessage("|cff00FF00║  ОКОНЧАТЕЛЬНАЯ СМЕРТЬ ОТКЛЮЧЕНА!    ║|r");
            ChatHandler(player->GetSession()).PSendSysMessage("|cff00FF00╚══════════════════════════════════════╝|r");
            ChatHandler(player->GetSession()).PSendSysMessage("|cffFFFF00Вы достигли %u уровня!|r", level);
            ChatHandler(player->GetSession()).PSendSysMessage("|cffFFFF00Смерть больше не окончательна.|r");
            ChatHandler(player->GetSession()).PSendSysMessage("|cff00FF00Хардкор-режим продолжает давать бонусы.|r");
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
            }
        }

        // Награды талантами
        if (sHardcore->hardcoreTalentRewards.find(level) != sHardcore->hardcoreTalentRewards.end())
        {
            uint32 talentPoints = sHardcore->hardcoreTalentRewards[level];
            player->RewardExtraBonusTalentPoints(talentPoints);
            ChatHandler(player->GetSession()).PSendSysMessage("|cffFFFF00[Хардкор]|r Получено очков талантов: %u", talentPoints);
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
            }
        }

        // Награды предметами
        if (sHardcore->hardcoreItemRewards.find(level) != sHardcore->hardcoreItemRewards.end())
        {
            uint32 itemEntry = sHardcore->hardcoreItemRewards[level];
            uint32 itemAmount = sHardcore->hardcoreItemRewardAmount;
            player->SendItemRetrievalMail({ { itemEntry, itemAmount } });
            ChatHandler(player->GetSession()).PSendSysMessage("|cffFFFF00[Хардкор]|r Награда отправлена по почте!");
        }

        // Автоматическое отключение режима при достижении уровня
        if (sHardcore->hardcoreDisableLevel > 0 && level >= sHardcore->hardcoreDisableLevel)
        {
            player->UpdatePlayerSetting("mod-hardcore", SETTING_HARDCORE, 0);
            
            // Убираем красную ауру
            uint32 spellId = sHardcore->hardcoreAuraSpellId;
            if (spellId && player->HasAura(spellId))
            {
                player->RemoveAura(spellId);
            }
            
            // Эпичное уведомление об отключении режима
            ChatHandler(player->GetSession()).PSendSysMessage("|cff00FF00╔══════════════════════════════════════╗|r");
            ChatHandler(player->GetSession()).PSendSysMessage("|cff00FF00║    ХАРДКОР-РЕЖИМ ЗАВЕРШЕН!          ║|r");
            ChatHandler(player->GetSession()).PSendSysMessage("|cff00FF00╚══════════════════════════════════════╝|r");
            ChatHandler(player->GetSession()).PSendSysMessage("|cffFFFF00Вы достигли %u уровня!|r", level);
            ChatHandler(player->GetSession()).PSendSysMessage("|cff00FF00Режим хардкор автоматически отключен.|r");
            ChatHandler(player->GetSession()).PSendSysMessage("|cffFFFF00Все ограничения сняты! Поздравляем!|r");
            
            // Глобальное оповещение о завершении испытания
            std::string completionAnnouncement = "|cff00FF00━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━|r\n"
                                                "|cffFFFF00[Сервер]|r |cff00FF00⚔ ПОБЕДА ⚔|r\n"
                                                "|cff00FF00" + player->GetName() + "|r достиг |cffFFFF00" + std::to_string(level) + " уровня|r\n"
                                                "в режиме |cffFF0000«Без права на ошибку»|r!\n"
                                                "|cff00FF00Испытание успешно пройдено!|r\n"
                                                "|cff00FF00━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━|r";
            ChatHandler(nullptr).SendWorldText(completionAnnouncement.c_str());
            
            // Глобальное оповещение на экране
            const std::string screenNotification = "⚔ " + player->GetName() + " завершил испытание ХАРДКОР на " + std::to_string(level) + " уровне! ⚔";
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

class gobject_hardcore : public GameObjectScript
{
public:
    gobject_hardcore() : GameObjectScript("gobject_hardcore") { }

    struct gobject_hardcoreAI : GameObjectAI
    {
        explicit gobject_hardcoreAI(GameObject* object) : GameObjectAI(object) { };

        bool CanBeSeen(Player const* player) override
        {
            // Видим только для новых персонажей (1 уровень или 55 для ДК)
            if ((player->GetLevel() > 1 && player->getClass() != CLASS_DEATH_KNIGHT) || (player->GetLevel() > 55))
            {
                return false;
            }
            return sHardcore->enabled();
        }
    };

    bool OnGossipHello(Player* player, GameObject* go) override
    {
        if (!sHardcore->isHardcorePlayer(player))
        {
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Активировать режим Хардкор", 0, 1);
        }
        SendGossipMenuFor(player, 12669, go->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player* player, GameObject* /*go*/, uint32 /*sender*/, uint32 /*action*/) override
    {
        // Проверка: рыцари смерти не могут участвовать
        if (sHardcore->hardcoreBlockDeathKnight && player->getClass() == CLASS_DEATH_KNIGHT)
        {
            ChatHandler(player->GetSession()).SendSysMessage("|cffff0000Рыцари смерти НЕ МОГУТ принять участие в режиме «Без права на ошибку»!|r");
            CloseGossipMenuFor(player);
            return true;
        }
        
        // Проверка: только для новых персонажей
        bool eligible = (player->GetLevel() == 1);
        if (!eligible)
        {
            ChatHandler(player->GetSession()).SendSysMessage("|cffff0000Хардкор-режим можно активировать только новым персонажам на 1 уровне!|r");
            CloseGossipMenuFor(player);
            return true;
        }

        player->UpdatePlayerSetting("mod-hardcore", SETTING_HARDCORE, 1);
        
        // Применяем красную ауру
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
        
        // Сообщение игроку
        ChatHandler(player->GetSession()).PSendSysMessage("|cffFF0000╔══════════════════════════════════════╗|r");
        ChatHandler(player->GetSession()).PSendSysMessage("|cffFF0000║   РЕЖИМ ХАРДКОР АКТИВИРОВАН!        ║|r");
        ChatHandler(player->GetSession()).PSendSysMessage("|cffFF0000╚══════════════════════════════════════╝|r");
        ChatHandler(player->GetSession()).PSendSysMessage("|cffFFFF00У вас только ОДНА жизнь!|r");
        ChatHandler(player->GetSession()).PSendSysMessage("|cffFFFF00Смерть необратима - воскрешение невозможно.|r");
        ChatHandler(player->GetSession()).PSendSysMessage("|cff00FF00Красная аура показывает ваш статус.|r");
        
        // Информация об ограничении уровня
        if (sHardcore->hardcoreMaxDeathLevel > 0)
        {
            ChatHandler(player->GetSession()).PSendSysMessage("|cffFF8800Окончательная смерть действует до %u уровня.|r", sHardcore->hardcoreMaxDeathLevel);
        }
        if (sHardcore->hardcoreDisableLevel > 0)
        {
            ChatHandler(player->GetSession()).PSendSysMessage("|cffFF8800Режим автоматически отключится на %u уровне.|r", sHardcore->hardcoreDisableLevel);
        }
        
        // Глобальное оповещение (чат) - яркое и заметное
        std::string announcement = "|cffFFFF00━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━|r\n"
                                  "|cffFFFF00[Сервер]|r Игрок |cff00FF00" + player->GetName() + "|r\n"
                                  "принял испытание |cffFF0000⚔ ХАРДКОР ⚔|r!\n"
                                  "|cffFF8800Одна жизнь. Одна смерть. Одна судьба.|r\n"
                                  "|cffFFFF00━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━|r";
        ChatHandler(nullptr).SendWorldText(announcement.c_str());

        // Глобальное оповещение на экране - более эпичное
        const std::string screenNotification = "⚔ " + player->GetName() + " начал испытание ХАРДКОР! ⚔";
        sWorldSessionMgr->DoForAllOnlinePlayers([&screenNotification](Player* onlinePlayer)
        {
            if (WorldSession* session = onlinePlayer->GetSession())
            {
                session->SendAreaTriggerMessage(screenNotification);
            }
        });
        
        CloseGossipMenuFor(player);
        return true;
    }

    GameObjectAI* GetAI(GameObject* object) const override
    {
        return new gobject_hardcoreAI(object);
    }
};

// Дополнительные ограничения режима хардкор
class Hardcore_AllScript : public AllCreatureScript
{
public:
    Hardcore_AllScript() : AllCreatureScript("Hardcore_AllScript") {}

    // Блокировка взаимодействия между хардкор и обычными игроками
    bool CanCreatureSendListInventory(Player* player, Creature* creature, uint32 /*vendorEntry*/) override
    {
        // Блокировка аукциона
        if (sHardcore->hardcoreBlockAuction && sHardcore->isHardcorePlayer(player))
        {
            if (creature->IsAuctioneer())
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
    bool OnAddMember(Group* group, ObjectGuid guid) override
    {
        Player* newPlayer = ObjectAccessor::FindPlayer(guid);
        if (!newPlayer)
            return true;

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
                return false;
            }

            // Проверка разницы уровней для хардкор игроков
            if (newPlayerIsHardcore && groupMemberIsHardcore)
            {
                uint8 levelDiff = abs(int(newPlayer->GetLevel()) - int(groupMember->GetLevel()));
                if (levelDiff > sHardcore->hardcoreMaxLevelDifference)
                {
                    ChatHandler(newPlayer->GetSession()).PSendSysMessage("|cffFF0000[Хардкор] Разница уровней не должна превышать %u! (Разница: %u)|r", 
                        sHardcore->hardcoreMaxLevelDifference, levelDiff);
                    return false;
                }
            }
        }

        return true;
    }
};

class Hardcore_BattlegroundScript : public BGScript
{
public:
    Hardcore_BattlegroundScript() : BGScript("Hardcore_BattlegroundScript") {}

    // Блокировка полей боя
    bool CanFillPlayersToBG(Battleground* /*bg*/, BattlegroundTypeId /*bgTypeId*/) override
    {
        return true;
    }
};

void AddSC_mod_hardcore()
{
    new Hardcore_WorldScript();
    new Hardcore_PlayerScript();
    new Hardcore_AllScript();
    new Hardcore_GroupScript();
    new Hardcore_BattlegroundScript();
    new gobject_hardcore();
}
