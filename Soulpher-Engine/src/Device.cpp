/**
 * @file Device.cpp
 * @brief Implementación del dispositivo D3D11 y creación de recursos gráficos.
 *
 * @details
 * Esta clase encapsula el manejo del dispositivo (`ID3D11Device`) en DirectX 11.
 * Permite crear y gestionar recursos esenciales para el renderizado como:
 * - Render Target Views
 * - Texturas 2D
 * - Depth Stencil Views
 * - Buffers (constantes, vértices, índices)
 * - Shaders (VS y PS)
 * - Sampler States, Blend States, Rasterizer States
 *
 * @note
 * Cada función valida sus parámetros antes de crear el recurso.
 * Esto ayuda a prevenir errores comunes como punteros nulos.
 *
 * @warning
 * El orden de destrucción es importante: siempre destruir recursos antes de liberar el dispositivo.
 *
 * @par Ejemplo de uso básico:
 * @code
 * Device device;
 * // Crear un Render Target View
 * ID3D11RenderTargetView* rtv = nullptr;
 * HRESULT hr = device.CreateRenderTargetView(backBuffer, nullptr, &rtv);
 * if (FAILED(hr)) { /* manejar error }
 *@endcode
 */

#include "Device.h"

     void
     Device::destroy() {
     /** @brief Libera el dispositivo principal de DirectX 11. */
     SAFE_RELEASE(m_device);
 }

 /**
  * @brief Crea un Render Target View para el pipeline de renderizado.
  * @param pResource Recurso de DirectX (generalmente una textura de back buffer).
  * @param pDesc Descriptor opcional del RTV. Si es nullptr, usa configuración por defecto.
  * @param ppRTView Puntero donde se almacenará la interfaz creada.
  * @return HRESULT indicando éxito o fallo.
  *
  * @note Un Render Target View es donde la GPU dibuja la imagen final antes de enviarla a pantalla.
  */
 HRESULT
     Device::CreateRenderTargetView(ID3D11Resource* pResource,
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
         MESSAGE("Device", "CreateRenderTargetView", "Render Target View creado correctamente.");
     }
     else {
         ERROR("Device", "CreateRenderTargetView", ("Fallo al crear RTV. HRESULT: " + std::to_string(hr)).c_str());
     }
     return hr;
 }

 /**
  * @brief Crea una textura 2D en GPU.
  * @param pDesc Descriptor de la textura (dimensiones, formato, etc.).
  * @param pInitialData Datos iniciales opcionales.
  * @param ppTexture2D Puntero donde se almacenará la textura creada.
  * @return HRESULT indicando éxito o fallo.
  *
  * @note En videojuegos, las texturas 2D pueden ser usadas para mapas de color, normales, iluminación, etc.
  */
 HRESULT
     Device::CreateTexture2D(const D3D11_TEXTURE2D_DESC* pDesc,
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
         MESSAGE("Device", "CreateTexture2D", "Texture2D creada correctamente.");
     }
     else {
         ERROR("Device", "CreateTexture2D", ("Fallo al crear textura. HRESULT: " + std::to_string(hr)).c_str());
     }
     return hr;
 }

 /**
  * @brief Crea un Depth Stencil View para control de profundidad y stencil.
  * @param pResource Recurso asociado (generalmente una textura de profundidad).
  * @param pDesc Descriptor del DSV.
  * @param ppDepthStencilView Puntero donde se almacenará la vista creada.
  * @return HRESULT indicando éxito o fallo.
  *
  * @note El Depth Stencil es vital para evitar que objetos lejanos se dibujen encima de cercanos.
  */
 HRESULT
     Device::CreateDepthStencilView(ID3D11Resource* pResource,
         const D3D11_DEPTH_STENCIL_VIEW_DESC* pDesc,
         ID3D11DepthStencilView** ppDepthStencilView) {
     if (!pResource) {
         ERROR("Device", "CreateDepthStencilView", "pResource is nullptr");
         return E_INVALIDARG;
     }
     if (!ppDepthStencilView) {
         ERROR("Device", "CreateDepthStencilView", "ppDepthStencilView is nullptr");
         return E_POINTER;
     }
     HRESULT hr = m_device->CreateDepthStencilView(pResource, pDesc, ppDepthStencilView);
     if (SUCCEEDED(hr)) {
         MESSAGE("Device", "CreateDepthStencilView", "Depth Stencil View creado correctamente.");
     }
     else {
         ERROR("Device", "CreateDepthStencilView", ("Fallo al crear DSV. HRESULT: " + std::to_string(hr)).c_str());
     }
     return hr;
 }

 void
 Device::init() {}

 void
 Device::update() {}

 void
 Device::render() {}

 HRESULT
 Device::CreateVertexShader(const void* pShaderBytecode,
     unsigned int BytecodeLength,
     ID3D11ClassLinkage* pClassLinkage,
     ID3D11VertexShader** ppVertexShader) {
     if (!pShaderBytecode) {
         ERROR("Device", "CreateVertexShader", "pShaderBytecode is nullptr");
         return E_INVALIDARG;
     }
     if (!ppVertexShader) {
         ERROR("Device", "CreateVertexShader", "ppVertexShader is nullptr");
         return E_POINTER;
     }
     HRESULT hr = m_device->CreateVertexShader(pShaderBytecode, BytecodeLength, pClassLinkage, ppVertexShader);
     if (SUCCEEDED(hr)) {
         MESSAGE("Device", "CreateVertexShader", "Vertex Shader creado correctamente.");
     }
     else {
         ERROR("Device", "CreateVertexShader", ("Fallo al crear Vertex Shader. HRESULT: " + std::to_string(hr)).c_str());
     }
     return hr;
 }

 HRESULT
 Device::CreatePixelShader(const void* pShaderBytecode,
     unsigned int BytecodeLength,
     ID3D11ClassLinkage* pClassLinkage,
     ID3D11PixelShader** ppPixelShader) {
     if (!pShaderBytecode) {
         ERROR("Device", "CreatePixelShader", "pShaderBytecode is nullptr");
         return E_INVALIDARG;
     }
     if (!ppPixelShader) {
         ERROR("Device", "CreatePixelShader", "ppPixelShader is nullptr");
         return E_POINTER;
     }
     HRESULT hr = m_device->CreatePixelShader(pShaderBytecode, BytecodeLength, pClassLinkage, ppPixelShader);
     if (SUCCEEDED(hr)) {
         MESSAGE("Device", "CreatePixelShader", "Pixel Shader creado correctamente.");
     }
     else {
         ERROR("Device", "CreatePixelShader", ("Fallo al crear Pixel Shader. HRESULT: " + std::to_string(hr)).c_str());
     }
     return hr;
 }

 HRESULT
 Device::CreateInputLayout(const D3D11_INPUT_ELEMENT_DESC* pInputElementDescs,
     unsigned int NumElements,
     const void* pShaderBytecodeWithInputSignature,
     unsigned int BytecodeLength,
     ID3D11InputLayout** ppInputLayout) {
     if (!pInputElementDescs) {
         ERROR("Device", "CreateInputLayout", "pInputElementDescs is nullptr");
         return E_INVALIDARG;
     }
     if (!pShaderBytecodeWithInputSignature) {
         ERROR("Device", "CreateInputLayout", "pShaderBytecodeWithInputSignature is nullptr");
         return E_INVALIDARG;
     }
     if (!ppInputLayout) {
         ERROR("Device", "CreateInputLayout", "ppInputLayout is nullptr");
         return E_POINTER;
     }
     HRESULT hr = m_device->CreateInputLayout(pInputElementDescs, NumElements,
         pShaderBytecodeWithInputSignature, BytecodeLength, ppInputLayout);
     if (SUCCEEDED(hr)) {
         MESSAGE("Device", "CreateInputLayout", "Input Layout creado correctamente.");
     }
     else {
         ERROR("Device", "CreateInputLayout", ("Fallo al crear Input Layout. HRESULT: " + std::to_string(hr)).c_str());
     }
     return hr;
 }

 HRESULT
 Device::CreateBuffer(const D3D11_BUFFER_DESC* pDesc,
     const D3D11_SUBRESOURCE_DATA* pInitialData,
     ID3D11Buffer** ppBuffer) {
     if (!pDesc) {
         ERROR("Device", "CreateBuffer", "pDesc is nullptr");
         return E_INVALIDARG;
     }
     if (!ppBuffer) {
         ERROR("Device", "CreateBuffer", "ppBuffer is nullptr");
         return E_POINTER;
     }
     HRESULT hr = m_device->CreateBuffer(pDesc, pInitialData, ppBuffer);
     if (SUCCEEDED(hr)) {
         MESSAGE("Device", "CreateBuffer", "Buffer creado correctamente.");
     }
     else {
         ERROR("Device", "CreateBuffer", ("Fallo al crear Buffer. HRESULT: " + std::to_string(hr)).c_str());
     }
     return hr;
 }

 HRESULT
 Device::CreateSamplerState(const D3D11_SAMPLER_DESC* pSamplerDesc,
     ID3D11SamplerState** ppSamplerState) {
     if (!pSamplerDesc) {
         ERROR("Device", "CreateSamplerState", "pSamplerDesc is nullptr");
         return E_INVALIDARG;
     }
     if (!ppSamplerState) {
         ERROR("Device", "CreateSamplerState", "ppSamplerState is nullptr");
         return E_POINTER;
     }
     HRESULT hr = m_device->CreateSamplerState(pSamplerDesc, ppSamplerState);
     if (SUCCEEDED(hr)) {
         MESSAGE("Device", "CreateSamplerState", "Sampler State creado correctamente.");
     }
     else {
         ERROR("Device", "CreateSamplerState", ("Fallo al crear Sampler State. HRESULT: " + std::to_string(hr)).c_str());
     }
     return hr;
 }

 HRESULT
 Device::CreateBlendState(const D3D11_BLEND_DESC* pBlendStateDesc,
     ID3D11BlendState** ppBlendState) {
     if (!pBlendStateDesc) {
         ERROR("Device", "CreateBlendState", "pBlendStateDesc is nullptr");
         return E_INVALIDARG;
     }
     if (!ppBlendState) {
         ERROR("Device", "CreateBlendState", "ppBlendState is nullptr");
         return E_POINTER;
     }
     HRESULT hr = m_device->CreateBlendState(pBlendStateDesc, ppBlendState);
     if (SUCCEEDED(hr)) {
         MESSAGE("Device", "CreateBlendState", "Blend State creado correctamente.");
     }
     else {
         ERROR("Device", "CreateBlendState", ("Fallo al crear Blend State. HRESULT: " + std::to_string(hr)).c_str());
     }
     return hr;
 }

 HRESULT
 Device::CreateDepthStencilState(const D3D11_DEPTH_STENCIL_DESC* pDepthStencilDesc,
     ID3D11DepthStencilState** ppDepthStencilState) {
     if (!pDepthStencilDesc) {
         ERROR("Device", "CreateDepthStencilState", "pDepthStencilDesc is nullptr");
         return E_INVALIDARG;
     }
     if (!ppDepthStencilState) {
         ERROR("Device", "CreateDepthStencilState", "ppDepthStencilState is nullptr");
         return E_POINTER;
     }
     HRESULT hr = m_device->CreateDepthStencilState(pDepthStencilDesc, ppDepthStencilState);
     if (SUCCEEDED(hr)) {
         MESSAGE("Device", "CreateDepthStencilState", "Depth Stencil State creado correctamente.");
     }
     else {
         ERROR("Device", "CreateDepthStencilState", ("Fallo al crear Depth Stencil State. HRESULT: " + std::to_string(hr)).c_str());
     }
     return hr;
 }

 HRESULT
 Device::CreateRasterizerState(const D3D11_RASTERIZER_DESC* pRasterizerDesc,
     ID3D11RasterizerState** ppRasterizerState) {
     if (!pRasterizerDesc) {
         ERROR("Device", "CreateRasterizerState", "pRasterizerDesc is nullptr");
         return E_INVALIDARG;
     }
     if (!ppRasterizerState) {
         ERROR("Device", "CreateRasterizerState", "ppRasterizerState is nullptr");
         return E_POINTER;
     }
     HRESULT hr = m_device->CreateRasterizerState(pRasterizerDesc, ppRasterizerState);
     if (SUCCEEDED(hr)) {
         MESSAGE("Device", "CreateRasterizerState", "Rasterizer State creado correctamente.");
     }
     else {
         ERROR("Device", "CreateRasterizerState", ("Fallo al crear Rasterizer State. HRESULT: " + std::to_string(hr)).c_str());
     }
     return hr;
 }
