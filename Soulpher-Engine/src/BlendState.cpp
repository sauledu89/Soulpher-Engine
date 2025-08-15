/**
 * @file BlendState.cpp
 * @brief Implementación del estado de blending para Direct3D 11.
 *
 * @details
 * El *alpha blending* combina el color que sale del pixel shader (fuente)
 * con el color ya presente en el render target (destino). Este archivo crea
 * un estado clásico de **transparencia no premultiplicada**:
 *
 *   ColorFinal = SrcColor * SrcAlpha + DstColor * (1 - SrcAlpha)
 *   AlphaFinal = SrcAlpha * 1 + DstAlpha * 0
 *
 * 🔹 **Para estudiantes**:
 * - Si usas **texturas con alpha “normal” (no premultiplicado)**, esta config te sirve.
 * - Si usas **alpha premultiplicado**, cambia a:
 *     SrcBlend = ONE, DestBlend = INV_SRC_ALPHA
 * - `render(reset=true)` restaura el estado por defecto (sin blend).
 */

#include "BlendState.h"
#include "Prerequisites.h"
#include "DeviceContext.h"
#include "Device.h"

 /**
  * @brief Crea un estado de blending estándar (SrcAlpha / InvSrcAlpha).
  * @param device Dispositivo de Direct3D 11.
  * @return HRESULT S_OK en éxito o error en caso contrario.
  *
  * @note Configuración pensada para **alpha no premultiplicado**:
  * - Color:   Src = SRC_ALPHA, Dst = INV_SRC_ALPHA, Op = ADD
  * - Alpha:   Src = ONE,       Dst = ZERO,         Op = ADD
  * - WriteMask: escribe en RGBA.
  */
HRESULT BlendState::init(Device& device) {
    if (!device.m_device) {
        ERROR("BlendState", "init", "Device is null.");
        return E_POINTER;
    }

    // Descriptor base del estado de blending
    D3D11_BLEND_DESC blendDesc = {};
    blendDesc.AlphaToCoverageEnable = FALSE;      // Multisample alpha-to-coverage desactivado
    blendDesc.IndependentBlendEnable = FALSE;     // Mismo blend para todos los RTVs

    // Config de mezcla para el RTV[0]
    D3D11_RENDER_TARGET_BLEND_DESC rtBlendDesc = {};
    rtBlendDesc.BlendEnable = TRUE;

    // Color: C_out = C_src * A_src + C_dst * (1 - A_src)
    rtBlendDesc.SrcBlend = D3D11_BLEND_SRC_ALPHA;
    rtBlendDesc.DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    rtBlendDesc.BlendOp = D3D11_BLEND_OP_ADD;

    // Alpha: A_out = A_src * 1 + A_dst * 0
    rtBlendDesc.SrcBlendAlpha = D3D11_BLEND_ONE;
    rtBlendDesc.DestBlendAlpha = D3D11_BLEND_ZERO;
    rtBlendDesc.BlendOpAlpha = D3D11_BLEND_OP_ADD;

    rtBlendDesc.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    blendDesc.RenderTarget[0] = rtBlendDesc;

    // Crear el estado
    HRESULT hr = device.m_device->CreateBlendState(&blendDesc, &m_blendState);
    if (FAILED(hr)) {
        ERROR("BlendState", "init",
            ("Failed to create blend state. HRESULT: " + std::to_string(hr)).c_str());
        return hr;
    }
    return S_OK;
}

/**
 * @brief Aplica (o resetea) el estado de blending en la etapa OM.
 * @param deviceContext Contexto de dispositivo.
 * @param blendFactor Vector RGBA para operaciones de blending (generalmente {0,0,0,0}).
 * @param sampleMask Máscara de muestras para MSAA (por defecto 0xFFFFFFFF).
 * @param reset Si es true, desactiva el blending (establece estado por defecto).
 *
 * @note
 * - Llama a `reset=false` antes de dibujar geometría **transparente**.
 * - Llama a `reset=true` para volver al estado por defecto antes de dibujar **opaco**.
 * - Recuerda **dibujar objetos transparentes al final** y **ordenados por profundidad**.
 */
void BlendState::render(DeviceContext& deviceContext,
    float* blendFactor,
    unsigned int sampleMask,
    bool reset) {
    if (!deviceContext.m_deviceContext) {
        ERROR("BlendState", "render", "DeviceContext is nullptr.");
        return;
    }
    if (!m_blendState && !reset) {
        ERROR("BlendState", "render", "BlendState is not initialized.");
        return;
    }

    float defaultBlendFactor[4] = { 0.f, 0.f, 0.f, 0.f };
    if (!blendFactor) blendFactor = defaultBlendFactor;

    if (!reset) {
        deviceContext.m_deviceContext->OMSetBlendState(m_blendState, blendFactor, sampleMask);
    }
    else {
        // Estado por defecto (blending off)
        deviceContext.m_deviceContext->OMSetBlendState(nullptr, blendFactor, sampleMask);
    }
}

/**
 * @brief Libera el recurso del estado de blending.
 */
void BlendState::destroy() {
    SAFE_RELEASE(m_blendState);
}
