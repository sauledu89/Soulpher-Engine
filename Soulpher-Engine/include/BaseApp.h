/**
 * @file BaseApp.h
 * @brief Clase principal de la aplicación.
 *
 * @details
 * `BaseApp` encapsula la inicialización, ciclo de vida y cierre de un motor basado en DirectX 11.
 * Sus responsabilidades incluyen:
 * - Creación de ventana y dispositivos gráficos.
 * - Configuración de swap chain, buffers y estados de renderizado.
 * - Carga de modelos, texturas y recursos iniciales de escena.
 * - Control del ciclo principal (game loop).
 * - Integración de la interfaz de usuario (`UserInterface`).
 *
 * @note Para estudiantes:
 * - Este es un ejemplo de **clase de alto nivel** que coordina múltiples subsistemas.
 * - La estructura sigue un patrón común en motores: `init()`, `update()`, `render()`, `destroy()`.
 * - Entender esta clase ayuda a comprender el flujo general de renderizado en DX11.
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
  * @brief Clase núcleo que inicializa y gestiona el motor DirectX 11, la escena y la interfaz de usuario.
  *
  * @details
  * Esta clase crea y mantiene todos los recursos principales:
  * - Dispositivo y contexto DirectX 11.
  * - Swap chain y buffers asociados.
  * - Shaders, constantes y matrices de cámara.
  * - Actores y plano de referencia.
  * - Interfaz de usuario basada en ImGui u otro sistema.
  *
  * Además, administra el ciclo de vida de la aplicación, desde `run()` hasta `destroy()`.
  */
class BaseApp {
public:
    /** @brief Constructor por defecto. */
    BaseApp() = default;

    /** @brief Destructor por defecto. */
    ~BaseApp() = default;

    /**
     * @brief Inicializa el motor y sus recursos gráficos.
     * @return `S_OK` si la inicialización fue exitosa; código de error en caso contrario.
     *
     * @note Configura el dispositivo, swap chain, render targets y recursos de escena.
     */
    HRESULT init();

    /**
     * @brief Actualiza la lógica de la aplicación por cuadro.
     *
     * @note Aquí se gestionan animaciones, entrada de usuario y lógica de juego.
     */
    void update();

    /**
     * @brief Renderiza la escena.
     *
     * @note Limpia el back buffer, dibuja actores, y presenta el frame en pantalla.
     */
    void render();

    /**
     * @brief Libera todos los recursos gráficos y de sistema.
     *
     * @note Llamar antes de cerrar la aplicación para evitar fugas de memoria.
     */
    void destroy();

    /**
     * @brief Ejecuta el ciclo principal de la aplicación.
     * @param hInstance Instancia de la aplicación.
     * @param hPrevInstance Instancia previa (no usada en Win32 moderno).
     * @param lpCmdLine Argumentos de línea de comandos.
     * @param nCmdShow Estado inicial de la ventana.
     * @param wndproc Procedimiento de ventana para manejar mensajes Win32.
     * @return Código de salida de la aplicación.
     *
     * @note Contiene el **game loop**: procesamiento de mensajes + llamadas a `update()` y `render()`.
     */
    int run(HINSTANCE hInstance,
        HINSTANCE hPrevInstance,
        LPWSTR lpCmdLine,
        int nCmdShow,
        WNDPROC wndproc);

private:
    // --- Núcleo DX11 ---
    Window          m_window;            ///< Ventana principal.
    Device          m_device;            ///< Dispositivo DirectX 11 (crea recursos GPU).
    DeviceContext   m_deviceContext;     ///< Contexto para enviar comandos a la GPU.
    SwapChain       m_swapChain;         ///< Intercambio de buffers para presentar en pantalla.

    // BackBuffer + RTV
    Texture         m_backBuffer;        ///< Textura del back buffer.
    RenderTargetView m_renderTargetView; ///< Vista de render para el back buffer.

    // Depth/Stencil
    Texture          m_depthStencil;     ///< Textura para pruebas de profundidad/stencil.
    DepthStencilView m_depthStencilView; ///< Vista asociada a la textura de profundidad.

    // Viewport y Shaders
    Viewport       m_viewport;           ///< Área de renderizado visible.
    ShaderProgram  m_shaderProgram;      ///< Programa de shaders principal.

    // Buffers constantes para cámara
    Buffer         m_neverChanges;       ///< Buffer constante (slot b0) con datos invariables.
    Buffer         m_changeOnResize;     ///< Buffer constante (slot b1) con datos que cambian al redimensionar.
    CBNeverChanges cbNeverChanges{};     ///< Datos de cámara/luz que casi no cambian.
    CBChangeOnResize cbChangesOnResize{};///< Datos que cambian cuando la ventana cambia de tamaño.

    // Matrices de cámara
    XMMATRIX       m_View;               ///< Matriz de vista (cámara).
    XMMATRIX       m_Projection;         ///< Matriz de proyección (perspectiva).

    // Color de limpieza del back buffer
    float          ClearColor[4] = { 0.0f, 0.125f, 0.3f, 1.0f };

    // Luz simple para iluminación básica
    XMFLOAT4       m_LightPos{ 2.0f, 4.0f, -2.0f, 1.0f };

    // Recursos
    ModelLoader    m_modelLoader;        ///< Cargador de modelos 3D.

    // Plano de referencia
    MeshComponent  planeMesh;            ///< Malla del plano base.
    Texture        m_PlaneTexture;       ///< Textura del plano.
    EU::TSharedPointer<Actor> m_APlane;  ///< Actor que representa el plano.

    // Interfaz de usuario y actores en escena
    UserInterface  m_userInterface;      ///< Interfaz de usuario.
    std::vector<EU::TSharedPointer<Actor>> m_actors; ///< Lista de actores en la escena.

    // Parámetros opcionales de cámara orbital
    float   m_camYawDeg = 0.0f;          ///< Ángulo de giro horizontal de cámara (grados).
    float   m_camPitchDeg = 15.0f;       ///< Ángulo de inclinación vertical de cámara (grados).
    float   m_camDistance = 10.0f;       ///< Distancia de la cámara al objetivo.
    XMFLOAT3 m_camTarget = XMFLOAT3(0.0f, -5.0f, 0.0f); ///< Punto objetivo de la cámara.
};
