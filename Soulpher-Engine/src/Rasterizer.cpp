/**
 * @file Rasterizer.cpp
 * @brief Implementación del estado de rasterización en Direct3D 11.
 *
 * @details
 * El rasterizador define cómo se transforman las primitivas (triángulos, líneas, puntos)
 * en fragmentos (pixeles) antes del proceso de renderizado final.
 * Controla aspectos como:
 *  - Modo de llenado (wireframe o sólido).
 *  - Dirección del culling (cara frontal o trasera).
 *  - Habilitación de clipping por profundidad.
 *
 * @note
 * Comprender el rasterizador es fundamental en el desarrollo de videojuegos,
 * ya que optimiza el renderizado al descartar geometría no visible
 * y controla la estética visual (por ejemplo, depuración en modo wireframe).
 */

#include "Rasterizer.h"
#include "Device.h"
#include "DeviceContext.h"

 /**
  * @brief Inicializa el estado de rasterización.
  *
  * @param device Referencia al dispositivo Direct3D 11 que se usará para crear el estado.
  * @return HRESULT Código de resultado de Direct3D:
  *         - S_OK si se creó correctamente.
  *         - Error específico si falla la creación.
  *
  * @note
  * En este caso:
  *  - FillMode está configurado como sólido (`D3D11_FILL_SOLID`), ideal para renderizado final.
  *  - CullMode descarta caras traseras (`D3D11_CULL_BACK`) para optimizar.
  *  - DepthClipEnable está activo para evitar dibujar objetos fuera del rango de profundidad.
  *
  * @warning
  * Cambiar FillMode a `D3D11_FILL_WIREFRAME` es útil para depurar geometría,
  * pero puede reducir la inmersión visual en el producto final.
  */
HRESULT Rasterizer::init(Device device) {
    D3D11_RASTERIZER_DESC rasterizerDesc = {};
    rasterizerDesc.FillMode = D3D11_FILL_SOLID;           ///< Relleno sólido (wireframe para depuración).
    rasterizerDesc.CullMode = D3D11_CULL_BACK;            ///< Culling de caras traseras.
    rasterizerDesc.FrontCounterClockwise = false;         ///< Orientación de vértices: horario = cara frontal.
    rasterizerDesc.DepthBias = 0;
    rasterizerDesc.SlopeScaledDepthBias = 0.0f;
    rasterizerDesc.DepthBiasClamp = 0.0f;
    rasterizerDesc.DepthClipEnable = true;                ///< Habilita el recorte por profundidad.
    rasterizerDesc.ScissorEnable = false;
    rasterizerDesc.MultisampleEnable = false;
    rasterizerDesc.AntialiasedLineEnable = false;

    HRESULT hr = device.CreateRasterizerState(&rasterizerDesc, &m_rasterizerState);

    if (FAILED(hr)) {
        ERROR("Rasterizer", "init", "CHECK FOR CreateRasterizerState()");
    }
    return hr;
}

/**
 * @brief Actualiza el estado del rasterizador.
 *
 * @note
 * Actualmente vacío. Puede usarse para alternar entre modos de renderizado
 * (ej. wireframe ↔ sólido) en tiempo real.
 */
void Rasterizer::update() {
}

/**
 * @brief Aplica el estado de rasterización al pipeline de render.
 *
 * @param deviceContext Referencia al contexto del dispositivo.
 *
 * @note
 * Debe llamarse antes de dibujar cualquier geometría para asegurar
 * que el rasterizador correcto esté activo.
 */
void Rasterizer::render(DeviceContext& deviceContext) {
    deviceContext.RSSetState(m_rasterizerState);
}

/**
 * @brief Libera los recursos asociados al rasterizador.
 *
 * @note
 * Es importante liberar el estado para evitar fugas de memoria
 * al cerrar el motor o cambiar de escena.
 */
void Rasterizer::destroy() {
    SAFE_RELEASE(m_rasterizerState);
}
