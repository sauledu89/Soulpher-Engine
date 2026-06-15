/**
 * @file ForwardRenderer.h
 * @brief Renderer de dos passes: shadow depth pass + color pass con PCF shadows.
 *
 * @details
 * ForwardRenderer implementa Forward Rendering con shadow maps dinamicos:
 *
 *  **Pass 1 — Shadow Depth Pass** (punto de vista de la luz):
 *   - Viewport fijo: 2048x2048 (m_shadowViewport).
 *   - Sin render target; solo DSV sobre m_shadowTex (R24G8_TYPELESS).
 *   - Shader: m_shadowDepthShader — solo VS (ShadowVS), PS nulo.
 *   - CBPerFrame (b0) con LightViewProjection debe estar ya en GPU.
 *   - Salida: textura de profundidad lista para leer en t6 del PS.
 *
 *  **Pass 2 — Color Pass** (punto de vista de la camara):
 *   - bindShadowMap() enlaza m_shadowSRV en slot t6 del Pixel Shader.
 *   - HLSL aplica filtrado PCF 3x3 para suavizar bordes de sombra.
 *   - LightViewProjection en CBPerFrame permite calcular coordenadas de shadow map.
 *
 * @note [GameDev] Forward Rendering es el pipeline mas simple y compatible:
 * cada objeto se dibuja con todos sus efectos en un solo pass. Es el estandar
 * para motores educativos, mobile (OpenGL ES) y juegos indie.
 * Deferred Rendering (Unreal, HDRP de Unity) dibuja primero a un G-Buffer
 * (normal, albedo, depth) y calcula iluminacion en un segundo pass. Escala mucho
 * mejor con cientos de luces, pero consume mas bandwidth de memoria y no funciona
 * bien con MSAA. Para shadow maps en deferred se usan Cascaded Shadow Maps (CSM),
 * que dividen el frustum en zonas con resoluciones distintas.
 *
 * @see BaseApp, RenderScene, CBPerFrame, ShaderProgram, Soulpher-Engine.fx
 */

#pragma once
#include "Prerequisites.h"
#include "Rendering/RenderTypes.h"
#include "Texture.h"
#include "DepthStencilView.h"
#include "Viewport.h"
#include "ShaderProgram.h"

class Device;
class DeviceContext;
class RenderTargetView;
class DepthStencilView;
class Actor;

/**
 * @class ForwardRenderer
 * @brief Orquesta el render de la escena en dos passes: shadow depth y color (opaco).
 *
 * Shadow pass: renderiza la geometria de todos los actores desde el punto de vista
 * de la luz direccional hacia una textura de profundidad de 2048x2048. El resultado
 * (shadow map) se enlaza en t6 durante el pass de color para calcular oclusión PCF.
 *
 * Uso en BaseApp:
 *  1. init() en BaseApp::init()
 *  2. computeLightViewProj() en BaseApp::update() → almacenar en CBPerFrame.
 *  3. renderShadowPass() al inicio de BaseApp::render(), antes de configurar el RTV principal.
 *  4. bindShadowMap() justo antes del loop de actores en el pass de color.
 *  5. destroy() en BaseApp::destroy().
 */
class ForwardRenderer {
public:
    ForwardRenderer()  = default;
    ~ForwardRenderer() = default;

    /**
     * @brief Crea todos los recursos del shadow map.
     * @param device Dispositivo Direct3D 11.
     * @param shadowMapSize Resolución del shadow map (ancho == alto). Por defecto 2048.
     */
    HRESULT init(Device& device, unsigned int shadowMapSize = 2048);

    /**
     * @brief Ejecuta el shadow depth pass.
     *
     * Limpia el shadow DSV, enlaza el shadow viewport, el shadow VS (sin PS),
     * y llama a Actor::renderDepth() de cada actor con castShadow=true.
     * CBPerFrame (b0) debe estar ya cargado en GPU antes de llamar a este método.
     */
    void renderShadowPass(DeviceContext& deviceContext,
                          const std::vector<EU::TSharedPointer<Actor>>& actors);

    /**
     * @brief Enlaza el shadow map como SRV en el slot t6 del Pixel Shader.
     * Llamar antes del draw loop del pass de color.
     */
    void bindShadowMap(DeviceContext& deviceContext);

    /**
     * @brief Desvincula el shadow SRV del PS (necesario antes de escribir en él de nuevo).
     */
    void unbindShadowMap(DeviceContext& deviceContext);

    /**
     * @brief Calcula la matriz LightViewProjection para la luz direccional dada.
     * @param lightDir Dirección de la luz (normalizada, apunta hacia la escena).
     * @param sceneCenter Centro de la escena en mundo.
     * @param sceneRadius Radio que cubre toda la escena (influye en el frustum ortográfico).
     */
    XMMATRIX computeLightViewProj(const EU::Vector3& lightDir,
                                   const XMFLOAT3&   sceneCenter,
                                   float              sceneRadius) const;

    /** @brief Libera todos los recursos del renderer. */
    void destroy();

private:
    // Shadow map depth texture (R24G8_TYPELESS, permite DSV + SRV simultáneo)
    Texture           m_shadowTex;
    // SRV alias sobre m_shadowTex (R24_UNORM_X8_TYPELESS) — lectura en PS
    Texture           m_shadowSRV;
    // Vista de profundidad para escritura durante el shadow pass
    DepthStencilView  m_shadowDSV;
    // Viewport fijo para el shadow pass
    Viewport          m_shadowViewport;
    // Shader de profundidad (solo VS: ShadowVS, InputLayout con POSITION)
    ShaderProgram     m_shadowDepthShader;

    unsigned int m_shadowMapSize = 2048;
};
