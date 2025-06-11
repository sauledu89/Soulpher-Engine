#pragma once
#include "Prerequisites.h"

/**
 * @file RenderTargetView.h
 * @brief Declaración de la clase RenderTargetView.
 *
 * Esta clase encapsula la creación, uso y destrucción de una
 * Render Target View (RTV) en DirectX 11. Una RTV permite que una
 * textura actúe como objetivo de renderizado, ya sea el backbuffer
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
 * Una RTV representa el destino donde se dibujan los píxeles renderizados.
 * Se asocia típicamente con el backbuffer de la swap chain o una textura.
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
     * Este método crea una vista de renderizado a partir del buffer de presentación.
     *
     * @param device Dispositivo de Direct3D utilizado para la creación.
     * @param backBuffer Textura asociada al backbuffer.
     * @param Format Formato de color de la vista (ej. DXGI_FORMAT_R8G8B8A8_UNORM).
     * @return HRESULT Código de éxito o error.
     */
    HRESULT init(Device& device, Texture& backBuffer, DXGI_FORMAT Format);

    /**
     * @brief Inicializa la RTV a partir de una textura personalizada.
     *
     * Este método se usa cuando se quiere renderizar a una textura diferente del backbuffer.
     *
     * @param device Dispositivo de Direct3D.
     * @param inTex Textura sobre la que se creará la RTV.
     * @param ViewDimension Tipo de vista (ej. D3D11_RTV_DIMENSION_TEXTURE2D).
     * @param Format Formato de color de la vista.
     * @return HRESULT Código de éxito o error.
     */
    HRESULT init(Device& device,
        Texture& inTex,
        D3D11_RTV_DIMENSION ViewDimension,
        DXGI_FORMAT Format);

    /**
     * @brief Método de actualización (actualmente sin implementación).
     *
     * Puede emplearse en el futuro para cambiar parámetros de renderizado dinámicamente.
     */
    void update();

    /**
     * @brief Enlaza esta RTV al pipeline gráfico junto con una vista de profundidad.
     *
     * Además de establecer las vistas, limpia el render target con el color indicado.
     *
     * @param deviceContext Contexto del dispositivo (para enviar comandos al pipeline).
     * @param depthStencilView Vista de profundidad/stencil a usar con esta RTV.
     * @param numViews Número de vistas de render (usualmente 1).
     * @param ClearColor Color de limpieza (formato RGBA).
     */
    void render(DeviceContext& deviceContext,
        DepthStencilView& depthStencilView,
        unsigned int numViews,
        const float ClearColor[4]);

    /**
     * @brief Enlaza la RTV sin limpiar el color ni usar profundidad.
     *
     * Este método es útil para operaciones como post-procesado o render targets auxiliares.
     *
     * @param deviceContext Contexto del dispositivo.
     * @param numViews Número de vistas a establecer.
     */
    void render(DeviceContext& deviceContext,
        unsigned int numViews);

    /**
     * @brief Libera la memoria ocupada por la RTV.
     *
     * Es importante llamar a este método para evitar fugas de memoria.
     */
    void destroy();

public:
    /**
     * @brief Puntero al recurso interno de DirectX para render target view.
     *
     * Representa la interfaz ID3D11RenderTargetView que se enlaza al pipeline de gráficos.
     */
    ID3D11RenderTargetView* m_renderTargetView = nullptr;
};
