# Примеры конфигурации mod-hardcore

## 1. Классический хардкор (до конца)

```ini
# Режим никогда не отключается, смерть окончательна на всех уровнях
Hardcore.Enable = 1
Hardcore.DisableLevel = 0           # Никогда не отключать
Hardcore.MaxDeathLevel = 0          # Окончательная смерть всегда
Hardcore.DamageDealtModifier = 0.9  # -10% урона
Hardcore.DamageTakenModifier = 1.5  # +50% получаемого урона
```

**Для кого:** Экстремальные игроки, полное испытание до 80 уровня

---

## 2. Хардкор до 60 уровня

```ini
# После 60 уровня режим полностью отключается
Hardcore.Enable = 1
Hardcore.DisableLevel = 60          # Отключить на 60
Hardcore.MaxDeathLevel = 0          # Смерть окончательна до 60
Hardcore.MinDeathAnnounceLevel = 20 # Анонсы с 20 уровня
```

**Для кого:** Классический левелинг с хардкором, TBC/WotLK контент без ограничений

---

## 3. Мягкий хардкор до 80

```ini
# Окончательная смерть только до 60, после - режим остаётся, но можно воскрешаться
Hardcore.Enable = 1
Hardcore.DisableLevel = 80          # Отключить на 80
Hardcore.MaxDeathLevel = 60         # Смерть окончательна до 60
Hardcore.DamageDealtModifier = 0.95 # -5% урона (легче)
Hardcore.DamageTakenModifier = 1.25 # +25% урона (легче)
```

**Для кого:** Игроки, желающие получить хардкор-опыт на левелинге, но с более мягкими условиями на эндгейме

---

## 4. Хардкор с минимальными ограничениями

```ini
# Только смерть окончательна, остальные ограничения отключены
Hardcore.Enable = 1
Hardcore.DisableLevel = 80
Hardcore.MaxDeathLevel = 0

# Разрешаем всё
Hardcore.BlockAuction = 0           # Аукцион разрешён
Hardcore.BlockMail = 0              # Почта разрешена
Hardcore.BlockGuildBank = 0         # Банк гильдии разрешён
Hardcore.BlockBattleground = 0      # БГ разрешены
Hardcore.ForcePvE = 0               # PvP разрешён

# Урон нормальный
Hardcore.DamageDealtModifier = 1.0
Hardcore.DamageTakenModifier = 1.0

# Группы без ограничений
Hardcore.MaxLevelDifference = 80
```

**Для кого:** Только механика "одной жизни", без экономических/социальных ограничений

---

## 5. Максимально сложный режим

```ini
# Все ограничения + усиленные модификаторы
Hardcore.Enable = 1
Hardcore.DisableLevel = 0
Hardcore.MaxDeathLevel = 0

# Экстремальные модификаторы
Hardcore.DamageDealtModifier = 0.7  # -30% урона
Hardcore.DamageTakenModifier = 2.0  # +100% получаемого урона
Hardcore.MinDeathAnnounceLevel = 1  # Анонсы всех смертей

# Жесткие ограничения
Hardcore.BlockAuction = 1
Hardcore.BlockMail = 1
Hardcore.BlockGuildBank = 1
Hardcore.BlockBattleground = 1
Hardcore.BlockArena = 1
Hardcore.BlockDeathKnight = 1
Hardcore.ForcePvE = 1
Hardcore.MaxLevelDifference = 3     # Группы только ±3 уровня
Hardcore.DungeonCooldown = 48       # 48 часов на подземелья
```

**Для кого:** Настоящие мазохисты

---

## 6. Режим "Испытание новичка" (до 20 уровня)

```ini
# Хардкор только на начальных уровнях
Hardcore.Enable = 1
Hardcore.DisableLevel = 20          # Отключить на 20
Hardcore.MaxDeathLevel = 20
Hardcore.MinDeathAnnounceLevel = 10

# Облегченные модификаторы для новичков
Hardcore.DamageDealtModifier = 1.0  # Нормальный урон
Hardcore.DamageTakenModifier = 1.2  # +20% урона

# Разрешаем торговлю для помощи новичкам
Hardcore.BlockAuction = 0
Hardcore.BlockMail = 0
```

**Для кого:** Знакомство с механикой хардкора, обучение новых игроков

---

## 7. PvP хардкор

```ini
# Хардкор с акцентом на PvP
Hardcore.Enable = 1
Hardcore.DisableLevel = 80
Hardcore.MaxDeathLevel = 0
Hardcore.ForcePvE = 0               # PvP разрешён!
Hardcore.BlockBattleground = 0      # БГ разрешены
Hardcore.BlockArena = 0             # Арена разрешена

# Стандартные модификаторы
Hardcore.DamageDealtModifier = 0.9
Hardcore.DamageTakenModifier = 1.5

# Социальные ограничения остаются
Hardcore.BlockAuction = 1
Hardcore.MaxLevelDifference = 5
```

**Для кого:** Любители PvP с элементом риска

---

## 8. Групповой хардкор (для гильдий)

```ini
# Оптимизировано для прохождения в группах
Hardcore.Enable = 1
Hardcore.DisableLevel = 80
Hardcore.MaxDeathLevel = 0

# Облегченные условия для группового контента
Hardcore.DamageDealtModifier = 0.95
Hardcore.DamageTakenModifier = 1.3
Hardcore.MaxLevelDifference = 10    # Большая разница уровней
Hardcore.DungeonCooldown = 12       # Можно проходить дважды в день

# Социальные функции работают
Hardcore.BlockGuildBank = 0         # Банк гильдии доступен
Hardcore.BlockMail = 0              # Почта работает (для гильдии)
```

**Для кого:** Гильдии, проходящие хардкор вместе

---

## Награды и мотивация

### Пример наград за уровни:

```ini
# Титулы за достижения
Hardcore.TitleRewards = "20 143, 40 144, 60 145, 80 146"

# Дополнительные таланты
Hardcore.TalentRewards = "40 1, 60 2, 80 5"

# Предметы на 80 уровне
Hardcore.ItemRewards = "80 54811"
Hardcore.ItemRewardAmount = 1

# Достижения
Hardcore.AchievementReward = "20 1234, 60 5678, 80 9999"
```

---

## Тестовая конфигурация

```ini
# Для тестирования механик без риска
Hardcore.Enable = 1
Hardcore.DisableLevel = 10          # Быстрое отключение
Hardcore.MaxDeathLevel = 5          # Смерть только до 5
Hardcore.MinDeathAnnounceLevel = 1
Hardcore.DamageDealtModifier = 1.0  # Без модификаторов
Hardcore.DamageTakenModifier = 1.0
Hardcore.MaxLevelDifference = 80
```

---

## Примечания

1. После изменения конфигурации перезапустите worldserver
2. Изменения применяются только к новым персонажам
3. Существующие хардкор-персонажи сохраняют настройки активации
4. Для сброса настроек персонажа удалите записи из `character_settings`

```sql
DELETE FROM character_settings 
WHERE source = 'mod-hardcore' AND guid = [GUID];
```
