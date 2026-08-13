/**
 * @file RenderTypes.h
 * @brief Tipos de datos centrales del sistema de render: enums, structs de luz,
 *        constant buffers y el objeto de render por frame.
 *
 * @details
 * Este archivo es el "contrato" entre el C++ del motor y los shaders HLSL.
 * Cualquier struct con prefijo `CB` (Constant Buffer) **debe coincidir byte a byte**
 * con su equivalente en los shaders:
 *  - `CBPerFrame`    ↔ `cbuffer CBPerFrame : register(b0)`
 *  - `CBPerObject`   ↔ `cbuffer CBPerObject : register(b1)`
 *  - `CBPerMaterial` ↔ `cbuffer CBPerMaterial : register(b2)`
 *
 * @warning Si añades o reordenas campos en algún CB aquí, actualiza el bloque
 * correspondiente en el shader y viceversa. Un mismatch silencioso produce
 * artefactos visuales muy difíciles de depurar.
 *
 * @note [GameDev] Los "constant buffers" son la forma más eficiente de enviar
 * parámetros de CPU a GPU en DX11. Se agrupan por frecuencia de actualización:
 *  - b0 (por frame): View, Projection, datos de luz — 1 update/frame.
 *  - b1 (por objeto): World matrix — 1 update/draw call.
 *  - b2 (por material): color, PBR params — cambia al cambiar material.
 * Esta estrategia minimiza el número de `UpdateSubresource` costosos.
 *
 * @note [GameDev] El sistema multi-luz (`LightPositionsRanges`, `LightColorsTypes`,
 * `LightDirectionsIntensities`) usa arreglos de `float4` para empaquetar los datos
 * de hasta `kMaxSceneLights` luces en un único constant buffer. Esto permite iluminar
 * una escena con múltiples luces dinámicas en un solo draw call, a diferencia del
 * Forward+ que requiere un light culling pass separado.
 *
 * @see ForwardRenderer, DeferredRenderer, BaseApp, Soulpher-Engine.fx
 */

#pragma once
#include "Prerequisites.h"
#include "EngineUtilities/Vectors/Vector3.h"

/** @brief Número máximo de luces dinámicas por frame. */
constexpr int kMaxSceneLights = 8;

class Mesh;
class MaterialInstance;

// ---------------------------------------------------------------------------
// Enumeraciones de render
// ---------------------------------------------------------------------------

/**
 * @enum MaterialDomain
 * @brief Define en qué pass de render se procesa un material.
 *
 * @note [GameDev] Separar opacos de transparentes es obligatorio en forward rendering
 * para que el blending funcione correctamente. Los opacos se dibujan primero (el orden
 * no importa gracias al depth test), los transparentes después ordenados back-to-front.
 */
enum class MaterialDomain {
    Opaque = 0,  ///< Material sólido, pasa depth test normal.
    Masked,      ///< Alpha cutoff: descarta píxeles bajo un umbral (ej. hojas de árbol).
    Transparent  ///< Blending alfa, requiere ordenación back-to-front.
};

/**
 * @enum BlendMode
 * @brief Modo de mezcla de color para el output merger.
 *
 * @note [GameDev] El blending define cómo se combina el píxel nuevo con el
 * que ya hay en el render target. Opaque ignora el alpha completamente;
 * Alpha hace `result = src.rgb * src.a + dst.rgb * (1 - src.a)` (blend clásico);
 * Additive suma colores (efectos de luz, partículas de fuego).
 */
enum class BlendMode {
    Opaque = 0,          ///< Sin mezcla; el píxel nuevo sobreescribe el anterior.
    Alpha,               ///< Mezcla alfa estándar (transparencia).
    Additive,            ///< Suma de colores (útil para partículas y efectos de luz).
    PremultipliedAlpha   ///< Alfa premultiplicado (evita halos en texturas con canal alfa).
};

/**
 * @enum RenderPassType
 * @brief Identifica el pass de render en el que se ejecuta un draw call.
 *
 * @note [GameDev] El orden de los passes importa: Shadow primero (escribe el shadow map),
 * luego Opaque (usa el shadow map para calcular oclusión), luego Transparent (blend
 * sobre los opacos), y finalmente Skybox (siempre detrás de todo gracias al depth=1).
 * El pass Editor es para herramientas de depuración (gizmos, líneas de raycast, etc.)
 * que se dibujan encima de la escena sin afectar el depth buffer del juego.
 */
