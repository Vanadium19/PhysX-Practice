#pragma once

#include <array>
#include <cstddef>

#include "PxPhysicsAPI.h"

struct MaterialSettings {
	float staticFriction;
	float dynamicFriction;
	float restitution;
};

struct ArenaBoxDefinition {
	physx::PxVec3 size;
	physx::PxVec3 position;
	physx::PxQuat rotation;
};

class GameConstants final {
public:
	GameConstants() = delete;

	static float DegreesToRadians(float degrees) {
		return degrees * physx::PxPi / 180.0f;
	}

	struct AppConfig {
		inline static constexpr const char* WindowTitle = "Homework 2 Shooter";
		inline static const physx::PxVec3 CameraPosition = physx::PxVec3(-12.0f, 8.0f, 18.0f);
		inline static const physx::PxVec3 CameraDirection = physx::PxVec3(0.52f, -0.22f, -0.82f);
		inline static constexpr float CameraSpeed = 0.35f;
		inline static constexpr float RenderNearClip = 0.1f;
		inline static constexpr float RenderFarClip = 300.0f;
	};

	struct MaterialConfig {
		inline static constexpr MaterialSettings Floor = { 0.90f, 0.80f, 0.02f };
		inline static constexpr MaterialSettings Obstacle = { 0.65f, 0.55f, 0.08f };
		inline static constexpr MaterialSettings Enemy = { 0.65f, 0.55f, 0.05f };
		inline static constexpr MaterialSettings Grenade = { 0.45f, 0.35f, 0.65f };
	};

	struct EnemyConfig {
		inline static constexpr float Radius = 0.45f;
		inline static constexpr float Height = 1.40f;
		inline static constexpr float Mass = 85.0f;
		inline static constexpr float MaxHealth = 100.0f;
		inline static constexpr float StandingHeight = 1.15f;
		inline static const physx::PxVec3 SpawnPosition = physx::PxVec3(12.0f, StandingHeight, 1.0f);

		inline static constexpr std::size_t RagdollPartCount = 6;
		inline static constexpr float HeadRadius = 0.22f;
		inline static const physx::PxVec3 TorsoSize = physx::PxVec3(0.55f, 0.80f, 0.32f);
		inline static constexpr float ArmRadius = 0.11f;
		inline static constexpr float ArmLength = 0.70f;
		inline static constexpr float LegRadius = 0.14f;
		inline static constexpr float LegLength = 0.90f;

		inline static constexpr float HeadMass = 5.0f;
		inline static constexpr float TorsoMass = 39.0f;
		inline static constexpr float ArmMass = 6.0f;
		inline static constexpr float LegMass = 14.5f;

		inline static const physx::PxVec3 HeadOffset = physx::PxVec3(0.0f, 0.78f, 0.0f);
		inline static const physx::PxVec3 LeftArmOffset = physx::PxVec3(-0.62f, 0.22f, 0.0f);
		inline static const physx::PxVec3 RightArmOffset = physx::PxVec3(0.62f, 0.22f, 0.0f);
		inline static const physx::PxVec3 LeftLegOffset = physx::PxVec3(-0.20f, -0.95f, 0.0f);
		inline static const physx::PxVec3 RightLegOffset = physx::PxVec3(0.20f, -0.95f, 0.0f);

		inline static const physx::PxVec3 NeckAnchorOffset = physx::PxVec3(0.0f, 0.54f, 0.0f);
		inline static const physx::PxVec3 LeftShoulderAnchorOffset = physx::PxVec3(-0.28f, 0.22f, 0.0f);
		inline static const physx::PxVec3 RightShoulderAnchorOffset = physx::PxVec3(0.28f, 0.22f, 0.0f);
		inline static const physx::PxVec3 LeftHipAnchorOffset = physx::PxVec3(-0.20f, -0.46f, 0.0f);
		inline static const physx::PxVec3 RightHipAnchorOffset = physx::PxVec3(0.20f, -0.46f, 0.0f);

		inline static constexpr float LiveLinearDamping = 0.15f;
		inline static constexpr float LiveAngularDamping = 0.2f;
		inline static constexpr float RagdollLinearDamping = 0.08f;
		inline static constexpr float RagdollAngularDamping = 0.18f;
		inline static constexpr float DirectionEpsilon = 1e-4f;

