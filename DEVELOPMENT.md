# Руководство разработчика mod-hardcore

Это руководство предназначено для разработчиков, которые хотят расширить или модифицировать mod-hardcore.

---

## Архитектура модуля

### Singleton Pattern

Модуль использует паттерн Singleton для доступа к конфигурации:

```cpp
// В Hardcore.h
class Hardcore {
public:
    static Hardcore* instance();
    // ...
};

#define sHardcore Hardcore::instance()

// Использование в любом месте:
if (sHardcore->isHardcorePlayer(player)) {
    // ...
}
```

### Script System

Модуль использует систему скриптов AzerothCore:

- **WorldScript** - глобальные события (загрузка конфига)
- **PlayerScript** - события игрока (логин, смерть, урон и т.д.)
- **AllCreatureScript** - взаимодействие с NPC
- **GroupScript** - события группы
- **BGScript** - события полей боя
- **GameObjectScript** - GameObject активации
- **CommandScript** - игровые команды

---

## Добавление новых функций

### 1. Добавление нового конфига

**Шаг 1:** Добавьте поле в `Hardcore.h`

```cpp
class Hardcore {
public:
    // ... существующие поля
    bool hardcoreNewFeature;  // Новая настройка
};
```

**Шаг 2:** Загрузите в `Hardcore_WorldScript::OnBeforeConfigLoad()`

```cpp
void OnBeforeConfigLoad(bool /*reload*/) override {
    // ... существующая загрузка
    sHardcore->hardcoreNewFeature = sConfigMgr->GetOption<bool>("Hardcore.NewFeature", false);
}
```

**Шаг 3:** Добавьте в `hardcore.conf.dist`

```ini
#    Hardcore.NewFeature
#        Описание: Включить новую функцию
#        По умолчанию: 0 (отключено)

Hardcore.NewFeature = 0
```

---

### 2. Добавление нового хука

**Пример:** Блокировка использования камня телепортации

```cpp
// В Hardcore_PlayerScript
void OnItemUse(Player* player, Item* item, SpellCastTargets const& /*targets*/, ObjectGuid /*castId*/) override
{
    if (!sHardcore->isHardcorePlayer(player))
        return;
    
    // 6948 = Hearthstone
    if (item->GetEntry() == 6948 && sHardcore->hardcoreBlockHearthstone)
    {
        ChatHandler(player->GetSession()).SendSysMessage("|cffFF0000[Хардкор] Камень телепортации заблокирован!|r");
        player->SendEquipError(EQUIP_ERR_CANT_DO_RIGHT_NOW, item, nullptr);
        return;
    }
}
```

---

### 3. Добавление новой команды

**Файл:** `HardcoreCommandScript.cpp`

```cpp
// Добавьте в ChatCommandTable
static ChatCommandTable hardcoreCommandTable =
{
    { "status",  HandleHardcoreStatusCommand,  SEC_PLAYER, Console::No },
    { "info",    HandleHardcoreInfoCommand,    SEC_PLAYER, Console::No },
    { "toggle",  HandleHardcoreToggleCommand,  SEC_PLAYER, Console::No }, // НОВОЕ
};

// Реализация
static bool HandleHardcoreToggleCommand(ChatHandler* handler)
{
    Player* player = handler->GetSession()->GetPlayer();
    if (!player)
        return false;

    // Логика команды
    bool isHardcore = sHardcore->isHardcorePlayer(player);
    
    if (isHardcore)
    {
        handler->SendSysMessage("Отключение режима невозможно после активации!");
        return false;
    }
    
    // ... код активации
    
    return true;
}
```

---

### 4. Добавление награды

**Пример:** Награда заклинаниями

**Шаг 1:** Добавьте map в `Hardcore.h`

```cpp
std::unordered_map<uint8, uint32> hardcoreSpellRewards;
```

**Шаг 2:** Загрузите в конфиге

```cpp
LoadStringToMap(sHardcore->hardcoreSpellRewards, 
    sConfigMgr->GetOption<std::string>("Hardcore.SpellRewards", ""));
```

**Шаг 3:** Обработайте в `OnLevelChanged`

```cpp
if (sHardcore->hardcoreSpellRewards.find(level) != sHardcore->hardcoreSpellRewards.end())
{
    uint32 spellId = sHardcore->hardcoreSpellRewards[level];
    player->LearnSpell(spellId, false);
    ChatHandler(player->GetSession()).PSendSysMessage("|cffFFFF00[Хардкор]|r Изучено заклинание!");
}
```

---

## Отладка

### Логирование

Используйте `LOG_INFO` для отладки:

```cpp
LOG_INFO("module", "mod-hardcore: Player {} activated hardcore mode", player->GetName());
LOG_DEBUG("module", "mod-hardcore: Dungeon cooldown: {} seconds remaining", remainingTime);
LOG_ERROR("module", "mod-hardcore: Failed to load configuration");
```

### Проверка в игре

```cpp
// Для тестирования добавьте отладочные сообщения
ChatHandler(player->GetSession()).PSendSysMessage("DEBUG: isHardcore=%d, isDead=%d", 
    sHardcore->isHardcorePlayer(player), 
    sHardcore->isHardcoreDead(player));
```

---

## Работа с базой данных

### PlayerSettings

Модуль использует систему `PlayerSettings` для хранения данных:

```cpp
// Чтение
uint32 value = player->GetPlayerSetting("mod-hardcore", SETTING_KEY).value;

// Запись
player->UpdatePlayerSetting("mod-hardcore", SETTING_KEY, newValue);
```

**Преимущества:**
- Автоматическое сохранение
- Не требует создания таблиц
- Синхронизация с базой

### Новая таблица (если нужна)

Если нужна отдельная таблица для статистики:

**1. Создайте SQL файл:** `hardcore_statistics.sql`

