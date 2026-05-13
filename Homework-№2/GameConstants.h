#pragma once

#include <array>
#include <cstddef>

#include "PxPhysicsAPI.h"

/// Набор физических параметров материала, используемых при создании `PxMaterial`.
struct MaterialSettings {
	/// Коэффициент статического трения: определяет, насколько трудно сдвинуть тело с места.
	float staticFriction;

	/// Коэффициент динамического трения: влияет на скольжение тела после начала движения.
	float dynamicFriction;

	/// Коэффициент упругости: определяет, насколько сильно тело отскакивает после удара.
	float restitution;
};

/// Геометрическое описание одного коробчатого препятствия на арене.
struct ArenaBoxDefinition {
	/// Размеры коробки по осям `X`, `Y`, `Z`.
	physx::PxVec3 size;

	/// Положение центра коробки в мировых координатах.
	physx::PxVec3 position;

	/// Поворот коробки относительно мировых осей.
	physx::PxQuat rotation;
};

/// Централизованное хранилище всех игровых констант проекта.
///
/// Константы сгруппированы по подсистемам, чтобы было удобно менять
/// баланс, размеры объектов, поведение AI и визуальные параметры из одного места.
class GameConstants final {
public:
	GameConstants() = delete;

	/// Переводит угол из градусов в радианы.
	///
	/// Используется там, где значения удобно хранить в градусах,
	/// а PhysX ожидает их в радианах.
	static float DegreesToRadians(float degrees) {
		return degrees * physx::PxPi / 180.0f;
	}

	/// Константы, связанные с приложением, камерой и параметрами рендера окна.
	struct AppConfig {
		/// Заголовок окна приложения.
		inline static constexpr const char* WindowTitle = "Homework 2 Shooter";

		/// Начальная позиция камеры в мире.
		inline static const physx::PxVec3 CameraPosition = physx::PxVec3(-12.0f, 8.0f, 18.0f);

		/// Начальное направление взгляда камеры.
		inline static const physx::PxVec3 CameraDirection = physx::PxVec3(0.52f, -0.22f, -0.82f);

		/// Скорость перемещения камеры в демо-оболочке.
		inline static constexpr float CameraSpeed = 0.35f;

		/// Ближняя плоскость отсечения для рендера сцены.
		inline static constexpr float RenderNearClip = 0.1f;

		/// Дальняя плоскость отсечения для рендера сцены.
		inline static constexpr float RenderFarClip = 300.0f;
	};

	/// Готовые профили материалов для разных типов поверхностей в игре.
	struct MaterialConfig {
		/// Материал пола: высокий уровень трения и почти отсутствующий отскок.
		inline static constexpr MaterialSettings Floor = { 0.90f, 0.80f, 0.02f };

		/// Материал укрытий и стен: умеренное трение и слабый отскок.
		inline static constexpr MaterialSettings Obstacle = { 0.65f, 0.55f, 0.08f };

		/// Материал живого противника и его ragdoll-частей.
		inline static constexpr MaterialSettings Enemy = { 0.65f, 0.55f, 0.05f };

		/// Материал гранаты: пониженное трение и заметный отскок.
		inline static constexpr MaterialSettings Grenade = { 0.45f, 0.35f, 0.65f };
	};

	/// Константы, описывающие живого противника и его ragdoll.
	struct EnemyConfig {
		/// Радиус капсулы живого противника.
		inline static constexpr float Radius = 0.45f;

		/// Высота цилиндрической части живой капсулы без учёта полусфер.
		inline static constexpr float Height = 1.40f;

		/// Масса живого противника в килограммах.
		inline static constexpr float Mass = 85.0f;

		/// Максимальный запас здоровья противника.
		inline static constexpr float MaxHealth = 100.0f;

		/// Высота центра живого противника над полом в состоянии покоя.
		inline static constexpr float StandingHeight = 1.15f;

		/// Стартовая позиция противника на арене.
		inline static const physx::PxVec3 SpawnPosition = physx::PxVec3(12.0f, StandingHeight, 1.0f);

		/// Количество физических частей ragdoll.
		inline static constexpr std::size_t RagdollPartCount = 6;

