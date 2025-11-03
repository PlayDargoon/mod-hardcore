/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license: https://github.com/azerothcore/azerothcore-wotlk/blob/master/LICENSE-AGPL3
 */

#include "Hardcore.h"
#include "ScriptMgr.h"
#include "Chat.h"
#include "Player.h"

using namespace Acore::ChatCommands;

class HardcoreCommandScript : public CommandScript
{
public:
    HardcoreCommandScript() : CommandScript("HardcoreCommandScript") { }

    ChatCommandTable GetCommands() const override
    {
        static ChatCommandTable hardcoreCommandTable =
        {
            { "status",  HandleHardcoreStatusCommand,  SEC_PLAYER, Console::No },
            { "info",    HandleHardcoreInfoCommand,    SEC_PLAYER, Console::No },
        };

        static ChatCommandTable commandTable =
        {
            { "hardcore", hardcoreCommandTable },
        };

        return commandTable;
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
            handler->PSendSysMessage("|cff00FF00Режим: АКТИВЕН|r");
            handler->PSendSysMessage("|cffFFFF00Уровень: %u|r", player->GetLevel());
            
            if (isDead)
            {
                handler->PSendSysMessage("|cffFF0000Статус: ПОГИБ НАВСЕГДА|r");
            }
            else
            {
                handler->PSendSysMessage("|cff00FF00Статус: ЖИВ|r");
            }

            // Информация об окончательной смерти
            if (sHardcore->hardcoreMaxDeathLevel > 0)
            {
                if (player->GetLevel() > sHardcore->hardcoreMaxDeathLevel)
                {
                    handler->PSendSysMessage("|cff00FF00Окончательная смерть: ОТКЛЮЧЕНА|r");
                }
                else
                {
                    handler->PSendSysMessage("|cffFF8800Окончательная смерть до уровня: %u|r", sHardcore->hardcoreMaxDeathLevel);
                }
            }
            else
            {
                handler->PSendSysMessage("|cffFF0000Окончательная смерть: ВСЕГДА|r");
            }

            // Информация об отключении режима
            if (sHardcore->hardcoreDisableLevel > 0)
            {
                if (player->GetLevel() >= sHardcore->hardcoreDisableLevel)
                {
                    handler->PSendSysMessage("|cff00FF00Режим: ЗАВЕРШЕН|r");
                }
                else
                {
                    handler->PSendSysMessage("|cffFFFF00Режим завершится на уровне: %u|r", sHardcore->hardcoreDisableLevel);
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
                        handler->PSendSysMessage("|cffFF8800Кулдаун подземелий: %u ч. %u мин.|r", hoursLeft, minutesLeft);
                    }
                    else
                    {
                        handler->PSendSysMessage("|cff00FF00Кулдаун подземелий: готово|r");
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
            handler->PSendSysMessage("• Группы: ±%u уровней", sHardcore->hardcoreMaxLevelDifference);
        if (sHardcore->hardcoreDungeonCooldown > 0)
            handler->PSendSysMessage("• Кулдаун подземелий: %u ч.", sHardcore->hardcoreDungeonCooldown);

        handler->PSendSysMessage(" ");
        
        if (sHardcore->hardcoreMaxDeathLevel > 0)
        {
            handler->PSendSysMessage("|cffFF8800Окончательная смерть до %u уровня|r", sHardcore->hardcoreMaxDeathLevel);
        }
        
        if (sHardcore->hardcoreDisableLevel > 0)
        {
            handler->PSendSysMessage("|cffFF8800Автоотключение на %u уровне|r", sHardcore->hardcoreDisableLevel);
        }

        handler->PSendSysMessage(" ");
        handler->PSendSysMessage("|cffFFFF00Используйте .hardcore status для проверки вашего статуса.|r");

        return true;
    }
};

void AddSC_HardcoreCommandScript()
{
    new HardcoreCommandScript();
}
