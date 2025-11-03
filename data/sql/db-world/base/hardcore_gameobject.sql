-- --------------------------------------------------------
-- Hardcore Mode GameObject
-- --------------------------------------------------------

DELETE FROM `gameobject_template` WHERE `entry` = 300001;
INSERT INTO `gameobject_template` (`entry`, `type`, `displayId`, `name`, `IconName`, `castBarCaption`, `unk1`, `size`, `Data0`, `Data1`, `Data2`, `Data3`, `Data4`, `Data5`, `Data6`, `Data7`, `Data8`, `Data9`, `Data10`, `Data11`, `Data12`, `Data13`, `Data14`, `Data15`, `Data16`, `Data17`, `Data18`, `Data19`, `Data20`, `Data21`, `Data22`, `Data23`, `AIName`, `ScriptName`, `VerifiedBuild`) VALUES
(300001, 10, 6658, 'Активация Хардкор', 'interact', '', '', 1.5, 0, 0, 0, 3000, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '', 'gobject_hardcore', 12340);

-- Размещение объектов на всех кладбищах
DELETE FROM `gameobject` WHERE `id` = 300001;

-- Альянс
INSERT INTO `gameobject` (`guid`, `id`, `map`, `spawnMask`, `phaseMask`, `position_x`, `position_y`, `position_z`, `orientation`, `rotation0`, `rotation1`, `rotation2`, `rotation3`, `spawntimesecs`, `animprogress`, `state`) VALUES
-- Нортшир (Эльвинский лес)
(5300001, 300001, 0, 1, 1, -8913.23, -137.61, 81.86, 3.14, 0, 0, 1, 0, 300, 0, 1),
-- Кхарнос (Дун Морог)
(5300002, 300001, 0, 1, 1, -6062.91, 331.59, 395.29, 0, 0, 0, 0, 1, 300, 0, 1),
-- Амман Вэйл (Азуремист Isles)
(5300003, 300001, 530, 1, 1, -4192.62, -12510.7, 44.52, 0, 0, 0, 0, 1, 300, 0, 1),
-- Даларан (Нордскол - нейтральное)
(5300004, 300001, 571, 1, 1, 5693.08, 612.97, 647.66, 0, 0, 0, 0, 1, 300, 0, 1);

-- Орда
INSERT INTO `gameobject` (`guid`, `id`, `map`, `spawnMask`, `phaseMask`, `position_x`, `position_y`, `position_z`, `orientation`, `rotation0`, `rotation1`, `rotation2`, `rotation3`, `spawntimesecs`, `animprogress`, `state`) VALUES
-- Долина Испытаний (Дуротар)
(5300005, 300001, 1, 1, 1, -618.52, -4251.23, 38.69, 0, 0, 0, 0, 1, 300, 0, 1),
-- Лагерь Наррач (Мулгор)
(5300006, 300001, 1, 1, 1, -2340.84, -346.06, -8.96, 0, 0, 0, 0, 1, 300, 0, 1),
-- Долина Теней (Тирисфаль)
(5300007, 300001, 0, 1, 1, 1859.45, 1567.72, 94.31, 0, 0, 0, 0, 1, 300, 0, 1),
-- Остров Зыбителей Солнца (Призрачные земли)
(5300008, 300001, 530, 1, 1, 10377.3, -6373.05, 35.97, 0, 0, 0, 0, 1, 300, 0, 1);
