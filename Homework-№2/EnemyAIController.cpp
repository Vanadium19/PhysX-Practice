#include "EnemyAIController.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

#include "Enemy.h"
#include "GameConstants.h"
#include "PhysicsEngine.h"

void EnemyAIController::Initialize(PhysicsEngine* physicsEngine, Enemy* enemy, const std::vector<CoverPoint>* coverPoints) {
	physicsEngine_ = physicsEngine;
	enemy_ = enemy;
	coverPoints_ = coverPoints;
	state_ = State::Idle;
	repathTimer_ = 0.0f;
	playerInsideView_ = false;
	ClearTarget();
}

void EnemyAIController::Shutdown() {
	if (enemy_) {
		enemy_->StopPlanarMovement();
	}

	physicsEngine_ = nullptr;
	enemy_ = nullptr;
	coverPoints_ = nullptr;
	state_ = State::Disabled;
	repathTimer_ = 0.0f;
	playerInsideView_ = false;
	ClearTarget();
}

void EnemyAIController::Update(float elapsedTime, const physx::PxVec3& playerEye) {
	if (!physicsEngine_ || !enemy_) {
		return;
	}

	if (!enemy_->CanUseAI()) {
		if (state_ != State::Disabled) {
			enemy_->StopPlanarMovement();
		}

		state_ = State::Disabled;
		playerInsideView_ = false;
		repathTimer_ = 0.0f;
		ClearTarget();
		return;
	}

	if (state_ == State::Disabled) {
		state_ = State::Idle;
	}

	const physx::PxVec3 enemyPosition = enemy_->GetPosition();
	physx::PxVec3 sightPlayerEye = playerEye;
	sightPlayerEye.y = enemyPosition.y;
	const bool isPlayerInsideView =
		(sightPlayerEye - enemyPosition).magnitudeSquared()
		<= GameConstants::AIConfig::ViewSphereRadius * GameConstants::AIConfig::ViewSphereRadius;
	if (!isPlayerInsideView) {
		state_ = State::Idle;
		playerInsideView_ = false;
		repathTimer_ = 0.0f;
		ClearTarget();
		enemy_->StopPlanarMovement();
		return;
	}

	if (!playerInsideView_) {
		repathTimer_ = 0.0f;
	}
	playerInsideView_ = true;

	AdvanceCurrentPath(enemyPosition);
	const bool reachedTarget =
		hasTarget_
		&& currentPathPointIndex_ >= currentPathPoints_.size()
		&& GetPlanarDistanceSquared(enemyPosition, currentTarget_)
			<= GameConstants::AIConfig::ArrivalThreshold * GameConstants::AIConfig::ArrivalThreshold;
	if (reachedTarget) {
		enemy_->StopPlanarMovement();
	}

	repathTimer_ -= elapsedTime;

	if (state_ == State::SeekingCover && hasCoverPoint_) {
		if (reachedTarget && IsPositionHiddenFromPlayer(enemyPosition, sightPlayerEye)) {
			currentObstacleHasProvidedCover_ = true;
		}
		else if (reachedTarget && !IsPositionHiddenFromPlayer(enemyPosition, sightPlayerEye)) {
			const physx::PxRigidActor* exhaustedObstacle = currentCoverPoint_.obstacle;
			if (currentObstacleHasProvidedCover_) {
				ResetCurrentObstacleTraversal();
				EvaluateBehavior(sightPlayerEye, exhaustedObstacle);
			}
			else {
				MarkCurrentCoverPointFailed();
				if (!TrySelectAnotherPointOnCurrentObstacle(sightPlayerEye)) {
					EvaluateBehavior(sightPlayerEye, exhaustedObstacle);
				}
			}

			repathTimer_ = GameConstants::AIConfig::RepathInterval;
			UpdateMovement();
			return;
		}
	}
	else if (state_ == State::Fleeing && reachedTarget) {
		repathTimer_ = 0.0f;
	}

	if (state_ == State::Idle || !hasTarget_ || (state_ == State::Fleeing && repathTimer_ <= 0.0f)) {
		EvaluateBehavior(sightPlayerEye);
		repathTimer_ = GameConstants::AIConfig::RepathInterval;
	}

	UpdateMovement();
}

