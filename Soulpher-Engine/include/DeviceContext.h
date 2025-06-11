/**
 * @file DeviceContext.h
 * @brief Declaraci�n de la clase DeviceContext.
 */

#pragma once
#include "Prerequisites.h"

 /**
  * @class DeviceContext
  * @brief Encapsula el contexto de dispositivo de Direct3D 11.
  *
  * Esta clase representa el "command list" activo de Direct3D 11, utilizado para emitir
  * instrucciones de renderizado al pipeline gr�fico. Aqu� se establecen viewports,
  * buffers, estados de render y se limpian las superficies de dibujo.
  *
  * @note En motores gr�ficos, cada frame se construye a trav�s del contexto del dispositivo.
  * Controlar correctamente este objeto es clave para renderizado eficiente y correcto.
  */
class DeviceContext {
public:
    /**
     * @brief Constructor por defecto.
     */
    DeviceContext() = default;

    /**
     * @brief Destructor por defecto.
     */
    ~DeviceContext() = default;

    /**
     * @brief Inicializa el contexto de dispositivo (si aplica l�gica adicional).
     *
     * Generalmente se obtiene directamente del dispositivo, pero puede incluir
     * configuraci�n personalizada en implementaciones m�s avanzadas.
     */
    void init();

    /**
     * @brief Actualizaci�n por cuadro (sin uso por defecto).
     *
     * Se puede usar para l�gica de renderizado dependiente del tiempo
     * o sincronizaci�n con otros subsistemas.
     */
    void update();

    /**
     * @brief Punto de entrada del renderizado por frame.
     *
     * M�todo placeholder donde normalmente se emitir�an comandos de dibujo,
     * configuraciones del pipeline y presentaci�n del backbuffer.
     */
    void render();

    /**
     * @brief Libera todos los recursos asociados al contexto del dispositivo.
     *
     * Llama a `Release()` sobre la interfaz y deja el puntero en nulo.
     */
    void destroy();

    /**
     * @brief Establece una o varias regiones de la pantalla donde se puede renderizar.
     *
     * En Direct3D, un viewport define el �rea del render target a utilizar.
     * Se pueden usar m�ltiples viewports para renderizado dividido o efectos especiales.
     *
     * @param NumViewports Cantidad de viewports.
     * @param pViewports Puntero al arreglo de estructuras D3D11_VIEWPORT.
     */
    void RSSetViewports(unsigned int NumViewports, const D3D11_VIEWPORT* pViewports);

    /**
     * @brief Limpia un buffer de profundidad y stencil.
     *
     * Elimina los valores anteriores del z-buffer y del stencil buffer,
     * asegurando que el nuevo frame comience sin residuos visuales.
     *
     * @param pDepthStencilView Vista de profundidad/stencil a limpiar.
     * @param ClearFlags Indicadores: D3D11_CLEAR_DEPTH, D3D11_CLEAR_STENCIL o ambos.
     * @param Depth Valor de profundidad (normalmente 1.0f).
     * @param Stencil Valor de stencil (normalmente 0).
     *
     * @note Esta operaci�n debe realizarse antes de renderizar geometr�a 3D.
     */
    void ClearDepthStencilView(ID3D11DepthStencilView* pDepthStencilView,
        unsigned int ClearFlags,
        float Depth,
        UINT8 Stencil);

    /**
     * @brief Limpia una vista de render (como el backbuffer) con un color s�lido.
     *
     * Se llama normalmente al inicio de cada frame para preparar el buffer de dibujo.
     *
     * @param pRenderTargetView Vista de render target a limpiar.
     * @param ColorRGBA Color en formato RGBA (valores entre 0.0 y 1.0).
     *
     * @note Com�nmente se utiliza un color de fondo del juego o del editor.
     */
    void ClearRenderTargetView(ID3D11RenderTargetView* pRenderTargetView,
        const float ColorRGBA[4]);

    /**
     * @brief Asocia vistas de render y vista de profundidad al pipeline.
     *
     * Permite que el pipeline gr�fico sepa a d�nde enviar los resultados
     * del render actual (color y profundidad).
     *
     * @param NumViews N�mero de vistas de render.
     * @param ppRenderTargetViews Arreglo de punteros a vistas de color.
     * @param pDepthStencilView Vista de profundidad/stencil.
     *
     * @note Esta configuraci�n debe hacerse antes de cualquier dibujo.
     */
    void OMSetRenderTargets(unsigned int NumViews,
        ID3D11RenderTargetView* const* ppRenderTargetViews,
        ID3D11DepthStencilView* pDepthStencilView);

public:
    /**
     * @brief Contexto de dispositivo de Direct3D 11.
     *
     * Esta interfaz permite emitir comandos al pipeline gr�fico:
     * dibujar geometr�a, limpiar buffers, cambiar estados, etc.
     *
     * @note El contexto es vital para construir y ejecutar cada frame del motor.
     */
    ID3D11DeviceContext* m_deviceContext = nullptr;
};
