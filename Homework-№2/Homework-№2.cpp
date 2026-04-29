#include "PhysicsEngine.h"
#include "ShooterGame.h"
#include "GameConstants.h"

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
		GameConstants::AppConfig::CameraPosition,
		GameConstants::AppConfig::CameraDirection
	);
	camera_->setSpeed(GameConstants::AppConfig::CameraSpeed);

	Snippets::setupDefault(
		GameConstants::AppConfig::WindowTitle,
		camera_,
		keyPressedCallback,
		renderCallback,
		exitCallback
	);

	physicsEngine_ = new PhysicsEngine();
	game_ = new ShooterGame(physicsEngine_, camera_);
	game_->Initialize();

	glutMainLoop();

	return 0;
}