		/// Радиус сферической головы ragdoll.
		inline static constexpr float HeadRadius = 0.22f;

		/// Полуразмеры тела ragdoll по осям `X`, `Y`, `Z`.
		inline static const physx::PxVec3 TorsoSize = physx::PxVec3(0.55f, 0.80f, 0.32f);

		/// Радиус капсул, используемых для рук ragdoll.
		inline static constexpr float ArmRadius = 0.11f;

		/// Длина цилиндрической части капсул рук.
		inline static constexpr float ArmLength = 0.70f;

		/// Радиус капсул, используемых для ног ragdoll.
		inline static constexpr float LegRadius = 0.14f;

		/// Длина цилиндрической части капсул ног.
		inline static constexpr float LegLength = 0.90f;

		/// Масса головы ragdoll.
		inline static constexpr float HeadMass = 5.0f;

		/// Масса туловища ragdoll.
		inline static constexpr float TorsoMass = 39.0f;

		/// Масса одной руки ragdoll.
		inline static constexpr float ArmMass = 6.0f;

		/// Масса одной ноги ragdoll.
		inline static constexpr float LegMass = 14.5f;

		/// Смещение головы относительно центра капсулы в момент активации ragdoll.
		inline static const physx::PxVec3 HeadOffset = physx::PxVec3(0.0f, 0.78f, 0.0f);

		/// Смещение левой руки относительно центра капсулы.
		inline static const physx::PxVec3 LeftArmOffset = physx::PxVec3(-0.62f, 0.22f, 0.0f);

		/// Смещение правой руки относительно центра капсулы.
		inline static const physx::PxVec3 RightArmOffset = physx::PxVec3(0.62f, 0.22f, 0.0f);

		/// Смещение левой ноги относительно центра капсулы.
		inline static const physx::PxVec3 LeftLegOffset = physx::PxVec3(-0.20f, -0.95f, 0.0f);

		/// Смещение правой ноги относительно центра капсулы.
		inline static const physx::PxVec3 RightLegOffset = physx::PxVec3(0.20f, -0.95f, 0.0f);

		/// Мировое смещение точки соединения шеи с туловищем.
		inline static const physx::PxVec3 NeckAnchorOffset = physx::PxVec3(0.0f, 0.54f, 0.0f);

		/// Мировое смещение точки соединения левого плеча с туловищем.
		inline static const physx::PxVec3 LeftShoulderAnchorOffset = physx::PxVec3(-0.28f, 0.22f, 0.0f);

		/// Мировое смещение точки соединения правого плеча с туловищем.
		inline static const physx::PxVec3 RightShoulderAnchorOffset = physx::PxVec3(0.28f, 0.22f, 0.0f);

		/// Мировое смещение точки соединения левого бедра с туловищем.
		inline static const physx::PxVec3 LeftHipAnchorOffset = physx::PxVec3(-0.20f, -0.46f, 0.0f);

		/// Мировое смещение точки соединения правого бедра с туловищем.
		inline static const physx::PxVec3 RightHipAnchorOffset = physx::PxVec3(0.20f, -0.46f, 0.0f);

		/// Линейное демпфирование живого противника.
		inline static constexpr float LiveLinearDamping = 0.15f;

		/// Угловое демпфирование живого противника.
		inline static constexpr float LiveAngularDamping = 0.2f;

		/// Линейное демпфирование частей ragdoll.
		inline static constexpr float RagdollLinearDamping = 0.08f;

		/// Угловое демпфирование частей ragdoll.
		inline static constexpr float RagdollAngularDamping = 0.18f;

		/// Минимальный порог длины вектора, ниже которого направление считается нулевым.
		inline static constexpr float DirectionEpsilon = 1e-4f;

		/// Количество позиционных итераций solver для живой капсулы.
		inline static constexpr physx::PxU32 LiveSolverPositionIterations = 8;

		/// Количество скоростных итераций solver для живой капсулы.
		inline static constexpr physx::PxU32 LiveSolverVelocityIterations = 4;

		/// Количество позиционных итераций solver для частей ragdoll.
		inline static constexpr physx::PxU32 RagdollSolverPositionIterations = 12;

