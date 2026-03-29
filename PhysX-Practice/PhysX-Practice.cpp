#include <cctype>
#include <cstdio>
#include <vector>

#include "PhysicsEngine.h"
#include "snippetrender/SnippetCamera.h"
#include "snippetrender/SnippetRender.h"

constexpr float kSimulationStep = 1.0f / 60.0f;

constexpr float kTableLength = 2.54f;
constexpr float kTableWidth = 1.27f;
constexpr float kTableSurfaceThickness = 0.04f;
constexpr float kRailThickness = 0.08f;
constexpr float kRailHeight = 0.10f;
constexpr float kPitDepth = 0.25f;
constexpr float kPitThickness = 0.02f;

constexpr float kCornerPocketCut = 0.12f;
constexpr float kSidePocketWidth = 0.18f;
constexpr float kSidePocketDepth = 0.12f;

constexpr float kBallRadius = 0.0285f;
constexpr float kBallMass = 0.17f;
constexpr float kBallLinearDamping = 0.75f;
constexpr float kBallStopThreshold = 0.06f;
constexpr float kPocketDestroyY = -0.10f;

constexpr float kCueLength = 0.42f;
constexpr float kCueHeight = 0.016f;
constexpr float kCueThickness = 0.014f;
constexpr float kCueTipGap = 0.01f;
constexpr float kCuePullbackMin = 0.08f;
constexpr float kCuePullbackMax = 0.18f;
constexpr float kCuePullbackDefault = 0.12f;
constexpr float kCuePullbackStep = 0.01f;
constexpr float kCueStrikeDuration = 0.18f;
constexpr float kAimStep = physx::PxPi / 90.0f;
constexpr float kAimGuideLength = 0.50f;

const physx::PxVec3 kTableColor(0.08f, 0.44f, 0.18f);
const physx::PxVec3 kRailColor(0.42f, 0.23f, 0.10f);
const physx::PxVec3 kPocketColor(0.08f, 0.08f, 0.08f);
const physx::PxVec3 kObjectBallColor(0.86f, 0.58f, 0.12f);
const physx::PxVec3 kCueBallColor(0.95f, 0.95f, 0.95f);
const physx::PxVec3 kCueColor(0.74f, 0.58f, 0.34f);
const physx::PxVec3 kAimColor(0.90f, 0.95f, 1.00f);

PhysicsEngine* physicsEngine = nullptr;
Snippets::Camera* camera = nullptr;

float clampRange(float value, float minimum, float maximum) {
	if (value < minimum) {
		return minimum;
	}
	if (value > maximum) {
		return maximum;
	}
	return value;
}

class BilliardsGame {
public:
	void Initialize() {
		physicsEngine->SetGravity(physx::PxVec3(0.0f, -9.81f, 0.0f));

		clothMaterial_ = physicsEngine->GetMaterial(0.60f, 0.60f, 0.00f);
		railMaterial_ = physicsEngine->GetMaterial(0.20f, 0.20f, 0.88f);
		ballMaterial_ = physicsEngine->GetMaterial(0.05f, 0.05f, 0.78f);
		cueMaterial_ = physicsEngine->GetMaterial(0.25f, 0.25f, 0.20f);

		ballShape_ = physicsEngine->CreateSphereShape(kBallRadius, ballMaterial_, CustomFilterData::eDYNAMIC);
		cueShape_ = physicsEngine->CreateBoxShape(
			physx::PxVec3(kCueLength, kCueHeight, kCueThickness),
			cueMaterial_,
			CustomFilterData::eOBSTACLE
		);

		CreateTable();
		CreateCue();
		Reset();
	}

