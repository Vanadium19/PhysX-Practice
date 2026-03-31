#pragma once

#include <PxPhysicsAPI.h>

class BilliardsEventCallback : public physx::PxSimulationEventCallback {
public:
	virtual void onConstraintBreak(physx::PxConstraintInfo* constraints, uint32_t count) override {};
	virtual void onWake(physx::PxActor** actors, uint32_t count) override {};
	virtual void onSleep(physx::PxActor** actors, uint32_t count) override {};
	virtual void onContact(const physx::PxContactPairHeader& pairHeader, const physx::PxContactPair* pairs, uint32_t nbPairs) override {};
	virtual void onTrigger(physx::PxTriggerPair* pairs, uint32_t count) override;
	virtual void onAdvance(const physx::PxRigidBody* const* bodyBuffer, const physx::PxTransform* poseBuffer, const uint32_t count) override {};
};