		/// Количество скоростных итераций solver для частей ragdoll.
		inline static constexpr physx::PxU32 RagdollSolverVelocityIterations = 4;

		/// Нижний предел кручения шеи в градусах.
		inline static constexpr float NeckLowerTwistDegrees = -30.0f;

		/// Верхний предел кручения шеи в градусах.
		inline static constexpr float NeckUpperTwistDegrees = 30.0f;

		/// Первый угол конуса для шеи в градусах.
		inline static constexpr float NeckSwing1Degrees = 25.0f;

		/// Второй угол конуса для шеи в градусах.
		inline static constexpr float NeckSwing2Degrees = 20.0f;

		/// Нижний предел кручения плечевого сустава в градусах.
		inline static constexpr float ShoulderLowerTwistDegrees = -35.0f;

		/// Верхний предел кручения плечевого сустава в градусах.
		inline static constexpr float ShoulderUpperTwistDegrees = 35.0f;

		/// Первый угол конуса для плечевого сустава в градусах.
		inline static constexpr float ShoulderSwing1Degrees = 65.0f;

		/// Второй угол конуса для плечевого сустава в градусах.
		inline static constexpr float ShoulderSwing2Degrees = 45.0f;

		/// Нижний предел кручения тазобедренного сустава в градусах.
		inline static constexpr float HipLowerTwistDegrees = -20.0f;

		/// Верхний предел кручения тазобедренного сустава в градусах.
		inline static constexpr float HipUpperTwistDegrees = 20.0f;

		/// Первый угол конуса для тазобедренного сустава в градусах.
		inline static constexpr float HipSwing1Degrees = 40.0f;

		/// Второй угол конуса для тазобедренного сустава в градусах.
		inline static constexpr float HipSwing2Degrees = 25.0f;
	};

	/// Константы, описывающие физические параметры гранаты.
	struct GrenadeConfig {
		/// Радиус сферы гранаты.
		inline static constexpr float Radius = 0.28f;

		/// Масса гранаты.
		inline static constexpr float Mass = 2.2f;

		/// Линейное демпфирование гранаты в полёте и после отскоков.
		inline static constexpr float LinearDamping = 0.10f;

		/// Угловое демпфирование гранаты при вращении.
		inline static constexpr float AngularDamping = 0.35f;
	};

	/// Константы арены, препятствий и точек укрытия.
	struct ArenaConfig {
		/// Отступ точки укрытия от поверхности obstacle.
		inline static constexpr float CoverPointOffset = 0.80f;

		/// Количество точек укрытия, создаваемых на одном obstacle.
		inline static constexpr int CoverPointCountPerObstacle = 2;

		/// Количество граничных коробок, образующих пределы арены.
		inline static constexpr int BoundaryObstacleCount = 4;

		/// Количество внутренних obstacle, которые могут использоваться как укрытия.
		inline static constexpr int CoverObstacleCount = 5;

		/// Минимальная координата по `X` и `Z`, в пределах которой AI может выбирать цели.
		inline static constexpr float ClampMin = -16.0f;

		/// Максимальная координата по `X` и `Z`, в пределах которой AI может выбирать цели.
		inline static constexpr float ClampMax = 16.0f;

		/// Размеры пола арены.
		inline static const physx::PxVec3 FloorSize = physx::PxVec3(36.0f, 1.0f, 36.0f);

		/// Положение пола арены.
		inline static const physx::PxVec3 FloorPosition = physx::PxVec3(0.0f, -0.5f, 0.0f);

		/// Набор внешних коробок, формирующих стены арены.
		inline static const std::array<ArenaBoxDefinition, BoundaryObstacleCount> BoundaryObstacles = {
			ArenaBoxDefinition{ physx::PxVec3(36.0f, 3.0f, 1.0f), physx::PxVec3(0.0f, 1.5f, 18.5f), physx::PxQuat(physx::PxIdentity) },
			ArenaBoxDefinition{ physx::PxVec3(36.0f, 3.0f, 1.0f), physx::PxVec3(0.0f, 1.5f, -18.5f), physx::PxQuat(physx::PxIdentity) },
			ArenaBoxDefinition{ physx::PxVec3(1.0f, 3.0f, 36.0f), physx::PxVec3(18.5f, 1.5f, 0.0f), physx::PxQuat(physx::PxIdentity) },
			ArenaBoxDefinition{ physx::PxVec3(1.0f, 3.0f, 36.0f), physx::PxVec3(-18.5f, 1.5f, 0.0f), physx::PxQuat(physx::PxIdentity) }
		};

