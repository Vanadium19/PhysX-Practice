#pragma once

#include "PxPhysicsAPI.h"

class PhysicsEngine;

class Grenade {
public:
	explicit Grenade(float fuseTimeSeconds);

	void Initialize(PhysicsEngine* physicsEngine, const physx::PxVec3& position, const physx::PxVec3& initialVelocity);
	void Shutdown();
	void Update(float elapsedTime);

	physx::PxRigidDynamic* GetActor() const;
	physx::PxVec3 GetPosition() const;
	bool ShouldExplode() const;

private:
	PhysicsEngine* physicsEngine_ = nullptr;
	physx::PxRigidDynamic* actor_ = nullptr;
	float elapsedTime_ = 0.0f;
	float fuseTimeSeconds_ = 0.0f;
};
