#pragma once

#include <vector>

#include "PxPhysicsAPI.h"

class PhysicsEngine;

namespace Snippets {
	class Camera;
}

class BilliardsGame {
public:
	BilliardsGame(PhysicsEngine* physicsEngine, Snippets::Camera* camera);

	void Initialize();
	void Shutdown();
	void HandleKey(unsigned char key);
	void RenderFrame();

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

	void CreateTable();
	void CreateCue();
	void Reset();
	void ClearBalls();
	void CreateBall(const physx::PxVec3& position, bool isCueBall);
	void AddStaticBox(
		std::vector<physx::PxRigidStatic*>& actors,
		const physx::PxVec3& size,
		const physx::PxVec3& position,
		physx::PxMaterial* material
	);
	void UpdateCue(float dt);
	void UpdatePocketedBalls();
	void PocketBall(Ball& ball);
	void StartCueStrike();
	bool CanManipulateCue() const;
	bool BallsAreStopped() const;
	void AdjustCuePullback(float delta);
	void WrapAimAngle();
	Ball* GetCueBall();
	const Ball* GetCueBall() const;
	float GetCueStrikeSpeed() const;
	void SetCuePose(const physx::PxVec3& cueBallPosition, float pullbackOffset, bool teleport);
	void ParkCue(bool teleport);
	void ApplyCuePose(const physx::PxTransform& pose, bool teleport);
	physx::PxVec3 GetAimDirection() const;
	physx::PxVec3 HiddenCuePosition() const;
	void RenderTable() const;
	void RenderBalls() const;
	void RenderCue() const;
	void RenderAimGuide() const;
	void RenderHud() const;

	PhysicsEngine* physicsEngine_ = nullptr;
	Snippets::Camera* camera_ = nullptr;

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
	float cuePullback_ = 0.0f;
	float cueStrikeTimer_ = 0.0f;
};
