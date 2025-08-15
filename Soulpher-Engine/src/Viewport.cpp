/**
 * @file Viewport.cpp
 * @brief Configuración y aplicación del viewport de render en DirectX 11.
 *
 * @details
 * El viewport define el área rectangular de la pantalla donde se dibujará
 * la escena 3D. Es uno de los pasos clave en la configuración del pipeline
 * gráfico, ya que:
 *  - Controla el tamaño de la imagen final en píxeles.
 *  - Permite renderizar a subregiones de la pantalla (por ejemplo, para split-screen).
 *  - Ajusta el mapeo de coordenadas normalizadas (NDC) al espacio de píxeles.
 *
 * @note
 * En DirectX 11, un viewport se aplica al **Rasterizer Stage**.
 * Si el viewport no coincide con el tamaño del render target, la imagen
 * se escalará, lo que puede afectar el rendimiento y la nitidez.
 *
 * @par Consejos para estudiantes:
 *  - Usar el tamaño de la ventana para render normal.
 *  - Cambiar el viewport permite efectos como mini-mapas o cámaras múltiples.
 *  - Es común reconfigurarlo si la ventana cambia de tamaño.
 */

#include "Viewport.h"
#include "Window.h"
#include "DeviceContext.h"

 /**
  * @brief Inicializa el viewport usando las dimensiones de una ventana.
  *
  * @param window Referencia a la ventana desde la que se tomarán ancho y alto.
  * @return HRESULT S_OK si se configura correctamente, código de error en caso contrario.
  *
  * @note Este método es útil para ajustar el viewport al tamaño exacto de la ventana,
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
 * @param width  Ancho en píxeles.
 * @param height Alto en píxeles.
 * @return HRESULT S_OK si se configura correctamente, código de error en caso contrario.
 *
 * @note Úsalo cuando quieras renderizar a un área específica distinta del tamaño de la ventana.
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
 * @brief Aplica el viewport al **Rasterizer Stage** del pipeline gráfico.
 *
 * @param deviceContext Contexto del dispositivo donde se aplicará el viewport.
 *
 * @note
 * Este método debe llamarse antes de renderizar cualquier geometría, para
 * asegurar que la imagen se dibuje en la región correcta.
 */
void Viewport::render(DeviceContext& deviceContext) {
    if (!deviceContext.m_deviceContext) {
        ERROR("Viewport", "render", "Device context is not set.");
        return;
    }
    deviceContext.RSSetViewports(1, &m_viewport);
}
