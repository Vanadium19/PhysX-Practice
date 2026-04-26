#pragma once

#include <memory>
#include <random>
#include <vector>

#include "PxPhysicsAPI.h"

#include "Enemy.h"

class Grenade;
class PhysicsEngine;

namespace Snippets {
class Camera;
}

class ShooterGame {
public:
	ShooterGame(PhysicsEngine* physicsEngine, Snippets::Camera* camera);
	~ShooterGame();

	void Initialize();
	void Shutdown();
	void HandleKey(unsigned char key, const physx::PxTransform& cameraTransform);
	void RenderFrame();

private:
	struct BulletTrace {
		physx::PxVec3 start;
		physx::PxVec3 end;
		physx::PxVec3 color;
		float remainingLifetime;
	};

	void Reset();
	void CreateArena();
	void CreateEnemy();
	void CreateStaticBox(
		std::vector<physx::PxRigidActor*>& actors,
		physx::PxVec3 size,
		physx::PxVec3 position,
		physx::PxMaterial* material,
		physx::PxQuat rotation = physx::PxQuat(physx::PxIdentity)
	);
	void ReleaseActors(std::vector<physx::PxRigidActor*>& actors);

	void Shoot(const physx::PxVec3& origin, const physx::PxVec3& direction);
	void ThrowGrenade(const physx::PxVec3& origin, const physx::PxVec3& direction);
	void ExplodeGrenade(const physx::PxVec3& explosionPosition);

	void UpdateGrenades(float elapsedTime);
	void UpdateBulletTraces(float elapsedTime);
	bool IsExplosionBlocked(const physx::PxVec3& explosionPosition, const physx::PxVec3& targetPosition) const;

	physx::PxVec3 ApplySpread(const physx::PxVec3& direction);
	void AddBulletTrace(const physx::PxVec3& start, const physx::PxVec3& end, const physx::PxVec3& color);

	void RenderHud() const;
	void RenderBulletTraces() const;

	PhysicsEngine* physicsEngine_ = nullptr;
	Snippets::Camera* camera_ = nullptr;

	std::vector<physx::PxRigidActor*> floorActors_;
	std::vector<physx::PxRigidActor*> obstacleActors_;
	std::vector<std::unique_ptr<Grenade>> grenades_;
	std::vector<BulletTrace> bulletTraces_;

	Enemy enemy_;
	std::mt19937 randomEngine_;
};
