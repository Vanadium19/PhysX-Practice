#include "Cue.h"
#include "Game.h"
#include "BilliardsConfig.h"

#include <snippetrender/SnippetRender.h>

void CustomEventCallback::onTrigger(physx::PxTriggerPair* pairs, uint32_t count) {
	extern Game* game_;
	for (uint32_t i = 0; i < count; i++) {
		const physx::PxTriggerPair& pair = pairs[i];
		switch (pair.status) {
		case physx::PxPairFlag::eNOTIFY_TOUCH_FOUND:
			game_->RemoveBall(pair.otherActor);
			break;
		default:
			break;
		}
	}
}

Game::Game(PhysicsEngine* physicsEngine, Snippets::Camera* camera) {
	physicsEngine_ = physicsEngine;
	camera_ = camera;

	CustomEventCallback* callback = new CustomEventCallback();
	physicsEngine_->SetCallback(callback);
}

void Game::Initialize() {
	CreateTable();

	CreateTrigger();

	InitBallsParams();
	CreateBalls();
}

void Game::HandleKey(unsigned char key) {
	switch (std::toupper(key)) {
	case 'Q':
		if (result_ == GameResult::ePLAYING) {
			aimAngle_ += aimStep;
			WrapAimAngle();
		}
		break;
	case 'E':
		if (result_ == GameResult::ePLAYING) {
			aimAngle_ -= aimStep;
			WrapAimAngle();
		}
		break;
	case 'R':
		Reset();
		break;
	case ' ':
		Shoot();
		break;
	default:
		break;
	}
}

void Game::RenderFrame() {
	physicsEngine_->Simulate(simulationStep);

	Snippets::startRender(camera_);

	if (!rails_.empty()) {
		Snippets::renderActors(rails_.data(), rails_.size(), false, railColor);
	}

	if (!fields_.empty()) {
		Snippets::renderActors(fields_.data(), fields_.size(), false, tableColor);
	}

	if (!balls_.empty()) {
		Snippets::renderActors(balls_.data(), balls_.size(), false, gaimBallColor);
	}

	if (mainBall_) {
		physx::PxRigidActor* ball = mainBall_;
		Snippets::renderActors(&ball, 1, false, mainBallColor);
	}

	Snippets::renderActors(&trigger_, 1, false, mainBallColor);

	RenderHud();
	RenderAimGuide();

	Snippets::finishRender();
}

void Game::Shutdown() {
}

void Game::RemoveBall(physx::PxActor* actor) {
	physicsEngine_->MarkActor(actor);

	if (actor == mainBall_)
	{
		mainBall_ = nullptr;
		result_ = GameResult::eLOST;
		return;
	}

	auto ball = std::find(balls_.begin(), balls_.end(), actor);

	if (ball != balls_.end()) {
		balls_.erase(ball);
	}

	if (balls_.empty()) {
		result_ = GameResult::eLOST;
	}
}

void Game::Reset() {
	aimAngle_ = 0.0f;

	for (auto* actor : balls_) {
		physicsEngine_->MarkActor(actor);
	}

	balls_.clear();

	if (mainBall_)
	{
		physicsEngine_->MarkActor(mainBall_);
		mainBall_ = nullptr;
	}

	result_ = GameResult::ePLAYING;
	CreateBalls();
}

void Game::InitBallsParams()
{
	ballMaterial_ = physicsEngine_->CreateMaterial(0.05f, 0.05f, 0.78f);
	ballShape_ = physicsEngine_->CreateSphereShape(ballRadius, ballMaterial_, CustomFilterData::eBall);
	mainBallShape_ = physicsEngine_->CreateSphereShape(ballRadius, ballMaterial_, CustomFilterData::eMAINBALL);
}


void Game::CreateGameBall(physx::PxVec3 position, float density) {
	physx::PxRigidDynamic* ball = physicsEngine_->AddDynamicActor(ballShape_, position, physx::PxQuat(physx::PxIdentity), density);
	ball->setLinearDamping(ballLinearDamping);
	ball->putToSleep();
	balls_.push_back(ball);
}

void Game::CreateMainBall(const float cueBallX, const float ballY, const float density) {
	mainBall_ = physicsEngine_->AddDynamicActor(ballShape_, physx::PxVec3(cueBallX, ballY, 0.0f), physx::PxQuat(physx::PxIdentity), density);
	mainBall_->setLinearDamping(ballLinearDamping);
}

