#pragma once

#include "PhysicsEngine.h"

#include "snippetrender/SnippetCamera.h"

enum class GameResult {
	ePLAYING,
	eWON,
	eLOST,
};

class CustomEventCallback : public physx::PxSimulationEventCallback {
public:
	virtual void onConstraintBreak(physx::PxConstraintInfo* constraints, uint32_t count) override {};
	virtual void onWake(physx::PxActor** actors, uint32_t count) override {};
	virtual void onSleep(physx::PxActor** actors, uint32_t count) override {};
	virtual void onContact(const physx::PxContactPairHeader& pairHeader, const physx::PxContactPair* pairs, uint32_t nbPairs) override {};
	virtual void onTrigger(physx::PxTriggerPair* pairs, uint32_t count) override;
	virtual void onAdvance(const physx::PxRigidBody* const* bodyBuffer, const physx::PxTransform* poseBuffer, const uint32_t count) override {};

};

class BilliardsGame {
public:
	BilliardsGame(PhysicsEngine* physicsEngine, Snippets::Camera* camera);

	void Initialize();
	void HandleKey(unsigned char key);
	void DecreaseAim();
	void IncreaseAim();
	void RenderFrame();
	void Shutdown();

	void RemoveBall(physx::PxActor* actor);

private:
	void Reset();

	void CreateMainBall(const float cueBallX, const float ballY, const float density);
	void CreateGameBall(physx::PxVec3 position, float density);
	void CreateBalls();

	void CreateTable();
	void CreateTablePart(std::vector<physx::PxRigidActor*>& actors, physx::PxVec3 size, physx::PxVec3 position, physx::PxMaterial* material);

	void CreateTrigger();

	void WrapAimAngle();
	void Shoot();
	bool CanShoot();
	bool BallsAreStopped();

	void RenderAimGuide();
	void RenderHud();

	PhysicsEngine* physicsEngine_;
	Snippets::Camera* camera_;

	physx::PxRigidActor* trigger_;

	std::vector<physx::PxRigidActor*> rails_;
	std::vector<physx::PxRigidActor*> fields_;

	std::vector<physx::PxRigidActor*> balls_;
	physx::PxRigidDynamic* mainBall_;

	physx::PxShape* ballShape_;

	float aimAngle_ = 0.0f;

	GameResult result_ = GameResult::ePLAYING;
};