		/// Набор внутренних коробок, которые используются как препятствия и укрытия для AI.
		inline static const std::array<ArenaBoxDefinition, CoverObstacleCount> CoverObstacles = {
			ArenaBoxDefinition{ physx::PxVec3(2.5f, 3.0f, 2.5f), physx::PxVec3(-8.0f, 1.5f, -5.0f), physx::PxQuat(physx::PxIdentity) },
			ArenaBoxDefinition{ physx::PxVec3(3.0f, 2.5f, 1.5f), physx::PxVec3(-3.0f, 1.25f, 3.5f), physx::PxQuat(physx::PxIdentity) },
			ArenaBoxDefinition{ physx::PxVec3(1.5f, 4.0f, 4.5f), physx::PxVec3(3.0f, 2.0f, -4.0f), physx::PxQuat(physx::PxIdentity) },
			ArenaBoxDefinition{ physx::PxVec3(4.0f, 3.0f, 1.5f), physx::PxVec3(7.0f, 1.5f, 5.5f), physx::PxQuat(physx::PxIdentity) },
			ArenaBoxDefinition{ physx::PxVec3(1.2f, 4.5f, 3.0f), physx::PxVec3(8.5f, 2.25f, -2.0f), physx::PxQuat(physx::PxIdentity) }
		};
	};

	/// Константы, управляющие поведением AI противника.
	struct AIConfig {
		/// Главный флаг включения AI в проекте.
		inline static constexpr bool Enabled = true;

		/// Радиус обнаружения игрока противником.
		inline static constexpr float ViewSphereRadius = 14.0f;

		/// Скорость передвижения противника по плоскости.
		inline static constexpr float MoveSpeed = 4.5f;

		/// Интервал между переоценкой цели AI.
		inline static constexpr float RepathInterval = 0.25f;

		/// Порог, при котором AI считает, что дошёл до целевой точки.
		inline static constexpr float ArrivalThreshold = 0.6f;

		/// Радиус, на котором AI выбирает одну точку побега.
		inline static constexpr float FleeSampleRadius = 6.0f;

		/// Количество направлений, проверяемых при поиске точки побега.
		inline static constexpr int FleeSampleCount = 8;

		/// Малое смещение, используемое в raycast-проверках и отсечениях.
		inline static constexpr float RaycastEpsilon = 0.05f;

		/// Половина размера крестика, рисуемого в debug-визуализации цели AI.
		inline static constexpr float DebugMarkerHalfSize = 0.25f;

		/// Порог, ниже которого вектор считается слишком маленьким для нормализации.
		inline static constexpr float DirectionEpsilon = 1e-4f;

		/// Количество сегментов в окружности debug-сферы AI.
		inline static constexpr int DebugCircleSegments = 24;

		/// Максимальное число повторных raycast-попыток при игнорировании собственного тела врага.
		inline static constexpr int MaxSelfHitPasses = 8;

		/// Цвет debug-сферы и линий в состоянии простоя.
		inline static const physx::PxVec3 IdleColor = physx::PxVec3(0.25f, 0.95f, 0.35f);

		/// Цвет debug-сферы и линий в состоянии поиска укрытия.
		inline static const physx::PxVec3 SeekingCoverColor = physx::PxVec3(0.25f, 0.60f, 1.0f);

		/// Цвет debug-сферы и линий в состоянии побега.
		inline static const physx::PxVec3 FleeingColor = physx::PxVec3(1.0f, 0.60f, 0.15f);
	};

	/// Константы игрового процесса стрельбы и гранат.
	struct ShooterConfig {
		/// Длительность одного фиксированного шага симуляции.
		inline static constexpr float SimulationStep = 1.0f / 60.0f;

