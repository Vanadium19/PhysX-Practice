#include "PhysicsEngine.h"
#include "snippetrender/SnippetCamera.h"
#include "snippetrender/SnippetRender.h"

PhysicsEngine* physicsEngine = nullptr;
Snippets::Camera* camera = nullptr;

void keyPressedCallback(unsigned char key, const physx::PxTransform& cameraTransform) {
	(void)key;
	(void)cameraTransform;
}

void renderCallback() {
	physicsEngine->Simulate(1.0f / 60.0f);

	const float nearClip = 0.1f;
	const float farClip = 10000.0f;
	Snippets::startRender(camera, nearClip, farClip);

	std::vector<physx::PxRigidActor*> actors = physicsEngine->GetActors();
	if (!actors.empty()) {
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
	Snippets::setupDefault("Homework 2", camera, keyPressedCallback, renderCallback, exitCallback);

	physicsEngine = new PhysicsEngine();

	glutMainLoop();

	return 0;
}