	void Shutdown() {
		ClearBalls();

		if (cueActor_) {
			physicsEngine->RemoveActor(cueActor_);
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

	void HandleKey(unsigned char key) {
		switch (std::toupper(key)) {
		case 'Q':
		case 'J':
		case '4':
			if (result_ == GameResult::ePLAYING && cueState_ != CueState::eSTRIKING) {
				aimAngle_ += kAimStep;
				WrapAimAngle();
			}
			break;
		case 'E':
		case 'L':
		case '6':
			if (result_ == GameResult::ePLAYING && cueState_ != CueState::eSTRIKING) {
				aimAngle_ -= kAimStep;
				WrapAimAngle();
			}
			break;
		case 'Z':
		case '2':
			if (result_ == GameResult::ePLAYING && cueState_ != CueState::eSTRIKING) {
				AdjustCuePullback(-kCuePullbackStep);
			}
			break;
		case 'X':
		case '8':
			if (result_ == GameResult::ePLAYING && cueState_ != CueState::eSTRIKING) {
				AdjustCuePullback(kCuePullbackStep);
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

	void RenderFrame() {
		UpdateCue(kSimulationStep);
		physicsEngine->Simulate(kSimulationStep);
		UpdatePocketedBalls();

		Snippets::startRender(camera, 0.1f, 60.0f, 45.0f);
		RenderTable();
		RenderBalls();
		RenderCue();
		RenderAimGuide();
		RenderHud();
		Snippets::finishRender();
	}

private:
	enum class GameResult {
		ePLAYING,
		eWON,
		eLOST
	};

	enum class CueState {
		eAIMING,
		eSTRIKING,
		eHIDDEN
	};

	struct Ball {
		physx::PxRigidDynamic* actor = nullptr;
		bool isCueBall = false;
	};

	void CreateTable() {
		const float halfLength = kTableLength * 0.5f;
		const float halfWidth = kTableWidth * 0.5f;
		const float surfaceY = -kTableSurfaceThickness * 0.5f;
		const float railY = kRailHeight * 0.5f;
		const float sidePocketHalfWidth = kSidePocketWidth * 0.5f;

		const float sideFloorWidth = halfLength - kCornerPocketCut - sidePocketHalfWidth;
		const float sideFloorCenterX = sidePocketHalfWidth + sideFloorWidth * 0.5f;
		const float endFloorDepth = kTableWidth - 2.0f * kCornerPocketCut;
		const float centerFloorDepth = kTableWidth - 2.0f * kSidePocketDepth;

		AddStaticBox(
			fieldActors_,
			physx::PxVec3(sideFloorWidth, kTableSurfaceThickness, kTableWidth),
			physx::PxVec3(-sideFloorCenterX, surfaceY, 0.0f),
			clothMaterial_
		);
		AddStaticBox(
			fieldActors_,
			physx::PxVec3(sideFloorWidth, kTableSurfaceThickness, kTableWidth),
			physx::PxVec3(sideFloorCenterX, surfaceY, 0.0f),
			clothMaterial_
		);
		AddStaticBox(
			fieldActors_,
			physx::PxVec3(kSidePocketWidth, kTableSurfaceThickness, centerFloorDepth),
			physx::PxVec3(0.0f, surfaceY, 0.0f),
			clothMaterial_
		);
		AddStaticBox(
			fieldActors_,
			physx::PxVec3(kCornerPocketCut, kTableSurfaceThickness, endFloorDepth),
			physx::PxVec3(-(halfLength - kCornerPocketCut * 0.5f), surfaceY, 0.0f),
			clothMaterial_
		);
		AddStaticBox(
			fieldActors_,
			physx::PxVec3(kCornerPocketCut, kTableSurfaceThickness, endFloorDepth),
			physx::PxVec3(halfLength - kCornerPocketCut * 0.5f, surfaceY, 0.0f),
			clothMaterial_
		);

		AddStaticBox(
			railActors_,
			physx::PxVec3(sideFloorWidth, kRailHeight, kRailThickness),
			physx::PxVec3(-sideFloorCenterX, railY, halfWidth + kRailThickness * 0.5f),
			railMaterial_
		);
		AddStaticBox(
			railActors_,
			physx::PxVec3(sideFloorWidth, kRailHeight, kRailThickness),
			physx::PxVec3(sideFloorCenterX, railY, halfWidth + kRailThickness * 0.5f),
			railMaterial_
		);
		AddStaticBox(
			railActors_,
			physx::PxVec3(sideFloorWidth, kRailHeight, kRailThickness),
			physx::PxVec3(-sideFloorCenterX, railY, -(halfWidth + kRailThickness * 0.5f)),
			railMaterial_
		);
		AddStaticBox(
			railActors_,
			physx::PxVec3(sideFloorWidth, kRailHeight, kRailThickness),
			physx::PxVec3(sideFloorCenterX, railY, -(halfWidth + kRailThickness * 0.5f)),
			railMaterial_
		);
		AddStaticBox(
			railActors_,
			physx::PxVec3(kRailThickness, kRailHeight, endFloorDepth),
			physx::PxVec3(-(halfLength + kRailThickness * 0.5f), railY, 0.0f),
			railMaterial_
		);
		AddStaticBox(
			railActors_,
			physx::PxVec3(kRailThickness, kRailHeight, endFloorDepth),
			physx::PxVec3(halfLength + kRailThickness * 0.5f, railY, 0.0f),
			railMaterial_
		);

		physx::PxShape* pitShape = physicsEngine->CreateBoxShape(
			physx::PxVec3(kTableLength, kPitThickness, kTableWidth),
			clothMaterial_,
			CustomFilterData::eOBSTACLE
		);
		staticShapes_.push_back(pitShape);
		pitActor_ = physicsEngine->AddStaticActor(
			pitShape,
			physx::PxVec3(0.0f, -kPitDepth, 0.0f),
			physx::PxQuat(physx::PxIdentity)
		);
	}

	void CreateCue() {
		const float cueMass = 0.60f;
		const float cueDensity = cueMass / PhysicsEngine::GetBoxVolume(physx::PxVec3(kCueLength, kCueHeight, kCueThickness));

		cueActor_ = physicsEngine->AddDynamicActor(
			cueShape_,
			HiddenCuePosition(),
			physx::PxQuat(physx::PxIdentity),
			cueDensity
		);
		cueActor_->setLinearDamping(0.0f);
		cueActor_->setAngularDamping(100.0f);
		cueActor_->setActorFlag(physx::PxActorFlag::eDISABLE_GRAVITY, true);
		cueActor_->setRigidDynamicLockFlag(physx::PxRigidDynamicLockFlag::eLOCK_LINEAR_Y, true);
		cueActor_->setRigidDynamicLockFlag(physx::PxRigidDynamicLockFlag::eLOCK_ANGULAR_X, true);
		cueActor_->setRigidDynamicLockFlag(physx::PxRigidDynamicLockFlag::eLOCK_ANGULAR_Y, true);
		cueActor_->setRigidDynamicLockFlag(physx::PxRigidDynamicLockFlag::eLOCK_ANGULAR_Z, true);
		PrepareCueForAiming();
		ParkCue(true);
	}

	void Reset() {
		ClearBalls();
		balls_.reserve(16);

		const float diameter = 2.0f * kBallRadius;
		const float rowStep = physx::PxSqrt(3.0f) * kBallRadius;
		const float rackApexX = kTableLength * 0.5f - 0.55f;
		const float cueBallX = -(kTableLength * 0.5f - 0.45f);
		const float ballY = kBallRadius + 0.001f;

		CreateBall(physx::PxVec3(cueBallX, ballY, 0.0f), true);

		for (int row = 0; row < 5; ++row) {
			const float rowX = rackApexX + row * rowStep;
			const float startZ = -row * kBallRadius;
			for (int column = 0; column <= row; ++column) {
				const float z = startZ + column * diameter;
				CreateBall(physx::PxVec3(rowX, ballY, z), false);
			}
		}

		remainingObjectBalls_ = 15;
		result_ = GameResult::ePLAYING;
		aimAngle_ = 0.0f;
		cuePullback_ = kCuePullbackDefault;
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

	void ClearBalls() {
		for (Ball& ball : balls_) {
			if (ball.actor) {
				physicsEngine->RemoveActor(ball.actor);
				ball.actor->release();
				ball.actor = nullptr;
			}
		}
		balls_.clear();
	}

	void CreateBall(const physx::PxVec3& position, bool isCueBall) {
		const float density = kBallMass / PhysicsEngine::GetSphereVolume(kBallRadius);
		physx::PxRigidDynamic* actor = physicsEngine->AddDynamicActor(
			ballShape_,
			position,
			physx::PxQuat(physx::PxIdentity),
			density
		);

		actor->setLinearDamping(kBallLinearDamping);
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

	void AddStaticBox(
		std::vector<physx::PxRigidStatic*>& actors,
		const physx::PxVec3& size,
		const physx::PxVec3& position,
		physx::PxMaterial* material
	) {
		physx::PxShape* shape = physicsEngine->CreateBoxShape(size, material, CustomFilterData::eOBSTACLE);
		staticShapes_.push_back(shape);
		actors.push_back(physicsEngine->AddStaticActor(shape, position, physx::PxQuat(physx::PxIdentity)));
	}

	void UpdateCue(float dt) {
		if (!cueActor_) {
			return;
		}

		if (cueState_ == CueState::eSTRIKING) {
			cueStrikeTimer_ += dt;
			if (cueStrikeTimer_ >= kCueStrikeDuration) {
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

	void UpdatePocketedBalls() {
		std::vector<Ball*> pocketedBalls;
		for (Ball& ball : balls_) {
			if (!ball.actor) {
				continue;
			}

			if (ball.actor->getGlobalPose().p.y < kPocketDestroyY) {
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

	void PocketBall(Ball& ball) {
		if (!ball.actor) {
			return;
		}

		physicsEngine->RemoveActor(ball.actor);
		ball.actor->release();
		ball.actor = nullptr;

		if (!ball.isCueBall && remainingObjectBalls_ > 0) {
			--remainingObjectBalls_;
		}
	}

	void StartCueStrike() {
		if (!CanManipulateCue()) {
			return;
		}

		Ball* cueBall = GetCueBall();
		if (!cueBall || !cueBall->actor) {
			return;
		}

		SetCuePose(cueBall->actor->getGlobalPose().p, cuePullback_, true);
		PrepareCueForStrike();

		const physx::PxVec3 direction = GetAimDirection();
		cueActor_->setLinearVelocity(direction * GetCueStrikeSpeed());
		cueActor_->wakeUp();

		cueStrikeTimer_ = 0.0f;
		cueState_ = CueState::eSTRIKING;
	}

	bool CanManipulateCue() const {
		return result_ == GameResult::ePLAYING && GetCueBall() && BallsAreStopped();
	}

	bool BallsAreStopped() const {
		for (const Ball& ball : balls_) {
			if (!ball.actor) {
				continue;
			}

			if (ball.actor->getLinearVelocity().magnitude() > kBallStopThreshold) {
				return false;
			}
		}
		return true;
	}

	void AdjustCuePullback(float delta) {
		cuePullback_ = clampRange(cuePullback_ + delta, kCuePullbackMin, kCuePullbackMax);
	}

	void WrapAimAngle() {
		const float fullTurn = 2.0f * physx::PxPi;
		while (aimAngle_ >= fullTurn) {
			aimAngle_ -= fullTurn;
		}
		while (aimAngle_ < 0.0f) {
			aimAngle_ += fullTurn;
		}
	}

	Ball* GetCueBall() {
		for (Ball& ball : balls_) {
			if (ball.isCueBall && ball.actor) {
				return &ball;
			}
		}
		return nullptr;
	}

	const Ball* GetCueBall() const {
		for (const Ball& ball : balls_) {
			if (ball.isCueBall && ball.actor) {
				return &ball;
			}
		}
		return nullptr;
	}

	float GetCueStrikeSpeed() const {
		return 1.8f + 6.0f * cuePullback_;
	}

	void SetCuePose(const physx::PxVec3& cueBallPosition, float pullbackOffset, bool teleport) {
		PrepareCueForAiming();
		const physx::PxVec3 direction = GetAimDirection();
		const physx::PxVec3 tipAnchor = cueBallPosition - direction * (kBallRadius + kCueTipGap + pullbackOffset);
		const physx::PxVec3 center = tipAnchor - direction * (kCueLength * 0.5f);
		const physx::PxQuat rotation(aimAngle_, physx::PxVec3(0.0f, 1.0f, 0.0f));
		ApplyCuePose(physx::PxTransform(center, rotation), teleport);
	}

	void ParkCue(bool teleport) {
		PrepareCueForAiming();
		ApplyCuePose(physx::PxTransform(HiddenCuePosition(), physx::PxQuat(physx::PxIdentity)), teleport);
	}

	void ApplyCuePose(const physx::PxTransform& pose, bool teleport) {
		if (!cueActor_) {
			return;
		}

		if (teleport) {
			cueActor_->setGlobalPose(pose);
		}
		cueActor_->setKinematicTarget(pose);
	}

	void PrepareCueForAiming() {
		if (!cueActor_) {
			return;
		}

		cueActor_->setLinearVelocity(physx::PxVec3(0.0f));
		cueActor_->setAngularVelocity(physx::PxVec3(0.0f));
		cueActor_->setRigidBodyFlag(physx::PxRigidBodyFlag::eKINEMATIC, true);
	}

	void PrepareCueForStrike() {
		if (!cueActor_) {
			return;
		}

		cueActor_->setLinearVelocity(physx::PxVec3(0.0f));
		cueActor_->setAngularVelocity(physx::PxVec3(0.0f));
		cueActor_->setRigidBodyFlag(physx::PxRigidBodyFlag::eKINEMATIC, false);
		cueActor_->setRigidBodyFlag(physx::PxRigidBodyFlag::eENABLE_CCD, true);
		cueActor_->setRigidBodyFlag(physx::PxRigidBodyFlag::eENABLE_SPECULATIVE_CCD, true);
	}

	physx::PxVec3 GetAimDirection() const {
		return physx::PxVec3(physx::PxCos(aimAngle_), 0.0f, physx::PxSin(aimAngle_));
	}

	physx::PxVec3 HiddenCuePosition() const {
		return physx::PxVec3(0.0f, 1.5f, -4.0f);
	}

	void RenderTable() const {
		if (pitActor_) {
			physx::PxRigidActor* actor = pitActor_;
			Snippets::renderActors(&actor, 1, false, kPocketColor, nullptr, false);
		}

		if (!fieldActors_.empty()) {
			std::vector<physx::PxRigidActor*> actors;
			actors.reserve(fieldActors_.size());
			for (physx::PxRigidStatic* actor : fieldActors_) {
				actors.push_back(actor);
			}
			Snippets::renderActors(actors.data(), static_cast<physx::PxU32>(actors.size()), false, kTableColor, nullptr, false);
		}

		if (!railActors_.empty()) {
			std::vector<physx::PxRigidActor*> actors;
			actors.reserve(railActors_.size());
			for (physx::PxRigidStatic* actor : railActors_) {
				actors.push_back(actor);
			}
			Snippets::renderActors(actors.data(), static_cast<physx::PxU32>(actors.size()), false, kRailColor, nullptr, false);
		}
	}

	void RenderBalls() const {
		std::vector<physx::PxRigidActor*> objectBalls;
		std::vector<physx::PxRigidActor*> cueBall;

		for (const Ball& ball : balls_) {
			if (!ball.actor) {
				continue;
			}

			if (ball.isCueBall) {
				cueBall.push_back(ball.actor);
			}
			else {
				objectBalls.push_back(ball.actor);
			}
		}

		if (!objectBalls.empty()) {
			Snippets::renderActors(objectBalls.data(), static_cast<physx::PxU32>(objectBalls.size()), false, kObjectBallColor, nullptr, false);
		}

		if (!cueBall.empty()) {
			Snippets::renderActors(cueBall.data(), static_cast<physx::PxU32>(cueBall.size()), false, kCueBallColor, nullptr, false);
		}
	}

	void RenderCue() const {
		if (!cueActor_ || cueState_ == CueState::eHIDDEN) {
			return;
		}

		physx::PxRigidActor* actor = cueActor_;
		Snippets::renderActors(&actor, 1, false, kCueColor, nullptr, false);
	}

	void RenderAimGuide() const {
		if (!CanManipulateCue()) {
			return;
		}

		const Ball* cueBall = GetCueBall();
		if (!cueBall || !cueBall->actor) {
			return;
		}

		const physx::PxVec3 start = cueBall->actor->getGlobalPose().p;
		const physx::PxVec3 direction(physx::PxCos(aimAngle_), 0.0f, physx::PxSin(aimAngle_));
		Snippets::DrawLine(start, start + direction * kAimGuideLength, kAimColor);
	}

	void RenderHud() const {
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

	physx::PxMaterial* clothMaterial_ = nullptr;
	physx::PxMaterial* railMaterial_ = nullptr;
	physx::PxMaterial* ballMaterial_ = nullptr;
	physx::PxMaterial* cueMaterial_ = nullptr;

	std::vector<physx::PxShape*> staticShapes_;
	physx::PxShape* ballShape_ = nullptr;
	physx::PxShape* cueShape_ = nullptr;

	std::vector<physx::PxRigidStatic*> fieldActors_;
	std::vector<physx::PxRigidStatic*> railActors_;
	physx::PxRigidStatic* pitActor_ = nullptr;
	physx::PxRigidDynamic* cueActor_ = nullptr;
	std::vector<Ball> balls_;

	GameResult result_ = GameResult::ePLAYING;
	CueState cueState_ = CueState::eHIDDEN;
	int remainingObjectBalls_ = 15;
	float aimAngle_ = 0.0f;
	float cuePullback_ = kCuePullbackDefault;
	float cueStrikeTimer_ = 0.0f;
};

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
	game = new BilliardsGame();
	game->Initialize();

	glutMainLoop();

	return 0;
}
