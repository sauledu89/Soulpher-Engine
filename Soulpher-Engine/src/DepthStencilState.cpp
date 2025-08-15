/**
 * @file DepthStencilState.cpp
 * @brief Configuración y uso del estado de profundidad y stencil en D3D11.
 *
 * @details
 * Este módulo crea y aplica un estado de **Depth/Stencil** típico:
 * - Depth test activable/desactivable (función: LESS).
 * - Escritura en profundidad habilitada.
 * - Stencil opcional, con operaciones por defecto para caras frontal/trasera.
 *
 * 🔹 Para estudiantes:
 * - Profundidad evita que se dibujen píxeles “detrás” de otros.
 * - Stencil permite efectos como espejos, outlines y máscaras.
 * - `OMSetDepthStencilState` establece el estado en la etapa Output Merger.
 */

#include "DepthStencilState.h"
#include "Device.h"
#include "DeviceContext.h"

 /**
  * @brief Inicializa el estado de profundidad/stencil.
  * @param device        Dispositivo D3D11.
  * @param enableDepth   Activa/desactiva la prueba de profundidad.
  * @param enableStencil Activa/desactiva la prueba de stencil.
  * @return HRESULT S_OK si ok, o código de error en fallo.
  *
  * @note
  * - DepthFunc = LESS: el fragmento se dibuja si su Z es menor (más cerca).
  * - Stencil por defecto con KEEP/INCR/DECR y ALWAYS como comparación.
  */
HRESULT DepthStencilState::init(Device& device, bool enableDepth, bool enableStencil) {
    if (!device.m_device) {
        ERROR("DepthStencilState", "init", "Device is null.");
        return E_POINTER;
    }

    D3D11_DEPTH_STENCIL_DESC desc = {};
    desc.DepthEnable = enableDepth;
    desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    desc.DepthFunc = D3D11_COMPARISON_LESS;

    desc.StencilEnable = enableStencil;
    desc.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
    desc.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;

    // Caras frontales
    desc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
    desc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
    desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
    desc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

    // Caras traseras
    desc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
    desc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
    desc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
    desc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

    HRESULT hr = device.CreateDepthStencilState(&desc, &m_depthStencilState);
    if (FAILED(hr)) {
        ERROR("DepthStencilState", "init", "Failed to create DepthStencilState");
        return hr; // <-- devolver el error real
    }
    return S_OK;
}

/**
 * @brief (Reservado) Punto para actualizar parámetros si fuera necesario.
 */
void DepthStencilState::update() {
    // Sin actualización dinámica en esta implementación
}

/**
 * @brief Aplica o resetea el estado en la etapa Output Merger.
 * @param deviceContext Contexto D3D11.
 * @param stencilRef    Valor de referencia para operaciones de stencil.
 * @param reset         Si true, desactiva el estado (vuelve al por defecto).
 *
 * @note
 * - Llama con `reset=false` para activar tu estado configurado.
 * - Llama con `reset=true` para volver al estado por defecto.
 */
void DepthStencilState::render(DeviceContext& deviceContext,
    unsigned int stencilRef,
    bool reset) {
    if (!deviceContext.m_deviceContext) {
        ERROR("DepthStencilState", "render", "DeviceContext is nullptr.");
        return;
    }
    if (!m_depthStencilState && !reset) {
        ERROR("DepthStencilState", "render", "DepthStencilState is nullptr");
        return;
    }

    if (!reset) {
        deviceContext.m_deviceContext->OMSetDepthStencilState(m_depthStencilState, stencilRef);
    }
    else {
        deviceContext.m_deviceContext->OMSetDepthStencilState(nullptr, stencilRef);
    }
}

/**
 * @brief Libera el recurso del estado de profundidad/stencil.
 */
void DepthStencilState::destroy() {
    SAFE_RELEASE(m_depthStencilState);
}
