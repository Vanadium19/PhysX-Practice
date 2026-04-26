#include "ShooterGame.h"

#include <cctype>
#include <cmath>
#include <cstdio>
#include <iostream>

#include "Grenade.h"
#include "PhysicsEngine.h"
#include "snippetrender/SnippetCamera.h"
#include "snippetrender/SnippetRender.h"

namespace {
const float kSimulationStep = 1.0f / 60.0f;
const float kBulletRange = 120.0f;
const float kBulletTraceLifetime = 0.35f;
const float kBulletSpreadRadians = 2.5f * physx::PxPi / 180.0f;
const float kBulletImpulse = 180.0f;
const float kBulletDamage = 20.0f;
const float kGrenadeFuseTime = 2.5f;
const float kGrenadeThrowSpeed = 18.0f;
const float kGrenadeUpwardVelocity = 6.0f;
const float kGrenadeExplosionRadius = 6.0f;
const float kGrenadeMaxDamage = 100.0f;
const float kGrenadeMaxImpulse = 240.0f;
const float kExplosionEffectLifetime = 0.45f;
const float kImpactMarkerHalfSize = 0.18f;

const physx::PxVec3 kFloorColor(0.25f, 0.25f, 0.28f);
const physx::PxVec3 kObstacleColor(0.25f, 0.50f, 0.75f);
const physx::PxVec3 kEnemyColor(0.85f, 0.25f, 0.25f);
const physx::PxVec3 kGrenadeColor(0.95f, 0.65f, 0.15f);
const physx::PxVec3 kBulletColor(1.0f, 0.95f, 0.30f);
const physx::PxVec3 kHitBulletColor(1.0f, 0.35f, 0.25f);
const physx::PxVec3 kExplosionColor(1.0f, 0.55f, 0.15f);
}

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
}

void ShooterGame::Shutdown() {
	bulletTraces_.clear();
	explosionEffects_.clear();

	for (std::unique_ptr<Grenade>& grenade : grenades_) {
		grenade->Shutdown();
	}
	grenades_.clear();

	enemy_.Shutdown();

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
	physicsEngine_->Simulate(kSimulationStep);
	UpdateGrenades(kSimulationStep);
	UpdateBulletTraces(kSimulationStep);
	UpdateExplosionEffects(kSimulationStep);

	Snippets::startRender(camera_, 0.1f, 300.0f);

	if (!floorActors_.empty()) {
		Snippets::renderActors(floorActors_.data(), static_cast<physx::PxU32>(floorActors_.size()), false, kFloorColor);
	}

	if (!obstacleActors_.empty()) {
		Snippets::renderActors(obstacleActors_.data(), static_cast<physx::PxU32>(obstacleActors_.size()), false, kObstacleColor);
	}

	std::vector<physx::PxRigidActor*> enemyActors;
	enemyActors.reserve(6);
	enemy_.AppendRenderActors(enemyActors);
	if (!enemyActors.empty()) {
		Snippets::renderActors(enemyActors.data(), static_cast<physx::PxU32>(enemyActors.size()), false, kEnemyColor);
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
			Snippets::renderActors(grenadeActors.data(), static_cast<physx::PxU32>(grenadeActors.size()), false, kGrenadeColor);
		}
	}

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
	physx::PxMaterial* floorMaterial = physicsEngine_->GetMaterial(0.90f, 0.80f, 0.02f);
	physx::PxMaterial* obstacleMaterial = physicsEngine_->GetMaterial(0.65f, 0.55f, 0.08f);

	CreateStaticBox(floorActors_, physx::PxVec3(36.0f, 1.0f, 36.0f), physx::PxVec3(0.0f, -0.5f, 0.0f), floorMaterial);

	CreateStaticBox(obstacleActors_, physx::PxVec3(36.0f, 3.0f, 1.0f), physx::PxVec3(0.0f, 1.5f, 18.5f), obstacleMaterial);
	CreateStaticBox(obstacleActors_, physx::PxVec3(36.0f, 3.0f, 1.0f), physx::PxVec3(0.0f, 1.5f, -18.5f), obstacleMaterial);
	CreateStaticBox(obstacleActors_, physx::PxVec3(1.0f, 3.0f, 36.0f), physx::PxVec3(18.5f, 1.5f, 0.0f), obstacleMaterial);
	CreateStaticBox(obstacleActors_, physx::PxVec3(1.0f, 3.0f, 36.0f), physx::PxVec3(-18.5f, 1.5f, 0.0f), obstacleMaterial);

	CreateStaticBox(obstacleActors_, physx::PxVec3(2.5f, 3.0f, 2.5f), physx::PxVec3(-8.0f, 1.5f, -5.0f), obstacleMaterial);
	CreateStaticBox(obstacleActors_, physx::PxVec3(3.0f, 2.5f, 1.5f), physx::PxVec3(-3.0f, 1.25f, 3.5f), obstacleMaterial);
	CreateStaticBox(obstacleActors_, physx::PxVec3(1.5f, 4.0f, 4.5f), physx::PxVec3(3.0f, 2.0f, -4.0f), obstacleMaterial);
	CreateStaticBox(obstacleActors_, physx::PxVec3(4.0f, 3.0f, 1.5f), physx::PxVec3(7.0f, 1.5f, 5.5f), obstacleMaterial);
	CreateStaticBox(obstacleActors_, physx::PxVec3(1.2f, 4.5f, 3.0f), physx::PxVec3(8.5f, 2.25f, -2.0f), obstacleMaterial);
}

