/**
 * @file SwapChain.h
 * @brief Encapsula la creación y gestión del swap chain en Direct3D 11.
 *
 * @details
 * El **Swap Chain** es un sistema de buffers que alterna entre:
 * - **Back Buffer**: Donde se dibuja la escena antes de mostrarla.
 * - **Front Buffer**: El que actualmente se muestra en pantalla.
 *
 * Este mecanismo permite **doble/triple buffering**, reduciendo el parpadeo
 * y mejorando la suavidad en el renderizado.
 *
 * 🔹 **Conceptos clave para estudiantes**:
 * - **Present()**: Envía el back buffer a la pantalla.
 * - **MSAA (Multi-Sample Anti-Aliasing)**: Mejora la calidad visual suavizando bordes.
 * - El Swap Chain se conecta a una **ventana** (HWND) y a un **back buffer** (textura).
 */

#pragma once
#include "Prerequisites.h"

class Device;
class DeviceContext;
class Window;
class Texture;

/**
 * @class SwapChain
 * @brief Maneja el intercambio de buffers (front/back) y la configuración de MSAA.
 *
 * @note Sin un swap chain, no habría forma eficiente de presentar imágenes
 * renderizadas en una ventana de Windows.
 */
class SwapChain {
public:
    /** @brief Constructor por defecto. */
    SwapChain() = default;

    /** @brief Destructor por defecto. */
    ~SwapChain() = default;

    /**
     * @brief Inicializa el swap chain.
     * @param device Dispositivo Direct3D 11 para la creación de recursos.
     * @param deviceContext Contexto de dispositivo para operaciones gráficas.
     * @param backBuffer Textura que actuará como back buffer.
     * @param window Ventana donde se presentará el contenido.
     * @return HRESULT indicando el resultado de la operación.
     *
     * @note También configura el **MSAA** y las propiedades de sincronización vertical (VSync).
     */
    HRESULT init(Device& device,
        DeviceContext& deviceContext,
        Texture& backBuffer,
        Window window);

    /** @brief Actualiza el estado del swap chain (en caso de cambios de ventana o configuración). */
    void update();

    /** @brief Lógica de renderización relacionada con el swap chain (opcional según implementación). */
    void render();

    /** @brief Libera los recursos asociados al swap chain. */
    void destroy();

    /**
     * @brief Presenta el back buffer en pantalla.
     *
     * @details
     * Este método **intercambia** el front y back buffer, mostrando el
     * contenido renderizado. Generalmente se llama al final de cada frame.
     */
    void present();

public:
    IDXGISwapChain* m_swapChain = nullptr; ///< Puntero al swap chain de DXGI.
    D3D_DRIVER_TYPE m_driverType = D3D_DRIVER_TYPE_NULL; ///< Tipo de driver utilizado (hardware, referencia, etc.).

private:
    D3D_FEATURE_LEVEL m_featureLevel = D3D_FEATURE_LEVEL_11_0; ///< Nivel de características de D3D.

    unsigned int m_sampleCount = 1;   ///< Número de muestras para MSAA.
    unsigned int m_qualityLevels = 0; ///< Niveles de calidad soportados para MSAA.

    // Punteros a interfaces DXGI usadas para crear el swap chain
    IDXGIDevice* m_dxgiDevice = nullptr;   ///< Dispositivo DXGI.
    IDXGIAdapter* m_dxgiAdapter = nullptr; ///< Adaptador DXGI (GPU).
    IDXGIFactory* m_dxgiFactory = nullptr; ///< Fábrica DXGI para crear recursos.
};