enum class RenderPassType {
    Shadow = 0,  ///< Pass de profundidad desde el punto de vista de la luz.
    Opaque,      ///< Pass de color para objetos sólidos.
    Transparent, ///< Pass de color para objetos transparentes (blend alfa).
    Skybox,      ///< Pass del cielo/environment (siempre al fondo, depth=1).
    Editor       ///< Pass de herramientas de editor (gizmos, wireframe debug).
};

/**
 * @enum LightType
 * @brief Tipo de fuente de luz.
 *
 * @note [GameDev] Los tres tipos clásicos de luz en cualquier motor 3D.
 * La directional no tiene posición (solo dirección, como el sol).
 * La point irradia en todas las direcciones desde un punto.
 * La spot es un cono (ej. linterna, foco de escenario).
 */
enum class LightType {
    Directional = 0, ///< Luz infinitamente lejana con dirección fija (sol, luna).
    Point,           ///< Luz puntual que irradia en todas las direcciones con `range`.
    Spot             ///< Cono de luz con `spotAngle` que define la apertura.
};

// ---------------------------------------------------------------------------
// Datos de luz (usado tanto en LightComponent como en RenderScene)
// ---------------------------------------------------------------------------

/**
 * @struct LightData
 * @brief Parámetros de una fuente de luz, compartidos entre CPU y el sistema de render.
 *
 * @note Los campos `position` y `range` solo aplican a Point/Spot.
 * `direction` solo aplica a Directional/Spot. `spotAngle` solo a Spot.
 */
struct LightData {
    LightType    type      = LightType::Directional;         ///< Tipo de luz.
    EU::Vector3  color     = EU::Vector3(1.0f, 1.0f, 1.0f); ///< Color RGB normalizado.
    float        intensity = 1.0f;                           ///< Multiplicador de intensidad.

    EU::Vector3  direction = EU::Vector3(0.0f, -1.0f, 0.0f); ///< Dirección DE la luz (normalizada).
    float        range     = 0.0f;                            ///< Radio de influencia (Point/Spot).

    EU::Vector3  position  = EU::Vector3(0.0f, 0.0f, 0.0f); ///< Posición en el mundo (Point/Spot).
    float        spotAngle = 0.0f;                           ///< Ángulo del cono en radianes (Spot).
};

// ---------------------------------------------------------------------------
// Parametros PBR por instancia de material
// ---------------------------------------------------------------------------

/**
 * @struct MaterialParams
 * @brief Parámetros PBR numéricos por instancia de material (CPU-side).
 *
 * @note Estos valores se envían a GPU como `CBPerMaterial` (b2).
 * Los valores en [0,1] reflejan convenciones físicas: `metallic=1` = metal puro,
 * `roughness=0` = espejo perfecto, `roughness=1` = completamente difuso.
 */
struct MaterialParams {
    XMFLOAT4 baseColor         = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f); ///< Color base RGBA (multiplica la textura de albedo).
    float    metallic          = 0.0f;  ///< 0 = dieléctrico, 1 = metal puro.
    float    roughness         = 0.5f;  ///< 0 = espejo, 1 = completamente rugoso.
    float    ao                = 1.0f;  ///< Ambient occlusion global (1 = sin oclusión).
    float    normalScale       = 1.0f;  ///< Intensidad del normal map (1 = original).
    float    emissiveStrength  = 0.0f;  ///< Multiplicador de emisión (0 = sin luz propia).
    float    alphaCutoff       = 0.5f;  ///< Umbral de alpha para MaterialDomain::Masked.
    XMFLOAT2 uvTiling          = XMFLOAT2(1.0f, 1.0f); ///< Repeticiones de la textura en U/V — multiplica las UVs del vértice antes de samplear (1,1 = sin tiling extra).
    XMFLOAT2 uvOffset          = XMFLOAT2(0.0f, 0.0f); ///< Desplazamiento de UV tras aplicar el tiling (0,0 = sin desplazamiento) — util para encuadrar un patron o animar texturas (agua, cintas).
};

