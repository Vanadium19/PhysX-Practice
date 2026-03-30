#include "Game.h"

#include "PhysicsEngine.h"

#include "snippetrender/SnippetCamera.h"
#include "snippetrender/SnippetRender.h"

PhysicsEngine* physicsEngine_ = nullptr;
Snippets::Camera* camera_ = nullptr;

Game* game_ = nullptr;

void keyPressedCallback(unsigned char key, const physx::PxTransform&) {
	if (game_) {
		game_->HandleKey(key);
	}
}

void renderCallback() {
	if (game_) {
		game_->RenderFrame();
	}
}

void exitCallback() {
	delete camera_;
	camera_ = nullptr;

	if (game_) {
		game_->Shutdown();
		delete game_;
		game_ = nullptr;
	}

	delete physicsEngine_;
	physicsEngine_ = nullptr;
}

int main() {
	camera_ = new Snippets::Camera(
		physx::PxVec3(0.0f, 2.0f, 0.0f),
		physx::PxVec3(0.0f, -0.99f, -0.89f)
	);
	camera_->setSpeed(0.25f);

	Snippets::setupDefault("PhysX Billiards", camera_, keyPressedCallback, renderCallback, exitCallback);

	physicsEngine_ = new PhysicsEngine();
	game_ = new Game(physicsEngine_, camera_);
	game_->Initialize();

	glutMainLoop();

	return 0;
}
