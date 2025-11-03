# Быстрая установка mod-hardcore

## Для опытных администраторов

### Linux

```bash
# 1. Скопируйте модуль
cp -r mod-hardcore /путь/к/azerothcore/modules/

# 2. Соберите сервер
cd /путь/к/azerothcore/build
cmake ../ -DCMAKE_INSTALL_PREFIX=/путь/к/серверу
make -j$(nproc) && make install

# 3. Настройте конфиг
cp modules/mod-hardcore/conf/hardcore.conf.dist /путь/к/серверу/etc/modules/hardcore.conf
nano /путь/к/серверу/etc/modules/hardcore.conf

# 4. Импортируйте SQL
mysql -u root -p acore_world < modules/mod-hardcore/data/sql/db-world/base/hardcore_gameobject.sql

# 5. Запустите сервер
cd /путь/к/серверу/bin
./worldserver
```

### Windows

```powershell
# 1. Скопируйте mod-hardcore в C:\azerothcore-wotlk\modules\

# 2. Соберите через Visual Studio
cd C:\azerothcore-wotlk\build
cmake ../ -DCMAKE_INSTALL_PREFIX="C:\azerothcore-server"
# Откройте AzerothCore.sln → Build Solution

# 3. Скопируйте конфиг
copy C:\azerothcore-wotlk\modules\mod-hardcore\conf\hardcore.conf.dist ^
     C:\azerothcore-server\etc\modules\hardcore.conf

# 4. Импортируйте SQL через HeidiSQL или:
mysql -u root -p acore_world < C:\azerothcore-wotlk\modules\mod-hardcore\data\sql\db-world\base\hardcore_gameobject.sql

# 5. Запустите worldserver.exe
cd C:\azerothcore-server\bin
.\worldserver.exe
```

---

## Минимальная конфигурация для теста

Отредактируйте `hardcore.conf`:

```ini
Hardcore.Enable = 1
Hardcore.AuraSpellId = 36573
Hardcore.DisableLevel = 80
Hardcore.MaxDeathLevel = 0

# Остальные параметры оставьте по умолчанию
```

---

## Проверка работы

1. Создайте нового персонажа (1 уровень)
2. Найдите объект "Активация Хардкор" на кладбище
3. Взаимодействуйте с объектом
4. Должна появиться красная аура

**Координаты объектов:**
- **Нортшир** (Люди): `-8913, -137, 81`
- **Дун Морог** (Дворфы/Гномы): `-6062, 331, 395`
- **Дуротар** (Орки/Тролли): `-618, -4251, 38`
- **Мулгор** (Таурены): `-2340, -346, -8`

---

## Быстрое решение проблем

**Модуль не загружается?**
- Проверьте `hardcore.conf` в папке `etc/modules/`
- Убедитесь что `Hardcore.Enable = 1`

**Объект не виден?**
- Импортируйте SQL файл
- Персонаж должен быть 1 уровня
- ДК заблокированы по умолчанию

**Нет ауры?**
- Попробуйте другой Spell ID: `57940`, `23493`, `32474`
- Перелогиньтесь

**Ошибки компиляции?**
- Обновите AzerothCore до последней версии
- Проверьте наличие всех include файлов

---

## Полная документация

Смотрите [README.md](README.md) для детальной информации.
