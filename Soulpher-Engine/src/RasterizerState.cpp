/**
 * @file RasterizerState.cpp
 * @brief Implementación del estado de rasterización configurable.
 * @ingroup rendering
 */
#include "RasterizerState.h"
#include "Device.h"
#include "DeviceContext.h"

HRESULT RasterizerState::init(Device& device) {
    return init(device, D3D11_FILL_SOLID, D3D11_CULL_BACK, false, true);
}

HRESULT RasterizerState::init(Device& device,
                               D3D11_FILL_MODE fillMode,
                               D3D11_CULL_MODE cullMode,
                               bool            frontCCW,
                               bool            depthClip) {
    if (!device.m_device) {
        LOG_ERROR("RasterizerState", "init", "Device is null.");
        return E_POINTER;
    }

    D3D11_RASTERIZER_DESC desc = {};
    desc.FillMode              = fillMode;
    desc.CullMode              = cullMode;
    desc.FrontCounterClockwise = frontCCW ? TRUE : FALSE;
    desc.DepthBias             = 0;
    desc.SlopeScaledDepthBias  = 0.0f;
    desc.DepthBiasClamp        = 0.0f;
    desc.DepthClipEnable       = depthClip ? TRUE : FALSE;
    desc.ScissorEnable         = FALSE;
    desc.MultisampleEnable     = FALSE;
    desc.AntialiasedLineEnable = FALSE;

    HRESULT hr = device.m_device->CreateRasterizerState(&desc, &m_rasterizerState);
    if (FAILED(hr)) {
        LOG_ERROR("RasterizerState", "init", "Failed to create RasterizerState.");
        return hr;
    }
    return S_OK;
}

void RasterizerState::render(DeviceContext& deviceContext) {
    if (!deviceContext.m_deviceContext) return;
    deviceContext.m_deviceContext->RSSetState(m_rasterizerState);
}

void RasterizerState::update() {}

void RasterizerState::destroy() {
    SAFE_RELEASE(m_rasterizerState);
}
