#include "ShooterGame.h"

#include <cctype>
#include <cmath>
#include <cstdio>
#include <iostream>

#include "GameConstants.h"
#include "Grenade.h"
#include "PhysicsEngine.h"
#include "snippetrender/SnippetCamera.h"
#include "snippetrender/SnippetRender.h"

ShooterGame::ShooterGame(PhysicsEngine* physicsEngine, Snippets::Camera* camera) :
	physicsEngine_(physicsEngine),
	camera_(camera),
	randomEngine_(std::random_device{}()) {
}

ShooterGame::~ShooterGame() {
	Shutdown();
}

void ShooterGame::Initialize() {
	CreateArena();
	CreateEnemy();
	if (GameConstants::AIConfig::Enabled) {
		enemyAI_.Initialize(physicsEngine_, &enemy_, &coverPoints_);
	}
}

void ShooterGame::Shutdown() {
	bulletTraces_.clear();
	explosionEffects_.clear();

	for (std::unique_ptr<Grenade>& grenade : grenades_) {
		grenade->Shutdown();
	}
	grenades_.clear();

	enemyAI_.Shutdown();
	enemy_.Shutdown();
	coverPoints_.clear();

	ReleaseActors(obstacleActors_);
	ReleaseActors(floorActors_);
}

void ShooterGame::HandleKey(unsigned char key, const physx::PxTransform& cameraTransform) {
	switch (std::toupper(static_cast<unsigned char>(key))) {
	case ' ':
		Shoot(cameraTransform.p, camera_->getDir());
		break;

	case 'G':
		ThrowGrenade(cameraTransform.p, camera_->getDir());
		break;

	case 'R':
		Reset();
		break;

	default:
		break;
	}
}

void ShooterGame::RenderFrame() {
	if (GameConstants::AIConfig::Enabled) {
		enemyAI_.Update(GameConstants::ShooterConfig::SimulationStep, camera_->getEye());
	}
	physicsEngine_->Simulate(GameConstants::ShooterConfig::SimulationStep);
	UpdateGrenades(GameConstants::ShooterConfig::SimulationStep);
	UpdateBulletTraces(GameConstants::ShooterConfig::SimulationStep);
	UpdateExplosionEffects(GameConstants::ShooterConfig::SimulationStep);

	Snippets::startRender(
		camera_,
		GameConstants::AppConfig::RenderNearClip,
		GameConstants::AppConfig::RenderFarClip
	);

	if (!floorActors_.empty()) {
		Snippets::renderActors(
			floorActors_.data(),
			static_cast<physx::PxU32>(floorActors_.size()),
			false,
			GameConstants::RenderConfig::FloorColor
		);
	}

	if (!obstacleActors_.empty()) {
		Snippets::renderActors(
			obstacleActors_.data(),
			static_cast<physx::PxU32>(obstacleActors_.size()),
			false,
			GameConstants::RenderConfig::ObstacleColor
		);
	}

	std::vector<physx::PxRigidActor*> enemyActors;
	enemyActors.reserve(GameConstants::RenderConfig::EnemyActorReserve);
	enemy_.AppendRenderActors(enemyActors);
	if (!enemyActors.empty()) {
		Snippets::renderActors(
			enemyActors.data(),
			static_cast<physx::PxU32>(enemyActors.size()),
			false,
			GameConstants::RenderConfig::EnemyColor
		);
	}

	if (!grenades_.empty()) {
		std::vector<physx::PxRigidActor*> grenadeActors;
		grenadeActors.reserve(grenades_.size());
		for (const std::unique_ptr<Grenade>& grenade : grenades_) {
			if (grenade->GetActor()) {
				grenadeActors.push_back(grenade->GetActor());
			}
		}

		if (!grenadeActors.empty()) {
			Snippets::renderActors(
				grenadeActors.data(),
				static_cast<physx::PxU32>(grenadeActors.size()),
				false,
				GameConstants::RenderConfig::GrenadeColor
			);
		}
	}

	RenderEnemyAIDebug();
	RenderBulletTraces();
	RenderExplosionEffects();
	RenderHud();

	Snippets::finishRender();
}

void ShooterGame::Reset() {
	Shutdown();
	Initialize();
	std::cout << "Scene reset.\n";
}

