#pragma once

#include <cstddef>
#include <vector>

#include "PxPhysicsAPI.h"

class PhysicsEngine;

/// Состояние противника на уровне высокоуровневой игровой логики.
enum class EnemyState {
	/// Противник ещё жив и представлен одной физической капсулой.
	Alive,

	/// Противник убит и заменён на набор ragdoll-частей.
	Ragdoll
};

/// Класс противника, объединяющий живое состояние, получение урона и ragdoll.
class Enemy {
public:
	/// Создаёт пустой объект противника без физического тела.
	Enemy() = default;

	/// Инициализирует противника в заданной позиции и создаёт живую капсулу.
	void Initialize(PhysicsEngine* physicsEngine, const physx::PxVec3& position);

	/// Полностью удаляет физические объекты противника и сбрасывает состояние.
	void Shutdown();

	/// Возвращает основной `PxRigidDynamic`, связанный с противником.
	///
	/// Для живого противника это капсула, для ragdoll — корневая часть тела.
	physx::PxRigidDynamic* GetActor() const;

	/// Добавляет все акторы противника в массив для последующей отрисовки.
	void AppendRenderActors(std::vector<physx::PxRigidActor*>& out) const;

	/// Возвращает текущую мировую позицию противника.
	physx::PxVec3 GetPosition() const;

	/// Проверяет, принадлежит ли переданный актор этому противнику.
	bool OwnsActor(const physx::PxActor* actor) const;

	/// Возвращает текущее количество здоровья.
	float GetHealth() const;

	/// Проверяет, жив ли противник.
	bool IsAlive() const;

	/// Проверяет, активирован ли у противника ragdoll.
	bool IsRagdollActive() const;

	/// Проверяет, может ли текущая форма противника управляться AI.
	bool CanUseAI() const;

	/// Задаёт противнику скорость только по плоскости `XZ`.
	void SetPlanarVelocity(const physx::PxVec3& velocity);

	/// Останавливает движение противника по плоскости `XZ`, не сбрасывая вертикальную скорость.
	void StopPlanarMovement();

	/// Применяет попадание пули: наносит урон и при необходимости активирует ragdoll.
	float ApplyBulletImpact(const physx::PxVec3& hitPosition, const physx::PxVec3& direction, float impulseStrength, float damage);

	/// Применяет урон и импульс от взрыва гранаты.
	float ApplyExplosionDamage(const physx::PxVec3& explosionPosition, float radius, float maxDamage, float maxImpulse);

private:
	/// Возвращает поворот, который ставит капсулу PhysX вертикально.
	static physx::PxQuat GetVerticalCapsuleRotation();

	/// Строит ориентацию локальной рамки сустава по направлению его основной оси.
	static physx::PxQuat GetJointFrameRotation(const physx::PxVec3& axisDirection);

	/// Собирает локальную рамку сустава относительно заданного актора.
	static physx::PxTransform BuildLocalJointFrame(const physx::PxRigidActor& actor, const physx::PxVec3& anchorWorldPosition, const physx::PxVec3& axisDirection);

	/// Идентификатор отдельной части ragdoll.
	enum class RagdollPartId : std::size_t {
		/// Голова ragdoll.
		Head = 0,

		/// Туловище ragdoll.
		Torso,

		/// Левая рука ragdoll.
		LeftArm,

		/// Правая рука ragdoll.
		RightArm,

		/// Левая нога ragdoll.
		LeftLeg,

		/// Правая нога ragdoll.
		RightLeg,

		/// Служебное количество частей ragdoll.
		Count
	};

	/// Создаёт физическую капсулу живого противника.
	void CreateLiveCapsule(const physx::PxVec3& position);

	/// Освобождает актор живой капсулы.
	void ReleaseLiveCapsule();

	/// Освобождает все части и суставы ragdoll.
	void ReleaseRagdoll();