void EnemyAIController::AppendDebugLines(std::vector<DebugLine>& out) const {
	if (!enemy_ || !enemy_->CanUseAI() || state_ == State::Disabled) {
		return;
	}

	const physx::PxVec3 color = GetStateColor();
	const physx::PxVec3 center = enemy_->GetPosition();

	AppendCircle(
		out,
		center,
		physx::PxVec3(1.0f, 0.0f, 0.0f),
		physx::PxVec3(0.0f, 1.0f, 0.0f),
		GameConstants::AIConfig::ViewSphereRadius,
		color
	);
	AppendCircle(
		out,
		center,
		physx::PxVec3(1.0f, 0.0f, 0.0f),
		physx::PxVec3(0.0f, 0.0f, 1.0f),
		GameConstants::AIConfig::ViewSphereRadius,
		color
	);
	AppendCircle(
		out,
		center,
		physx::PxVec3(0.0f, 1.0f, 0.0f),
		physx::PxVec3(0.0f, 0.0f, 1.0f),
		GameConstants::AIConfig::ViewSphereRadius,
		color
	);

	if (!hasTarget_ || (state_ != State::SeekingCover && state_ != State::Fleeing)) {
		return;
	}

	const physx::PxVec3 activeTarget = GetActiveTarget();
	out.push_back(DebugLine{ center, activeTarget, color });
	out.push_back(DebugLine{
		activeTarget + physx::PxVec3(-GameConstants::AIConfig::DebugMarkerHalfSize, 0.0f, 0.0f),
		activeTarget + physx::PxVec3(GameConstants::AIConfig::DebugMarkerHalfSize, 0.0f, 0.0f),
		color
	});
	out.push_back(DebugLine{
		activeTarget + physx::PxVec3(0.0f, -GameConstants::AIConfig::DebugMarkerHalfSize, 0.0f),
		activeTarget + physx::PxVec3(0.0f, GameConstants::AIConfig::DebugMarkerHalfSize, 0.0f),
		color
	});
	out.push_back(DebugLine{
		activeTarget + physx::PxVec3(0.0f, 0.0f, -GameConstants::AIConfig::DebugMarkerHalfSize),
		activeTarget + physx::PxVec3(0.0f, 0.0f, GameConstants::AIConfig::DebugMarkerHalfSize),
		color
	});
}

const char* EnemyAIController::GetStateName() const {
	switch (state_) {
	case State::Idle:
		return "Idle";
	case State::SeekingCover:
		return "SeekingCover";
	case State::Fleeing:
		return "Fleeing";
	case State::Disabled:
	default:
		return "Disabled";
	}
}

physx::PxVec3 EnemyAIController::ClampToArena(const physx::PxVec3& position) {
	return physx::PxVec3(
		std::clamp(position.x, GameConstants::ArenaConfig::ClampMin, GameConstants::ArenaConfig::ClampMax),
		position.y,
		std::clamp(position.z, GameConstants::ArenaConfig::ClampMin, GameConstants::ArenaConfig::ClampMax)
	);
}

physx::PxVec3 EnemyAIController::MakePlanar(const physx::PxVec3& vector) {
	return physx::PxVec3(vector.x, 0.0f, vector.z);
}

float EnemyAIController::GetPlanarDistanceSquared(const physx::PxVec3& a, const physx::PxVec3& b) {
	return MakePlanar(a - b).magnitudeSquared();
}

bool EnemyAIController::AreCoverPointsEquivalent(const physx::PxVec3& a, const physx::PxVec3& b) {
	return (a - b).magnitudeSquared() <= GameConstants::AIConfig::CoverPointComparisonEpsilon;
}

void EnemyAIController::ClearTarget() {
	hasTarget_ = false;
	hasCoverPoint_ = false;
	currentObstacleHasProvidedCover_ = false;
	currentCoverPoint_ = CoverPoint{};
	currentTarget_ = physx::PxVec3(0.0f);
	ClearCurrentPath();
	ResetCurrentObstacleTraversal();
}

