/**
 * @file Device.cpp
 * @brief Implementación de la clase Device.
 *
 * Esta clase se encarga de la creación de todos los recursos base de Direct3D 11:
 * - Shaders
 * - Buffers
 * - Vistas (RTV, DSV)
 * - Texturas
 * - Estados (Blend, Rasterizer, DepthStencil, Sampler)
 *
 * Es uno de los núcleos del motor gráfico, ya que todo lo visible depende de los recursos
 * que se crean desde aquí.
 */

#include "Device.h"

 /**
  * @brief Libera la interfaz ID3D11Device.
  */
void Device::destroy() {
    SAFE_RELEASE(m_device);
}

/**
 * @brief Crea una vista de renderizado (RTV) desde un recurso dado.
 */
HRESULT Device::CreateRenderTargetView(ID3D11Resource* pResource,
    const D3D11_RENDER_TARGET_VIEW_DESC* pDesc,
    ID3D11RenderTargetView** ppRTView) {

    if (!pResource) {
        ERROR("Device", "CreateRenderTargetView", "pResource is nullptr");
        return E_INVALIDARG;
    }
    if (!ppRTView) {
        ERROR("Device", "CreateRenderTargetView", "ppRTView is nullptr");
        return E_POINTER;
    }

    HRESULT hr = m_device->CreateRenderTargetView(pResource, pDesc, ppRTView);

    if (SUCCEEDED(hr)) {
        MESSAGE("Device", "CreateRenderTargetView", "Render Target View created successfully!");
    }
    else {
        ERROR("Device", "CreateRenderTargetView", ("Failed. HRESULT: " + std::to_string(hr)).c_str());
    }

    return hr;
}

/**
 * @brief Crea una textura 2D.
 */
HRESULT Device::CreateTexture2D(const D3D11_TEXTURE2D_DESC* pDesc,
    const D3D11_SUBRESOURCE_DATA* pInitialData,
    ID3D11Texture2D** ppTexture2D) {

    if (!pDesc) {
        ERROR("Device", "CreateTexture2D", "pDesc is nullptr");
        return E_INVALIDARG;
    }
    if (!ppTexture2D) {
        ERROR("Device", "CreateTexture2D", "ppTexture2D is nullptr");
        return E_POINTER;
    }

    HRESULT hr = m_device->CreateTexture2D(pDesc, pInitialData, ppTexture2D);

    if (SUCCEEDED(hr)) {
        MESSAGE("Device", "CreateTexture2D", "Texture2D created successfully!");
    }
    else {
        ERROR("Device", "CreateTexture2D", ("Failed. HRESULT: " + std::to_string(hr)).c_str());
    }

    return hr;
}

/**
 * @brief Crea una vista de profundidad y stencil (DSV).
 */
HRESULT Device::CreateDepthStencilView(ID3D11Resource* pResource,
    const D3D11_DEPTH_STENCIL_VIEW_DESC* pDesc,
    ID3D11DepthStencilView** ppDepthStencilView) {

    if (!pResource || !ppDepthStencilView) {
        ERROR("Device", "CreateDepthStencilView", "Nullptr en argumentos.");
        return E_POINTER;
    }

    HRESULT hr = m_device->CreateDepthStencilView(pResource, pDesc, ppDepthStencilView);
    if (SUCCEEDED(hr)) {
        MESSAGE("Device", "CreateDepthStencilView", "DSV creado exitosamente.");
    }
    else {
        ERROR("Device", "CreateDepthStencilView", ("Fallo. HRESULT: " + std::to_string(hr)).c_str());
    }

    return hr;
}

/**
 * @brief Crea un vertex shader desde su bytecode.
 */
HRESULT Device::CreateVertexShader(const void* pShaderBytecode,
    unsigned int BytecodeLength,
    ID3D11ClassLinkage* pClassLinkage,
    ID3D11VertexShader** ppVertexShader) {

    if (!pShaderBytecode || !ppVertexShader)
        return E_POINTER;

    HRESULT hr = m_device->CreateVertexShader(pShaderBytecode, BytecodeLength, pClassLinkage, ppVertexShader);
    return hr;
}

/**
 * @brief Crea un input layout para mapear vértices al shader.
 */
HRESULT Device::CreateInputLayout(const D3D11_INPUT_ELEMENT_DESC* pInputElementDescs,
    unsigned int NumElements,
    const void* pShaderBytecodeWithInputSignature,
    unsigned int BytecodeLength,
    ID3D11InputLayout** ppInputLayout) {

    if (!pInputElementDescs || !ppInputLayout)
        return E_POINTER;

    return m_device->CreateInputLayout(pInputElementDescs, NumElements, pShaderBytecodeWithInputSignature, BytecodeLength, ppInputLayout);
}

/**
 * @brief Crea un pixel shader desde su bytecode.
 */
HRESULT Device::CreatePixelShader(const void* pShaderBytecode,
    unsigned int BytecodeLength,
    ID3D11ClassLinkage* pClassLinkage,
    ID3D11PixelShader** ppPixelShader) {

    if (!pShaderBytecode || !ppPixelShader)
        return E_POINTER;

    return m_device->CreatePixelShader(pShaderBytecode, BytecodeLength, pClassLinkage, ppPixelShader);
}

/**
 * @brief Crea un estado de muestreo para texturas.
 */
HRESULT Device::CreateSamplerState(const D3D11_SAMPLER_DESC* pSamplerDesc,
    ID3D11SamplerState** ppSamplerState) {

    if (!pSamplerDesc || !ppSamplerState)
        return E_POINTER;

    return m_device->CreateSamplerState(pSamplerDesc, ppSamplerState);
}

/**
 * @brief Crea un buffer (constante, vértices, índices...).
 */
HRESULT Device::CreateBuffer(const D3D11_BUFFER_DESC* pDesc,
    const D3D11_SUBRESOURCE_DATA* pInitialData,
    ID3D11Buffer** ppBuffer) {

    if (!pDesc || !ppBuffer)
        return E_POINTER;

    return m_device->CreateBuffer(pDesc, pInitialData, ppBuffer);
}

/**
 * @brief Crea un estado de mezcla (blending).
 */
HRESULT Device::CreateBlendState(const D3D11_BLEND_DESC* pBlendStateDesc,
    ID3D11BlendState** ppBlendState) {

    if (!pBlendStateDesc || !ppBlendState)
        return E_POINTER;

    return m_device->CreateBlendState(pBlendStateDesc, ppBlendState);
}

/**
 * @brief Crea un estado de profundidad/stencil.
 */
HRESULT Device::CreateDepthStencilState(const D3D11_DEPTH_STENCIL_DESC* pDepthStencilDesc,
    ID3D11DepthStencilState** ppDepthStencilState) {

    if (!pDepthStencilDesc || !ppDepthStencilState)
        return E_POINTER;

    return m_device->CreateDepthStencilState(pDepthStencilDesc, ppDepthStencilState);
}

/**
 * @brief Crea un estado de rasterizado.
 */
HRESULT Device::CreateRasterizerState(const D3D11_RASTERIZER_DESC* pRasterizerDesc,
    ID3D11RasterizerState** ppRasterizerState) {

    if (!pRasterizerDesc || !ppRasterizerState)
        return E_POINTER;

    return m_device->CreateRasterizerState(pRasterizerDesc, ppRasterizerState);
}
