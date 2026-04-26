#include "Enemy.h"

#include "PhysicsEngine.h"

namespace {
const float kEnemyRadius = 0.45f;
const float kEnemyHeight = 1.40f;
const float kEnemyMass = 85.0f;
const float kEnemyMaxHealth = 100.0f;
}

void Enemy::Initialize(PhysicsEngine* physicsEngine, const physx::PxVec3& position) {
	Shutdown();

	physicsEngine_ = physicsEngine;
	health_ = kEnemyMaxHealth;

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
	actor_ = physicsEngine_->AddDynamicActor(shape, position, physx::PxQuat(physx::PxIdentity), density);
	shape->release();
	actor_->setLinearDamping(0.15f);
	actor_->setAngularDamping(0.2f);
	actor_->setSolverIterationCounts(8, 4);
}

void Enemy::Shutdown() {
	if (actor_) {
		actor_->release();
		actor_ = nullptr;
	}

	physicsEngine_ = nullptr;
	health_ = kEnemyMaxHealth;
}

physx::PxRigidDynamic* Enemy::GetActor() const {
	return actor_;
}

physx::PxVec3 Enemy::GetPosition() const {
	return actor_ ? actor_->getGlobalPose().p : physx::PxVec3(0.0f);
}

bool Enemy::OwnsActor(const physx::PxActor* actor) const {
	return actor_ == actor;
}

float Enemy::GetHealth() const {
	return health_;
}

void Enemy::ApplyBulletImpact(const physx::PxVec3& hitPosition, const physx::PxVec3& direction, float impulseStrength) {
	if (!actor_) {
		return;
	}

	const physx::PxVec3 impulseDirection = direction.getNormalized();
	actor_->wakeUp();
	physx::PxRigidBodyExt::addForceAtPos(
		*actor_,
		impulseDirection * impulseStrength,
		hitPosition,
		physx::PxForceMode::eIMPULSE,
		true
	);
}

float Enemy::ApplyExplosionDamage(const physx::PxVec3& explosionPosition, float radius, float maxDamage, float maxImpulse) {
	if (!actor_) {
		return 0.0f;
	}

	physx::PxVec3 offset = GetPosition() - explosionPosition;
	float distance = offset.magnitude();
	if (distance > radius) {
		return 0.0f;
	}

	if (distance < 1e-4f) {
		offset = physx::PxVec3(0.0f, 1.0f, 0.0f);
		distance = 0.0f;
	}

	const float effectFactor = 1.0f - distance / radius;
	const float damage = maxDamage * effectFactor;
	const float impulse = maxImpulse * effectFactor;

	health_ -= damage;
	if (health_ < 0.0f) {
		health_ = 0.0f;
	}

	actor_->wakeUp();
	actor_->addForce(offset.getNormalized() * impulse, physx::PxForceMode::eIMPULSE, true);

	return damage;
}

physx::PxQuat Enemy::GetVerticalCapsuleRotation() {
	return physx::PxQuat(physx::PxHalfPi, physx::PxVec3(0.0f, 0.0f, 1.0f));
}
