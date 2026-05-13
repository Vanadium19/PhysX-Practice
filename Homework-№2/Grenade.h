#pragma once

#include "PxPhysicsAPI.h"

class PhysicsEngine;

/// Класс гранаты с физическим телом, таймером взрыва и обновлением по времени.
class Grenade {
public:
	/// Создаёт гранату с заданным временем до взрыва.
	explicit Grenade(float fuseTimeSeconds);

	/// Создаёт физическое тело гранаты и задаёт ей стартовую скорость.
	void Initialize(PhysicsEngine* physicsEngine, const physx::PxVec3& position, const physx::PxVec3& initialVelocity);

	/// Удаляет физическое тело гранаты и сбрасывает её внутреннее состояние.
	void Shutdown();

	/// Обновляет внутренний таймер гранаты.
	void Update(float elapsedTime);

	/// Возвращает физический актор гранаты.
	physx::PxRigidDynamic* GetActor() const;

	/// Возвращает текущую мировую позицию гранаты.
	physx::PxVec3 GetPosition() const;

	/// Проверяет, достигла ли граната времени взрыва.
	bool ShouldExplode() const;

private:
	/// Указатель на физический движок, через который создаётся актор гранаты.
	PhysicsEngine* physicsEngine_ = nullptr;

	/// Физический актор гранаты.
	physx::PxRigidDynamic* actor_ = nullptr;

	/// Время, прошедшее с момента броска.
	float elapsedTime_ = 0.0f;

	/// Полное время до взрыва после инициализации.
	float fuseTimeSeconds_ = 0.0f;
};
