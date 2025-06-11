#pragma once
#include "Prerequisites.h"

//--------------------------------------------------------------------------------------
// Forward declarations
// Se declaran clases esenciales para evitar dependencias circulares.
//--------------------------------------------------------------------------------------
class Device;
class DeviceContext;
class Window;
class Texture;

/**
 * @file SwapChain.h
 * @class SwapChain
 * @brief Maneja la presentación de imágenes renderizadas en pantalla mediante buffers intercambiables.
 *
 * La clase SwapChain encapsula el uso de la interfaz IDXGISwapChain de DirectX 11. Esta interfaz
 * gestiona la cadena de intercambio (swap chain), que permite alternar entre un backbuffer
 * donde se dibuja y un frontbuffer que se muestra en pantalla. Este mecanismo evita parpadeos
 * y permite el uso de técnicas como el triple buffering o MSAA.
 */
class SwapChain {
public:
    /**
     * @brief Constructor por defecto.
     */
    SwapChain() = default;

    /**
     * @brief Destructor por defecto.
     */
    ~SwapChain() = default;

    /**
     * @brief Inicializa la swap chain con sus interfaces internas y configura MSAA.
     *
     * Este método conecta el dispositivo de Direct3D con la ventana nativa, creando una swap chain
     * que se usará para presentar cada frame. También obtiene el backbuffer como textura para usarlo
     * en el render.
     *
     * @param device Referencia al objeto Device (ID3D11Device).
     * @param deviceContext Contexto de dispositivo que se enlazará al swap.
     * @param backBuffer Textura que representa el backbuffer (render target).
     * @param window Ventana donde se mostrará el contenido renderizado.
     * @return HRESULT indicando si la inicialización fue exitosa (S_OK) o si falló.
     */
    HRESULT init(Device& device,
        DeviceContext& deviceContext,
        Texture& backBuffer,
        Window window);

    /**
     * @brief Método de actualización por cuadro. Actualmente no hace nada.
     */
    void update();

    /**
     * @brief Método de renderizado por cuadro. Actualmente no implementado.
     */
    void render();

    /**
     * @brief Libera todas las interfaces y recursos usados por la swap chain.
     */
    void destroy();

    /**
     * @brief Presenta el backbuffer en pantalla.
     *
     * Este método alterna el contenido del backbuffer al frontbuffer (swap),
     * mostrando el frame recién dibujado en la ventana. También puede sincronizarse
     * con el V-Sync, dependiendo de la configuración.
     */
    void present();

    // ----------------------------------------------------------------------------------
    // Recursos públicos (accesibles desde el exterior)
    // ----------------------------------------------------------------------------------

    /**
     * @brief Puntero principal a la swap chain de DXGI.
     */
    IDXGISwapChain* m_swapChain = nullptr;

    /**
     * @brief Tipo de driver usado (hardware, WARP o referencia).
     *
     * Por defecto se busca D3D_DRIVER_TYPE_HARDWARE, pero puede cambiar si no se soporta.
     */
    D3D_DRIVER_TYPE m_driverType = D3D_DRIVER_TYPE_NULL;

private:
    // ----------------------------------------------------------------------------------
    // Configuración interna y soporte
    // ----------------------------------------------------------------------------------

    /**
     * @brief Nivel de características de DirectX 11 aceptado por el sistema (como 11_0, 10_1...).
     */
    D3D_FEATURE_LEVEL m_featureLevel = D3D_FEATURE_LEVEL_11_0;

    /**
     * @brief Cantidad de muestras para antialiasing MSAA (Multi-Sample Anti-Aliasing).
     */
    unsigned int m_sampleCount;

    /**
     * @brief Niveles de calidad disponibles para MSAA en el hardware actual.
     */
    unsigned int m_qualityLevels;

    // ----------------------------------------------------------------------------------
    // Interfaces de DXGI necesarias para construir la swap chain correctamente
    // ----------------------------------------------------------------------------------

    /**
     * @brief Dispositivo DXGI que expone capacidades de intercambio de buffers.
     */
    IDXGIDevice* m_dxgiDevice = nullptr;

    /**
     * @brief Adaptador de hardware (tarjeta gráfica) usado por DXGI.
     */
    IDXGIAdapter* m_dxgiAdapter = nullptr;

    /**
     * @brief Fábrica de DXGI para crear la cadena de intercambio.
     */
    IDXGIFactory* m_dxgiFactory = nullptr;
};
