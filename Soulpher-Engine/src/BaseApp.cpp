#include "BaseApp.h"

int BaseApp::run(HINSTANCE hInstance, int nCmdShow) {
    if (FAILED(this->init(hInstance, nCmdShow))) {
        this->destroy();
        return 0;
    }

    MSG msg = { 0 };
    while (WM_QUIT != msg.message) {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else {
            this->update();
            this->render();
        }
    }

    this->destroy();
    return (int)msg.wParam;
}

HRESULT BaseApp::init(HINSTANCE hInstance, int nCmdShow) {
    HRESULT hr = S_OK;

    if (FAILED(g_window.init(hInstance, nCmdShow, BaseApp::WndProc)))
        return E_FAIL;

    hr = g_swapChain.init(g_device, g_deviceContext, g_backBuffer, g_window);
    if (FAILED(hr)) return hr;

    hr = g_renderTargetView.init(g_device, g_backBuffer, DXGI_FORMAT_R8G8B8A8_UNORM);
    if (FAILED(hr)) return hr;

    hr = g_depthStencil.init(g_device, g_window.m_width, g_window.m_height, DXGI_FORMAT_D24_UNORM_S8_UINT,
        D3D11_BIND_DEPTH_STENCIL, 4, 0);
    if (FAILED(hr)) return hr;

    hr = g_depthStencilView.init(g_device, g_depthStencil, DXGI_FORMAT_D24_UNORM_S8_UINT);
    if (FAILED(hr)) return hr;

    hr = g_viewport.init(g_window);
    if (FAILED(hr)) return hr;

    std::vector<D3D11_INPUT_ELEMENT_DESC> layout = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };
    hr = g_shaderProgram.init(g_device, "Soulpher-Engine.fx", layout);
    if (FAILED(hr)) return hr;

    if (cubeMesh.m_vertex.empty() || cubeMesh.m_index.empty())
        return E_FAIL;

    if (FAILED(m_vertexBuffer.init(g_device, cubeMesh, D3D11_BIND_VERTEX_BUFFER))) return E_FAIL;
    if (FAILED(m_indexBuffer.init(g_device, cubeMesh, D3D11_BIND_INDEX_BUFFER))) return E_FAIL;

    g_deviceContext.IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    if (FAILED(m_neverChanges.init(g_device, sizeof(CBNeverChanges)))) return E_FAIL;
    if (FAILED(m_changeOnResize.init(g_device, sizeof(CBChangeOnResize)))) return E_FAIL;
    if (FAILED(m_changeEveryFrame.init(g_device, sizeof(CBChangesEveryFrame)))) return E_FAIL;

    hr = D3DX11CreateShaderResourceViewFromFile(g_device.m_device, "seafloor.dds", NULL, NULL, &g_pTextureRV, NULL);
    if (FAILED(hr)) return hr;

    D3D11_SAMPLER_DESC sampDesc = {};
    sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sampDesc.MinLOD = 0;
    sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
    if (FAILED(g_device.CreateSamplerState(&sampDesc, &g_pSamplerLinear))) return E_FAIL;

    XMVECTOR Eye = XMVectorSet(0.0f, 3.0f, -6.0f, 0.0f);
    XMVECTOR At = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    XMVECTOR Up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    g_View = XMMatrixLookAtLH(Eye, At, Up);
    g_Projection = XMMatrixPerspectiveFovLH(XM_PIDIV4, g_window.m_width / (FLOAT)g_window.m_height, 0.01f, 100.0f);
    cbNeverChanges.mView = XMMatrixTranspose(g_View);
    cbChangesOnResize.mProjection = XMMatrixTranspose(g_Projection);

    if (FAILED(g_shaderShadow.CreateShader(g_device, PIXEL_SHADER, "Soulpher-Engine.fx"))) return E_FAIL;
    if (FAILED(m_constShadow.init(g_device, sizeof(CBChangesEveryFrame)))) return E_FAIL;
    if (FAILED(g_shadowBlendState.init(g_device))) return E_FAIL;

    D3D11_DEPTH_STENCIL_DESC dsDesc = {};
    dsDesc.DepthEnable = TRUE;
    dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    dsDesc.DepthFunc = D3D11_COMPARISON_LESS;
    dsDesc.StencilEnable = FALSE;
    if (FAILED(g_device.CreateDepthStencilState(&dsDesc, &g_pShadowDepthStencilState))) return E_FAIL;

    g_vMeshColor = XMFLOAT4(0.7f, 0.7f, 0.7f, 1.0f);
    g_LightPos = XMFLOAT4(2.0f, 4.0f, -2.0f, 1.0f);
    g_ModelScale = 0.005f;
    g_ModelRotation = { 0.0f, 0.0f, 0.0f };

    // ✅ Inicializar UI de forma segura
    g_userInterface.init(g_window.m_hWnd, g_device.m_device, g_deviceContext.m_deviceContext);

    return S_OK;
}

