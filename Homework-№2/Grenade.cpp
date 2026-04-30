#include "Grenade.h"

#include "GameConstants.h"
#include "PhysicsEngine.h"

Grenade::Grenade(float fuseTimeSeconds) :
	fuseTimeSeconds_(fuseTimeSeconds) {
}

void Grenade::Initialize(PhysicsEngine* physicsEngine, const physx::PxVec3& position, const physx::PxVec3& initialVelocity) {
	Shutdown();

	physicsEngine_ = physicsEngine;
	elapsedTime_ = 0.0f;

	const MaterialSettings grenadeMaterial = GameConstants::MaterialConfig::Grenade;
	physx::PxMaterial* material = physicsEngine_->GetMaterial(
		grenadeMaterial.staticFriction,
		grenadeMaterial.dynamicFriction,
		grenadeMaterial.restitution
	);
	physx::PxShape* shape = physicsEngine_->CreateSphereShape(
		GameConstants::GrenadeConfig::Radius,
		material,
		CustomFilterData::eDYNAMIC,
		true
	);

	const float density =
		GameConstants::GrenadeConfig::Mass / PhysicsEngine::GetSphereVolume(GameConstants::GrenadeConfig::Radius);
	actor_ = physicsEngine_->AddDynamicActor(shape, position, physx::PxQuat(physx::PxIdentity), density);
	shape->release();
	actor_->setLinearVelocity(initialVelocity);
	actor_->setLinearDamping(GameConstants::GrenadeConfig::LinearDamping);
	actor_->setAngularDamping(GameConstants::GrenadeConfig::AngularDamping);
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