// ---------------------------------------------------------------------------
// Constant buffers (deben coincidir exactamente con el layout HLSL)
// ---------------------------------------------------------------------------

/**
 * @struct CBPerFrame
 * @brief Constant buffer b0: datos que cambian una vez por frame.
 *
 * @details
 * Contiene las matrices de cámara, la LightViewProjection para el shadow map,
 * los parámetros de la luz principal y los arreglos multi-luz para deferred.
 *
 * @warning Los floats `pad0`, `pad1` y `pad2` son relleno obligatorio para que los
 * `Vector3` (12 bytes) queden alineados a 16 bytes como exige HLSL.
 * **No los elimines** aunque parezcan innecesarios.
 *
 * @note [GameDev] Los arreglos `LightPositionsRanges`, `LightColorsTypes` y
 * `LightDirectionsIntensities` empaquetan 4 floats por luz en un `float4` para
 * minimizar el espacio en el constant buffer. En HLSL se acceden por índice con
 * `LightPositionsRanges[i].xyz` (posición) y `LightPositionsRanges[i].w` (range).
 * El número real de luces activas en el frame se pasa con `LightCount`.
 */
struct CBPerFrame {
    XMFLOAT4X4  View{};                                              ///< Matriz de vista (transpuesta antes de subir).
    XMFLOAT4X4  Projection{};                                        ///< Matriz de proyección (transpuesta antes de subir).
    XMFLOAT4X4  LightViewProjection{};                               ///< View*Proj desde el punto de vista de la luz (shadow map).
    EU::Vector3 CameraPos{};                                         ///< Posición de la cámara en espacio mundo.
    float       pad0 = 0.0f;                                         ///< Padding HLSL: float3 → float4.
    EU::Vector3 LightDir   = EU::Vector3(0.0f, -1.0f, 0.0f);        ///< Dirección DE la luz (normalizada, apunta hacia la escena).
    float       pad1 = 0.0f;                                         ///< Padding HLSL: float3 → float4.
    EU::Vector3 LightColor = EU::Vector3(1.0f, 1.0f, 1.0f);         ///< Color de la luz principal.
    float       LightRange = 10.0f;                                  ///< Radio de influencia de la luz principal (Point/Spot).
    EU::Vector3 LightPosition = EU::Vector3(0.0f, 3.0f, 0.0f);      ///< Posición de la luz principal en espacio mundo.
    int         LightType = 0;                                       ///< Tipo de luz principal (0=Directional, 1=Point, 2=Spot).
    XMFLOAT4    LightPositionsRanges[kMaxSceneLights]{};             ///< xyz=posición, w=range de cada luz dinámica.
    XMFLOAT4    LightColorsTypes[kMaxSceneLights]{};                 ///< xyz=color RGB, w=tipo de cada luz dinámica.
    XMFLOAT4    LightDirectionsIntensities[kMaxSceneLights]{};       ///< xyz=dirección, w=intensidad de cada luz dinámica.
    int         LightCount = 0;                                      ///< Número de luces activas en el frame actual.
    XMFLOAT3    pad2 = XMFLOAT3(0.0f, 0.0f, 0.0f);                 ///< Padding para alinear LightCount a 16 bytes.
    XMFLOAT4    LightSpotAngles[kMaxSceneLights]{};                  ///< .x = semiángulo del cono (radianes) de cada luz Spot; resto sin usar.
};

/**
 * @struct CBPerObject
 * @brief Constant buffer b1: datos que cambian por draw call (World matrix).
 *
 * @note La matriz se transpone antes de subir a GPU porque HLSL usa column-major
 * por defecto y XNAMath usa row-major.
 */
struct CBPerObject {
    XMFLOAT4X4 World; ///< Matriz de transformación local → mundo (transpuesta).
};

/**
 * @struct CBPerMaterial
 * @brief Constant buffer b2: parámetros PBR que cambian por material/instancia.
 *
 * @note `pad0`–`pad1` alinean el struct a 64 bytes (múltiplo de 16) para cumplir el
 * requisito de HLSL de que los constant buffers sean múltiplos de 16 bytes. `UVTiling` y
 * `UVOffset` ocupan los cuatro floats que antes eran `pad2`–`pad5` — mismo layout binario,
 * solo dos `float2` con nombre en vez de floats sueltos sin usar.
 */