void EnemyAIController::ResetCurrentObstacleTraversal() {
	failedCurrentObstaclePoints_.clear();
}

void EnemyAIController::ClearCurrentPath() {
	currentPathPoints_.clear();
	currentPathPointIndex_ = 0;
}

bool EnemyAIController::HasFailedCurrentObstaclePoint(const CoverPoint& coverPoint) const {
	for (const physx::PxVec3& failedPoint : failedCurrentObstaclePoints_) {
		if (AreCoverPointsEquivalent(failedPoint, coverPoint.position)) {
			return true;
		}
	}

	return false;
}

void EnemyAIController::MarkCurrentCoverPointFailed() {
	if (!hasCoverPoint_ || HasFailedCurrentObstaclePoint(currentCoverPoint_)) {
		return;
	}

	failedCurrentObstaclePoints_.push_back(currentCoverPoint_.position);
}

bool EnemyAIController::BuildSequentialObstacleRoute(
	const CoverPoint& startPoint,
	const CoverPoint& destinationPoint,
	std::vector<physx::PxVec3>& routePoints,
	float& routeLength
) const {
	routePoints.clear();
	routeLength = 0.0f;

	if (!coverPoints_
		|| startPoint.obstacle == nullptr
		|| destinationPoint.obstacle == nullptr
		|| startPoint.obstacle != destinationPoint.obstacle
		|| startPoint.obstaclePointIndex == destinationPoint.obstaclePointIndex) {
		return false;
	}

	std::vector<const CoverPoint*> obstaclePoints;
	for (const CoverPoint& coverPoint : *coverPoints_) {
		if (coverPoint.obstacle == startPoint.obstacle) {
			obstaclePoints.push_back(&coverPoint);
		}
	}

	if (obstaclePoints.size() < 2) {
		return false;
	}

	std::sort(
		obstaclePoints.begin(),
		obstaclePoints.end(),
		[](const CoverPoint* left, const CoverPoint* right) {
			return left->obstaclePointIndex < right->obstaclePointIndex;
		}
	);

	const auto findPointByIndex = [&](int obstaclePointIndex) -> const CoverPoint* {
		for (const CoverPoint* coverPoint : obstaclePoints) {
			if (coverPoint->obstaclePointIndex == obstaclePointIndex) {
				return coverPoint;
			}
		}

		return nullptr;
	};

	const auto buildRouteInDirection = [&](int directionStep, std::vector<physx::PxVec3>& candidateRoutePoints, float& candidateRouteLength) {
		candidateRoutePoints.clear();
		candidateRouteLength = 0.0f;

		physx::PxVec3 previousPoint = startPoint.position;
		int currentPointIndex = startPoint.obstaclePointIndex;
			for (int stepCount = 0; stepCount < GameConstants::ArenaConfig::CoverPointCountPerObstacle - 1; ++stepCount) {
				currentPointIndex =
					(currentPointIndex + directionStep + GameConstants::ArenaConfig::CoverPointCountPerObstacle)
					% GameConstants::ArenaConfig::CoverPointCountPerObstacle;
			const CoverPoint* nextPoint = findPointByIndex(currentPointIndex);
			if (!nextPoint || !IsPathClear(previousPoint, nextPoint->position)) {
				return false;
			}

			candidateRouteLength += std::sqrt(GetPlanarDistanceSquared(previousPoint, nextPoint->position));
			candidateRoutePoints.push_back(nextPoint->position);
			if (currentPointIndex == destinationPoint.obstaclePointIndex) {
				return true;
			}

			previousPoint = nextPoint->position;
		}

		return false;
	};

	std::vector<physx::PxVec3> clockwiseRoutePoints;
	std::vector<physx::PxVec3> counterClockwiseRoutePoints;
	float clockwiseRouteLength = 0.0f;
	float counterClockwiseRouteLength = 0.0f;
	const bool hasClockwiseRoute = buildRouteInDirection(1, clockwiseRoutePoints, clockwiseRouteLength);
	const bool hasCounterClockwiseRoute = buildRouteInDirection(-1, counterClockwiseRoutePoints, counterClockwiseRouteLength);
	if (!hasClockwiseRoute && !hasCounterClockwiseRoute) {
		return false;
	}

	if (!hasCounterClockwiseRoute || (hasClockwiseRoute && clockwiseRouteLength <= counterClockwiseRouteLength)) {
		routePoints = std::move(clockwiseRoutePoints);
		routeLength = clockwiseRouteLength;
		return true;
	}

	routePoints = std::move(counterClockwiseRoutePoints);
	routeLength = counterClockwiseRouteLength;
	return true;
}

