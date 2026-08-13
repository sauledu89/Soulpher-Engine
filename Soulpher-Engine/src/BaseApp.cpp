/**
 * @file BaseApp.cpp
 * @brief Bucle principal, inicialización de DX11 y orquestación del render.
 *
 * @details
 * Implementa la lógica de arranque del motor (swap chain, device/context, RTV/DSV, viewport),
 * la carga de shaders y constant buffers de cámara, la preparación de la escena (personaje FBX
 * y plano de referencia), el bucle de actualización por cuadro (incluida la cámara orbital
 * controlada con el ratón), y el pase de render (limpieza, draw de actores, UI y Present).
 *
 * Orden de inicialización crítica:
 *  1) SwapChain (crea ID3D11Device + ID3D11DeviceContext) y back buffer.
 *  2) RenderTargetView (RTV) del back buffer.
 *  3) DepthStencil (textura + vista) con sample count = 1 (igual al swap chain).
 *  4) Viewport.
 *  5) Shaders + InputLayout.
 *  6) Constant buffers de cámara.
 *  7) Escena (actores y texturas).
 *  8) Inicialización de ImGui.
 *
 * @note [GameDev] El bucle init/update/render/destroy es el patron universal de motores:
 *  - init():    carga recursos, crea el mundo (equivale a BeginPlay en Unreal).
 *  - update():  logica del juego + actualizacion de transforms (equivale a Tick).
 *  - render():  envio de draw calls a la GPU (equivale al render thread en Unreal).
 *  - destroy(): libera recursos al cerrar (equivale a EndPlay + destruccion de actores).
 * La camara orbital con raton es un controlador de debug, no un actor ECS real.
 * En produccion se implementaria como un CameraComponent con una CameraManager.
 */

#include "BaseApp.h"
#include "ECS/Transform.h"
#include "EngineUtilities/Vectors/Vector3.h"
#include "imgui.h"
#include <cstdio>
#include <cfloat>
#include <cmath>
#include <algorithm>
#include <cctype>

 // Color de limpieza por defecto (RGBA)
static const float kClear[4] = { 0.0f, 0.125f, 0.30f, 1.0f };

/**
 * @brief Callback de ImDrawList que desactiva el blending para el siguiente draw call.
 * @details Los paneles de debug de G-Buffer muestran texturas MRT crudas cuyo canal alpha
 * no es transparencia real (guarda Metallic/Roughness/AO/Alpha-de-material según el RT) — el
 * blend state normal de ImGui usa ese alpha para mezclar con el fondo de la ventana, por lo que
 * canales con alpha bajo (ej. Metallic=0 en Albedo) se ven "vacíos". OMSetBlendState(nullptr,...)
 * aplica el blend state DEFAULT de D3D11 (BlendEnable=FALSE, opaco), ignorando ese alpha.
 * Debe emparejarse con ImDrawCallback_ResetRenderState después del Image() para que el resto
 * de la UI de ImGui (que sí necesita blending real) se dibuje con su estado normal.
 */
static void ForceOpaqueBlendCallback(const ImDrawList*, const ImDrawCmd* cmd) {
    ID3D11DeviceContext* ctx = static_cast<ID3D11DeviceContext*>(cmd->UserCallbackData);
    if (!ctx) return;
    const float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    ctx->OMSetBlendState(nullptr, blendFactor, 0xFFFFFFFF);
}

// Layout del editor: paneles laterales de ancho FIJO (izquierdo: Hierarchy+Inspector; derecho:
// G-Buffers); el viewport 3D ocupa dinámicamente todo el ancho restante entre ambos, y su alto
// llega hasta donde empieza la consola (debajo).
static constexpr float kLeftPanelWidth     = 220.0f;
static constexpr float kRightPanelWidth    = 180.0f;
static constexpr float kPanelGap           = 4.0f;
static constexpr float kViewportHeightFrac = 0.72f;

/**
 * @brief Inicializa todos los subsistemas gráficos y la escena.
 *
 * Configura:
 *  - Swap chain, dispositivo y contexto de DirectX 11 (sin MSAA para evitar mismatches).
 *  - BackBuffer y su RenderTargetView.
 *  - DepthStencil (textura + vista) con sample count = 1 (igual al swap chain).
 *  - Viewport.
 *  - Shaders (.fx) e InputLayout (POSITION, TEXCOORD).
 *  - Constant buffers de cámara (b0 y b1).
 *  - Escena: actores FBX opcionales (Kirby) + plano de referencia con textura.
 *  - Inicialización de ImGui.
 *
 * @return HRESULT Código de resultado:
 *  - @c S_OK en caso de éxito.
 *  - Código de error DirectX/Win32 en caso de fallo.
 */
