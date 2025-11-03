/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license: https://github.com/azerothcore/azerothcore-wotlk/blob/master/LICENSE-AGPL3
 */

#include "Hardcore.h"
#include "ScriptMgr.h"
#include "Chat.h"
#include "Player.h"
#include "ObjectAccessor.h"

using namespace Acore::ChatCommands;

class HardcoreCommandScript : public CommandScript
{
public:
    HardcoreCommandScript() : CommandScript("HardcoreCommandScript") { }

    ChatCommandTable GetCommands() const override
    {
        static ChatCommandTable hardcoreCommandTable =
        {
            { "start",   HandleHardcoreStartCommand,   SEC_PLAYER, Console::No },
            { "decline", HandleHardcoreDeclineCommand, SEC_PLAYER, Console::No },
            { "status",  HandleHardcoreStatusCommand,  SEC_PLAYER, Console::No },
            { "info",    HandleHardcoreInfoCommand,    SEC_PLAYER, Console::No },
            { "top",     HandleHardcoreTopCommand,     SEC_PLAYER, Console::No },
        };

        static ChatCommandTable commandTable =
        {
            { "hardcore", hardcoreCommandTable },
        };

        return commandTable;
    }

    static bool HandleHardcoreDeclineCommand(ChatHandler* handler)
    {
        Player* player = handler->GetSession()->GetPlayer();
        if (!player)
            return false;

        if (!sHardcore->enabled())
        {
            handler->SendSysMessage("|cffFF0000Режим хардкор отключен на сервере.|r");
            return true;
        }

        // Проверка: уже активирован хардкор?
        if (sHardcore->isHardcorePlayer(player))
        {
            handler->SendSysMessage("|cffFF8800Вы уже активировали режим хардкор!|r");
            handler->SendSysMessage("|cffFFFF00Отказаться невозможно - испытание принято.|r");
            return true;
        }

        // Проверка: слишком высокий уровень?
        if (player->GetLevel() > 1)
        {
            handler->SendSysMessage("|cffFF8800Вы уже прошли 1 уровень.|r");
            handler->SendSysMessage("|cffFFFF00Свиток испытания больше не актуален.|r");
            
            // Удаляем предмет если есть
            player->DestroyItemCount(60000, 1, true);
            return true;
        }

        // Отказ от испытания
        handler->SendSysMessage(" ");
        handler->SendSysMessage("|cffFF0000========================================|r");
        handler->SendSysMessage("|cffFF0000   ВЫ ОТКАЗАЛИСЬ ОТ ИСПЫТАНИЯ|r");
        handler->SendSysMessage("|cffFF0000========================================|r");
        handler->SendSysMessage(" ");
        handler->SendSysMessage("|cffFFFF00Свиток испытания Хардкор удален.|r");
        handler->SendSysMessage("|cffFFFF00Вы больше не сможете активировать этот режим.|r");
        handler->SendSysMessage("|cff00FF00Удачи в обычных приключениях!|r");
        handler->SendSysMessage(" ");

        // Сохраняем отказ в настройках персонажа
        player->UpdatePlayerSetting("mod-hardcore", 10, 1); // 10 = HARDCORE_DECLINED
        
        // Удаляем предмет
        player->DestroyItemCount(60000, 1, true);

        return true;
    }

