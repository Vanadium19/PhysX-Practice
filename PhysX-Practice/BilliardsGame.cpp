#include "BilliardsGame.h"

#include <cctype>
#include <cstdio>

#include "BilliardsConfig.h"
#include "PhysicsEngine.h"
#include "snippetrender/SnippetRender.h"

namespace {
	float clampRange(float value, float minimum, float maximum) {
		if (value < minimum) {
			return minimum;
		}
		if (value > maximum) {
			return maximum;
		}
		return value;
	}
}

BilliardsGame::BilliardsGame(PhysicsEngine* physicsEngine, Snippets::Camera* camera)
	: physicsEngine_(physicsEngine), camera_(camera), cuePullback_(cuePullbackDefault) {
}

void BilliardsGame::Initialize() {
	physicsEngine_->SetGravity(physx::PxVec3(0.0f, -9.81f, 0.0f));

	clothMaterial_ = physicsEngine_->GetMaterial(0.60f, 0.60f, 0.00f);
	railMaterial_ = physicsEngine_->GetMaterial(0.20f, 0.20f, 0.88f);
	ballMaterial_ = physicsEngine_->GetMaterial(0.05f, 0.05f, 0.78f);
	cueMaterial_ = physicsEngine_->GetMaterial(0.25f, 0.25f, 0.20f);

	ballShape_ = physicsEngine_->CreateSphereShape(ballRadius, ballMaterial_, CustomFilterData::eDYNAMIC);
	cueShape_ = physicsEngine_->CreateBoxShape(
		physx::PxVec3(cueLength, cueHeight, cueThickness),
		physx::PxVec3(-cueLength * 0.5f, 0.0f, 0.0f),
		physx::PxQuat(physx::PxIdentity),
		cueMaterial_,
		CustomFilterData::eOBSTACLE
	);

	CreateTable();
	CreateCue();
	Reset();
}

void BilliardsGame::Shutdown() {
	ClearBalls();

	if (cueActor_) {
		physicsEngine_->RemoveActor(cueActor_);
		cueActor_->release();
		cueActor_ = nullptr;
	}

	if (cueShape_) {
		cueShape_->release();
		cueShape_ = nullptr;
	}
	if (ballShape_) {
		ballShape_->release();
		ballShape_ = nullptr;
	}

	for (physx::PxShape* shape : staticShapes_) {
		if (shape) {
			shape->release();
		}
	}
	staticShapes_.clear();

	fieldActors_.clear();
	railActors_.clear();
	pitActor_ = nullptr;
}

void BilliardsGame::HandleKey(unsigned char key) {
	switch (std::toupper(key)) {
	case 'Q':
	case 'J':
	case '4':
		if (result_ == GameResult::ePLAYING && cueState_ != CueState::eSTRIKING) {
			aimAngle_ += aimStep;
			WrapAimAngle();
		}
		break;
	case 'E':
	case 'L':
	case '6':
		if (result_ == GameResult::ePLAYING && cueState_ != CueState::eSTRIKING) {
			aimAngle_ -= aimStep;
			WrapAimAngle();
		}
		break;
	case 'Z':
	case '2':
		if (result_ == GameResult::ePLAYING && cueState_ != CueState::eSTRIKING) {
			AdjustCuePullback(-cuePullbackStep);
		}
		break;
	case 'X':
	case '8':
		if (result_ == GameResult::ePLAYING && cueState_ != CueState::eSTRIKING) {
			AdjustCuePullback(cuePullbackStep);
		}
		break;
	case 'R':
		Reset();
		break;
	case ' ':
		StartCueStrike();
		break;
	default:
		break;
	}
}

void BilliardsGame::RenderFrame() {
	UpdateCue(simulationStep);
	physicsEngine_->Simulate(simulationStep);
	UpdatePocketedBalls();

	Snippets::startRender(camera_, 0.1f, 60.0f, 45.0f);
	std::vector<physx::PxRigidActor*> actors = physicsEngine_->GetActors();
	if (!actors.empty()) {
		Snippets::renderActors(actors.data(), static_cast<physx::PxU32>(actors.size()));
	}
	RenderAimGuide();
	RenderHud();
	Snippets::finishRender();
}

