#include <iostream>
#include "PhysicsEngine.h"
#include "snippetrender/SnippetRender.h"
#include "snippetrender/SnippetCamera.h"

PhysicsEngine* physicsEngine;
Snippets::Camera* camera;

void keyPressedCallback(unsigned char key, const physx::PxTransform& cameraTransform) {
	std::cout << "\"" << key << "\", " << toupper(key) << '\n';
	switch (toupper(key)) {
	case ' ':
	{
		const static physx::PxVec3 boxSize = physx::PxVec3(1.0f, 1.0f, 1.0f);
		const static physx::PxVec3 startPosition = physx::PxVec3(0.0f, 5.0f, 0.0f);
		const static float boxMass = 10.0f;
		const static float boxDensity = boxMass / PhysicsEngine::GetBoxVolume(boxSize);
		physx::PxShape* boxShape = physicsEngine->CreateBoxShape(boxSize, physicsEngine->GetMaterial(0.5f, 0.5f, 0.1f), CustomFilterData::eDYNAMIC);
		physicsEngine->AddDynamicActor(boxShape, startPosition, physx::PxQuat(1.0f), boxDensity);
	}
	break;
	case 'N':
	{
		physicsEngine->SetFilterShaderConstantBlock(!physicsEngine->GetFilterShaderConstantBlock());
	}
	break;
	default:
		break;
	}
}

void renderCallback() {
	physicsEngine->Simulate(1.0f / 60.0f);

	Snippets::startRender(camera);

	std::vector<physx::PxRigidActor*> actors = physicsEngine->GetActors();
	if (actors.size() > 0) {
		Snippets::renderActors(actors.data(), actors.size());
	}

	Snippets::finishRender();
}

void exitCallback() {
	delete camera;
	delete physicsEngine;
}

int main() {
	camera = new Snippets::Camera(physx::PxVec3(0.0f, 10.0f, 30.0f), physx::PxVec3(0.0f, -0.1f, -0.3f));
	Snippets::setupDefault("PhysX Example", camera, keyPressedCallback, renderCallback, exitCallback);

	physicsEngine = new PhysicsEngine();

	physx::PxMaterial* defaultMaterial = physicsEngine->CreateMaterial(0.5f, 0.5f, 0.1f);

	const physx::PxVec3 groundNormal = physx::PxVec3(0.3f, 1.0f, 0.0f).getNormalized();
	const float groundDistance = 0.0f;
	physicsEngine->AddGround(groundNormal, groundDistance, defaultMaterial);

	const physx::PxVec3 obstacleSize = physx::PxVec3(1.0f, 1.0f, 40.0f);
	const physx::PxVec3 obstaclePosition = physx::PxVec3(-0.1f, 0.0f, 0.0f);
	const physx::PxQuat obstacleRotation = physx::PxQuat(physx::PxPiDivFour, physx::PxVec3(0.0f, 0.0f, 1.0f));
	physx::PxShape* obstacleShape = physicsEngine->CreateBoxShape(obstacleSize, defaultMaterial, CustomFilterData::eOBSTACLE, true);
	physicsEngine->AddStaticActor(obstacleShape, obstaclePosition, obstacleRotation);

	const physx::PxVec3 triggerSize = physx::PxVec3(1.0f, 40.0f, 40.0f);
	const physx::PxVec3 triggerPosition = physx::PxVec3(5.0f, 0.0f, 0.0f);
	physx::PxShape* triggerShape = physicsEngine->CreateBoxShape(triggerSize, defaultMaterial, CustomFilterData::eTRIGGER, true, physx::PxShapeFlag::eTRIGGER_SHAPE);
	physicsEngine->AddStaticActor(triggerShape, triggerPosition, physx::PxQuat(1.0f));

	glutMainLoop();

	return 0;
}