    static bool HandleHardcoreStatusCommand(ChatHandler* handler)
    {
        Player* player = handler->GetSession()->GetPlayer();
        if (!player)
            return false;

        if (!sHardcore->enabled())
        {
            handler->SendSysMessage("|cffFF0000Режим хардкор отключен на сервере.|r");
            return true;
        }

        bool isHardcore = sHardcore->isHardcorePlayer(player);
        bool isDead = sHardcore->isHardcoreDead(player);

        handler->PSendSysMessage("|cffFFFF00╔══════════════════════════════════════╗|r");
        handler->PSendSysMessage("|cffFFFF00║     СТАТУС ХАРДКОР-РЕЖИМА           ║|r");
        handler->PSendSysMessage("|cffFFFF00╚══════════════════════════════════════╝|r");

        if (isHardcore)
        {
            handler->SendSysMessage("|cff00FF00Режим: АКТИВЕН|r");
            std::string levelMsg = "|cffFFFF00Уровень: " + std::to_string(player->GetLevel()) + "|r";
            handler->SendSysMessage(levelMsg.c_str());
            
            if (isDead)
            {
                handler->SendSysMessage("|cffFF0000Статус: ПОГИБ НАВСЕГДА|r");
            }
            else
            {
                handler->SendSysMessage("|cff00FF00Статус: ЖИВ|r");
            }

            // Информация об окончательной смерти
            if (sHardcore->hardcoreMaxDeathLevel > 0)
            {
                if (player->GetLevel() > sHardcore->hardcoreMaxDeathLevel)
                {
                    handler->SendSysMessage("|cff00FF00Окончательная смерть: ОТКЛЮЧЕНА|r");
                }
                else
                {
                    std::string maxDeathMsg = "|cffFF8800Окончательная смерть до уровня: " + std::to_string(sHardcore->hardcoreMaxDeathLevel) + "|r";
                    handler->SendSysMessage(maxDeathMsg.c_str());
                }
            }
            else
            {
                handler->SendSysMessage("|cffFF0000Окончательная смерть: ВСЕГДА|r");
            }

            // Информация об отключении режима
            if (sHardcore->hardcoreDisableLevel > 0)
            {
                if (player->GetLevel() >= sHardcore->hardcoreDisableLevel)
                {
                    handler->SendSysMessage("|cff00FF00Режим: ЗАВЕРШЕН|r");
                }
                else
                {
                    std::string disableLevelMsg = "|cffFFFF00Режим завершится на уровне: " + std::to_string(sHardcore->hardcoreDisableLevel) + "|r";
                    handler->SendSysMessage(disableLevelMsg.c_str());
                }
            }

            // Информация о кулдауне подземелий
            if (sHardcore->hardcoreDungeonCooldown > 0)
            {
                uint32 lastDungeonTime = player->GetPlayerSetting("mod-hardcore", HARDCORE_LAST_DUNGEON_TIME).value;
                if (lastDungeonTime > 0)
                {
                    uint32 currentTime = uint32(time(nullptr));
                    uint32 cooldownSeconds = sHardcore->hardcoreDungeonCooldown * HOUR;
                    
                    if ((currentTime - lastDungeonTime) < cooldownSeconds)
                    {
                        uint32 remainingTime = cooldownSeconds - (currentTime - lastDungeonTime);
                        uint32 hoursLeft = remainingTime / HOUR;
                        uint32 minutesLeft = (remainingTime % HOUR) / MINUTE;
                        std::string cooldownMsg = "|cffFF8800Кулдаун подземелий: " + std::to_string(hoursLeft) + " ч. " + std::to_string(minutesLeft) + " мин.|r";
                        handler->SendSysMessage(cooldownMsg.c_str());
                    }
                    else
                    {
                        handler->SendSysMessage("|cff00FF00Кулдаун подземелий: готово|r");
                    }
                }
            }
        }
        else
        {
            handler->PSendSysMessage("|cffFF8800Режим: НЕ АКТИВЕН|r");
            handler->PSendSysMessage("|cffFFFF00Используйте игровой объект для активации.|r");
        }

        return true;
    }