void BilliardsGame::CreateTable() {
	const float halfLength = tableLength * 0.5f;
	const float halfWidth = tableWidth * 0.5f;
	const float surfaceY = -tableSurfaceThickness * 0.5f;
	const float railY = railHeight * 0.5f;
	const float sidePocketHalfWidth = sidePocketWidth * 0.5f;

	const float sideFloorWidth = halfLength - cornerPocketCut - sidePocketHalfWidth;
	const float sideFloorCenterX = sidePocketHalfWidth + sideFloorWidth * 0.5f;
	const float endFloorDepth = tableWidth - 2.0f * cornerPocketCut;
	const float centerFloorDepth = tableWidth - 2.0f * sidePocketDepth;

	AddStaticBox(
		fieldActors_,
		physx::PxVec3(sideFloorWidth, tableSurfaceThickness, tableWidth),
		physx::PxVec3(-sideFloorCenterX, surfaceY, 0.0f),
		clothMaterial_
	);
	AddStaticBox(
		fieldActors_,
		physx::PxVec3(sideFloorWidth, tableSurfaceThickness, tableWidth),
		physx::PxVec3(sideFloorCenterX, surfaceY, 0.0f),
		clothMaterial_
	);
	AddStaticBox(
		fieldActors_,
		physx::PxVec3(sidePocketWidth, tableSurfaceThickness, centerFloorDepth),
		physx::PxVec3(0.0f, surfaceY, 0.0f),
		clothMaterial_
	);
	AddStaticBox(
		fieldActors_,
		physx::PxVec3(cornerPocketCut, tableSurfaceThickness, endFloorDepth),
		physx::PxVec3(-(halfLength - cornerPocketCut * 0.5f), surfaceY, 0.0f),
		clothMaterial_
	);
	AddStaticBox(
		fieldActors_,
		physx::PxVec3(cornerPocketCut, tableSurfaceThickness, endFloorDepth),
		physx::PxVec3(halfLength - cornerPocketCut * 0.5f, surfaceY, 0.0f),
		clothMaterial_
	);

	AddStaticBox(
		railActors_,
		physx::PxVec3(sideFloorWidth, railHeight, railThickness),
		physx::PxVec3(-sideFloorCenterX, railY, halfWidth + railThickness * 0.5f),
		railMaterial_
	);
	AddStaticBox(
		railActors_,
		physx::PxVec3(sideFloorWidth, railHeight, railThickness),
		physx::PxVec3(sideFloorCenterX, railY, halfWidth + railThickness * 0.5f),
		railMaterial_
	);
	AddStaticBox(
		railActors_,
		physx::PxVec3(sideFloorWidth, railHeight, railThickness),
		physx::PxVec3(-sideFloorCenterX, railY, -(halfWidth + railThickness * 0.5f)),
		railMaterial_
	);
	AddStaticBox(
		railActors_,
		physx::PxVec3(sideFloorWidth, railHeight, railThickness),
		physx::PxVec3(sideFloorCenterX, railY, -(halfWidth + railThickness * 0.5f)),
		railMaterial_
	);
	AddStaticBox(
		railActors_,
		physx::PxVec3(railThickness, railHeight, endFloorDepth),
		physx::PxVec3(-(halfLength + railThickness * 0.5f), railY, 0.0f),
		railMaterial_
	);
	AddStaticBox(
		railActors_,
		physx::PxVec3(railThickness, railHeight, endFloorDepth),
		physx::PxVec3(halfLength + railThickness * 0.5f, railY, 0.0f),
		railMaterial_
	);

	physx::PxShape* pitShape = physicsEngine_->CreateBoxShape(
		physx::PxVec3(tableLength, pitThickness, tableWidth),
		clothMaterial_,
		CustomFilterData::eOBSTACLE
	);
	staticShapes_.push_back(pitShape);
	pitActor_ = physicsEngine_->AddStaticActor(
		pitShape,
		physx::PxVec3(0.0f, -pitDepth, 0.0f),
		physx::PxQuat(physx::PxIdentity)
	);
}