HRESULT BaseApp::init()
{
    HRESULT hr = S_OK;

    LOG_MESSAGE("BaseApp", "init", "started");

    // 1) SwapChain + Device + Context + BackBuffer (sin MSAA para evitar mismatches)
    hr = m_swapChain.init(m_device, m_deviceContext, m_backBuffer, m_window);
    if (FAILED(hr)) {
        LOG_ERROR("Main", "InitDevice", ("Failed to initialize SwapChain. hr=" + std::to_string(hr)).c_str());
        return hr;
    }
    LOG_MESSAGE("BaseApp", "init", "OK SwapChain + Device");

    // 2) RenderTargetView sobre el backbuffer
    hr = m_renderTargetView.init(m_device, m_backBuffer, DXGI_FORMAT_R8G8B8A8_UNORM);
    if (FAILED(hr)) {
        LOG_ERROR("Main", "InitDevice", ("Failed to initialize RenderTargetView. hr=" + std::to_string(hr)).c_str());
        return hr;
    }

    // 3) DepthStencil (textura + view) con sampleCount=1 (igual al swapchain)
    hr = m_depthStencil.init(
        m_device,
        m_window.m_width,
        m_window.m_height,
        DXGI_FORMAT_D24_UNORM_S8_UINT,
        D3D11_BIND_DEPTH_STENCIL,
        1,      // <- IMPORTANTE: igual que swap chain
        0
    );
    if (FAILED(hr)) {
        LOG_ERROR("Main", "InitDevice", ("Failed to initialize DepthStencil texture. hr=" + std::to_string(hr)).c_str());
        return hr;
    }

    hr = m_depthStencilView.init(m_device, m_depthStencil, DXGI_FORMAT_D24_UNORM_S8_UINT);
    if (FAILED(hr)) {
        LOG_ERROR("Main", "InitDevice", ("Failed to initialize DepthStencilView. hr=" + std::to_string(hr)).c_str());
        return hr;
    }

    // 4) Viewport
    hr = m_viewport.init(m_window);
    if (FAILED(hr)) {
        LOG_ERROR("Main", "InitDevice", ("Failed to initialize Viewport. hr=" + std::to_string(hr)).c_str());
        return hr;
    }

    // 5) InputLayout: debe coincidir exactamente con SimpleVertex { Pos, Normal, Tangent, Bitangent, Tex }
    std::vector<D3D11_INPUT_ELEMENT_DESC> layout;
    {
        auto addElem = [&](const char* sem, DXGI_FORMAT fmt) {
            D3D11_INPUT_ELEMENT_DESC e{};
            e.SemanticName         = sem;
            e.SemanticIndex        = 0;
            e.Format               = fmt;
            e.InputSlot            = 0;
            e.AlignedByteOffset    = D3D11_APPEND_ALIGNED_ELEMENT;
            e.InputSlotClass       = D3D11_INPUT_PER_VERTEX_DATA;
            e.InstanceDataStepRate = 0;
            layout.push_back(e);
        };
        addElem("POSITION",  DXGI_FORMAT_R32G32B32_FLOAT);
        addElem("NORMAL",    DXGI_FORMAT_R32G32B32_FLOAT);
        addElem("TANGENT",   DXGI_FORMAT_R32G32B32_FLOAT);
        addElem("BITANGENT", DXGI_FORMAT_R32G32B32_FLOAT);
        addElem("TEXCOORD",  DXGI_FORMAT_R32G32_FLOAT);
    }

    // 6) Shaders (.fx)
    hr = m_shaderProgram.init(m_device, "Soulpher-Engine.fx", layout);
    if (FAILED(hr)) {
        LOG_ERROR("Main", "InitDevice", ("Failed to initialize ShaderProgram. hr=" + std::to_string(hr)).c_str());
        return hr;
    }
    LOG_MESSAGE("BaseApp", "init", "OK Soulpher-Engine.fx compiled");

    // 7) Constant Buffer b0: CBPerFrame (View + Projection + luz)
    hr = m_cbPerFrameBuffer.init(m_device, sizeof(CBPerFrame));
    if (FAILED(hr)) {
        LOG_ERROR("Main", "InitDevice", ("Failed to create CBPerFrame buffer. hr=" + std::to_string(hr)).c_str());
        return hr;
    }

    // 8) Matrices iniciales de camara — update() las recalcula cada frame.
    {
        m_Projection = XMMatrixPerspectiveFovLH(
            XM_PIDIV4,
            m_window.m_width / (FLOAT)m_window.m_height,
            0.01f, 100.0f
        );
        // Vista inicial: reproduce el punto de partida de la vieja camara orbital
        // (yaw=0, pitch=m_camPitchDeg, pivote (0,-5,0), distancia=m_camDistance) ya
        // en formato free-fly. update() la ajusta cada frame a partir de aqui.
        float pitch0 = XMConvertToRadians(m_camPitchDeg);
        XMVECTOR Eye = XMVectorSet(0.0f, -5.0f + sinf(pitch0) * m_camDistance, cosf(pitch0) * m_camDistance, 0.0f);
        XMVECTOR At  = XMVectorSet(0.0f, -5.0f, 0.0f, 0.0f);
        XMVECTOR Up  = XMVectorSet(0.0f,  1.0f, 0.0f, 0.0f);
        m_View = XMMatrixLookAtLH(Eye, At, Up);
        XMStoreFloat3(&m_camPos, Eye);
    }

    // 9) Valores iniciales de luz (la UI los modifica en tiempo real desde aquí)
    {
        // Luz diagonal normalizada: (0.3, -1, 0.5) / ||(0.3,-1,0.5)||
        float lx = 0.30f, ly = -1.0f, lz = 0.50f;
        float len = sqrtf(lx*lx + ly*ly + lz*lz);
        m_cbPerFrame.LightDir      = EU::Vector3(lx/len, ly/len, lz/len);
        m_cbPerFrame.LightColor    = EU::Vector3(1.0f, 1.0f, 1.0f);
        m_cbPerFrame.LightRange    = 10.0f;
        m_cbPerFrame.LightPosition = EU::Vector3(0.0f, 3.0f, 0.0f);
        m_cbPerFrame.LightType     = 0; // Directional
    }

    // 9.5) Textura de fallback: checkerboard magenta/negro generado en memoria (sin
    // archivo). Se usa como albedo cuando falla la carga de la textura real de un modelo
    // (ver Kirby y Plano más abajo).
    if (FAILED(m_defaultCheckerTexture.initCheckerboard(m_device))) {
        LOG_ERROR("Main", "InitDevice", "Failed to create default checkerboard texture.");
    }

    // 10) Actor: Kirby (FBX) — fallo no fatal, continúa sin el modelo
    {
        m_modelLoader.meshes.clear();

        auto kirby = EU::MakeShared<Actor>(m_device);
        if (kirby.isNull()) {
            LOG_WARNING("Main", "InitDevice", "Failed to create Kirby Actor — skipping.");
        }
        else {
            const std::string kFBX = "ModelsFBX\\kirby\\KirbyTest.fbx";

            if (!m_modelLoader.LoadFBXModel(kFBX) || m_modelLoader.meshes.empty()) {
                LOG_WARNING("Main", "InitDevice", ("FBX not found or empty: " + kFBX + " — skipping actor.").c_str());
            }
            else {
                kirby->setMesh(m_device, m_modelLoader.meshes);
                m_kirbyMesh = Mesh::buildFrom(m_device, m_modelLoader.meshes);
                LOG_MESSAGE("BaseApp", "init", "Kirby FBX loaded, submeshes=" + std::to_string((int)m_kirbyMesh.getSubmeshes().size()));

                kirby->getComponent<Transform>()->setTransform(
                    EU::Vector3( 3.00f, -5.00f,  0.00f), // Position
                    EU::Vector3( 0.00f,  0.00f,  0.00f), // Rotation (grados)
                    EU::Vector3( 1.00f,  1.00f,  1.00f)  // Scale
                );
                kirby->setName("Kirby");
                kirby->setCastShadow(true);

                m_actors.push_back(kirby);
            }
        }
    }

    // 10b) Actor: Sci-Fi Toad (FBX) — fallo no fatal, continúa sin el modelo
    {
        m_modelLoader.meshes.clear();

        auto sciFiToad = EU::MakeShared<Actor>(m_device);
        if (sciFiToad.isNull()) {
            LOG_WARNING("Main", "InitDevice", "Failed to create Sci-Fi Toad Actor — skipping.");
        }
        else {
            const std::string kToadFBX = "ModelsFBX\\sci-fi-toad\\Bake_Sci-fiToad.fbx";

            if (!m_modelLoader.LoadFBXModel(kToadFBX) || m_modelLoader.meshes.empty()) {
                LOG_WARNING("Main", "InitDevice", ("FBX not found or empty: " + kToadFBX + " — skipping actor.").c_str());
            }
            else {
                sciFiToad->setMesh(m_device, m_modelLoader.meshes);
                m_sciFiToadMesh = Mesh::buildFrom(m_device, m_modelLoader.meshes);
                LOG_MESSAGE("BaseApp", "init", "Sci-Fi Toad FBX loaded, submeshes=" + std::to_string((int)m_sciFiToadMesh.getSubmeshes().size()));

                // Un MaterialInstance por submesh, elegido por el nombre del nodo FBX de origen
                // (Body/Glass/Head). materialSlot == indice en m_modelLoader.meshes (ver
                // Mesh::buildFrom), asi que se puede recuperar el nombre original por indice.
                // Los MaterialInstance en si se configuran mas abajo (texturas del modelo).
                // Bucket (0=Body/1=Glass/2=Head), no MaterialInstance* directo: asi el Material
                // Editor puede reasignar que MaterialInstance respalda cada bucket en tiempo real
                // (ver m_sciFiToadBodySlotMaterial/... y su uso en render()) sin tener que volver
                // a analizar los nombres de nodo FBX.
                m_sciFiToadSubmeshMaterials.assign(m_sciFiToadMesh.getSubmeshes().size(), nullptr);
                m_sciFiToadSubmeshBucket.assign(m_sciFiToadMesh.getSubmeshes().size(), 0);
                for (size_t i = 0; i < m_sciFiToadMesh.getSubmeshes().size(); ++i) {
                    unsigned int slot = m_sciFiToadMesh.getSubmeshes()[i].materialSlot;
                    std::string nodeName = (slot < m_modelLoader.meshes.size()) ? m_modelLoader.meshes[slot].m_name : std::string();
                    std::string lowerName = nodeName;
                    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);

                    int bucket = 0; // fallback: Body
                    if (lowerName.find("glass") != std::string::npos) {
                        bucket = 1; // incluye "GlassHead" (visor transparente)
                    } else if (lowerName.find("eye") != std::string::npos) {
                        bucket = 2; // Eyes_low comparte atlas con Head (BC/N/AO)
                    } else if (lowerName.find("head") != std::string::npos) {
                        bucket = 2;
                    }
                    m_sciFiToadSubmeshBucket[i] = bucket;
                    LOG_MESSAGE("BaseApp", "init", "Sci-Fi Toad submesh[" + std::to_string(i) + "] node='" + nodeName + "'");
                }

                sciFiToad->getComponent<Transform>()->setTransform(
                    EU::Vector3( 3.00f,  0.00f,  0.00f), // Position — a la derecha de Kirby
                    EU::Vector3( 0.00f,  0.00f,  0.00f), // Rotation (grados)
                    EU::Vector3( 1.00f,  1.00f,  1.00f)  // Scale
                );
                sciFiToad->setName("SciFiToad");
                sciFiToad->setCastShadow(true);

                m_actors.push_back(sciFiToad);
            }
        }
    }

    // 11) ACTOR: Plano simple (suelo con piedra.jpg)
    {
        const float kSize = 20.0f; // mitad del tamaño (=> 40x40)
        const float kTiling = 6.0f;  // repetición UV

        m_APlane = EU::MakeShared<Actor>(m_device);
        if (m_APlane.isNull()) {
            LOG_ERROR("Main", "InitDevice", "Failed to create Plane Actor.");
            return E_FAIL;
        }

        // Malla del plano horizontal. Normal=(0,1,0), Tangent=(1,0,0), Bitangent=(0,0,1).
        const XMFLOAT3 planeN = { 0.0f, 1.0f, 0.0f };
        const XMFLOAT3 planeT = { 1.0f, 0.0f, 0.0f };
        const XMFLOAT3 planeB = { 0.0f, 0.0f, 1.0f };
        SimpleVertex planeVertices[] =
        {
            { XMFLOAT3(-kSize, 0.0f, -kSize), planeN, planeT, planeB, XMFLOAT2(0.0f,    0.0f) },
            { XMFLOAT3( kSize, 0.0f, -kSize), planeN, planeT, planeB, XMFLOAT2(kTiling, 0.0f) },
            { XMFLOAT3( kSize, 0.0f,  kSize), planeN, planeT, planeB, XMFLOAT2(kTiling, kTiling) },
            { XMFLOAT3(-kSize, 0.0f,  kSize), planeN, planeT, planeB, XMFLOAT2(0.0f,    kTiling) },
        };
        WORD planeIndices[] = { 0,2,1, 0,3,2 };

        planeMesh.m_vertex.assign(std::begin(planeVertices), std::end(planeVertices));
        planeMesh.m_index.assign(std::begin(planeIndices), std::end(planeIndices));
        planeMesh.m_numVertex = 4;
        planeMesh.m_numIndex = 6;

        std::vector<MeshComponent> planeMeshes{ planeMesh };
        m_APlane->setMesh(m_device, planeMeshes);
        m_planeMeshGpu = Mesh::buildFrom(m_device, planeMeshes);

        LOG_MESSAGE("BaseApp", "init", "Plane mesh created, submeshes=" + std::to_string((int)m_planeMeshGpu.getSubmeshes().size()));

        // Textura del piso: ModelsFBX\piedra (jpg o png) → checkerboard procedural si falla
        // (ver m_planeMaterialInstance.setAlbedo más abajo).
        HRESULT hrTx = m_PlaneTexture.init(m_device, "ModelsFBX\\piedra", JPG);
        if (FAILED(hrTx)) {
            hrTx = m_PlaneTexture.init(m_device, "ModelsFBX\\piedra", PNG);
        }

        if (m_PlaneTexture.m_textureFromImg)
            LOG_MESSAGE("BaseApp", "init", "Plane texture (piedra): loaded");
        else
            LOG_WARNING("BaseApp", "init", "Plane texture (piedra): NOT FOUND — using default checkerboard");

        // Coloca el suelo a Y = -5
        m_APlane->getComponent<Transform>()->setTransform(
            EU::Vector3(0.0f, -5.0f, 0.0f),   // posición
            EU::Vector3(0.0f, 0.0f, 0.0f),   // rotación
            EU::Vector3(1.0f, 1.0f, 1.0f)    // escala
        );

        m_APlane->setName("Suelo");
        m_APlane->setCastShadow(false);
        m_APlane->setReceiveShadow(true);

        m_actors.push_back(m_APlane);
    }

    // 11b) Actor: Sun — luz direccional por defecto. Reemplaza el antiguo sistema global de
    // un solo color/dirección fijo controlado por lightPanel(): ahora es un Light Actor mas,
    // editable desde el Inspector (Hierarchy -> "Sun" -> Transform + seccion Light).
    {
        auto sun = EU::MakeShared<Actor>(m_device);
        if (sun.isNull()) {
            LOG_WARNING("Main", "InitDevice", "Failed to create Sun Actor — continuing without a default light.");
        } else {
            auto lightComp = EU::MakeShared<LightComponent>();
            LightData& lightData = lightComp->getLightData();
            lightData.type      = LightType::Directional;
            lightData.color     = EU::Vector3(1.0f, 1.0f, 1.0f);
            lightData.intensity = 1.0f;
            sun->addComponent(lightComp);

            sun->getComponent<Transform>()->setTransform(
                EU::Vector3(0.0f, 5.0f, 0.0f),  // Position (referencial — Directional no la usa para iluminar)
                EU::Vector3(0.4f, 0.3f, 0.0f),  // Rotation (radianes) -> direccion diagonal hacia abajo
                EU::Vector3(1.0f, 1.0f, 1.0f)   // Scale (sin uso para luces)
            );
            sun->setName("Sun");
            sun->setCastShadow(false); // Flag de sombra de geometria (Actor), no aplica a luces

            m_actors.push_back(sun);
        }
    }

    // 12) Luz
    m_LightPos = XMFLOAT4(2.0f, 4.0f, -2.0f, 1.0f);

    // 13) Deferred Renderer + recursos de escena
    {
        // Cámara (proyección inicial)
        m_camera.setLens(XM_PIDIV4,
                         m_window.m_width / (float)m_window.m_height,
                         0.01f, 100.0f);

        // EditorViewportPass: target offscreen donde el deferred escribe el resultado
        hr = m_editorViewportPass.init(m_device, m_window.m_width, m_window.m_height);
        if (FAILED(hr)) {
            LOG_ERROR("Main", "InitDevice", "Failed to init EditorViewportPass for deferred.");
            return hr;
        }
        LOG_MESSAGE("BaseApp", "init",
            "EditorViewportPass " + std::to_string(m_window.m_width) + "x" + std::to_string(m_window.m_height) +
            " SRV=" + (m_editorViewportPass.getSRV() ? "valid" : "NULL"));

        // Estados de render compartidos por los materiales de los actores
        m_defaultRasterizer.init(m_device, D3D11_FILL_SOLID, D3D11_CULL_BACK, false, false);
        m_defaultDepthStencil.init(m_device, true, D3D11_DEPTH_WRITE_MASK_ALL, D3D11_COMPARISON_LESS);
        m_defaultSampler.init(m_device);

        // Material base opaco (el DeferredRenderer usa su propio gBufferShader, no el de aquí)
        m_defaultMaterial.setRasterizerState(&m_defaultRasterizer);
        m_defaultMaterial.setDepthStencilState(&m_defaultDepthStencil);
        m_defaultMaterial.setSamplerState(&m_defaultSampler);
        m_defaultMaterial.setDomain(MaterialDomain::Opaque);
        m_defaultMaterial.setBlendMode(BlendMode::Opaque);

        // Material "Masked": mismos render states que el opaco (tambien pasa por el
        // G-Buffer deferred, no por m_shaderProgram), solo cambia el dominio — es lo que
        // activa el `clip(albedo.a - AlphaCutoff)` de DeferredGBuffer.hlsl:105.
        m_maskedMaterial.setRasterizerState(&m_defaultRasterizer);
        m_maskedMaterial.setDepthStencilState(&m_defaultDepthStencil);
        m_maskedMaterial.setSamplerState(&m_defaultSampler);
        m_maskedMaterial.setDomain(MaterialDomain::Masked);
        m_maskedMaterial.setBlendMode(BlendMode::Opaque);

        // Material de transparencias (vidrio, etc.). El pass transparente del DeferredRenderer
        // (renderTransparentPass/renderForwardObject) es forward, no deferred, y necesita un
        // ShaderProgram propio del Material (material->getShader()) — se reusa m_shaderProgram,
        // ya compilado desde Soulpher-Engine.fx (paso 6 de este init) con el mismo input layout
        // que las submallas (Pos/Normal/Tangent/Bitangent/Tex) y los mismos CBPerFrame/
        // CBPerObject/CBPerMaterial que el resto del motor.
        m_transparentMaterial.setRasterizerState(&m_defaultRasterizer);
        m_transparentMaterial.setDepthStencilState(&m_defaultDepthStencil);
        m_transparentMaterial.setSamplerState(&m_defaultSampler);
        m_transparentMaterial.setShader(&m_shaderProgram);
        m_transparentMaterial.setDomain(MaterialDomain::Transparent);
        m_transparentMaterial.setBlendMode(BlendMode::Alpha);

        // Texturas de albedo independientes para los MaterialInstances.
        // Si la carga real falla, se usa m_defaultCheckerTexture (checkerboard procedural)
        // en vez de dejar el albedo en nullptr.
        {
            HRESULT th = m_kirbyAlbedoTex.init(m_device, "ModelsFBX\\kirby\\baking", PNG);
            m_kirbyMaterialInstance.setMaterial(&m_defaultMaterial);
            if (SUCCEEDED(th)) {
                m_kirbyMaterialInstance.setAlbedo(&m_kirbyAlbedoTex);
            } else {
                LOG_WARNING("BaseApp", "init", "Kirby albedo texture not found — using default checkerboard.");
                m_kirbyMaterialInstance.setAlbedo(&m_defaultCheckerTexture);
            }
        }
        // Sci-Fi Toad: 3 MaterialInstance independientes (Body/Glass/Head), cada uno con
        // su propio set de mapas PBR. Solo el albedo cae a checkerboard si falla; los
        // mapas N/M/R/AO simplemente se omiten (el shader usa un default neutro por slot,
        // ver DeferredRenderer::renderGeometryObject).
        {
            HRESULT bBC = m_sciFiToadBodyAlbedoTex.init(m_device, "ModelsFBX\\sci-fi-toad\\textures\\Sci-FIToad_Body_BC", PNG);
            HRESULT bN  = m_sciFiToadBodyNormalTex.init(m_device, "ModelsFBX\\sci-fi-toad\\textures\\Sci-FIToad_Body_N", PNG);
            HRESULT bR  = m_sciFiToadBodyRoughnessTex.init(m_device, "ModelsFBX\\sci-fi-toad\\textures\\Sci-FIToad_Body_R", PNG);
            HRESULT bM  = m_sciFiToadBodyMetallicTex.init(m_device, "ModelsFBX\\sci-fi-toad\\textures\\Sci-FIToad_Body_M", PNG);
            HRESULT bAO = m_sciFiToadBodyAOTex.init(m_device, "ModelsFBX\\sci-fi-toad\\textures\\Sci-FIToad_Body_AO", PNG);

            m_sciFiToadBodyMaterialInstance.setMaterial(&m_defaultMaterial);
            if (SUCCEEDED(bBC)) {
                m_sciFiToadBodyMaterialInstance.setAlbedo(&m_sciFiToadBodyAlbedoTex);
            } else {
                LOG_WARNING("BaseApp", "init", "Sci-Fi Toad Body albedo texture not found — using default checkerboard.");
                m_sciFiToadBodyMaterialInstance.setAlbedo(&m_defaultCheckerTexture);
            }
            if (SUCCEEDED(bN))  m_sciFiToadBodyMaterialInstance.setNormal(&m_sciFiToadBodyNormalTex);
            else LOG_WARNING("BaseApp", "init", "Sci-Fi Toad Body normal map not found — skipping.");
            if (SUCCEEDED(bR))  m_sciFiToadBodyMaterialInstance.setRoughness(&m_sciFiToadBodyRoughnessTex);
            else LOG_WARNING("BaseApp", "init", "Sci-Fi Toad Body roughness map not found — skipping.");
            if (SUCCEEDED(bM))  m_sciFiToadBodyMaterialInstance.setMetallic(&m_sciFiToadBodyMetallicTex);
            else LOG_WARNING("BaseApp", "init", "Sci-Fi Toad Body metallic map not found — skipping.");
            if (SUCCEEDED(bAO)) m_sciFiToadBodyMaterialInstance.setAO(&m_sciFiToadBodyAOTex);
            else LOG_WARNING("BaseApp", "init", "Sci-Fi Toad Body AO map not found — skipping.");

            HRESULT gBC = m_sciFiToadGlassAlbedoTex.init(m_device, "ModelsFBX\\sci-fi-toad\\textures\\Sci-FIToad_Glass_BC", PNG);
            HRESULT gR  = m_sciFiToadGlassRoughnessTex.init(m_device, "ModelsFBX\\sci-fi-toad\\textures\\Sci-FIToad_Glass_R", PNG);

            m_sciFiToadGlassMaterialInstance.setMaterial(&m_transparentMaterial);
            if (SUCCEEDED(gBC)) {
                m_sciFiToadGlassMaterialInstance.setAlbedo(&m_sciFiToadGlassAlbedoTex);
            } else {
                LOG_WARNING("BaseApp", "init", "Sci-Fi Toad Glass albedo texture not found — using default checkerboard.");
                m_sciFiToadGlassMaterialInstance.setAlbedo(&m_defaultCheckerTexture);
            }
            if (SUCCEEDED(gR)) m_sciFiToadGlassMaterialInstance.setRoughness(&m_sciFiToadGlassRoughnessTex);
            else LOG_WARNING("BaseApp", "init", "Sci-Fi Toad Glass roughness map not found — skipping.");

            // Sci-FIToad_Glass_BC.png es RGB puro (sin canal alpha) — sin este valor, BlendMode::Alpha
            // no tendria ningun efecto visible porque el alpha muestreado siempre seria 1.0 (opaco).
            // Se fija una sola vez aqui (no cada frame en render()) para que el Material Editor
            // pueda editarlo despues sin que se revierta en el siguiente frame.
            m_sciFiToadGlassMaterialInstance.getParams().baseColor.w = 0.35f;

            // Head/Eyes: BC+N+AO. SSS existe en disco pero no se usa todavia — el shader
            // (DeferredGBuffer.hlsl) no tiene un slot/termino de subsurface scattering.
            HRESULT hBC = m_sciFiToadHeadAlbedoTex.init(m_device, "ModelsFBX\\sci-fi-toad\\textures\\Sci-FIToad_Head_BC", PNG);
            HRESULT hN  = m_sciFiToadHeadNormalTex.init(m_device, "ModelsFBX\\sci-fi-toad\\textures\\Sci-FIToad_Head_N", PNG);
            HRESULT hAO = m_sciFiToadHeadAOTex.init(m_device, "ModelsFBX\\sci-fi-toad\\textures\\Sci-FIToad_Head_AO", PNG);

            m_sciFiToadHeadMaterialInstance.setMaterial(&m_defaultMaterial);
            if (SUCCEEDED(hBC)) {
                m_sciFiToadHeadMaterialInstance.setAlbedo(&m_sciFiToadHeadAlbedoTex);
            } else {
                LOG_WARNING("BaseApp", "init", "Sci-Fi Toad Head albedo texture not found — using default checkerboard.");
                m_sciFiToadHeadMaterialInstance.setAlbedo(&m_defaultCheckerTexture);
            }
            if (SUCCEEDED(hN)) m_sciFiToadHeadMaterialInstance.setNormal(&m_sciFiToadHeadNormalTex);
            else LOG_WARNING("BaseApp", "init", "Sci-Fi Toad Head normal map not found — skipping.");
            if (SUCCEEDED(hAO)) m_sciFiToadHeadMaterialInstance.setAO(&m_sciFiToadHeadAOTex);
            else LOG_WARNING("BaseApp", "init", "Sci-Fi Toad Head AO map not found — skipping.");
        }
        m_planeMaterialInstance.setMaterial(&m_defaultMaterial);
        m_planeMaterialInstance.setAlbedo(m_PlaneTexture.m_textureFromImg ? &m_PlaneTexture : &m_defaultCheckerTexture);

        // DeferredRenderer: carga ShadowMap.hlsl, DeferredGBuffer.hlsl, DeferredLighting.hlsl
        // Usa las mismas dimensiones que EditorViewportPass para que G-Buffer y DSV coincidan.
        LOG_MESSAGE("BaseApp", "init", "Starting DeferredRenderer::init (" + std::to_string(m_window.m_width) + "x" + std::to_string(m_window.m_height) + ")...");
        hr = m_deferredRenderer.init(m_device, m_window.m_width, m_window.m_height);
        if (FAILED(hr)) {
            LOG_ERROR("Main", "InitDevice", ("Failed to init DeferredRenderer. hr=" + std::to_string(hr)).c_str());
            return hr;
        }
        LOG_MESSAGE("BaseApp", "init", "DeferredRenderer ready");
        LOG_MESSAGE("BaseApp", "init", "Init complete - entering game loop");
    }

    // 13b) ForwardRenderer (shadow map)
    hr = m_forwardRenderer.init(m_device);
    if (FAILED(hr)) {
        LOG_ERROR("Main", "InitDevice", ("Failed to initialize ForwardRenderer. hr=" + std::to_string(hr)).c_str());
        return hr;
    }

    // 13c) GizmoRenderer (gizmos de transformación del editor: T/E/R)
    hr = m_gizmoRenderer.init(m_device);
    if (FAILED(hr)) {
        LOG_ERROR("Main", "InitDevice", ("Failed to initialize GizmoRenderer. hr=" + std::to_string(hr)).c_str());
        return hr;
    }

    // 13c-2) LightGizmoRenderer (iconos wireframe de Light Actors)
    hr = m_lightGizmoRenderer.init(m_device);
    if (FAILED(hr)) {
        LOG_ERROR("Main", "InitDevice", ("Failed to initialize LightGizmoRenderer. hr=" + std::to_string(hr)).c_str());
        return hr;
    }

    // 13d) Skybox (cubemap de fondo) — fallo no fatal: sin skybox la escena sigue siendo
    // funcional (RenderScene::skybox se queda nullptr y DeferredRenderer::renderSkyboxPass
    // simplemente no dibuja nada ese frame).
    if (FAILED(m_skybox.init(m_device, "Assets\\skybox.dds"))) {
        LOG_WARNING("BaseApp", "init", "Skybox not loaded — continuing without it.");
    }

    // 14) ImGui (al final del init gráfico)
    m_userInterface.init(
        m_window.m_hWnd,
        m_device.m_device,
        m_deviceContext.m_deviceContext
    );

    // 15) Material Editor: lista persistente de materiales editables + slots de render
    // asignables. Se puebla una sola vez aquí (no cada frame) — el editor le hace push_back
    // directo a m_materialEditorEntries al crear un material nuevo (ver UserInterface::materialEditor).
    m_materialEditorEntries = {
        { "Kirby",             &m_kirbyMaterialInstance },
        { "Plane",             &m_planeMaterialInstance },
        { "SciFiToad - Body",  &m_sciFiToadBodyMaterialInstance },
        { "SciFiToad - Glass", &m_sciFiToadGlassMaterialInstance },
        { "SciFiToad - Head",  &m_sciFiToadHeadMaterialInstance },
    };
    m_materialRenderSlots = {
        { "Kirby",             &m_kirbySlotMaterial },
        { "Plane",             &m_planeSlotMaterial },
        { "SciFiToad - Body",  &m_sciFiToadBodySlotMaterial },
        { "SciFiToad - Glass", &m_sciFiToadGlassSlotMaterial },
        { "SciFiToad - Head",  &m_sciFiToadHeadSlotMaterial },
    };

    return S_OK;
}

