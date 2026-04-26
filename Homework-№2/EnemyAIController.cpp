#include "EnemyAIController.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

#include "Enemy.h"
#include "PhysicsEngine.h"

namespace {
const float kViewSphereRadius = 14.0f;
const float kMoveSpeed = 4.5f;
const float kRepathInterval = 0.25f;
const float kArrivalThreshold = 0.6f;
const float kFleeSampleRadius = 6.0f;
const float kRaycastEpsilon = 0.05f;
const float kDebugMarkerHalfSize = 0.25f;
const float kArenaClampMin = -16.0f;
const float kArenaClampMax = 16.0f;
const int kDebugCircleSegments = 24;
const int kMaxSelfHitPasses = 8;

const physx::PxVec3 kIdleColor(0.25f, 0.95f, 0.35f);
const physx::PxVec3 kSeekingCoverColor(0.25f, 0.60f, 1.0f);
const physx::PxVec3 kFleeingColor(1.0f, 0.60f, 0.15f);
}

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
	const bool isPlayerInsideView = (playerEye - enemyPosition).magnitudeSquared() <= kViewSphereRadius * kViewSphereRadius;
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

	if (hasTarget_ && GetPlanarDistanceSquared(enemyPosition, currentTarget_) <= kArrivalThreshold * kArrivalThreshold) {
		enemy_->StopPlanarMovement();
		if (state_ == State::Fleeing) {
			repathTimer_ = 0.0f;
		}
	}

	repathTimer_ -= elapsedTime;
	if (repathTimer_ <= 0.0f || state_ == State::Idle) {
		EvaluateBehavior(playerEye);
		repathTimer_ = kRepathInterval;
	}

	UpdateMovement();
}

void EnemyAIController::AppendDebugLines(std::vector<DebugLine>& out) const {
	if (!enemy_ || !enemy_->CanUseAI() || state_ == State::Disabled) {
		return;
	}

	const physx::PxVec3 color = GetStateColor();
	const physx::PxVec3 center = enemy_->GetPosition();

	AppendCircle(out, center, physx::PxVec3(1.0f, 0.0f, 0.0f), physx::PxVec3(0.0f, 1.0f, 0.0f), kViewSphereRadius, color);
	AppendCircle(out, center, physx::PxVec3(1.0f, 0.0f, 0.0f), physx::PxVec3(0.0f, 0.0f, 1.0f), kViewSphereRadius, color);
	AppendCircle(out, center, physx::PxVec3(0.0f, 1.0f, 0.0f), physx::PxVec3(0.0f, 0.0f, 1.0f), kViewSphereRadius, color);

	if (!hasTarget_ || (state_ != State::SeekingCover && state_ != State::Fleeing)) {
		return;
	}

	out.push_back(DebugLine{ center, currentTarget_, color });
	out.push_back(DebugLine{ currentTarget_ + physx::PxVec3(-kDebugMarkerHalfSize, 0.0f, 0.0f), currentTarget_ + physx::PxVec3(kDebugMarkerHalfSize, 0.0f, 0.0f), color });
	out.push_back(DebugLine{ currentTarget_ + physx::PxVec3(0.0f, -kDebugMarkerHalfSize, 0.0f), currentTarget_ + physx::PxVec3(0.0f, kDebugMarkerHalfSize, 0.0f), color });
	out.push_back(DebugLine{ currentTarget_ + physx::PxVec3(0.0f, 0.0f, -kDebugMarkerHalfSize), currentTarget_ + physx::PxVec3(0.0f, 0.0f, kDebugMarkerHalfSize), color });
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
		std::clamp(position.x, kArenaClampMin, kArenaClampMax),
		position.y,
		std::clamp(position.z, kArenaClampMin, kArenaClampMax)
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
	currentTarget_ = physx::PxVec3(0.0f);
}

void EnemyAIController::EvaluateBehavior(const physx::PxVec3& playerEye) {
	CoverPoint bestCover;
	if (FindBestCover(playerEye, bestCover)) {
		state_ = State::SeekingCover;
		hasTarget_ = true;
		currentTarget_ = bestCover.position;
		return;
	}

	physx::PxVec3 bestFleeTarget(0.0f);
	state_ = State::Fleeing;
	if (FindBestFleeTarget(playerEye, bestFleeTarget)) {
		hasTarget_ = true;
		currentTarget_ = bestFleeTarget;
		return;
	}

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
	physx::PxVec3 direction = MakePlanar(currentTarget_ - enemyPosition);
	if (direction.magnitudeSquared() <= kArrivalThreshold * kArrivalThreshold) {
		enemy_->StopPlanarMovement();
		return;
	}

	enemy_->SetPlanarVelocity(direction.getNormalized() * kMoveSpeed);
}

