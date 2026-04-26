#include "Enemy.h"

#include <algorithm>
#include <limits>

#include "PhysicsEngine.h"
#include "extensions/PxD6Joint.h"
#include "foundation/PxMathUtils.h"

namespace {
const float kEnemyRadius = 0.45f;
const float kEnemyHeight = 1.40f;
const float kEnemyMass = 85.0f;
const float kEnemyMaxHealth = 100.0f;

const float kHeadRadius = 0.22f;
const physx::PxVec3 kTorsoSize(0.55f, 0.80f, 0.32f);
const float kArmRadius = 0.11f;
const float kArmLength = 0.70f;
const float kLegRadius = 0.14f;
const float kLegLength = 0.90f;

const float kHeadMass = 5.0f;
const float kTorsoMass = 39.0f;
const float kArmMass = 6.0f;
const float kLegMass = 14.5f;

const physx::PxVec3 kHeadOffset(0.0f, 0.78f, 0.0f);
const physx::PxVec3 kLeftArmOffset(-0.62f, 0.22f, 0.0f);
const physx::PxVec3 kRightArmOffset(0.62f, 0.22f, 0.0f);
const physx::PxVec3 kLeftLegOffset(-0.20f, -0.95f, 0.0f);
const physx::PxVec3 kRightLegOffset(0.20f, -0.95f, 0.0f);

const physx::PxVec3 kNeckAnchorOffset(0.0f, 0.54f, 0.0f);
const physx::PxVec3 kLeftShoulderAnchorOffset(-0.28f, 0.22f, 0.0f);
const physx::PxVec3 kRightShoulderAnchorOffset(0.28f, 0.22f, 0.0f);
const physx::PxVec3 kLeftHipAnchorOffset(-0.20f, -0.46f, 0.0f);
const physx::PxVec3 kRightHipAnchorOffset(0.20f, -0.46f, 0.0f);

const float kLiveLinearDamping = 0.15f;
const float kLiveAngularDamping = 0.2f;
const float kRagdollLinearDamping = 0.08f;
const float kRagdollAngularDamping = 0.18f;
const float kDirectionEpsilon = 1e-4f;

float ToRadians(float degrees) {
	return degrees * physx::PxPi / 180.0f;
}
}

void Enemy::Initialize(PhysicsEngine* physicsEngine, const physx::PxVec3& position) {
	Shutdown();

	physicsEngine_ = physicsEngine;
	health_ = kEnemyMaxHealth;
	state_ = EnemyState::Alive;

	CreateLiveCapsule(position);
}

void Enemy::Shutdown() {
	ReleaseRagdoll();
	ReleaseLiveCapsule();

	physicsEngine_ = nullptr;
	health_ = kEnemyMaxHealth;
	state_ = EnemyState::Alive;
}

physx::PxRigidDynamic* Enemy::GetActor() const {
	return GetRootActor();
}

void Enemy::AppendRenderActors(std::vector<physx::PxRigidActor*>& out) const {
	if (capsuleActor_) {
		out.push_back(capsuleActor_);
		return;
	}

	for (physx::PxRigidDynamic* part : ragdollParts_) {
		if (part) {
			out.push_back(part);
		}
	}
}

physx::PxVec3 Enemy::GetPosition() const {
	physx::PxRigidDynamic* rootActor = GetRootActor();
	return rootActor ? rootActor->getGlobalPose().p : physx::PxVec3(0.0f);
}

bool Enemy::OwnsActor(const physx::PxActor* actor) const {
	if (capsuleActor_ == actor) {
		return true;
	}

	for (physx::PxRigidDynamic* part : ragdollParts_) {
		if (part == actor) {
			return true;
		}
	}

	return false;
}

float Enemy::GetHealth() const {
	return health_;
}

bool Enemy::IsAlive() const {
	return health_ > 0.0f;
}

bool Enemy::IsRagdollActive() const {
	return state_ == EnemyState::Ragdoll;
}

bool Enemy::CanUseAI() const {
	return capsuleActor_ != nullptr && health_ > 0.0f && state_ == EnemyState::Alive;
}

void Enemy::SetPlanarVelocity(const physx::PxVec3& velocity) {
	if (!capsuleActor_) {
		return;
	}

	const physx::PxVec3 currentVelocity = capsuleActor_->getLinearVelocity();
	capsuleActor_->wakeUp();
	capsuleActor_->setLinearVelocity(physx::PxVec3(velocity.x, currentVelocity.y, velocity.z), true);
}