void BilliardsGame::CreateCue() {
	const float cueMass = 0.60f;
	const float cueDensity = cueMass / PhysicsEngine::GetBoxVolume(physx::PxVec3(cueLength, cueHeight, cueThickness));

	cueActor_ = physicsEngine_->AddDynamicActor(
		cueShape_,
		HiddenCuePosition(),
		physx::PxQuat(physx::PxIdentity),
		cueDensity
	);
	cueActor_->setLinearDamping(0.0f);
	cueActor_->setAngularDamping(100.0f);
	cueActor_->setActorFlag(physx::PxActorFlag::eDISABLE_GRAVITY, true);
	cueActor_->setRigidBodyFlag(physx::PxRigidBodyFlag::eENABLE_CCD, true);
	cueActor_->setRigidBodyFlag(physx::PxRigidBodyFlag::eENABLE_SPECULATIVE_CCD, true);
	cueActor_->setRigidDynamicLockFlag(physx::PxRigidDynamicLockFlag::eLOCK_LINEAR_Y, true);
	cueActor_->setRigidDynamicLockFlag(physx::PxRigidDynamicLockFlag::eLOCK_ANGULAR_X, true);
	cueActor_->setRigidDynamicLockFlag(physx::PxRigidDynamicLockFlag::eLOCK_ANGULAR_Y, true);
	cueActor_->setRigidDynamicLockFlag(physx::PxRigidDynamicLockFlag::eLOCK_ANGULAR_Z, true);
	ParkCue(true);
}

void BilliardsGame::Reset() {
	ClearBalls();
	balls_.reserve(16);

	const float diameter = 2.0f * ballRadius;
	const float rowStep = physx::PxSqrt(3.0f) * ballRadius;
	const float rackApexX = tableLength * 0.5f - 0.55f;
	const float cueBallX = -(tableLength * 0.5f - 0.45f);
	const float ballY = ballRadius + 0.001f;

	CreateBall(physx::PxVec3(cueBallX, ballY, 0.0f), true);

	for (int row = 0; row < 5; ++row) {
		const float rowX = rackApexX + row * rowStep;
		const float startZ = -row * ballRadius;
		for (int column = 0; column <= row; ++column) {
			const float z = startZ + column * diameter;
			CreateBall(physx::PxVec3(rowX, ballY, z), false);
		}
	}

	remainingObjectBalls_ = 15;
	result_ = GameResult::ePLAYING;
	aimAngle_ = 0.0f;
	cuePullback_ = cuePullbackDefault;
	cueStrikeTimer_ = 0.0f;
	cueState_ = CueState::eAIMING;

	Ball* cueBall = GetCueBall();
	if (cueBall && cueBall->actor) {
		SetCuePose(cueBall->actor->getGlobalPose().p, cuePullback_, true);
	}
	else {
		ParkCue(true);
	}
}

void BilliardsGame::ClearBalls() {
	for (Ball& ball : balls_) {
		if (ball.actor) {
			physicsEngine_->RemoveActor(ball.actor);
			ball.actor->release();
			ball.actor = nullptr;
		}
	}
	balls_.clear();
}

void BilliardsGame::CreateBall(const physx::PxVec3& position, bool isCueBall) {
	const float density = ballMass / PhysicsEngine::GetSphereVolume(ballRadius);
	physx::PxRigidDynamic* actor = physicsEngine_->AddDynamicActor(
		ballShape_,
		position,
		physx::PxQuat(physx::PxIdentity),
		density
	);

	actor->setLinearDamping(ballLinearDamping);
	actor->setAngularDamping(20.0f);
	actor->setRigidBodyFlag(physx::PxRigidBodyFlag::eENABLE_CCD, true);
	actor->setRigidBodyFlag(physx::PxRigidBodyFlag::eENABLE_SPECULATIVE_CCD, true);
	actor->setRigidDynamicLockFlag(physx::PxRigidDynamicLockFlag::eLOCK_ANGULAR_X, true);
	actor->setRigidDynamicLockFlag(physx::PxRigidDynamicLockFlag::eLOCK_ANGULAR_Y, true);
	actor->setRigidDynamicLockFlag(physx::PxRigidDynamicLockFlag::eLOCK_ANGULAR_Z, true);
	actor->setLinearVelocity(physx::PxVec3(0.0f));
	actor->setAngularVelocity(physx::PxVec3(0.0f));

	Ball ball;
	ball.actor = actor;
	ball.isCueBall = isCueBall;
	balls_.push_back(ball);
}

