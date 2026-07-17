/**
 * @file Skybox.h
 * @brief Cubo unitario con una panorámica equirectangular .dds, dibujado como fondo.
 *
 * @details
 * Se dibuja como un pase forward independiente después del lighting pass del
 * DeferredRenderer (fullscreen quad que ya llenó el render target), usando el depth
 * del geometry pass con comparación LESS_EQUAL y depth write deshabilitado: el vertex
 * shader fuerza z=w (profundidad 1.0 tras la división perspectiva) para que el cubo
 * solo sea visible donde ningún objeto opaco escribió un depth más cercano.
 *
 * La textura se carga como Texture2D normal (no TextureCube): el .dds disponible es
 * una panorámica equirectangular 2:1 (ancho ≠ alto, típica de HDRIs descargados), no
 * un cubemap de 6 caras — el pixel shader convierte la dirección 3D interpolada a UV
 * esféricas (atan2/asin) en vez de usar TextureCube::Sample.
 *
 * @note [GameDev] Un skybox es un cubo o esfera enorme centrado en la cámara que simula
 * el fondo del cielo/ambiente. Se dibuja DESPUÉS de los opacos con depth write OFF y
 * depth func LESS_EQUAL para que siempre quede "detrás" de la geometría (z=1 tras la
 * división perspectiva). En engines AAA se usa un "sky dome" con scattering atmosférico.
 *
 * @see DeferredRenderer::renderSkyboxPass, RenderScene::skybox, Skybox.hlsl
 * @ingroup rendering
 */
#pragma once
#include "Prerequisites.h"
#include "Buffer.h"
#include "ShaderProgram.h"
#include "DepthStencilState.h"
#include "RasterizerState.h"
#include "SamplerState.h"
#include "Texture.h"

class Device;
class DeviceContext;
class Camera;

/** @brief Constant buffer b0 del shader del skybox: únicamente la matriz ViewProj (transpuesta). */
struct CBSkybox {
    XMFLOAT4X4 ViewProj{};
};

/**
 * @class Skybox
 * @brief Renderizable de fondo: se dibuja en el skybox pass del DeferredRenderer.
 *
 * @details
 * `RenderScene::skybox` es un puntero no-owning — `BaseApp` posee la única instancia
 * real (`m_skybox`) y solo asigna su dirección cada frame. Si `init()` falla (archivo
 * no encontrado, DDS corrupto), `m_initialized` queda en `false` y `render()` se
 * convierte en no-op silencioso: el motor sigue funcionando sin fondo en vez de crashear
 * por un asset faltante — el mismo principio de "fallo no fatal" que ya usa el motor
 * para texturas de actor (fallback a checkerboard) y modelos FBX opcionales.
 *
 * @note [GameDev] Equirectangular vs. cubemap es una decisión de contenido, no solo de
 * código: un cubemap (6 texturas cuadradas o una `TextureCube`) tiene mejor distribución
 * de píxeles por estereorradián (menos distorsión en los polos) y es más barato de
 * muestrear en el pixel shader, pero requiere herramientas de conversión o renderizado
 * en 6 direcciones. Una panorámica equirectangular (una sola imagen 2:1, como la que usa
 * este motor) es el formato nativo de casi todos los HDRIs descargables — el trade-off
 * es aceptar la distorsión hacia los polos (cenit/nadir) y el costo extra de un
 * `atan2`/`asin` por píxel en el shader (ver `Skybox.hlsl`) en vez de un
 * `TextureCube::Sample` directo. Unreal Engine y Unity aceptan ambos formatos y
 * convierten equirectangular→cubemap en el importer cuando hace falta.
 */
class Skybox {
public:
    Skybox()  = default;
    ~Skybox() = default;

    /**
     * @brief Carga la panorámica, compila el shader y crea la geometría/estados del skybox.
     * @param device  Dispositivo Direct3D 11.
     * @param ddsPath Ruta del archivo .dds (panorámica equirectangular 2:1), relativa al
     *                directorio de trabajo del proceso (con extensión, ej. "Assets\\skybox.dds").
     * @return S_OK si todos los recursos se crearon correctamente.
     */
    HRESULT init(Device& device, const std::string& ddsPath);

    /**
     * @brief Dibuja el cubo del skybox al render target actualmente enlazado.
     * @param deviceContext Contexto Direct3D 11.
     * @param camera Cámara activa — se usa `GetViewNoTranslation()` (el skybox nunca se aleja
     *               de la cámara) combinada con su matriz de proyección.
     */
    void render(DeviceContext& deviceContext, const Camera& camera);

    /** @brief Libera todos los recursos GPU del skybox. */
    void destroy();

private:
    ShaderProgram     m_shader;
    Buffer            m_cbSkybox;   ///< b0: CBSkybox (ViewProj).
    Buffer            m_cubeVB;
    Buffer            m_cubeIB;
    unsigned int      m_cubeIndexCount = 0;

    Texture           m_panorama;        ///< SRV Texture2D equirectangular cargada desde el .dds.
    RasterizerState   m_rasterizer;      ///< CULL_NONE — cubo convexo cerrado, cámara siempre adentro.
    DepthStencilState m_depthStencil;    ///< Depth enable, write OFF, LESS_EQUAL.
    SamplerState      m_sampler;         ///< Bilineal/wrap por defecto.

    bool m_initialized = false;
};