void Enemy::StopPlanarMovement() {
	if (!capsuleActor_) {
		return;
	}

	const physx::PxVec3 currentVelocity = capsuleActor_->getLinearVelocity();
	capsuleActor_->setLinearVelocity(physx::PxVec3(0.0f, currentVelocity.y, 0.0f), true);
}

float Enemy::ApplyBulletImpact(const physx::PxVec3& hitPosition, const physx::PxVec3& direction, float impulseStrength, float damage) {
	const physx::PxVec3 impulseDirection =
		direction.magnitudeSquared() > kDirectionEpsilon ? direction.getNormalized() : physx::PxVec3(0.0f, 0.0f, 1.0f);

	if (state_ == EnemyState::Ragdoll) {
		ApplyImpulseToClosestRagdollPart(hitPosition, impulseDirection, impulseStrength);
		return 0.0f;
	}

	if (!capsuleActor_) {
		return 0.0f;
	}

	const float requestedDamage = std::max(0.0f, damage);
	const float appliedDamage = std::min(requestedDamage, health_);
	health_ -= appliedDamage;
	if (health_ < 0.0f) {
		health_ = 0.0f;
	}

	if (!IsAlive()) {
		const physx::PxTransform capsulePose = capsuleActor_->getGlobalPose();
		const physx::PxVec3 linearVelocity = capsuleActor_->getLinearVelocity();
		const physx::PxVec3 angularVelocity = capsuleActor_->getAngularVelocity();
		ReleaseLiveCapsule();
		ActivateRagdollFromBullet(capsulePose, linearVelocity, angularVelocity, hitPosition, impulseDirection, impulseStrength);
		return appliedDamage;
	}

	capsuleActor_->wakeUp();
	physx::PxRigidBodyExt::addForceAtPos(
		*capsuleActor_,
		impulseDirection * impulseStrength,
		hitPosition,
		physx::PxForceMode::eIMPULSE,
		true
	);

	return appliedDamage;
}

float Enemy::ApplyExplosionDamage(const physx::PxVec3& explosionPosition, float radius, float maxDamage, float maxImpulse) {
	if (radius <= 0.0f) {
		return 0.0f;
	}

	if (state_ == EnemyState::Ragdoll) {
		ApplyExplosionImpulseToRagdoll(explosionPosition, radius, maxImpulse);
		return 0.0f;
	}

	if (!capsuleActor_) {
		return 0.0f;
	}

	physx::PxVec3 offset = GetPosition() - explosionPosition;
	float distance = offset.magnitude();
	if (distance > radius) {
		return 0.0f;
	}

	if (distance < kDirectionEpsilon) {
		offset = physx::PxVec3(0.0f, 1.0f, 0.0f);
		distance = 0.0f;
	}

	const float effectFactor = std::max(0.0f, 1.0f - distance / radius);
	const float requestedDamage = std::max(0.0f, maxDamage) * effectFactor;
	const float appliedDamage = std::min(requestedDamage, health_);
	const float impulse = std::max(0.0f, maxImpulse) * effectFactor;

	health_ -= appliedDamage;
	if (health_ < 0.0f) {
		health_ = 0.0f;
	}

	if (!IsAlive()) {
		const physx::PxTransform capsulePose = capsuleActor_->getGlobalPose();
		const physx::PxVec3 linearVelocity = capsuleActor_->getLinearVelocity();
		const physx::PxVec3 angularVelocity = capsuleActor_->getAngularVelocity();
		ReleaseLiveCapsule();
		ActivateRagdollFromExplosion(
			capsulePose,
			linearVelocity,
			angularVelocity,
			explosionPosition,
			radius,
			std::max(0.0f, maxImpulse)
		);
		return appliedDamage;
	}

	capsuleActor_->wakeUp();
	capsuleActor_->addForce(offset.getNormalized() * impulse, physx::PxForceMode::eIMPULSE, true);

	return appliedDamage;
}

physx::PxQuat Enemy::GetVerticalCapsuleRotation() {
	return physx::PxQuat(physx::PxHalfPi, physx::PxVec3(0.0f, 0.0f, 1.0f));
}

