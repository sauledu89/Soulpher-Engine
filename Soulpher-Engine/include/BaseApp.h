/**
 * @file BaseApp.h
 * @brief Núcleo principal de la aplicación y gestor de la escena.
 *
 * @details
 * Inicializa y gestiona el núcleo de DirectX 11, crea la ventana y el dispositivo,
 * configura el swap chain, RTV/DSV y viewport; compila/carga shaders y buffers
 * de cámara; y prepara la escena inicial con un plano de referencia con textura
 * y actores FBX opcionales (Kirby). Las rutas de assets son relativas al ejecutable en bin/.
 */

#pragma once
#include "Prerequisites.h"
#include "Rendering/RenderTypes.h"

#include "Window.h"
#include "Device.h"
#include "DeviceContext.h"
#include "SwapChain.h"
#include "Texture.h"
#include "RenderTargetView.h"
#include "DepthStencilView.h"
#include "Viewport.h"
#include "ShaderProgram.h"
#include "Buffer.h"
#include "MeshComponent.h"
#include "ModelLoader.h"
#include "UserInterface.h"
#include "ECS/Actor.h"
#include "ECS/LightComponent.h"
#include "Rendering/ForwardRenderer.h"
#include "Rendering/DeferredRenderer.h"
#include "Rendering/GizmoRenderer.h"
#include "Rendering/LightGizmoRenderer.h"
#include "Rendering/Material.h"
#include "Rendering/MaterialInstance.h"
#include "Rendering/Mesh.h"
#include "Rendering/RenderScene.h"
#include "EngineUtilities/Utilities/Camera.h"
#include "EngineUtilities/Utilities/EditorViewportPass.h"
#include "EngineUtilities/Utilities/Skybox.h"
#include "RasterizerState.h"
#include "DepthStencilState.h"
#include "SamplerState.h"
#include <vector>

 /**
  * @class BaseApp
  * @brief Clase principal que gestiona la inicialización, ciclo de vida y renderizado de la aplicación.
  */
class BaseApp {
public:
    /** @brief Constructor por defecto. */
    BaseApp() = default;
    /** @brief Destructor por defecto. */
    ~BaseApp() = default;

    /** @brief Inicializa el motor y todos los recursos necesarios. */
    HRESULT init();
    /** @brief Actualiza la lógica de la escena en cada frame. */
    void update();
    /** @brief Renderiza el contenido de la escena. */
    void render();
    /** @brief Libera los recursos utilizados por el motor. */
    void destroy();

    /**
     * @brief Ejecuta el bucle principal de la aplicación (game loop).
     * @param hInstance Instancia de la aplicación.
     * @param hPrevInstance Instancia previa (no usada).
     * @param lpCmdLine Argumentos de línea de comandos.
     * @param nCmdShow Estado inicial de la ventana.
     * @param wndproc Procedimiento de ventana.
     * @return Código de salida de la aplicación.
     */
    int run(HINSTANCE hInstance,
        HINSTANCE hPrevInstance,
        LPWSTR lpCmdLine,
        int nCmdShow,
        WNDPROC wndproc);

private:
    /**
     * @brief Redimensiona swap chain, depth/stencil, viewport D3D11, aspect de camara,
     *        EditorViewportPass y G-Buffers del DeferredRenderer al nuevo tamaño del cliente.
     * @details Sin esto el backbuffer/G-Buffer quedan congelados al tamaño de init() y ImGui
     *          estira esa textura vieja al tamaño nuevo de "##DeferredViewport" — la camara
     *          sigue usando el aspect ratio viejo para proyectar, así que el rayo de picking
     *          (que asume el aspect actual) deja de coincidir con lo que se ve en pantalla.
     *          Llamado desde update() cuando detecta que el cliente de la ventana cambió de tamaño.
     */
    void onResize(unsigned int width, unsigned int height);

    // --- Core DX11 ---
    Window          m_window;            ///< Ventana principal.
    Device          m_device;            ///< Dispositivo DirectX 11.
    DeviceContext   m_deviceContext;     ///< Contexto de dispositivo.
    SwapChain       m_swapChain;         ///< Intercambio de buffers (front/back).

    // BackBuffer + RTV
    Texture           m_backBuffer;        ///< Textura de back buffer.
    RenderTargetView  m_renderTargetView;  ///< Vista de renderizado (RTV).