bool EnemyAIController::TrySelectAnotherPointOnCurrentObstacle(const physx::PxVec3& playerEye) {
	if (!enemy_ || !coverPoints_ || !hasCoverPoint_ || currentCoverPoint_.obstacle == nullptr) {
		return false;
	}

	CoverPoint bestProtectedCover;
	CoverPoint bestReachableCover;
	std::vector<physx::PxVec3> bestProtectedRoutePoints;
	std::vector<physx::PxVec3> bestReachableRoutePoints;
	bool foundProtectedCover = false;
	bool foundReachableCover = false;
	float bestProtectedRouteLength = std::numeric_limits<float>::max();
	float bestReachableRouteLength = std::numeric_limits<float>::max();

	for (const CoverPoint& coverPoint : *coverPoints_) {
		if (coverPoint.obstacle != currentCoverPoint_.obstacle) {
			continue;
		}

		if (AreCoverPointsEquivalent(coverPoint.position, currentCoverPoint_.position) || HasFailedCurrentObstaclePoint(coverPoint)) {
			continue;
		}

		std::vector<physx::PxVec3> routePoints;
		float routeLength = 0.0f;
		if (!BuildSequentialObstacleRoute(currentCoverPoint_, coverPoint, routePoints, routeLength)) {
			continue;
		}

		if (IsCoverProtected(coverPoint, playerEye)) {
			if (!foundProtectedCover || routeLength < bestProtectedRouteLength) {
				bestProtectedRouteLength = routeLength;
				bestProtectedCover = coverPoint;
				bestProtectedRoutePoints = std::move(routePoints);
				foundProtectedCover = true;
			}
		}
		else if (!foundReachableCover || routeLength < bestReachableRouteLength) {
			bestReachableRouteLength = routeLength;
			bestReachableCover = coverPoint;
			bestReachableRoutePoints = std::move(routePoints);
			foundReachableCover = true;
		}
	}

	if (!foundProtectedCover && !foundReachableCover) {
		return false;
	}

	const CoverPoint& nextCoverPoint = foundProtectedCover ? bestProtectedCover : bestReachableCover;
	std::vector<physx::PxVec3>& nextRoutePoints = foundProtectedCover ? bestProtectedRoutePoints : bestReachableRoutePoints;
	state_ = State::SeekingCover;
	hasTarget_ = true;
	hasCoverPoint_ = true;
	currentCoverPoint_ = nextCoverPoint;
	currentTarget_ = nextCoverPoint.position;
	currentPathPoints_ = std::move(nextRoutePoints);
	currentPathPointIndex_ = 0;
	return true;
}

void EnemyAIController::EvaluateBehavior(const physx::PxVec3& playerEye, const physx::PxRigidActor* obstacleToAvoid) {
	CoverPoint bestCover;
	if (FindBestCover(playerEye, bestCover, obstacleToAvoid)) {
		if (!hasCoverPoint_ || currentCoverPoint_.obstacle != bestCover.obstacle) {
			ResetCurrentObstacleTraversal();
			currentObstacleHasProvidedCover_ = false;
		}

		state_ = State::SeekingCover;
		hasTarget_ = true;
		hasCoverPoint_ = true;
		currentCoverPoint_ = bestCover;
		currentTarget_ = bestCover.position;
		ClearCurrentPath();
		return;
	}

	physx::PxVec3 bestFleeTarget(0.0f);
	state_ = State::Fleeing;
	if (FindBestFleeTarget(playerEye, bestFleeTarget)) {
		ResetCurrentObstacleTraversal();
		hasTarget_ = true;
		hasCoverPoint_ = false;
		currentObstacleHasProvidedCover_ = false;
		currentCoverPoint_ = CoverPoint{};
		currentTarget_ = bestFleeTarget;
		ClearCurrentPath();
		return;
	}

	ClearTarget();
	enemy_->StopPlanarMovement();
}

