#pragma once
#include "Prerequisites.h"

/**
 * @file RenderTargetView.h
 * @brief Declaraci�n de la clase RenderTargetView.
 *
 * Esta clase encapsula la creaci�n, uso y destrucci�n de una
 * Render Target View (RTV) en DirectX 11. Una RTV permite que una
 * textura act�e como objetivo de renderizado, ya sea el backbuffer
 * o una textura personalizada.
 */

 // Forward declarations para evitar dependencias innecesarias
class Device;
class DeviceContext;
class Texture;
class DepthStencilView;

/**
 * @class RenderTargetView
 * @brief Vista de renderizado que permite a DirectX dibujar sobre una textura.
 *
 * Una RTV representa el destino donde se dibujan los p�xeles renderizados.
 * Se asocia t�picamente con el backbuffer de la swap chain o una textura.
 */
class RenderTargetView {
public:
    /**
     * @brief Constructor por defecto.
     */
    RenderTargetView() = default;

    /**
     * @brief Destructor por defecto.
     */
    ~RenderTargetView() = default;

    /**
     * @brief Inicializa la RTV usando el backbuffer de la swap chain.
     *
     * Este m�todo crea una vista de renderizado a partir del buffer de presentaci�n.
     *
     * @param device Dispositivo de Direct3D utilizado para la creaci�n.
     * @param backBuffer Textura asociada al backbuffer.
     * @param Format Formato de color de la vista (ej. DXGI_FORMAT_R8G8B8A8_UNORM).
     * @return HRESULT C�digo de �xito o error.
     */
    HRESULT init(Device& device, Texture& backBuffer, DXGI_FORMAT Format);

    /**
     * @brief Inicializa la RTV a partir de una textura personalizada.
     *
     * Este m�todo se usa cuando se quiere renderizar a una textura diferente del backbuffer.
     *
     * @param device Dispositivo de Direct3D.
     * @param inTex Textura sobre la que se crear� la RTV.
     * @param ViewDimension Tipo de vista (ej. D3D11_RTV_DIMENSION_TEXTURE2D).
     * @param Format Formato de color de la vista.
     * @return HRESULT C�digo de �xito o error.
     */
    HRESULT init(Device& device,
        Texture& inTex,
        D3D11_RTV_DIMENSION ViewDimension,
        DXGI_FORMAT Format);

    /**
     * @brief M�todo de actualizaci�n (actualmente sin implementaci�n).
     *
     * Puede emplearse en el futuro para cambiar par�metros de renderizado din�micamente.
     */
    void update();

    /**
     * @brief Enlaza esta RTV al pipeline gr�fico junto con una vista de profundidad.
     *
     * Adem�s de establecer las vistas, limpia el render target con el color indicado.
     *
     * @param deviceContext Contexto del dispositivo (para enviar comandos al pipeline).
     * @param depthStencilView Vista de profundidad/stencil a usar con esta RTV.
     * @param numViews N�mero de vistas de render (usualmente 1).
     * @param ClearColor Color de limpieza (formato RGBA).
     */
    void render(DeviceContext& deviceContext,
        DepthStencilView& depthStencilView,
        unsigned int numViews,
        const float ClearColor[4]);

    /**
     * @brief Enlaza la RTV sin limpiar el color ni usar profundidad.
     *
     * Este m�todo es �til para operaciones como post-procesado o render targets auxiliares.
     *
     * @param deviceContext Contexto del dispositivo.
     * @param numViews N�mero de vistas a establecer.
     */
    void render(DeviceContext& deviceContext,
        unsigned int numViews);

    /**
     * @brief Libera la memoria ocupada por la RTV.
     *
     * Es importante llamar a este m�todo para evitar fugas de memoria.
     */
    void destroy();

public:
    /**
     * @brief Puntero al recurso interno de DirectX para render target view.
     *
     * Representa la interfaz ID3D11RenderTargetView que se enlaza al pipeline de gr�ficos.
     */
    ID3D11RenderTargetView* m_renderTargetView = nullptr;
};