    // Depth/Stencil
    Texture           m_depthStencil;      ///< Textura de profundidad/stencil.
    DepthStencilView  m_depthStencilView;  ///< Vista de profundidad/stencil.

    // Viewport y Shaders
    Viewport       m_viewport;           ///< Viewport principal.
    ShaderProgram  m_shaderProgram;      ///< Programa de shaders activos.

    // CBuffer de cámara y luz (b0) — reemplaza los tres CBs anteriores
    Buffer    m_cbPerFrameBuffer;   ///< Constant buffer slot b0: View, Projection, luz.
    CBPerFrame m_cbPerFrame{};      ///< Datos del frame actual para el GPU.
    XMFLOAT3  m_camPos{};          ///< Posicion libre de la camara en mundo (free-fly, no derivada de un pivote).

    // Matrices de cámara
    XMMATRIX       m_View;               ///< Matriz de vista.
    XMMATRIX       m_Projection;         ///< Matriz de proyección.

    // Color de limpieza
    float          ClearColor[4] = { 0.0f, 0.125f, 0.3f, 1.0f }; ///< Color de fondo.

    // Luz simple
    XMFLOAT4       m_LightPos{ 2.0f, 4.0f, -2.0f, 1.0f }; ///< Posición de la luz principal.

    // Recursos
    ModelLoader    m_modelLoader;        ///< Cargador de modelos (FBX/OBJ).

    // Plano de referencia
    MeshComponent  planeMesh;            ///< Malla del plano.
    Texture        m_PlaneTexture;       ///< Textura del plano.
    EU::TSharedPointer<Actor> m_APlane;  ///< Actor que representa el plano.

    // Interfaz y actores
    UserInterface  m_userInterface;      ///< Interfaz de usuario (ImGui).
    std::vector<EU::TSharedPointer<Actor>> m_actors; ///< Lista de actores en la escena.

    // Forward renderer con shadow map (mantenido para referencia)
    ForwardRenderer m_forwardRenderer;    ///< Orquesta shadow pass + binding del shadow map.

    // ── Deferred Rendering ───────────────────────────────────────────────────
    DeferredRenderer   m_deferredRenderer;    ///< Pipeline deferred: shadow → G-buffer → lighting.
    Camera             m_camera;              ///< Cámara para el DeferredRenderer.
    EditorViewportPass m_editorViewportPass;  ///< Render target offscreen donde escribe el deferred.
    RenderScene        m_renderScene;         ///< Snapshot del frame actual (se reconstruye cada frame).
    Skybox             m_skybox;              ///< Cubemap de fondo — RenderScene.skybox apunta aquí (no-owning).
    bool               m_viewportHovered = false; ///< True si el mouse está sobre "##DeferredViewport" (frame anterior).
    XMFLOAT2           m_viewportPos  = XMFLOAT2(0.0f, 0.0f); ///< Esquina superior-izq. real (screen space) de la imagen del viewport (frame anterior).
    XMFLOAT2           m_viewportSize = XMFLOAT2(1.0f, 1.0f); ///< Tamaño real (px) de la imagen del viewport (frame anterior; GetContentRegionAvail()).

    // ── Iconos de Light Actors (flecha/esfera/cono wireframe) ──────────────────
    LightGizmoRenderer    m_lightGizmoRenderer; ///< Dibuja el icono de cada Light Actor de la escena.

    // ── Gizmo de transformación (T=Translate, E=Scale, R=Rotate) ───────────────
    GizmoRenderer         m_gizmoRenderer;                              ///< Geometría + draw calls del gizmo.
    GizmoRenderer::Mode   m_gizmoMode     = GizmoRenderer::Mode::Translate; ///< Modo activo (solo se dibuja/pickea con actor seleccionado).
    GizmoRenderer::Axis   m_gizmoHoverAxis = GizmoRenderer::Axis::None; ///< Eje bajo el cursor este frame.
    GizmoRenderer::Axis   m_gizmoDragAxis  = GizmoRenderer::Axis::None; ///< Eje siendo arrastrado (None = sin arrastre).
    float                 m_gizmoScreenScale = 1.0f;                    ///< Escala recalculada cada frame (tamaño constante en pantalla).
    // Estado capturado al iniciar un arrastre (drag):
    float                 m_gizmoDragStartParam = 0.0f;   ///< Traslación/Escala: posición inicial a lo largo del eje (closest-point-on-line).
    float                 m_gizmoDragStartAngleRad = 0.0f; ///< Rotación: ángulo inicial en el plano del eje.
    EU::Vector3           m_gizmoDragStartPosition{};     ///< Position del actor al iniciar el drag.
    EU::Vector3           m_gizmoDragStartScale{};        ///< Scale del actor al iniciar el drag.
    EU::Vector3           m_gizmoDragStartRotation{};     ///< Rotation del actor al iniciar el drag.

