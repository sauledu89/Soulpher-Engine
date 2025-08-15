/**
 * @file RenderTargetView.cpp
 * @brief Implementación para la creación, configuración y limpieza de Render Target Views (RTV) en Direct3D 11.
 *
 * @details
 * Un **Render Target View** es la interfaz que permite a Direct3D escribir en una textura como si fuera la pantalla.
 * Normalmente, el RTV principal está vinculado al **back buffer** de la swap chain, pero también se pueden crear RTV
 * personalizados para renderizado a texturas (por ejemplo, en efectos de post-proceso o reflejos).
 *
 * @note
 * Entender el RTV es clave en motores gráficos, ya que:
 *  - Controla el destino del renderizado.
 *  - Permite trabajar con renderizado diferido (G-Buffers).
 *  - Es esencial en técnicas como render-to-texture o renderizado de sombras.
 */

#include "RenderTargetView.h"
#include "Device.h"
#include "Texture.h"
#include "DeviceContext.h"
#include "DepthStencilView.h"

 /**
  * @brief Inicializa un Render Target View usando el back buffer.
  *
  * @param device       Dispositivo Direct3D 11.
  * @param backBuffer   Textura del back buffer obtenida de la swap chain.
  * @param Format       Formato deseado. Si se pasa `DXGI_FORMAT_UNKNOWN`, se infiere del recurso.
  * @return HRESULT     `S_OK` si se crea correctamente, o un código de error si falla.
  *
  * @note
  * Esta versión está pensada para inicializar el RTV principal de la aplicación.
  * Detecta automáticamente si la textura usa **MSAA** para configurar la vista como `TEXTURE2DMS`.
  */
HRESULT RenderTargetView::init(Device& device, Texture& backBuffer, DXGI_FORMAT Format) {
    if (!device.m_device) {
        ERROR("RenderTargetView", "init", "Device is nullptr.");
        return E_POINTER;
    }
    if (!backBuffer.m_texture) {
        ERROR("RenderTargetView", "init", "Texture is nullptr.");
        return E_POINTER;
    }

    D3D11_TEXTURE2D_DESC texDesc{};
    backBuffer.m_texture->GetDesc(&texDesc);

    D3D11_RENDER_TARGET_VIEW_DESC rtvDesc{};
    rtvDesc.Format = (Format == DXGI_FORMAT_UNKNOWN) ? texDesc.Format : Format;

    if (texDesc.SampleDesc.Count > 1) {
        rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMS;
    }
    else {
        rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
        rtvDesc.Texture2D.MipSlice = 0;
    }

    HRESULT hr = device.m_device->CreateRenderTargetView(backBuffer.m_texture, &rtvDesc, &m_renderTargetView);
    if (FAILED(hr)) {
        // Fallback: deja que D3D infiera la descripción
        hr = device.m_device->CreateRenderTargetView(backBuffer.m_texture, nullptr, &m_renderTargetView);
        if (FAILED(hr)) {
            ERROR("RenderTargetView", "init",
                ("Failed to create RTV. HRESULT: " + std::to_string(hr)).c_str());
            return hr;
        }
    }
    return S_OK;
}

/**
 * @brief Inicializa un RTV a partir de una textura arbitraria.
 *
 * @param device        Dispositivo Direct3D 11.
 * @param inTex         Textura de entrada donde se escribirá el render.
 * @param ViewDimension Dimensión de la vista (ej. `TEXTURE2D`, `TEXTURE2DARRAY`).
 * @param Format        Formato deseado o `DXGI_FORMAT_UNKNOWN` para inferir.
 * @return HRESULT      `S_OK` si se crea correctamente, o un código de error si falla.
 *
 * @note
 * Esta versión permite crear RTV para texturas auxiliares, ideal para:
 *  - Render-to-texture.
 *  - Renderizado de cubemaps o arrays.
 *  - Buffers para post-procesado.
 */
