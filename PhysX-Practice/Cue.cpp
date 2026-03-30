#include "Cue.h"

#include <PxPhysicsAPI.h>
#include <snippetrender/SnippetRender.h>

#include "BilliardsConfig.h"
#include "PhysicsEngine.h"

CueState state_ = CueState::eHIDDEN;

float strikeTimer_ = 0.0f;
float cuePullback_ = 0.0f;
float aimAngle_ = 0.0f;

Cue::Cue(PhysicsEngine* physicsEngine) {
	physicsEngine_ = physicsEngine;

	const float mass = 0.60f;
	const float density = mass / PhysicsEngine::GetBoxVolume(physx::PxVec3(cueLength, cueHeight, cueThickness));

	physx::PxMaterial* material = physicsEngine_->GetMaterial(0.25f, 0.25f, 0.20f);
	physx::PxVec3 size = physx::PxVec3(cueLength, cueHeight, cueThickness);
	physx::PxVec3 position = physx::PxVec3(-cueLength * 0.5f, 0.0f, 0.0f);
	physx::PxShape* shpae = physicsEngine_->CreateBoxShape(size, material, CustomFilterData::eCUE);

	actor_ = physicsEngine_->AddDynamicActor(shpae, position, physx::PxQuat(physx::PxIdentity), density);
	Park(true);
}

CueState Cue::State() {
	return state_;
}

void Cue::Render() {
	physx::PxRigidActor* actor = actor_;
	Snippets::renderActors(&actor, 1, false, mainBallColor);
}

void Cue::IncreaseAimAngle() {
	aimAngle_ += aimStep;
	WrapAimAngle();
}

void Cue::DecreaseAimAngle() {
	aimAngle_ -= aimStep;
	WrapAimAngle();
}

void Cue::AdjustCuePullback(float delta) {
	cuePullback_ = Clamp(cuePullback_ + delta, cuePullbackMin, cuePullbackMax);
}

void Cue::StartCueStrike(physx::PxRigidActor* ball) {
	if (!CanManipulate()) {
		return;
	}

	if (!ball) {
		return;
	}

	SetPositionAroundBall(ball->getGlobalPose().p, cuePullback_, true);

	const physx::PxVec3 direction = physx::PxVec3(physx::PxCos(aimAngle_), 0.0f, physx::PxSin(aimAngle_));
	actor_->setLinearVelocity(direction * (2.8f + 8.0f * cuePullback_));
	actor_->wakeUp();

	strikeTimer_ = 0.0f;
	state_ = CueState::eSTRIKING;
}

void Cue::Update(float deltaTime, physx::PxRigidActor* ball) {
	if (!actor_) {
		return;
	}

	if (!ball) {
		return;
	}

	if (state_ == CueState::eSTRIKING) {
		strikeTimer_ += deltaTime;

		if (strikeTimer_ >= cueStrikeDuration) {
			state_ = CueState::eHIDDEN;
			Park(true);
		}

		return;
	}

	if (CanManipulate()) {
		state_ = CueState::eAIMING;
		SetPositionAroundBall(ball->getGlobalPose().p, cuePullback_, true);
		return;
	}

	state_ = CueState::eHIDDEN;
	Park(true);
}

bool Cue::CanManipulate() {
	return true;
	//return result_ == GameResult::ePLAYING && GetCueBall() && BallsAreStopped();
}

bool Cue::BallsAreStopped() {
	return true;
	//for (const Ball& ball : balls_) {
	//	if (!ball.actor) {
	//		continue;
	//	}

	//	if (ball.actor->getLinearVelocity().magnitude() > ballStopThreshold) {
	//		return false;
	//	}
	//}
	//return true;
}

void Cue::WrapAimAngle() {
	const float fullTurn = 2.0f * physx::PxPi;

	while (aimAngle_ >= fullTurn) {
		aimAngle_ -= fullTurn;
	}

	while (aimAngle_ < 0.0f) {
		aimAngle_ += fullTurn;
	}
}

void Cue::SetPositionAroundBall(const physx::PxVec3& position, float pullbackOffset, bool teleport) {
	const physx::PxVec3 direction = physx::PxVec3(physx::PxCos(aimAngle_), 0.0f, physx::PxSin(aimAngle_));
	const physx::PxVec3 tipAnchor = position - direction * (ballRadius + cueTipGap + pullbackOffset);
	const physx::PxQuat rotation(-aimAngle_, physx::PxVec3(0.0f, 1.0f, 0.0f));
	SetPosition(physx::PxTransform(tipAnchor, rotation), teleport);
}

void Cue::Park(bool teleport) {
	physx::PxTransform transform = physx::PxTransform(hiddenCuePosition, physx::PxQuat(physx::PxIdentity));
	SetPosition(transform, teleport);
}

void Cue::SetPosition(physx::PxTransform transform, bool teleport)
{
	actor_->setLinearVelocity(physx::PxVec3(0.0f));
	actor_->setAngularVelocity(physx::PxVec3(0.0f));
	actor_->setGlobalPose(transform);

	if (teleport) {
		actor_->putToSleep();
	}
}

float Cue::Clamp(float value, float minimum, float maximum) {
	if (value < minimum) {
		return minimum;
	}

	if (value > maximum) {
		return maximum;
	}

	return value;
}