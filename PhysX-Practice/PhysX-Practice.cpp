#include <array>
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

constexpr float kBallRadius = 0.0285f;
constexpr float kBallMass = 0.17f;
constexpr float kBallLinearDamping = 1.25f;
constexpr float kBallStopThreshold = 0.05f;
constexpr float kShotSpeed = 3.5f;
constexpr float kAimStep = physx::PxPi / 90.0f;
constexpr float kAimGuideLength = 0.45f;
constexpr float kPocketCaptureRadius = 0.05f;

const physx::PxVec3 kTableColor(0.08f, 0.44f, 0.18f);
const physx::PxVec3 kRailColor(0.42f, 0.23f, 0.10f);
const physx::PxVec3 kObjectBallColor(0.86f, 0.58f, 0.12f);
const physx::PxVec3 kCueBallColor(0.95f, 0.95f, 0.95f);
const physx::PxVec3 kAimColor(0.90f, 0.95f, 1.00f);
const physx::PxVec3 kPocketColor(0.95f, 0.82f, 0.20f);

PhysicsEngine* physicsEngine = nullptr;
Snippets::Camera* camera = nullptr;

float clamp01(float value) {
	if (value < 0.0f) {
		return 0.0f;
	}
	if (value > 1.0f) {
		return 1.0f;
	}
	return value;
}

class BilliardsGame {
public:
	void Initialize() {
		physicsEngine->SetGravity(physx::PxVec3(0.0f, 0.0f, 0.0f));

		tableMaterial_ = physicsEngine->GetMaterial(0.4f, 0.4f, 0.05f);
		railMaterial_ = physicsEngine->GetMaterial(0.2f, 0.2f, 0.92f);
		ballMaterial_ = physicsEngine->GetMaterial(0.05f, 0.05f, 0.96f);

		tableShape_ = physicsEngine->CreateBoxShape(
			physx::PxVec3(kTableLength, kTableSurfaceThickness, kTableWidth),
			tableMaterial_,
			CustomFilterData::eOBSTACLE
		);
		longRailShape_ = physicsEngine->CreateBoxShape(
			physx::PxVec3(kTableLength + 2.0f * kRailThickness, kRailHeight, kRailThickness),
			railMaterial_,
			CustomFilterData::eOBSTACLE
		);
		shortRailShape_ = physicsEngine->CreateBoxShape(
			physx::PxVec3(kRailThickness, kRailHeight, kTableWidth),
			railMaterial_,
			CustomFilterData::eOBSTACLE
		);
		ballShape_ = physicsEngine->CreateSphereShape(kBallRadius, ballMaterial_, CustomFilterData::eDYNAMIC);

		CreateTable();
		Reset();
	}

	void Shutdown() {
		ClearBalls();

		if (ballShape_) {
			ballShape_->release();
			ballShape_ = nullptr;
		}
		if (shortRailShape_) {
			shortRailShape_->release();
			shortRailShape_ = nullptr;
		}
		if (longRailShape_) {
			longRailShape_->release();
			longRailShape_ = nullptr;
		}
		if (tableShape_) {
			tableShape_->release();
			tableShape_ = nullptr;
		}

		tableActor_ = nullptr;
		railActors_.clear();
	}

	void HandleKey(unsigned char key) {
		switch (std::toupper(key)) {
		case 'Q':
		case 'J':
		case '4':
			if (result_ == GameResult::ePLAYING) {
				aimAngle_ += kAimStep;
				WrapAimAngle();
			}
			break;
		case 'E':
		case 'L':
		case '6':
			if (result_ == GameResult::ePLAYING) {
				aimAngle_ -= kAimStep;
				WrapAimAngle();
			}
			break;
		case 'R':
			Reset();
			break;
		case ' ':
			ShootCueBall();
			break;
		default:
			break;
		}
	}