void EnemyAIController::AdvanceCurrentPath(const physx::PxVec3& enemyPosition) {
	while (currentPathPointIndex_ < currentPathPoints_.size()
		&& GetPlanarDistanceSquared(enemyPosition, currentPathPoints_[currentPathPointIndex_])
			<= GameConstants::AIConfig::ArrivalThreshold * GameConstants::AIConfig::ArrivalThreshold) {
		++currentPathPointIndex_;
	}
}

physx::PxVec3 EnemyAIController::GetActiveTarget() const {
	if (currentPathPointIndex_ < currentPathPoints_.size()) {
		return currentPathPoints_[currentPathPointIndex_];
	}

	return currentTarget_;
}

physx::PxVec3 EnemyAIController::GetCoverProbePosition(const CoverPoint& coverPoint) const {
	if (coverPoint.outwardNormal.magnitudeSquared() < GameConstants::AIConfig::DirectionEpsilon) {
		return coverPoint.position;
	}

	// Check LOS against the body edge that should be tucked toward the obstacle,
	// not just against the target point at the capsule center.
	return coverPoint.position - coverPoint.outwardNormal.getNormalized() * GameConstants::AIConfig::VisibilityProbeInset;
}

physx::PxVec3 EnemyAIController::GetVisibilityProbePosition(const physx::PxVec3& position) const {
	if (state_ == State::SeekingCover && hasCoverPoint_) {
		const physx::PxVec3 probeOffset = GetCoverProbePosition(currentCoverPoint_) - currentCoverPoint_.position;
		return position + probeOffset;
	}

	return position;
}

void EnemyAIController::UpdateMovement() {
	if (!enemy_ || !enemy_->CanUseAI()) {
		return;
	}

	if (!hasTarget_) {
		enemy_->StopPlanarMovement();
		return;
	}

	const physx::PxVec3 enemyPosition = enemy_->GetPosition();
	physx::PxVec3 direction = MakePlanar(GetActiveTarget() - enemyPosition);
	if (direction.magnitudeSquared()
		<= GameConstants::AIConfig::ArrivalThreshold * GameConstants::AIConfig::ArrivalThreshold) {
		enemy_->StopPlanarMovement();
		return;
	}

	enemy_->SetPlanarVelocity(direction.getNormalized() * GameConstants::AIConfig::MoveSpeed);
}

bool EnemyAIController::FindBestCover(
	const physx::PxVec3& playerEye,
	CoverPoint& bestCover,
	const physx::PxRigidActor* obstacleToAvoid
) const {
	if (!enemy_ || !coverPoints_) {
		return false;
	}

	const physx::PxVec3 enemyPosition = enemy_->GetPosition();
	const float maxDistanceSquared =
		GameConstants::AIConfig::ViewSphereRadius * GameConstants::AIConfig::ViewSphereRadius;
	float bestEnemyDistanceSquared = std::numeric_limits<float>::max();
	float bestPlayerDistanceSquared = -1.0f;
	bool foundCover = false;

	for (const CoverPoint& coverPoint : *coverPoints_) {
		if (obstacleToAvoid != nullptr && coverPoint.obstacle == obstacleToAvoid) {
			continue;
		}

		const float enemyDistanceSquared = GetPlanarDistanceSquared(enemyPosition, coverPoint.position);
		if (enemyDistanceSquared > maxDistanceSquared) {
			continue;
		}

		if (!IsPathClear(enemyPosition, coverPoint.position)) {
			continue;
		}

		if (!IsCoverProtected(coverPoint, playerEye)) {
			continue;
		}

		const float playerDistanceSquared = (playerEye - coverPoint.position).magnitudeSquared();
		if (!foundCover
			|| enemyDistanceSquared < bestEnemyDistanceSquared
			|| (std::abs(enemyDistanceSquared - bestEnemyDistanceSquared) < GameConstants::AIConfig::CoverPointComparisonEpsilon
				&& playerDistanceSquared > bestPlayerDistanceSquared)) {
			bestEnemyDistanceSquared = enemyDistanceSquared;
			bestPlayerDistanceSquared = playerDistanceSquared;
			bestCover = coverPoint;
			foundCover = true;
		}
	}

	return foundCover;
}

