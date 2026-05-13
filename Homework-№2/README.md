# Homework 2 Shooter

## Описание проекта

`Homework 2 Shooter` — это учебный шутер на `PhysX`, в котором реализованы:

- стрельба через `raycast` с визуализацией трассы пули;
- граната с физическим телом, таймером взрыва и проверкой преград;
- противник с капсульным коллайдером в живом состоянии;
- переход в `ragdoll` после смертельного урона;
- простейший AI, который либо ищет укрытие, либо пытается выйти из зоны обнаружения игрока.

## Управление

- `W`, `A`, `S`, `D` и мышь — управление камерой;
- `Space` — выстрел;
- `G` — бросок гранаты;
- `R` — сброс сцены.

## Структура кода

- [Homework-№2.cpp](/Users/vanadium/Repos/PhysX-Practice/Homework-№2/Homework-№2.cpp) — точка входа, создание камеры, движка и игрового объекта.
- [ShooterGame.h](/Users/vanadium/Repos/PhysX-Practice/Homework-№2/ShooterGame.h) / [ShooterGame.cpp](/Users/vanadium/Repos/PhysX-Practice/Homework-№2/ShooterGame.cpp) — основной игровой цикл, сцена, обработка ввода, урон, эффекты и HUD.
- [Enemy.h](/Users/vanadium/Repos/PhysX-Practice/Homework-№2/Enemy.h) / [Enemy.cpp](/Users/vanadium/Repos/PhysX-Practice/Homework-№2/Enemy.cpp) — противник, получение урона, живое состояние и `ragdoll`.
- [EnemyAIController.h](/Users/vanadium/Repos/PhysX-Practice/Homework-№2/EnemyAIController.h) / [EnemyAIController.cpp](/Users/vanadium/Repos/PhysX-Practice/Homework-№2/EnemyAIController.cpp) — логика обнаружения игрока, выбора укрытия и побега.
- [Grenade.h](/Users/vanadium/Repos/PhysX-Practice/Homework-№2/Grenade.h) / [Grenade.cpp](/Users/vanadium/Repos/PhysX-Practice/Homework-№2/Grenade.cpp) — физика и жизненный цикл гранаты.
- [GameConstants.h](/Users/vanadium/Repos/PhysX-Practice/Homework-№2/GameConstants.h) — централизованный набор игровых констант.

## Принципы организации

- Логика разделена по ответственности: игра, противник, AI, граната и константы вынесены в отдельные классы.
- Все игровые константы сгруппированы в `GameConstants` по подсистемам.
- Документация на русском добавлена прямо в заголовочные файлы, чтобы описание методов и констант было видно рядом с кодом.

## AI противника

Текущая версия AI работает так:

1. Противник проверяет, находится ли игрок в радиусе обнаружения.
2. Если игрок попал в радиус, AI пускает `raycast` между врагом и игроком.
3. Если луч не встречает препятствие, игрок считается замеченным.
4. После этого AI пытается выбрать безопасную точку укрытия.
5. Если точка укрытия найдена, строится маршрут к ней.
6. Если безопасной точки нет, выбирается точка побега.

## Замечание по документации

Подробные русские комментарии добавлены для игровых классов и констант проекта. `PhysicsEngine` я сознательно не документировал и не менял, потому что ранее мы договорились не трогать этот класс.