physx::PxQuat Enemy::GetJointFrameRotation(const physx::PxVec3& axisDirection) {
	if (axisDirection.magnitudeSquared() < kDirectionEpsilon) {
		return physx::PxQuat(physx::PxIdentity);
	}

	return physx::PxShortestRotation(physx::PxVec3(1.0f, 0.0f, 0.0f), axisDirection.getNormalized());
}

physx::PxTransform Enemy::BuildLocalJointFrame(
	const physx::PxRigidActor& actor,
	const physx::PxVec3& anchorWorldPosition,
	const physx::PxVec3& axisDirection
) {
	return actor.getGlobalPose().transformInv(physx::PxTransform(anchorWorldPosition, GetJointFrameRotation(axisDirection)));
}

void Enemy::CreateLiveCapsule(const physx::PxVec3& position) {
	if (!physicsEngine_) {
		return;
	}

	physx::PxMaterial* material = physicsEngine_->GetMaterial(0.65f, 0.55f, 0.05f);

	// PhysX capsules are aligned along the X axis by default, so rotate the shape to stand it upright.
	physx::PxShape* shape = physicsEngine_->CreateCapsuleShape(
		kEnemyRadius,
		kEnemyHeight,
		physx::PxVec3(0.0f),
		GetVerticalCapsuleRotation(),
		material,
		CustomFilterData::eDYNAMIC,
		true
	);

	const float density = kEnemyMass / PhysicsEngine::GetCapsuleVolume(kEnemyRadius, kEnemyHeight);
	capsuleActor_ = physicsEngine_->AddDynamicActor(shape, position, physx::PxQuat(physx::PxIdentity), density);
	shape->release();

	if (!capsuleActor_) {
		return;
	}

	capsuleActor_->setRigidDynamicLockFlag(physx::PxRigidDynamicLockFlag::eLOCK_ANGULAR_X, true);
	capsuleActor_->setRigidDynamicLockFlag(physx::PxRigidDynamicLockFlag::eLOCK_ANGULAR_Y, true);
	capsuleActor_->setRigidDynamicLockFlag(physx::PxRigidDynamicLockFlag::eLOCK_ANGULAR_Z, true);
	capsuleActor_->setLinearDamping(kLiveLinearDamping);
	capsuleActor_->setAngularDamping(kLiveAngularDamping);
	capsuleActor_->setSolverIterationCounts(8, 4);
	capsuleActor_->setRigidBodyFlag(physx::PxRigidBodyFlag::eENABLE_CCD, true);
}

void Enemy::ReleaseLiveCapsule() {
	if (capsuleActor_) {
		capsuleActor_->release();
		capsuleActor_ = nullptr;
	}
}

void Enemy::ReleaseRagdoll() {
	for (physx::PxJoint* joint : ragdollJoints_) {
		if (joint) {
			joint->release();
		}
	}
	ragdollJoints_.clear();

	for (physx::PxRigidDynamic* part : ragdollParts_) {
		if (part) {
			part->release();
		}
	}
	ragdollParts_.clear();

	torsoActor_ = nullptr;
	if (state_ == EnemyState::Ragdoll) {
		state_ = EnemyState::Alive;
	}
}

