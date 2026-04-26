#include <iostream>
#include "PhysicsEngine.h"
#include "snippetrender/SnippetRender.h"
#include "snippetrender/SnippetCamera.h"

PhysicsEngine* physicsEngine;
Snippets::Camera* camera;
uint32_t frameCounter;
uint32_t hitFrame;

void keyPressedCallback(unsigned char key, const physx::PxTransform& cameraTransform) {
	std::cout << "\"" << key << "\", " << toupper(key) << '\n';
	switch (toupper(key)) {
	case ' ':
	{
		static physx::PxRaycastHit hits[32];
		physx::PxRaycastBuffer buffer(hits, 32);
		if (physicsEngine->Raycast(cameraTransform.p, camera->getDir(), 10e5, buffer)) {
			bool hasHit = false;
			uint32_t hitsCount = buffer.getNbTouches();
			for (int i = 0; i < hitsCount; i++) {
				const physx::PxRaycastHit& hit = buffer.getTouch(i);
				std::cout << "Raycast hit at position (" << hit.position.x << ", " << hit.position.y << ", " << hit.position.z << ")\n";
				physx::PxActorType::Enum type = hit.actor->getType();
				if (!hasHit && type == physx::PxActorType::eRIGID_DYNAMIC) {
					physicsEngine->MarkActor(hit.actor);
					hasHit = true;
				}
			}
		}
		hitFrame = frameCounter;
	}
	break;
	case 'B':
	{
		static float projectileVelocity = 100.0f;
		static float projectileRadius = 0.25f;
		static physx::PxMaterial* projectileMaterial = physicsEngine->CreateMaterial(0.1f, 0.1f, 0.7f);
		static physx::PxShape* projectileShape = physicsEngine->CreateSphereShape(projectileRadius, projectileMaterial, CustomFilterData::eDYNAMIC);
		physx::PxRigidDynamic* projectileActor = physicsEngine->AddDynamicActor(projectileShape, cameraTransform.p, physx::PxQuat(1.0f), 300.0f);
		projectileActor->setLinearVelocity(camera->getDir() * projectileVelocity);
	}
	break;
	default:
		break;
	}
}

void renderCallback() {
	physicsEngine->Simulate(1.0f / 60.0f);

	float nearClip = 0.1f;
	float farClip = 10000.0f;
	Snippets::startRender(camera, nearClip, farClip);

	std::vector<physx::PxRigidActor*> actors = physicsEngine->GetActors();
	if (actors.size() > 0) {
		Snippets::renderActors(actors.data(), actors.size());
	}

	static const uint32_t maxFrames = 10;
	if (frameCounter < hitFrame + maxFrames) {
		physx::PxVec3 upVector = physx::PxVec3(0.0f, 1.0f, 0.0f);
		physx::PxVec3 rightVector = camera->getDir().cross(upVector);
		physx::PxVec3 startLine = camera->getEye() + 0.01f * rightVector - 0.02f * upVector;
		physx::PxVec3 endLine = startLine + camera->getDir() * 1000.0f;
		Snippets::DrawLine(startLine, endLine, physx::PxVec3(1.0f, 1.0f, 1.0f));
	}

	Snippets::finishRender();

	frameCounter++;
}

void exitCallback() {
	delete camera;
	delete physicsEngine;
}

int main() {
	frameCounter = 0;

	camera = new Snippets::Camera(physx::PxVec3(0.0f, 10.0f, 30.0f), physx::PxVec3(0.0f, -0.1f, -0.3f));
	Snippets::setupDefault("PhysX Example", camera, keyPressedCallback, renderCallback, exitCallback);

	physicsEngine = new PhysicsEngine();

	physx::PxMaterial* defaultMaterial = physicsEngine->CreateMaterial(0.5f, 0.5f, 0.1f);

	physicsEngine->AddGround(physx::PxVec3(0.0f, 1.0f, 0.0f), 0.0f, defaultMaterial);

	const physx::PxVec3 targetSize = physx::PxVec3(1.0f);
	const std::vector<physx::PxVec3> targetPositions = {
		physx::PxVec3(-2.0f, 0.75f, 0.0f),
		physx::PxVec3(-1.0f, 0.75f, 0.0f),
		physx::PxVec3(0.0f, 0.75f, 0.0f),
		physx::PxVec3(1.0f, 0.75f, 0.0f),
		physx::PxVec3(2.0f, 0.75f, 0.0f),
		physx::PxVec3(-1.5f, 1.75f, 0.0f),
		physx::PxVec3(-0.5f, 1.75f, 0.0f),
		physx::PxVec3(0.5f, 1.75f, 0.0f),
		physx::PxVec3(1.5f, 1.75f, 0.0f),
		physx::PxVec3(-1.0f, 2.75f, 0.0f),
		physx::PxVec3(0.0f, 2.75f, 0.0f),
		physx::PxVec3(1.0f, 2.75f, 0.0f),
		physx::PxVec3(-0.5f, 3.75f, 0.0f),
		physx::PxVec3(0.5f, 3.75f, 0.0f),
		physx::PxVec3(0.0f, 4.75f, 0.0f)
	};
	physx::PxShape* targetShape = physicsEngine->CreateBoxShape(targetSize, defaultMaterial, CustomFilterData::eOBSTACLE);
	for (physx::PxVec3 position : targetPositions) {
		physicsEngine->AddDynamicActor(targetShape, position, physx::PxQuat(1.0f), 500.0f);
	}

	glutMainLoop();

	return 0;
}