/**
 * @brief Redimensiona todos los recursos GPU dependientes del tamaño de ventana.
 *
 * @details
 * El motor no tenia ningun manejo de WM_SIZE: swap chain, depth/stencil, viewport D3D11,
 * aspect ratio de la camara, EditorViewportPass y los G-Buffers del DeferredRenderer se
 * creaban una sola vez en init() con el tamaño inicial de la ventana y quedaban congelados
 * para siempre. Al redimensionar el OS window, io.DisplaySize (que ImGui lee del cliente real)
 * si cambiaba, asi que "##DeferredViewport" se agrandaba/achicaba — pero ImGui::Image()
 * terminaba estirando una textura con la resolucion VIEJA dentro de un rectangulo de tamaño
 * NUEVO, y la matriz de Projection de la camara seguia asumiendo el aspect ratio viejo. El
 * rayo de picking (unproyectado con esa Projection vieja) dejaba de coincidir con lo que
 * realmente se veia en pantalla — de ahi que el click perdiera precision justo despues de
 * redimensionar. Llamado desde update() (polling de GetClientRect) cuando cambia el tamaño.
 */
void BaseApp::onResize(unsigned int width, unsigned int height) {
    if (width == 0 || height == 0) return; // minimizada u otro estado degenerado -> no hay nada que redimensionar

    // El backbuffer/RTV/DSV actuales siguen enlazados al pipeline desde el render() del frame
    // anterior — DXGI no permite ResizeBuffers() mientras el backbuffer este en uso. Se llama al
    // ID3D11DeviceContext crudo (no al wrapper DeviceContext::OMSetRenderTargets) porque ese
    // wrapper trata (0, nullptr, nullptr) como caso de error y no reenvia la llamada real —
    // silenciaria este unbind y ResizeBuffers seguiria viendo el backbuffer en uso.
    m_deviceContext.m_deviceContext->OMSetRenderTargets(0, nullptr, nullptr);

    m_renderTargetView.destroy();
    m_backBuffer.destroy();

    HRESULT hr = m_swapChain.m_swapChain->ResizeBuffers(1, width, height, DXGI_FORMAT_R8G8B8A8_UNORM, 0);
    if (FAILED(hr)) {
        LOG_ERROR("BaseApp", "onResize", ("ResizeBuffers failed. hr=" + std::to_string(hr)).c_str());
        return;
    }

    ID3D11Texture2D* bb = nullptr;
    hr = m_swapChain.m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&bb);
    if (FAILED(hr)) {
        LOG_ERROR("BaseApp", "onResize", ("GetBuffer failed tras ResizeBuffers. hr=" + std::to_string(hr)).c_str());
        return;
    }
    m_backBuffer.attach(bb);

    hr = m_renderTargetView.init(m_device, m_backBuffer, DXGI_FORMAT_R8G8B8A8_UNORM);
    if (FAILED(hr)) {
        LOG_ERROR("BaseApp", "onResize", "Failed to recreate RenderTargetView.");
        return;
    }

    m_depthStencilView.destroy();
    m_depthStencil.destroy();
    hr = m_depthStencil.init(m_device, width, height, DXGI_FORMAT_D24_UNORM_S8_UINT, D3D11_BIND_DEPTH_STENCIL, 1, 0);
    if (FAILED(hr)) {
        LOG_ERROR("BaseApp", "onResize", "Failed to recreate DepthStencil texture.");
        return;
    }
    hr = m_depthStencilView.init(m_device, m_depthStencil, DXGI_FORMAT_D24_UNORM_S8_UINT);
    if (FAILED(hr)) {
        LOG_ERROR("BaseApp", "onResize", "Failed to recreate DepthStencilView.");
        return;
    }

    m_viewport.init(width, height);

    // Aspect ratio de la camara — la misma fuente que usa el picking (m_camera.getProj())
    // debe coincidir siempre con el tamaño real del render target, no con el de init().
    const float aspect = width / (FLOAT)height;
    m_Projection = XMMatrixPerspectiveFovLH(XM_PIDIV4, aspect, 0.01f, 100.0f);
    m_camera.setLens(XM_PIDIV4, aspect, 0.01f, 100.0f);

    hr = m_editorViewportPass.resize(m_device, width, height);
    if (FAILED(hr)) {
        LOG_ERROR("BaseApp", "onResize", "Failed to resize EditorViewportPass.");
        return;
    }
    m_deferredRenderer.resize(m_device, width, height);

    m_window.m_width  = width;
    m_window.m_height = height;

    LOG_MESSAGE("BaseApp", "onResize", "Resized to " + std::to_string(width) + "x" + std::to_string(height));
}

