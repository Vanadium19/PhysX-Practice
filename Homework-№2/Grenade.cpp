#include "Grenade.h"

#include "PhysicsEngine.h"

namespace {
const float kGrenadeRadius = 0.28f;
const float kGrenadeMass = 2.2f;
}

Grenade::Grenade(float fuseTimeSeconds) :
	fuseTimeSeconds_(fuseTimeSeconds) {
}

void Grenade::Initialize(PhysicsEngine* physicsEngine, const physx::PxVec3& position, const physx::PxVec3& initialVelocity) {
	Shutdown();

	physicsEngine_ = physicsEngine;
	elapsedTime_ = 0.0f;

	physx::PxMaterial* material = physicsEngine_->GetMaterial(0.45f, 0.35f, 0.65f);
	physx::PxShape* shape = physicsEngine_->CreateSphereShape(
		kGrenadeRadius,
		material,
		CustomFilterData::eDYNAMIC,
		true
	);

	const float density = kGrenadeMass / PhysicsEngine::GetSphereVolume(kGrenadeRadius);
	actor_ = physicsEngine_->AddDynamicActor(shape, position, physx::PxQuat(physx::PxIdentity), density);
	shape->release();
	actor_->setLinearVelocity(initialVelocity);
	actor_->setLinearDamping(0.10f);
	actor_->setAngularDamping(0.35f);
	actor_->setRigidBodyFlag(physx::PxRigidBodyFlag::eENABLE_CCD, true);
}

void Grenade::Shutdown() {
	if (actor_) {
		actor_->release();
		actor_ = nullptr;
	}

	physicsEngine_ = nullptr;
	elapsedTime_ = 0.0f;
}

void Grenade::Update(float elapsedTime) {
	elapsedTime_ += elapsedTime;
}

physx::PxRigidDynamic* Grenade::GetActor() const {
	return actor_;
}

physx::PxVec3 Grenade::GetPosition() const {
	return actor_ ? actor_->getGlobalPose().p : physx::PxVec3(0.0f);
}

bool Grenade::ShouldExplode() const {
	return actor_ && elapsedTime_ >= fuseTimeSeconds_;
}
