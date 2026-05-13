#include "PhysicsEngine.h"
#include "ShooterGame.h"
#include "GameConstants.h"

#include "snippetrender/SnippetCamera.h"
#include "snippetrender/SnippetRender.h"

/// Глобальный экземпляр физического движка, живущий в течение всей программы.
PhysicsEngine* physicsEngine_ = nullptr;

/// Камера игрока, используемая как для рендера, так и для логики AI.
Snippets::Camera* camera_ = nullptr;

/// Основной игровой объект, управляющий сценой.
ShooterGame* game_ = nullptr;

/// Callback обработки нажатия игровых клавиш.
void keyPressedCallback(unsigned char key, const physx::PxTransform& cameraTransform) {
	if (game_) {
		game_->HandleKey(key, cameraTransform);
	}
}

/// Callback отрисовки одного кадра.
void renderCallback() {
	if (game_) {
		game_->RenderFrame();
	}
}

/// Callback корректного завершения приложения и освобождения ресурсов.
void exitCallback() {
	delete game_;
	game_ = nullptr;

	delete camera_;
	camera_ = nullptr;

	delete physicsEngine_;
	physicsEngine_ = nullptr;
}

/// Точка входа приложения.
///
/// Создаёт камеру, окно, физический движок и игровой объект,
/// после чего передаёт управление в главный цикл `GLUT`.
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