bool EnemyAIController::FindBestCover(const physx::PxVec3& playerEye, CoverPoint& bestCover) const {
	if (!enemy_ || !coverPoints_) {
		return false;
	}

	const physx::PxVec3 enemyPosition = enemy_->GetPosition();
	const float maxDistanceSquared = kViewSphereRadius * kViewSphereRadius;
	float bestEnemyDistanceSquared = std::numeric_limits<float>::max();
	float bestPlayerDistanceSquared = -1.0f;
	bool foundCover = false;

	for (const CoverPoint& coverPoint : *coverPoints_) {
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
			|| (std::abs(enemyDistanceSquared - bestEnemyDistanceSquared) < 1e-4f && playerDistanceSquared > bestPlayerDistanceSquared)) {
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
	if (oppositeDirection.magnitudeSquared() < 1e-4f) {
		oppositeDirection = physx::PxVec3(0.0f, 0.0f, 1.0f);
	}
	oppositeDirection = oppositeDirection.getNormalized();

	const physx::PxVec3 rightDirection(-oppositeDirection.z, 0.0f, oppositeDirection.x);

	bool foundTarget = false;
	float bestPlayerDistanceSquared = -1.0f;

	for (int sampleIndex = 0; sampleIndex < 8; ++sampleIndex) {
		const float angle = -physx::PxHalfPi + static_cast<float>(sampleIndex) * (physx::PxPi / 7.0f);
		const physx::PxVec3 sampleDirection =
			oppositeDirection * static_cast<float>(std::cos(angle))
			+ rightDirection * static_cast<float>(std::sin(angle));

		physx::PxVec3 candidate = enemyPosition + sampleDirection.getNormalized() * kFleeSampleRadius;
		candidate = ClampToArena(candidate);

		if (GetPlanarDistanceSquared(enemyPosition, candidate) <= kArrivalThreshold * kArrivalThreshold) {
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
	if (distance < kRaycastEpsilon) {
		return true;
	}

	physx::PxRaycastHit hit;
	return !RaycastIgnoringEnemy(start, ray.getNormalized(), distance - kRaycastEpsilon, hit);
}

bool EnemyAIController::IsCoverProtected(const CoverPoint& coverPoint, const physx::PxVec3& playerEye) const {
	const physx::PxVec3 planarToPlayer = MakePlanar(playerEye - coverPoint.position);
	if (planarToPlayer.magnitudeSquared() < kRaycastEpsilon * kRaycastEpsilon) {
		return false;
	}

	if (planarToPlayer.dot(coverPoint.outwardNormal) >= 0.0f) {
		return false;
	}

	const physx::PxVec3 ray = playerEye - coverPoint.position;
	const float distance = ray.magnitude();
	if (distance < kRaycastEpsilon) {
		return false;
	}

	physx::PxRaycastHit hit;
	if (!RaycastIgnoringEnemy(coverPoint.position, ray.getNormalized(), distance - kRaycastEpsilon, hit)) {
		return false;
	}

	return hit.distance < distance - kRaycastEpsilon;
}

bool EnemyAIController::RaycastIgnoringEnemy(
	const physx::PxVec3& start,
	const physx::PxVec3& direction,
	float maxDistance,
	physx::PxRaycastHit& hit
) const {
	if (!physicsEngine_ || !enemy_ || maxDistance <= 0.0f || direction.magnitudeSquared() < 1e-4f) {
		return false;
	}

	physx::PxVec3 origin = start;
	float remainingDistance = maxDistance;
	const physx::PxVec3 unitDirection = direction.getNormalized();

	for (int passIndex = 0; passIndex < kMaxSelfHitPasses && remainingDistance > kRaycastEpsilon; ++passIndex) {
		physx::PxRaycastBuffer hitBuffer;
		if (!physicsEngine_->Raycast(origin, unitDirection, remainingDistance, hitBuffer) || !hitBuffer.hasBlock) {
			return false;
		}

		if (!enemy_->OwnsActor(hitBuffer.block.actor)) {
			hit = hitBuffer.block;
			return true;
		}

		const float advanceDistance = hitBuffer.block.distance + kRaycastEpsilon;
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
	for (int segmentIndex = 0; segmentIndex < kDebugCircleSegments; ++segmentIndex) {
		const float angle0 = 2.0f * physx::PxPi * static_cast<float>(segmentIndex) / static_cast<float>(kDebugCircleSegments);
		const float angle1 = 2.0f * physx::PxPi * static_cast<float>(segmentIndex + 1) / static_cast<float>(kDebugCircleSegments);

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
		return kSeekingCoverColor;
	case State::Fleeing:
		return kFleeingColor;
	case State::Idle:
	default:
		return kIdleColor;
	}
}
