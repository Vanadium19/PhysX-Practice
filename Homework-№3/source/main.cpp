#include <cmath>
#include <cctype>
#include <cstdint>
#include <iostream>
#include <vector>
#include "PhysicsEngine.h"
#include "snippetrender/SnippetRender.h"
#include "snippetrender/SnippetCamera.h"

PhysicsEngine *physicsEngine;
Snippets::Camera *camera;
uint32_t frameCounter;

namespace {
constexpr float kFrameDt = 1.0f / 60.0f;

constexpr uint32_t kFlagColumns = 12;
constexpr uint32_t kFlagRows = 8;
constexpr float kFlagWidth = 8.0f;
constexpr float kFlagHeight = 4.5f;
constexpr float kFlagTopY = 6.0f;
constexpr float kFlagZ = 0.0f;

constexpr float kClothDragCoefficient = 0.08f;
constexpr float kClothLiftCoefficient = 0.04f;

uint32_t GetFlagIndex(uint32_t row, uint32_t column) {
	return row * kFlagColumns + column;
}

void CreateFlagMesh(std::vector<physx::PxVec3> &points, std::vector<uint32_t> &triangles, std::vector<float> &invMasses) {
	points.clear();
	triangles.clear();
	invMasses.clear();

	points.reserve(kFlagColumns * kFlagRows);
	triangles.reserve((kFlagColumns - 1) * (kFlagRows - 1) * 6);
	invMasses.reserve(kFlagColumns * kFlagRows);

	for (uint32_t row = 0; row < kFlagRows; row++) {
		for (uint32_t column = 0; column < kFlagColumns; column++) {
			const float x = -kFlagWidth * 0.5f + kFlagWidth * static_cast<float>(column) / static_cast<float>(kFlagColumns - 1);
			const float y = kFlagTopY - kFlagHeight * static_cast<float>(row) / static_cast<float>(kFlagRows - 1);
			const bool isPinnedCorner = row == 0 && (column == 0 || column == kFlagColumns - 1);

			points.push_back(physx::PxVec3(x, y, kFlagZ));
			invMasses.push_back(isPinnedCorner ? 0.0f : 1.0f);
		}
	}

	for (uint32_t row = 0; row < kFlagRows - 1; row++) {
		for (uint32_t column = 0; column < kFlagColumns - 1; column++) {
			const uint32_t topLeft = GetFlagIndex(row, column);
			const uint32_t topRight = GetFlagIndex(row, column + 1);
			const uint32_t bottomLeft = GetFlagIndex(row + 1, column);
			const uint32_t bottomRight = GetFlagIndex(row + 1, column + 1);

			triangles.push_back(topLeft);
			triangles.push_back(topRight);
			triangles.push_back(bottomRight);

			triangles.push_back(topLeft);
			triangles.push_back(bottomRight);
			triangles.push_back(bottomLeft);
		}
	}
}

physx::PxVec3 CalculateWindVelocity() {
	const float time = static_cast<float>(frameCounter) * kFrameDt;
	const float strength = 2.0f + 3.0f * (0.5f + 0.5f * sinf(time * 0.7f));
	const float angle = time * 0.9f;

	return physx::PxVec3(cosf(angle), 0.15f * sinf(time), sinf(angle)) * strength;
}
}

void keyPressedCallback(unsigned char key, const physx::PxTransform &cameraTransform) {
	std::cout << "\"" << key << "\", " << toupper(key) << '\n';
	static float coefficient = kClothDragCoefficient;
	switch (toupper(key)) {
	case ' ':
	{
		static float projectileVelocity = 100.0f;
		static float projectileRadius = 0.25f;
		static physx::PxMaterial *projectileMaterial = physicsEngine->CreateMaterial(0.1f, 0.1f, 0.7f);
		static physx::PxShape *projectileShape = physicsEngine->CreateSphereShape(projectileRadius, projectileMaterial, CustomFilterData::eDYNAMIC);
		physx::PxRigidDynamic *projectileActor = physicsEngine->AddDynamicActor(projectileShape, cameraTransform.p, physx::PxQuat(1.0f), 300.0f);
		projectileActor->setLinearVelocity(camera->getDir() * projectileVelocity);
	}
	break;
	case '+':
	{
		coefficient += 0.0001f;
		for (Cloth *cloth : physicsEngine->GetCloths()) {
			cloth->SetDragCoefficient(coefficient);
			cloth->SetLiftCoefficient(coefficient);
		}
	}
	break;
	case '-':
	{
		coefficient -= 0.0001f;
		for (Cloth *cloth : physicsEngine->GetCloths()) {
			cloth->SetDragCoefficient(coefficient);
			cloth->SetLiftCoefficient(coefficient);
		}
	}
	break;
	default:
		break;
	}
}

