#include "PhysicsEngine.h"
#include "ShooterGame.h"

#include "snippetrender/SnippetCamera.h"
#include "snippetrender/SnippetRender.h"

PhysicsEngine* physicsEngine_ = nullptr;
Snippets::Camera* camera_ = nullptr;
ShooterGame* game_ = nullptr;

void keyPressedCallback(unsigned char key, const physx::PxTransform& cameraTransform) {
	if (game_) {
		game_->HandleKey(key, cameraTransform);
	}
}

void renderCallback() {
	if (game_) {
		game_->RenderFrame();
	}
}

void exitCallback() {
	delete game_;
	game_ = nullptr;

	delete camera_;
	camera_ = nullptr;

	delete physicsEngine_;
	physicsEngine_ = nullptr;
}

int main() {
	camera_ = new Snippets::Camera(
		physx::PxVec3(-12.0f, 8.0f, 18.0f),
		physx::PxVec3(0.52f, -0.22f, -0.82f)
	);
	camera_->setSpeed(0.35f);

	Snippets::setupDefault("Homework 2 Shooter", camera_, keyPressedCallback, renderCallback, exitCallback);

	physicsEngine_ = new PhysicsEngine();
	game_ = new ShooterGame(physicsEngine_, camera_);
	game_->Initialize();

	glutMainLoop();

	return 0;
}
