#include "PxPhysicsAPI.h"

constexpr float simulationStep = 1.0f / 60.0f;

constexpr float tableLength = 2.54f;
constexpr float tableWidth = 1.27f;
constexpr float tableSurfaceThickness = 0.04f;
constexpr float railThickness = 0.08f;
constexpr float railHeight = 0.10f;
constexpr float pitDepth = 0.25f;
constexpr float pitThickness = 0.02f;

constexpr float cornerPocketCut = 0.12f;
constexpr float sidePocketWidth = 0.18f;
constexpr float sidePocketDepth = 0.12f;

constexpr float ballRadius = 0.0285f;
constexpr float ballMass = 0.17f;
constexpr float ballLinearDamping = 0.75f;
constexpr float ballStopThreshold = 0.06f;
constexpr float pocketDestroyY = -0.10f;

constexpr float cueLength = 0.42f;
constexpr float cueHeight = 0.016f;
constexpr float cueThickness = 0.014f;
constexpr float cueTipGap = 0.01f;
constexpr float cuePullbackMin = 0.03f;
constexpr float cuePullbackMax = 0.12f;
constexpr float cuePullbackDefault = 0.06f;
constexpr float cuePullbackStep = 0.01f;
constexpr float cueStrikeDuration = 0.18f;
constexpr float aimStep = physx::PxPi / 90.0f;
constexpr float aimGuideLength = 0.50f;

const physx::PxVec3 tableColor(0.08f, 0.44f, 0.18f);
const physx::PxVec3 railColor(0.42f, 0.23f, 0.10f);
const physx::PxVec3 pocketColor(0.08f, 0.08f, 0.08f);
const physx::PxVec3 objectBallColor(0.86f, 0.58f, 0.12f);
const physx::PxVec3 cueBallColor(0.95f, 0.95f, 0.95f);
const physx::PxVec3 cueColor(0.74f, 0.58f, 0.34f);
const physx::PxVec3 aimColor(0.90f, 0.95f, 1.00f);