void BaseApp::update() {
    g_userInterface.update();

    ImGui::Begin("Model Viewer Controls");
    g_userInterface.floatControl("Scale", &g_ModelScale, 0.5f);
    g_userInterface.vec3Control("Rotation", &g_ModelRotation.x, 0.0f);
    ImGui::Separator();
    g_userInterface.vec3Control("Light Position", &g_LightPos.x, 2.0f);
    ImGui::End();

    m_neverChanges.update(g_deviceContext, nullptr, 0, nullptr, &cbNeverChanges, 0, 0);
    m_changeOnResize.update(g_deviceContext, nullptr, 0, nullptr, &cbChangesOnResize, 0, 0);

    XMMATRIX matScale = XMMatrixScaling(g_ModelScale, g_ModelScale, g_ModelScale);
    XMMATRIX matRotation = XMMatrixRotationRollPitchYaw(
        XMConvertToRadians(g_ModelRotation.x),
        XMConvertToRadians(g_ModelRotation.y),
        XMConvertToRadians(g_ModelRotation.z));
    XMMATRIX matTranslation = XMMatrixTranslation(0.0f, 0.0f, 0.0f);
    g_World = matScale * matRotation * matTranslation;

    cb.mWorld = XMMatrixTranspose(g_World);
    cb.vMeshColor = g_vMeshColor;
    m_changeEveryFrame.update(g_deviceContext, nullptr, 0, nullptr, &cb, 0, 0);

    float dot = g_LightPos.y;
    XMMATRIX shadowMatrix = XMMATRIX(dot, -g_LightPos.x, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, -g_LightPos.z, dot, 0.0f, 0.0f, -1.0f, 0.0f, dot);
    XMMATRIX shadowWorld = g_World * shadowMatrix;
    cbShadow.mWorld = XMMatrixTranspose(shadowWorld);
    cbShadow.vMeshColor = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.5f);
    m_constShadow.update(g_deviceContext, nullptr, 0, nullptr, &cbShadow, 0, 0);
}

void BaseApp::render() {
    float ClearColor[4] = { 0.0f, 0.125f, 0.3f, 1.0f };
    float blendFactor[4] = { 0.f, 0.f, 0.f, 0.f };

    g_renderTargetView.render(g_deviceContext, g_depthStencilView, 1, ClearColor);
    g_viewport.render(g_deviceContext);
    g_depthStencilView.render(g_deviceContext);
    g_shaderProgram.render(g_deviceContext);
    m_neverChanges.render(g_deviceContext, 0, 1);
    m_changeOnResize.render(g_deviceContext, 1, 1);

    m_vertexBuffer.render(g_deviceContext, 0, 1);
    m_indexBuffer.render(g_deviceContext, 0, 1, false, DXGI_FORMAT_R32_UINT);
    m_changeEveryFrame.render(g_deviceContext, 2, 1);
    m_changeEveryFrame.render(g_deviceContext, 2, 1, true);
    g_deviceContext.PSSetShaderResources(0, 1, &g_pTextureRV);
    g_deviceContext.PSSetSamplers(0, 1, &g_pSamplerLinear);
    g_deviceContext.DrawIndexed(cubeMesh.m_index.size(), 0, 0);

    g_shaderShadow.render(g_deviceContext, PIXEL_SHADER);
    g_shadowBlendState.render(g_deviceContext, blendFactor, 0xffffffff);
    g_deviceContext.OMSetDepthStencilState(g_pShadowDepthStencilState, 0);
    m_vertexBuffer.render(g_deviceContext, 0, 1);
    m_indexBuffer.render(g_deviceContext, 0, 1, false, DXGI_FORMAT_R32_UINT);
    m_constShadow.render(g_deviceContext, 2, 1, true);
    g_deviceContext.DrawIndexed(cubeMesh.m_index.size(), 0, 0);
    g_shadowBlendState.render(g_deviceContext, blendFactor, 0xffffffff, true);
    g_deviceContext.OMSetDepthStencilState(nullptr, 0);

    g_userInterface.render();  // <- Verifica internamente que el contexto esté bien
    g_swapChain.present();
}

void BaseApp::destroy() {
    g_userInterface.destroy();  // <- Esto ahora controla bien el contexto

    g_deviceContext.ClearState();
    g_shadowBlendState.destroy();
    if (g_pShadowDepthStencilState) g_pShadowDepthStencilState->Release();
    g_shaderShadow.destroy();
    if (g_pSamplerLinear) g_pSamplerLinear->Release();
    if (g_pTextureRV) g_pTextureRV->Release();

    m_neverChanges.destroy();
    m_changeOnResize.destroy();
    m_changeEveryFrame.destroy();
    m_constShadow.destroy();
    m_vertexBuffer.destroy();
    m_indexBuffer.destroy();
    g_shaderProgram.destroy();
    g_depthStencil.destroy();
    g_depthStencilView.destroy();
    g_renderTargetView.destroy();
    g_swapChain.destroy();
    g_deviceContext.destroy();

    if (g_device.m_device) g_device.m_device->Release();
}

LRESULT CALLBACK BaseApp::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam)) return true;

    switch (message) {
    case WM_PAINT:
        BeginPaint(hWnd, nullptr);
        EndPaint(hWnd, nullptr);
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}