	void RenderFrame() {
		StoreBallPositions();
		physicsEngine->Simulate(kSimulationStep);
		UpdatePocketedBalls();

		Snippets::startRender(camera, 0.1f, 50.0f, 45.0f);
		RenderTable();
		RenderPocketMarkers();
		RenderBalls();
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

	struct Ball {
		physx::PxRigidDynamic* actor = nullptr;
		physx::PxVec3 previousPosition = physx::PxVec3(0.0f);
		bool isCueBall = false;
		bool pocketed = false;
	};

	void CreateTable() {
		tableActor_ = physicsEngine->AddStaticActor(
			tableShape_,
			physx::PxVec3(0.0f, -kTableSurfaceThickness * 0.5f, 0.0f),
			physx::PxQuat(physx::PxIdentity)
		);

		railActors_.push_back(
			physicsEngine->AddStaticActor(
				longRailShape_,
				physx::PxVec3(0.0f, kRailHeight * 0.5f, kTableWidth * 0.5f + kRailThickness * 0.5f),
				physx::PxQuat(physx::PxIdentity)
			)
		);
		railActors_.push_back(
			physicsEngine->AddStaticActor(
				longRailShape_,
				physx::PxVec3(0.0f, kRailHeight * 0.5f, -(kTableWidth * 0.5f + kRailThickness * 0.5f)),
				physx::PxQuat(physx::PxIdentity)
			)
		);
		railActors_.push_back(
			physicsEngine->AddStaticActor(
				shortRailShape_,
				physx::PxVec3(kTableLength * 0.5f + kRailThickness * 0.5f, kRailHeight * 0.5f, 0.0f),
				physx::PxQuat(physx::PxIdentity)
			)
		);
		railActors_.push_back(
			physicsEngine->AddStaticActor(
				shortRailShape_,
				physx::PxVec3(-(kTableLength * 0.5f + kRailThickness * 0.5f), kRailHeight * 0.5f, 0.0f),
				physx::PxQuat(physx::PxIdentity)
			)
		);

		const float halfLength = kTableLength * 0.5f;
		const float halfWidth = kTableWidth * 0.5f;
		const float innerLength = halfLength - kBallRadius;
		const float innerWidth = halfWidth - kBallRadius;
		const float markerHeight = 0.005f;

		pocketCapturePoints_ = {
			physx::PxVec2(innerLength, innerWidth),
			physx::PxVec2(innerLength, -innerWidth),
			physx::PxVec2(-innerLength, innerWidth),
			physx::PxVec2(-innerLength, -innerWidth),
			physx::PxVec2(0.0f, innerWidth),
			physx::PxVec2(0.0f, -innerWidth)
		};

		pocketMarkerPoints_ = {
			physx::PxVec3(halfLength, markerHeight, halfWidth),
			physx::PxVec3(halfLength, markerHeight, -halfWidth),
			physx::PxVec3(-halfLength, markerHeight, halfWidth),
			physx::PxVec3(-halfLength, markerHeight, -halfWidth),
			physx::PxVec3(0.0f, markerHeight, halfWidth),
			physx::PxVec3(0.0f, markerHeight, -halfWidth)
		};
	}

	void Reset() {
		ClearBalls();
		balls_.reserve(16);

		const float diameter = 2.0f * kBallRadius;
		const float rowStep = physx::PxSqrt(3.0f) * kBallRadius;
		const float rackApexX = kTableLength * 0.5f - 0.55f;
		const float cueBallX = -(kTableLength * 0.5f - 0.45f);

		CreateBall(physx::PxVec3(cueBallX, kBallRadius, 0.0f), true);

		for (int row = 0; row < 5; ++row) {
			const float rowX = rackApexX + row * rowStep;
			const float startZ = -row * kBallRadius;
			for (int column = 0; column <= row; ++column) {
				const float z = startZ + column * diameter;
				CreateBall(physx::PxVec3(rowX, kBallRadius, z), false);
			}
		}

		remainingObjectBalls_ = 15;
		result_ = GameResult::ePLAYING;
		aimAngle_ = 0.0f;
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
		actor->setAngularDamping(100.0f);
		actor->setRigidDynamicLockFlag(physx::PxRigidDynamicLockFlag::eLOCK_LINEAR_Y, true);
		actor->setRigidDynamicLockFlag(physx::PxRigidDynamicLockFlag::eLOCK_ANGULAR_X, true);
		actor->setRigidDynamicLockFlag(physx::PxRigidDynamicLockFlag::eLOCK_ANGULAR_Y, true);
		actor->setRigidDynamicLockFlag(physx::PxRigidDynamicLockFlag::eLOCK_ANGULAR_Z, true);
		actor->setLinearVelocity(physx::PxVec3(0.0f));

		Ball ball;
		ball.actor = actor;
		ball.previousPosition = position;
		ball.isCueBall = isCueBall;
		ball.pocketed = false;
		balls_.push_back(ball);
	}

	void StoreBallPositions() {
		for (Ball& ball : balls_) {
			if (ball.actor) {
				ball.previousPosition = ball.actor->getGlobalPose().p;
			}
		}
	}

	void UpdatePocketedBalls() {
		std::vector<Ball*> ballsToPocket;
		for (Ball& ball : balls_) {
			if (!ball.actor) {
				continue;
			}

			const physx::PxVec3 currentPosition = ball.actor->getGlobalPose().p;
			const physx::PxVec2 start(ball.previousPosition.x, ball.previousPosition.z);
			const physx::PxVec2 end(currentPosition.x, currentPosition.z);

			for (const physx::PxVec2& pocketPoint : pocketCapturePoints_) {
				if (DistanceSquaredToSegment(start, end, pocketPoint) <= kPocketCaptureRadius * kPocketCaptureRadius) {
					ballsToPocket.push_back(&ball);
					break;
				}
			}
		}

		for (Ball* ball : ballsToPocket) {
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
		ball.pocketed = true;

		if (!ball.isCueBall && remainingObjectBalls_ > 0) {
			--remainingObjectBalls_;
		}
	}

	void ShootCueBall() {
		if (!CanShoot()) {
			return;
		}

		Ball* cueBall = GetCueBall();
		if (!cueBall || !cueBall->actor) {
			return;
		}

		const physx::PxVec3 shotDirection(physx::PxCos(aimAngle_), 0.0f, physx::PxSin(aimAngle_));
		cueBall->actor->setLinearVelocity(shotDirection * kShotSpeed);
		cueBall->actor->wakeUp();
	}

	bool CanShoot() const {
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

	void RenderTable() const {
		if (tableActor_) {
			physx::PxRigidActor* actor = tableActor_;
			Snippets::renderActors(&actor, 1, false, kTableColor, nullptr, false);
		}

		if (!railActors_.empty()) {
			std::vector<physx::PxRigidActor*> actors;
			actors.reserve(railActors_.size());
			for (physx::PxRigidStatic* rail : railActors_) {
				actors.push_back(rail);
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

	void RenderPocketMarkers() const {
		const physx::PxVec3 alongX(0.035f, 0.0f, 0.0f);
		const physx::PxVec3 alongZ(0.0f, 0.0f, 0.035f);

		for (const physx::PxVec3& point : pocketMarkerPoints_) {
			Snippets::DrawLine(point - alongX, point + alongX, kPocketColor);
			Snippets::DrawLine(point - alongZ, point + alongZ, kPocketColor);
		}
	}

	void RenderAimGuide() const {
		if (!CanShoot()) {
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
		char line[128];
		Snippets::print("Controls: mouse/WASD - camera, Q/E or J/L or 4/6 - aim, Space - shoot, R - reset");

		std::snprintf(line, sizeof(line), "Object balls left: %d", remainingObjectBalls_);
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

	static float DistanceSquaredToSegment(const physx::PxVec2& start, const physx::PxVec2& end, const physx::PxVec2& point) {
		const physx::PxVec2 segment = end - start;
		const float segmentLengthSquared = segment.magnitudeSquared();
		if (segmentLengthSquared < 1e-6f) {
			return (point - start).magnitudeSquared();
		}

		const float projection = (point - start).dot(segment) / segmentLengthSquared;
		const float t = clamp01(projection);
		const physx::PxVec2 closestPoint = start + segment * t;
		return (point - closestPoint).magnitudeSquared();
	}

	physx::PxMaterial* tableMaterial_ = nullptr;
	physx::PxMaterial* railMaterial_ = nullptr;
	physx::PxMaterial* ballMaterial_ = nullptr;

	physx::PxShape* tableShape_ = nullptr;
	physx::PxShape* longRailShape_ = nullptr;
	physx::PxShape* shortRailShape_ = nullptr;
	physx::PxShape* ballShape_ = nullptr;

	physx::PxRigidStatic* tableActor_ = nullptr;
	std::vector<physx::PxRigidStatic*> railActors_;
	std::vector<Ball> balls_;

	std::array<physx::PxVec2, 6> pocketCapturePoints_{};
	std::array<physx::PxVec3, 6> pocketMarkerPoints_{};

	GameResult result_ = GameResult::ePLAYING;
	int remainingObjectBalls_ = 15;
	float aimAngle_ = 0.0f;
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
		physx::PxVec3(0.0f, 2.3f, 3.1f),
		physx::PxVec3(0.0f, -0.45f, -0.90f)
	);
	camera->setSpeed(0.25f);

	Snippets::setupDefault("PhysX Billiards", camera, keyPressedCallback, renderCallback, exitCallback);

	physicsEngine = new PhysicsEngine();
	game = new BilliardsGame();
	game->Initialize();

	glutMainLoop();

	return 0;
}