void Enemy::ActivateRagdoll(
	const physx::PxTransform& capsulePose,
	const physx::PxVec3& linearVelocity,
	const physx::PxVec3& angularVelocity
) {
	ReleaseRagdoll();
	state_ = EnemyState::Ragdoll;

	ragdollParts_.assign(static_cast<std::size_t>(RagdollPartId::Count), nullptr);

	const physx::PxQuat bodyRotation = capsulePose.q;
	const physx::PxQuat legRotation = bodyRotation * GetVerticalCapsuleRotation();

	ragdollParts_[static_cast<std::size_t>(RagdollPartId::Head)] =
		CreateRagdollSphere(kHeadRadius, capsulePose.transform(kHeadOffset), kHeadMass);
	ragdollParts_[static_cast<std::size_t>(RagdollPartId::Torso)] =
		CreateRagdollBox(kTorsoSize, capsulePose.p, bodyRotation, kTorsoMass);
	ragdollParts_[static_cast<std::size_t>(RagdollPartId::LeftArm)] =
		CreateRagdollCapsule(kArmRadius, kArmLength, capsulePose.transform(kLeftArmOffset), bodyRotation, kArmMass);
	ragdollParts_[static_cast<std::size_t>(RagdollPartId::RightArm)] =
		CreateRagdollCapsule(kArmRadius, kArmLength, capsulePose.transform(kRightArmOffset), bodyRotation, kArmMass);
	ragdollParts_[static_cast<std::size_t>(RagdollPartId::LeftLeg)] =
		CreateRagdollCapsule(kLegRadius, kLegLength, capsulePose.transform(kLeftLegOffset), legRotation, kLegMass);
	ragdollParts_[static_cast<std::size_t>(RagdollPartId::RightLeg)] =
		CreateRagdollCapsule(kLegRadius, kLegLength, capsulePose.transform(kRightLegOffset), legRotation, kLegMass);

	for (physx::PxRigidDynamic* part : ragdollParts_) {
		if (part) {
			ConfigureRagdollPart(*part, linearVelocity, angularVelocity);
		}
	}

	torsoActor_ = GetRagdollPart(RagdollPartId::Torso);
	if (!torsoActor_) {
		return;
	}

	const physx::PxVec3 neckAxis = bodyRotation.rotate(physx::PxVec3(0.0f, 1.0f, 0.0f));
	const physx::PxVec3 leftShoulderAxis = bodyRotation.rotate(physx::PxVec3(-1.0f, 0.0f, 0.0f));
	const physx::PxVec3 rightShoulderAxis = bodyRotation.rotate(physx::PxVec3(1.0f, 0.0f, 0.0f));
	const physx::PxVec3 leftHipAxis = bodyRotation.rotate(kLeftLegOffset.getNormalized());
	const physx::PxVec3 rightHipAxis = bodyRotation.rotate(kRightLegOffset.getNormalized());

	if (physx::PxD6Joint* joint = CreateLimitedJoint(
		torsoActor_,
		GetRagdollPart(RagdollPartId::Head),
		capsulePose.transform(kNeckAnchorOffset),
		neckAxis,
		ToRadians(-30.0f),
		ToRadians(30.0f),
		ToRadians(25.0f),
		ToRadians(20.0f))) {
		ragdollJoints_.push_back(joint);
	}

	if (physx::PxD6Joint* joint = CreateLimitedJoint(
		torsoActor_,
		GetRagdollPart(RagdollPartId::LeftArm),
		capsulePose.transform(kLeftShoulderAnchorOffset),
		leftShoulderAxis,
		ToRadians(-35.0f),
		ToRadians(35.0f),
		ToRadians(65.0f),
		ToRadians(45.0f))) {
		ragdollJoints_.push_back(joint);
	}

	if (physx::PxD6Joint* joint = CreateLimitedJoint(
		torsoActor_,
		GetRagdollPart(RagdollPartId::RightArm),
		capsulePose.transform(kRightShoulderAnchorOffset),
		rightShoulderAxis,
		ToRadians(-35.0f),
		ToRadians(35.0f),
		ToRadians(65.0f),
		ToRadians(45.0f))) {
		ragdollJoints_.push_back(joint);
	}

	if (physx::PxD6Joint* joint = CreateLimitedJoint(
		torsoActor_,
		GetRagdollPart(RagdollPartId::LeftLeg),
		capsulePose.transform(kLeftHipAnchorOffset),
		leftHipAxis,
		ToRadians(-20.0f),
		ToRadians(20.0f),
		ToRadians(40.0f),
		ToRadians(25.0f))) {
		ragdollJoints_.push_back(joint);
	}

	if (physx::PxD6Joint* joint = CreateLimitedJoint(
		torsoActor_,
		GetRagdollPart(RagdollPartId::RightLeg),
		capsulePose.transform(kRightHipAnchorOffset),
		rightHipAxis,
		ToRadians(-20.0f),
		ToRadians(20.0f),
		ToRadians(40.0f),
		ToRadians(25.0f))) {
		ragdollJoints_.push_back(joint);
	}

	for (physx::PxRigidDynamic* part : ragdollParts_) {
		if (part) {
			part->wakeUp();
		}
	}
}

void Enemy::ActivateRagdollFromBullet(
	const physx::PxTransform& capsulePose,
	const physx::PxVec3& linearVelocity,
	const physx::PxVec3& angularVelocity,
	const physx::PxVec3& hitPosition,
	const physx::PxVec3& direction,
	float impulseStrength
) {
	ActivateRagdoll(capsulePose, linearVelocity, angularVelocity);
	ApplyImpulseToClosestRagdollPart(hitPosition, direction, impulseStrength);
}