bool EnemyAIController::FindBestFleeTarget(const physx::PxVec3& playerEye, physx::PxVec3& bestTarget) const {
	if (!enemy_) {
		return false;
	}

	const physx::PxVec3 enemyPosition = enemy_->GetPosition();
	physx::PxVec3 oppositeDirection = MakePlanar(enemyPosition - playerEye);
	if (oppositeDirection.magnitudeSquared() < GameConstants::AIConfig::DirectionEpsilon) {
		oppositeDirection = physx::PxVec3(0.0f, 0.0f, 1.0f);
	}
	oppositeDirection = oppositeDirection.getNormalized();

	const physx::PxVec3 rightDirection(-oppositeDirection.z, 0.0f, oppositeDirection.x);

	bool foundTarget = false;
	float bestPlayerDistanceSquared = -1.0f;

	for (int sampleIndex = 0; sampleIndex < GameConstants::AIConfig::FleeSampleCount; ++sampleIndex) {
		const float angle =
			-physx::PxHalfPi
			+ static_cast<float>(sampleIndex) * (physx::PxPi / static_cast<float>(GameConstants::AIConfig::FleeSampleCount - 1));
		const physx::PxVec3 sampleDirection =
			oppositeDirection * static_cast<float>(std::cos(angle))
			+ rightDirection * static_cast<float>(std::sin(angle));

		physx::PxVec3 candidate =
			enemyPosition + sampleDirection.getNormalized() * GameConstants::AIConfig::FleeSampleRadius;
		candidate = ClampToArena(candidate);

		if (GetPlanarDistanceSquared(enemyPosition, candidate)
			<= GameConstants::AIConfig::ArrivalThreshold * GameConstants::AIConfig::ArrivalThreshold) {
			continue;
		}

		if (!IsPathClear(enemyPosition, candidate)) {
			continue;
		}

		const float playerDistanceSquared = (playerEye - candidate).magnitudeSquared();
		if (!foundTarget || playerDistanceSquared > bestPlayerDistanceSquared) {
			bestPlayerDistanceSquared = playerDistanceSquared;
			bestTarget = candidate;
			foundTarget = true;
		}
	}

	return foundTarget;
}

bool EnemyAIController::IsPathClear(const physx::PxVec3& start, const physx::PxVec3& destination) const {
	const physx::PxVec3 ray = destination - start;
	const float distance = ray.magnitude();
	if (distance < GameConstants::AIConfig::RaycastEpsilon) {
		return true;
	}

	physx::PxRaycastHit hit;
	return !RaycastIgnoringEnemy(start, ray.getNormalized(), distance - GameConstants::AIConfig::RaycastEpsilon, hit);
}

bool EnemyAIController::IsPositionHiddenFromPlayer(const physx::PxVec3& position, const physx::PxVec3& playerEye) const {
	const physx::PxVec3 probePosition = GetVisibilityProbePosition(position);
	const physx::PxVec3 ray = probePosition - playerEye;
	const float distance = ray.magnitude();
	if (distance < GameConstants::AIConfig::RaycastEpsilon) {
		return false;
	}

	physx::PxRaycastHit hit;
	return RaycastIgnoringEnemy(
		playerEye,
		ray.getNormalized(),
		distance - GameConstants::AIConfig::RaycastEpsilon,
		hit
	);
}