    static bool HandleHardcoreInfoCommand(ChatHandler* handler)
    {
        if (!sHardcore->enabled())
        {
            handler->SendSysMessage("|cffFF0000Режим хардкор отключен на сервере.|r");
            return true;
        }

        handler->PSendSysMessage("|cffFFFF00╔══════════════════════════════════════╗|r");
        handler->PSendSysMessage("|cffFFFF00║   ИНФОРМАЦИЯ О ХАРДКОР-РЕЖИМЕ       ║|r");
        handler->PSendSysMessage("|cffFFFF00╚══════════════════════════════════════╝|r");
        handler->PSendSysMessage("|cff00FF00Режим «Без права на ошибку»|r");
        handler->PSendSysMessage(" ");
        handler->PSendSysMessage("|cffFFFF00Особенности:|r");
        handler->PSendSysMessage("• Только одна жизнь");
        handler->PSendSysMessage("• Смерть необратима");
        handler->PSendSysMessage("• Воскрешение невозможно");
        handler->PSendSysMessage(" ");
        handler->PSendSysMessage("|cffFFFF00Модификаторы:|r");
        handler->PSendSysMessage("• Урон: %.0f%%", sHardcore->hardcoreDamageDealtModifier * 100);
        handler->PSendSysMessage("• Получаемый урон: %.0f%%", sHardcore->hardcoreDamageTakenModifier * 100);
        
        if (sHardcore->hardcoreXPMultiplier != 1.0f)
        {
            handler->PSendSysMessage("• Опыт: %.0f%%", sHardcore->hardcoreXPMultiplier * 100);
        }
        
        handler->PSendSysMessage(" ");
        handler->PSendSysMessage("|cffFFFF00Ограничения:|r");
        
        if (sHardcore->hardcoreBlockDeathKnight)
            handler->PSendSysMessage("• Рыцари смерти запрещены");
        if (sHardcore->hardcoreBlockAuction)
            handler->PSendSysMessage("• Аукцион заблокирован");
        if (sHardcore->hardcoreBlockMail)
            handler->PSendSysMessage("• Почта заблокирована");
        if (sHardcore->hardcoreBlockGuildBank)
            handler->PSendSysMessage("• Гильдейский банк заблокирован");
        if (sHardcore->hardcoreBlockBattleground)
            handler->PSendSysMessage("• Поля боя заблокированы");
        if (sHardcore->hardcoreBlockArena)
            handler->PSendSysMessage("• Арена заблокирована");
        if (sHardcore->hardcoreForcePvE)
            handler->PSendSysMessage("• Принудительный PvE");
        if (sHardcore->hardcoreMaxLevelDifference > 0)
        {
            std::string groupMsg = "• Группы: ±" + std::to_string(sHardcore->hardcoreMaxLevelDifference) + " уровней";
            handler->SendSysMessage(groupMsg.c_str());
        }
        if (sHardcore->hardcoreDungeonCooldown > 0)
        {
            std::string cooldownMsg = "• Кулдаун подземелий: " + std::to_string(sHardcore->hardcoreDungeonCooldown) + " ч.";
            handler->SendSysMessage(cooldownMsg.c_str());
        }

        handler->SendSysMessage(" ");
        
        if (sHardcore->hardcoreMaxDeathLevel > 0)
        {
            std::string maxDeathMsg = "|cffFF8800Окончательная смерть до " + std::to_string(sHardcore->hardcoreMaxDeathLevel) + " уровня|r";
            handler->SendSysMessage(maxDeathMsg.c_str());
        }
        
        if (sHardcore->hardcoreDisableLevel > 0)
        {
            std::string disableMsg = "|cffFF8800Автоотключение на " + std::to_string(sHardcore->hardcoreDisableLevel) + " уровне|r";
            handler->SendSysMessage(disableMsg.c_str());
        }

        handler->PSendSysMessage(" ");
        handler->PSendSysMessage("|cffFFFF00Используйте .hardcore status для проверки вашего статуса.|r");

        return true;
    }

