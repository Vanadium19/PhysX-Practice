#include "BannerScene.h"

#include <cmath>
#include "BannerSceneConfig.h"
#include "snippetrender/SnippetRender.h"

PhysicsEngine *physicsEngine = nullptr;

namespace {
BannerScene *activeScene = nullptr;

void OnKeyPressed(unsigned char, const physx::PxTransform &) {}

void OnRender() {
	if (activeScene) {
		activeScene->Render();
	}
}

void OnExit() {
	if (activeScene) {
		activeScene->Shutdown();
	}
}
}

BannerScene::BannerScene()
	: engine(nullptr),
	camera(nullptr),
	frameCounter(BannerSceneConfig::InitialFrameCounter) {
}

BannerScene::~BannerScene() {
	Shutdown();
}

void BannerScene::Run() {
	Initialize();
	glutMainLoop();
}

void BannerScene::Initialize() {
	if (engine || camera) {
		return;
	}

	activeScene = this;
	frameCounter = BannerSceneConfig::InitialFrameCounter;

	camera = new Snippets::Camera(
		physx::PxVec3(
			BannerSceneConfig::CameraPositionX,
			BannerSceneConfig::CameraPositionY,
			BannerSceneConfig::CameraPositionZ
		),
		physx::PxVec3(
			BannerSceneConfig::CameraDirectionX,
			BannerSceneConfig::CameraDirectionY,
			BannerSceneConfig::CameraDirectionZ
		)
	);
	Snippets::setupDefault(BannerSceneConfig::WindowTitle, camera, OnKeyPressed, OnRender, OnExit);

	engine = new PhysicsEngine();
	physicsEngine = engine;

	CreateWorld();
}

void BannerScene::Shutdown() {
	if (activeScene == this) {
		activeScene = nullptr;
	}

	delete camera;
	camera = nullptr;

	delete engine;
	engine = nullptr;
	physicsEngine = nullptr;
}

void BannerScene::CreateWorld() {
	physx::PxMaterial *defaultMaterial = engine->CreateMaterial(
		BannerSceneConfig::DefaultStaticFriction,
		BannerSceneConfig::DefaultDynamicFriction,
		BannerSceneConfig::DefaultRestitution
	);
	const physx::PxVec3 groundNormal(
		BannerSceneConfig::GroundNormalX,
		BannerSceneConfig::GroundNormalY,
		BannerSceneConfig::GroundNormalZ
	);

	engine->AddGround(groundNormal, BannerSceneConfig::GroundDistance, defaultMaterial);
	CreateBanner(defaultMaterial, groundNormal);
	CreateSideFlag(defaultMaterial, groundNormal);
}

void BannerScene::CreateBanner(physx::PxMaterial *defaultMaterial, const physx::PxVec3 &groundNormal) {
	ClothMesh mesh = CreateFlagMesh();
	CreateAnchorMarkers(mesh, defaultMaterial);

	Cloth *cloth = new Cloth(mesh.points, mesh.triangles, mesh.invMasses);
	cloth->SetDamping(physx::PxVec3(
		BannerSceneConfig::ClothDampingX,
		BannerSceneConfig::ClothDampingY,
		BannerSceneConfig::ClothDampingZ
	));
	cloth->SetDragCoefficient(BannerSceneConfig::ClothDragCoefficient);
	cloth->SetLiftCoefficient(BannerSceneConfig::ClothLiftCoefficient);

	std::vector<physx::PxVec4> planes = { physx::PxVec4(groundNormal, BannerSceneConfig::GroundDistance) };
	std::vector<uint32_t> planesIndices = { BannerSceneConfig::GroundPlaneConvexMask };
	cloth->SetPlaneCollisions(planes, planesIndices);

	engine->AddCloth(cloth);
}

