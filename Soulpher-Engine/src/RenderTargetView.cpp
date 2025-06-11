/**
 * @file RenderTargetView.cpp
 * @brief Implementaci�n de la clase RenderTargetView.
 *
 * Esta clase se encarga de crear y gestionar una Render Target View (RTV),
 * que es el buffer de color principal sobre el que se dibuja la escena.
 * Tambi�n permite limpiarlo y asignarlo al pipeline gr�fico.
 */

#include "RenderTargetView.h"
#include "Device.h"
#include "Texture.h"
#include "DeviceContext.h"
#include "DepthStencilView.h"

 /**
  * @brief Inicializa la RTV a partir de una textura de backbuffer.
  *
  * Se usa t�picamente para el backbuffer de la swap chain.
  * Formato y vista est�n fijos para MSAA (multisample).
  *
  * @param device Referencia al dispositivo.
  * @param backBuffer Textura sobre la cual se dibujar�.
  * @param Format Formato del color (por ejemplo DXGI_FORMAT_R8G8B8A8_UNORM).
  * @return HRESULT indicando si fue exitosa la creaci�n.
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
    if (Format == DXGI_FORMAT_UNKNOWN) {
        ERROR("RenderTargetView", "init", "Format is DXGI_FORMAT_UNKNOWN.");
        return E_INVALIDARG;
    }

    D3D11_RENDER_TARGET_VIEW_DESC desc = {};
    desc.Format = Format;
    desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMS;  // MSAA

    HRESULT hr = device.m_device->CreateRenderTargetView(backBuffer.m_texture, &desc, &m_renderTargetView);
    if (FAILED(hr)) {
        ERROR("RenderTargetView", "init",
            ("Failed to create render target view. HRESULT: " + std::to_string(hr)).c_str());
        return hr;
    }

    return S_OK;
}

/**
 * @brief Alternativa para inicializar RTV con vista personalizada.
 *
 * Permite m�s flexibilidad para usar distintas dimensiones o tipos de textura.
 *
 * @param device Dispositivo gr�fico.
 * @param inTex Textura base.
 * @param ViewDimension Tipo de vista (ej: 2D, 2DMS, array...).
 * @param Format Formato del color.
 * @return HRESULT de �xito o error.
 */
HRESULT RenderTargetView::init(Device& device,
    Texture& inTex,
    D3D11_RTV_DIMENSION ViewDimension,
    DXGI_FORMAT Format) {

    if (!device.m_device) {
        ERROR("RenderTargetView", "init", "Device is nullptr.");
        return E_POINTER;
    }
    if (!inTex.m_texture) {
        ERROR("RenderTargetView", "init", "Texture is nullptr.");
        return E_POINTER;
    }
    if (Format == DXGI_FORMAT_UNKNOWN) {
        ERROR("RenderTargetView", "init", "Format is DXGI_FORMAT_UNKNOWN.");
        return E_INVALIDARG;
    }

    D3D11_RENDER_TARGET_VIEW_DESC desc = {};
    desc.Format = Format;
    desc.ViewDimension = ViewDimension;

    HRESULT hr = device.m_device->CreateRenderTargetView(inTex.m_texture, &desc, &m_renderTargetView);
    if (FAILED(hr)) {
        ERROR("RenderTargetView", "init",
            ("Failed to create render target view. HRESULT: " + std::to_string(hr)).c_str());
        return hr;
    }

    return S_OK;
}

/**
 * @brief Limpia y asigna el render target + depth stencil al pipeline.
 *
 * Este m�todo es crucial para iniciar el dibujo del frame. Se limpia el color y
 * se asocian los buffers al Output Merger (OM).
 *
 * @param deviceContext Contexto de dispositivo.
 * @param depthStencilView Vista del buffer de profundidad.
 * @param numViews Cu�ntas RTVs se est�n asignando (usualmente 1).
 * @param ClearColor Color de fondo para limpiar el render target.
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

    deviceContext.m_deviceContext->ClearRenderTargetView(m_renderTargetView, ClearColor);
    deviceContext.m_deviceContext->OMSetRenderTargets(numViews,
        &m_renderTargetView,
        depthStencilView.m_depthStencilView);
}

/**
 * @brief Versi�n del render sin profundidad (�til para debug o efectos post-proceso).
 *
 * Solo asigna el render target al pipeline sin limpiar ni asociar Z-buffer.
 *
 * @param deviceContext Contexto del dispositivo.
 * @param numViews N�mero de RTVs.
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

    deviceContext.m_deviceContext->OMSetRenderTargets(numViews,
        &m_renderTargetView,
        nullptr);
}

/**
 * @brief Libera el recurso RTV del GPU.
 *
 * Muy importante para evitar memory leaks. Se debe llamar antes de cerrar el programa.
 */
void RenderTargetView::destroy() {
    SAFE_RELEASE(m_renderTargetView);
}