void ShooterGame::CreateArena() {
	coverPoints_.clear();

	const MaterialSettings floorSettings = GameConstants::MaterialConfig::Floor;
	const MaterialSettings obstacleSettings = GameConstants::MaterialConfig::Obstacle;
	physx::PxMaterial* floorMaterial = physicsEngine_->GetMaterial(
		floorSettings.staticFriction,
		floorSettings.dynamicFriction,
		floorSettings.restitution
	);
	physx::PxMaterial* obstacleMaterial = physicsEngine_->GetMaterial(
		obstacleSettings.staticFriction,
		obstacleSettings.dynamicFriction,
		obstacleSettings.restitution
	);

	CreateStaticBox(
		floorActors_,
		GameConstants::ArenaConfig::FloorSize,
		GameConstants::ArenaConfig::FloorPosition,
		floorMaterial
	);

	for (const ArenaBoxDefinition& obstacle : GameConstants::ArenaConfig::BoundaryObstacles) {
		CreateStaticBox(obstacleActors_, obstacle.size, obstacle.position, obstacleMaterial, obstacle.rotation);
	}

	for (const ArenaBoxDefinition& obstacle : GameConstants::ArenaConfig::CoverObstacles) {
		physx::PxRigidStatic* coverObstacle =
			CreateStaticBox(obstacleActors_, obstacle.size, obstacle.position, obstacleMaterial, obstacle.rotation);
		AddCoverPointsForBox(obstacle.size, obstacle.position, obstacle.rotation, coverObstacle);
	}
}

void ShooterGame::CreateEnemy() {
	enemy_.Initialize(physicsEngine_, GameConstants::EnemyConfig::SpawnPosition);
}

physx::PxRigidStatic* ShooterGame::CreateStaticBox(
	std::vector<physx::PxRigidActor*>& actors,
	physx::PxVec3 size,
	physx::PxVec3 position,
	physx::PxMaterial* material,
	physx::PxQuat rotation
) {
	physx::PxShape* shape = physicsEngine_->CreateBoxShape(size, material, CustomFilterData::eOBSTACLE, true);
	physx::PxRigidStatic* actor = physicsEngine_->AddStaticActor(shape, position, rotation);
	shape->release();
	actors.push_back(actor);
	return actor;
}

void ShooterGame::AddCoverPointsForBox(
	physx::PxVec3 size,
	physx::PxVec3 position,
	physx::PxQuat rotation,
	physx::PxRigidActor* obstacle
) {
	const physx::PxVec3 axisX = rotation.rotate(physx::PxVec3(1.0f, 0.0f, 0.0f));
	const physx::PxVec3 axisZ = rotation.rotate(physx::PxVec3(0.0f, 0.0f, 1.0f));
	const float halfX = size.x * 0.5f;
	const float halfZ = size.z * 0.5f;

	const auto addPoint = [&](const physx::PxVec3& pointOffset) {
		physx::PxVec3 point = position + pointOffset;
		point.y = GameConstants::EnemyConfig::StandingHeight;
		coverPoints_.push_back(CoverPoint{ point, obstacle });
	};

	if (size.x >= size.z) {
		addPoint(axisZ * (halfZ + GameConstants::ArenaConfig::CoverPointOffset));
		addPoint(-axisZ * (halfZ + GameConstants::ArenaConfig::CoverPointOffset));
		return;
	}

	addPoint(axisX * (halfX + GameConstants::ArenaConfig::CoverPointOffset));
	addPoint(-axisX * (halfX + GameConstants::ArenaConfig::CoverPointOffset));
}

void ShooterGame::ReleaseActors(std::vector<physx::PxRigidActor*>& actors) {
	for (physx::PxRigidActor* actor : actors) {
		if (actor) {
			actor->release();
		}
	}

	actors.clear();
}

