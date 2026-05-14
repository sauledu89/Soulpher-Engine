#pragma once
#include "Prerequisites.h"
#include "EngineUtilities/Vectors/Vector3.h"

class Mesh;
class MaterialInstance;

// ---------------------------------------------------------------------------
// Enumeraciones de render
// ---------------------------------------------------------------------------

enum class MaterialDomain {
    Opaque = 0,
    Masked,
    Transparent
};

enum class BlendMode {
    Opaque = 0,
    Alpha,
    Additive,
    PremultipliedAlpha
};

enum class RenderPassType {
    Shadow = 0,
    Opaque,
    Transparent
};

enum class LightType {
    Directional = 0,
    Point,
    Spot
};

// ---------------------------------------------------------------------------
// Datos de luz (usado tanto en LightComponent como en RenderScene)
// ---------------------------------------------------------------------------

struct LightData {
    LightType    type      = LightType::Directional;
    EU::Vector3  color     = EU::Vector3(1.0f, 1.0f, 1.0f);
    float        intensity = 1.0f;

    EU::Vector3  direction = EU::Vector3(0.0f, -1.0f, 0.0f);
    float        range     = 0.0f;

    EU::Vector3  position  = EU::Vector3(0.0f, 0.0f, 0.0f);
    float        spotAngle = 0.0f;
};

// ---------------------------------------------------------------------------
// Parametros PBR por instancia de material
// ---------------------------------------------------------------------------

struct MaterialParams {
    XMFLOAT4 baseColor         = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    float    metallic          = 0.0f;
    float    roughness         = 0.5f;
    float    ao                = 1.0f;
    float    normalScale       = 1.0f;
    float    emissiveStrength  = 0.0f;
    float    alphaCutoff       = 0.5f;
};

// ---------------------------------------------------------------------------
// Constant buffers (deben coincidir exactamente con el layout HLSL)
// ---------------------------------------------------------------------------

struct CBPerFrame {
    XMFLOAT4X4  View;
    XMFLOAT4X4  Projection;
    XMFLOAT4X4  LightViewProjection;
    EU::Vector3 CameraPos;
    float       pad0 = 0.0f;
    EU::Vector3 LightDir   = EU::Vector3(0.0f, -1.0f, 0.0f);
    float       pad1 = 0.0f;
    EU::Vector3 LightColor = EU::Vector3(1.0f, 1.0f, 1.0f);
    float       ShadowBias = 0.003f;
};

struct CBPerObject {
    XMFLOAT4X4 World;
};

struct CBPerMaterial {
    XMFLOAT4 BaseColor        = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    float    Metallic         = 0.0f;
    float    Roughness        = 0.5f;
    float    AO               = 1.0f;
    float    NormalScale      = 1.0f;
    float    EmissiveStrength = 0.0f;
    float    AlphaCutoff      = 0.0f;
    float    pad0             = 0.0f;
    float    pad1             = 0.0f;
};

// ---------------------------------------------------------------------------
// Objeto renderable (snapshot de un actor para el frame actual)
// ---------------------------------------------------------------------------

struct RenderObject {
    Mesh*                       mesh              = nullptr;
    MaterialInstance*           materialInstance  = nullptr;
    std::vector<MaterialInstance*> materialInstances;
    XMMATRIX                    world             = XMMatrixIdentity();
    bool                        castShadow        = true;
    bool                        transparent       = false;
    float                       distanceToCamera  = 0.0f;
};