		inline static constexpr physx::PxU32 LiveSolverPositionIterations = 8;
		inline static constexpr physx::PxU32 LiveSolverVelocityIterations = 4;
		inline static constexpr physx::PxU32 RagdollSolverPositionIterations = 12;
		inline static constexpr physx::PxU32 RagdollSolverVelocityIterations = 4;

		inline static constexpr float NeckLowerTwistDegrees = -30.0f;
		inline static constexpr float NeckUpperTwistDegrees = 30.0f;
		inline static constexpr float NeckSwing1Degrees = 25.0f;
		inline static constexpr float NeckSwing2Degrees = 20.0f;

		inline static constexpr float ShoulderLowerTwistDegrees = -35.0f;
		inline static constexpr float ShoulderUpperTwistDegrees = 35.0f;
		inline static constexpr float ShoulderSwing1Degrees = 65.0f;
		inline static constexpr float ShoulderSwing2Degrees = 45.0f;

		inline static constexpr float HipLowerTwistDegrees = -20.0f;
		inline static constexpr float HipUpperTwistDegrees = 20.0f;
		inline static constexpr float HipSwing1Degrees = 40.0f;
		inline static constexpr float HipSwing2Degrees = 25.0f;
	};

	struct GrenadeConfig {
		inline static constexpr float Radius = 0.28f;
		inline static constexpr float Mass = 2.2f;
		inline static constexpr float LinearDamping = 0.10f;
		inline static constexpr float AngularDamping = 0.35f;
	};

	struct ArenaConfig {
		inline static constexpr float CoverPointOffset = 0.80f;
		inline static constexpr int CoverPointCountPerObstacle = 8;
		inline static constexpr int BoundaryObstacleCount = 4;
		inline static constexpr int CoverObstacleCount = 5;
		inline static constexpr float ClampMin = -16.0f;
		inline static constexpr float ClampMax = 16.0f;

		inline static const physx::PxVec3 FloorSize = physx::PxVec3(36.0f, 1.0f, 36.0f);
		inline static const physx::PxVec3 FloorPosition = physx::PxVec3(0.0f, -0.5f, 0.0f);

		inline static const std::array<ArenaBoxDefinition, BoundaryObstacleCount> BoundaryObstacles = {
			ArenaBoxDefinition{ physx::PxVec3(36.0f, 3.0f, 1.0f), physx::PxVec3(0.0f, 1.5f, 18.5f), physx::PxQuat(physx::PxIdentity) },
			ArenaBoxDefinition{ physx::PxVec3(36.0f, 3.0f, 1.0f), physx::PxVec3(0.0f, 1.5f, -18.5f), physx::PxQuat(physx::PxIdentity) },
			ArenaBoxDefinition{ physx::PxVec3(1.0f, 3.0f, 36.0f), physx::PxVec3(18.5f, 1.5f, 0.0f), physx::PxQuat(physx::PxIdentity) },
			ArenaBoxDefinition{ physx::PxVec3(1.0f, 3.0f, 36.0f), physx::PxVec3(-18.5f, 1.5f, 0.0f), physx::PxQuat(physx::PxIdentity) }
		};

		inline static const std::array<ArenaBoxDefinition, CoverObstacleCount> CoverObstacles = {
			ArenaBoxDefinition{ physx::PxVec3(2.5f, 3.0f, 2.5f), physx::PxVec3(-8.0f, 1.5f, -5.0f), physx::PxQuat(physx::PxIdentity) },
			ArenaBoxDefinition{ physx::PxVec3(3.0f, 2.5f, 1.5f), physx::PxVec3(-3.0f, 1.25f, 3.5f), physx::PxQuat(physx::PxIdentity) },
			ArenaBoxDefinition{ physx::PxVec3(1.5f, 4.0f, 4.5f), physx::PxVec3(3.0f, 2.0f, -4.0f), physx::PxQuat(physx::PxIdentity) },
			ArenaBoxDefinition{ physx::PxVec3(4.0f, 3.0f, 1.5f), physx::PxVec3(7.0f, 1.5f, 5.5f), physx::PxQuat(physx::PxIdentity) },
			ArenaBoxDefinition{ physx::PxVec3(1.2f, 4.5f, 3.0f), physx::PxVec3(8.5f, 2.25f, -2.0f), physx::PxQuat(physx::PxIdentity) }
		};
	};