void Enemy::ActivateRagdollFromExplosion(
	const physx::PxTransform& capsulePose,
	const physx::PxVec3& linearVelocity,
	const physx::PxVec3& angularVelocity,
	const physx::PxVec3& explosionPosition,
	float radius,
	float maxImpulse
) {
	ActivateRagdoll(capsulePose, linearVelocity, angularVelocity);
	ApplyExplosionImpulseToRagdoll(explosionPosition, radius, maxImpulse);
}

physx::PxRigidDynamic* Enemy::CreateRagdollBox(const physx::PxVec3& size, const physx::PxVec3& position, const physx::PxQuat& rotation, float mass) {
	if (!physicsEngine_) {
		return nullptr;
	}

	physx::PxMaterial* material = physicsEngine_->GetMaterial(0.65f, 0.55f, 0.05f);
	physx::PxShape* shape = physicsEngine_->CreateBoxShape(size, material, CustomFilterData::eDYNAMIC, true);
	const float density = mass / PhysicsEngine::GetBoxVolume(size);
	physx::PxRigidDynamic* actor = physicsEngine_->AddDynamicActor(shape, position, rotation, density);
	shape->release();
	return actor;
}

physx::PxRigidDynamic* Enemy::CreateRagdollSphere(float radius, const physx::PxVec3& position, float mass) {
	if (!physicsEngine_) {
		return nullptr;
	}

	physx::PxMaterial* material = physicsEngine_->GetMaterial(0.65f, 0.55f, 0.05f);
	physx::PxShape* shape = physicsEngine_->CreateSphereShape(radius, material, CustomFilterData::eDYNAMIC, true);
	const float density = mass / PhysicsEngine::GetSphereVolume(radius);
	physx::PxRigidDynamic* actor = physicsEngine_->AddDynamicActor(shape, position, physx::PxQuat(physx::PxIdentity), density);
	shape->release();
	return actor;
}

physx::PxRigidDynamic* Enemy::CreateRagdollCapsule(
	float radius,
	float size,
	const physx::PxVec3& position,
	const physx::PxQuat& rotation,
	float mass
) {
	if (!physicsEngine_) {
		return nullptr;
	}

	physx::PxMaterial* material = physicsEngine_->GetMaterial(0.65f, 0.55f, 0.05f);
	physx::PxShape* shape = physicsEngine_->CreateCapsuleShape(radius, size, material, CustomFilterData::eDYNAMIC, true);
	const float density = mass / PhysicsEngine::GetCapsuleVolume(radius, size);
	physx::PxRigidDynamic* actor = physicsEngine_->AddDynamicActor(shape, position, rotation, density);
	shape->release();
	return actor;
}

physx::PxD6Joint* Enemy::CreateLimitedJoint(
	physx::PxRigidDynamic* actor0,
	physx::PxRigidDynamic* actor1,
	const physx::PxVec3& anchorWorldPosition,
	const physx::PxVec3& axisDirection,
	float lowerTwistLimit,
	float upperTwistLimit,
	float swing1Limit,
	float swing2Limit
) {
	if (!physicsEngine_ || !actor0 || !actor1) {
		return nullptr;
	}

	physx::PxD6Joint* joint = physx::PxD6JointCreate(
		physicsEngine_->GetPhysics(),
		actor0,
		BuildLocalJointFrame(*actor0, anchorWorldPosition, axisDirection),
		actor1,
		BuildLocalJointFrame(*actor1, anchorWorldPosition, axisDirection)
	);

	if (!joint) {
		return nullptr;
	}

	joint->setMotion(physx::PxD6Axis::eX, physx::PxD6Motion::eLOCKED);
	joint->setMotion(physx::PxD6Axis::eY, physx::PxD6Motion::eLOCKED);
	joint->setMotion(physx::PxD6Axis::eZ, physx::PxD6Motion::eLOCKED);
	joint->setMotion(physx::PxD6Axis::eTWIST, physx::PxD6Motion::eLIMITED);
	joint->setMotion(physx::PxD6Axis::eSWING1, physx::PxD6Motion::eLIMITED);
	joint->setMotion(physx::PxD6Axis::eSWING2, physx::PxD6Motion::eLIMITED);
	joint->setTwistLimit(physx::PxJointAngularLimitPair(lowerTwistLimit, upperTwistLimit));
	joint->setSwingLimit(physx::PxJointLimitCone(swing1Limit, swing2Limit));

	return joint;
}

