#include <iostream>
#include "PxPhysicsAPI.h"

int main()
{
	physx::PxDefaultAllocator allocator;
	physx::PxDefaultErrorCallback errorCallback;
	physx::PxFoundation* foundation = PxCreateFoundation(PX_PHYSICS_VERSION, allocator, errorCallback);

	physx::PxPvd* pvd = physx::PxCreatePvd(*foundation);
	physx::PxPvdTransport* transport = physx::PxDefaultPvdSocketTransportCreate("127.0.0.1", 5425, 10000);

	if (pvd != nullptr && transport != nullptr) {
		bool succes = pvd->connect(*transport, physx::PxPvdInstrumentationFlag::eALL);

		if (!succes) {
			std::cerr << "PVD was not create\n";
		}
	}

	physx::PxPhysics* physics = PxCreatePhysics(PX_PHYSICS_VERSION, *foundation, physx::PxTolerancesScale(), true, pvd);

	physx::PxSceneDesc sceneDesc = physx::PxSceneDesc(physics->getTolerancesScale());
	sceneDesc.gravity = physx::PxVec3(0.0f, -9.81f, 0.0f);
	sceneDesc.cpuDispatcher = physx::PxDefaultCpuDispatcherCreate(2);
	sceneDesc.filterShader = physx::PxDefaultSimulationFilterShader;
	physx::PxScene* scene = physics->createScene(sceneDesc);

	if (pvd && pvd->isConnected()) {
		physx::PxPvdSceneClient* pvdClient = scene->getScenePvdClient();

		if (pvdClient) {
			pvdClient->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS, true);
			pvdClient->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_CONTACTS, true);
			pvdClient->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES, true);
		}
	}

	const physx::PxVec3 planeNormal = physx::PxVec3(0.0f, 1.0f, 0.0f);
	physx::PxMaterial* defaultMaterial = physics->createMaterial(0.6f, 0.6f, 0.1f);

	physx::PxPlane plane = physx::PxPlane(planeNormal, 0.0f);
	physx::PxRigidStatic* groundActor = physx::PxCreatePlane(*physics, plane, *defaultMaterial);
	scene->addActor(*groundActor);

	physx::PxBoxGeometry boxGeometry = physx::PxBoxGeometry(physx::PxVec3(0.5f, 0.5f, 0.5f));
	physx::PxShape* boxShape = physics->createShape(boxGeometry, *defaultMaterial, true);
	physx::PxRigidDynamic* boxActor = physics->createRigidDynamic(physx::PxTransform(physx::PxVec3(0.0f, 10.0f, 0.0f)));
	boxActor->attachShape(*boxShape);
	physx::PxRigidBodyExt::updateMassAndInertia(*boxActor, 10.0f);
	scene->addActor(*boxActor);

	for (int i = 0; i < 200; i++) {
		scene->simulate(1.0f / 60.0f);
		scene->fetchResults(true);
	}

	scene->release();
	physics->release();
	foundation->release();

	return 0;
}