	struct AIConfig {
		inline static constexpr float ViewSphereRadius = 14.0f;
		inline static constexpr float MoveSpeed = 4.5f;
		inline static constexpr float RepathInterval = 0.25f;
		inline static constexpr float ArrivalThreshold = 0.6f;
		inline static constexpr float FleeSampleRadius = 6.0f;
		inline static constexpr int FleeSampleCount = 8;
		inline static constexpr float RaycastEpsilon = 0.05f;
		inline static constexpr float DebugMarkerHalfSize = 0.25f;
		inline static constexpr float VisibilityProbeInset = 0.45f;
		inline static constexpr float CoverPointComparisonEpsilon = 1e-4f;
		inline static constexpr float DirectionEpsilon = 1e-4f;
		inline static constexpr int DebugCircleSegments = 24;
		inline static constexpr int MaxSelfHitPasses = 8;

		inline static const physx::PxVec3 IdleColor = physx::PxVec3(0.25f, 0.95f, 0.35f);
		inline static const physx::PxVec3 SeekingCoverColor = physx::PxVec3(0.25f, 0.60f, 1.0f);
		inline static const physx::PxVec3 FleeingColor = physx::PxVec3(1.0f, 0.60f, 0.15f);
	};

	struct ShooterConfig {
		inline static constexpr float SimulationStep = 1.0f / 60.0f;
		inline static constexpr float BulletRange = 120.0f;
		inline static constexpr float BulletTraceLifetime = 0.35f;
		inline static constexpr float BulletSpreadRadians = 2.5f * physx::PxPi / 180.0f;
		inline static constexpr float BulletImpulse = 180.0f;
		inline static constexpr float BulletDamage = 20.0f;
		inline static constexpr float GrenadeFuseTime = 2.5f;
		inline static constexpr float GrenadeThrowSpeed = 18.0f;
		inline static constexpr float GrenadeUpwardVelocity = 6.0f;
		inline static constexpr float GrenadeSpawnOffset = 1.5f;
		inline static constexpr float GrenadeExplosionRadius = 6.0f;
		inline static constexpr float GrenadeMaxDamage = 100.0f;
		inline static constexpr float GrenadeMaxImpulse = 240.0f;
		inline static constexpr float ExplosionEffectLifetime = 0.45f;
		inline static constexpr float ExplosionBlockEpsilon = 1e-4f;
		inline static constexpr float SpreadDirectionEpsilon = 1e-4f;
	};

	struct RenderConfig {
		inline static constexpr float ImpactMarkerHalfSize = 0.18f;
		inline static constexpr float ExplosionBurstRadiusScale = 0.75f;
		inline static constexpr float ExplosionDiagonalBurstScale = 0.70f;
		inline static constexpr std::size_t EnemyActorReserve = EnemyConfig::RagdollPartCount;
		inline static constexpr std::size_t AIDebugLineReserve = 96;
		inline static constexpr std::size_t HudLineBufferSize = 160;

		inline static const physx::PxVec3 FloorColor = physx::PxVec3(0.25f, 0.25f, 0.28f);
		inline static const physx::PxVec3 ObstacleColor = physx::PxVec3(0.25f, 0.50f, 0.75f);
		inline static const physx::PxVec3 EnemyColor = physx::PxVec3(0.85f, 0.25f, 0.25f);
		inline static const physx::PxVec3 GrenadeColor = physx::PxVec3(0.95f, 0.65f, 0.15f);
		inline static const physx::PxVec3 BulletColor = physx::PxVec3(1.0f, 0.95f, 0.30f);
		inline static const physx::PxVec3 HitBulletColor = physx::PxVec3(1.0f, 0.35f, 0.25f);
		inline static const physx::PxVec3 ExplosionColor = physx::PxVec3(1.0f, 0.55f, 0.15f);
	};
};