    // Render states compartidos por los materiales de los actores
    RasterizerState    m_defaultRasterizer;    ///< FILL_SOLID, CULL_BACK.
    DepthStencilState  m_defaultDepthStencil;  ///< Depth enable, LESS, write all.
    SamplerState       m_defaultSampler;       ///< Sampler bilineal por defecto.
    Material           m_defaultMaterial;      ///< Material opaco base (sin shader propio — el DeferredRenderer usa m_gBufferShader).
    Material           m_transparentMaterial;  ///< Material para vidrio/transparencias: MaterialDomain::Transparent + BlendMode::Alpha. Usa m_shaderProgram (Soulpher-Engine.fx) porque el pass transparente es forward, no deferred.
    Texture            m_defaultCheckerTexture; ///< Checkerboard magenta/negro procedural — fallback cuando falla la carga de una textura real.

    // Recursos GPU por actor (Mesh + textura separada para MaterialInstance)
    Mesh             m_kirbyMesh;                ///< Submallas GPU de Kirby.
    Texture          m_kirbyAlbedoTex;           ///< Textura de albedo de Kirby (para MaterialInstance).
    MaterialInstance m_kirbyMaterialInstance;    ///< Material instance de Kirby.

    Mesh             m_planeMeshGpu;             ///< Submallas GPU del plano de suelo.
    MaterialInstance m_planeMaterialInstance;    ///< Material instance del suelo.

    // Sci-Fi Toad: un submesh + MaterialInstance por parte (Body/Glass/Head), elegidos por
    // nombre del nodo FBX de origen (ver bloque "Sci-Fi Toad" en BaseApp::init()).
    Mesh                            m_sciFiToadMesh;                 ///< Submallas GPU de Sci-Fi Toad (Body/Glass/Head).
    std::vector<MaterialInstance*>  m_sciFiToadSubmeshMaterials;     ///< Un MaterialInstance* por submesh, indexado por Submesh::materialSlot.

    Texture          m_sciFiToadBodyAlbedoTex;      ///< Body: base color.
    Texture          m_sciFiToadBodyNormalTex;      ///< Body: normal map.
    Texture          m_sciFiToadBodyMetallicTex;    ///< Body: metallic map.
    Texture          m_sciFiToadBodyRoughnessTex;   ///< Body: roughness map.
    Texture          m_sciFiToadBodyAOTex;          ///< Body: ambient occlusion map.
    MaterialInstance m_sciFiToadBodyMaterialInstance;

    Texture          m_sciFiToadGlassAlbedoTex;     ///< Glass: base color.
    Texture          m_sciFiToadGlassRoughnessTex;  ///< Glass: roughness map.
    MaterialInstance m_sciFiToadGlassMaterialInstance;

    Texture          m_sciFiToadHeadAlbedoTex;      ///< Head/Eyes: base color.
    Texture          m_sciFiToadHeadNormalTex;      ///< Head/Eyes: normal map.
    Texture          m_sciFiToadHeadAOTex;          ///< Head/Eyes: ambient occlusion map.
    MaterialInstance m_sciFiToadHeadMaterialInstance; ///< Compartido por los submeshes Head_low y Eyes_low (mismo atlas de textura).

    // Parámetros de cámara free-fly (estilo Unreal/Roblox)
    float    m_camYawDeg = 0.0f;          ///< Ángulo de giro horizontal (orientación, no pivote).
    float    m_camPitchDeg = 15.0f;       ///< Ángulo de inclinación vertical, clamped ±89°.
    float    m_camDistance = 10.0f;       ///< Distancia de encuadre usada por Focus (F) y escala del dolly de zoom.

    // ── Stats de frame (panel de stats) ─────────────────────────────────────
    float        m_fps = 0.0f;            ///< FPS suavizado (EMA) para el panel de stats.
    float        m_frameTimeMs = 0.0f;    ///< Tiempo del último frame en milisegundos.
    unsigned int m_lastDrawCallCount = 0; ///< Draw calls del último frame completado.
};
