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
 * if (FAILED(hr)) { /* manejar error */ }
 *@endcode
     * /

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

 // ... 📌 Aquí seguirías documentando con el mismo formato
 // para CreateVertexShader, CreateInputLayout, CreatePixelShader,
 // CreateSamplerState, CreateBuffer, CreateBlendState, CreateDepthStencilState y CreateRasterizerState.
 // La idea es incluir:
 // - Qué recurso crea.
 // - Para qué se usa en un motor de videojuegos.
 // - Validaciones.
 // - Ejemplo corto si es útil.
 // - Notas o advertencias clave.
