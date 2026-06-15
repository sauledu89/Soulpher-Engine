/**
 * @file Viewport.cpp
 * @brief Configuraci�n y aplicaci�n del viewport de render en DirectX 11.
 *
 * @details
 * El viewport define el �rea rectangular de la pantalla donde se dibujar�
 * la escena 3D. Es uno de los pasos clave en la configuraci�n del pipeline
 * gr�fico, ya que:
 *  - Controla el tama�o de la imagen final en p�xeles.
 *  - Permite renderizar a subregiones de la pantalla (por ejemplo, para split-screen).
 *  - Ajusta el mapeo de coordenadas normalizadas (NDC) al espacio de p�xeles.
 *
 * @note
 * En DirectX 11, un viewport se aplica al **Rasterizer Stage**.
 * Si el viewport no coincide con el tama�o del render target, la imagen
 * se escalar�, lo que puede afectar el rendimiento y la nitidez.
 *
 * @par Consejos para estudiantes:
 *  - Usar el tama�o de la ventana para render normal.
 *  - Cambiar el viewport permite efectos como mini-mapas o c�maras m�ltiples.
 *  - Es com�n reconfigurarlo si la ventana cambia de tama�o.
 *
 * @note [GameDev] El viewport define la transformacion de Normalized Device Coordinates (NDC)
 * al espacio de pantalla (pixels). NDC va de [-1,1] en X e Y; el viewport escala eso
 * a [TopLeftX, TopLeftX+Width] x [TopLeftY, TopLeftY+Height].
 * MinDepth=0 y MaxDepth=1 definen el rango del depth buffer: 0.0 = near, 1.0 = far.
 * Tener multiples viewports permite split-screen, rear-view mirrors o debug overlays
 * del shadow map. ForwardRenderer usa un viewport fijo de 2048x2048 para el shadow pass.
 */

#include "Viewport.h"
#include "Window.h"
#include "DeviceContext.h"

 /**
  * @brief Inicializa el viewport usando las dimensiones de una ventana.
  *
  * @param window Referencia a la ventana desde la que se tomar�n ancho y alto.
  * @return HRESULT S_OK si se configura correctamente, c�digo de error en caso contrario.
  *
  * @note Este m�todo es �til para ajustar el viewport al tama�o exacto de la ventana,
  *       asegurando que la escena se renderice a pantalla completa.
  */
HRESULT
Viewport::init(const Window& window) {
    if (!window.m_hWnd) {
        ERROR("Viewport", "init", "Window handle (m_hWnd) is nullptr");
        return E_POINTER;
    }
    if (window.m_width == 0 || window.m_height == 0) {
        ERROR("Viewport", "init", "Window dimensions are zero.");
        return E_INVALIDARG;
    }

    m_viewport.Width = static_cast<float>(window.m_width);
    m_viewport.Height = static_cast<float>(window.m_height);
    m_viewport.MinDepth = 0.0f;
    m_viewport.MaxDepth = 1.0f;
    m_viewport.TopLeftX = 0;
    m_viewport.TopLeftY = 0;

    return S_OK;
}

/**
 * @brief Inicializa el viewport con dimensiones personalizadas.
 *
 * @param width  Ancho en p�xeles.
 * @param height Alto en p�xeles.
 * @return HRESULT S_OK si se configura correctamente, c�digo de error en caso contrario.
 *
 * @note �salo cuando quieras renderizar a un �rea espec�fica distinta del tama�o de la ventana.
 */
HRESULT
Viewport::init(unsigned int width, unsigned int height) {
    if (width == 0 || height == 0) {
        ERROR("Viewport", "init", "Window dimensions are zero.");
        return E_INVALIDARG;
    }

    m_viewport.Width = static_cast<float>(width);
    m_viewport.Height = static_cast<float>(height);
    m_viewport.MinDepth = 0.0f;
    m_viewport.MaxDepth = 1.0f;
    m_viewport.TopLeftX = 0;
    m_viewport.TopLeftY = 0;

    return S_OK;
}

/**
 * @brief Aplica el viewport al **Rasterizer Stage** del pipeline gr�fico.
 *
 * @param deviceContext Contexto del dispositivo donde se aplicar� el viewport.
 *
 * @note
 * Este m�todo debe llamarse antes de renderizar cualquier geometr�a, para
 * asegurar que la imagen se dibuje en la regi�n correcta.
 */
void Viewport::render(DeviceContext& deviceContext) {
    if (!deviceContext.m_deviceContext) {
        ERROR("Viewport", "render", "Device context is not set.");
        return;
    }
    deviceContext.RSSetViewports(1, &m_viewport);
}
