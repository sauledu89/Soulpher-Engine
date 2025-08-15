#pragma once
#include "Prerequisites.h"

// Forward Declarations
class Device;
class DeviceContext;
class Texture;
class DepthStencilView;

/**
 * @class RenderTargetView
 * @brief Encapsula la creación, gestión y uso de vistas de renderizado (RTV) en Direct3D 11.
 *
 * @details
 * Un **Render Target View** (RTV) es un objeto que le indica a Direct3D dónde dibujar
 * el resultado final de la etapa de pixel shader. Normalmente:
 * - El RTV principal está asociado al **back buffer** de la swap chain (lo que se ve en pantalla).
 * - También se pueden crear RTV adicionales para renderizado a texturas (Render to Texture).
 *
 * Esta clase permite:
 * - Crear un RTV desde el back buffer o desde una textura personalizada.
 * - Limpiar el contenido del RTV con un color.
 * - Enlazar el RTV al pipeline junto con una vista de profundidad (DSV).
 * - Liberar recursos cuando ya no son necesarios.
 *
 * @note En un motor de videojuegos, cambiar de RTV permite implementar efectos
 *       como renderizado diferido (deferred rendering), postprocesado o shadow maps.
 */
class RenderTargetView {
public:
    RenderTargetView() = default;  ///< Constructor por defecto.
    ~RenderTargetView() = default; ///< Destructor por defecto.

    /**
     * @brief Inicializa la vista de render a partir del back buffer.
     * @param device Referencia al dispositivo Direct3D.
     * @param backBuffer Textura que actúa como back buffer (proviene de la swap chain).
     * @param Format Formato de píxel a utilizar (por ejemplo DXGI_FORMAT_R8G8B8A8_UNORM).
     * @return HRESULT con el estado de la operación.
     *
     * @note Este método se usa típicamente para la vista principal en pantalla.
     */
    HRESULT init(Device& device, Texture& backBuffer, DXGI_FORMAT Format);

    /**
     * @brief Inicializa la vista de render para una textura específica.
     * @param device Referencia al dispositivo Direct3D.
     * @param inTex Textura de entrada.
     * @param ViewDimension Dimensión de la vista RTV (ej. `D3D11_RTV_DIMENSION_TEXTURE2D`).
     * @param Format Formato de píxel a utilizar.
     * @return HRESULT con el estado de la operación.
     *
     * @note Este método es clave para renderizado fuera de pantalla (off-screen rendering).
     */
    HRESULT init(Device& device,
        Texture& inTex,
        D3D11_RTV_DIMENSION ViewDimension,
        DXGI_FORMAT Format);

    /**
     * @brief Actualiza el estado del Render Target View.
     *
     * @note Útil si el RTV debe cambiar dinámicamente (por ejemplo, al redimensionar ventana).
     */
    void update();

    /**
     * @brief Renderiza limpiando y estableciendo el RTV junto al Depth Stencil.
     * @param deviceContext Contexto del dispositivo.
     * @param depthStencilView Vista de profundidad asociada (DSV).
     * @param numViews Número de vistas a enlazar.
     * @param ClearColor Color con el que se limpiará el RTV (RGBA).
     *
     * @note Este es el uso más común: limpiar y enlazar antes de dibujar un frame.
     */
    void render(DeviceContext& deviceContext,
        DepthStencilView& depthStencilView,
        unsigned int numViews,
        const float ClearColor[4]);

    /**
     * @brief Renderiza estableciendo únicamente el RTV sin limpieza de color.
     * @param deviceContext Contexto del dispositivo.
     * @param numViews Número de vistas a enlazar.
     *
     * @note Se usa cuando queremos preservar el contenido previo del RTV.
     */
    void render(DeviceContext& deviceContext,
        unsigned int numViews);

    /**
     * @brief Libera los recursos asociados al Render Target View.
     *
     * @warning Siempre llamar a este método antes de destruir el objeto
     *          para evitar fugas de memoria (memory leaks).
     */
    void destroy();

private:
    ID3D11RenderTargetView* m_renderTargetView = nullptr; ///< Puntero a la vista de render D3D11.
};