    static bool HandleHardcoreStartCommand(ChatHandler* handler)
    {
        Player* player = handler->GetSession()->GetPlayer();
        if (!player)
            return false;

        if (!sHardcore->enabled())
        {
            handler->SendSysMessage("|cffFF0000Режим хардкор отключен на сервере.|r");
            return true;
        }

        // Проверка: уже активирован?
        if (sHardcore->isHardcorePlayer(player))
        {
            handler->SendSysMessage("|cffFF8800Режим хардкор уже активирован для вашего персонажа!|r");
            handler->SendSysMessage("|cffFFFF00Используйте .hardcore status для проверки статуса.|r");
            return true;
        }

        // Проверка: только 1 уровень (или 55 для ДК)
        uint8 requiredLevel = (player->getClass() == CLASS_DEATH_KNIGHT) ? 55 : 1;
        if (player->GetLevel() != requiredLevel)
        {
            std::string msg = "|cffFF0000Слишком поздно! Хардкор можно активировать только на " + 
                             std::to_string(requiredLevel) + " уровне!|r";
            handler->SendSysMessage(msg.c_str());
            handler->SendSysMessage("|cffFF8800Используйте 'Свиток испытания Хардкор' в инвентаре для активации.|r");
            return true;
        }

        // Проверка: запрещённый класс?
        if (sHardcore->hardcoreBlockDeathKnight && player->getClass() == CLASS_DEATH_KNIGHT)
        {
            handler->SendSysMessage("|cffFF0000Рыцари смерти не могут использовать хардкор-режим!|r");
            return true;
        }

        // АКТИВАЦИЯ ХАРДКОРА
        player->SetPlayerSetting("mod-hardcore", SETTING_HARDCORE, 1);
        
        // Применить красную ауру
        uint32 spellId = sHardcore->hardcoreAuraSpellId;
        if (spellId && !player->HasAura(spellId))
        {
            player->AddAura(spellId, player);
        }

        // ТРИГГЕР ХУКА: Активация хардкора
        sHardcore->TriggerOnActivate(player);
        
        // Сообщение игроку
        handler->SendSysMessage("|cffFF0000========================================|r");
        handler->SendSysMessage("|cffFF0000   РЕЖИМ ХАРДКОР АКТИВИРОВАН!|r");
        handler->SendSysMessage("|cffFF0000========================================|r");
        handler->SendSysMessage(" ");
        handler->SendSysMessage("|cffFFFF00У вас только ОДНА жизнь!|r");
        handler->SendSysMessage("|cffFFFF00Смерть необратима - воскрешение невозможно.|r");
        handler->SendSysMessage("|cff00FF00Красная аура показывает ваш статус.|r");
        
        // Информация об ограничении уровня
        if (sHardcore->hardcoreMaxDeathLevel > 0)
        {
            std::string maxDeathMsg = "|cffFF8800Окончательная смерть действует до " + 
                                     std::to_string(sHardcore->hardcoreMaxDeathLevel) + " уровня.|r";
            handler->SendSysMessage(maxDeathMsg.c_str());
        }
        if (sHardcore->hardcoreDisableLevel > 0)
        {
            std::string disableLevelMsg = "|cffFF8800Режим автоматически отключится на " + 
                                         std::to_string(sHardcore->hardcoreDisableLevel) + " уровне.|r";
            handler->SendSysMessage(disableLevelMsg.c_str());
        }
        
        // Глобальное оповещение (чат)
        std::string announcement = "|cffFFFF00==========================================|r\n"
                                  "|cffFFFF00[Сервер]|r Игрок |cff00FF00" + player->GetName() + "|r\n"
                                  "принял испытание |cffFF0000ХАРДКОР|r!\n"
                                  "|cffFF8800Одна жизнь. Одна смерть. Одна судьба.|r\n"
                                  "|cffFFFF00==========================================|r";
        ChatHandler(nullptr).SendWorldText(announcement.c_str());

        // Глобальное оповещение на экране
        const std::string screenNotification = player->GetName() + " начал испытание ХАРДКОР!";
        sWorldSessionMgr->DoForAllOnlinePlayers([&screenNotification](Player* onlinePlayer)
        {
            if (WorldSession* session = onlinePlayer->GetSession())
            {
                session->SendAreaTriggerMessage(screenNotification);
            }
        });

        return true;
    }