void BannerScene::CreateSideFlag(physx::PxMaterial *defaultMaterial, const physx::PxVec3 &groundNormal) {
	ClothMesh mesh = CreateSideFlagMesh();
	CreateSideFlagAnchorMarkers(mesh, defaultMaterial);

	Cloth *cloth = new Cloth(mesh.points, mesh.triangles, mesh.invMasses);
	cloth->SetDamping(physx::PxVec3(
		BannerSceneConfig::ClothDampingX,
		BannerSceneConfig::ClothDampingY,
		BannerSceneConfig::ClothDampingZ
	));
	cloth->SetDragCoefficient(BannerSceneConfig::ClothDragCoefficient);
	cloth->SetLiftCoefficient(BannerSceneConfig::ClothLiftCoefficient);

	std::vector<physx::PxVec4> planes = { physx::PxVec4(groundNormal, BannerSceneConfig::GroundDistance) };
	std::vector<uint32_t> planesIndices = { BannerSceneConfig::GroundPlaneConvexMask };
	cloth->SetPlaneCollisions(planes, planesIndices);

	engine->AddCloth(cloth);
}

void BannerScene::CreateAnchorMarkers(const ClothMesh &mesh, physx::PxMaterial *defaultMaterial) {
	const physx::PxVec3 anchorMarkerSize(
		BannerSceneConfig::AnchorMarkerSizeX,
		BannerSceneConfig::AnchorMarkerSizeY,
		BannerSceneConfig::AnchorMarkerSizeZ
	);
	physx::PxShape *leftAnchorMarker = engine->CreateBoxShape(anchorMarkerSize, defaultMaterial, CustomFilterData::eOBSTACLE);
	physx::PxShape *rightAnchorMarker = engine->CreateBoxShape(anchorMarkerSize, defaultMaterial, CustomFilterData::eOBSTACLE);

	engine->AddStaticActor(
		leftAnchorMarker,
		mesh.points[GetFlagIndex(BannerSceneConfig::TopPinnedRow, BannerSceneConfig::LeftPinnedColumn)],
		physx::PxQuat(BannerSceneConfig::IdentityQuaternionW)
	);
	engine->AddStaticActor(
		rightAnchorMarker,
		mesh.points[GetFlagIndex(
			BannerSceneConfig::TopPinnedRow,
			BannerSceneConfig::RightPinnedColumn
		)],
		physx::PxQuat(BannerSceneConfig::IdentityQuaternionW)
	);
}

void BannerScene::CreateSideFlagAnchorMarkers(const ClothMesh &mesh, physx::PxMaterial *defaultMaterial) {
	const physx::PxVec3 anchorMarkerSize(
		BannerSceneConfig::AnchorMarkerSizeX,
		BannerSceneConfig::AnchorMarkerSizeY,
		BannerSceneConfig::AnchorMarkerSizeZ
	);
	physx::PxShape *topAnchorMarker = engine->CreateBoxShape(anchorMarkerSize, defaultMaterial, CustomFilterData::eOBSTACLE);
	physx::PxShape *bottomAnchorMarker = engine->CreateBoxShape(anchorMarkerSize, defaultMaterial, CustomFilterData::eOBSTACLE);

	engine->AddStaticActor(
		topAnchorMarker,
		mesh.points[GetFlagIndex(BannerSceneConfig::TopPinnedRow, BannerSceneConfig::RightPinnedColumn)],
		physx::PxQuat(BannerSceneConfig::IdentityQuaternionW)
	);
	engine->AddStaticActor(
		bottomAnchorMarker,
		mesh.points[GetFlagIndex(BannerSceneConfig::BottomPinnedRow, BannerSceneConfig::RightPinnedColumn)],
		physx::PxQuat(BannerSceneConfig::IdentityQuaternionW)
	);
}

void BannerScene::Render() {
	UpdateWind();
	engine->Simulate(BannerSceneConfig::SimulationStepSeconds);

	Snippets::startRender(camera, BannerSceneConfig::NearClip, BannerSceneConfig::FarClip);
	RenderActors();
	RenderCloths();
	Snippets::finishRender();

	frameCounter++;
}

void BannerScene::UpdateWind() {
	const physx::PxVec3 windVelocity = CalculateWindVelocity();
	for (Cloth *cloth : engine->GetCloths()) {
		cloth->SetWindVelocity(windVelocity);
	}
}

void BannerScene::RenderActors() {
	std::vector<physx::PxRigidActor *> actors = engine->GetActors();
	if (!actors.empty()) {
		Snippets::renderActors(actors.data(), actors.size());
	}
}

