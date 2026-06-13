#pragma once

#include <cstdint>
#include <vector>
#include "PhysicsEngine.h"
#include "snippetrender/SnippetCamera.h"

class BannerScene {
public:
	BannerScene();
	~BannerScene();

	void Run();
	void Shutdown();
	void Render();

private:
	struct ClothMesh {
		std::vector<physx::PxVec3> points;
		std::vector<uint32_t> triangles;
		std::vector<float> invMasses;
	};

	void Initialize();
	void CreateWorld();
	void CreateBanner(physx::PxMaterial *defaultMaterial, const physx::PxVec3 &groundNormal);
	void CreateAnchorMarkers(const ClothMesh &mesh, physx::PxMaterial *defaultMaterial);
	void RenderActors();
	void RenderCloths();
	void UpdateWind();

	ClothMesh CreateFlagMesh() const;
	physx::PxVec3 CalculateWindVelocity() const;
	uint32_t GetFlagIndex(uint32_t row, uint32_t column) const;

	PhysicsEngine *engine;
	Snippets::Camera *camera;
	uint32_t frameCounter;
};