void Enemy::ConfigureRagdollPart(
	physx::PxRigidDynamic& actor,
	const physx::PxVec3& linearVelocity,
	const physx::PxVec3& angularVelocity
) const {
	actor.setLinearDamping(kRagdollLinearDamping);
	actor.setAngularDamping(kRagdollAngularDamping);
	actor.setSolverIterationCounts(12, 4);
	actor.setRigidBodyFlag(physx::PxRigidBodyFlag::eENABLE_CCD, true);
	actor.setLinearVelocity(linearVelocity);
	actor.setAngularVelocity(angularVelocity);
}

void Enemy::ApplyImpulseToClosestRagdollPart(const physx::PxVec3& hitPosition, const physx::PxVec3& direction, float impulseStrength) {
	const std::size_t partIndex = FindClosestRagdollPartIndex(hitPosition);
	if (partIndex >= ragdollParts_.size()) {
		return;
	}

	physx::PxRigidDynamic* part = ragdollParts_[partIndex];
	if (!part) {
		return;
	}

	const physx::PxVec3 impulseDirection =
		direction.magnitudeSquared() > kDirectionEpsilon ? direction.getNormalized() : physx::PxVec3(0.0f, 0.0f, 1.0f);

	part->wakeUp();
	physx::PxRigidBodyExt::addForceAtPos(
		*part,
		impulseDirection * impulseStrength,
		hitPosition,
		physx::PxForceMode::eIMPULSE,
		true
	);
}

void Enemy::ApplyExplosionImpulseToRagdoll(const physx::PxVec3& explosionPosition, float radius, float maxImpulse) {
	if (radius <= 0.0f || ragdollParts_.empty()) {
		return;
	}

	for (physx::PxRigidDynamic* part : ragdollParts_) {
		if (!part) {
			continue;
		}

		physx::PxVec3 offset = part->getGlobalPose().p - explosionPosition;
		float distance = offset.magnitude();
		if (distance > radius) {
			continue;
		}

		if (distance >= kDirectionEpsilon && physicsEngine_) {
			physx::PxRaycastBuffer hit;
			const bool hasHit = physicsEngine_->Raycast(explosionPosition, offset.getNormalized(), distance, hit);
			if (hasHit && hit.hasBlock && !OwnsActor(hit.block.actor)) {
				continue;
			}
		}

		if (distance < kDirectionEpsilon) {
			offset = physx::PxVec3(0.0f, 1.0f, 0.0f);
			distance = 0.0f;
		}

		const float effectFactor = std::max(0.0f, 1.0f - distance / radius);
		const float impulse = std::max(0.0f, maxImpulse) * effectFactor;

		part->wakeUp();
		part->addForce(offset.getNormalized() * impulse, physx::PxForceMode::eIMPULSE, true);
	}
}

std::size_t Enemy::FindClosestRagdollPartIndex(const physx::PxVec3& position) const {
	float closestDistanceSquared = std::numeric_limits<float>::max();
	std::size_t closestIndex = ragdollParts_.size();

	for (std::size_t index = 0; index < ragdollParts_.size(); ++index) {
		physx::PxRigidDynamic* part = ragdollParts_[index];
		if (!part) {
			continue;
		}

		const float distanceSquared = (part->getGlobalPose().p - position).magnitudeSquared();
		if (distanceSquared < closestDistanceSquared) {
			closestDistanceSquared = distanceSquared;
			closestIndex = index;
		}
	}

	return closestIndex;
}

physx::PxRigidDynamic* Enemy::GetRagdollPart(RagdollPartId partId) const {
	const std::size_t partIndex = static_cast<std::size_t>(partId);
	return partIndex < ragdollParts_.size() ? ragdollParts_[partIndex] : nullptr;
}

physx::PxRigidDynamic* Enemy::GetRootActor() const {
	if (capsuleActor_) {
		return capsuleActor_;
	}

	if (torsoActor_) {
		return torsoActor_;
	}

	for (physx::PxRigidDynamic* part : ragdollParts_) {
		if (part) {
			return part;
		}
	}

	return nullptr;
}