void BannerScene::RenderCloths() {
	const physx::PxVec3 color(
		BannerSceneConfig::ClothColorR,
		BannerSceneConfig::ClothColorG,
		BannerSceneConfig::ClothColorB
	);

	for (Cloth *cloth : engine->GetCloths()) {
		uint32_t particleNum = cloth->GetNumParticles();
		physx::PxVec4 *particles = cloth->GetCurrentParticles();
		std::vector<uint32_t> indices = cloth->GetMeshIndices();

		glDisable(GL_CULL_FACE);
		Snippets::renderMesh(particleNum, particles, indices.size() / 3, indices.data(), color);
		glEnable(GL_CULL_FACE);
	}
}

BannerScene::ClothMesh BannerScene::CreateFlagMesh() const {
	ClothMesh mesh;
	mesh.points.reserve(BannerSceneConfig::FlagColumns * BannerSceneConfig::FlagRows);
	mesh.triangles.reserve(
		(BannerSceneConfig::FlagColumns - BannerSceneConfig::GridNeighborOffset)
		* (BannerSceneConfig::FlagRows - BannerSceneConfig::GridNeighborOffset)
		* BannerSceneConfig::TriangleIndicesPerCell
	);
	mesh.invMasses.reserve(BannerSceneConfig::FlagColumns * BannerSceneConfig::FlagRows);

	for (uint32_t row = 0; row < BannerSceneConfig::FlagRows; row++) {
		for (uint32_t column = 0; column < BannerSceneConfig::FlagColumns; column++) {
			const float rowRatio = static_cast<float>(row)
				/ static_cast<float>(BannerSceneConfig::FlagRows - BannerSceneConfig::GridNeighborOffset);
			const float columnRatio = static_cast<float>(column)
				/ static_cast<float>(BannerSceneConfig::FlagColumns - BannerSceneConfig::GridNeighborOffset);
			const float centeredColumnRatio = columnRatio - BannerSceneConfig::Half;
			const float straightSideHeight = BannerSceneConfig::FlagHeight * BannerSceneConfig::FlagStraightSideHeightRatio;
			const float pointDepth = BannerSceneConfig::FlagHeight
				* (BannerSceneConfig::One - BannerSceneConfig::FlagStraightSideHeightRatio);
			const float pointInfluence = BannerSceneConfig::One
				- std::fabs(centeredColumnRatio) / BannerSceneConfig::Half;
			const float x = -BannerSceneConfig::FlagWidth * BannerSceneConfig::Half
				+ BannerSceneConfig::FlagWidth * columnRatio;
			const float y = BannerSceneConfig::FlagTopY
				- (straightSideHeight + pointDepth * pointInfluence) * rowRatio;
			const bool isPinnedCorner = row == BannerSceneConfig::TopPinnedRow
				&& (
					column == BannerSceneConfig::LeftPinnedColumn
					|| column == BannerSceneConfig::RightPinnedColumn
			);

			mesh.points.push_back(physx::PxVec3(x, y, BannerSceneConfig::FlagZ));
			mesh.invMasses.push_back(
				isPinnedCorner
					? BannerSceneConfig::PinnedParticleInvMass
					: BannerSceneConfig::FreeParticleInvMass
			);
		}
	}

	for (uint32_t row = 0; row < BannerSceneConfig::FlagRows - BannerSceneConfig::GridNeighborOffset; row++) {
		for (uint32_t column = 0; column < BannerSceneConfig::FlagColumns - BannerSceneConfig::GridNeighborOffset; column++) {
			const uint32_t topLeft = GetFlagIndex(row, column);
			const uint32_t topRight = GetFlagIndex(row, column + BannerSceneConfig::GridNeighborOffset);
			const uint32_t bottomLeft = GetFlagIndex(row + BannerSceneConfig::GridNeighborOffset, column);
			const uint32_t bottomRight = GetFlagIndex(
				row + BannerSceneConfig::GridNeighborOffset,
				column + BannerSceneConfig::GridNeighborOffset
			);

			mesh.triangles.push_back(topLeft);
			mesh.triangles.push_back(topRight);
			mesh.triangles.push_back(bottomRight);

			mesh.triangles.push_back(topLeft);
			mesh.triangles.push_back(bottomRight);
			mesh.triangles.push_back(bottomLeft);
		}
	}

	return mesh;
}

