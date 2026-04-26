#pragma once

#include "PxPhysicsAPI.h"

class PhysicsEngine;

class Enemy {
public:
	Enemy() = default;

	void Initialize(PhysicsEngine* physicsEngine, const physx::PxVec3& position);
	void Shutdown();

	physx::PxRigidDynamic* GetActor() const;
	physx::PxVec3 GetPosition() const;
	bool OwnsActor(const physx::PxActor* actor) const;
	float GetHealth() const;

	void ApplyBulletImpact(const physx::PxVec3& hitPosition, const physx::PxVec3& direction, float impulseStrength);
	float ApplyExplosionDamage(const physx::PxVec3& explosionPosition, float radius, float maxDamage, float maxImpulse);

private:
	static physx::PxQuat GetVerticalCapsuleRotation();

	PhysicsEngine* physicsEngine_ = nullptr;
	physx::PxRigidDynamic* actor_ = nullptr;
	float health_ = 100.0f;
};
