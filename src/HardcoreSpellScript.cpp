/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>
 * Released under GNU AGPL v3 License
 */

#include "ScriptMgr.h"
#include "Player.h"
#include "ScriptedGossip.h"
#include "Chat.h"
#include "Hardcore.h"
#include "SpellScript.h"
#include "SpellMgr.h"
#include "ObjectAccessor.h"

// Forward declaration
void ShowHardcoreMenuToPlayer(Player* player);

// Спелл 38057 - Меню режима Хардкор
class spell_hardcore_menu : public SpellScript
{
    PrepareSpellScript(spell_hardcore_menu);

    void HandleDummy(SpellEffIndex /*effIndex*/)
    {
        if (Unit* caster = GetCaster())
        {
            if (Player* player = caster->ToPlayer())
            {
                ShowHardcoreMenuToPlayer(player);
            }
        }
    }

    void Register()
    {
        OnEffectHitTarget += SpellEffectFn(spell_hardcore_menu::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
    }
};

// Реализация функции показа меню
void ShowHardcoreMenuToPlayer(Player* player)
{
    ClearGossipMenuFor(player);
    
    bool isHardcore = sHardcore->isHardcorePlayer(player);
    uint32 declined = player->GetPlayerSetting("mod-hardcore", 10).value;
    
    // Заголовок
    AddGossipItemFor(player, GOSSIP_ICON_BATTLE, "|cffFFD700[РЕЖИМ ХАРДКОР]|r", GOSSIP_SENDER_MAIN, 0);
    AddGossipItemFor(player, GOSSIP_ICON_TALK, " ", GOSSIP_SENDER_MAIN, 0);
    
    if (isHardcore)
    {
        // Игрок уже в режиме хардкор
        AddGossipItemFor(player, GOSSIP_ICON_DOT, "|cff00FF00Статус: АКТИВЕН|r", GOSSIP_SENDER_MAIN, 1);
        AddGossipItemFor(player, GOSSIP_ICON_TALK, " ", GOSSIP_SENDER_MAIN, 0);
        AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Показать мой статус", GOSSIP_SENDER_MAIN, 2);
        AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Таблица лидеров", GOSSIP_SENDER_MAIN, 3);
        AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Информация о режиме", GOSSIP_SENDER_MAIN, 4);
    }
    else if (declined != 0)
    {
        // Игрок отказался от режима
        AddGossipItemFor(player, GOSSIP_ICON_DOT, "|cffFF0000Вы отказались от испытания|r", GOSSIP_SENDER_MAIN, 0);
        AddGossipItemFor(player, GOSSIP_ICON_TALK, " ", GOSSIP_SENDER_MAIN, 0);
        AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Информация о режиме", GOSSIP_SENDER_MAIN, 4);
    }
    else
    {
        // Игрок может активировать режим
        if (player->GetLevel() == 1 || (player->GetLevel() == 55 && player->getClass() == CLASS_DEATH_KNIGHT))
        {
            AddGossipItemFor(player, GOSSIP_ICON_DOT, "|cffFFFF00Режим доступен для активации!|r", GOSSIP_SENDER_MAIN, 0);
            AddGossipItemFor(player, GOSSIP_ICON_TALK, " ", GOSSIP_SENDER_MAIN, 0);
            AddGossipItemFor(player, GOSSIP_ICON_BATTLE, "|cff00FF00[НАЧАТЬ ИСПЫТАНИЕ]|r", GOSSIP_SENDER_MAIN, 5, "Вы уверены? После активации отменить нельзя!", 0, false);
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Информация о режиме", GOSSIP_SENDER_MAIN, 4);
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, " ", GOSSIP_SENDER_MAIN, 0);
            AddGossipItemFor(player, GOSSIP_ICON_TRAINER, "|cffFF0000Отказаться от испытания|r", GOSSIP_SENDER_MAIN, 6, "Вы точно хотите отказаться? Потом не сможете активировать!", 0, false);
        }
        else
        {
            AddGossipItemFor(player, GOSSIP_ICON_DOT, "|cffFF0000НЕДОСТУПНО: слишком высокий уровень|r", GOSSIP_SENDER_MAIN, 0);
            AddGossipItemFor(player, GOSSIP_ICON_TALK, "Режим можно активировать только на 1 уровне", GOSSIP_SENDER_MAIN, 0);
            AddGossipItemFor(player, GOSSIP_ICON_TALK, " ", GOSSIP_SENDER_MAIN, 0);
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Информация о режиме", GOSSIP_SENDER_MAIN, 4);
        }
    }
    
    SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, player->GetGUID());
}

// Обработчик меню
class hardcore_menu_gossip : public PlayerScript
{
public:
    hardcore_menu_gossip() : PlayerScript("hardcore_menu_gossip") { }

    void OnGossipSelect(Player* player, uint32 /*menu_id*/, uint32 /*sender*/, uint32 action)
    {
        ClearGossipMenuFor(player);
        ChatHandler handler(player->GetSession());
        
        switch (action)
        {
            case 0: // Пустой пункт
                CloseGossipMenuFor(player);
                break;
                
            case 1: // Статус активен (показать детали)
            case 2: // Показать статус
            {
                std::string status = "|cff00FF00АКТИВЕН|r";
                if (sHardcore->isHardcoreDead(player))
                {
                    status = "|cffFF0000МЕРТВ|r";
                }
                
                handler.SendSysMessage("|cffFFD700========================================|r");
                handler.SendSysMessage("|cffFFD700     ВАШ СТАТУС ХАРДКОР|r");
                handler.SendSysMessage("|cffFFD700========================================|r");
                std::string msg = "|cffFFFFFFСтатус: " + status;
                handler.SendSysMessage(msg.c_str());
                std::string lvlMsg = "|cffFFFFFFУровень: |cffFFFF00" + std::to_string(player->GetLevel()) + "|r";
                handler.SendSysMessage(lvlMsg.c_str());
                handler.SendSysMessage("|cffFF6347Смертей: 0 (одна смерть = конец)|r");
                handler.SendSysMessage("|cffFFD700========================================|r");
                CloseGossipMenuFor(player);
                break;
            }
            
            case 3: // Таблица лидеров
            {
                auto const& players = ObjectAccessor::GetPlayers();
                std::vector<std::pair<std::string, uint8>> hardcorePlayers;
                
                for (auto const& pair : players)
                {
                    if (Player* p = pair.second)
                    {
                        if (sHardcore->isHardcorePlayer(p) && !sHardcore->isHardcoreDead(p))
                        {
                            hardcorePlayers.push_back({p->GetName(), p->GetLevel()});
                        }
                    }
                }
                
                std::sort(hardcorePlayers.begin(), hardcorePlayers.end(),
                    [](const std::pair<std::string, uint8>& a, const std::pair<std::string, uint8>& b) {
                        return a.second > b.second;
                    });
                
                handler.SendSysMessage("|cffFFD700========================================|r");
                handler.SendSysMessage("|cffFFD700     ТАБЛИЦА ЛИДЕРОВ ХАРДКОР|r");
                handler.SendSysMessage("|cffFFD700========================================|r");
                
                int count = 0;
                for (const auto& p : hardcorePlayers)
                {
                    if (++count > 10) break;
                    
                    std::string line = "|cffFFFF00" + std::to_string(count) + ". |cffFFFFFF" + 
                                      p.first + " |cff00FF00[" + std::to_string(p.second) + "]|r";
                    handler.SendSysMessage(line.c_str());
                }
                
                if (hardcorePlayers.empty())
                {
                    handler.SendSysMessage("|cffFF8800Пока нет игроков в режиме хардкор онлайн.|r");
                }
                
                handler.SendSysMessage("|cffFFD700========================================|r");
                CloseGossipMenuFor(player);
                break;
            }
            
            case 4: // Информация о режиме
            {
                handler.SendSysMessage("|cffFF0000==========================================|r");
                handler.SendSysMessage("|cffFFD700     РЕЖИМ ХАРДКОР - ПРАВИЛА|r");
                handler.SendSysMessage("|cffFF0000==========================================|r");
                handler.SendSysMessage(" ");
                handler.SendSysMessage("|cffFF6347 * ОДНА ЖИЗНЬ|r - одна смерть = удаление персонажа");
                handler.SendSysMessage("|cffFF6347 * БЕЗ ТОРГОВЛИ|r - запрещен аукцион и почта");
                handler.SendSysMessage("|cffFF6347 * БЕЗ ГРУППЫ|r - только соло игра");
                handler.SendSysMessage("|cffFF6347 * БЕЗ ГИЛЬДИИ|r - путь одиночки");
                handler.SendSysMessage(" ");
                handler.SendSysMessage("|cff00FF00Награды:|r");
                handler.SendSysMessage("|cffFFFF00 - Красная аура героя|r");
                handler.SendSysMessage("|cffFFFF00 - Глобальные анонсы достижений|r");
                handler.SendSysMessage("|cffFFFF00 - Место в таблице лидеров|r");
                handler.SendSysMessage("|cffFFFF00 - Слава и уважение|r");
                handler.SendSysMessage(" ");
                handler.SendSysMessage("|cffFF0000ВНИМАНИЕ: После активации отменить нельзя!|r");
                handler.SendSysMessage("|cffFF0000==========================================|r");
                CloseGossipMenuFor(player);
                break;
            }
            
            case 5: // Начать испытание
            {
                if (player->GetLevel() > 1 && !(player->GetLevel() == 55 && player->getClass() == CLASS_DEATH_KNIGHT))
                {
                    handler.SendSysMessage("|cffFF0000Слишком высокий уровень!|r");
                    CloseGossipMenuFor(player);
                    return;
                }
                
                if (sHardcore->isHardcorePlayer(player))
                {
                    handler.SendSysMessage("|cffFF0000Вы уже в режиме хардкор!|r");
                    CloseGossipMenuFor(player);
                    return;
                }
                
                // Активируем режим
                player->UpdatePlayerSetting("mod-hardcore", SETTING_HARDCORE, 1);
                player->UpdatePlayerSetting("mod-hardcore", 1, 0); // HARDCORE_DEAD = 0
                
                // Глобальное объявление
                std::string announcement = "|cffFF0000==========================================|r\n"
                                          "|cffFFD700   НОВЫЙ ГЕРОЙ ХАРДКОРА!|r\n"
                                          "|cffFF0000==========================================|r\n"
                                          "|cffFFFFFF" + std::string(player->GetName()) + " начал испытание ХАРДКОР!|r\n"
                                          "|cffFF8800Одна жизнь. Одна смерть. Одна судьба.|r\n"
                                          "|cffFFFF00==========================================|r";
                ChatHandler(nullptr).SendWorldText(announcement.c_str());
                
                // Оповещение на экране
                const std::string screenNotification = player->GetName() + " начал испытание ХАРДКОР!";
                auto const& players = ObjectAccessor::GetPlayers();
                for (auto const& pair : players)
                {
                    if (Player* onlinePlayer = pair.second)
                    {
                        if (WorldSession* session = onlinePlayer->GetSession())
                        {
                            session->SendAreaTriggerMessage(screenNotification);
                        }
                    }
                }
                
                handler.SendSysMessage("|cff00FF00Режим хардкор активирован! Удачи, герой!|r");
                CloseGossipMenuFor(player);
                break;
            }
            
            case 6: // Отказаться
            {
                player->UpdatePlayerSetting("mod-hardcore", 10, 1); // HARDCORE_DECLINED = 1
                handler.SendSysMessage("|cffFFFF00Вы отказались от испытания хардкор.|r");
                handler.SendSysMessage("|cffFF8800Предложение больше не будет появляться.|r");
                CloseGossipMenuFor(player);
                break;
            }
        }
    }
};

void AddSC_hardcore_spell_scripts()
{
    RegisterSpellScript(spell_hardcore_menu);
    new hardcore_menu_gossip();
}
