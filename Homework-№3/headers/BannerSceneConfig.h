#pragma once

#include <cstdint>

namespace BannerSceneConfig {
// Заголовок окна, которое создаёт примерный GLUT-рендерер.
static const char WindowTitle[] = "PhysX NvCloth Banner";

// Фиксированный шаг симуляции для PhysX, NvCloth и анимации ветра.
constexpr float SimulationStepSeconds = 1.0f / 60.0f;

// Начальный номер кадра, чтобы время ветра начиналось с нуля.
constexpr uint32_t InitialFrameCounter = 0;

// Положение камеры, из которого видны весь баннер и плоскость пола.
constexpr float CameraPositionX = 0.0f;
constexpr float CameraPositionY = 10.0f;
constexpr float CameraPositionZ = 30.0f;

// Направление камеры: немного вниз, в сторону ткани.
constexpr float CameraDirectionX = 0.0f;
constexpr float CameraDirectionY = -0.1f;
constexpr float CameraDirectionZ = -0.3f;

// Ближняя и дальняя плоскости отсечения для рендера.
constexpr float NearClip = 0.1f;
constexpr float FarClip = 10000.0f;

// Синий цвет, которым отображается меш ткани.
constexpr float ClothColorR = 0.0f;
constexpr float ClothColorG = 0.0f;
constexpr float ClothColorB = 1.0f;

// Параметры PhysX-материала для пола и визуальных маркеров крепления.
constexpr float DefaultStaticFriction = 0.5f;
constexpr float DefaultDynamicFriction = 0.5f;
constexpr float DefaultRestitution = 0.1f;

// Нормаль плоскости пола и её расстояние от начала координат.
constexpr float GroundNormalX = 0.0f;
constexpr float GroundNormalY = 1.0f;
constexpr float GroundNormalZ = 0.0f;
constexpr float GroundDistance = 0.5f;

// Маска convex-коллизии NvCloth для единственной плоскости пола.
constexpr uint32_t GroundPlaneConvexMask = 1;

// Разрешение сетки баннера: 12 x 8 даёт 96 частиц, что больше требуемых 50.
constexpr uint32_t FlagColumns = 12;
constexpr uint32_t FlagRows = 8;

// Каждая четырёхугольная ячейка рендерится двумя треугольниками по три индекса.
constexpr uint32_t TriangleIndicesPerCell = 6;

// Именованные смещения сетки для обращения к вершинам баннера.
constexpr uint32_t TopPinnedRow = 0;
constexpr uint32_t LeftPinnedColumn = 0;
constexpr uint32_t GridNeighborOffset = 1;

// Размеры баннера и его положение в мировых координатах.
constexpr float FlagWidth = 8.0f;
constexpr float FlagHeight = 4.5f;
constexpr float FlagTopY = 6.0f;
constexpr float FlagZ = 0.0f;

// Обратные массы NvCloth: ноль закрепляет частицу, единица оставляет её свободной.
constexpr float PinnedParticleInvMass = 0.0f;
constexpr float FreeParticleInvMass = 1.0f;

// Множитель половины ширины, чтобы центрировать баннер вокруг X = 0.
constexpr float Half = 0.5f;

// Демпфирование NvCloth, применяемое к скорости частиц по каждой оси.
constexpr float ClothDampingX = 0.08f;
constexpr float ClothDampingY = 0.08f;
constexpr float ClothDampingZ = 0.08f;

// Аэродинамические коэффициенты, через которые ветер действует на треугольники ткани.
constexpr float ClothDragCoefficient = 0.08f;
constexpr float ClothLiftCoefficient = 0.04f;

// Сила ветра плавно меняется от базового значения до базового значения плюс амплитуда.
constexpr float WindBaseStrength = 2.0f;
constexpr float WindStrengthAmplitude = 3.0f;
constexpr float WindStrengthFrequency = 0.7f;

// Направление ветра вращается в плоскости XZ с небольшим вертикальным колебанием.
constexpr float WindDirectionAngularSpeed = 0.9f;
constexpr float WindVerticalAmplitude = 0.15f;

// Статические кубики-маркеры, расположенные в двух закреплённых углах баннера.
constexpr float AnchorMarkerSizeX = 0.18f;
constexpr float AnchorMarkerSizeY = 0.18f;
constexpr float AnchorMarkerSizeZ = 0.18f;

// Скаляр единичного кватерниона для статических акторов без поворота.
constexpr float IdentityQuaternionW = 1.0f;
}
