#pragma once

#include <iostream>
#include <vector>
#include <unordered_set>
#include <functional>
#include "PxPhysicsAPI.h"
#include "NvCloth/Factory.h"
#include "NvCloth/Solver.h"
#include "Cloth.h"

#ifdef _DEBUG
#define USE_PVD
#define PVD_HOST "127.0.0.1"
#endif

class CustomEventCallback : public physx::PxSimulationEventCallback {
public:
	virtual void onConstraintBreak(physx::PxConstraintInfo *constraints, uint32_t count) override {};
	virtual void onWake(physx::PxActor **actors, uint32_t count) override {};
	virtual void onSleep(physx::PxActor **actors, uint32_t count) override {};
	virtual void onContact(const physx::PxContactPairHeader &pairHeader, const physx::PxContactPair *pairs, uint32_t nbPairs) override;
	virtual void onTrigger(physx::PxTriggerPair *pairs, uint32_t count) override {};
	virtual void onAdvance(const physx::PxRigidBody *const *bodyBuffer, const physx::PxTransform *poseBuffer, const uint32_t count) override {};

};

enum CustomFilterData {
	eDYNAMIC = 1,
	eOBSTACLE,
	eTRIGGER
};

class PhysicsEngine {
	struct JointDesc {
		physx::PxRigidActor *actor0;
		physx::PxTransform transform0;
		physx::PxRigidActor *actor1;
		physx::PxTransform transform1;
		bool collisionEnabled;
		bool visualizationEnabled;
		bool isBreakable;
		float forceThreshold;
		float torqueThreshold;
	};

public:
	struct FixedJointDesc : public JointDesc {};
	struct SphericalJointDesc : public JointDesc {
		bool limitEnabled;
		float yLimitAngle;
		float zLimitAngle;
	};
	struct RevoluteJointDesc : public JointDesc {
		bool limitEnabled;
		float lowerLimit;
		float upperLimit;
	};
	struct PrismaticJointDesc : public JointDesc {
		bool limitEnabled;
		float lowerLimit;
		float upperLimit;
	};
	struct DistanceJointDesc : public JointDesc {
		bool minDistanceEnabled;
		float minDistance;
		bool maxDistanceEnabled;
		float maxDistance;
	};
	struct D6JointDesc : public JointDesc {
		physx::PxD6Motion::Enum motions[physx::PxD6Axis::eCOUNT];
		float xLowerLimit;
		float xUpperLimit;
		float yLowerLimit;
		float yUpperLimit;
		float zLowerLimit;
		float zUpperLimit;
		float twistLowerLimit;
		float twistUpperLimit;
		float yLimitAngle;
		float zLimitAngle;
	};

public:
	PhysicsEngine();
	~PhysicsEngine();
	void SetGravity(physx::PxVec3 gravity);
	void Simulate(float elapsedTime);
	bool Raycast(physx::PxVec3 start, physx::PxVec3 direction, float maxDistance, physx::PxRaycastBuffer &hit);
	physx::PxMaterial *CreateMaterial(float staticFriction, float dynamicFriction, float restitution);
	physx::PxMaterial *GetMaterial(float staticFriction, float dynamicFriction, float restitution);
	physx::PxShape *CreateBoxShape(
		physx::PxVec3 size,
		physx::PxMaterial *material,
		CustomFilterData filterData,
		bool isExclusive = false,
		physx::PxShapeFlags shapeFlags = physx::PxShapeFlag::eVISUALIZATION | physx::PxShapeFlag::eSCENE_QUERY_SHAPE | physx::PxShapeFlag::eSIMULATION_SHAPE
	);
	physx::PxShape *CreateBoxShape(
		physx::PxVec3 size,
		physx::PxVec3 position,
		physx::PxQuat rotation,
		physx::PxMaterial *material,
		CustomFilterData filterData,
		bool isExclusive = false,
		physx::PxShapeFlags shapeFlags = physx::PxShapeFlag::eVISUALIZATION | physx::PxShapeFlag::eSCENE_QUERY_SHAPE | physx::PxShapeFlag::eSIMULATION_SHAPE
	);
	physx::PxShape *CreateSphereShape(
		float radius,
		physx::PxMaterial *material,
		CustomFilterData filterData,
		bool isExclusive = false,
		physx::PxShapeFlags shapeFlags = physx::PxShapeFlag::eVISUALIZATION | physx::PxShapeFlag::eSCENE_QUERY_SHAPE | physx::PxShapeFlag::eSIMULATION_SHAPE
	);
	physx::PxShape *CreateSphereShape(
		float radius,
		physx::PxVec3 position,
		physx::PxMaterial *material,
		CustomFilterData filterData,
		bool isExclusive = false,
		physx::PxShapeFlags shapeFlags = physx::PxShapeFlag::eVISUALIZATION | physx::PxShapeFlag::eSCENE_QUERY_SHAPE | physx::PxShapeFlag::eSIMULATION_SHAPE
	);
	physx::PxShape *CreateCapsuleShape(
		float radius,
		float size,
		physx::PxMaterial *material,
		CustomFilterData filterData,
		bool isExclusive = false,
		physx::PxShapeFlags shapeFlags = physx::PxShapeFlag::eVISUALIZATION | physx::PxShapeFlag::eSCENE_QUERY_SHAPE | physx::PxShapeFlag::eSIMULATION_SHAPE
	);
	physx::PxShape *CreateCapsuleShape(
		float radius,
		float size,
		physx::PxVec3 position,
		physx::PxQuat rotation,
		physx::PxMaterial *material,
		CustomFilterData filterData,
		bool isExclusive = false,
		physx::PxShapeFlags shapeFlags = physx::PxShapeFlag::eVISUALIZATION | physx::PxShapeFlag::eSCENE_QUERY_SHAPE | physx::PxShapeFlag::eSIMULATION_SHAPE
	);
	static float GetBoxVolume(physx::PxVec3 size);
	static float GetSphereVolume(float radius);
	static float GetCapsuleVolume(float radius, float size);
	physx::PxRigidStatic *AddGround(physx::PxVec3 normal, float distance, physx::PxMaterial *material);
	physx::PxRigidStatic *AddStaticActor(physx::PxShape *shape, physx::PxVec3 position, physx::PxQuat rotation);
	physx::PxRigidDynamic *AddDynamicActor(physx::PxShape *shape, physx::PxVec3 position, physx::PxQuat rotation, float density);
	physx::PxRigidStatic *AddStaticActor(std::vector<physx::PxShape *> shapes, physx::PxVec3 position, physx::PxQuat rotation);
	physx::PxRigidDynamic *AddDynamicActor(std::vector <physx::PxShape *> shapes, physx::PxVec3 position, physx::PxQuat rotation, float density);
	std::vector<physx::PxRigidActor *> GetActors(physx::PxActorTypeFlags types = physx::PxActorTypeFlag::eRIGID_STATIC | physx::PxActorTypeFlag::eRIGID_DYNAMIC);
	void RemoveActor(physx::PxActor *actor);
	void MarkActor(physx::PxActor *actor);
	void AddCloth(Cloth *cloth);
	std::vector<Cloth *> GetCloths() const;
	physx::PxJoint *CreateFixedJoint(FixedJointDesc desc);
	physx::PxJoint *CreateSphericalJoint(SphericalJointDesc desc);
	physx::PxJoint *CreateRevoluteJoint(RevoluteJointDesc desc);
	physx::PxJoint *CreatePrismaticJoint(PrismaticJointDesc desc);
	physx::PxJoint *CreateDistanceJoint(DistanceJointDesc desc);
	physx::PxJoint *CreateD6Joint(D6JointDesc desc);

private:
	physx::PxDefaultAllocator allocatorCallback;
	physx::PxDefaultErrorCallback errorCallback;
	physx::PxFoundation *foundation;
	physx::PxPhysics *physics;
#ifdef USE_PVD
	physx::PxPvd *pvd;
	physx::PxPvdTransport *transport;
#endif
	physx::PxScene *scene;

	nv::cloth::Factory *factory;
	nv::cloth::Solver *solver;
	std::vector<Cloth *> cloths;

	std::unordered_set<physx::PxActor *> markedActors;
	CustomEventCallback eventCallback;

	static physx::PxFilterFlags CustomFilterShader(
		physx::PxFilterObjectAttributes attributes0,
		physx::PxFilterData filterData0,
		physx::PxFilterObjectAttributes attributes1,
		physx::PxFilterData filterData1,
		physx::PxPairFlags &pairFlags,
		const void *constantBlock,
		uint32_t constantBlockSize
	);

	friend class Cloth;

};
