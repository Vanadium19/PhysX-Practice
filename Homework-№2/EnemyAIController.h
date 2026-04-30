#pragma once

#include <vector>

#include "PxPhysicsAPI.h"

class Enemy;
class PhysicsEngine;

struct CoverPoint {
	physx::PxVec3 position;
	physx::PxRigidActor* obstacle = nullptr;
};

class EnemyAIController {
public:
	struct DebugLine {
		physx::PxVec3 start;
		physx::PxVec3 end;
		physx::PxVec3 color;
	};

	enum class State {
		Idle,
		SeekingCover,
		Fleeing,
		Disabled
	};

	EnemyAIController() = default;

	void Initialize(PhysicsEngine* physicsEngine, Enemy* enemy, const std::vector<CoverPoint>* coverPoints);
	void Shutdown();
	void Update(float elapsedTime, const physx::PxVec3& playerEye);
	void AppendDebugLines(std::vector<DebugLine>& out) const;
	const char* GetStateName() const;

private:
	static physx::PxVec3 ClampToArena(const physx::PxVec3& position);
	static physx::PxVec3 MakePlanar(const physx::PxVec3& vector);
	static float GetPlanarDistanceSquared(const physx::PxVec3& a, const physx::PxVec3& b);

	void ClearTarget();
	void SelectBehavior(const physx::PxVec3& enemyPosition, const physx::PxVec3& playerPosition);
	void UpdateMovement();
	bool FindBestCoverPoint(const physx::PxVec3& enemyPosition, const physx::PxVec3& playerPosition, CoverPoint& bestCover) const;
	bool FindBestFleeTarget(const physx::PxVec3& enemyPosition, const physx::PxVec3& playerPosition, physx::PxVec3& bestTarget) const;
	bool CanSeePosition(const physx::PxVec3& observerPosition, const physx::PxVec3& targetPosition, float viewRadius) const;
	bool IsPathClear(const physx::PxVec3& start, const physx::PxVec3& destination) const;
	bool RaycastIgnoringEnemy(const physx::PxVec3& start, const physx::PxVec3& direction, float maxDistance, physx::PxRaycastHit& hit) const;
	void AppendCircle(std::vector<DebugLine>& out, const physx::PxVec3& center, const physx::PxVec3& axisA, const physx::PxVec3& axisB, float radius, const physx::PxVec3& color) const;
	physx::PxVec3 GetStateColor() const;

	PhysicsEngine* physicsEngine_ = nullptr;
	Enemy* enemy_ = nullptr;
	const std::vector<CoverPoint>* coverPoints_ = nullptr;

	State state_ = State::Disabled;
	CoverPoint currentCoverPoint_{};
	physx::PxVec3 currentTarget_ = physx::PxVec3(0.0f);
	bool hasTarget_ = false;
	bool hasCoverPoint_ = false;
	float repathTimer_ = 0.0f;
};