		/// Максимальная дальность пули при raycast-выстреле.
		inline static constexpr float BulletRange = 120.0f;

		/// Время жизни визуального следа пули.
		inline static constexpr float BulletTraceLifetime = 0.35f;

		/// Максимальный угол разброса пули в радианах.
		inline static constexpr float BulletSpreadRadians = 2.5f * physx::PxPi / 180.0f;

		/// Импульс, прикладываемый пулей при попадании.
		inline static constexpr float BulletImpulse = 180.0f;

		/// Урон от одной пули.
		inline static constexpr float BulletDamage = 20.0f;

		/// Время до взрыва гранаты после броска.
		inline static constexpr float GrenadeFuseTime = 2.5f;

		/// Горизонтальная составляющая скорости броска гранаты.
		inline static constexpr float GrenadeThrowSpeed = 18.0f;

		/// Дополнительная вертикальная составляющая скорости броска гранаты.
		inline static constexpr float GrenadeUpwardVelocity = 6.0f;

		/// Смещение точки появления гранаты вперёд от камеры.
		inline static constexpr float GrenadeSpawnOffset = 1.5f;

		/// Радиус поражения взрыва гранаты.
		inline static constexpr float GrenadeExplosionRadius = 6.0f;

		/// Максимальный урон от взрыва в эпицентре.
		inline static constexpr float GrenadeMaxDamage = 100.0f;

		/// Максимальный импульс от взрыва в эпицентре.
		inline static constexpr float GrenadeMaxImpulse = 240.0f;

		/// Время жизни визуального эффекта взрыва.
		inline static constexpr float ExplosionEffectLifetime = 0.45f;

		/// Минимальный порог длины луча для проверки экранирования взрыва.
		inline static constexpr float ExplosionBlockEpsilon = 1e-4f;

		/// Порог длины входного направления, ниже которого нельзя корректно рассчитать разброс.
		inline static constexpr float SpreadDirectionEpsilon = 1e-4f;
	};

	/// Константы, связанные только с отрисовкой и HUD.
	struct RenderConfig {
		/// Половина размера маркера попадания, рисуемого в конце трассы пули.
		inline static constexpr float ImpactMarkerHalfSize = 0.18f;

		/// Масштаб вспомогательных лучей взрыва относительно текущего радиуса сферы.
		inline static constexpr float ExplosionBurstRadiusScale = 0.75f;

		/// Масштаб диагональных лучей взрыва относительно текущего радиуса сферы.
		inline static constexpr float ExplosionDiagonalBurstScale = 0.70f;

		/// Резервируемое число акторов для отрисовки противника и ragdoll.
		inline static constexpr std::size_t EnemyActorReserve = EnemyConfig::RagdollPartCount;

		/// Резервируемое количество debug-линий AI за кадр.
		inline static constexpr std::size_t AIDebugLineReserve = 96;

		/// Размер буфера под одну строку HUD.
		inline static constexpr std::size_t HudLineBufferSize = 160;

		/// Цвет пола.
		inline static const physx::PxVec3 FloorColor = physx::PxVec3(0.25f, 0.25f, 0.28f);

		/// Цвет obstacle и стен.
		inline static const physx::PxVec3 ObstacleColor = physx::PxVec3(0.25f, 0.50f, 0.75f);

		/// Цвет противника и частей ragdoll.
		inline static const physx::PxVec3 EnemyColor = physx::PxVec3(0.85f, 0.25f, 0.25f);

		/// Цвет гранаты.
		inline static const physx::PxVec3 GrenadeColor = physx::PxVec3(0.95f, 0.65f, 0.15f);

		/// Цвет обычной трассы пули.
		inline static const physx::PxVec3 BulletColor = physx::PxVec3(1.0f, 0.95f, 0.30f);

		/// Цвет трассы при попадании во врага.
		inline static const physx::PxVec3 HitBulletColor = physx::PxVec3(1.0f, 0.35f, 0.25f);

		/// Цвет визуального эффекта взрыва.
		inline static const physx::PxVec3 ExplosionColor = physx::PxVec3(1.0f, 0.55f, 0.15f);
	};
};
