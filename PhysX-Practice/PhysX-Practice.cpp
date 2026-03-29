#include <iostream>
#include "PxPhysicsAPI.h"

#include "snippetrender/SnippetRender.h"
#include "snippetrender/SnippetCamera.h"

Snippets::Camera* camera;

physx::PxDefaultAllocator allocator;
physx::PxDefaultErrorCallback errorCallback;

physx::PxFoundation* foundation;
physx::PxPhysics* physics;
physx::PxScene* scene;
physx::PxArray<physx::PxRigidActor*> actors;

void initPhysics() {
	foundation = PxCreateFoundation(PX_PHYSICS_VERSION, allocator, errorCallback);

	physx::PxPvd* pvd = physx::PxCreatePvd(*foundation);
	physx::PxPvdTransport* transport = physx::PxDefaultPvdSocketTransportCreate("127.0.0.1", 5425, 10000);

	if (pvd != nullptr && transport != nullptr) {
		bool succes = pvd->connect(*transport, physx::PxPvdInstrumentationFlag::eALL);

		if (!succes) {
			std::cerr << "PVD was not create\n";
		}
	}

	physics = PxCreatePhysics(PX_PHYSICS_VERSION, *foundation, physx::PxTolerancesScale(), true, pvd);

	physx::PxSceneDesc sceneDesc = physx::PxSceneDesc(physics->getTolerancesScale());
	sceneDesc.gravity = physx::PxVec3(0.0f, -9.81f, 0.0f);
	sceneDesc.cpuDispatcher = physx::PxDefaultCpuDispatcherCreate(2);
	sceneDesc.filterShader = physx::PxDefaultSimulationFilterShader;
	scene = physics->createScene(sceneDesc);

	if (pvd && pvd->isConnected()) {
		physx::PxPvdSceneClient* pvdClient = scene->getScenePvdClient();

		if (pvdClient) {
			pvdClient->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS, true);
			pvdClient->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_CONTACTS, true);
			pvdClient->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES, true);
		}
	}

	physx::PxMaterial* rockMaterial = physics->createMaterial(0.5f, 0.5f, 0.1f);
	physx::PxMaterial* metalMaterial = physics->createMaterial(0.15f, 0.15f, 0.1f);
	physx::PxMaterial* iceMaterial = physics->createMaterial(0.028f, 0.028f, 0.1f);

	const physx::PxVec3 planeNormal = physx::PxVec3(0.3f, 1.0f, 0.0f).getNormalized();

	physx::PxPlane plane = physx::PxPlane(planeNormal, 0.0f);
	physx::PxRigidStatic* groundActor = physx::PxCreatePlane(*physics, plane, *rockMaterial);
	scene->addActor(*groundActor);
	actors.pushBack(groundActor);

	physx::PxBoxGeometry boxGeometry = physx::PxBoxGeometry(physx::PxVec3(0.5f, 0.5f, 0.5f));

	physx::PxShape* rockBoxShape = physics->createShape(boxGeometry, *rockMaterial, true);
	physx::PxRigidDynamic* rockBoxActor = physics->createRigidDynamic(physx::PxTransform(physx::PxVec3(-2.0f, 2.0f, -2.0f)));
	rockBoxActor->attachShape(*rockBoxShape);
	physx::PxRigidBodyExt::updateMassAndInertia(*rockBoxActor, 10.0f);
	scene->addActor(*rockBoxActor);
	actors.pushBack(rockBoxActor);

	physx::PxShape* metalBoxShape = physics->createShape(boxGeometry, *metalMaterial, true);
	physx::PxRigidDynamic* metalBoxActor = physics->createRigidDynamic(physx::PxTransform(physx::PxVec3(0.0f, 2.0f, 0.0f)));
	metalBoxActor->attachShape(*metalBoxShape);
	physx::PxRigidBodyExt::updateMassAndInertia(*metalBoxActor, 10.0f);
	scene->addActor(*metalBoxActor);
	actors.pushBack(metalBoxActor);

	physx::PxShape* iceBoxShape = physics->createShape(boxGeometry, *iceMaterial, true);
	physx::PxRigidDynamic* iceBoxActor = physics->createRigidDynamic(physx::PxTransform(physx::PxVec3(0.0f, 2.0f, 2.0f)));
	iceBoxActor->attachShape(*iceBoxShape);
	physx::PxRigidBodyExt::updateMassAndInertia(*iceBoxActor, 10.0f);
	scene->addActor(*iceBoxActor);
	actors.pushBack(iceBoxActor);
}

void keyPress(unsigned char key, const physx::PxTransform& camera) {
	std::cout << toupper(key) << "\n";
}

void exitCallback() {
	delete camera;

	scene->release();
	physics->release();
	foundation->release();
}

void renderCallback() {
	scene->simulate(1.0f / 60.0f);
	scene->fetchResults(true);

	Snippets::startRender(camera);

	if (actors.size() > 0) {
		Snippets::renderActors(&actors[0], actors.size(), false);
	}

	Snippets::finishRender();
}

int main() {
	camera = new Snippets::Camera(physx::PxVec3(0.0f, 20.0f, 20.0f), physx::PxVec3(0.0f, -1.0f, -1.0f));

	Snippets::setupDefault("PhysX test", camera, keyPress, renderCallback, exitCallback);

	initPhysics();

	glutMainLoop();

	return 0;
}