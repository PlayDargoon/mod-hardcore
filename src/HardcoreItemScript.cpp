/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license: https://github.com/azerothcore/azerothcore-wotlk/blob/master/LICENSE-AGPL3
 */

#include "Hardcore.h"
#include "ScriptMgr.h"
#include "Player.h"
#include "Item.h"
#include "Chat.h"
#include "WorldSession.h"

#define HARDCORE_ITEM_ENTRY 60000

class item_hardcore_scroll : public ItemScript
{
public:
    item_hardcore_scroll() : ItemScript("item_hardcore_scroll") { }

    bool OnUse(Player* player, Item* item, SpellCastTargets const& /*targets*/) override
    {
        if (!player || !item)
            return false;

        if (!sHardcore->enabled())
        {
            ChatHandler(player->GetSession()).SendSysMessage("|cffFF0000Режим хардкор отключен на сервере.|r");
            return false;
        }

        // Показываем меню с опциями
        ShowHardcoreMenu(player);
        
        return false; // Не удаляем предмет
    }

private:
    void ShowHardcoreMenu(Player* player)
    {
        ChatHandler handler(player->GetSession());
        
        handler.SendSysMessage(" ");
        handler.SendSysMessage("|cffFFD700==========================================|r");
        handler.SendSysMessage("|cffFFD700   СВИТОК ИСПЫТАНИЯ ХАРДКОР|r");
        handler.SendSysMessage("|cffFFD700==========================================|r");
        handler.SendSysMessage(" ");
        
        bool isHardcore = sHardcore->isHardcorePlayer(player);
        
        if (!isHardcore)
        {
            // Игрок еще не активировал хардкор
            if (player->GetLevel() > sHardcore->hardcoreMaxStartingLevel)
            {
                std::string msg = "|cffFF0000НЕДОСТУПНО: Слишком высокий уровень!|r";
                handler.SendSysMessage(msg.c_str());
                std::string requireMsg = "|cffFF8800Хардкор можно активировать только на " + 
                                        std::to_string(sHardcore->hardcoreMaxStartingLevel) + " уровне или ниже.|r";
                handler.SendSysMessage(requireMsg.c_str());
            }
            else if (sHardcore->hardcoreBlockDeathKnight && player->getClass() == CLASS_DEATH_KNIGHT)
            {
                handler.SendSysMessage("|cffFF0000НЕДОСТУПНО: Рыцари смерти запрещены!|r");
            }
            else
            {
                handler.SendSysMessage("|cff00FF00ДОСТУПНА АКТИВАЦИЯ РЕЖИМА ХАРДКОР!|r");
                handler.SendSysMessage(" ");
                handler.SendSysMessage("|cffFFFF00Команды:|r");
                handler.SendSysMessage("|cffFFFF00• .hardcore start|r - |cff00FF00Активировать режим|r");
                handler.SendSysMessage("|cffFFFF00• .hardcore info|r - Информация о режиме");
                handler.SendSysMessage("|cffFFFF00• .hardcore top|r - Таблица лидеров");
                handler.SendSysMessage(" ");
                handler.SendSysMessage("|cffFF0000⚠ ВНИМАНИЕ! АКТИВАЦИЯ НЕОБРАТИМА! ⚠|r");
                handler.SendSysMessage("|cffFF8800После активации:|r");
                handler.SendSysMessage("|cffFF8800• Смерть окончательна (до определенного уровня)|r");
                handler.SendSysMessage("|cffFF8800• Воскрешение невозможно|r");
                handler.SendSysMessage("|cffFF8800• Ограничения на группы, почту, аукцион|r");
            }
        }
        else
        {
            // Игрок уже в хардкор-режиме
            handler.SendSysMessage("|cff00FF00РЕЖИМ ХАРДКОР: АКТИВЕН|r");
            handler.SendSysMessage(" ");
            
            // Краткий статус
            std::string levelMsg = "|cffFFFF00Уровень: " + std::to_string(player->GetLevel()) + "|r";
            handler.SendSysMessage(levelMsg.c_str());
            
            bool isDead = sHardcore->isHardcoreDead(player);
            if (isDead)
            {
                handler.SendSysMessage("|cffFF0000Статус: ПОГИБ НАВСЕГДА|r");
            }
            else
            {
                handler.SendSysMessage("|cff00FF00Статус: ЖИВ|r");
            }
            
            // Окончательная смерть
            if (sHardcore->hardcoreMaxDeathLevel > 0)
            {
                if (player->GetLevel() > sHardcore->hardcoreMaxDeathLevel)
                {
                    handler.SendSysMessage("|cff00FF00Окончательная смерть: ОТКЛЮЧЕНА|r");
                }
                else
                {
                    std::string maxDeathMsg = "|cffFF8800Окончательная смерть до: " + 
                                             std::to_string(sHardcore->hardcoreMaxDeathLevel) + " ур.|r";
                    handler.SendSysMessage(maxDeathMsg.c_str());
                }
            }
            
            handler.SendSysMessage(" ");
            handler.SendSysMessage("|cffFFFF00Команды:|r");
            handler.SendSysMessage("|cffFFFF00• .hardcore status|r - Полный статус");
            handler.SendSysMessage("|cffFFFF00• .hardcore info|r - Информация о режиме");
            handler.SendSysMessage("|cffFFFF00• .hardcore top|r - Таблица лидеров");
        }
        
        handler.SendSysMessage(" ");
        handler.SendSysMessage("|cffFFD700==========================================|r");
        handler.SendSysMessage(" ");
    }
};

void AddSC_item_hardcore_scroll()
{
    new item_hardcore_scroll();
}
