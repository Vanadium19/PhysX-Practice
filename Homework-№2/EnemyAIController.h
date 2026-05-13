#pragma once

#include <array>
#include <vector>

#include "PxPhysicsAPI.h"

class Enemy;
class PhysicsEngine;

/// Описание одной точки укрытия, к которой может стремиться AI.
struct CoverPoint {
	/// Мировая позиция точки, куда должен добежать противник.
	physx::PxVec3 position;

	/// Укрытие, которому принадлежит эта точка.
	physx::PxRigidActor* obstacle = nullptr;
};

/// Контроллер простейшего AI противника.
///
/// Он отвечает только за выбор состояния, точки укрытия и маршрута движения.
class EnemyAIController {
public:
	/// Отладочная линия для визуализации состояния AI на сцене.
	struct DebugLine {
		/// Начало линии.
		physx::PxVec3 start;

		/// Конец линии.
		physx::PxVec3 end;

		/// Цвет линии.
		physx::PxVec3 color;
	};

	/// Высокоуровневое состояние AI.
	enum class State {
		/// Игрок не обнаружен, противник стоит на месте.
		Idle,

		/// Игрок обнаружен, противник ищет и занимает укрытие.
		SeekingCover,

		/// Укрытие не найдено, противник пытается отступить.
		Fleeing,

		/// AI выключен или временно недоступен.
		Disabled
	};

	/// Создаёт контроллер без связанного противника и сцены.
	EnemyAIController() = default;

	/// Привязывает AI к физике, противнику и списку точек укрытия.
	void Initialize(PhysicsEngine* physicsEngine, Enemy* enemy, const std::vector<CoverPoint>* coverPoints);

	/// Отвязывает AI и останавливает управляемое движение.
	void Shutdown();

	/// Обновляет обнаружение игрока, выбор состояния и маршрут движения.
	void Update(float elapsedTime, const physx::PxVec3& playerEye);

	/// Добавляет debug-линии AI в выходной массив для последующей отрисовки.
	void AppendDebugLines(std::vector<DebugLine>& out) const;

	/// Возвращает человекочитаемое имя текущего состояния AI.
	const char* GetStateName() const;

private:
	/// Ограничивает точку пределами арены.
	static physx::PxVec3 ClampToArena(const physx::PxVec3& position);

	/// Обнуляет компоненту `Y`, оставляя только движение по плоскости.
	static physx::PxVec3 MakePlanar(const physx::PxVec3& vector);

	/// Вычисляет квадрат расстояния между двумя точками только в плоскости `XZ`.
	static float GetPlanarDistanceSquared(const physx::PxVec3& a, const physx::PxVec3& b);

	/// Сбрасывает текущую цель и связанные с ней флаги.
	void ClearTarget();

	/// Очищает список промежуточных точек маршрута.
	void ClearCurrentPath();

	/// Продвигает индекс активного waypoint, если противник уже дошёл до него.
	void AdvanceCurrentPath(const physx::PxVec3& enemyPosition);

	/// Возвращает текущую активную точку движения: waypoint или финальную цель.
	physx::PxVec3 GetActiveTarget() const;

	/// Выбирает новое состояние AI и соответствующую цель движения.
	void SelectBehavior(const physx::PxVec3& enemyPosition, const physx::PxVec3& playerPosition);

	/// Переводит выбранную цель в фактическую скорость противника.
	void UpdateMovement();

	/// Ищет лучшую точку укрытия и одновременно строит маршрут к ней.
	bool FindBestCoverPoint(
		const physx::PxVec3& enemyPosition,
		const physx::PxVec3& playerPosition,
		CoverPoint& bestCover,
		std::vector<physx::PxVec3>& bestRoute
	) const;

	/// Ищет лучшую точку побега, если безопасного укрытия не нашлось.
	bool FindBestFleeTarget(const physx::PxVec3& enemyPosition, const physx::PxVec3& playerPosition, physx::PxVec3& bestTarget) const;

	/// Строит маршрут до точки укрытия напрямую или в обход углов obstacle.
	bool BuildPathToCoverPoint(
		const physx::PxVec3& start,
		const CoverPoint& coverPoint,
		std::vector<physx::PxVec3>& routePoints,
		float& routeLength
	) const;

	/// Возвращает четыре угловые точки контура obstacle для построения обхода.
	bool GetObstacleRouteCorners(const physx::PxRigidActor* obstacle, std::array<physx::PxVec3, 4>& corners) const;

	/// Проверяет, пересекает ли прямой путь расширенный контур obstacle.
	bool DoesPathCrossObstacleFootprint(
		const physx::PxVec3& start,
		const physx::PxVec3& destination,
		const physx::PxRigidActor* obstacle
	) const;

	/// Проверяет, видит ли наблюдатель цель с учётом радиуса и преград.
	bool CanSeePosition(const physx::PxVec3& observerPosition, const physx::PxVec3& targetPosition, float viewRadius) const;

	/// Проверяет, свободен ли прямой путь между двумя точками.
	bool IsPathClear(const physx::PxVec3& start, const physx::PxVec3& destination) const;

	/// Выполняет raycast, пропуская столкновения с телом самого противника.
	bool RaycastIgnoringEnemy(const physx::PxVec3& start, const physx::PxVec3& direction, float maxDistance, physx::PxRaycastHit& hit) const;

	/// Добавляет окружность в виде набора debug-линий.
	void AppendCircle(std::vector<DebugLine>& out, const physx::PxVec3& center, const physx::PxVec3& axisA, const physx::PxVec3& axisB, float radius, const physx::PxVec3& color) const;

	/// Возвращает цвет для текущего состояния AI.
	physx::PxVec3 GetStateColor() const;

	/// Указатель на физический движок для raycast-проверок.
	PhysicsEngine* physicsEngine_ = nullptr;

	/// Противник, которым управляет этот AI.
	Enemy* enemy_ = nullptr;

	/// Внешний список всех доступных точек укрытия на сцене.
	const std::vector<CoverPoint>* coverPoints_ = nullptr;

	/// Текущее состояние AI.
	State state_ = State::Disabled;

	/// Выбранная точка укрытия, если AI находится в поиске cover.
	CoverPoint currentCoverPoint_{};

	/// Финальная целевая точка движения.
	physx::PxVec3 currentTarget_ = physx::PxVec3(0.0f);

	/// Список промежуточных waypoint’ов маршрута.
	std::vector<physx::PxVec3> currentPathPoints_;

	/// Индекс активного waypoint в текущем маршруте.
	std::size_t currentPathPointIndex_ = 0;

	/// Флаг наличия любой активной цели движения.
	bool hasTarget_ = false;

	/// Флаг того, что текущая цель относится именно к укрытию.
	bool hasCoverPoint_ = false;

	/// Таймер до следующей полной переоценки поведения.
	float repathTimer_ = 0.0f;
};
