-- =====================================================
-- ХАРДКОР ПРЕДМЕТ: Свиток испытания
-- =====================================================

DELETE FROM `item_template` WHERE `entry` = 60000;
INSERT INTO `item_template` (`entry`, `class`, `subclass`, `name`, `displayid`, `Quality`, `Flags`, `FlagsExtra`, 
`BuyCount`, `BuyPrice`, `SellPrice`, `InventoryType`, `AllowableClass`, `AllowableRace`, `ItemLevel`, `RequiredLevel`, 
`maxcount`, `stackable`, `ContainerSlots`, `StatsCount`, `spellid_1`, `spellcategory_1`, `spellcooldown_1`, 
`spellcharges_1`, `bonding`, `description`, `PageText`, `LanguageID`, `PageMaterial`, `startquest`, `lockid`, 
`Material`, `sheath`, `RandomProperty`, `RandomSuffix`, `block`, `itemset`, `MaxDurability`, `area`, `Map`, 
`BagFamily`, `TotemCategory`, `socketColor_1`, `socketContent_1`, `socketColor_2`, `socketContent_2`, 
`socketColor_3`, `socketContent_3`, `socketBonus`, `GemProperties`, `RequiredDisenchantSkill`, 
`ArmorDamageModifier`, `duration`, `ItemLimitCategory`, `HolidayId`, `ScriptName`, `DisenchantID`, 
`FoodType`, `minMoneyLoot`, `maxMoneyLoot`, `flagsCustom`, `VerifiedBuild`) 
VALUES 
(60000, 15, 0, 'Свиток испытания «Хардкор»', 2621, 4, 64, 0, 
1, 0, 0, 0, -1, -1, 1, 1, 
1, 1, 0, 0, 0, 0, -1, 
-1, 1, 'Древний свиток с описанием легендарного испытания. Использование необратимо.', 0, 0, 0, 0, 0, 
-1, 0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, -1, 
0, 0, 0, 0, 'item_hardcore_scroll', 0, 
0, 0, 0, 0, 12340);

-- ПРИМЕЧАНИЕ: Предмет выдается автоматически при логине персонажа 1-го уровня
-- через скрипт OnLogin в Hardcore.cpp
-- Если нужна ручная выдача, используйте:
-- .additem 60000 1
