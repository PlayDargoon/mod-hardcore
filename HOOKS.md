# Система хуков mod-hardcore

## Описание

Модуль `mod-hardcore` предоставляет callback-систему, которая позволяет другим модулям подписываться на события хардкор-режима без необходимости модификации исходного кода.

## Доступные хуки

### 1. OnActivate - Активация хардкор-режима
**Сигнатура:** `void(Player* player)`

Вызывается при активации хардкор-режима игроком через GameObject.

**Пример использования:**
```cpp
sHardcore->RegisterOnActivate([](Player* player) {
    // Ваша логика при активации хардкора
    ChatHandler(player->GetSession()).SendSysMessage("Хардкор активирован!");
});
```

---

### 2. OnDeath - Смерть хардкор-игрока
**Сигнатура:** `void(Player* player)`

Вызывается при окончательной смерти хардкор-игрока (PvP или PvE).

**Пример использования:**
```cpp
sHardcore->RegisterOnDeath([](Player* player) {
    // Ваша логика при смерти
    LOG_INFO("module", "Hardcore player {} died at level {}", player->GetName(), player->GetLevel());
});
```

---

### 3. OnLevelUp - Повышение уровня
**Сигнатура:** `void(Player* player, uint8 level)`

Вызывается при повышении уровня хардкор-игрока.

**Пример использования:**
```cpp
sHardcore->RegisterOnLevelUp([](Player* player, uint8 level) {
    // Ваша логика при повышении уровня
    if (level == 10)
    {
        ChatHandler(player->GetSession()).SendSysMessage("Поздравляем с 10 уровнем!");
    }
});
```

---

### 4. OnDungeonEnter - Вход в подземелье
**Сигнатура:** `bool(Player* player, uint32 mapId)`

Вызывается при входе хардкор-игрока в подземелье. Возвращает `false` для блокировки входа.

**Пример использования:**
```cpp
sHardcore->RegisterOnDungeonEnter([](Player* player, uint32 mapId) -> bool {
    // Проверка на кастомные условия
    if (mapId == 533) // Наксрамас
    {
        if (player->GetLevel() < 80)
        {
            ChatHandler(player->GetSession()).SendSysMessage("Вход в Наксрамас доступен с 80 уровня!");
            return false; // Блокируем вход
        }
    }
    return true; // Разрешаем вход
});
```

---

### 5. OnReward - Получение награды
**Сигнатура:** `void(Player* player, uint32 rewardType, uint32 value)`

Вызывается при получении награды хардкор-игроком (титул, таланты, достижение, предмет).

**Параметры:**
- `rewardType` - тип события (всегда `HARDCORE_EVENT_REWARD`)
- `value` - ID титула/достижения/предмета или количество талантов

**Пример использования:**
```cpp
sHardcore->RegisterOnReward([](Player* player, uint32 rewardType, uint32 value) {
    // Дополнительная награда
    LOG_INFO("module", "Player {} received hardcore reward: {}", player->GetName(), value);
});
```

---

### 6. OnComplete - Завершение хардкор-режима
**Сигнатура:** `void(Player* player)`

Вызывается при успешном завершении хардкор-режима (достижение уровня отключения).

**Пример использования:**
```cpp
sHardcore->RegisterOnComplete([](Player* player) {
    // Эпичная награда за завершение
    player->ModifyMoney(1000000); // 100 золота
    ChatHandler(player->GetSession()).SendSysMessage("Награда за завершение хардкора!");
});
```

---

### 7. OnMapChanged - Смена карты (альтернативный хук)
**Сигнатура:** `bool(Player* player, uint32 mapId)`

Вызывается при смене карты хардкор-игроком. Возвращает `false` для блокировки перехода.

**Пример использования:**
```cpp
sHardcore->RegisterOnMapChanged([](Player* player, uint32 mapId) -> bool {
    // Кастомная логика смены карты
    if (mapId == 571) // Нордскол
    {
        if (player->GetLevel() < 68)
        {
            ChatHandler(player->GetSession()).SendSysMessage("Нордскол доступен с 68 уровня!");
            return false;
        }
    }
    return true;
});
```

---

### 8. OnMailSend - Отправка почты
**Сигнатура:** `bool(Player* player)`

Вызывается при попытке отправки почты хардкор-игроком. Возвращает `false` для блокировки.

**Пример использования:**
```cpp
sHardcore->RegisterOnMailSend([](Player* player) -> bool {
    // Кастомная логика блокировки почты
    if (player->GetLevel() < 20)
    {
        ChatHandler(player->GetSession()).SendSysMessage("Почта доступна с 20 уровня!");
        return false;
    }
    return true;
});
```

---

### 9. OnMailReceive - Получение почты
**Сигнатура:** `bool(Player* player)`

Вызывается при попытке получения почты хардкор-игроком. Возвращает `false` для блокировки.

**Пример использования:**
```cpp
sHardcore->RegisterOnMailReceive([](Player* player) -> bool {
    // Логика получения почты
    return true; // Разрешаем получение
});
```