void ShooterGame::Shoot(const physx::PxVec3& origin, const physx::PxVec3& direction) {
	const physx::PxVec3 normalizedDirection = ApplySpread(direction);
	physx::PxRaycastBuffer hit;
	const bool hasHit = physicsEngine_->Raycast(origin, normalizedDirection, GameConstants::ShooterConfig::BulletRange, hit);

	physx::PxVec3 impactPoint = origin + normalizedDirection * GameConstants::ShooterConfig::BulletRange;
	physx::PxVec3 traceColor = GameConstants::RenderConfig::BulletColor;

	if (hasHit && hit.hasBlock) {
		impactPoint = hit.block.position;

		if (enemy_.OwnsActor(hit.block.actor)) {
			const bool enemyWasAlive = enemy_.IsAlive();
			const float damage = enemy_.ApplyBulletImpact(
				hit.block.position,
				normalizedDirection,
				GameConstants::ShooterConfig::BulletImpulse,
				GameConstants::ShooterConfig::BulletDamage
			);
			traceColor = GameConstants::RenderConfig::HitBulletColor;

			if (enemyWasAlive) {
				std::cout
					<< "Bullet hit the enemy at ("
					<< hit.block.position.x << ", "
					<< hit.block.position.y << ", "
					<< hit.block.position.z << "). Damage: "
					<< damage
					<< ". Health left: "
					<< enemy_.GetHealth()
					<< ".\n";

				if (!enemy_.IsAlive()) {
					std::cout << "Enemy eliminated by bullet damage. Ragdoll activated.\n";
				}

				if (GameConstants::AIConfig::Enabled) {
					enemyAI_.Update(0.0f, camera_->getEye());
				}
			}
			else {
				std::cout
					<< "Bullet hit the ragdoll at ("
					<< hit.block.position.x << ", "
					<< hit.block.position.y << ", "
					<< hit.block.position.z << ").\n";
			}
		}
		else {
			std::cout
				<< "Bullet hit an obstacle at ("
				<< hit.block.position.x << ", "
				<< hit.block.position.y << ", "
				<< hit.block.position.z << ").\n";
		}
	}
	else {
		std::cout << "Shot missed.\n";
	}

	AddBulletTrace(origin, impactPoint, traceColor, hasHit && hit.hasBlock);
}

void ShooterGame::ThrowGrenade(const physx::PxVec3& origin, const physx::PxVec3& direction) {
	const physx::PxVec3 normalizedDirection = direction.getNormalized();
	const physx::PxVec3 spawnPosition =
		origin + normalizedDirection * GameConstants::ShooterConfig::GrenadeSpawnOffset;
	const physx::PxVec3 initialVelocity =
		normalizedDirection * GameConstants::ShooterConfig::GrenadeThrowSpeed
		+ physx::PxVec3(0.0f, GameConstants::ShooterConfig::GrenadeUpwardVelocity, 0.0f);

	std::unique_ptr<Grenade> grenade =
		std::make_unique<Grenade>(GameConstants::ShooterConfig::GrenadeFuseTime);
	grenade->Initialize(physicsEngine_, spawnPosition, initialVelocity);
	grenades_.push_back(std::move(grenade));

	std::cout << "Grenade thrown.\n";
}

void ShooterGame::ExplodeGrenade(const physx::PxVec3& explosionPosition) {
	AddExplosionEffect(explosionPosition, GameConstants::ShooterConfig::GrenadeExplosionRadius);

	std::cout
		<< "Grenade exploded at ("
		<< explosionPosition.x << ", "
		<< explosionPosition.y << ", "
		<< explosionPosition.z << ").\n";

	if (!enemy_.GetActor()) {
		return;
	}

	if (!enemy_.IsAlive()) {
		enemy_.ApplyExplosionDamage(
			explosionPosition,
			GameConstants::ShooterConfig::GrenadeExplosionRadius,
			GameConstants::ShooterConfig::GrenadeMaxDamage,
			GameConstants::ShooterConfig::GrenadeMaxImpulse
		);
		return;
	}

	const physx::PxVec3 enemyPosition = enemy_.GetPosition();
	const float distanceToEnemy = (enemyPosition - explosionPosition).magnitude();

	if (distanceToEnemy > GameConstants::ShooterConfig::GrenadeExplosionRadius) {
		std::cout << "Enemy is outside the explosion radius.\n";
		return;
	}

	if (IsExplosionBlocked(explosionPosition, enemyPosition)) {
		std::cout << "Obstacle blocked the explosion. Enemy took no damage.\n";
		return;
	}

	const float damage = enemy_.ApplyExplosionDamage(
		explosionPosition,
		GameConstants::ShooterConfig::GrenadeExplosionRadius,
		GameConstants::ShooterConfig::GrenadeMaxDamage,
		GameConstants::ShooterConfig::GrenadeMaxImpulse
	);

	std::cout
		<< "Enemy took " << damage
		<< " explosion damage. Health left: "
		<< enemy_.GetHealth() << ".\n";

	if (!enemy_.IsAlive()) {
		std::cout << "Enemy eliminated by explosion damage. Ragdoll activated.\n";
	}

	if (GameConstants::AIConfig::Enabled) {
		enemyAI_.Update(0.0f, camera_->getEye());
	}
}

