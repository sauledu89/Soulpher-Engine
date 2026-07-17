#include "EngineUtilities/Utilities/EditorViewportPass.h"
#include "Device.h"
#include "DeviceContext.h"

HRESULT EditorViewportPass::init(Device& device, unsigned int width, unsigned int height) {
    if (!device.m_device) {
        LOG_ERROR("EditorViewportPass", "init", "Device is null.");
        return E_POINTER;
    }
    m_width  = width  > 0 ? width  : 1;
    m_height = height > 0 ? height : 1;

    HRESULT hr = m_colorTexture.init(device, m_width, m_height,
        DXGI_FORMAT_R8G8B8A8_UNORM,
        D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE);
    if (FAILED(hr)) {
        LOG_ERROR("EditorViewportPass", "init", "Failed to create color texture.");
        return hr;
    }

    hr = m_colorSRV.init(device, m_colorTexture, DXGI_FORMAT_R8G8B8A8_UNORM);
    if (FAILED(hr)) {
        LOG_ERROR("EditorViewportPass", "init", "Failed to create color SRV.");
        return hr;
    }

    hr = m_rtv.init(device, m_colorTexture, D3D11_RTV_DIMENSION_TEXTURE2D, DXGI_FORMAT_R8G8B8A8_UNORM);
    if (FAILED(hr)) {
        LOG_ERROR("EditorViewportPass", "init", "Failed to create RTV.");
        return hr;
    }

    hr = m_depthTexture.init(device, m_width, m_height,
        DXGI_FORMAT_D24_UNORM_S8_UINT,
        D3D11_BIND_DEPTH_STENCIL);
    if (FAILED(hr)) {
        LOG_ERROR("EditorViewportPass", "init", "Failed to create depth texture.");
        return hr;
    }

    hr = m_dsv.init(device, m_depthTexture, DXGI_FORMAT_D24_UNORM_S8_UINT);
    if (FAILED(hr)) {
        LOG_ERROR("EditorViewportPass", "init", "Failed to create DSV.");
        return hr;
    }

    return S_OK;
}

HRESULT EditorViewportPass::resize(Device& device, unsigned int width, unsigned int height) {
    destroy();
    return init(device, width, height);
}

void EditorViewportPass::begin(DeviceContext& dc, const float clearColor[4]) {
    if (m_rtv.get())
        dc.m_deviceContext->ClearRenderTargetView(m_rtv.get(), clearColor);
    if (m_dsv.m_depthStencilView)
        dc.m_deviceContext->ClearDepthStencilView(
            m_dsv.m_depthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}

void EditorViewportPass::setViewport(DeviceContext& dc) {
    D3D11_VIEWPORT vp = {};
    vp.TopLeftX = 0.0f;
    vp.TopLeftY = 0.0f;
    vp.Width    = static_cast<float>(m_width);
    vp.Height   = static_cast<float>(m_height);
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    dc.m_deviceContext->RSSetViewports(1, &vp);
}

void EditorViewportPass::clearDepth(DeviceContext& dc) {
    if (m_dsv.m_depthStencilView)
        dc.m_deviceContext->ClearDepthStencilView(
            m_dsv.m_depthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);
}

void EditorViewportPass::destroy() {
    m_rtv.destroy();
    m_dsv.destroy();
    m_depthTexture.destroy();
    m_colorSRV.destroy();
    m_colorTexture.destroy();
    m_width  = 1;
    m_height = 1;
}