void Game::CreateBalls() {
	const float ballY = ballRadius + 0.001f;
	const float density = ballMass / PhysicsEngine::GetSphereVolume(ballRadius);

	const float cueBallX = -(tableLength * 0.5f - 0.45f);
	CreateMainBall(cueBallX, ballY, density);

	const float rackApexX = tableLength * 0.5f - 0.55f;
	const float diameter = 2.0f * ballRadius;
	const float rowStep = physx::PxSqrt(3.0f) * ballRadius;

	for (int row = 0; row < 5; ++row) {
		const float rowX = rackApexX + row * rowStep;
		const float startZ = -row * ballRadius;

		for (int column = 0; column <= row; ++column) {
			const float z = startZ + column * diameter;
			CreateGameBall(physx::PxVec3(rowX, ballY, z), density);
		}
	}
}

void Game::CreateTable() {
	const float halfLength = tableLength * 0.5f;
	const float halfWidth = tableWidth * 0.5f;

	const float sidePocketHalfWidth = sidePocketWidth * 0.5f;
	const float sideFloorWidth = halfLength - cornerPocketCut - sidePocketHalfWidth;
	const float endFloorDepth = tableWidth - 2.0f * cornerPocketCut;

	const float railY = railHeight * 0.5f;
	const float sideFloorCenterX = sidePocketHalfWidth + sideFloorWidth * 0.5f;
	const float sideFloorCenterZ = halfWidth + railThickness * 0.5f;
	const float frontFloorCenterX = halfLength + railThickness * 0.5f;

	physx::PxMaterial* railMaterial = physicsEngine_->CreateMaterial(0.20f, 0.20f, 0.88f);

	CreateTablePart(rails_, physx::PxVec3(sideFloorWidth, railHeight, railThickness), physx::PxVec3(-sideFloorCenterX, railY, sideFloorCenterZ), railMaterial);
	CreateTablePart(rails_, physx::PxVec3(sideFloorWidth, railHeight, railThickness), physx::PxVec3(sideFloorCenterX, railY, sideFloorCenterZ), railMaterial);
	CreateTablePart(rails_, physx::PxVec3(sideFloorWidth, railHeight, railThickness), physx::PxVec3(-sideFloorCenterX, railY, -sideFloorCenterZ), railMaterial);
	CreateTablePart(rails_, physx::PxVec3(sideFloorWidth, railHeight, railThickness), physx::PxVec3(sideFloorCenterX, railY, -sideFloorCenterZ), railMaterial);

	CreateTablePart(rails_, physx::PxVec3(railThickness, railHeight, endFloorDepth), physx::PxVec3(-frontFloorCenterX, railY, 0.0f), railMaterial);
	CreateTablePart(rails_, physx::PxVec3(railThickness, railHeight, endFloorDepth), physx::PxVec3(frontFloorCenterX, railY, 0.0f), railMaterial);

	CreateTablePart(rails_, physx::PxVec3(tableLength + 2.0f * railThickness, railHeight, railThickness), physx::PxVec3(0.0f, railY, halfWidth + railThickness), railMaterial);
	CreateTablePart(rails_, physx::PxVec3(tableLength + 2.0f * railThickness, railHeight, railThickness), physx::PxVec3(0.0f, railY, -halfWidth - railThickness), railMaterial);

	CreateTablePart(rails_, physx::PxVec3(railThickness, railHeight, tableWidth + 2.0f * railThickness), physx::PxVec3(halfLength + railThickness, railY, 0.0f), railMaterial);
	CreateTablePart(rails_, physx::PxVec3(railThickness, railHeight, tableWidth + 2.0f * railThickness), physx::PxVec3(-halfLength - railThickness, railY, 0.0f), railMaterial);

	const float surfaceY = -tableSurfaceThickness * 0.5f;
	const float centerFloorDepth = tableWidth - 2.0f * sidePocketDepth;

	physx::PxMaterial* clothMaterial = physicsEngine_->CreateMaterial(0.60f, 0.60f, 0.00f);

	CreateTablePart(fields_, physx::PxVec3(sideFloorWidth, tableSurfaceThickness, tableWidth), physx::PxVec3(-sideFloorCenterX, surfaceY, 0.0f), clothMaterial);
	CreateTablePart(fields_, physx::PxVec3(sideFloorWidth, tableSurfaceThickness, tableWidth), physx::PxVec3(sideFloorCenterX, surfaceY, 0.0f), clothMaterial);
	CreateTablePart(fields_, physx::PxVec3(sidePocketWidth, tableSurfaceThickness, centerFloorDepth), physx::PxVec3(0.0f, surfaceY, 0.0f), clothMaterial);
	CreateTablePart(fields_, physx::PxVec3(cornerPocketCut, tableSurfaceThickness, endFloorDepth), physx::PxVec3(-(halfLength - cornerPocketCut * 0.5f), surfaceY, 0.0f), clothMaterial);
	CreateTablePart(fields_, physx::PxVec3(cornerPocketCut, tableSurfaceThickness, endFloorDepth), physx::PxVec3(halfLength - cornerPocketCut * 0.5f, surfaceY, 0.0f), clothMaterial);
}