HRESULT RenderTargetView::init(Device& device, Texture& inTex,
    D3D11_RTV_DIMENSION ViewDimension, DXGI_FORMAT Format) {
    if (!device.m_device) {
        ERROR("RenderTargetView", "init", "Device is nullptr.");
        return E_POINTER;
    }
    if (!inTex.m_texture) {
        ERROR("RenderTargetView", "init", "Texture is nullptr.");
        return E_POINTER;
    }

    D3D11_TEXTURE2D_DESC texDesc{};
    inTex.m_texture->GetDesc(&texDesc);

    D3D11_RENDER_TARGET_VIEW_DESC desc{};
    desc.Format = (Format == DXGI_FORMAT_UNKNOWN) ? texDesc.Format : Format;
    desc.ViewDimension = ViewDimension;

    if (desc.ViewDimension == D3D11_RTV_DIMENSION_TEXTURE2D) {
        desc.Texture2D.MipSlice = 0;
    }
    else if (desc.ViewDimension == D3D11_RTV_DIMENSION_TEXTURE2DARRAY) {
        desc.Texture2DArray.MipSlice = 0;
        desc.Texture2DArray.FirstArraySlice = 0;
        desc.Texture2DArray.ArraySize = 1;
    }

    HRESULT hr = device.m_device->CreateRenderTargetView(inTex.m_texture, &desc, &m_renderTargetView);
    if (FAILED(hr)) {
        ERROR("RenderTargetView", "init",
            ("Failed to create RTV. HRESULT: " + std::to_string(hr)).c_str());
        return hr;
    }
    return S_OK;
}

/**
 * @brief Configura y limpia el RTV junto con un Depth Stencil View.
 *
 * @param deviceContext Contexto de dispositivo.
 * @param depthStencilView Depth Stencil View asociado.
 * @param numViews      Número de vistas de render (normalmente 1).
 * @param ClearColor    Color de limpieza en formato RGBA.
 *
 * @note
 * Este método:
 *  1. Asigna el RTV y el DSV al pipeline.
 *  2. Limpia ambos para preparar el siguiente frame.
 */
void RenderTargetView::render(DeviceContext& deviceContext,
    DepthStencilView& depthStencilView,
    unsigned int numViews,
    const float ClearColor[4]) {
    if (!deviceContext.m_deviceContext) {
        ERROR("RenderTargetView", "render", "DeviceContext is nullptr.");
        return;
    }
    if (!m_renderTargetView) {
        ERROR("RenderTargetView", "render", "RenderTargetView is nullptr.");
        return;
    }

    ID3D11RenderTargetView* rtv = m_renderTargetView;
    ID3D11DepthStencilView* dsv = depthStencilView.m_depthStencilView;

    deviceContext.m_deviceContext->OMSetRenderTargets(numViews, &rtv, dsv);
    deviceContext.m_deviceContext->ClearRenderTargetView(rtv, ClearColor);

    if (dsv) {
        deviceContext.m_deviceContext->ClearDepthStencilView(
            dsv, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
    }
}

/**
 * @brief Configura el RTV sin un Depth Stencil View.
 *
 * @param deviceContext Contexto de dispositivo.
 * @param numViews      Número de vistas de render.
 *
 * @note
 * Útil para casos donde no se requiere profundidad, como renderizado de HUD o post-proceso.
 */
void RenderTargetView::render(DeviceContext& deviceContext, unsigned int numViews) {
    if (!deviceContext.m_deviceContext) {
        ERROR("RenderTargetView", "render", "DeviceContext is nullptr.");
        return;
    }
    if (!m_renderTargetView) {
        ERROR("RenderTargetView", "render", "RenderTargetView is nullptr.");
        return;
    }

    ID3D11RenderTargetView* rtv = m_renderTargetView;
    deviceContext.m_deviceContext->OMSetRenderTargets(numViews, &rtv, nullptr);
}

/**
 * @brief Libera el recurso del RTV.
 *
 * @note
 * Debe llamarse al cerrar la aplicación o al cambiar de render target
 * para evitar **memory leaks**.
 */
void RenderTargetView::destroy() {
    SAFE_RELEASE(m_renderTargetView);
}