    static bool HandleHardcoreTopCommand(ChatHandler* handler)
    {
        if (!sHardcore->enabled())
        {
            handler->SendSysMessage("|cffFF0000Режим хардкор отключен на сервере.|r");
            return true;
        }

        // Собираем всех онлайн хардкор-игроков
        struct HardcorePlayerInfo
        {
            std::string name;
            uint8 level;
            uint8 classId;
            bool isDead;
        };

        std::vector<HardcorePlayerInfo> hardcorePlayers;

        // Перебираем всех онлайн игроков через ObjectAccessor
        HashMapHolder<Player>::MapType const& players = ObjectAccessor::GetPlayers();
        for (HashMapHolder<Player>::MapType::const_iterator itr = players.begin(); itr != players.end(); ++itr)
        {
            Player* player = itr->second;
            if (player)
            {
                if (sHardcore->isHardcorePlayer(player))
                {
                    HardcorePlayerInfo info;
                    info.name = player->GetName();
                    info.level = player->GetLevel();
                    info.classId = player->getClass();
                    info.isDead = sHardcore->isHardcoreDead(player);
                    
                    // Добавляем только живых игроков
                    if (!info.isDead)
                    {
                        hardcorePlayers.push_back(info);
                    }
                }
            }
        }

        // Сортируем по уровню (от большего к меньшему)
        std::sort(hardcorePlayers.begin(), hardcorePlayers.end(),
            [](const HardcorePlayerInfo& a, const HardcorePlayerInfo& b) {
                return a.level > b.level;
            });

        // Выводим таблицу лидеров
        handler->SendSysMessage("|cffFFD700==========================================|r");
        handler->SendSysMessage("|cffFFD700    ТАБЛИЦА ЛИДЕРОВ ХАРДКОР|r");
        handler->SendSysMessage("|cffFFD700==========================================|r");
        handler->SendSysMessage(" ");

        if (hardcorePlayers.empty())
        {
            handler->SendSysMessage("|cffFF8800Сейчас нет активных хардкор-игроков онлайн.|r");
        }
        else
        {
            handler->SendSysMessage("|cffFFFF00ТОП-10 ЖИВЫХ ХАРДКОР-ГЕРОЕВ:|r");
            handler->SendSysMessage(" ");

            uint32 count = 0;
            for (const auto& info : hardcorePlayers)
            {
                if (++count > 10) // Показываем только топ-10
                    break;

                // Определяем цвет класса
                std::string classColor;
                switch (info.classId)
                {
                    case CLASS_WARRIOR:      classColor = "|cffC79C6E"; break; // Коричневый
                    case CLASS_PALADIN:      classColor = "|cffF58CBA"; break; // Розовый
                    case CLASS_HUNTER:       classColor = "|cffABD473"; break; // Зелёный
                    case CLASS_ROGUE:        classColor = "|cffFFF569"; break; // Жёлтый
                    case CLASS_PRIEST:       classColor = "|cffFFFFFF"; break; // Белый
                    case CLASS_DEATH_KNIGHT: classColor = "|cffC41F3B"; break; // Красный
                    case CLASS_SHAMAN:       classColor = "|cff0070DE"; break; // Синий
                    case CLASS_MAGE:         classColor = "|cff69CCF0"; break; // Голубой
                    case CLASS_WARLOCK:      classColor = "|cff9482C9"; break; // Фиолетовый
                    case CLASS_DRUID:        classColor = "|cffFF7D0A"; break; // Оранжевый
                    default:                 classColor = "|cffFFFFFF"; break;
                }

                // Медали для топ-3
                std::string medal;
                if (count == 1) medal = "|cffFFD700[1]|r";
                else if (count == 2) medal = "|cffC0C0C0[2]|r";
                else if (count == 3) medal = "|cffCD7F32[3]|r";
                else medal = "[" + std::to_string(count) + "]";

                std::string msg = medal + " " + classColor + info.name + "|r |cffFFFF00(ур. " + 
                                 std::to_string(info.level) + ")|r";
                
                handler->SendSysMessage(msg.c_str());
            }

            handler->SendSysMessage(" ");
            std::string totalMsg = "|cff00FF00Всего активных: " + std::to_string(hardcorePlayers.size()) + " игроков|r";
            handler->SendSysMessage(totalMsg.c_str());
        }

        handler->SendSysMessage(" ");
        handler->SendSysMessage("|cffFFD700==========================================|r");

        return true;
    }
};

void AddSC_HardcoreCommandScript()
{
    new HardcoreCommandScript();
}
