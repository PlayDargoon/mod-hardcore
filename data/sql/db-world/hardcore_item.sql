-- =====================================================
-- ХАРДКОР ПРЕДМЕТ: Свиток испытания
-- =====================================================

DELETE FROM `item_template` WHERE `entry` = 60000;
INSERT INTO `item_template` (`entry`, `class`, `subclass`, `name`, `displayid`, `Quality`, `Flags`, `FlagsExtra`, 
`BuyCount`, `BuyPrice`, `SellPrice`, `InventoryType`, `AllowableClass`, `AllowableRace`, `ItemLevel`, `RequiredLevel`, 
`maxcount`, `stackable`, `bonding`, `description`, `ScriptName`) 
VALUES 
(60000, 15, 0, 'Свиток испытания Хардкор', 2621, 4, 64, 0, 
1, 0, 0, 0, -1, -1, 1, 1, 
1, 1, 1, 'Древний свиток с описанием легендарного испытания. Использование необратимо.', 'item_hardcore_scroll');

-- ПРИМЕЧАНИЕ: Предмет выдается автоматически при логине персонажа 1-го уровня
-- через скрипт OnLogin в Hardcore.cpp
-- Если нужна ручная выдача, используйте:
-- .additem 60000 1