struct CBPerMaterial {
    XMFLOAT4 BaseColor        = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f); ///< Color base RGBA.
    float    Metallic         = 0.0f;  ///< 0 = dieléctrico, 1 = metal puro.
    float    Roughness        = 0.5f;  ///< 0 = espejo perfecto, 1 = difuso total.
    float    AO               = 1.0f;  ///< Oclusión ambiental (1 = sin oclusión).
    float    NormalScale      = 1.0f;  ///< Intensidad del normal map.
    float    EmissiveStrength = 0.0f;  ///< Intensidad de emisión.
    float    AlphaCutoff      = 0.0f;  ///< Umbral de alpha para Masked.
    float    pad0             = 0.0f;  ///< Padding de alineación HLSL.
    float    pad1             = 0.0f;  ///< Padding de alineación HLSL.
    XMFLOAT2 UVTiling         = XMFLOAT2(1.0f, 1.0f); ///< Repeticiones de textura en U/V — multiplica las UVs antes de samplear (ver Soulpher-Engine.fx / DeferredGBuffer.hlsl).
    XMFLOAT2 UVOffset         = XMFLOAT2(0.0f, 0.0f); ///< Desplazamiento de UV aplicado despues del tiling.
};

// ---------------------------------------------------------------------------
// Objeto renderable (snapshot de un actor para el frame actual)
// ---------------------------------------------------------------------------

/**
 * @struct RenderObject
 * @brief Snapshot de un actor para el frame actual, listo para el renderer.
 *
 * @details
 * Un `RenderObject` es un "ticket de render": contiene todo lo que el
 * `ForwardRenderer` necesita para dibujar un objeto sin referirse al Actor original.
 * Se crea al inicio del frame y se descarta al finalizar.
 *
 * @note [GameDev] Desacoplar el Actor (lógica de juego) del RenderObject (datos de render)
 * es la base del "render thread separation" de Unreal Engine. El game thread produce
 * RenderObjects; el render thread los consume de forma independiente.
 * En este motor ambos están en el mismo thread, pero la estructura ya está preparada
 * para esa separación futura.
 *
 * @note [GameDev] `tint` existe porque en algún momento el resaltado de selección del
 * viewport (Outliner/picking) escribía directamente sobre `MaterialInstance::getParams()
 * .baseColor` cada frame — mutaba el ÚNICO `MaterialParams` que el actor y el Material
 * Editor comparten. El bug resultante: el usuario bajaba el slider de Base Color, pero al
 * siguiente frame `BaseApp::render()` lo pisaba con el tinte "normal" (blanco) antes de
 * que el draw call lo usara — el valor editado nunca sobrevivía un frame. La lección
 * general: cuando varios sistemas (aquí, "el actor está seleccionado" y "el usuario está
 * editando su material") quieren influir el mismo dato compartido, hay que separar cuál
 * es la fuente de verdad persistente (`MaterialParams`, propiedad del material) de cuál es
 * un modificador transitorio de UN SOLO draw (`tint`, propiedad del `RenderObject` de este
 * frame) — igual que Unreal separa el color base de un `MaterialInstance` del "selection
 * outline" que dibuja el editor encima, sin tocar el asset.
 */
struct RenderObject {
    Mesh*                          mesh              = nullptr;          ///< Geometría GPU a dibujar.
    MaterialInstance*              materialInstance  = nullptr;          ///< Material principal (legacy).
    std::vector<MaterialInstance*> materialInstances;                    ///< Materiales por submalla (uno por Submesh).
    XMMATRIX                       world             = XMMatrixIdentity(); ///< Matriz de transformación al mundo.
    bool                           castShadow        = true;             ///< Si este objeto participa en el shadow pass.
    bool                           transparent       = false;            ///< Si pertenece a la lista transparente.
    float                          distanceToCamera  = 0.0f;             ///< Distancia a la cámara (para ordenación back-to-front en transparentes).
    XMFLOAT3                       tint              = XMFLOAT3(1.0f, 1.0f, 1.0f); ///< Multiplicador RGB aplicado sobre BaseColor solo en este draw (ej. resaltado de selección). No se escribe en MaterialParams — así el color autorado por el Material Editor no se pierde entre frames.
};