BannerScene::ClothMesh BannerScene::CreateSideFlagMesh() const {
	ClothMesh mesh;
	mesh.points.reserve(BannerSceneConfig::FlagColumns * BannerSceneConfig::FlagRows);
	mesh.triangles.reserve(
		(BannerSceneConfig::FlagColumns - BannerSceneConfig::GridNeighborOffset)
		* (BannerSceneConfig::FlagRows - BannerSceneConfig::GridNeighborOffset)
		* BannerSceneConfig::TriangleIndicesPerCell
	);
	mesh.invMasses.reserve(BannerSceneConfig::FlagColumns * BannerSceneConfig::FlagRows);

	for (uint32_t row = 0; row < BannerSceneConfig::FlagRows; row++) {
		for (uint32_t column = 0; column < BannerSceneConfig::FlagColumns; column++) {
			const float rowRatio = static_cast<float>(row)
				/ static_cast<float>(BannerSceneConfig::FlagRows - BannerSceneConfig::GridNeighborOffset);
			const float columnRatio = static_cast<float>(column)
				/ static_cast<float>(BannerSceneConfig::FlagColumns - BannerSceneConfig::GridNeighborOffset);
			const float x = BannerSceneConfig::SideFlagRightX
				- BannerSceneConfig::SideFlagWidth
				+ BannerSceneConfig::SideFlagWidth * columnRatio;
			const float y = BannerSceneConfig::SideFlagTopY - BannerSceneConfig::SideFlagHeight * rowRatio;
			const bool isPinnedSidePoint = column == BannerSceneConfig::RightPinnedColumn
				&& (
					row == BannerSceneConfig::TopPinnedRow
					|| row == BannerSceneConfig::BottomPinnedRow
				);

			mesh.points.push_back(physx::PxVec3(x, y, BannerSceneConfig::SideFlagZ));
			mesh.invMasses.push_back(
				isPinnedSidePoint
					? BannerSceneConfig::PinnedParticleInvMass
					: BannerSceneConfig::FreeParticleInvMass
			);
		}
	}

	for (uint32_t row = 0; row < BannerSceneConfig::FlagRows - BannerSceneConfig::GridNeighborOffset; row++) {
		for (uint32_t column = 0; column < BannerSceneConfig::FlagColumns - BannerSceneConfig::GridNeighborOffset; column++) {
			const uint32_t topLeft = GetFlagIndex(row, column);
			const uint32_t topRight = GetFlagIndex(row, column + BannerSceneConfig::GridNeighborOffset);
			const uint32_t bottomLeft = GetFlagIndex(row + BannerSceneConfig::GridNeighborOffset, column);
			const uint32_t bottomRight = GetFlagIndex(
				row + BannerSceneConfig::GridNeighborOffset,
				column + BannerSceneConfig::GridNeighborOffset
			);

			mesh.triangles.push_back(topLeft);
			mesh.triangles.push_back(topRight);
			mesh.triangles.push_back(bottomRight);

			mesh.triangles.push_back(topLeft);
			mesh.triangles.push_back(bottomRight);
			mesh.triangles.push_back(bottomLeft);
		}
	}

	return mesh;
}

physx::PxVec3 BannerScene::CalculateWindVelocity() const {
	const float time = static_cast<float>(frameCounter) * BannerSceneConfig::SimulationStepSeconds;
	const float strength = BannerSceneConfig::WindBaseStrength
		+ BannerSceneConfig::WindStrengthAmplitude
		* (
			BannerSceneConfig::Half
			+ BannerSceneConfig::Half * std::sin(time * BannerSceneConfig::WindStrengthFrequency)
		);
	const float angle = time * BannerSceneConfig::WindDirectionAngularSpeed;

	return physx::PxVec3(
		std::cos(angle),
		BannerSceneConfig::WindVerticalAmplitude * std::sin(time),
		std::sin(angle)
	) * strength;
}

uint32_t BannerScene::GetFlagIndex(uint32_t row, uint32_t column) const {
	return row * BannerSceneConfig::FlagColumns + column;
}