/**
 * @brief Actualiza la lógica de la aplicación en cada frame.
 *
 * Tareas:
 *  - Avanza el frame de ImGui y muestra paneles (inspector / outliner).
 *  - Gestión de selección de actores.
 *  - Cálculo de tiempo simple.
 *  - Controles de cámara orbital:
 *      - RMB (botón derecho): orbitar yaw/pitch.
 *      - Rueda: zoom.
 *      - MMB (botón medio): pan.
 *  - Recalcula la vista y sube buffers de cámara (b0: View, b1: Projection).
 *  - Llama a update() de todos los actores.
 */

/**
 * @brief Test de intersección rayo-AABB (slab method), en espacio mundo.
 * @param origin Origen del rayo.
 * @param dir    Dirección del rayo (se asume normalizada).
 * @param bmin   Esquina mínima del AABB (mundo).
 * @param bmax   Esquina máxima del AABB (mundo).
 * @param tOut   Distancia a lo largo del rayo hasta el primer punto de entrada (si hay hit).
 * @return true si el rayo intersecta el AABB con t >= 0.
 */
static bool RayIntersectsAABB(FXMVECTOR origin, FXMVECTOR dir,
                               const XMFLOAT3& bmin, const XMFLOAT3& bmax, float& tOut) {
    XMFLOAT3 o, d;
    XMStoreFloat3(&o, origin);
    XMStoreFloat3(&d, dir);
    float orig[3] = { o.x, o.y, o.z };
    float dirs[3] = { d.x, d.y, d.z };
    float mn[3]   = { bmin.x, bmin.y, bmin.z };
    float mx[3]   = { bmax.x, bmax.y, bmax.z };

    float tmin = 0.0f, tmax = FLT_MAX;
    for (int i = 0; i < 3; ++i) {
        if (fabsf(dirs[i]) < 1e-8f) {
            if (orig[i] < mn[i] || orig[i] > mx[i]) return false;
        } else {
            float t1 = (mn[i] - orig[i]) / dirs[i];
            float t2 = (mx[i] - orig[i]) / dirs[i];
            if (t1 > t2) std::swap(t1, t2);
            tmin = (std::max)(tmin, t1);
            tmax = (std::min)(tmax, t2);
            if (tmin > tmax) return false;
        }
    }
    tOut = tmin;
    return true;
}

/**
 * @brief Punto mas cercano entre un rayo (origin+t*dir) y una recta infinita (lineOrigin+t*lineDir).
 * @param outLineT Parametro a lo largo de la recta (util como "distancia con signo" cuando lineDir
 *                 es un eje unitario del mundo — es exactamente el delta de traslacion/escala).
 * @param outRayT  Parametro a lo largo del rayo (para saber que tan cerca de la camara esta el punto).
 * @return false si el rayo y la recta son (casi) paralelos.
 */
static bool ClosestPointRayLine(FXMVECTOR rayOrigin, FXMVECTOR rayDir,
                                 FXMVECTOR lineOrigin, FXMVECTOR lineDir,
                                 float& outLineT, float& outRayT) {
    XMVECTOR r = XMVectorSubtract(rayOrigin, lineOrigin);
    float a = XMVectorGetX(XMVector3Dot(rayDir, rayDir));
    float b = XMVectorGetX(XMVector3Dot(rayDir, lineDir));
    float c = XMVectorGetX(XMVector3Dot(lineDir, lineDir));
    float d = XMVectorGetX(XMVector3Dot(rayDir, r));
    float e = XMVectorGetX(XMVector3Dot(lineDir, r));
    float denom = a * c - b * b;
    if (fabsf(denom) < 1e-6f) return false;
    outRayT  = (b * e - c * d) / denom;
    outLineT = (a * e - b * d) / denom;
    return true;
}

/** @brief Interseccion rayo-plano (plano definido por un punto y su normal). t>=0 requerido. */
static bool RayPlaneIntersect(FXMVECTOR rayOrigin, FXMVECTOR rayDir,
                               FXMVECTOR planePoint, FXMVECTOR planeNormal, float& outT) {
    float denom = XMVectorGetX(XMVector3Dot(rayDir, planeNormal));
    if (fabsf(denom) < 1e-6f) return false;
    float t = XMVectorGetX(XMVector3Dot(XMVectorSubtract(planePoint, rayOrigin), planeNormal)) / denom;
    if (t < 0.0f) return false;
    outT = t;
    return true;
}

/** @brief Angulo (radianes) de un punto proyectado sobre el plano perpendicular a un eje del mundo. */
static float GizmoAngleOnPlane(FXMVECTOR point, FXMVECTOR center, GizmoRenderer::Axis axis) {
    XMVECTOR u, v;
    switch (axis) {
        case GizmoRenderer::Axis::X: u = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f); v = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f); break;
        case GizmoRenderer::Axis::Y: u = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f); v = XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f); break;
        default:                    u = XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f); v = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f); break; // Z
    }
    XMVECTOR vec = XMVectorSubtract(point, center);
    float du = XMVectorGetX(XMVector3Dot(vec, u));
    float dv = XMVectorGetX(XMVector3Dot(vec, v));
    return atan2f(dv, du);
}

