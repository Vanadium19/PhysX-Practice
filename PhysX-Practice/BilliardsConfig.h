#pragma once

#include "PxPhysicsAPI.h"

constexpr float simulationStep = 1.0f / 60.0f;

constexpr float tableLength = 2.54f;
constexpr float tableWidth = 1.27f;
constexpr float tableSurfaceThickness = 0.04f;
constexpr float railThickness = 0.08f;
constexpr float railHeight = 0.10f;

constexpr float cornerPocketCut = 0.12f;
constexpr float sidePocketWidth = 0.18f;
constexpr float sidePocketDepth = 0.12f;

constexpr float ballRadius = 0.0285f;
constexpr float ballMass = 0.17f;
constexpr float ballLinearDamping = 0.75f;
constexpr float ballStopThreshold = 0.06f;

constexpr float cueStrikeDuration = 0.18f;
constexpr float aimStep = physx::PxPi / 90.0f;
constexpr float aimGuideLength = 0.50f;

constexpr float shotSpeed = 3.0f;

const physx::PxVec3 tableColor(0.08f, 0.44f, 0.18f);
const physx::PxVec3 railColor(0.42f, 0.23f, 0.10f);
const physx::PxVec3 gaimBallColor(0.86f, 0.58f, 0.12f);
const physx::PxVec3 mainBallColor(1.0f, 1.0f, 1.0f);
const physx::PxVec3 aimColor(0.90f, 0.95f, 1.00f);
