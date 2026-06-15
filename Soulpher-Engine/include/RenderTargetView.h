/**
 * @file RenderTargetView.h
 * @brief Encapsula la vista de render target (RTV) de Direct3D 11.
 *
 * @details
 * Un Render Target View (RTV) es el enlace entre una textura y el Output Merger Stage:
 * le indica al pipeline "escribe los resultados del pixel shader en esta textura".
 * El RTV principal esta enlazado al back buffer del swap chain (lo que aparece en pantalla).
 * RTVs adicionales permiten Render-to-Texture (RTT): dibujar una escena completa a una
 * textura intermedia para postprocesado, reflexiones dinamicas o generacion de shadow maps.
 *
 * @note [GameDev] En Unreal Engine el equivalente es FRenderTarget y el sistema de
 * Render Passes. En Unity es RenderTexture. La idea clave es que el render no siempre
 * va a pantalla: puede ir a una textura que otro shader usa despues (ping-pong buffers,
 * bloom, DOF, SSR). Sin este mecanismo no existiria el postprocesado en tiempo real.
 *
 * @see Device, DepthStencilView, SwapChain, Texture
 */

#pragma once
#include "Prerequisites.h"

// Forward Declarations
class Device;
class DeviceContext;
class Texture;
class DepthStencilView;

/**
 * @class RenderTargetView
 * @brief Encapsula la creaci�n, gesti�n y uso de vistas de renderizado (RTV) en Direct3D 11.
 *
 * @details
 * Un **Render Target View** (RTV) es un objeto que le indica a Direct3D d�nde dibujar
 * el resultado final de la etapa de pixel shader. Normalmente:
 * - El RTV principal est� asociado al **back buffer** de la swap chain (lo que se ve en pantalla).
 * - Tambi�n se pueden crear RTV adicionales para renderizado a texturas (Render to Texture).
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
     * @param backBuffer Textura que act�a como back buffer (proviene de la swap chain).
     * @param Format Formato de p�xel a utilizar (por ejemplo DXGI_FORMAT_R8G8B8A8_UNORM).
     * @return HRESULT con el estado de la operaci�n.
     *
     * @note Este m�todo se usa t�picamente para la vista principal en pantalla.
     */
    HRESULT init(Device& device, Texture& backBuffer, DXGI_FORMAT Format);

    /**
     * @brief Inicializa la vista de render para una textura espec�fica.
     * @param device Referencia al dispositivo Direct3D.
     * @param inTex Textura de entrada.
     * @param ViewDimension Dimensi�n de la vista RTV (ej. `D3D11_RTV_DIMENSION_TEXTURE2D`).
     * @param Format Formato de p�xel a utilizar.
     * @return HRESULT con el estado de la operaci�n.
     *
     * @note Este m�todo es clave para renderizado fuera de pantalla (off-screen rendering).
     */
    HRESULT init(Device& device,
        Texture& inTex,
        D3D11_RTV_DIMENSION ViewDimension,
        DXGI_FORMAT Format);

    /**
     * @brief Actualiza el estado del Render Target View.
     *
     * @note �til si el RTV debe cambiar din�micamente (por ejemplo, al redimensionar ventana).
     */
    void update();

    /**
     * @brief Renderiza limpiando y estableciendo el RTV junto al Depth Stencil.
     * @param deviceContext Contexto del dispositivo.
     * @param depthStencilView Vista de profundidad asociada (DSV).
     * @param numViews N�mero de vistas a enlazar.
     * @param ClearColor Color con el que se limpiar� el RTV (RGBA).
     *
     * @note Este es el uso m�s com�n: limpiar y enlazar antes de dibujar un frame.
     */
    void render(DeviceContext& deviceContext,
        DepthStencilView& depthStencilView,
        unsigned int numViews,
        const float ClearColor[4]);

    /**
     * @brief Renderiza estableciendo �nicamente el RTV sin limpieza de color.
     * @param deviceContext Contexto del dispositivo.
     * @param numViews N�mero de vistas a enlazar.
     *
     * @note Se usa cuando queremos preservar el contenido previo del RTV.
     */
    void render(DeviceContext& deviceContext,
        unsigned int numViews);

    /**
     * @brief Libera los recursos asociados al Render Target View.
     *
     * @warning Siempre llamar a este m�todo antes de destruir el objeto
     *          para evitar fugas de memoria (memory leaks).
     */
    void destroy();

    /** @brief Devuelve el puntero nativo al Render Target View. */
    ID3D11RenderTargetView* get() const { return m_renderTargetView; }

private:
    ID3D11RenderTargetView* m_renderTargetView = nullptr; ///< Puntero a la vista de render D3D11.
};