void BaseApp::update()
{
    // Poll de resize: el motor no engancha WM_SIZE, asi que el cambio de tamaño real del
    // cliente se detecta aqui, antes de que ImGui arranque el frame (mismo GetClientRect que
    // usa internamente el backend Win32 de ImGui para calcular io.DisplaySize este frame).
    {
        RECT rc{};
        if (GetClientRect(m_window.m_hWnd, &rc)) {
            unsigned int newW = static_cast<unsigned int>(rc.right - rc.left);
            unsigned int newH = static_cast<unsigned int>(rc.bottom - rc.top);
            if (newW > 0 && newH > 0 && (newW != m_window.m_width || newH != m_window.m_height)) {
                onResize(newW, newH);
            }
        }
    }

    // --- UI frame ---
    m_userInterface.update();

    // Layout fijo del panel izquierdo (Hierarchy arriba + Inspector abajo), ancho fijo
    // kLeftPanelWidth: el viewport central resta este ancho + el de G-Buffers (ver render()).
    // topMargin = alto de la main menu bar de este frame (ya abierta por m_userInterface.update()).
    const float topMargin    = ImGui::GetFrameHeight();
    const float leftColH     = ImGui::GetIO().DisplaySize.y - topMargin;
    const float hierarchyH   = leftColH * 0.4f;
    const float inspectorH   = leftColH - hierarchyH - kPanelGap;

    // Panel de sombras/UI general — la edicion de luces vive en el Inspector (por Light Actor).
    // shadowBias local: el slider UI lo modifica pero el shader usa 0.003f hardcodeado
    static float s_shadowBias = 0.003f;
    static bool s_viewShadowMap = false;
    m_userInterface.lightPanel(s_shadowBias, s_viewShadowMap);
    m_deferredRenderer.setDeferredDebugViewMode(s_viewShadowMap ? 7 : 0);
    m_userInterface.interfacePanel();

    // Inspector + Outliner
    // selectedActorIndex == -1 es un estado valido ("nada seleccionado", ver picking abajo).
    // Solo se corrige si el indice quedo fuera de rango (p.ej. un actor fue eliminado).
    if (!m_actors.empty())
    {
        if (m_userInterface.selectedActorIndex >= (int)m_actors.size())
        {
            m_userInterface.selectedActorIndex = -1;
        }
        if (m_userInterface.selectedActorIndex >= 0)
        {
            // FirstUseEver (no Always): fija una posicion/tamaño inicial razonable, pero deja
            // que el usuario mueva, redimensione o acople (dock) libremente esta ventana
            // despues — con Always quedaba "clavada" cada frame y no se podia mover ni
            // acoplarle otras ventanas (ej. Lighting) encima.
            ImGui::SetNextWindowPos(ImVec2(0.0f, topMargin + hierarchyH + kPanelGap), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowSize(ImVec2(kLeftPanelWidth, inspectorH), ImGuiCond_FirstUseEver);
            m_userInterface.inspectorGeneral(m_actors[m_userInterface.selectedActorIndex]);
        }
    }
    ImGui::SetNextWindowPos(ImVec2(0.0f, topMargin), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(kLeftPanelWidth, hierarchyH), ImGuiCond_FirstUseEver);
    m_userInterface.outliner(m_actors);

    // Material Editor: edita en vivo los MaterialInstance reales que este mismo metodo
    // asigna por nombre a los RenderObject de la escena (ver bloque "if (name == ...)"
    // mas abajo) — la reasignación de qué MaterialInstance usa cada slot pasa por la
    // indirección de puntero en m_materialRenderSlots, no por tocar ese if/else.
    m_userInterface.materialEditor(m_materialEditorEntries, m_materialRenderSlots,
        m_device, m_editorLoadedTextures, m_editorCreatedMaterials,
        m_defaultMaterial, m_transparentMaterial, m_maskedMaterial);

    // Creación de Light Actors desde el botón "+" del outliner (Hierarchy).
    if (m_userInterface.requestedLightType >= 0)
    {
        LightType requestedType = static_cast<LightType>(m_userInterface.requestedLightType);
        m_userInterface.requestedLightType = -1;

        auto lightActor = EU::MakeShared<Actor>(m_device);
        if (!lightActor.isNull())
        {
            auto lightComp = EU::MakeShared<LightComponent>();
            LightData& lightData = lightComp->getLightData();
            lightData.type      = requestedType;
            lightData.color     = EU::Vector3(1.0f, 1.0f, 1.0f);
            lightData.intensity = 1.0f;
            if (requestedType == LightType::Point || requestedType == LightType::Spot) {
                lightData.range = 10.0f;
            }
            if (requestedType == LightType::Spot) {
                lightData.spotAngle = XMConvertToRadians(30.0f);
            }
            lightActor->addComponent(lightComp);

            lightActor->getComponent<Transform>()->setTransform(
                EU::Vector3(0.0f, 2.0f, 0.0f),
                EU::Vector3(0.0f, 0.0f, 0.0f),
                EU::Vector3(1.0f, 1.0f, 1.0f)
            );

            const char* typeName = requestedType == LightType::Directional ? "Directional Light"
                                  : requestedType == LightType::Point       ? "Point Light"
                                                                              : "Spot Light";
            lightActor->setName(typeName);
            lightActor->setCastShadow(false);

            m_actors.push_back(lightActor);
            m_userInterface.selectedActorIndex = (int)m_actors.size() - 1;
        }
    }

    // Borrado de actores desde el menú contextual (clic derecho) del outliner.
    if (m_userInterface.requestedDeleteIndex >= 0 && m_userInterface.requestedDeleteIndex < (int)m_actors.size())
    {
        int deleteIndex = m_userInterface.requestedDeleteIndex;
        m_userInterface.requestedDeleteIndex = -1;

        auto& actorToDelete = m_actors[deleteIndex];
        if (!actorToDelete.isNull()) {
            // "Suelo" se renderiza siempre vía el puntero dedicado m_APlane (no por nombre en
            // m_actors, a diferencia de Kirby) — hay que soltarlo aquí o seguiría dibujándose.
            if (actorToDelete.get() == m_APlane.get()) {
                m_APlane = EU::TSharedPointer<Actor>();
            }
            actorToDelete->destroy();
        }
        m_actors.erase(m_actors.begin() + deleteIndex);

        if (m_userInterface.selectedActorIndex == deleteIndex) {
            m_userInterface.selectedActorIndex = -1;
        } else if (m_userInterface.selectedActorIndex > deleteIndex) {
            m_userInterface.selectedActorIndex -= 1;
        }
    }

    // Duplicación de actores desde el menú contextual (clic derecho) del outliner.
    // NOTA: la copia conserva el mismo nombre que el original — Kirby se identifica por nombre
    // en render() (no por instancia), así que renombrar la copia haría que no se dibuje. Por la
    // misma razón, duplicar "Suelo" no produce una copia visible: ese actor se renderiza siempre
    // vía el puntero único m_APlane, nunca iterando m_actors (limitación existente del motor).
    if (m_userInterface.requestedDuplicateIndex >= 0 && m_userInterface.requestedDuplicateIndex < (int)m_actors.size())
    {
        int duplicateIndex = m_userInterface.requestedDuplicateIndex;
        m_userInterface.requestedDuplicateIndex = -1;

        auto& source = m_actors[duplicateIndex];
        if (!source.isNull()) {
            auto copy = EU::MakeShared<Actor>(m_device);
            if (!copy.isNull()) {
                copy->setName(source->getName());
                copy->setCastShadow(source->canCastShadow());

                auto srcTransform = source->getComponent<Transform>();
                copy->getComponent<Transform>()->setTransform(
                    srcTransform->getPosition(), srcTransform->getRotation(), srcTransform->getScale());

                EU::TSharedPointer<LightComponent> srcLight = source->getComponent<LightComponent>();
                if (!srcLight.isNull()) {
                    auto lightCopy = EU::MakeShared<LightComponent>();
                    lightCopy->getLightData() = srcLight->getLightData();
                    lightCopy->setCastShadow(srcLight->canCastShadow());
                    copy->addComponent(lightCopy);
                }

                m_actors.push_back(copy);
                m_userInterface.selectedActorIndex = (int)m_actors.size() - 1;
            }
        }
    }

    // Consola de logs — franja debajo del viewport (posición inicial; el usuario puede
    // moverla/acoplarla libremente después). m_viewportPos/m_viewportSize son del frame
    // anterior (mismo patrón de latencia que el resto de UI relativa al viewport).
    {
        float consoleX = m_viewportPos.x;
        float consoleY = m_viewportPos.y + m_viewportSize.y + 4.0f;
        float consoleW = m_viewportSize.x;
        float consoleH = (std::max)(ImGui::GetIO().DisplaySize.y - consoleY - 4.0f, 80.0f);
        ImGui::SetNextWindowPos(ImVec2(consoleX, consoleY), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(consoleW, consoleH), ImGuiCond_FirstUseEver);
    }
    // stats con un frame de latencia: se calculan al final de render(), update() corre antes
    // que render() en el mismo ciclo del loop.
    m_userInterface.output();
    m_userInterface.statsPanel(m_fps, m_frameTimeMs, m_lastDrawCallCount);

    // --- Tiempo ---
    static float t = 0.0f;
    static DWORD t0 = 0;
    DWORD tNow = GetTickCount();
    if (t0 == 0) t0 = tNow;
    t = (tNow - t0) / 1000.0f;

    // ----------------------------------------------------
    // CONTROLES DE CÁMARA FREE-FLY (estilo Unreal/Roblox)
    //   RMB + mouse  -> rotar (yaw/pitch)
    //   RMB + WASD   -> mover (forward/right), Shift = x3 velocidad
    //   MMB + drag   -> pan (right/up de la camara)
    //   Rueda        -> dolly adelante/atrás
    //   F            -> encuadra el actor seleccionado sin cambiar la orientación
    // ----------------------------------------------------
    {
        ImGuiIO& io = ImGui::GetIO();
        // io.WantCaptureMouse es true tambien al pasar el mouse sobre "##DeferredViewport"
        // (es una ventana ImGui como cualquier otra), lo que bloqueaba los controles
        // justo en el area central y solo los dejaba operar en los huecos entre los
        // paneles de G-Buffer. m_viewportHovered (actualizado en render(), frame anterior)
        // excluye esa ventana del bloqueo. Este flag solo gatea el INICIO de un drag o
        // una pulsación puntual (rueda, F) — una vez que RMB/MMB ya está sostenido el
        // drag continúa aunque el mouse salga del viewport, y solo se corta al soltar
        // el botón (comportamiento estilo Unreal/Maya con "captura" de mouse).
        bool uiCapturaMouse = io.WantCaptureMouse && !m_viewportHovered;

        static bool orbitando = false, paneando = false;
        static POINT ultimoOrbit{}, ultimoPan{};
        static bool fWasDown = false;
        static ULONGLONG lastTickMs = 0;

        // Delta time real (no el tiempo total "t") para que WASD/pan no dependan del framerate.
        ULONGLONG nowMs = GetTickCount64();
        if (lastTickMs == 0) lastTickMs = nowMs;
        float dt = (nowMs - lastTickMs) / 1000.0f;
        lastTickMs = nowMs;
        dt = (std::min)(dt, 0.1f); // evita saltos de posicion tras un stall (breakpoint, carga, etc.)

        auto computeBasis = [](float yawDeg, float pitchDeg, XMVECTOR& fwd, XMVECTOR& right, XMVECTOR& up)
        {
            float yaw = XMConvertToRadians(yawDeg);
            float pitch = XMConvertToRadians(pitchDeg);
            fwd = XMVectorSet(sinf(yaw) * cosf(pitch), sinf(pitch), cosf(yaw) * cosf(pitch), 0.0f);
            XMVECTOR worldUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
            right = XMVector3Normalize(XMVector3Cross(worldUp, fwd));
            up    = XMVector3Normalize(XMVector3Cross(fwd, right));
        };

        // ROTAR (RMB) + MOVER (WASD mientras RMB está sostenido)
        bool rmbDown = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;
        if (rmbDown && !orbitando && !uiCapturaMouse)
        {
            orbitando = true;
            GetCursorPos(&ultimoOrbit);
        }
        if (!rmbDown) orbitando = false;

        if (orbitando)
        {
            POINT p; GetCursorPos(&p);
            float dx = float(p.x - ultimoOrbit.x);
            float dy = float(p.y - ultimoOrbit.y);
            m_camYawDeg += dx * 0.25f;
            m_camPitchDeg -= dy * 0.25f;
            m_camPitchDeg = std::clamp(m_camPitchDeg, -89.0f, 89.0f);
            ultimoOrbit = p;

            XMVECTOR fwd, right, up;
            computeBasis(m_camYawDeg, m_camPitchDeg, fwd, right, up);

            float speed = 6.0f; // unidades/seg
            if (GetAsyncKeyState(VK_SHIFT) & 0x8000) speed *= 3.0f;
            float moveDist = speed * dt;

            XMVECTOR pos = XMLoadFloat3(&m_camPos);
            if (GetAsyncKeyState('W') & 0x8000) pos = XMVectorMultiplyAdd(XMVectorReplicate( moveDist), fwd,   pos);
            if (GetAsyncKeyState('S') & 0x8000) pos = XMVectorMultiplyAdd(XMVectorReplicate(-moveDist), fwd,   pos);
            if (GetAsyncKeyState('D') & 0x8000) pos = XMVectorMultiplyAdd(XMVectorReplicate( moveDist), right, pos);
            if (GetAsyncKeyState('A') & 0x8000) pos = XMVectorMultiplyAdd(XMVectorReplicate(-moveDist), right, pos);
            XMStoreFloat3(&m_camPos, pos);
        }

        // ZOOM (rueda) — dolly a lo largo de forward
        if (!uiCapturaMouse && io.MouseWheel != 0.0f)
        {
            XMVECTOR fwd, right, up;
            computeBasis(m_camYawDeg, m_camPitchDeg, fwd, right, up);
            XMVECTOR pos = XMLoadFloat3(&m_camPos);
            pos = XMVectorMultiplyAdd(XMVectorReplicate(io.MouseWheel * m_camDistance * 0.1f), fwd, pos);
            XMStoreFloat3(&m_camPos, pos);
        }

        // PAN (MMB)
        bool mmbDown = (GetAsyncKeyState(VK_MBUTTON) & 0x8000) != 0;
        if (mmbDown && !paneando && !uiCapturaMouse)
        {
            paneando = true;
            GetCursorPos(&ultimoPan);
        }
        if (!mmbDown) paneando = false;

        if (paneando)
        {
            POINT p; GetCursorPos(&p);
            float dx = float(p.x - ultimoPan.x);
            float dy = float(p.y - ultimoPan.y);
            ultimoPan = p;

            XMVECTOR fwd, right, up;
            computeBasis(m_camYawDeg, m_camPitchDeg, fwd, right, up);

            float panSpeed = m_camDistance * 0.0025f;
            XMVECTOR pos = XMLoadFloat3(&m_camPos);
            pos = XMVectorMultiplyAdd(XMVectorReplicate(-dx * panSpeed), right, pos);
            pos = XMVectorMultiplyAdd(XMVectorReplicate( dy * panSpeed), up,    pos);
            XMStoreFloat3(&m_camPos, pos);
        }

        // FOCUS (F) — encuadra el actor seleccionado sin reapuntar la camara
        bool fDown = (GetAsyncKeyState('F') & 0x8000) != 0;
        if (fDown && !fWasDown && !uiCapturaMouse &&
            !m_actors.empty() &&
            m_userInterface.selectedActorIndex >= 0 &&
            m_userInterface.selectedActorIndex < (int)m_actors.size())
        {
            const auto& actor = m_actors[m_userInterface.selectedActorIndex];
            if (!actor.isNull())
            {
                const EU::Vector3& targetPos = actor->getComponent<Transform>()->getPosition();
                XMVECTOR fwd, right, up;
                computeBasis(m_camYawDeg, m_camPitchDeg, fwd, right, up);
                XMVECTOR target = XMVectorSet(targetPos.x, targetPos.y, targetPos.z, 1.0f);
                XMVECTOR pos = XMVectorMultiplyAdd(XMVectorReplicate(-m_camDistance), fwd, target);
                XMStoreFloat3(&m_camPos, pos);
            }
        }
        fWasDown = fDown;

        // MODO DE GIZMO (T=Translate, E=Scale, R=Rotate). Gateado por WantTextInput (true solo
        // mientras un InputText de ImGui esta activo, p.ej. el nombre del actor en el inspector)
        // en vez de WantCaptureKeyboard: con ImGuiConfigFlags_NavEnableKeyboard activo (ver
        // UserInterface::init) WantCaptureKeyboard es casi siempre true con solo tener un panel
        // enfocado, lo que bloqueaba E/R permanentemente (T "funcionaba" solo porque ya es el modo
        // por defecto). No compite con WASD: son teclas distintas y WASD solo se lee con RMB.
        {
            static bool tWasDown = false, eWasDown = false, rWasDown = false;
            bool tDown = (GetAsyncKeyState('T') & 0x8000) != 0;
            bool eDown = (GetAsyncKeyState('E') & 0x8000) != 0;
            bool rDown = (GetAsyncKeyState('R') & 0x8000) != 0;
            if (!io.WantTextInput) {
                if (tDown && !tWasDown) m_gizmoMode = GizmoRenderer::Mode::Translate;
                if (eDown && !eWasDown) m_gizmoMode = GizmoRenderer::Mode::Scale;
                if (rDown && !rWasDown) m_gizmoMode = GizmoRenderer::Mode::Rotate;
            }
            tWasDown = tDown; eWasDown = eDown; rWasDown = rDown;
        }

        // Recalcular la vista (m_View) con posicion libre + yaw/pitch actuales
        {
            XMVECTOR fwd, right, up;
            computeBasis(m_camYawDeg, m_camPitchDeg, fwd, right, up);

            XMVECTOR eye = XMLoadFloat3(&m_camPos);
            XMVECTOR at  = XMVectorAdd(eye, fwd);
            XMVECTOR worldUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

            m_View = XMMatrixLookAtLH(eye, at, worldUp);

            // Sincronizar Camera class para DeferredRenderer
            XMFLOAT3 eyeF, atF, upF(0.0f, 1.0f, 0.0f);
            XMStoreFloat3(&eyeF, eye);
            XMStoreFloat3(&atF, at);
            m_camera.lookAt(eyeF, atF, upF);
            m_camera.updateViewMatrix();
        }

        // ----------------------------------------------------
        // RAYO DE MOUSE — calculado cada frame que el cursor esta sobre el viewport y no se
        // esta orbitando; lo reutilizan tanto el hover/arrastre del gizmo como el picking de
        // actores (antes cada uno recalculaba su propia version de este mismo rayo).
        // ----------------------------------------------------
        bool     rayValid  = false;
        XMVECTOR rayOrigin = XMVectorZero();
        XMVECTOR rayDir    = XMVectorZero();
        if (!uiCapturaMouse && !orbitando)
        {
            // Rect real del viewport (medido en render() del frame anterior — mismo patron de
            // latencia que m_viewportHovered). Antes se asumia origen (0,0) y ancho
            // "io.DisplaySize.x * 0.68f", que se desalinea de la imagen real mostrada (viewports/
            // docking de ImGui no garantizan que la ventana arranque en (0,0), y ese 0.68f fijo
            // no sigue el tamaño real medido por GetContentRegionAvail() al redimensionar).
            const float vpX = m_viewportPos.x;
            const float vpY = m_viewportPos.y;
            const float vpW = m_viewportSize.x;
            const float vpH = m_viewportSize.y;

            // Viewport degenerado (ventana minimizada/tamano 0 momentaneo) -> no hay nada que picar.
            bool viewportValid = (vpW > 1.0f && vpH > 1.0f);

            // Validar View*Proj ANTES de invertir: XMMatrixInverse de esta version de xnamath
            // (a diferencia de DirectXMath moderno) exige un puntero real para pDeterminant
            // (XMASSERT en xnamathmatrix.inl) y no protege contra matrices singulares — una
            // camara con FOV=0, near==far o aspect invalido produce una Projection singular,
            // y View*Proj hereda esa singularidad (determinante ~0).
            XMMATRIX viewProj = m_camera.getView() * m_camera.getProj();
            XMVECTOR determinant;
            XMMATRIX invVP = XMMatrixInverse(&determinant, viewProj);
            float det = XMVectorGetX(determinant);
            bool matrixValid = viewportValid && std::isfinite(det) && fabsf(det) > 1e-6f;

            if (!matrixValid) {
                LOG_WARNING("BaseApp", "update",
                    "Picking cancelado: View*Proj invalida o singular (det=" + std::to_string(det) +
                    "). Revisa FOV/near/far/aspect de la camara.");
            }
            else {
                // Mouse -> NDC. io.MousePos esta en screen space (mismo espacio que
                // GetCursorScreenPos()); hay que restar la esquina superior-izquierda real del
                // viewport (vpX,vpY) antes de normalizar — sin esto el mouse local dentro de la
                // imagen queda desplazado por el offset de la ventana.
                float u = (io.MousePos.x - vpX) / vpW;
                float v = (io.MousePos.y - vpY) / vpH;
                float ndcX = u * 2.0f - 1.0f;
                float ndcY = 1.0f - v * 2.0f;

                // Unproyectar el punto NDC en el plano far; el rayo va del ojo de la camara a ese punto.
                XMVECTOR ndcFar = XMVectorSet(ndcX, ndcY, 1.0f, 1.0f);
                XMVECTOR worldFar = XMVector4Transform(ndcFar, invVP);
                worldFar = XMVectorDivide(worldFar, XMVectorSplatW(worldFar));

                rayOrigin = XMLoadFloat3(&m_camPos);
                rayDir = XMVector3Normalize(worldFar - rayOrigin);
                rayValid = true;
            }
        }

        // ----------------------------------------------------
        // GIZMO DE TRANSFORMACION — hover + arrastre por eje del actor seleccionado.
        // Tiene prioridad sobre el picking de actores: un click sobre un handle inicia el
        // arrastre en vez de (de)seleccionar un actor.
        // ----------------------------------------------------
        bool hasSelection = !m_actors.empty() &&
            m_userInterface.selectedActorIndex >= 0 &&
            m_userInterface.selectedActorIndex < (int)m_actors.size() &&
            !m_actors[m_userInterface.selectedActorIndex].isNull();

        EU::TSharedPointer<Transform> selectedTransform = hasSelection
            ? m_actors[m_userInterface.selectedActorIndex]->getComponent<Transform>()
            : EU::TSharedPointer<Transform>();

        XMFLOAT3 gizmoCenter{};
        if (!selectedTransform.isNull()) {
            const EU::Vector3& p = selectedTransform->getPosition();
            gizmoCenter = XMFLOAT3(p.x, p.y, p.z);
            XMVECTOR distVec = XMVector3Length(XMVectorSubtract(XMLoadFloat3(&gizmoCenter), XMLoadFloat3(&m_camPos)));
            // Tamano constante en pantalla: el radio del gizmo escala linealmente con la
            // distancia a la camara (aproximacion estandar, ignora FOV/aspect exactos).
            m_gizmoScreenScale = (std::max)(XMVectorGetX(distVec) * 0.12f, 0.01f);
        }

        GizmoRenderer::Axis hoverAxis = GizmoRenderer::Axis::None;
        if (rayValid && !selectedTransform.isNull() &&
            m_gizmoMode != GizmoRenderer::Mode::None && m_gizmoDragAxis == GizmoRenderer::Axis::None)
        {
            XMVECTOR centerVec = XMLoadFloat3(&gizmoCenter);
            float bestT = FLT_MAX;

            // Handle central de escala uniforme (solo Mode::Scale): distancia punto-rayo al
            // centro del gizmo, sin depender de ningun eje.
            if (m_gizmoMode == GizmoRenderer::Mode::Scale) {
                XMVECTOR toCenter = XMVectorSubtract(centerVec, rayOrigin);
                float tClosest = XMVectorGetX(XMVector3Dot(toCenter, rayDir));
                if (tClosest > 0.0f) {
                    XMVECTOR closestOnRay = XMVectorAdd(rayOrigin, XMVectorScale(rayDir, tClosest));
                    float distToCenter = XMVectorGetX(XMVector3Length(XMVectorSubtract(closestOnRay, centerVec)));
                    float centerPickWorld = GizmoRenderer::kCenterPickRadius * m_gizmoScreenScale;
                    if (distToCenter < centerPickWorld && tClosest < bestT) {
                        bestT = tClosest;
                        hoverAxis = GizmoRenderer::Axis::All;
                    }
                }
            }

            const GizmoRenderer::Axis axes[3] = { GizmoRenderer::Axis::X, GizmoRenderer::Axis::Y, GizmoRenderer::Axis::Z };
            for (GizmoRenderer::Axis axis : axes) {
                XMVECTOR axisDir = GizmoRenderer::axisDirectionVec(axis);
                if (m_gizmoMode == GizmoRenderer::Mode::Rotate) {
                    float t;
                    if (RayPlaneIntersect(rayOrigin, rayDir, centerVec, axisDir, t) && t < bestT) {
                        XMVECTOR hitPoint = XMVectorAdd(rayOrigin, XMVectorScale(rayDir, t));
                        float distFromCenter = XMVectorGetX(XMVector3Length(XMVectorSubtract(hitPoint, centerVec)));
                        float ringWorldRadius = GizmoRenderer::kRingRadius * m_gizmoScreenScale;
                        float tolWorld = GizmoRenderer::kRingPickTol * m_gizmoScreenScale;
                        if (fabsf(distFromCenter - ringWorldRadius) < tolWorld) {
                            bestT = t;
                            hoverAxis = axis;
                        }
                    }
                } else {
                    float lineT, rayT;
                    if (ClosestPointRayLine(rayOrigin, rayDir, centerVec, axisDir, lineT, rayT) &&
                        rayT > 0.0f && rayT < bestT) {
                        float armWorld = GizmoRenderer::kArmLength * m_gizmoScreenScale;
                        float lineTClamped = std::clamp(lineT, 0.0f, armWorld);
                        XMVECTOR linePoint = XMVectorAdd(centerVec, XMVectorScale(axisDir, lineTClamped));
                        XMVECTOR rayPoint  = XMVectorAdd(rayOrigin, XMVectorScale(rayDir, rayT));
                        float dist = XMVectorGetX(XMVector3Length(XMVectorSubtract(linePoint, rayPoint)));
                        float pickWorld = GizmoRenderer::kPickRadius * m_gizmoScreenScale;
                        if (dist < pickWorld) {
                            bestT = rayT;
                            hoverAxis = axis;
                        }
                    }
                }
            }
        }
        m_gizmoHoverAxis = hoverAxis;

        bool gizmoConsumedClick = false;

        // Inicio de arrastre: click sobre un handle con hover activo este mismo frame.
        if (rayValid && !selectedTransform.isNull() && hoverAxis != GizmoRenderer::Axis::None &&
            m_gizmoDragAxis == GizmoRenderer::Axis::None &&
            ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
            m_gizmoDragAxis = hoverAxis;
            m_gizmoDragStartPosition = selectedTransform->getPosition();
            m_gizmoDragStartScale    = selectedTransform->getScale();
            m_gizmoDragStartRotation = selectedTransform->getRotation();

            XMVECTOR centerVec = XMLoadFloat3(&gizmoCenter);
            if (hoverAxis == GizmoRenderer::Axis::All) {
                // Escala uniforme: distancia al centro medida sobre un plano que mira a la
                // camara (no hay un eje unico sobre el que proyectar).
                XMVECTOR planeNormal = XMVector3Normalize(XMVectorSubtract(XMLoadFloat3(&m_camPos), centerVec));
                float t;
                if (RayPlaneIntersect(rayOrigin, rayDir, centerVec, planeNormal, t)) {
                    XMVECTOR hitPoint = XMVectorAdd(rayOrigin, XMVectorScale(rayDir, t));
                    m_gizmoDragStartParam = XMVectorGetX(XMVector3Length(XMVectorSubtract(hitPoint, centerVec)));
                }
            } else {
                XMVECTOR axisDir = GizmoRenderer::axisDirectionVec(hoverAxis);
                if (m_gizmoMode == GizmoRenderer::Mode::Rotate) {
                    float t;
                    if (RayPlaneIntersect(rayOrigin, rayDir, centerVec, axisDir, t)) {
                        XMVECTOR hitPoint = XMVectorAdd(rayOrigin, XMVectorScale(rayDir, t));
                        m_gizmoDragStartAngleRad = GizmoAngleOnPlane(hitPoint, centerVec, hoverAxis);
                    }
                } else {
                    float lineT, rayT;
                    if (ClosestPointRayLine(rayOrigin, rayDir, centerVec, axisDir, lineT, rayT)) {
                        m_gizmoDragStartParam = lineT;
                    }
                }
            }
            gizmoConsumedClick = true;
        }

        // Continuar arrastre mientras LMB este sostenido; termina al soltar el boton.
        if (m_gizmoDragAxis != GizmoRenderer::Axis::None)
        {
            gizmoConsumedClick = true;
            if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                m_gizmoDragAxis = GizmoRenderer::Axis::None;
            }
            else if (rayValid && !selectedTransform.isNull()) {
                // Ancla FIJA capturada al iniciar el arrastre (m_gizmoDragStartPosition), no la
                // posicion actual/en vivo del actor: usar la posicion en vivo aqui reintroduce
                // el mismo objeto como origen de la parametrizacion en cada frame, y como esa
                // posicion cambia por el propio arrastre (en Translate), el "0" de la medicion
                // se desplaza junto con el objeto — cada frame sobrecorrige respecto al frame
                // anterior y el resultado son saltos/oscilaciones en vez de un movimiento suave.
                XMVECTOR centerVec = XMVectorSet(m_gizmoDragStartPosition.x, m_gizmoDragStartPosition.y,
                                                  m_gizmoDragStartPosition.z, 1.0f);

                if (m_gizmoDragAxis == GizmoRenderer::Axis::All) {
                    // Escala uniforme: razon (distancia actual / distancia inicial) al centro,
                    // medida sobre un plano que mira a la camara — aplicada por igual a X/Y/Z.
                    XMVECTOR planeNormal = XMVector3Normalize(XMVectorSubtract(XMLoadFloat3(&m_camPos), centerVec));
                    float t;
                    if (RayPlaneIntersect(rayOrigin, rayDir, centerVec, planeNormal, t) &&
                        m_gizmoDragStartParam > 1e-5f) {
                        XMVECTOR hitPoint = XMVectorAdd(rayOrigin, XMVectorScale(rayDir, t));
                        float distNow = XMVectorGetX(XMVector3Length(XMVectorSubtract(hitPoint, centerVec)));
                        float ratio = (std::max)(distNow / m_gizmoDragStartParam, 0.02f);
                        EU::Vector3 newScale(
                            (std::max)(0.01f, m_gizmoDragStartScale.x * ratio),
                            (std::max)(0.01f, m_gizmoDragStartScale.y * ratio),
                            (std::max)(0.01f, m_gizmoDragStartScale.z * ratio));
                        selectedTransform->setScale(newScale);
                    }
                } else {
                    XMVECTOR axisDir = GizmoRenderer::axisDirectionVec(m_gizmoDragAxis);

                    if (m_gizmoMode == GizmoRenderer::Mode::Rotate) {
                        float t;
                        if (RayPlaneIntersect(rayOrigin, rayDir, centerVec, axisDir, t)) {
                            XMVECTOR hitPoint = XMVectorAdd(rayOrigin, XMVectorScale(rayDir, t));
                            float angleNow = GizmoAngleOnPlane(hitPoint, centerVec, m_gizmoDragAxis);
                            float deltaAngle = angleNow - m_gizmoDragStartAngleRad;
                            EU::Vector3 newRot = m_gizmoDragStartRotation;
                            if (m_gizmoDragAxis == GizmoRenderer::Axis::X) newRot.x += deltaAngle;
                            else if (m_gizmoDragAxis == GizmoRenderer::Axis::Y) newRot.y += deltaAngle;
                            else newRot.z += deltaAngle;
                            selectedTransform->setRotation(newRot);
                        }
                    } else {
                        float lineT, rayT;
                        if (ClosestPointRayLine(rayOrigin, rayDir, centerVec, axisDir, lineT, rayT)) {
                            float delta = lineT - m_gizmoDragStartParam;
                            if (m_gizmoMode == GizmoRenderer::Mode::Translate) {
                                EU::Vector3 newPos = m_gizmoDragStartPosition;
                                if (m_gizmoDragAxis == GizmoRenderer::Axis::X) newPos.x += delta;
                                else if (m_gizmoDragAxis == GizmoRenderer::Axis::Y) newPos.y += delta;
                                else newPos.z += delta;
                                selectedTransform->setPosition(newPos);
                            } else { // Scale (por eje individual)
                                EU::Vector3 newScale = m_gizmoDragStartScale;
                                float* comp = (m_gizmoDragAxis == GizmoRenderer::Axis::X) ? &newScale.x
                                            : (m_gizmoDragAxis == GizmoRenderer::Axis::Y) ? &newScale.y
                                                                                           : &newScale.z;
                                *comp = (std::max)(0.01f, *comp + delta);
                                selectedTransform->setScale(newScale);
                            }
                        }
                    }
                }
            }
        }

        // PICKING (LMB) — raycast contra los AABB de los actores en escena.
        // Gateado por uiCapturaMouse (dentro del viewport), !orbitando (RMB no sostenido) y que
        // el click no haya sido consumido por el gizmo (hover/arrastre de un handle).
        if (!uiCapturaMouse && !orbitando && !gizmoConsumedClick && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
            if (!rayValid) {
                LOG_WARNING("BaseApp", "update", "Picking cancelado: rayo de mouse invalido (viewport degenerado o camara singular).");
            }
            else {
            int   hitIndex = -1;
            float closestT = FLT_MAX;

            for (int i = 0; i < (int)m_actors.size(); ++i)
            {
                auto& actor = m_actors[i];
                if (actor.isNull()) continue;

                // Light Actors no tienen mesh/AABB: se pickean por distancia rayo-punto contra
                // su posicion, con un radio escalado a la distancia de camara (constante en
                // pantalla, mismo patron que el picking del gizmo de escala uniforme).
                EU::TSharedPointer<LightComponent> lightComp = actor->getComponent<LightComponent>();
                if (!lightComp.isNull()) {
                    const EU::Vector3& lp = actor->getComponent<Transform>()->getPosition();
                    XMVECTOR lightPos = XMVectorSet(lp.x, lp.y, lp.z, 1.0f);
                    XMVECTOR toLight = XMVectorSubtract(lightPos, rayOrigin);
                    float tClosest = XMVectorGetX(XMVector3Dot(toLight, rayDir));
                    if (tClosest > 0.0f && tClosest < closestT) {
                        XMVECTOR closestOnRay = XMVectorAdd(rayOrigin, XMVectorScale(rayDir, tClosest));
                        float dist = XMVectorGetX(XMVector3Length(XMVectorSubtract(closestOnRay, lightPos)));
                        float distToCam = XMVectorGetX(XMVector3Length(XMVectorSubtract(lightPos, XMLoadFloat3(&m_camPos))));
                        float pickWorld = LightGizmoRenderer::kIconPickScale * distToCam;
                        if (dist < pickWorld) {
                            closestT = tClosest;
                            hitIndex = i;
                        }
                    }
                    continue;
                }

                if (!actor->hasBounds()) continue;

                XMMATRIX world = actor->getComponent<Transform>()->matrix;
                const XMFLOAT3& lmin = actor->getLocalBoundsMin();
                const XMFLOAT3& lmax = actor->getLocalBoundsMax();

                // El AABB local no es axis-aligned una vez transformado por World, asi que
                // se transforman las 8 esquinas y se toma la envolvente (nuevo AABB en mundo).
                XMFLOAT3 corners[8] = {
                    { lmin.x, lmin.y, lmin.z }, { lmax.x, lmin.y, lmin.z },
                    { lmin.x, lmax.y, lmin.z }, { lmax.x, lmax.y, lmin.z },
                    { lmin.x, lmin.y, lmax.z }, { lmax.x, lmin.y, lmax.z },
                    { lmin.x, lmax.y, lmax.z }, { lmax.x, lmax.y, lmax.z },
                };
                XMVECTOR wmin = XMVectorReplicate(FLT_MAX);
                XMVECTOR wmax = XMVectorReplicate(-FLT_MAX);
                for (const XMFLOAT3& c : corners) {
                    XMVECTOR wc = XMVector3Transform(XMLoadFloat3(&c), world);
                    wmin = XMVectorMin(wmin, wc);
                    wmax = XMVectorMax(wmax, wc);
                }

                XMFLOAT3 bmin, bmax;
                XMStoreFloat3(&bmin, wmin);
                XMStoreFloat3(&bmax, wmax);

                float t;
                if (RayIntersectsAABB(rayOrigin, rayDir, bmin, bmax, t) && t < closestT) {
                    closestT = t;
                    hitIndex = i;
                }
            }

            // Sin hit (espacio vacio) -> deselecciona (-1).
            m_userInterface.selectedActorIndex = hitIndex;
            }
        }
    }
    // ----------------------------------------------------

    // Subir CBPerFrame (b0): vista, proyeccion, camara
    // NOTA: m_cbPerFrame/m_cbPerFrameBuffer (y el computeLightViewProj de abajo) son parte del
    // camino legacy de m_forwardRenderer, que ya no se invoca en render() (solo se usa
    // m_deferredRenderer). LightDir/LightColor quedan en su valor de init() sin actualizarse
    // por Light Actor — el shadow pass REAL se calcula en DeferredRenderer::updateLightMatrices
    // a partir de RenderScene.lights, independiente de este bloque.
    XMStoreFloat4x4(&m_cbPerFrame.View,       XMMatrixTranspose(m_View));
    XMStoreFloat4x4(&m_cbPerFrame.Projection, XMMatrixTranspose(m_Projection));
    m_cbPerFrame.CameraPos = EU::Vector3(m_camPos.x, m_camPos.y, m_camPos.z);

    // Calcular LightViewProjection para el shadow pass
    {
        XMFLOAT3 sceneCenter = { 0.0f, -5.0f, 0.0f };
        float    sceneRadius = 20.0f;
        XMMATRIX lvp = m_forwardRenderer.computeLightViewProj(
            m_cbPerFrame.LightDir, sceneCenter, sceneRadius);
        XMStoreFloat4x4(&m_cbPerFrame.LightViewProjection, XMMatrixTranspose(lvp));
    }

    m_cbPerFrameBuffer.update(m_deviceContext, nullptr, 0, nullptr, &m_cbPerFrame, 0, 0);

    // Actores
    for (auto& a : m_actors)
        if (!a.isNull())
            a->update(t, m_deviceContext);
}

