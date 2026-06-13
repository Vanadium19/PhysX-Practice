#include "Cloth.h"
#include "PhysicsEngine.h"

extern PhysicsEngine *physicsEngine;

// Шпаргалка: конструктор получает готовый меш ткани и создаёт из него NvCloth-объект.
// points задают положение вершин, triangles задают треугольники, invMasses задают закрепление:
// invMass = 0 фиксирует частицу, invMass > 0 оставляет её свободной.
Cloth::Cloth(std::vector<physx::PxVec3> points, std::vector<uint32_t> triangles, std::vector<float> invMasses) {
	nv::cloth::Factory *factory = physicsEngine->factory;
	
	// Шпаргалка: ClothMeshDesc описывает исходную геометрию для NvCloth cooker.
	nv::cloth::ClothMeshDesc desc;
	desc.setToDefault();
	desc.points.count = points.size();
	desc.points.stride = sizeof(physx::PxVec3);
	desc.points.data = points.data();
	desc.triangles.count = triangles.size() / 3;
	desc.triangles.stride = 3 * sizeof(uint32_t);
	desc.triangles.data = triangles.data();
	desc.invMasses.count = invMasses.size();
	desc.invMasses.stride = sizeof(float);
	desc.invMasses.data = invMasses.data();

	// Шпаргалка: fabric хранит внутренние constraints ткани, рассчитанные по мешу и гравитации.
	fabric = NvClothCookFabricFromMesh(factory, desc, physicsEngine->scene->getGravity());

	// Шпаргалка: NvCloth хранит частицу как PxVec4, где xyz это позиция, а w это обратная масса.
	std::vector<physx::PxVec4> particles(points.size());
	for (size_t i = 0; i < points.size(); i++) {
		particles[i] = physx::PxVec4(points[i], invMasses[i]);
	}
	nv::cloth::Range<physx::PxVec4> particlesRange(particles.data(), particles.data() + particles.size());
	// Шпаргалка: createCloth создаёт симулируемую ткань из частиц и заранее приготовленного fabric.
	cloth = factory->createCloth(particlesRange, *fabric);
	cloth->setGravity(physicsEngine->scene->getGravity());

	// Шпаргалка: индексы сохраняются отдельно, чтобы рендерить тот же меш после симуляции.
	indices = triangles;
}

// Шпаргалка: деструктор освобождает NvCloth-объект и уменьшает счётчик ссылок fabric.
Cloth::~Cloth() {
	NV_CLOTH_DELETE(cloth);
	fabric->decRefCount();
}

// Шпаргалка: возвращает низкоуровневый nv::cloth::Cloth, который нужен физическому движку.
nv::cloth::Cloth *Cloth::Get() const {
	return cloth;
}

// Шпаргалка: возвращает индексы треугольников исходного меша для отрисовки ткани.
std::vector<uint32_t> Cloth::GetMeshIndices() const {
	return indices;
}

// Шпаргалка: возвращает количество частиц, то есть количество вершин cloth-меша.
uint32_t Cloth::GetNumParticles() const {
	return cloth->getNumParticles();
}

// Шпаргалка: возвращает текущие позиции частиц после шага симуляции.
physx::PxVec4 *Cloth::GetCurrentParticles() const {
	return &cloth->getCurrentParticles()[0];
}

// Шпаргалка: задаёт collision planes, чтобы ткань могла сталкиваться с плоскостью пола.
void Cloth::SetPlaneCollisions(std::vector<physx::PxVec4> planes, std::vector<uint32_t> planesIndices) {
	nv::cloth::Range<physx::PxVec4> planesRange(planes.data(), planes.data() + planes.size());
	cloth->setPlanes(planesRange, 0, 0);

	nv::cloth::Range<uint32_t> planesIndicesRange(planesIndices.data(), planesIndices.data() + planesIndices.size());
	cloth->setConvexes(planesIndicesRange, 0, 0);
}

// Шпаргалка: задаёт damping, который гасит скорость частиц и делает движение менее резким.
void Cloth::SetDamping(physx::PxVec3 damping) {
	cloth->setDamping(damping);
}

// Шпаргалка: задаёт сопротивление воздуха, через которое ветер начинает заметно влиять на ткань.
void Cloth::SetDragCoefficient(float dragCoefficient) {
	cloth->setDragCoefficient(dragCoefficient);
}

// Шпаргалка: задаёт подъёмную силу от потока воздуха для дополнительного эффекта колыхания.
void Cloth::SetLiftCoefficient(float liftCoefficient) {
	cloth->setLiftCoefficient(liftCoefficient);
}

// Шпаргалка: задаёт текущую скорость ветра, которую сцена обновляет каждый кадр.
void Cloth::SetWindVelocity(physx::PxVec3 wind) {
	cloth->setWindVelocity(wind);
}
