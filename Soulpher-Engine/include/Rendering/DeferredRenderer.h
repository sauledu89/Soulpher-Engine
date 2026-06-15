/**
 * @file DeferredRenderer.h
 * @brief Renderer deferred de 4-MRT con shadow map y pass de iluminación en fullscreen quad.
 *
 * @details
 * Implementa el pipeline **Deferred Rendering** en cuatro fases:
 *
 *  **1. Shadow Pass** (punto de vista de la luz):
 *   - Render a depth-only target (2048×2048).
 *   - Guarda el shadow map para el pass de iluminación.
 *
 *  **2. Geometry Pass (G-Buffer)**:
 *   - 4 Render Targets simultáneos (MRT):
 *     - RT0: Albedo + Metallic  (R8G8B8A8_UNORM).
 *     - RT1: Normal + Roughness (R16G16B16A16_FLOAT).
 *     - RT2: WorldPos + AO      (R32G32B32A32_FLOAT).
 *     - RT3: Emissive + Alpha   (R16G16B16A16_FLOAT).
 *   - El G-Buffer acumula toda la información de superficie visible en pantalla.
 *
 *  **3. Lighting Pass** (fullscreen quad):
 *   - Lee los 4 G-Buffers como SRVs + el shadow map.
 *   - Calcula iluminación para TODOS los píxeles en un solo draw call.
 *   - Admite hasta `kMaxSceneLights` fuentes de luz dinámicas.
 *
 *  **4. Transparent Forward Pass**:
 *   - Objetos transparentes se renderizan en forward sobre el resultado deferred.
 *
 * @note [GameDev] La principal ventaja del Deferred Rendering sobre Forward es el
 * complejidad de iluminación: en Forward cada objeto paga O(N_lights) en el draw call;
 * en Deferred el lighting pass paga O(N_pixels × N_lights) UNA VEZ en pantalla.
 * Para escenas con pocas luces, Forward es más eficiente (menos memoria). Para escenas
 * con decenas de luces dinámicas, Deferred escala mucho mejor.
 * La desventaja principal es el bandwidth: los 4 G-Buffers consumen ~64 bytes/píxel en
 * 1080p → 132 MB que deben escribirse y leerse cada frame. El otro problema es MSAA:
 * el G-buffer no puede ser multisampleado fácilmente (requiere MSAA deferred especial).
 *
 * @note [GameDev] El layout multi-luz usa `XMFLOAT4` para empacar los datos de cada
 * luz: `LightPositionsRanges[i] = float4(position.xyz, range)`. Esto minimiza el
 * tamaño del constant buffer (3 float4 por luz vs 9 floats separados) y alinea
 * naturalmente a 16 bytes como requiere HLSL.
 *
 * @see ISceneRenderer, RenderTypes.h, ForwardRenderer
 * @ingroup rendering
 */
#pragma once
#include "Buffer.h"
#include "DepthStencilState.h"
#include "DepthStencilView.h"
#include "RasterizerState.h"
#include "RenderTargetView.h"
#include "Rendering/ISceneRenderer.h"
#include "Rendering/RenderScene.h"
#include "Rendering/RenderTypes.h"
#include "SamplerState.h"
#include "ShaderProgram.h"
#include "Texture.h"
#include "EngineUtilities/Utilities/EditorViewportPass.h"

class Device;
class DeviceContext;
class Camera;
class Material;

/**
 * @class DeferredRenderer
 * @brief Renderer deferred: shadow pass → G-buffer → lighting quad → transparent forward.
 *
 * Hereda de `ISceneRenderer` e implementa el pipeline completo de Deferred Rendering
 * con soporte para shadow maps PCF, múltiples luces y un debug pass de G-buffer.
 */