	/// Активирует ragdoll без дополнительного внешнего импульса.
	void ActivateRagdoll(const physx::PxTransform& capsulePose, const physx::PxVec3& linearVelocity, const physx::PxVec3& angularVelocity);

	/// Активирует ragdoll после смертельного выстрела и переносит импульс пули в ближайшую часть.
	void ActivateRagdollFromBullet(
		const physx::PxTransform& capsulePose,
		const physx::PxVec3& linearVelocity,
		const physx::PxVec3& angularVelocity,
		const physx::PxVec3& hitPosition,
		const physx::PxVec3& direction,
		float impulseStrength
	);

	/// Активирует ragdoll после смертельного взрыва и разбрасывает части наружу.
	void ActivateRagdollFromExplosion(
		const physx::PxTransform& capsulePose,
		const physx::PxVec3& linearVelocity,
		const physx::PxVec3& angularVelocity,
		const physx::PxVec3& explosionPosition,
		float radius,
		float maxImpulse
	);

	/// Создаёт коробчатую часть ragdoll.
	physx::PxRigidDynamic* CreateRagdollBox(const physx::PxVec3& size, const physx::PxVec3& position, const physx::PxQuat& rotation, float mass);

	/// Создаёт сферическую часть ragdoll.
	physx::PxRigidDynamic* CreateRagdollSphere(float radius, const physx::PxVec3& position, float mass);

	/// Создаёт капсульную часть ragdoll.
	physx::PxRigidDynamic* CreateRagdollCapsule(float radius, float size, const physx::PxVec3& position, const physx::PxQuat& rotation, float mass);

	/// Создаёт ограниченный `PxD6Joint` между двумя частями ragdoll.
	physx::PxD6Joint* CreateLimitedJoint(
		physx::PxRigidDynamic* actor0,
		physx::PxRigidDynamic* actor1,
		const physx::PxVec3& anchorWorldPosition,
		const physx::PxVec3& axisDirection,
		float lowerTwistLimit,
		float upperTwistLimit,
		float swing1Limit,
		float swing2Limit
	);

	/// Применяет общие параметры физики к одной части ragdoll.
	void ConfigureRagdollPart(physx::PxRigidDynamic& actor, const physx::PxVec3& linearVelocity, const physx::PxVec3& angularVelocity) const;

	/// Прикладывает импульс к той части ragdoll, которая ближе всего к точке попадания.
	void ApplyImpulseToClosestRagdollPart(const physx::PxVec3& hitPosition, const physx::PxVec3& direction, float impulseStrength);

	/// Распределяет импульс взрыва по всем частям ragdoll в зависимости от расстояния.
	void ApplyExplosionImpulseToRagdoll(const physx::PxVec3& explosionPosition, float radius, float maxImpulse);

	/// Находит индекс ближайшей ragdoll-части к заданной точке.
	std::size_t FindClosestRagdollPartIndex(const physx::PxVec3& position) const;

	/// Возвращает указатель на конкретную часть ragdoll по идентификатору.
	physx::PxRigidDynamic* GetRagdollPart(RagdollPartId partId) const;

	/// Возвращает корневой актор противника для высокоуровневых запросов.
	physx::PxRigidDynamic* GetRootActor() const;

	/// Указатель на физический движок, через который создаются тела и материалы.
	PhysicsEngine* physicsEngine_ = nullptr;

	/// Актор живой капсулы, существующий до активации ragdoll.
	physx::PxRigidDynamic* capsuleActor_ = nullptr;

	/// Центральная часть ragdoll, используемая как его корень.
	physx::PxRigidDynamic* torsoActor_ = nullptr;

	/// Список всех физических частей ragdoll.
	std::vector<physx::PxRigidDynamic*> ragdollParts_;

	/// Список всех суставов ragdoll.
	std::vector<physx::PxJoint*> ragdollJoints_;

	/// Текущее состояние противника.
	EnemyState state_ = EnemyState::Alive;

	/// Текущий запас здоровья.
	float health_ = 100.0f;
};