void ShooterGame::UpdateGrenades(float elapsedTime) {
	for (std::size_t index = 0; index < grenades_.size();) {
		std::unique_ptr<Grenade>& grenade = grenades_[index];
		grenade->Update(elapsedTime);

		if (grenade->ShouldExplode()) {
			const physx::PxVec3 explosionPosition = grenade->GetPosition();
			grenade->Shutdown();
			grenades_.erase(grenades_.begin() + index);
			ExplodeGrenade(explosionPosition);
			continue;
		}

		++index;
	}
}

void ShooterGame::UpdateBulletTraces(float elapsedTime) {
	for (std::size_t index = 0; index < bulletTraces_.size();) {
		BulletTrace& trace = bulletTraces_[index];
		trace.remainingLifetime -= elapsedTime;

		if (trace.remainingLifetime <= 0.0f) {
			bulletTraces_.erase(bulletTraces_.begin() + index);
			continue;
		}

		++index;
	}
}

void ShooterGame::UpdateExplosionEffects(float elapsedTime) {
	for (std::size_t index = 0; index < explosionEffects_.size();) {
		ExplosionEffect& effect = explosionEffects_[index];
		effect.remainingLifetime -= elapsedTime;

		if (effect.remainingLifetime <= 0.0f) {
			explosionEffects_.erase(explosionEffects_.begin() + index);
			continue;
		}

		++index;
	}
}

bool ShooterGame::IsExplosionBlocked(const physx::PxVec3& explosionPosition, const physx::PxVec3& targetPosition) const {
	const physx::PxVec3 ray = targetPosition - explosionPosition;
	const float distance = ray.magnitude();

	if (distance < GameConstants::ShooterConfig::ExplosionBlockEpsilon) {
		return false;
	}

	physx::PxRaycastBuffer hit;
	const bool hasHit = physicsEngine_->Raycast(explosionPosition, ray.getNormalized(), distance, hit);
	if (!hasHit || !hit.hasBlock) {
		return false;
	}

	return !enemy_.OwnsActor(hit.block.actor);
}

physx::PxVec3 ShooterGame::ApplySpread(const physx::PxVec3& direction) {
	const physx::PxVec3 forward = direction.getNormalized();
	const physx::PxVec3 worldUp(0.0f, 1.0f, 0.0f);

	physx::PxVec3 right = forward.cross(worldUp);
	if (right.magnitudeSquared() < GameConstants::ShooterConfig::SpreadDirectionEpsilon) {
		right = forward.cross(physx::PxVec3(1.0f, 0.0f, 0.0f));
	}
	right = right.getNormalized();

	const physx::PxVec3 up = right.cross(forward).getNormalized();

	std::uniform_real_distribution<float> distribution(
		-GameConstants::ShooterConfig::BulletSpreadRadians,
		GameConstants::ShooterConfig::BulletSpreadRadians
	);
	const float horizontalSpread = distribution(randomEngine_);
	const float verticalSpread = distribution(randomEngine_);

	const physx::PxVec3 spreadDirection =
		forward
		+ right * static_cast<float>(std::tan(horizontalSpread))
		+ up * static_cast<float>(std::tan(verticalSpread));

	return spreadDirection.getNormalized();
}

void ShooterGame::AddBulletTrace(const physx::PxVec3& start, const physx::PxVec3& end, const physx::PxVec3& color, bool hasImpactMarker) {
	bulletTraces_.push_back(BulletTrace{
		start,
		end,
		color,
		hasImpactMarker,
		GameConstants::ShooterConfig::BulletTraceLifetime
	});
}

void ShooterGame::AddExplosionEffect(const physx::PxVec3& position, float maxRadius) {
	explosionEffects_.push_back(ExplosionEffect{
		position,
		maxRadius,
		GameConstants::ShooterConfig::ExplosionEffectLifetime,
		GameConstants::ShooterConfig::ExplosionEffectLifetime
	});
}

void ShooterGame::RenderHud() const {
	char line[GameConstants::RenderConfig::HudLineBufferSize];

	Snippets::print("Controls: mouse/WASD - camera, Space - shoot, G - throw grenade, R - reset");
	Snippets::print("Bullets use raycast with a small spread. Grenades explode after a fixed timer.");

	std::snprintf(line, sizeof(line), "Enemy health: %.1f", enemy_.GetHealth());
	Snippets::print(line);

	std::snprintf(
		line,
		sizeof(line),
		"Enemy AI: %s",
		GameConstants::AIConfig::Enabled ? enemyAI_.GetStateName() : "Off"
	);
	Snippets::print(line);

	std::snprintf(line, sizeof(line), "Active grenades: %zu", grenades_.size());
	Snippets::print(line);
}

