/**
 * @file ForwardRenderer.cpp
 * @brief Shadow map renderer: depth pass (light POV) + shadow map binding.
 */

#include "Rendering/ForwardRenderer.h"
#include "Device.h"
#include "DeviceContext.h"
#include "ECS/Actor.h"

HRESULT
ForwardRenderer::init(Device& device, unsigned int shadowMapSize) {
    m_shadowMapSize = shadowMapSize;

    // Shadow depth texture: R24G8_TYPELESS allows both DSV and SRV on the same resource
    HRESULT hr = m_shadowTex.init(
        device,
        shadowMapSize, shadowMapSize,
        DXGI_FORMAT_R24G8_TYPELESS,
        D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE
    );
    if (FAILED(hr)) {
        ERROR("ForwardRenderer", "init", "Failed to create shadow depth texture.");
        return hr;
    }

    // Depth-stencil view for writing during shadow pass
    hr = m_shadowDSV.init(device, m_shadowTex, DXGI_FORMAT_D24_UNORM_S8_UINT);
    if (FAILED(hr)) {
        ERROR("ForwardRenderer", "init", "Failed to create shadow DSV.");
        return hr;
    }

    // SRV alias for reading in the main PS (R24_UNORM_X8_TYPELESS reads the depth channel)
    hr = m_shadowSRV.init(device, m_shadowTex, DXGI_FORMAT_R24_UNORM_X8_TYPELESS);
    if (FAILED(hr)) {
        ERROR("ForwardRenderer", "init", "Failed to create shadow SRV.");
        return hr;
    }

    // Fixed shadow viewport
    hr = m_shadowViewport.init(shadowMapSize, shadowMapSize);
    if (FAILED(hr)) {
        ERROR("ForwardRenderer", "init", "Failed to create shadow viewport.");
        return hr;
    }

    // Compile shadow depth VS (entry point must be "VS" — ShaderProgram is hardcoded to that name)
    hr = m_shadowDepthShader.CreateShader(device, VERTEX_SHADER, "ShadowDepth.hlsl");
    if (FAILED(hr)) {
        ERROR("ForwardRenderer", "init", "Failed to compile ShadowDepth.hlsl.");
        return hr;
    }

    // Input layout: only POSITION (float3).
    // The stride of SimpleVertex (56 bytes) is set by the vertex buffer itself,
    // so D3D11 reads the first 12 bytes per vertex as position and skips the rest.
    std::vector<D3D11_INPUT_ELEMENT_DESC> posLayout;
    {
        D3D11_INPUT_ELEMENT_DESC e{};
        e.SemanticName         = "POSITION";
        e.SemanticIndex        = 0;
        e.Format               = DXGI_FORMAT_R32G32B32_FLOAT;
        e.InputSlot            = 0;
        e.AlignedByteOffset    = 0;
        e.InputSlotClass       = D3D11_INPUT_PER_VERTEX_DATA;
        e.InstanceDataStepRate = 0;
        posLayout.push_back(e);
    }
    hr = m_shadowDepthShader.CreateInputLayout(device, posLayout);
    if (FAILED(hr)) {
        ERROR("ForwardRenderer", "init", "Failed to create shadow input layout.");
        return hr;
    }

    return S_OK;
}

void
ForwardRenderer::renderShadowPass(DeviceContext& deviceContext,
                                   const std::vector<EU::TSharedPointer<Actor>>& actors) {
    // Ensure the shadow SRV is not bound as input while we write to the DSV
    unbindShadowMap(deviceContext);

    // Depth-only render target (no color output)
    ID3D11RenderTargetView* nullRTV = nullptr;
    deviceContext.OMSetRenderTargets(1, &nullRTV, m_shadowDSV.m_depthStencilView);

    // Clear shadow depth to 1.0 (farthest)
    deviceContext.ClearDepthStencilView(
        m_shadowDSV.m_depthStencilView,
        D3D11_CLEAR_DEPTH, 1.0f, 0
    );

    // Shadow viewport (2048x2048 or configured size)
    m_shadowViewport.render(deviceContext);

    // Shadow pipeline: depth-only VS, no PS, POSITION-only input layout
    deviceContext.IASetInputLayout(m_shadowDepthShader.m_inputLayout.m_inputLayout);
    deviceContext.VSSetShader(m_shadowDepthShader.m_VertexShader, nullptr, 0);
    deviceContext.PSSetShader(nullptr, nullptr, 0);

    // Render depth for every shadow-casting actor
    // CBPerFrame (b0, containing LightViewProjection) must already be bound by the caller
    for (const auto& actor : actors) {
        if (!actor.isNull() && actor->canCastShadow()) {
            actor->renderDepth(deviceContext);
        }
    }
}

void
ForwardRenderer::bindShadowMap(DeviceContext& deviceContext) {
    ID3D11ShaderResourceView* srv = m_shadowSRV.srv();
    deviceContext.PSSetShaderResources(6, 1, &srv);
}

void
ForwardRenderer::unbindShadowMap(DeviceContext& deviceContext) {
    ID3D11ShaderResourceView* nullSRV = nullptr;
    deviceContext.PSSetShaderResources(6, 1, &nullSRV);
}

XMMATRIX
ForwardRenderer::computeLightViewProj(const EU::Vector3& lightDir,
                                       const XMFLOAT3&   sceneCenter,
                                       float              sceneRadius) const {
    XMVECTOR L       = XMVector3Normalize(XMVectorSet(lightDir.x, lightDir.y, lightDir.z, 0.0f));
    XMVECTOR center  = XMLoadFloat3(&sceneCenter);
    // Place the light source behind the scene along the light direction
    XMVECTOR lightPos = center - L * sceneRadius;

    // Choose an up vector that avoids singularity when L is vertical
    XMVECTOR up = (fabsf(XMVectorGetY(L)) < 0.99f)
        ? XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)
        : XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);

    XMMATRIX lightView = XMMatrixLookAtLH(lightPos, center, up);

    // Orthographic frustum that covers the full scene sphere
    float extent = sceneRadius * 2.0f;
    XMMATRIX lightProj = XMMatrixOrthographicLH(extent, extent, 0.1f, extent * 2.0f);

    return lightView * lightProj;
}

void
ForwardRenderer::destroy() {
    m_shadowDepthShader.destroy();
    // m_shadowViewport has no GPU resources (D3D11_VIEWPORT is a plain struct)
    m_shadowDSV.destroy();
    m_shadowSRV.destroy();
    m_shadowTex.destroy();
}