---

### 10. OnBGQueue - Вход в очередь поля боя
**Сигнатура:** `bool(Player* player, uint32 bgTypeId)`

Вызывается при попытке входа в очередь поля боя. Возвращает `false` для блокировки.

**Пример использования:**
```cpp
sHardcore->RegisterOnBGQueue([](Player* player, uint32 bgTypeId) -> bool {
    // Разрешаем только WSG
    if (bgTypeId == BATTLEGROUND_WS)
    {
        return true;
    }
    ChatHandler(player->GetSession()).SendSysMessage("Доступна только Ущелье Песни Войны!");
    return false;
});
```

---

### 11. OnArenaQueue - Вход в очередь арены
**Сигнатура:** `bool(Player* player, uint32 arenaType)`

Вызывается при попытке входа в очередь арены. Возвращает `false` для блокировки.

**Пример использования:**
```cpp
sHardcore->RegisterOnArenaQueue([](Player* player, uint32 arenaType) -> bool {
    // Полная блокировка арены
    ChatHandler(player->GetSession()).SendSysMessage("Арена недоступна в хардкор-режиме!");
    return false;
});
```

---

## Интеграция в ваш модуль

### Шаг 1: Подключение заголовка
```cpp
#include "../mod-hardcore/src/Hardcore.h"
```

### Шаг 2: Регистрация хуков в WorldScript
```cpp
class YourModule_WorldScript : public WorldScript
{
public:
    YourModule_WorldScript() : WorldScript("YourModule_WorldScript") {}

    void OnStartup() override
    {
        // Регистрируем все нужные хуки
        sHardcore->RegisterOnActivate([](Player* player) {
            // Ваша логика
        });
        
        sHardcore->RegisterOnDeath([](Player* player) {
            // Ваша логика
        });
        
        // ... остальные хуки
    }
};
```

### Шаг 3: Добавление в loader
```cpp
void AddSC_YourModule()
{
    new YourModule_WorldScript();
}
```

---

## Примеры реальных кейсов

### Пример 1: Интеграция с системой ботов (mod-playerbots)
```cpp
sHardcore->RegisterOnActivate([](Player* player) {
    // Уведомляем систему ботов о хардкор-игроке
    if (player->GetPlayerbotAI())
    {
        player->GetPlayerbotAI()->SetHardcoreMode(true);
    }
});

sHardcore->RegisterOnDeath([](Player* player) {
    // Удаляем бота при смерти владельца
    if (player->GetPlayerbotMgr())
    {
        player->GetPlayerbotMgr()->RemoveAllBots();
    }
});
```

### Пример 2: Система достижений
```cpp
sHardcore->RegisterOnLevelUp([](Player* player, uint8 level) {
    // Выдаём кастомные достижения
    if (level == 80)
    {
        player->CompletedAchievement(sAchievementStore.LookupEntry(12345));
    }
});

sHardcore->RegisterOnComplete([](Player* player) {
    // Эпичное достижение за завершение
    player->CompletedAchievement(sAchievementStore.LookupEntry(99999));
});
```

### Пример 3: Экономическая система
```cpp
sHardcore->RegisterOnDungeonEnter([](Player* player, uint32 mapId) -> bool {
    // Платный вход в рейды
    if (sMapStore.LookupEntry(mapId)->IsRaid())
    {
        uint32 cost = 100 * GOLD;
        if (player->GetMoney() < cost)
        {
            ChatHandler(player->GetSession()).PSendSysMessage("Вход в рейд стоит %u золота!", cost / GOLD);
            return false;
        }
        player->ModifyMoney(-int32(cost));
    }
    return true;
});
```

---

## Технические детали

### Порядок вызова хуков
Хуки вызываются в порядке регистрации. Если несколько модулей регистрируют один и тот же хук, они будут вызваны последовательно.

### Возвращаемые значения (bool hooks)
Для хуков, возвращающих `bool`:
- Если хотя бы один callback вернёт `false`, конечный результат будет `false`
- Это позволяет любому модулю блокировать действие

### Проверка доступности
Всегда проверяйте, что `sHardcore` доступен:
```cpp
if (sHardcore && sHardcore->enabled())
{
    sHardcore->RegisterOnActivate(/* ... */);
}
```

---

## FAQ

**Q: Когда регистрировать хуки?**  
A: В методе `OnStartup()` вашего `WorldScript` или в `OnConfigLoad()`.

**Q: Можно ли отменить регистрацию хука?**  
A: В текущей реализации нет, хуки регистрируются на весь сеанс сервера.

**Q: Влияет ли порядок загрузки модулей?**  
A: Да, убедитесь, что `mod-hardcore` загружается раньше вашего модуля.

**Q: Можно ли модифицировать данные игрока в хуке?**  
A: Да, вы получаете полный доступ к `Player*` объекту.

---

## Поддержка

- GitHub: https://github.com/PlayDargoon/mod-hardcore
- Версия системы хуков: 1.2.0
- Совместимость: AzerothCore 3.3.5a (mod-playerbots fork)