```sql
CREATE TABLE IF NOT EXISTS `hardcore_statistics` (
  `guid` INT UNSIGNED NOT NULL,
  `deaths` INT UNSIGNED DEFAULT 0,
  `kills` INT UNSIGNED DEFAULT 0,
  `dungeons_completed` INT UNSIGNED DEFAULT 0,
  PRIMARY KEY (`guid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
```

**2. Используйте в коде:**

```cpp
// Запись
WorldDatabase.Execute("INSERT INTO hardcore_statistics (guid, deaths) VALUES ({}, 1) "
    "ON DUPLICATE KEY UPDATE deaths = deaths + 1", player->GetGUID().GetCounter());

// Чтение
QueryResult result = WorldDatabase.Query("SELECT deaths FROM hardcore_statistics WHERE guid = {}", 
    player->GetGUID().GetCounter());
if (result)
{
    uint32 deaths = (*result)[0].Get<uint32>();
    // ...
}
```

---

## Совместимость с другими модулями

### Проверка наличия другого модуля

Многие модули регистрируют свои синглтоны. Проверяйте их наличие:

```cpp
// Пример проверки mod-eluna
#ifdef ELUNA
    // Код для взаимодействия с Eluna
#endif
```

### События между модулями

Используйте `sScriptMgr` для вызова хуков:

```cpp
// Вызвать событие для других модулей
sScriptMgr->OnPlayerHardcoreActivated(player);
```

---

## Тестирование

### Unit Tests (TODO)

Создайте тесты в `src/test/`:

```cpp
#include "TestCase.h"

class HardcoreTest : public TestCase
{
public:
    void Execute() override
    {
        // Тест 1: Проверка активации
        ASSERT_TRUE(sHardcore->enabled());
        
        // Тест 2: Проверка конфига
        ASSERT_EQ(sHardcore->hardcoreDungeonCooldown, 24);
    }
};
```

### Интеграционные тесты

См. `TESTING.md` для полного списка сценариев.

---

## Производительность

### Оптимизация хуков

**Плохо:**
```cpp
void OnUpdate(Player* player, uint32 /*diff*/) override
{
    // Вызывается каждый тик - ОЧЕНЬ ДОРОГО!
    if (sHardcore->isHardcorePlayer(player))
    {
        // ...
    }
}
```

**Хорошо:**
```cpp
void OnLogin(Player* player) override
{
    // Вызывается один раз при входе
    if (sHardcore->isHardcorePlayer(player))
    {
        // Кэшируем результат или устанавливаем флаг
        player->SetFlag(PLAYER_FLAGS_EXTRA, PLAYER_FLAGS_EXTRA_HARDCORE);
    }
}
```

### Кэширование

Используйте локальные переменные для частых проверок:

```cpp
bool isHardcore = sHardcore->isHardcorePlayer(player);
if (isHardcore)
{
    // Много проверок isHardcore - используем кэш
}
```

---

## Стиль кода

### Именование

- **Классы:** `PascalCase` - `Hardcore_PlayerScript`
- **Методы:** `camelCase` - `isHardcorePlayer()`
- **Переменные:** `camelCase` - `hardcoreEnabled`
- **Константы:** `UPPER_CASE` - `HARDCORE_DEAD`

### Комментарии

```cpp
// Однострочный комментарий для простых пояснений

/*
 * Многострочный комментарий
 * для сложных объяснений
 */

/// Документационный комментарий для Doxygen
```

### Форматирование

- Отступы: 4 пробела
- Скобки: на новой строке для функций
- Пробелы вокруг операторов

```cpp
void MyFunction()
{
    if (condition)
    {
        DoSomething();
    }
}
```

---

## Частые ошибки

### 1. Забыли проверить nullptr

**Плохо:**
```cpp
Player* player = target->ToPlayer();
player->GetName(); // CRASH если target не игрок!
```

**Хорошо:**
```cpp
Player* player = target ? target->ToPlayer() : nullptr;
if (!player)
    return;

player->GetName(); // Безопасно
```

### 2. Неправильный порядок проверок

**Плохо:**
```cpp
if (player->GetLevel() > 10 && sHardcore->isHardcorePlayer(player))
```

**Хорошо:**
```cpp
// Сначала дешёвые проверки
if (!sHardcore->isHardcorePlayer(player))
    return;

if (player->GetLevel() <= 10)
    return;
```

### 3. Утечка памяти

**Плохо:**
```cpp
std::string* message = new std::string("Test");
// Забыли delete - утечка!
```

**Хорошо:**
```cpp
std::string message = "Test"; // Автоматическое управление памятью
```

---

## Полезные ссылки

- [AzerothCore Doxygen](https://www.azerothcore.org/pages/doxygen/)
- [ScriptMgr Hooks](https://github.com/azerothcore/azerothcore-wotlk/blob/master/src/server/game/Scripting/ScriptMgr.h)
- [Module Development](https://www.azerothcore.org/wiki/Create-a-Module)
- [C++ Best Practices](https://github.com/cpp-best-practices/cppbestpractices)

---

## Вклад в проект

Хотите улучшить модуль? Следуйте этим шагам:

1. Форкните репозиторий
2. Создайте ветку для новой функции
3. Напишите код + тесты
4. Обновите документацию (README, CHANGELOG)
5. Создайте Pull Request

**Чеклист перед PR:**
- [ ] Код скомпилирован без ошибок и предупреждений
- [ ] Все тесты проходят (см. TESTING.md)
- [ ] Добавлена документация
- [ ] Обновлен CHANGELOG.md
- [ ] Код соответствует стилю проекта
- [ ] Нет утечек памяти (проверено Valgrind/ASAN)

---

Спасибо за вклад в развитие mod-hardcore! 🎮