void BilliardsGame::AddStaticBox(
	std::vector<physx::PxRigidStatic*>& actors,
	const physx::PxVec3& size,
	const physx::PxVec3& position,
	physx::PxMaterial* material
) {
	physx::PxShape* shape = physicsEngine_->CreateBoxShape(size, material, CustomFilterData::eOBSTACLE);
	staticShapes_.push_back(shape);
	actors.push_back(physicsEngine_->AddStaticActor(shape, position, physx::PxQuat(physx::PxIdentity)));
}

void BilliardsGame::UpdateCue(float dt) {
	if (!cueActor_) {
		return;
	}

	if (cueState_ == CueState::eSTRIKING) {
		cueStrikeTimer_ += dt;
		if (cueStrikeTimer_ >= cueStrikeDuration) {
			cueState_ = CueState::eHIDDEN;
			ParkCue(true);
		}
		return;
	}

	if (CanManipulateCue()) {
		Ball* cueBall = GetCueBall();
		if (cueBall && cueBall->actor) {
			cueState_ = CueState::eAIMING;
			SetCuePose(cueBall->actor->getGlobalPose().p, cuePullback_, true);
			return;
		}
	}

	cueState_ = CueState::eHIDDEN;
	ParkCue(true);
}

void BilliardsGame::UpdatePocketedBalls() {
	std::vector<Ball*> pocketedBalls;
	for (Ball& ball : balls_) {
		if (!ball.actor) {
			continue;
		}

		if (ball.actor->getGlobalPose().p.y < pocketDestroyY) {
			pocketedBalls.push_back(&ball);
		}
	}

	for (Ball* ball : pocketedBalls) {
		PocketBall(*ball);
	}

	if (!GetCueBall()) {
		result_ = GameResult::eLOST;
		return;
	}

	if (remainingObjectBalls_ == 0) {
		result_ = GameResult::eWON;
	}
}

void BilliardsGame::PocketBall(Ball& ball) {
	if (!ball.actor) {
		return;
	}

	physicsEngine_->RemoveActor(ball.actor);
	ball.actor->release();
	ball.actor = nullptr;

	if (!ball.isCueBall && remainingObjectBalls_ > 0) {
		--remainingObjectBalls_;
	}
}

void BilliardsGame::StartCueStrike() {
	if (!CanManipulateCue()) {
		return;
	}

	Ball* cueBall = GetCueBall();
	if (!cueBall || !cueBall->actor) {
		return;
	}

	SetCuePose(cueBall->actor->getGlobalPose().p, cuePullback_, true);

	const physx::PxVec3 direction = GetAimDirection();
	cueActor_->setLinearVelocity(direction * GetCueStrikeSpeed());
	cueActor_->wakeUp();

	cueStrikeTimer_ = 0.0f;
	cueState_ = CueState::eSTRIKING;
}

bool BilliardsGame::CanManipulateCue() const {
	return result_ == GameResult::ePLAYING && GetCueBall() && BallsAreStopped();
}

bool BilliardsGame::BallsAreStopped() const {
	for (const Ball& ball : balls_) {
		if (!ball.actor) {
			continue;
		}

		if (ball.actor->getLinearVelocity().magnitude() > ballStopThreshold) {
			return false;
		}
	}
	return true;
}

void BilliardsGame::AdjustCuePullback(float delta) {
	cuePullback_ = clampRange(cuePullback_ + delta, cuePullbackMin, cuePullbackMax);
}

