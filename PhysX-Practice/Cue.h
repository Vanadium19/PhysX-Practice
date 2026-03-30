#pragma once

#include "PhysicsEngine.h"

#include "PxPhysicsAPI.h"

enum class CueState {
	eAIMING,
	eSTRIKING,
	eHIDDEN,
};

class Cue {
public:
	Cue(PhysicsEngine* physicsEngine);

	CueState State();

	void Update(float deltaTime, physx::PxRigidActor* ball);

	void IncreaseAimAngle();
	void DecreaseAimAngle();

	void StartCueStrike(physx::PxRigidActor* ball);
	void AdjustCuePullback(float delta);

	void Render();

private:
	bool CanManipulate();
	bool BallsAreStopped();
	void WrapAimAngle();
	void SetPositionAroundBall(const physx::PxVec3& position, float pullbackOffset, bool teleport);
	void Park(bool teleport);
	void SetPosition(physx::PxTransform transform, bool teleport);
	float Clamp(float value, float minimum, float maximum);

	PhysicsEngine* physicsEngine_;
	physx::PxRigidDynamic* actor_;
};