void Game::CreateTablePart(std::vector<physx::PxRigidActor*>& actors, physx::PxVec3 size, physx::PxVec3 position, physx::PxMaterial* material) {
	physx::PxShape* shape = physicsEngine_->CreateBoxShape(size, material, CustomFilterData::eOBSTACLE);
	physx::PxRigidActor* actor = physicsEngine_->AddStaticActor(shape, position, physx::PxQuat(physx::PxIdentity));
	actors.push_back(actor);
}

void Game::CreateTrigger()
{
	physx::PxMaterial* material = physicsEngine_->CreateMaterial(0.0f, 0.0f, 0.0f);

	const physx::PxVec3 triggerSize = physx::PxVec3(tableWidth * 100, tableSurfaceThickness * 100, tableLength * 100);
	const physx::PxVec3 triggerPosition = physx::PxVec3(0.0f, -tableSurfaceThickness * 100 - 2.0f, 0.0f);

	physx::PxShape* triggerShape = physicsEngine_->CreateBoxShape(triggerSize, material, CustomFilterData::eTRIGGER, true, physx::PxShapeFlag::eTRIGGER_SHAPE);
	trigger_ = physicsEngine_->AddStaticActor(triggerShape, triggerPosition, physx::PxQuat(physx::PxIdentity));
}

void Game::WrapAimAngle() {
	const float fullTurn = 2.0f * physx::PxPi;
	while (aimAngle_ >= fullTurn) {
		aimAngle_ -= fullTurn;
	}
	while (aimAngle_ < 0.0f) {
		aimAngle_ += fullTurn;
	}
}

void Game::Shoot() {
	if (!CanShoot()) {
		return;
	}

	const physx::PxVec3 shotDirection(physx::PxCos(aimAngle_), 0.0f, physx::PxSin(aimAngle_));
	mainBall_->setLinearVelocity(shotDirection * shotSpeed);
	mainBall_->wakeUp();
}

bool Game::CanShoot() {
	return result_ == GameResult::ePLAYING && BallsAreStopped();
}

bool Game::BallsAreStopped() {
	for (auto* actor : balls_) {
		if (!actor) {
			continue;
		}

		physx::PxRigidDynamic* ball = static_cast<physx::PxRigidDynamic*>(actor);
		float vel = ball->getLinearVelocity().magnitude();

		if (vel > ballStopThreshold) {
			return false;
		}
	}

	if (!mainBall_) {
		return false;
	}

	physx::PxRigidDynamic* ball = static_cast<physx::PxRigidDynamic*>(mainBall_);
	bool res = ball->getLinearVelocity().magnitude() <= ballStopThreshold;
	return res;
}

void Game::RenderAimGuide() {
	if (!CanShoot()) {
		return;
	}

	const physx::PxVec3 start = mainBall_->getGlobalPose().p;
	const physx::PxVec3 direction(physx::PxCos(aimAngle_), 0.0f, physx::PxSin(aimAngle_));
	Snippets::DrawLine(start, start + direction * aimGuideLength, aimColor);
}

void Game::RenderHud() {
	char line[128];
	Snippets::print("Controls: mouse/WASD - camera, Q/E - aim, Space - shoot, R - reset");

	std::snprintf(line, sizeof(line), "Object balls left: %d", balls_.size());
	Snippets::print(line);

	const float angleDegrees = aimAngle_ * 180.0f / physx::PxPi;
	std::snprintf(line, sizeof(line), "Aim angle: %.0f deg", angleDegrees);
	Snippets::print(line);

	switch (result_) {
	case GameResult::ePLAYING:
		if (CanShoot()) {
			Snippets::print("Ready for the next shot.");
		}
		else {
			Snippets::print("Balls are moving. Wait until the table settles.");
		}
		break;
	case GameResult::eWON:
		Snippets::print("Victory: every object ball is pocketed. Press R to play again.");
		break;
	case GameResult::eLOST:
		Snippets::print("Defeat: the cue ball was pocketed. Press R to restart.");
		break;
	}
}