/**
 * @brief Renderiza la escena completa.
 *
 * Pasos:
 *  1) Limpia RTV/DSV y configura el viewport.
 *  2) Setea el pipeline (shaders, input layout) y sube constant buffers (b0/b1).
 *  3) Dibuja los actores.
 *  4) Renderiza la interfaz ImGui.
 *  5) Presenta el back buffer en pantalla.
 *
 * @note El orden es importante: primero 3D, luego UI.
 */
void BaseApp::render() {
    // --- Stats de frame (FPS / frame-time / draw calls) ---
    {
        static ULONGLONG lastFrameTickMs = 0;
        ULONGLONG nowMs = GetTickCount64();
        if (lastFrameTickMs == 0) lastFrameTickMs = nowMs;
        float frameMs = float(nowMs - lastFrameTickMs);
        lastFrameTickMs = nowMs;

        m_frameTimeMs = frameMs;
        float instantFps = frameMs > 0.0f ? 1000.0f / frameMs : 0.0f;
        // EMA para que el número no titile frame a frame
        m_fps = (m_fps == 0.0f) ? instantFps : (m_fps * 0.9f + instantFps * 0.1f);

        m_lastDrawCallCount = m_deviceContext.getDrawCallCount();
        m_deviceContext.resetDrawCallCount();
    }

    // 1) Construir RenderScene desde actores y luz del frame actual
    m_renderScene.clear();
    m_renderScene.skybox = &m_skybox; // no-owning; nullptr solo si Skybox::init() falló en init()

    // Luces: cualquier actor con LightComponent aporta una entrada (incluye el actor "Sun"
    // creado en init(), que reemplaza el antiguo sistema global de un solo color/direccion
    // fijo controlado por lightPanel()). position/direction se resuelven desde el Transform
    // de cada actor — ver LightComponent::resolve().
    for (auto& actor : m_actors) {
        if (actor.isNull()) continue;
        EU::TSharedPointer<LightComponent> lightComp = actor->getComponent<LightComponent>();
        if (lightComp.isNull()) continue;
        if ((int)m_renderScene.lights.size() >= kMaxSceneLights) break;
        m_renderScene.lights.push_back(lightComp->resolve(*actor->getComponent<Transform>()));
    }

    // Resaltado visual del actor seleccionado (picking del viewport): tinte calido aplicado
    // solo en el draw (RenderObject::tint, ver DeferredRenderer), nunca escrito en el
    // MaterialParams del MaterialInstance — asi el BaseColor que edita el Material Editor
    // persiste entre frames en vez de revertirse al tinte "normal" cada vez.
    Actor* selectedActorPtr = nullptr;
    if (m_userInterface.selectedActorIndex >= 0 &&
        m_userInterface.selectedActorIndex < (int)m_actors.size() &&
        !m_actors[m_userInterface.selectedActorIndex].isNull()) {
        selectedActorPtr = m_actors[m_userInterface.selectedActorIndex].get();
    }
    const XMFLOAT3 kNormalTint   = XMFLOAT3(1.0f, 1.0f, 1.0f);
    const XMFLOAT3 kSelectedTint = XMFLOAT3(1.6f, 1.3f, 0.3f); // realce calido/amarillo

    /**
     * @brief true si el material único del objeto, o alguno de sus materiales por submesh,
     * es `MaterialDomain::Transparent` — decide si el objeto también debe existir en
     * `transparentObjects` además de `opaqueObjects` (ver uso más abajo).
     * @note No es exclusivo de SciFiToad/Glass: cualquier material creado como
     * "Transparent" en el Material Editor y asignado a cualquier slot (Kirby, Plane, etc.)
     * necesita este mismo tratamiento para no volverse invisible.
     * @note [GameDev] Antes de esta función, solo el actor SciFiToad se agregaba a
     * `transparentObjects` (hardcodeado: `if (submeshMats) { ... }`), porque era el único
     * actor que originalmente tenía una parte transparente (el vidrio). Cuando el Material
     * Editor hizo posible asignar un material Transparent a CUALQUIER slot (Kirby, Plane),
     * ese hardcodeo dejó de alcanzar: el pase opaco salta cualquier submesh Transparent
     * (`if (domain == Transparent) continue;` en `DeferredRenderer::renderGeometryObject`),
     * así que un objeto que solo existe en `opaqueObjects` y nunca en `transparentObjects`
     * no se dibuja en NINGÚN pase — desaparece por completo. La lección: cuando una
     * condición hardcodeada para un caso ("si es SciFiToad") en realidad depende de una
     * propiedad más general ("si tiene un material Transparent"), conviene generalizarla en
     * cuanto un segundo caso la necesite, en vez de ir agregando `|| name == "OtroActor"`.
     */
    auto hasTransparentMaterial = [](const RenderObject& o) {
        auto isTransparent = [](MaterialInstance* mi) {
            return mi && mi->getMaterial() && mi->getMaterial()->getDomain() == MaterialDomain::Transparent;
        };
        if (isTransparent(o.materialInstance)) return true;
        for (MaterialInstance* mi : o.materialInstances) {
            if (isTransparent(mi)) return true;
        }
        return false;
    };

    // Plano — siempre usa m_APlane directamente (sin depender de su índice en m_actors)
    if (!m_APlane.isNull()) {
        RenderObject obj;
        obj.mesh             = &m_planeMeshGpu;
        obj.materialInstance = m_planeSlotMaterial;
        obj.world            = m_APlane->getComponent<Transform>()->matrix;
        obj.castShadow       = false;
        obj.tint             = (m_APlane.get() == selectedActorPtr) ? kSelectedTint : kNormalTint;
        m_renderScene.opaqueObjects.push_back(obj);

        // Ver nota junto a hasTransparentMaterial: el pass opaco salta materiales Transparent
        // (DeferredRenderer::renderGeometryObject), asi que sin esto el objeto desaparecia.
        if (hasTransparentMaterial(obj)) {
            RenderObject transparentObj = obj;
            transparentObj.transparent = true;
            EU::Vector3 actorPos = m_APlane->getComponent<Transform>()->getPosition();
            EU::Vector3 camPos   = m_camera.getPosition();
            transparentObj.distanceToCamera = (actorPos - camPos).magnitude();
            m_renderScene.transparentObjects.push_back(transparentObj);
        }
    }
    // Modelos FBX — buscamos por nombre para obtener la world matrix correcta
    for (auto& actor : m_actors) {
        if (actor.isNull()) continue;
        const std::string& name = actor->getName();
        Mesh* mesh = nullptr;
        MaterialInstance* mat = nullptr;
        std::vector<MaterialInstance*>* submeshMats = nullptr;
        if (name == "Kirby") {
            if (m_kirbyMesh.getSubmeshes().empty()) continue;
            mesh = &m_kirbyMesh;
            mat  = m_kirbySlotMaterial;
        } else if (name == "SciFiToad") {
            if (m_sciFiToadMesh.getSubmeshes().empty()) continue;
            mesh = &m_sciFiToadMesh;
            // Reconstruido cada frame (barato: unos pocos submeshes) a partir del bucket fijo
            // por nodo FBX y los tres *SlotMaterial, que el Material Editor puede reasignar.
            MaterialInstance* buckets[3] = { m_sciFiToadBodySlotMaterial, m_sciFiToadGlassSlotMaterial, m_sciFiToadHeadSlotMaterial };
            for (size_t i = 0; i < m_sciFiToadSubmeshMaterials.size(); ++i) {
                int bucket = (i < m_sciFiToadSubmeshBucket.size()) ? m_sciFiToadSubmeshBucket[i] : 0;
                m_sciFiToadSubmeshMaterials[i] = buckets[bucket >= 0 && bucket < 3 ? bucket : 0];
            }
            submeshMats = &m_sciFiToadSubmeshMaterials;
        } else {
            continue;
        }
        RenderObject obj;
        obj.mesh             = mesh;
        obj.materialInstance = mat;
        if (submeshMats) obj.materialInstances = *submeshMats;
        obj.world            = actor->getComponent<Transform>()->matrix;
        obj.castShadow       = true;
        obj.tint             = (actor.get() == selectedActorPtr) ? kSelectedTint : kNormalTint;
        m_renderScene.opaqueObjects.push_back(obj);

        // Ver nota junto a hasTransparentMaterial: cualquier submesh (o el material unico)
        // en MaterialDomain::Transparent — Glass por defecto, o cualquier material creado
        // como "Transparent" y asignado aqui desde el Material Editor — necesita que este
        // mismo RenderObject exista tambien en transparentObjects, o el pass opaco lo salta
        // (getDomain()==Transparent) y nunca llega a dibujarse en ningun pass.
        if (hasTransparentMaterial(obj)) {
            RenderObject transparentObj = obj;
            transparentObj.castShadow  = false; // lo transparente no proyecta sombra opaca
            transparentObj.transparent = true;
            EU::Vector3 actorPos = actor->getComponent<Transform>()->getPosition();
            EU::Vector3 camPos   = m_camera.getPosition();
            transparentObj.distanceToCamera = (actorPos - camPos).magnitude();
            m_renderScene.transparentObjects.push_back(transparentObj);
        }
    }

    // Log de primer frame para verificar el estado de la escena
    static bool s_loggedFirstFrame = false;
    if (!s_loggedFirstFrame) {
        s_loggedFirstFrame = true;
        LOG_MESSAGE("BaseApp", "render",
            "First frame - opaqueObjects=" + std::to_string((int)m_renderScene.opaqueObjects.size()) +
            " SRV=" + (m_editorViewportPass.getSRV() ? "OK" : "NULL") +
            " GBuf=" + (m_deferredRenderer.getGBufferAlbedoMetallicSRV() ? "OK" : "NULL") +
            " PlaneSubmesh=" + std::to_string((int)m_planeMeshGpu.getSubmeshes().size()) +
            " PlaneTex=" + (m_PlaneTexture.m_textureFromImg ? "OK" : "NULL") +
            " APlane=" + (!m_APlane.isNull() ? "OK" : "NULL"));
    }

    // 2) Pipeline Deferred: shadow → G-buffer → lighting → escribe en EditorViewportPass
    m_deferredRenderer.render(m_deviceContext, m_camera, m_renderScene, m_editorViewportPass);

    // 2b) Gizmos del editor (transformación + iconos de luces). Se dibujan mientras el RTV/DSV
    // de EditorViewportPass siguen enlazados (el DeferredRenderer no los desvincula al salir).
    XMMATRIX editorViewProj = m_camera.getView() * m_camera.getProj();

    // Gizmo de transformación del actor seleccionado — depth test deshabilitado para quedar
    // siempre encima de la escena.
    if (m_userInterface.selectedActorIndex >= 0 &&
        m_userInterface.selectedActorIndex < (int)m_actors.size() &&
        !m_actors[m_userInterface.selectedActorIndex].isNull())
    {
        const EU::Vector3& p = m_actors[m_userInterface.selectedActorIndex]->getComponent<Transform>()->getPosition();
        XMFLOAT3 gizmoCenter(p.x, p.y, p.z);
        m_gizmoRenderer.render(m_deviceContext, editorViewProj, gizmoCenter, m_gizmoScreenScale,
            m_gizmoMode, m_gizmoHoverAxis, m_gizmoDragAxis);
    }

    // Iconos de Light Actors (flecha/esfera/cono) — todas las luces de la escena, no solo la
    // seleccionada. Depth test normal (LESS): se ocultan detras de geometria opaca.
    {
        std::vector<LightGizmoRenderer::Instance> lightInstances;
        const XMFLOAT4 kNormalIconTint   = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
        const XMFLOAT4 kSelectedIconTint = XMFLOAT4(1.6f, 1.3f, 0.3f, 1.0f);
        for (int i = 0; i < (int)m_actors.size(); ++i) {
            auto& actor = m_actors[i];
            if (actor.isNull()) continue;
            EU::TSharedPointer<LightComponent> lc = actor->getComponent<LightComponent>();
            if (lc.isNull()) continue;

            LightData resolved = lc->resolve(*actor->getComponent<Transform>());
            LightGizmoRenderer::Instance inst{};
            inst.type          = resolved.type;
            inst.position      = XMFLOAT3(resolved.position.x, resolved.position.y, resolved.position.z);
            inst.direction     = XMFLOAT3(resolved.direction.x, resolved.direction.y, resolved.direction.z);
            inst.range         = (resolved.range > 0.0f) ? resolved.range : 5.0f;
            inst.spotAngleRad  = (resolved.spotAngle > 0.0f) ? resolved.spotAngle : XMConvertToRadians(30.0f);
            const XMFLOAT4& tint = (i == m_userInterface.selectedActorIndex) ? kSelectedIconTint : kNormalIconTint;
            inst.color = XMFLOAT4(resolved.color.x * tint.x, resolved.color.y * tint.y,
                                   resolved.color.z * tint.z, 1.0f);
            lightInstances.push_back(inst);
        }
        m_lightGizmoRenderer.render(m_deviceContext, editorViewProj, lightInstances);
    }

    // 3) Restaurar back buffer como destino de render
    m_renderTargetView.render(m_deviceContext, m_depthStencilView, 1, kClear);
    m_viewport.render(m_deviceContext);

    // 4) Ventana principal: resultado final del deferred renderer. Ancho calculado
    // DINÁMICAMENTE = ancho_pantalla - panel_izquierdo (Hierarchy/Inspector, kLeftPanelWidth)
    // - panel_derecho (G-Buffers, kRightPanelWidth): ocupa TODO el espacio central restante,
    // sin dejar huecos. Alto: hasta donde empieza la consola (kViewportHeightFrac), debajo de
    // la main menu bar (topMargin).
    const float topMargin = ImGui::GetFrameHeight();
    {
        ImGuiIO& io = ImGui::GetIO();
        const float vpX = kLeftPanelWidth + kPanelGap;
        const float vpW = io.DisplaySize.x - kLeftPanelWidth - kRightPanelWidth - 2.0f * kPanelGap;
        const float vpH = (io.DisplaySize.y - topMargin) * kViewportHeightFrac;
        ImGui::SetNextWindowPos(ImVec2(vpX, topMargin), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(vpW, vpH), ImGuiCond_Always);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::Begin("##DeferredViewport", nullptr,
            ImGuiWindowFlags_NoDecoration |
            ImGuiWindowFlags_NoBringToFrontOnFocus |
            ImGuiWindowFlags_NoMove);
        m_viewportHovered = ImGui::IsWindowHovered();

        // Rect REAL (screen space) de la imagen, medido aqui en vez de asumido (no siempre
        // coincide con el tamaño/posicion nominal de la ventana — p.ej. con viewports/docking
        // habilitado GetWindowPos() no es necesariamente (0,0), y GetContentRegionAvail()
        // puede diferir por redondeo durante un resize). El picking/gizmo en update() usa
        // estos mismos valores (con 1 frame de latencia, igual que m_viewportHovered).
        ImVec2 imgPos  = ImGui::GetCursorScreenPos();
        ImVec2 imgSize = ImGui::GetContentRegionAvail();
        m_viewportPos  = XMFLOAT2(imgPos.x, imgPos.y);
        m_viewportSize = XMFLOAT2(imgSize.x, imgSize.y);

        if (m_editorViewportPass.getSRV())
            ImGui::Image(reinterpret_cast<ImTextureID>(m_editorViewportPass.getSRV()), imgSize);
        ImGui::End();
        ImGui::PopStyleVar();
    }

    // 5) Paneles de G-Buffer — 4 ventanas separadas, apiladas verticalmente a la derecha del
    // viewport desde el arranque (posición fija calculada una vez por panel, vía
    // ImGuiCond_FirstUseEver: nacen ya acomodadas en su lugar, pero el usuario puede moverlas/
    // redimensionarlas/acoplarlas después con total libertad). Cada miniatura se escala
    // manteniendo proporción 16:9, con un checkbox propio para mostrar/ocultar su imagen.
    {
        static bool showAlbedo   = true;
        static bool showNormals  = true;
        static bool showWorldPos = true;
        static bool showEmissive = true;

        const float imgW = kRightPanelWidth - 16.0f; // margen interno de la ventana
        const float imgH = imgW * 9.0f / 16.0f;      // proporción 16:9
        const float panH = imgH + 44.0f;             // + header/checkbox/padding
        const float startX = ImGui::GetIO().DisplaySize.x - kRightPanelWidth;
        const float startY = topMargin;

        struct GBufPanel { const char* label; ID3D11ShaderResourceView* srv; bool* show; };
        GBufPanel panels[4] = {
            { "GB: Albedo+Metallic", m_deferredRenderer.getGBufferAlbedoMetallicSRV(),  &showAlbedo   },
            { "GB: Normal+Rough",    m_deferredRenderer.getGBufferNormalRoughnessSRV(), &showNormals  },
            { "GB: WorldPos+AO",     m_deferredRenderer.getGBufferWorldAoSRV(),         &showWorldPos },
            { "GB: Emissive+Alpha",  m_deferredRenderer.getGBufferEmissiveAlphaSRV(),   &showEmissive },
        };

        for (int i = 0; i < 4; ++i) {
            ImGui::SetNextWindowPos(ImVec2(startX, startY + i * (panH + kPanelGap)), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowSize(ImVec2(kRightPanelWidth, panH), ImGuiCond_FirstUseEver);
            ImGui::Begin(panels[i].label);
            ImGui::Checkbox("Show", panels[i].show);
            if (*panels[i].show) {
                if (panels[i].srv) {
                    // El alpha de estos MRT crudos es un parametro PBR (Metallic/Roughness/AO/
                    // Alpha-de-material), no transparencia — sin esto ImGui lo alpha-blendea
                    // contra el fondo de la ventana y canales con alpha bajo (ej. Metallic=0 en
                    // Albedo) se ven vacios/transparentes en vez de mostrar su color real.
                    ImDrawList* dl = ImGui::GetWindowDrawList();
                    dl->AddCallback(ForceOpaqueBlendCallback, m_deviceContext.m_deviceContext);
                    ImGui::Image(reinterpret_cast<ImTextureID>(panels[i].srv), ImVec2(imgW, imgH));
                    dl->AddCallback(ImDrawCallback_ResetRenderState, nullptr);
                } else {
                    ImGui::TextUnformatted("(no data)");
                }
            }
            ImGui::End();
        }
    }


    // 6) UI (outliner, inspector, light panel) + Present
    m_userInterface.render();
    m_swapChain.present();
}

/**
 * @brief Libera todos los recursos gráficos y de la escena.
 *
 * - Cierra ImGui correctamente (evita Live Objects).
 * - Limpia actores y sus recursos.
 * - Destruye buffers, shaders, texturas y vistas.
 * - Libera el estado del contexto y las referencias del dispositivo.
 *
 * @note Llamado al cerrar la aplicación (o si falla @ref init) para evitar fugas de memoria.
 */
void BaseApp::destroy() {
    // Cierra ImGui correctamente (evita Live Objects)
    m_userInterface.destroy();

    if (m_deviceContext.m_deviceContext)
        m_deviceContext.m_deviceContext->ClearState();

    for (auto& a : m_actors) if (!a.isNull()) a->destroy();
    m_actors.clear();

    // Deferred rendering resources
    m_deferredRenderer.destroy();
    m_editorViewportPass.destroy();
    m_kirbyMesh.destroy();
    m_kirbyAlbedoTex.destroy();
    m_defaultCheckerTexture.destroy();
    m_planeMeshGpu.destroy();

    m_forwardRenderer.destroy();
    m_gizmoRenderer.destroy();
    m_lightGizmoRenderer.destroy();
    m_skybox.destroy();
    m_cbPerFrameBuffer.destroy();
    m_shaderProgram.destroy();
    m_depthStencil.destroy();
    m_depthStencilView.destroy();
    m_renderTargetView.destroy();
    m_swapChain.destroy();

    if (m_deviceContext.m_deviceContext) m_deviceContext.m_deviceContext->Release();
    if (m_device.m_device)               m_device.m_device->Release();
}

/**
 * @brief Método principal que ejecuta el bucle de la aplicación.
 *
 * @param hInstance     Instancia de la aplicación.
 * @param hPrevInstance Instancia previa (no usada).
 * @param lpCmdLine     Línea de comandos.
 * @param nCmdShow      Estado de visualización de la ventana.
 * @param wndproc       Procedimiento de ventana Win32.
 *
 * @return Código de salida de la aplicación (valor de @c WM_QUIT).
 *
 * @details
 * - Inicializa la ventana y llama a @ref init.
 * - Entra en el bucle de mensajes Win32 y, cuando está libre de mensajes,
 *   ejecuta @ref update y @ref render.
 * - Al salir, llama a @ref destroy y retorna el código @c wParam del mensaje quit.
 */
int BaseApp::run(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow, WNDPROC wndproc) {
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    if (FAILED(m_window.init(hInstance, nCmdShow, wndproc)))
        return 0;

    if (FAILED(init())) {
        destroy();
        return 0;
    }

    MSG msg = { 0 };
    while (WM_QUIT != msg.message) {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else {
            update();
            render();
        }
    }

    destroy();
    return (int)msg.wParam;
}