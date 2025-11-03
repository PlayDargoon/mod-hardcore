-- =====================================================
-- ХАРДКОР МЕНЮ: Спелл 38057
-- =====================================================

-- Обновляем спелл 38057 для меню хардкора
DELETE FROM `spell_script_names` WHERE `spell_id` = 38057 AND `ScriptName` = 'spell_hardcore_menu';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (38057, 'spell_hardcore_menu');

-- Добавляем описание спелла (опционально, если хотите изменить текст)
-- UPDATE `spell_dbc` SET `SpellName_Lang_enUS` = 'Hardcore Menu' WHERE `Id` = 38057;