void ShooterGame::CreateEnemy() {
	enemy_.Initialize(physicsEngine_, physx::PxVec3(12.0f, 1.15f, 1.0f));
}

void ShooterGame::CreateStaticBox(
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
	const bool hasHit = physicsEngine_->Raycast(origin, normalizedDirection, kBulletRange, hit);

	physx::PxVec3 impactPoint = origin + normalizedDirection * kBulletRange;
	physx::PxVec3 traceColor = kBulletColor;

	if (hasHit && hit.hasBlock) {
		impactPoint = hit.block.position;

		if (enemy_.OwnsActor(hit.block.actor)) {
			const bool enemyWasAlive = enemy_.IsAlive();
			const float damage = enemy_.ApplyBulletImpact(hit.block.position, normalizedDirection, kBulletImpulse, kBulletDamage);
			traceColor = kHitBulletColor;

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
	const physx::PxVec3 spawnPosition = origin + normalizedDirection * 1.5f;
	const physx::PxVec3 initialVelocity = normalizedDirection * kGrenadeThrowSpeed + physx::PxVec3(0.0f, kGrenadeUpwardVelocity, 0.0f);

	std::unique_ptr<Grenade> grenade = std::make_unique<Grenade>(kGrenadeFuseTime);
	grenade->Initialize(physicsEngine_, spawnPosition, initialVelocity);
	grenades_.push_back(std::move(grenade));

	std::cout << "Grenade thrown.\n";
}

void ShooterGame::ExplodeGrenade(const physx::PxVec3& explosionPosition) {
	AddExplosionEffect(explosionPosition, kGrenadeExplosionRadius);

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
			kGrenadeExplosionRadius,
			kGrenadeMaxDamage,
			kGrenadeMaxImpulse
		);
		return;
	}

	const physx::PxVec3 enemyPosition = enemy_.GetPosition();
	const float distanceToEnemy = (enemyPosition - explosionPosition).magnitude();

	if (distanceToEnemy > kGrenadeExplosionRadius) {
		std::cout << "Enemy is outside the explosion radius.\n";
		return;
	}

	if (IsExplosionBlocked(explosionPosition, enemyPosition)) {
		std::cout << "Obstacle blocked the explosion. Enemy took no damage.\n";
		return;
	}

	const float damage = enemy_.ApplyExplosionDamage(
		explosionPosition,
		kGrenadeExplosionRadius,
		kGrenadeMaxDamage,
		kGrenadeMaxImpulse
	);

	std::cout
		<< "Enemy took " << damage
		<< " explosion damage. Health left: "
		<< enemy_.GetHealth() << ".\n";

	if (!enemy_.IsAlive()) {
		std::cout << "Enemy eliminated by explosion damage. Ragdoll activated.\n";
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

	if (distance < 1e-4f) {
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
	if (right.magnitudeSquared() < 1e-4f) {
		right = forward.cross(physx::PxVec3(1.0f, 0.0f, 0.0f));
	}
	right = right.getNormalized();

	const physx::PxVec3 up = right.cross(forward).getNormalized();

	std::uniform_real_distribution<float> distribution(-kBulletSpreadRadians, kBulletSpreadRadians);
	const float horizontalSpread = distribution(randomEngine_);
	const float verticalSpread = distribution(randomEngine_);

	const physx::PxVec3 spreadDirection =
		forward
		+ right * static_cast<float>(std::tan(horizontalSpread))
		+ up * static_cast<float>(std::tan(verticalSpread));

	return spreadDirection.getNormalized();
}

void ShooterGame::AddBulletTrace(const physx::PxVec3& start, const physx::PxVec3& end, const physx::PxVec3& color, bool hasImpactMarker) {
	bulletTraces_.push_back(BulletTrace{ start, end, color, hasImpactMarker, kBulletTraceLifetime });
}

void ShooterGame::AddExplosionEffect(const physx::PxVec3& position, float maxRadius) {
	explosionEffects_.push_back(ExplosionEffect{ position, maxRadius, kExplosionEffectLifetime, kExplosionEffectLifetime });
}

void ShooterGame::RenderHud() const {
	char line[160];

	Snippets::print("Controls: mouse/WASD - camera, Space - shoot, G - throw grenade, R - reset");
	Snippets::print("Bullets use raycast with a small spread. Grenades explode after a fixed timer.");

	std::snprintf(line, sizeof(line), "Enemy health: %.1f", enemy_.GetHealth());
	Snippets::print(line);

	std::snprintf(line, sizeof(line), "Active grenades: %zu", grenades_.size());
	Snippets::print(line);
}

void ShooterGame::RenderBulletTraces() const {
	for (const BulletTrace& trace : bulletTraces_) {
		Snippets::DrawLine(trace.start, trace.end, trace.color);

		if (trace.hasImpactMarker) {
			Snippets::DrawLine(
				trace.end + physx::PxVec3(-kImpactMarkerHalfSize, 0.0f, 0.0f),
				trace.end + physx::PxVec3(kImpactMarkerHalfSize, 0.0f, 0.0f),
				trace.color
			);
			Snippets::DrawLine(
				trace.end + physx::PxVec3(0.0f, -kImpactMarkerHalfSize, 0.0f),
				trace.end + physx::PxVec3(0.0f, kImpactMarkerHalfSize, 0.0f),
				trace.color
			);
			Snippets::DrawLine(
				trace.end + physx::PxVec3(0.0f, 0.0f, -kImpactMarkerHalfSize),
				trace.end + physx::PxVec3(0.0f, 0.0f, kImpactMarkerHalfSize),
				trace.color
			);
		}
	}
}

void ShooterGame::RenderExplosionEffects() const {
	for (const ExplosionEffect& effect : explosionEffects_) {
		const float lifeProgress = 1.0f - effect.remainingLifetime / effect.totalLifetime;
		const float currentRadius = effect.maxRadius * lifeProgress;
		const float burstRadius = currentRadius * 0.75f;

		physx::PxSphereGeometry sphere(currentRadius);
		physx::PxGeometryHolder geometry(sphere);
		physx::PxTransform pose(effect.position);

		Snippets::renderGeoms(1, &geometry, &pose, false, kExplosionColor);

		Snippets::DrawLine(effect.position + physx::PxVec3(-burstRadius, 0.0f, 0.0f), effect.position + physx::PxVec3(burstRadius, 0.0f, 0.0f), kExplosionColor);
		Snippets::DrawLine(effect.position + physx::PxVec3(0.0f, -burstRadius, 0.0f), effect.position + physx::PxVec3(0.0f, burstRadius, 0.0f), kExplosionColor);
		Snippets::DrawLine(effect.position + physx::PxVec3(0.0f, 0.0f, -burstRadius), effect.position + physx::PxVec3(0.0f, 0.0f, burstRadius), kExplosionColor);
		Snippets::DrawLine(
			effect.position + physx::PxVec3(-burstRadius * 0.7f, 0.0f, -burstRadius * 0.7f),
			effect.position + physx::PxVec3(burstRadius * 0.7f, 0.0f, burstRadius * 0.7f),
			kExplosionColor
		);
		Snippets::DrawLine(
			effect.position + physx::PxVec3(-burstRadius * 0.7f, 0.0f, burstRadius * 0.7f),
			effect.position + physx::PxVec3(burstRadius * 0.7f, 0.0f, -burstRadius * 0.7f),
			kExplosionColor
		);
	}
}
