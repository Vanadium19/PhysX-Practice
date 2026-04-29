#pragma once

#include <vector>

#include "PxPhysicsAPI.h"

class Enemy;
class PhysicsEngine;

struct CoverPoint {
	physx::PxVec3 position;
	physx::PxVec3 outwardNormal;
	physx::PxRigidActor* obstacle = nullptr;
	int obstaclePointIndex = 0;
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
	static bool AreCoverPointsEquivalent(const physx::PxVec3& a, const physx::PxVec3& b);

	void ClearTarget();
	void EvaluateBehavior(const physx::PxVec3& playerEye, const physx::PxRigidActor* obstacleToAvoid = nullptr);
	void ResetCurrentObstacleTraversal();
	bool HasFailedCurrentObstaclePoint(const CoverPoint& coverPoint) const;
	void MarkCurrentCoverPointFailed();
	bool TrySelectAnotherPointOnCurrentObstacle(const physx::PxVec3& playerEye);
	bool BuildSequentialObstacleRoute(
		const CoverPoint& startPoint,
		const CoverPoint& destinationPoint,
		std::vector<physx::PxVec3>& routePoints,
		float& routeLength
	) const;
	void ClearCurrentPath();
	void AdvanceCurrentPath(const physx::PxVec3& enemyPosition);
	physx::PxVec3 GetActiveTarget() const;
	physx::PxVec3 GetCoverProbePosition(const CoverPoint& coverPoint) const;
	physx::PxVec3 GetVisibilityProbePosition(const physx::PxVec3& position) const;
	void UpdateMovement();
	bool FindBestCover(const physx::PxVec3& playerEye, CoverPoint& bestCover, const physx::PxRigidActor* obstacleToAvoid = nullptr) const;
	bool FindBestFleeTarget(const physx::PxVec3& playerEye, physx::PxVec3& bestTarget) const;
	bool IsPathClear(const physx::PxVec3& start, const physx::PxVec3& destination) const;
	bool IsPositionHiddenFromPlayer(const physx::PxVec3& position, const physx::PxVec3& playerEye) const;
	bool IsCoverProtected(const CoverPoint& coverPoint, const physx::PxVec3& playerEye) const;
	bool RaycastIgnoringEnemy(const physx::PxVec3& start, const physx::PxVec3& direction, float maxDistance, physx::PxRaycastHit& hit) const;
	void AppendCircle(std::vector<DebugLine>& out, const physx::PxVec3& center, const physx::PxVec3& axisA, const physx::PxVec3& axisB, float radius, const physx::PxVec3& color) const;
	physx::PxVec3 GetStateColor() const;

	PhysicsEngine* physicsEngine_ = nullptr;
	Enemy* enemy_ = nullptr;
	const std::vector<CoverPoint>* coverPoints_ = nullptr;

	State state_ = State::Disabled;
	CoverPoint currentCoverPoint_{};
	physx::PxVec3 currentTarget_ = physx::PxVec3(0.0f);
	std::vector<physx::PxVec3> currentPathPoints_;
	std::vector<physx::PxVec3> failedCurrentObstaclePoints_;
	std::size_t currentPathPointIndex_ = 0;
	bool hasTarget_ = false;
	bool hasCoverPoint_ = false;
	bool currentObstacleHasProvidedCover_ = false;
	bool playerInsideView_ = false;
	float repathTimer_ = 0.0f;
};