void renderCallback() {
	const physx::PxVec3 windVelocity = CalculateWindVelocity();
	for (Cloth *cloth : physicsEngine->GetCloths()) {
		cloth->SetWindVelocity(windVelocity);
	}

	physicsEngine->Simulate(kFrameDt);

	float nearClip = 0.1f;
	float farClip = 10000.0f;
	Snippets::startRender(camera, nearClip, farClip);

	std::vector<physx::PxRigidActor *> actors = physicsEngine->GetActors();
	if (actors.size() > 0) {
		Snippets::renderActors(actors.data(), actors.size());
	}

	const physx::PxVec3 color(0.0f, 0.0f, 1.0f);
	for (Cloth *cloth : physicsEngine->GetCloths()) {
		uint32_t particleNum = cloth->GetNumParticles();
		physx::PxVec4 *particles = cloth->GetCurrentParticles();
		std::vector<uint32_t> indices = cloth->GetMeshIndices();

		glDisable(GL_CULL_FACE);
		Snippets::renderMesh(particleNum, particles, indices.size() / 3, indices.data(), color);
		glEnable(GL_CULL_FACE);
	}

	Snippets::finishRender();

	frameCounter++;
}

void exitCallback() {
	delete camera;
	delete physicsEngine;
}

int main() {
	camera = new Snippets::Camera(physx::PxVec3(0.0f, 10.0f, 30.0f), physx::PxVec3(0.0f, -0.1f, -0.3f));
	Snippets::setupDefault("PhysX Example", camera, keyPressedCallback, renderCallback, exitCallback);

	frameCounter = 0;

	physicsEngine = new PhysicsEngine();

	physx::PxMaterial *defaultMaterial = physicsEngine->CreateMaterial(0.5f, 0.5f, 0.1f);

	const physx::PxVec3 groundNormal = physx::PxVec3(0.0f, 1.0f, 0.0f);
	const float groundDistance = 0.5f;
	physicsEngine->AddGround(groundNormal, groundDistance, defaultMaterial);

	std::vector<physx::PxVec3> points;
	std::vector<uint32_t> triangles;
	std::vector<float> invMasses;
	CreateFlagMesh(points, triangles, invMasses);

	const physx::PxVec3 anchorMarkerSize(0.18f, 0.18f, 0.18f);
	physx::PxShape *leftAnchorMarker = physicsEngine->CreateBoxShape(anchorMarkerSize, defaultMaterial, CustomFilterData::eOBSTACLE);
	physx::PxShape *rightAnchorMarker = physicsEngine->CreateBoxShape(anchorMarkerSize, defaultMaterial, CustomFilterData::eOBSTACLE);
	physicsEngine->AddStaticActor(leftAnchorMarker, points[GetFlagIndex(0, 0)], physx::PxQuat(1.0f));
	physicsEngine->AddStaticActor(rightAnchorMarker, points[GetFlagIndex(0, kFlagColumns - 1)], physx::PxQuat(1.0f));

	Cloth *cloth = new Cloth(points, triangles, invMasses);

	cloth->SetDamping(physx::PxVec3(0.08f, 0.08f, 0.08f));
	cloth->SetDragCoefficient(kClothDragCoefficient);
	cloth->SetLiftCoefficient(kClothLiftCoefficient);

	std::vector<physx::PxVec4> planes = { physx::PxVec4(groundNormal, groundDistance) };
	std::vector<uint32_t> planesIndices = { 1 };
	cloth->SetPlaneCollisions(planes, planesIndices);

	physicsEngine->AddCloth(cloth);

	glutMainLoop();

	return 0;
}