void ShooterGame::RenderEnemyAIDebug() const {
	if (!GameConstants::AIConfig::Enabled) {
		return;
	}

	std::vector<EnemyAIController::DebugLine> debugLines;
	debugLines.reserve(GameConstants::RenderConfig::AIDebugLineReserve);
	enemyAI_.AppendDebugLines(debugLines);

	for (const EnemyAIController::DebugLine& line : debugLines) {
		Snippets::DrawLine(line.start, line.end, line.color);
	}
}

void ShooterGame::RenderBulletTraces() const {
	for (const BulletTrace& trace : bulletTraces_) {
		Snippets::DrawLine(trace.start, trace.end, trace.color);

		if (trace.hasImpactMarker) {
			Snippets::DrawLine(
				trace.end + physx::PxVec3(-GameConstants::RenderConfig::ImpactMarkerHalfSize, 0.0f, 0.0f),
				trace.end + physx::PxVec3(GameConstants::RenderConfig::ImpactMarkerHalfSize, 0.0f, 0.0f),
				trace.color
			);
			Snippets::DrawLine(
				trace.end + physx::PxVec3(0.0f, -GameConstants::RenderConfig::ImpactMarkerHalfSize, 0.0f),
				trace.end + physx::PxVec3(0.0f, GameConstants::RenderConfig::ImpactMarkerHalfSize, 0.0f),
				trace.color
			);
			Snippets::DrawLine(
				trace.end + physx::PxVec3(0.0f, 0.0f, -GameConstants::RenderConfig::ImpactMarkerHalfSize),
				trace.end + physx::PxVec3(0.0f, 0.0f, GameConstants::RenderConfig::ImpactMarkerHalfSize),
				trace.color
			);
		}
	}
}

void ShooterGame::RenderExplosionEffects() const {
	for (const ExplosionEffect& effect : explosionEffects_) {
		const float lifeProgress = 1.0f - effect.remainingLifetime / effect.totalLifetime;
		const float currentRadius = effect.maxRadius * lifeProgress;
		const float burstRadius = currentRadius * GameConstants::RenderConfig::ExplosionBurstRadiusScale;

		physx::PxSphereGeometry sphere(currentRadius);
		physx::PxGeometryHolder geometry(sphere);
		physx::PxTransform pose(effect.position);

		Snippets::renderGeoms(1, &geometry, &pose, false, GameConstants::RenderConfig::ExplosionColor);

		Snippets::DrawLine(
			effect.position + physx::PxVec3(-burstRadius, 0.0f, 0.0f),
			effect.position + physx::PxVec3(burstRadius, 0.0f, 0.0f),
			GameConstants::RenderConfig::ExplosionColor
		);
		Snippets::DrawLine(
			effect.position + physx::PxVec3(0.0f, -burstRadius, 0.0f),
			effect.position + physx::PxVec3(0.0f, burstRadius, 0.0f),
			GameConstants::RenderConfig::ExplosionColor
		);
		Snippets::DrawLine(
			effect.position + physx::PxVec3(0.0f, 0.0f, -burstRadius),
			effect.position + physx::PxVec3(0.0f, 0.0f, burstRadius),
			GameConstants::RenderConfig::ExplosionColor
		);
		Snippets::DrawLine(
			effect.position + physx::PxVec3(
				-burstRadius * GameConstants::RenderConfig::ExplosionDiagonalBurstScale,
				0.0f,
				-burstRadius * GameConstants::RenderConfig::ExplosionDiagonalBurstScale
			),
			effect.position + physx::PxVec3(
				burstRadius * GameConstants::RenderConfig::ExplosionDiagonalBurstScale,
				0.0f,
				burstRadius * GameConstants::RenderConfig::ExplosionDiagonalBurstScale
			),
			GameConstants::RenderConfig::ExplosionColor
		);
		Snippets::DrawLine(
			effect.position + physx::PxVec3(
				-burstRadius * GameConstants::RenderConfig::ExplosionDiagonalBurstScale,
				0.0f,
				burstRadius * GameConstants::RenderConfig::ExplosionDiagonalBurstScale
			),
			effect.position + physx::PxVec3(
				burstRadius * GameConstants::RenderConfig::ExplosionDiagonalBurstScale,
				0.0f,
				-burstRadius * GameConstants::RenderConfig::ExplosionDiagonalBurstScale
			),
			GameConstants::RenderConfig::ExplosionColor
		);
	}
}
