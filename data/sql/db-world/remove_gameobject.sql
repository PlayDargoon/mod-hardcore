-- =====================================================
-- УДАЛЕНИЕ СТАРОГО GAMEOBJECT ХАРДКОР
-- =====================================================

-- Удаляем шаблон GameObject
DELETE FROM `gameobject_template` WHERE `entry` = 300000;

-- Удаляем все спавны GameObject в мире
DELETE FROM `gameobject` WHERE `id` = 300000;

-- Очищаем кэш
DELETE FROM `gameobject_template_addon` WHERE `entry` = 300000;
