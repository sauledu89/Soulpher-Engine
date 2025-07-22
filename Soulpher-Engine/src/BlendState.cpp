#include "BlendState.h"
#include "Device.h"
#include "DeviceContext.h"

HRESULT
BlendState::init(Device & device) {
    if (!device.m_device) {
        ERROR("ShaderProgram", "init", "Device is null.");
        return E_POINTER;
    }

    D3D11_BLEND_DESC blendDesc = {};
    blendDesc.AlphaToCoverageEnable = FALSE;
    blendDesc.IndependentBlendEnable = FALSE;

    D3D11_RENDER_TARGET_BLEND_DESC rtBlendDesc = {};
    rtBlendDesc.BlendEnable = TRUE;
    rtBlendDesc.SrcBlend = D3D11_BLEND_SRC_ALPHA;
    rtBlendDesc.DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    rtBlendDesc.BlendOp = D3D11_BLEND_OP_ADD;
    rtBlendDesc.SrcBlendAlpha = D3D11_BLEND_ONE;
    rtBlendDesc.DestBlendAlpha = D3D11_BLEND_ZERO;
    rtBlendDesc.BlendOpAlpha = D3D11_BLEND_OP_ADD;
    rtBlendDesc.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    blendDesc.RenderTarget[0] = rtBlendDesc;

    HRESULT hr = device.m_device->CreateBlendState(&blendDesc, &m_blendState);
    if (FAILED(hr)) {
        ERROR("BlendState", "init",
            ("Failed to create blend state. HRESULT: " + std::to_string(hr)).c_str());
        return hr;
    }

    return S_OK;
}

void
BlendState::render(DeviceContext& deviceContext,
    float* blendFactor,
    unsigned int sampleMask,
    bool reset) {
    if (!m_blendState) {
        ERROR("BlendState", "render", "BlendState is not initialized.");
        return;
    }
    float defaultBlendFactor[4] = { 0.f, 0.f, 0.f, 0.f };

    if (!blendFactor) {
        blendFactor = defaultBlendFactor;
    }

    if (!reset) {
        deviceContext.OMSetBlendState(m_blendState, blendFactor, sampleMask);
    }
    else {
        deviceContext.OMSetBlendState(nullptr, blendFactor, sampleMask);
    }
}

void
BlendState::destroy() {
    SAFE_RELEASE(m_blendState);
}