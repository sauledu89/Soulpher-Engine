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
 */

#include "BaseApp.h"
#include "ECS/Transform.h"
#include "EngineUtilities/Vectors/Vector3.h"
#include "imgui.h"

 // Color de limpieza por defecto (RGBA)
static const float kClear[4] = { 0.0f, 0.125f, 0.30f, 1.0f };

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
 *  - Escena: carga modelo FBX (Martis Ashura King) + plano de referencia con textura.
 *  - Inicialización de ImGui.
 *
 * @return HRESULT Código de resultado:
 *  - @c S_OK en caso de éxito.
 *  - Código de error DirectX/Win32 en caso de fallo.
 */
HRESULT BaseApp::init()
{
    HRESULT hr = S_OK;

    // 1) SwapChain + Device + Context + BackBuffer (sin MSAA para evitar mismatches)
    hr = m_swapChain.init(m_device, m_deviceContext, m_backBuffer, m_window);
    if (FAILED(hr)) {
        ERROR("Main", "InitDevice", ("Failed to initialize SwapChain. hr=" + std::to_string(hr)).c_str());
        return hr;
    }

    // 2) RenderTargetView sobre el backbuffer
    hr = m_renderTargetView.init(m_device, m_backBuffer, DXGI_FORMAT_R8G8B8A8_UNORM);
    if (FAILED(hr)) {
        ERROR("Main", "InitDevice", ("Failed to initialize RenderTargetView. hr=" + std::to_string(hr)).c_str());
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
        ERROR("Main", "InitDevice", ("Failed to initialize DepthStencil texture. hr=" + std::to_string(hr)).c_str());
        return hr;
    }

    hr = m_depthStencilView.init(m_device, m_depthStencil, DXGI_FORMAT_D24_UNORM_S8_UINT);
    if (FAILED(hr)) {
        ERROR("Main", "InitDevice", ("Failed to initialize DepthStencilView. hr=" + std::to_string(hr)).c_str());
        return hr;
    }

    // 4) Viewport
    hr = m_viewport.init(m_window);
    if (FAILED(hr)) {
        ERROR("Main", "InitDevice", ("Failed to initialize Viewport. hr=" + std::to_string(hr)).c_str());
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
    hr = m_shaderProgram.init(m_device, "Soulpher-Engine.fx", layout);  // <- usa aquí el .fx real en tu bin
    if (FAILED(hr)) {
        ERROR("Main", "InitDevice", ("Failed to initialize ShaderProgram. hr=" + std::to_string(hr)).c_str());
        return hr;
    }

    // 7) Constant Buffer b0: CBPerFrame (View + Projection + luz)
    hr = m_cbPerFrameBuffer.init(m_device, sizeof(CBPerFrame));
    if (FAILED(hr)) {
        ERROR("Main", "InitDevice", ("Failed to create CBPerFrame buffer. hr=" + std::to_string(hr)).c_str());
        return hr;
    }

    // 8) Matrices iniciales de camara — update() las recalcula cada frame.
    {
        m_Projection = XMMatrixPerspectiveFovLH(
            XM_PIDIV4,
            m_window.m_width / (FLOAT)m_window.m_height,
            0.01f, 100.0f
        );
        // Vista inicial: update() la sobreescribira con la camara orbital.
        XMVECTOR Eye = XMVectorSet(0.0f, 3.0f, -10.0f, 0.0f);
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
        m_cbPerFrame.LightDir   = EU::Vector3(lx/len, ly/len, lz/len);
        m_cbPerFrame.LightColor = EU::Vector3(1.0f, 1.0f, 1.0f);
        m_cbPerFrame.ShadowBias = 0.003f;
    }

    // 10) Actor: Martis Ashura King (FBX) — fallo no fatal, continúa sin el modelo
    {
        auto martis = EU::MakeShared<Actor>(m_device);
        if (martis.isNull()) {
            MESSAGE("Main", "InitDevice", "Warning: Failed to create Martis Actor — skipping.");
        }
        else {
            const std::string kFBX =
                "ModelsFBX\\martis-ashura-king\\Martis\\hero_asura.fbx";

            if (!m_modelLoader.LoadFBXModel(kFBX) || m_modelLoader.meshes.empty()) {
                MESSAGE("Main", "InitDevice", ("Warning: FBX not found or empty: " + kFBX + " — skipping actor.").c_str());
            }
            else {
                // Malla(s)
                martis->setMesh(m_device, m_modelLoader.meshes);

                // Textura difusa principal (PNG). Intentamos axl_D y, si falla, axl_wq_D.
                Texture diffuse;
                HRESULT th = diffuse.init(
                    m_device,
                    "ModelsFBX\\martis-ashura-king\\Martis\\axl_D",
                    PNG);

                if (FAILED(th)) {
                    th = diffuse.init(
                        m_device,
                        "ModelsFBX\\martis-ashura-king\\Martis\\axl_wq_D",
                        PNG);
                }

                std::vector<Texture> mats;
                if (SUCCEEDED(th)) {
                    mats.push_back(diffuse);
                }
                else {
                    // Fallback a textura por defecto si existe
                    Texture fallback;
                    if (SUCCEEDED(fallback.init(m_device, "Textures\\Default", DDS)) ||
                        SUCCEEDED(fallback.init(m_device, "Textures\\Default", PNG))) {
                        mats.push_back(fallback);
                    }
                    else {
                        MESSAGE("Main", "InitDevice", "Warning: No diffuse texture found for Martis — rendering without texture.");
                    }
                }
                martis->setTextures(mats);

                // Transform (FBX suele venir en cm; escala típica)
                martis->getComponent<Transform>()->setTransform(
                    EU::Vector3(-0.50f, -5.00f, 0.00f), // Position
                    EU::Vector3(-1.50f,  0.00f, 0.00f), // Rotation (grados)
                    EU::Vector3( 5.00f,  5.00f, 5.00f)  // Scale
                );
                martis->setName("Martis Ashura King");
                martis->setCastShadow(true);

                m_actors.push_back(martis);
            }
        }
    }

    // 10) Actor: Kirby (FBX) — fallo no fatal, continúa sin el modelo
    {
        m_modelLoader.meshes.clear();

        auto kirby = EU::MakeShared<Actor>(m_device);
        if (kirby.isNull()) {
            MESSAGE("Main", "InitDevice", "Warning: Failed to create Kirby Actor — skipping.");
        }
        else {
            const std::string kFBX = "ModelsFBX\\kirby\\KirbyTest.fbx";

            if (!m_modelLoader.LoadFBXModel(kFBX) || m_modelLoader.meshes.empty()) {
                MESSAGE("Main", "InitDevice", ("Warning: FBX not found or empty: " + kFBX + " — skipping actor.").c_str());
            }
            else {
                kirby->setMesh(m_device, m_modelLoader.meshes);

                Texture kirbyDiffuse;
                HRESULT th = kirbyDiffuse.init(m_device, "ModelsFBX\\kirby\\baking", PNG);

                std::vector<Texture> mats;
                if (SUCCEEDED(th)) {
                    mats.push_back(kirbyDiffuse);
                }
                else {
                    Texture fallback;
                    if (SUCCEEDED(fallback.init(m_device, "Textures\\Default", DDS)) ||
                        SUCCEEDED(fallback.init(m_device, "Textures\\Default", PNG))) {
                        mats.push_back(fallback);
                    }
                    else {
                        MESSAGE("Main", "InitDevice", "Warning: No texture found for Kirby — rendering without texture.");
                    }
                }
                kirby->setTextures(mats);

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

    // 11) ACTOR: Plano simple (suelo con piedra.jpg)
    {
        const float kSize = 20.0f; // mitad del tamaño (=> 40x40)
        const float kTiling = 6.0f;  // repetición UV

        m_APlane = EU::MakeShared<Actor>(m_device);
        if (m_APlane.isNull()) {
            ERROR("Main", "InitDevice", "Failed to create Plane Actor.");
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

        // Textura del piso: ModelsFBX\piedra → fallback Textures\Default
        HRESULT hrTx = m_PlaneTexture.init(m_device, "ModelsFBX\\piedra", JPG);
        if (FAILED(hrTx)) {
            hrTx = m_PlaneTexture.init(m_device, "ModelsFBX\\piedra", PNG);
        }
        if (FAILED(hrTx)) {
            if (FAILED(m_PlaneTexture.init(m_device, "Textures\\Default", DDS)))
                m_PlaneTexture.init(m_device, "Textures\\Default", PNG);
        }

        std::vector<Texture> planeTextures{ m_PlaneTexture };
        m_APlane->setTextures(planeTextures);

        // Coloca el suelo a Y = -5 (donde tienes al personaje)
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

    // 12) Luz
    m_LightPos = XMFLOAT4(2.0f, 4.0f, -2.0f, 1.0f);

    // 13) ForwardRenderer (shadow map)
    hr = m_forwardRenderer.init(m_device);
    if (FAILED(hr)) {
        ERROR("Main", "InitDevice", ("Failed to initialize ForwardRenderer. hr=" + std::to_string(hr)).c_str());
        return hr;
    }

    // 14) ImGui (al final del init gráfico)
    m_userInterface.init(
        m_window.m_hWnd,
        m_device.m_device,
        m_deviceContext.m_deviceContext
    );

    return S_OK;
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
void BaseApp::update()
{
    // --- UI frame ---
    m_userInterface.update();

    // Panel de iluminación/sombras (modifica m_cbPerFrame.LightDir/Color y ShadowBias)
    m_userInterface.lightPanel(
        &m_cbPerFrame.LightDir.x,
        &m_cbPerFrame.LightColor.x,
        m_cbPerFrame.ShadowBias
    );

    // Inspector + Outliner
    if (!m_actors.empty())
    {
        if (m_userInterface.selectedActorIndex < 0 ||
            m_userInterface.selectedActorIndex >= (int)m_actors.size())
        {
            m_userInterface.selectedActorIndex = 0;
        }
        m_userInterface.inspectorGeneral(m_actors[m_userInterface.selectedActorIndex]);
    }
    m_userInterface.outliner(m_actors);

    // --- Tiempo ---
    static float t = 0.0f;
    static DWORD t0 = 0;
    DWORD tNow = GetTickCount();
    if (t0 == 0) t0 = tNow;
    t = (tNow - t0) / 1000.0f;

    // ----------------------------------------------------
    // CONTROLES DE CÁMARA (RMB orbitar, rueda zoom, MMB pan)
    // ----------------------------------------------------
    {
        ImGuiIO& io = ImGui::GetIO();
        bool uiCapturaMouse = io.WantCaptureMouse;

        static bool orbitando = false, paneando = false;
        static POINT ultimo{};

        if (!uiCapturaMouse)
        {
            // ORBIT (RMB)
            if (GetAsyncKeyState(VK_RBUTTON) & 0x8000)
            {
                POINT p; GetCursorPos(&p);
                if (!orbitando) { orbitando = true; ultimo = p; }
                float dx = float(p.x - ultimo.x);
                float dy = float(p.y - ultimo.y);
                m_camYawDeg += dx * 0.25f;
                m_camPitchDeg -= dy * 0.25f;
                m_camPitchDeg = std::clamp(m_camPitchDeg, -89.0f, 89.0f);
                ultimo = p;
            }
            else orbitando = false;

            // ZOOM (rueda)
            if (io.MouseWheel != 0.0f)
            {
                m_camDistance *= (io.MouseWheel > 0 ? 0.9f : 1.1f);
                m_camDistance = std::clamp(m_camDistance, 2.0f, 50.0f);
            }

            // PAN (MMB)
            if (GetAsyncKeyState(VK_MBUTTON) & 0x8000)
            {
                POINT p; GetCursorPos(&p);
                if (!paneando) { paneando = true; ultimo = p; }
                float dx = float(p.x - ultimo.x);
                float dy = float(p.y - ultimo.y);
                ultimo = p;

                float yaw = XMConvertToRadians(m_camYawDeg);
                float pitch = XMConvertToRadians(m_camPitchDeg);

                XMVECTOR forward = XMVectorSet(sinf(yaw) * cosf(pitch), sinf(pitch),
                    cosf(yaw) * cosf(pitch), 0);
                XMVECTOR right = XMVector3Normalize(XMVector3Cross(XMVectorSet(0, 1, 0, 0), forward));
                XMVECTOR up = XMVectorSet(0, 1, 0, 0);

                float panSpeed = m_camDistance * 0.0025f;
                XMVECTOR delta = -dx * panSpeed * right + dy * panSpeed * up;

                m_camTarget.x += XMVectorGetX(delta);
                m_camTarget.y += XMVectorGetY(delta);
                m_camTarget.z += XMVectorGetZ(delta);
            }
            else paneando = false;
        }

        // Recalcular la vista (m_View) con yaw/pitch/distancia/target
        {
            float yaw = XMConvertToRadians(m_camYawDeg);
            float pitch = XMConvertToRadians(m_camPitchDeg);
            float cy = cosf(yaw), sy = sinf(yaw);
            float cp = cosf(pitch), sp = sinf(pitch);

            XMVECTOR eye = XMVectorSet(m_camTarget.x + sy * cp * m_camDistance,
                m_camTarget.y + sp * m_camDistance,
                m_camTarget.z + cy * cp * m_camDistance, 1.0f);
            XMVECTOR at = XMVectorSet(m_camTarget.x, m_camTarget.y, m_camTarget.z, 1.0f);
            XMVECTOR up = XMVectorSet(0, 1, 0, 0);

            m_View = XMMatrixLookAtLH(eye, at, up);
            XMStoreFloat3(&m_camPos, eye);  // guarda posicion para CBPerFrame
        }
    }
    // ----------------------------------------------------

    // Subir CBPerFrame (b0): vista, proyeccion, camara
    // LightDir/LightColor/ShadowBias son controlados por lightPanel() — no sobreescribir aqui
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
    // 1) Bind CBPerFrame (b0) antes del shadow pass — el shadow VS necesita LightViewProjection
    m_cbPerFrameBuffer.render(m_deviceContext, 0, 1, true);

    // 2) Shadow depth pass: renderiza la escena desde el punto de vista de la luz
    m_forwardRenderer.renderShadowPass(m_deviceContext, m_actors);

    // 3) Restaurar render target principal y limpiar
    m_renderTargetView.render(m_deviceContext, m_depthStencilView, 1, kClear);
    m_viewport.render(m_deviceContext);
    m_depthStencilView.render(m_deviceContext);

    // 4) Pipeline de color principal
    m_shaderProgram.render(m_deviceContext);

    // 5) Re-enlazar CBPerFrame (b0) al pipeline de color (VS y PS lo necesitan)
    m_cbPerFrameBuffer.render(m_deviceContext, 0, 1, true);

    // 6) Enlazar shadow map en t6 antes del draw loop
    m_forwardRenderer.bindShadowMap(m_deviceContext);

    // 7) Dibujo de actores (color pass)
    for (auto& a : m_actors) if (!a.isNull()) a->render(m_deviceContext);

    // 8) Desenlazar shadow map (debe estar libre antes del shadow pass del siguiente frame)
    m_forwardRenderer.unbindShadowMap(m_deviceContext);

    // 9) UI + Present
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

    m_forwardRenderer.destroy();
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