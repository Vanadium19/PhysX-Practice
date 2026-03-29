#include "BilliardsGame.h"
#include "PhysicsEngine.h"
#include "snippetrender/SnippetCamera.h"
#include "snippetrender/SnippetRender.h"

PhysicsEngine* physicsEngine = nullptr;
Snippets::Camera* camera = nullptr;

BilliardsGame* game = nullptr;

void keyPressedCallback(unsigned char key, const physx::PxTransform&) {
	if (game) {
		game->HandleKey(key);
	}
}

void renderCallback() {
	if (game) {
		game->RenderFrame();
	}
}

void exitCallback() {
	delete camera;
	camera = nullptr;

	if (game) {
		game->Shutdown();
		delete game;
		game = nullptr;
	}

	delete physicsEngine;
	physicsEngine = nullptr;
}

int main() {
	camera = new Snippets::Camera(
		physx::PxVec3(0.0f, 2.4f, 3.4f),
		physx::PxVec3(0.0f, -0.46f, -0.89f)
	);
	camera->setSpeed(0.25f);

	Snippets::setupDefault("PhysX Billiards", camera, keyPressedCallback, renderCallback, exitCallback);

	physicsEngine = new PhysicsEngine();
	game = new BilliardsGame(physicsEngine, camera);
	game->Initialize();

	glutMainLoop();

	return 0;
}
