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
		repathTimer_ = 0.0f;
		ClearTarget();
		return;
	}

	if (state_ == State::Disabled) {
		state_ = State::Idle;
	}

	const physx::PxVec3 enemyPosition = enemy_->GetPosition();
	physx::PxVec3 alignedPlayerPosition = playerEye;
	alignedPlayerPosition.y = enemyPosition.y;
	AdvanceCurrentPath(enemyPosition);

	const bool seesPlayer = CanSeePosition(
		enemyPosition,
		alignedPlayerPosition,
		GameConstants::AIConfig::ViewSphereRadius
	);
	if (!seesPlayer) {
		state_ = State::Idle;
		repathTimer_ = 0.0f;
		ClearTarget();
		enemy_->StopPlanarMovement();
		return;
	}

	const bool reachedTarget =
		hasTarget_
		&& GetPlanarDistanceSquared(enemyPosition, currentTarget_)
			<= GameConstants::AIConfig::ArrivalThreshold * GameConstants::AIConfig::ArrivalThreshold;

	if (state_ == State::SeekingCover && hasCoverPoint_ && !reachedTarget) {
		const bool coverPointIsStillSafe =
			!CanSeePosition(
				alignedPlayerPosition,
				currentCoverPoint_.position,
				GameConstants::AIConfig::ViewSphereRadius
			);
		if (coverPointIsStillSafe) {
			UpdateMovement();
			return;
		}
	}

	repathTimer_ -= elapsedTime;
	if (state_ == State::Fleeing
		&& hasTarget_
		&& !reachedTarget
		&& IsPathClear(enemyPosition, currentTarget_)
		&& repathTimer_ > 0.0f) {
		UpdateMovement();
		return;
	}

	SelectBehavior(enemyPosition, alignedPlayerPosition);
	repathTimer_ = GameConstants::AIConfig::RepathInterval;
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

	out.push_back(DebugLine{ center, GetActiveTarget(), color });
	out.push_back(DebugLine{
		GetActiveTarget() + physx::PxVec3(-GameConstants::AIConfig::DebugMarkerHalfSize, 0.0f, 0.0f),
		GetActiveTarget() + physx::PxVec3(GameConstants::AIConfig::DebugMarkerHalfSize, 0.0f, 0.0f),
		color
	});
	out.push_back(DebugLine{
		GetActiveTarget() + physx::PxVec3(0.0f, -GameConstants::AIConfig::DebugMarkerHalfSize, 0.0f),
		GetActiveTarget() + physx::PxVec3(0.0f, GameConstants::AIConfig::DebugMarkerHalfSize, 0.0f),
		color
	});
	out.push_back(DebugLine{
		GetActiveTarget() + physx::PxVec3(0.0f, 0.0f, -GameConstants::AIConfig::DebugMarkerHalfSize),
		GetActiveTarget() + physx::PxVec3(0.0f, 0.0f, GameConstants::AIConfig::DebugMarkerHalfSize),
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

void EnemyAIController::ClearTarget() {
	hasTarget_ = false;
	hasCoverPoint_ = false;
	currentCoverPoint_ = CoverPoint{};
	currentTarget_ = physx::PxVec3(0.0f);
	ClearCurrentPath();
}

void EnemyAIController::ClearCurrentPath() {
	currentPathPoints_.clear();
	currentPathPointIndex_ = 0;
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

void EnemyAIController::SelectBehavior(const physx::PxVec3& enemyPosition, const physx::PxVec3& playerPosition) {
	CoverPoint bestCover;
	std::vector<physx::PxVec3> bestCoverRoute;
	if (FindBestCoverPoint(enemyPosition, playerPosition, bestCover, bestCoverRoute)) {
		state_ = State::SeekingCover;
		hasTarget_ = true;
		hasCoverPoint_ = true;
		currentCoverPoint_ = bestCover;
		currentTarget_ = bestCover.position;
		currentPathPoints_ = std::move(bestCoverRoute);
		currentPathPointIndex_ = 0;
		return;
	}

	physx::PxVec3 bestFleeTarget(0.0f);
	if (FindBestFleeTarget(enemyPosition, playerPosition, bestFleeTarget)) {
		state_ = State::Fleeing;
		hasTarget_ = true;
		hasCoverPoint_ = false;
		currentCoverPoint_ = CoverPoint{};
		currentTarget_ = bestFleeTarget;
		ClearCurrentPath();
		return;
	}

	state_ = State::Idle;
	ClearTarget();
	enemy_->StopPlanarMovement();
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

bool EnemyAIController::FindBestCoverPoint(
	const physx::PxVec3& enemyPosition,
	const physx::PxVec3& playerPosition,
	CoverPoint& bestCover,
	std::vector<physx::PxVec3>& bestRoute
) const {
	if (!coverPoints_) {
		return false;
	}

	const float maxDistanceSquared =
		GameConstants::AIConfig::ViewSphereRadius * GameConstants::AIConfig::ViewSphereRadius;
	float bestRouteLength = std::numeric_limits<float>::max();
	bool foundCover = false;

	for (const CoverPoint& coverPoint : *coverPoints_) {
		const float enemyDistanceSquared = GetPlanarDistanceSquared(enemyPosition, coverPoint.position);
		if (enemyDistanceSquared > maxDistanceSquared) {
			continue;
		}

		if (CanSeePosition(
			playerPosition,
			coverPoint.position,
			GameConstants::AIConfig::ViewSphereRadius
		)) {
			continue;
		}

		std::vector<physx::PxVec3> routePoints;
		float routeLength = 0.0f;
		if (!BuildPathToCoverPoint(enemyPosition, coverPoint, routePoints, routeLength)) {
			continue;
		}

		if (!foundCover || routeLength < bestRouteLength) {
			bestRouteLength = routeLength;
			bestCover = coverPoint;
			bestRoute = std::move(routePoints);
			foundCover = true;
		}
	}

	return foundCover;
}

bool EnemyAIController::FindBestFleeTarget(
	const physx::PxVec3& enemyPosition,
	const physx::PxVec3& playerPosition,
	physx::PxVec3& bestTarget
) const {
	physx::PxVec3 oppositeDirection = MakePlanar(enemyPosition - playerPosition);
	if (oppositeDirection.magnitudeSquared() < GameConstants::AIConfig::DirectionEpsilon) {
		oppositeDirection = physx::PxVec3(0.0f, 0.0f, 1.0f);
	}
	oppositeDirection = oppositeDirection.getNormalized();

	const physx::PxVec3 rightDirection(-oppositeDirection.z, 0.0f, oppositeDirection.x);

	bool foundHiddenTarget = false;
	bool foundAnyTarget = false;
	float bestHiddenDistanceSquared = -1.0f;
	float bestAnyDistanceSquared = -1.0f;
	physx::PxVec3 bestAnyTarget(0.0f);

	for (int sampleIndex = 0; sampleIndex < GameConstants::AIConfig::FleeSampleCount; ++sampleIndex) {
		const float angle =
			-physx::PxHalfPi
			+ static_cast<float>(sampleIndex)
				* (physx::PxPi / static_cast<float>(GameConstants::AIConfig::FleeSampleCount - 1));
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

		const float playerDistanceSquared = GetPlanarDistanceSquared(playerPosition, candidate);
		const bool hiddenFromPlayer =
			!CanSeePosition(
				playerPosition,
				candidate,
				GameConstants::AIConfig::ViewSphereRadius
			);

		if (hiddenFromPlayer && (!foundHiddenTarget || playerDistanceSquared > bestHiddenDistanceSquared)) {
			bestHiddenDistanceSquared = playerDistanceSquared;
			bestTarget = candidate;
			foundHiddenTarget = true;
		}

		if (!foundAnyTarget || playerDistanceSquared > bestAnyDistanceSquared) {
			bestAnyDistanceSquared = playerDistanceSquared;
			bestAnyTarget = candidate;
			foundAnyTarget = true;
		}
	}

	if (foundHiddenTarget) {
		return true;
	}

	if (foundAnyTarget) {
		bestTarget = bestAnyTarget;
		return true;
	}

	return false;
}

bool EnemyAIController::BuildPathToCoverPoint(
	const physx::PxVec3& start,
	const CoverPoint& coverPoint,
	std::vector<physx::PxVec3>& routePoints,
	float& routeLength
) const {
	routePoints.clear();
	routeLength = 0.0f;

	if (IsPathClear(start, coverPoint.position)) {
		routeLength = std::sqrt(GetPlanarDistanceSquared(start, coverPoint.position));
		return true;
	}

	std::array<physx::PxVec3, 4> corners;
	if (!GetObstacleRouteCorners(coverPoint.obstacle, corners)) {
		return false;
	}

	bool foundRoute = false;
	float bestRouteLength = std::numeric_limits<float>::max();
	std::vector<physx::PxVec3> bestRoutePoints;

	for (int startCornerIndex = 0; startCornerIndex < 4; ++startCornerIndex) {
		if (!IsPathClear(start, corners[startCornerIndex])) {
			continue;
		}

		for (int endCornerIndex = 0; endCornerIndex < 4; ++endCornerIndex) {
			if (!IsPathClear(corners[endCornerIndex], coverPoint.position)) {
				continue;
			}

			for (int directionStep : { 1, -1 }) {
				std::vector<physx::PxVec3> candidateRoutePoints;
				candidateRoutePoints.push_back(corners[startCornerIndex]);

				float candidateRouteLength =
					std::sqrt(GetPlanarDistanceSquared(start, corners[startCornerIndex]));
				int currentCornerIndex = startCornerIndex;
				bool validRoute = true;

				for (int iteration = 0; iteration < 4 && currentCornerIndex != endCornerIndex; ++iteration) {
					const int nextCornerIndex = (currentCornerIndex + directionStep + 4) % 4;
					if (!IsPathClear(corners[currentCornerIndex], corners[nextCornerIndex])) {
						validRoute = false;
						break;
					}

					candidateRouteLength += std::sqrt(
						GetPlanarDistanceSquared(corners[currentCornerIndex], corners[nextCornerIndex])
					);
					candidateRoutePoints.push_back(corners[nextCornerIndex]);
					currentCornerIndex = nextCornerIndex;
				}

				if (!validRoute || currentCornerIndex != endCornerIndex) {
					continue;
				}

				candidateRouteLength += std::sqrt(
					GetPlanarDistanceSquared(corners[endCornerIndex], coverPoint.position)
				);
				if (!foundRoute || candidateRouteLength < bestRouteLength) {
					bestRouteLength = candidateRouteLength;
					bestRoutePoints = std::move(candidateRoutePoints);
					foundRoute = true;
				}
			}
		}
	}

	if (!foundRoute) {
		return false;
	}

	routePoints = std::move(bestRoutePoints);
	routeLength = bestRouteLength;
	return true;
}

bool EnemyAIController::GetObstacleRouteCorners(
	const physx::PxRigidActor* obstacle,
	std::array<physx::PxVec3, 4>& corners
) const {
	if (!obstacle || obstacle->getNbShapes() == 0) {
		return false;
	}

	physx::PxShape* shape = nullptr;
	obstacle->getShapes(&shape, 1);
	if (!shape) {
		return false;
	}

	const physx::PxGeometryHolder geometry = shape->getGeometry();
	if (geometry.getType() != physx::PxGeometryType::eBOX) {
		return false;
	}

	const physx::PxBoxGeometry boxGeometry = geometry.box();
	const physx::PxTransform obstaclePose = obstacle->getGlobalPose() * shape->getLocalPose();
	const physx::PxVec3 axisX = obstaclePose.q.rotate(physx::PxVec3(1.0f, 0.0f, 0.0f));
	const physx::PxVec3 axisZ = obstaclePose.q.rotate(physx::PxVec3(0.0f, 0.0f, 1.0f));
	const float halfX = boxGeometry.halfExtents.x + GameConstants::ArenaConfig::CoverPointOffset;
	const float halfZ = boxGeometry.halfExtents.z + GameConstants::ArenaConfig::CoverPointOffset;
	physx::PxVec3 center = obstaclePose.p;
	center.y = GameConstants::EnemyConfig::StandingHeight;

	corners[0] = center + axisX * halfX + axisZ * halfZ;
	corners[1] = center - axisX * halfX + axisZ * halfZ;
	corners[2] = center - axisX * halfX - axisZ * halfZ;
	corners[3] = center + axisX * halfX - axisZ * halfZ;
	return true;
}

bool EnemyAIController::CanSeePosition(
	const physx::PxVec3& observerPosition,
	const physx::PxVec3& targetPosition,
	float viewRadius
) const {
	if (GetPlanarDistanceSquared(observerPosition, targetPosition) > viewRadius * viewRadius) {
		return false;
	}

	const physx::PxVec3 ray = targetPosition - observerPosition;
	const float distance = ray.magnitude();
	if (distance < GameConstants::AIConfig::RaycastEpsilon) {
		return true;
	}

	physx::PxRaycastHit hit;
	return !RaycastIgnoringEnemy(
		observerPosition,
		ray.getNormalized(),
		distance - GameConstants::AIConfig::RaycastEpsilon,
		hit
	);
}

bool EnemyAIController::IsPathClear(const physx::PxVec3& start, const physx::PxVec3& destination) const {
	const physx::PxVec3 ray = destination - start;
	const float distance = ray.magnitude();
	if (distance < GameConstants::AIConfig::RaycastEpsilon) {
		return true;
	}

	physx::PxRaycastHit hit;
	return !RaycastIgnoringEnemy(
		start,
		ray.getNormalized(),
		distance - GameConstants::AIConfig::RaycastEpsilon,
		hit
	);
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
			2.0f * physx::PxPi * static_cast<float>(segmentIndex)
			/ static_cast<float>(GameConstants::AIConfig::DebugCircleSegments);
		const float angle1 =
			2.0f * physx::PxPi * static_cast<float>(segmentIndex + 1)
			/ static_cast<float>(GameConstants::AIConfig::DebugCircleSegments);

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