void BilliardsGame::WrapAimAngle() {
	const float fullTurn = 2.0f * physx::PxPi;
	while (aimAngle_ >= fullTurn) {
		aimAngle_ -= fullTurn;
	}
	while (aimAngle_ < 0.0f) {
		aimAngle_ += fullTurn;
	}
}

BilliardsGame::Ball* BilliardsGame::GetCueBall() {
	for (Ball& ball : balls_) {
		if (ball.isCueBall && ball.actor) {
			return &ball;
		}
	}
	return nullptr;
}

const BilliardsGame::Ball* BilliardsGame::GetCueBall() const {
	for (const Ball& ball : balls_) {
		if (ball.isCueBall && ball.actor) {
			return &ball;
		}
	}
	return nullptr;
}

float BilliardsGame::GetCueStrikeSpeed() const {
	return 2.8f + 8.0f * cuePullback_;
}

void BilliardsGame::SetCuePose(const physx::PxVec3& cueBallPosition, float pullbackOffset, bool teleport) {
	const physx::PxVec3 direction = GetAimDirection();
	const physx::PxVec3 tipAnchor = cueBallPosition - direction * (ballRadius + cueTipGap + pullbackOffset);
	const physx::PxQuat rotation(-aimAngle_, physx::PxVec3(0.0f, 1.0f, 0.0f));
	ApplyCuePose(physx::PxTransform(tipAnchor, rotation), teleport);
}

void BilliardsGame::ParkCue(bool teleport) {
	ApplyCuePose(physx::PxTransform(HiddenCuePosition(), physx::PxQuat(physx::PxIdentity)), teleport);
}

void BilliardsGame::ApplyCuePose(const physx::PxTransform& pose, bool teleport) {
	if (!cueActor_) {
		return;
	}

	cueActor_->setLinearVelocity(physx::PxVec3(0.0f));
	cueActor_->setAngularVelocity(physx::PxVec3(0.0f));
	cueActor_->setGlobalPose(pose);
	if (teleport) {
		cueActor_->putToSleep();
	}
}

physx::PxVec3 BilliardsGame::GetAimDirection() const {
	return physx::PxVec3(physx::PxCos(aimAngle_), 0.0f, physx::PxSin(aimAngle_));
}

physx::PxVec3 BilliardsGame::HiddenCuePosition() const {
	return physx::PxVec3(0.0f, 1.5f, -4.0f);
}

void BilliardsGame::RenderAimGuide() const {
	if (!CanManipulateCue()) {
		return;
	}

	const Ball* cueBall = GetCueBall();
	if (!cueBall || !cueBall->actor) {
		return;
	}

	const physx::PxVec3 start = cueBall->actor->getGlobalPose().p;
	const physx::PxVec3 direction(physx::PxCos(aimAngle_), 0.0f, physx::PxSin(aimAngle_));
	Snippets::DrawLine(start, start + direction * aimGuideLength, aimColor);
}

void BilliardsGame::RenderHud() const {
	char line[160];
	Snippets::print("Controls: mouse/WASD camera, Q/E or J/L or 4/6 aim, Z/X or 2/8 cue pullback, Space strike, R reset");

	std::snprintf(line, sizeof(line), "Object balls left: %d", remainingObjectBalls_);
	Snippets::print(line);

	const float angleDegrees = aimAngle_ * 180.0f / physx::PxPi;
	std::snprintf(line, sizeof(line), "Aim angle: %.0f deg", angleDegrees);
	Snippets::print(line);

	std::snprintf(line, sizeof(line), "Cue pullback: %.2f", cuePullback_);
	Snippets::print(line);

	switch (result_) {
	case GameResult::ePLAYING:
		if (cueState_ == CueState::eSTRIKING || !CanManipulateCue()) {
			Snippets::print("Balls are moving. Wait until every ball stops.");
		}
		else {
			Snippets::print("Cue is ready. Strike the cue ball with Space.");
		}
		break;
	case GameResult::eWON:
		Snippets::print("Victory: every object ball fell into a pocket. Press R to play again.");
		break;
	case GameResult::eLOST:
		Snippets::print("Defeat: the cue ball fell into a pocket. Press R to restart.");
		break;
	}
}
