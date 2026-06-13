#include <iostream>
#include "PhysicsEngine.h"
#include "snippetrender/SnippetRender.h"
#include "snippetrender/SnippetCamera.h"

PhysicsEngine *physicsEngine;
Snippets::Camera *camera;
uint32_t frameCounter;

void keyPressedCallback(unsigned char key, const physx::PxTransform &cameraTransform) {
	std::cout << "\"" << key << "\", " << toupper(key) << '\n';
	static float coefficient = 0.0f;
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
	physicsEngine->Simulate(1.0f / 60.0f);

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
		
		for (uint32_t i = 0; i < particleNum; i++) {
			physx::PxVec4 particle = particles[i];
			std::cout << "(" << particle.x << ", " << particle.y << ", " << particle.z << ") ";
		}
		std::cout << "\n";

		glDisable(GL_CULL_FACE);
		Snippets::renderMesh(particleNum, particles, indices.size() / 3, indices.data(), color);
		glEnable(GL_CULL_FACE);

		cloth->SetWindVelocity(physx::PxVec3(0.1f, 0.0f, 0.1f) * frameCounter);
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

	std::vector<physx::PxVec3> points = {
		physx::PxVec3(-1.0f, 4.0f, 0.0f),
		physx::PxVec3( 0.0f, 4.0f, 0.0f),
		physx::PxVec3( 1.0f, 4.0f, 0.0f),
		physx::PxVec3(-1.0f, 3.0f, 0.0f),
		physx::PxVec3( 0.0f, 3.0f, 0.0f),
		physx::PxVec3( 1.0f, 3.0f, 0.0f),
		physx::PxVec3(-1.0f, 2.0f, 0.0f),
		physx::PxVec3( 0.0f, 2.0f, 0.0f),
		physx::PxVec3( 1.0f, 2.0f, 0.0f)
	};
	std::vector<uint32_t> triangles = {
		0, 1, 4, 0, 4, 3,
		1, 2, 5, 1, 5, 4,
		3, 4, 7, 3, 7, 6,
		4, 5, 8, 4, 8, 7
	};
	std::vector<float> invMasses = {
		0.0f, 1.0f, 0.0f,
		1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f
	};
	Cloth *cloth = new Cloth(points, triangles, invMasses);

	cloth->SetDamping(physx::PxVec3(0.1f, 0.1f, 0.1f));

	std::vector<physx::PxVec4> planes = { physx::PxVec4(groundNormal, groundDistance) };
	std::vector<uint32_t> planesIndices = { 1 };
	cloth->SetPlaneCollisions(planes, planesIndices);

	physicsEngine->AddCloth(cloth);

	glutMainLoop();

	return 0;
}