bool EnemyAIController::IsCoverProtected(const CoverPoint& coverPoint, const physx::PxVec3& playerEye) const {
	const physx::PxVec3 probePosition = GetCoverProbePosition(coverPoint);
	const physx::PxVec3 planarToPlayer = MakePlanar(playerEye - probePosition);
	if (planarToPlayer.magnitudeSquared()
		< GameConstants::AIConfig::RaycastEpsilon * GameConstants::AIConfig::RaycastEpsilon) {
		return false;
	}

	if (planarToPlayer.dot(coverPoint.outwardNormal) >= 0.0f) {
		return false;
	}

	const physx::PxVec3 ray = probePosition - playerEye;
	const float distance = ray.magnitude();
	if (distance < GameConstants::AIConfig::RaycastEpsilon) {
		return false;
	}

	physx::PxRaycastHit hit;
	if (!RaycastIgnoringEnemy(
		playerEye,
		ray.getNormalized(),
		distance - GameConstants::AIConfig::RaycastEpsilon,
		hit
	)) {
		return false;
	}

	return hit.distance < distance - GameConstants::AIConfig::RaycastEpsilon;
}

bool EnemyAIController::RaycastIgnoringEnemy(
	const physx::PxVec3& start,
	const physx::PxVec3& direction,
	float maxDistance,
	physx::PxRaycastHit& hit
) const {
	if (!physicsEngine_
		|| !enemy_
		|| maxDistance <= 0.0f
		|| direction.magnitudeSquared() < GameConstants::AIConfig::DirectionEpsilon) {
		return false;
	}

	physx::PxVec3 origin = start;
	float remainingDistance = maxDistance;
	const physx::PxVec3 unitDirection = direction.getNormalized();

	for (int passIndex = 0;
		passIndex < GameConstants::AIConfig::MaxSelfHitPasses && remainingDistance > GameConstants::AIConfig::RaycastEpsilon;
		++passIndex) {
		physx::PxRaycastBuffer hitBuffer;
		if (!physicsEngine_->Raycast(origin, unitDirection, remainingDistance, hitBuffer) || !hitBuffer.hasBlock) {
			return false;
		}

		if (!enemy_->OwnsActor(hitBuffer.block.actor)) {
			hit = hitBuffer.block;
			return true;
		}

		const float advanceDistance = hitBuffer.block.distance + GameConstants::AIConfig::RaycastEpsilon;
		origin += unitDirection * advanceDistance;
		remainingDistance -= advanceDistance;
	}

	return false;
}

void EnemyAIController::AppendCircle(
	std::vector<DebugLine>& out,
	const physx::PxVec3& center,
	const physx::PxVec3& axisA,
	const physx::PxVec3& axisB,
	float radius,
	const physx::PxVec3& color
) const {
	for (int segmentIndex = 0; segmentIndex < GameConstants::AIConfig::DebugCircleSegments; ++segmentIndex) {
		const float angle0 =
			2.0f * physx::PxPi * static_cast<float>(segmentIndex) / static_cast<float>(GameConstants::AIConfig::DebugCircleSegments);
		const float angle1 =
			2.0f * physx::PxPi * static_cast<float>(segmentIndex + 1) / static_cast<float>(GameConstants::AIConfig::DebugCircleSegments);

		const physx::PxVec3 point0 =
			center
			+ axisA * (radius * static_cast<float>(std::cos(angle0)))
			+ axisB * (radius * static_cast<float>(std::sin(angle0)));
		const physx::PxVec3 point1 =
			center
			+ axisA * (radius * static_cast<float>(std::cos(angle1)))
			+ axisB * (radius * static_cast<float>(std::sin(angle1)));

		out.push_back(DebugLine{ point0, point1, color });
	}
}

physx::PxVec3 EnemyAIController::GetStateColor() const {
	switch (state_) {
	case State::SeekingCover:
		return GameConstants::AIConfig::SeekingCoverColor;
	case State::Fleeing:
		return GameConstants::AIConfig::FleeingColor;
	case State::Idle:
	default:
		return GameConstants::AIConfig::IdleColor;
	}
}