class
DeferredRenderer : public ISceneRenderer {
public:
    /**
     * @brief Crea todos los recursos GPU: G-Buffer textures, shadow DSV, shaders, samplers.
     * @param device Dispositivo Direct3D 11.
     * @return S_OK si todos los recursos se crearon correctamente.
     */
    HRESULT
    init(Device& device) override;

    /**
     * @brief Redimensiona los G-Buffers cuando cambia el tamaño del viewport.
     * @param device Dispositivo D3D11.
     * @param width  Nuevo ancho en píxeles.
     * @param height Nuevo alto en píxeles.
     */
    void
    resize(Device& device, unsigned int width, unsigned int height) override;

    /**
     * @brief Ejecuta el frame completo: shadow → geometry → lighting → transparents.
     * @param deviceContext Contexto D3D11 para los draw calls.
     * @param camera        Cámara activa del frame.
     * @param scene         Escena con todos los objetos renderizables.
     * @param viewportPass  Target final donde se escribe el resultado.
     */
    void
    render(DeviceContext& deviceContext,
           const Camera& camera,
           RenderScene& scene,
           EditorViewportPass& viewportPass) override;

    /** @brief Libera todos los recursos GPU del renderer. */
    void
    destroy() override;

    /** @brief SRV del shadow map (slot t6 en el lighting pass). */
    ID3D11ShaderResourceView*
    getShadowMapSRV() const override { return m_shadowDepthSRV.m_textureFromImg; }

    /** @brief SRV del debug pre-shadow (para ImGui preview del shadow map). */
    ID3D11ShaderResourceView*
    getPreShadowSRV() const override { return m_preShadowDebugPass.getSRV(); }

    /** @brief SRV del G-Buffer RT0: Albedo (RGB) + Metallic (A). */
    ID3D11ShaderResourceView*
    getGBufferAlbedoMetallicSRV() const override { return m_gBufferAlbedoMetallicSRV.m_textureFromImg; }

    /** @brief SRV del G-Buffer RT1: Normal (RGB) + Roughness (A). */
    ID3D11ShaderResourceView*
    getGBufferNormalRoughnessSRV() const override { return m_gBufferNormalRoughnessSRV.m_textureFromImg; }

    /** @brief SRV del G-Buffer RT2: WorldPosition (RGB) + AO (A). */
    ID3D11ShaderResourceView*
    getGBufferWorldAoSRV() const override { return m_gBufferWorldAoSRV.m_textureFromImg; }

    /** @brief SRV del G-Buffer RT3: Emissive (RGB) + Alpha (A). */
    ID3D11ShaderResourceView*
    getGBufferEmissiveAlphaSRV() const override { return m_gBufferEmissiveAlphaSRV.m_textureFromImg; }

    /**
     * @brief Activa/desactiva el factor de sombra en el lighting pass (debug).
     * @param enabled `false` = ilumina todo sin sombras (útil para verificar geometría).
     */
    void
    setShadowFactorDebugEnabled(bool enabled) override { m_shadowFactorDebugEnabled = enabled; }

    /**
     * @brief Selecciona qué G-Buffer visualizar en la UI de debug.
     * @param mode 0=resultado final, 1=albedo, 2=normal, 3=worldpos, 4=roughness, etc.
     */
    void
    setDeferredDebugViewMode(int mode) override { m_deferredDebugViewMode = mode; }

    /** @brief Nombre del renderer para logs y UI de debug. */
    const char*
    getDebugName() const override { return "DeferredRenderer"; }

private:
    // ── Organización de frame ───────────────────────────────────────────────

    /** @brief Clasifica los objetos de la escena en colas opaco/transparente. */
    void buildQueues(RenderScene& scene, const Camera& camera);

    /** @brief Actualiza CBPerFrame (matrices, luz, datos multi-luz) y lo sube a GPU. */
    void updatePerFrame(const Camera& camera, const RenderScene& scene, DeviceContext& deviceContext);

    /** @brief Calcula LightViewProjection para la luz principal (shadow map matrix). */
    void updateLightMatrices(const Camera& camera, const RenderScene& scene);

    // ── Binding de targets ──────────────────────────────────────────────────

    /** @brief Enlaza los 4 G-Buffer RTVs como targets de render activos. */
    void bindGBufferTargets(DeviceContext& deviceContext, ID3D11DepthStencilView* depthStencilView);

    /** @brief Restaura el target final (back buffer o EditorViewportPass) como activo. */
    void bindFinalTarget(DeviceContext& deviceContext, ID3D11RenderTargetView* renderTargetView, ID3D11DepthStencilView* depthStencilView);

    /** @brief Desvincula los G-Buffer SRVs del PS para poder usarlos como RTVs de nuevo. */
    void clearDeferredSRVs(DeviceContext& deviceContext);

    // ── Passes de render ────────────────────────────────────────────────────

    /** @brief Dibuja todos los opacos al G-Buffer (RT0–RT3). */
    void renderGeometryPass(DeviceContext& deviceContext);

    /** @brief Dibuja un objeto individual al G-Buffer: bind material → draw. */
    void renderGeometryObject(DeviceContext& deviceContext, const RenderObject& object);

    /** @brief Fullscreen quad: lee G-Buffer + shadow map y calcula iluminación. */
    void renderLightingPass(DeviceContext& deviceContext);

    /** @brief Dibuja el skybox en la profundidad máxima (Z=1) para quedar al fondo. */
    void renderSkyboxPass(DeviceContext& deviceContext, RenderScene& scene);

    /** @brief Dibuja objetos transparentes en forward sobre el resultado deferred. */
    void renderTransparentPass(DeviceContext& deviceContext);

    /** @brief Dibuja un objeto en forward con el pass indicado (Opaque/Transparent). */
    void renderForwardObject(DeviceContext& deviceContext, const RenderObject& object, RenderPassType passType);

    /** @brief Shadow depth pass: dibuja la escena desde el punto de vista de la luz. */
    void renderShadowPass(DeviceContext& deviceContext);

    /** @brief Dibuja un objeto individual al shadow depth target. */
    void renderShadowObject(DeviceContext& deviceContext, const RenderObject& object);

    /** @brief Dibuja la escena completa a un target arbitrario (para debug pre-shadow). */
    void renderSceneToTarget(DeviceContext& deviceContext, RenderScene& scene, EditorViewportPass& targetPass, bool applyShadows);

    // ── Creación de recursos ────────────────────────────────────────────────

    /** @brief Crea la textura, DSV y shader del shadow map. */
    HRESULT createShadowResources(Device& device);

    /** @brief Crea las 4 texturas y RTVs del G-Buffer. */
    HRESULT createGBufferResources(Device& device, unsigned int width, unsigned int height);

    /**
     * @brief Crea un G-Buffer target individual (textura + SRV alias + RTV).
     * @param format Formato DXGI del target (R8G8B8A8, R16G16B16A16, R32G32B32A32).
     */
    HRESULT createGBufferTarget(Device& device,
        unsigned int width,
        unsigned int height,
        DXGI_FORMAT format,
        Texture& texture,
        Texture& srv,
        RenderTargetView& rtv);

    /** @brief Crea el shader de lighting y el sampler para leer G-Buffers. */
    HRESULT createLightingResources(Device& device);

    /** @brief Crea el vertex/index buffer del fullscreen quad (2 triángulos = pantalla completa). */
    HRESULT createFullScreenQuad(Device& device);

    /** @brief Crea los 4 blend states: opaque, alpha, additive, premultiplied. */
    HRESULT createBlendStates(Device& device);

    /** @brief Selecciona el blend state apropiado según el Material del objeto. */
    ID3D11BlendState* resolveBlendState(const Material* material) const;

private:
    // ── Constant Buffers GPU ────────────────────────────────────────────────
    Buffer m_perFrameBuffer;          ///< b0: CBPerFrame — matrices + luces.
    Buffer m_perObjectBuffer;         ///< b1: CBPerObject — World matrix por draw call.
    Buffer m_perMaterialBuffer;       ///< b2: CBPerMaterial — parámetros PBR.
    Buffer m_lightingDebugBuffer;     ///< Constante del lighting pass (modo debug).
    Buffer m_fullscreenVertexBuffer;  ///< VB del fullscreen quad (2 triángulos).
    Buffer m_fullscreenIndexBuffer;   ///< IB del fullscreen quad (6 índices).

    // ── Depth Stencil States ────────────────────────────────────────────────
    DepthStencilState m_transparentDepthStencil; ///< LESS_EQUAL + write OFF (transparentes).
    DepthStencilState m_disabledDepthStencil;    ///< depth OFF (fullscreen lighting quad).
    DepthStencilState m_shadowDepthStencil;      ///< LESS + write ON (shadow pass).

    // ── Blend States (raw COM, 4 variantes) ────────────────────────────────
    ID3D11BlendState* m_alphaBlendState         = nullptr; ///< Alfa estándar.
    ID3D11BlendState* m_opaqueBlendState        = nullptr; ///< Sin mezcla.
    ID3D11BlendState* m_additiveBlendState      = nullptr; ///< Suma de colores (partículas, fuego).
    ID3D11BlendState* m_premultipliedBlendState = nullptr; ///< Alpha premultiplicado.
    float m_blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

    // ── Shadow Map ──────────────────────────────────────────────────────────
    Texture          m_shadowDepthTexture; ///< Textura R24G8_TYPELESS del shadow map.
    Texture          m_shadowDepthSRV;     ///< Alias SRV R24_UNORM_X8_TYPELESS para leer en PS.
    DepthStencilView m_shadowDSV;          ///< DSV para escribir durante el shadow pass.
    ShaderProgram    m_shadowShader;       ///< Solo VS (sin PS) para el shadow depth pass.
    RasterizerState  m_shadowRasterizer;   ///< CULL_FRONT para reducir shadow acne.
    unsigned int     m_shadowMapSize = 2048; ///< Resolución del shadow map (cuadrado).

    // ── G-Buffer Shaders ────────────────────────────────────────────────────
    ShaderProgram   m_gBufferShader;           ///< Shader del geometry pass (escribe los 4 MRTs).
    ShaderProgram   m_deferredLightingShader;  ///< Shader del lighting pass (fullscreen quad).
    SamplerState    m_lightingSampler;         ///< Sampler para leer G-Buffers en el lighting pass.
    RasterizerState m_fullscreenRasterizer;    ///< CULL_NONE para el fullscreen quad.

    // ── G-Buffer RT0: Albedo (RGB) + Metallic (A) ──────────────────────────
    Texture          m_gBufferAlbedoMetallicTexture; ///< R8G8B8A8_UNORM.
    Texture          m_gBufferAlbedoMetallicSRV;
    RenderTargetView m_gBufferAlbedoMetallicRTV;

    // ── G-Buffer RT1: Normal (RGB) + Roughness (A) ─────────────────────────
    Texture          m_gBufferNormalRoughnessTexture; ///< R16G16B16A16_FLOAT.
    Texture          m_gBufferNormalRoughnessSRV;
    RenderTargetView m_gBufferNormalRoughnessRTV;

    // ── G-Buffer RT2: WorldPosition (RGB) + AO (A) ─────────────────────────
    Texture          m_gBufferWorldAoTexture; ///< R32G32B32A32_FLOAT.
    Texture          m_gBufferWorldAoSRV;
    RenderTargetView m_gBufferWorldAoRTV;

    // ── G-Buffer RT3: Emissive (RGB) + Alpha (A) ───────────────────────────
    Texture          m_gBufferEmissiveAlphaTexture; ///< R16G16B16A16_FLOAT.
    Texture          m_gBufferEmissiveAlphaSRV;
    RenderTargetView m_gBufferEmissiveAlphaRTV;

    // ── Debug ───────────────────────────────────────────────────────────────
    EditorViewportPass m_preShadowDebugPass; ///< Target de debug para visualizar shadow map en ImGui.
    bool         m_applyShadows  = true;  ///< Si false, ilumina sin sombras (debug).
    unsigned int m_renderWidth   = 1280;
    unsigned int m_renderHeight  = 720;

    // ── CPU-side Constant Buffer mirrors ───────────────────────────────────
    CBPerFrame    m_cbPerFrame{};
    CBPerObject   m_cbPerObject{};
    CBPerMaterial m_cbPerMaterial{};

    /**
     * @struct DeferredLightingDebugData
     * @brief Parámetros de debug del lighting pass, subidos como constant buffer.
     */
    struct DeferredLightingDebugData {
        int   DebugViewMode  = 0;   ///< Qué G-Buffer visualizar (0 = resultado final).
        float ShadowStrength = 1.0f; ///< Factor de sombra (1=normal, 0=sin sombra).
        float pad0           = 0.0f;
        float pad1           = 0.0f;
    } m_lightingDebugData{};

    bool m_shadowFactorDebugEnabled = false;
    int  m_deferredDebugViewMode    = 0;

    // ── Queues de render ───────────────────────────────────────────────────
    std::vector<const RenderObject*> m_opaqueQueue;      ///< Objetos sólidos (ordenados por material).
    std::vector<const RenderObject*> m_transparentQueue; ///< Objetos transparentes (back-to-front).
};
