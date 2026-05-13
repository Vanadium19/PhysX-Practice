#pragma once

#include <memory>
#include <random>
#include <vector>

#include "PxPhysicsAPI.h"

#include "EnemyAIController.h"
#include "Enemy.h"

class Grenade;
class PhysicsEngine;

namespace Snippets {
class Camera;
}

/// Основной игровой класс, управляющий сценой, вводом и обновлением мира.
class ShooterGame {
public:
	/// Создаёт игровой объект и сохраняет ссылки на физику и камеру.
	ShooterGame(PhysicsEngine* physicsEngine, Snippets::Camera* camera);

	/// Корректно освобождает все игровые объекты.
	~ShooterGame();

	/// Создаёт сцену, врага и инициализирует AI.
	void Initialize();

	/// Полностью очищает сцену и удаляет все игровые объекты.
	void Shutdown();

	/// Обрабатывает нажатия игровых клавиш.
	void HandleKey(unsigned char key, const physx::PxTransform& cameraTransform);

	/// Выполняет один игровой кадр: AI, физика, эффекты и рендер.
	void RenderFrame();

private:
	/// Данные одной визуализируемой трассы пули.
	struct BulletTrace {
		/// Начало трассы.
		physx::PxVec3 start;

		/// Конец трассы.
		physx::PxVec3 end;

		/// Цвет линии трассы.
		physx::PxVec3 color;

		/// Флаг, показывающий, нужно ли рисовать маркер попадания.
		bool hasImpactMarker;

		/// Оставшееся время жизни трассы.
		float remainingLifetime;
	};

	/// Данные одного визуального эффекта взрыва.
	struct ExplosionEffect {
		/// Центр эффекта взрыва.
		physx::PxVec3 position;

		/// Максимальный радиус эффекта.
		float maxRadius;

		/// Полная длительность жизни эффекта.
		float totalLifetime;

		/// Оставшееся время жизни эффекта.
		float remainingLifetime;
	};

	/// Полностью пересоздаёт сцену в исходное состояние.
	void Reset();

	/// Создаёт пол, стены и коробчатые укрытия.
	void CreateArena();

	/// Создаёт противника в стартовой точке.
	void CreateEnemy();

	/// Создаёт одну статическую коробку и добавляет её в заданный список акторов.
	physx::PxRigidStatic* CreateStaticBox(
		std::vector<physx::PxRigidActor*>& actors,
		physx::PxVec3 size,
		physx::PxVec3 position,
		physx::PxMaterial* material,
		physx::PxQuat rotation = physx::PxQuat(physx::PxIdentity)
	);

	/// Генерирует точки укрытия для одного obstacle.
	void AddCoverPointsForBox(
		physx::PxVec3 size,
		physx::PxVec3 position,
		physx::PxQuat rotation,
		physx::PxRigidActor* obstacle
	);

	/// Освобождает все акторы из указанного массива.
	void ReleaseActors(std::vector<physx::PxRigidActor*>& actors);

	/// Выполняет выстрел пули по направлению взгляда.
	void Shoot(const physx::PxVec3& origin, const physx::PxVec3& direction);

	/// Создаёт и бросает гранату по направлению взгляда.
	void ThrowGrenade(const physx::PxVec3& origin, const physx::PxVec3& direction);

	/// Обрабатывает взрыв гранаты, урон и визуальные эффекты.
	void ExplodeGrenade(const physx::PxVec3& explosionPosition);

	/// Обновляет все активные гранаты.
	void UpdateGrenades(float elapsedTime);

	/// Обновляет время жизни визуальных трасс пуль.
	void UpdateBulletTraces(float elapsedTime);

	/// Обновляет время жизни визуальных эффектов взрыва.
	void UpdateExplosionEffects(float elapsedTime);

	/// Проверяет, перекрыт ли взрыв препятствием по пути к цели.
	bool IsExplosionBlocked(const physx::PxVec3& explosionPosition, const physx::PxVec3& targetPosition) const;

	/// Применяет случайный разброс к направлению выстрела.
	physx::PxVec3 ApplySpread(const physx::PxVec3& direction);

	/// Добавляет новую визуальную трассу пули.
	void AddBulletTrace(const physx::PxVec3& start, const physx::PxVec3& end, const physx::PxVec3& color, bool hasImpactMarker);

	/// Добавляет новый визуальный эффект взрыва.
	void AddExplosionEffect(const physx::PxVec3& position, float maxRadius);

	/// Отрисовывает текстовый HUD поверх сцены.
	void RenderHud() const;

	/// Отрисовывает debug-визуализацию AI.
	void RenderEnemyAIDebug() const;

	/// Отрисовывает все активные трассы пуль.
	void RenderBulletTraces() const;

	/// Отрисовывает все активные эффекты взрывов.
	void RenderExplosionEffects() const;

	/// Ссылка на физический движок сцены.
	PhysicsEngine* physicsEngine_ = nullptr;

	/// Камера, через которую игрок наблюдает сцену.
	Snippets::Camera* camera_ = nullptr;

	/// Акторы пола.
	std::vector<physx::PxRigidActor*> floorActors_;

	/// Акторы стен и укрытий.
	std::vector<physx::PxRigidActor*> obstacleActors_;

	/// Все точки укрытия, доступные AI.
	std::vector<CoverPoint> coverPoints_;

	/// Все активные гранаты.
	std::vector<std::unique_ptr<Grenade>> grenades_;

	/// Все текущие визуальные трассы пуль.
	std::vector<BulletTrace> bulletTraces_;

	/// Все текущие визуальные эффекты взрывов.
	std::vector<ExplosionEffect> explosionEffects_;

	/// Игровой объект противника.
	Enemy enemy_;

	/// Контроллер AI противника.
	EnemyAIController enemyAI_;

	/// Генератор случайных чисел для разброса пуль.
	std::mt19937 randomEngine_;
};
