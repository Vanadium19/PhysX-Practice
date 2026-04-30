#pragma once

#include <cstddef>
#include <vector>

#include "PxPhysicsAPI.h"

class PhysicsEngine;

enum class EnemyState {
	Alive,
	Ragdoll
};

class Enemy {
public:
	Enemy() = default;

	void Initialize(PhysicsEngine* physicsEngine, const physx::PxVec3& position);
	void Shutdown();

	physx::PxRigidDynamic* GetActor() const;
	void AppendRenderActors(std::vector<physx::PxRigidActor*>& out) const;
	physx::PxVec3 GetPosition() const;
	bool OwnsActor(const physx::PxActor* actor) const;
	float GetHealth() const;
	bool IsAlive() const;
	bool IsRagdollActive() const;
	bool CanUseAI() const;
	void SetPlanarVelocity(const physx::PxVec3& velocity);
	void StopPlanarMovement();

	float ApplyBulletImpact(const physx::PxVec3& hitPosition, const physx::PxVec3& direction, float impulseStrength, float damage);
	float ApplyExplosionDamage(const physx::PxVec3& explosionPosition, float radius, float maxDamage, float maxImpulse);

private:
	static physx::PxQuat GetVerticalCapsuleRotation();
	static physx::PxQuat GetJointFrameRotation(const physx::PxVec3& axisDirection);
	static physx::PxTransform BuildLocalJointFrame(const physx::PxRigidActor& actor, const physx::PxVec3& anchorWorldPosition, const physx::PxVec3& axisDirection);

	enum class RagdollPartId : std::size_t {
		Head = 0,
		Torso,
		LeftArm,
		RightArm,
		LeftLeg,
		RightLeg,
		Count
	};

	void CreateLiveCapsule(const physx::PxVec3& position);
	void ReleaseLiveCapsule();
	void ReleaseRagdoll();
	void ActivateRagdoll(const physx::PxTransform& capsulePose, const physx::PxVec3& linearVelocity, const physx::PxVec3& angularVelocity);
	void ActivateRagdollFromBullet(
		const physx::PxTransform& capsulePose,
		const physx::PxVec3& linearVelocity,
		const physx::PxVec3& angularVelocity,
		const physx::PxVec3& hitPosition,
		const physx::PxVec3& direction,
		float impulseStrength
	);
	void ActivateRagdollFromExplosion(
		const physx::PxTransform& capsulePose,
		const physx::PxVec3& linearVelocity,
		const physx::PxVec3& angularVelocity,
		const physx::PxVec3& explosionPosition,
		float radius,
		float maxImpulse
	);
	physx::PxRigidDynamic* CreateRagdollBox(const physx::PxVec3& size, const physx::PxVec3& position, const physx::PxQuat& rotation, float mass);
	physx::PxRigidDynamic* CreateRagdollSphere(float radius, const physx::PxVec3& position, float mass);
	physx::PxRigidDynamic* CreateRagdollCapsule(float radius, float size, const physx::PxVec3& position, const physx::PxQuat& rotation, float mass);
	physx::PxD6Joint* CreateLimitedJoint(
		physx::PxRigidDynamic* actor0,
		physx::PxRigidDynamic* actor1,
		const physx::PxVec3& anchorWorldPosition,
		const physx::PxVec3& axisDirection,
		float lowerTwistLimit,
		float upperTwistLimit,
		float swing1Limit,
		float swing2Limit
	);
	void ConfigureRagdollPart(physx::PxRigidDynamic& actor, const physx::PxVec3& linearVelocity, const physx::PxVec3& angularVelocity) const;
	void ApplyImpulseToClosestRagdollPart(const physx::PxVec3& hitPosition, const physx::PxVec3& direction, float impulseStrength);
	void ApplyExplosionImpulseToRagdoll(const physx::PxVec3& explosionPosition, float radius, float maxImpulse);
	std::size_t FindClosestRagdollPartIndex(const physx::PxVec3& position) const;
	physx::PxRigidDynamic* GetRagdollPart(RagdollPartId partId) const;
	physx::PxRigidDynamic* GetRootActor() const;

	PhysicsEngine* physicsEngine_ = nullptr;
	physx::PxRigidDynamic* capsuleActor_ = nullptr;
	physx::PxRigidDynamic* torsoActor_ = nullptr;
	std::vector<physx::PxRigidDynamic*> ragdollParts_;
	std::vector<physx::PxJoint*> ragdollJoints_;
	EnemyState state_ = EnemyState::Alive;
	float health_ = 100.0f;
};
