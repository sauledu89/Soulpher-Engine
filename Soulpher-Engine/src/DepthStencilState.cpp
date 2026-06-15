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
 *
 * @note [GameDev] Durante el shadow depth pass, ForwardRenderer solo necesita el DSV
 * (ningun RTV) y el depth write debe estar activo. Durante el color pass normal,
 * el depth test es LESS (se dibuja si es mas cercano que lo que ya hay).
 * El DepthStencilState con enableDepth=false se usa para el renderShadow() legacy del Actor
 * (sombra proyectada plana) porque esa geometria proyectada esta exactamente sobre el suelo
 * y fallaria el depth test normal (z-fighting).
 */

#include "DepthStencilState.h"
#include "Device.h"
#include "DeviceContext.h"

/**
 * @brief Inicializa el estado de profundidad.
 * @param device      Dispositivo D3D11.
 * @param enableDepth Activa/desactiva la prueba de profundidad.
 * @param writeMask   Máscara de escritura del depth buffer.
 * @param compareFunc Función de comparación de profundidad.
 * @return HRESULT S_OK si ok, o código de error en fallo.
 *
 * @note [GameDev] Los tres estados más usados en deferred rendering:
 *  - init(device, true, D3D11_DEPTH_WRITE_MASK_ALL, LESS)       → opaco/shadow (default).
 *  - init(device, true, D3D11_DEPTH_WRITE_MASK_ZERO, LESS_EQUAL) → transparentes.
 *  - init(device, false)                                          → fullscreen quad lighting.
 */
HRESULT DepthStencilState::init(Device& device,
                                 bool                   enableDepth,
                                 D3D11_DEPTH_WRITE_MASK writeMask,
                                 D3D11_COMPARISON_FUNC  compareFunc) {
    if (!device.m_device) {
        ERROR("DepthStencilState", "init", "Device is null.");
        return E_POINTER;
    }

    D3D11_DEPTH_STENCIL_DESC desc = {};
    desc.DepthEnable    = enableDepth ? TRUE : FALSE;
    desc.DepthWriteMask = writeMask;
    desc.DepthFunc      = compareFunc;
    desc.StencilEnable  = FALSE;

    HRESULT hr = device.CreateDepthStencilState(&desc, &m_depthStencilState);
    if (FAILED(hr)) {
        ERROR("DepthStencilState", "init", "Failed to create DepthStencilState");
        return hr;
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
