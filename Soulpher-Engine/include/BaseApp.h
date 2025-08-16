/**
 * @file BaseApp.h
 * @brief Núcleo principal de la aplicación y gestor de la escena.
 *
 * @details
 * Inicializa y gestiona el núcleo de DirectX 11, crea la ventana y el dispositivo,
 * configura el swap chain, RTV/DSV y viewport; compila/carga shaders y buffers
 * de cámara; y prepara la escena inicial con:
 *  - Un plano de referencia con textura.
 *  - Un Actor cargado desde FBX (actualmente *Martis Ashura King*), con
 *    malla en:
 *      bin/ModelsFBX/martis-ashura-king/Martis/hero_asura.fbx
 *    y textura difusa en:
 *      bin/ModelsFBX/martis-ashura-king/Martis/axl_D.png
 *    (alternativa: axl_wq_D.png en la misma carpeta).
 *
 * Las rutas de assets se tratan como *relativas al ejecutable* ubicado en bin/.
 */

#pragma once
#include "Prerequisites.h"

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

    // CBuffers de cámara
    Buffer           m_neverChanges;       ///< Buffer constante slot b0 (parámetros estáticos de cámara).
    Buffer           m_changeOnResize;     ///< Buffer constante slot b1 (parámetros que cambian con el tamaño de ventana).
    CBNeverChanges   cbNeverChanges{};     ///< Datos que casi no cambian (posición de luz, etc.).
    CBChangeOnResize cbChangesOnResize{};  ///< Datos que cambian al redimensionar.

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

    // Parámetros de cámara orbital
    float    m_camYawDeg = 0.0f;          ///< Ángulo de giro horizontal.
    float    m_camPitchDeg = 15.0f;       ///< Ángulo de inclinación vertical.
    float    m_camDistance = 10.0f;       ///< Distancia de la cámara al objetivo.
    XMFLOAT3 m_camTarget = XMFLOAT3(0.0f, -5.0f, 0.0f); ///< Punto de enfoque de la cámara.
};
