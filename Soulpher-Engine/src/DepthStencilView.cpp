/**
 * @file DepthStencilView.cpp
 * @brief Creación, limpieza y gestión de Depth Stencil Views (DSV) en Direct3D 11.
 *
 * @details
 * El Depth Stencil View es el recurso que permite:
 * - **Prueba de profundidad (Depth Test)**: saber qué fragmentos están más cerca de la cámara.
 * - **Prueba de stencil (Stencil Test)**: aplicar máscaras o efectos especiales.
 *
 * 🔹 Para estudiantes:
 * - El DSV se asocia normalmente a un **Depth Stencil Buffer** (textura 2D especial).
 * - En la etapa Output Merger (OM), el DSV se usa junto al Render Target View (RTV).
 * - El formato (`DXGI_FORMAT`) y el tipo (`ViewDimension`) dependen de la textura.
 *
 * @note [GameDev] En Soulpher-Engine hay DOS DSVs:
 *  1. DSV del back buffer (DepthStencilView en BaseApp) con formato D24_UNORM_S8_UINT.
 *  2. DSV del shadow map (m_shadowDSV en ForwardRenderer) con el MISMO formato,
 *     pero creado sobre una textura R24G8_TYPELESS que tambien tiene un SRV alias.
 * El truco TYPELESS permite que la GPU escriba el shadow map via DSV y luego lo lea
 * como SRV en el pixel shader, todo sobre la misma VRAM sin copiar.
 */

#include "DepthStencilView.h"
#include "Device.h"
#include "DeviceContext.h"
#include "Texture.h"

 /**
  * @brief Inicializa un Depth Stencil View (DSV) a partir de una textura de profundidad.
  * @param device       Dispositivo D3D11.
  * @param depthStencil Textura que contiene el buffer de profundidad/stencil.
  * @param format       Formato DXGI a usar (si es UNKNOWN, se toma de la textura).
  * @return HRESULT S_OK si se creó correctamente, o código de error en caso contrario.
  *
  * @note
  * - Si la textura usa **MSAA** (`SampleDesc.Count > 1`), el ViewDimension será `TEXTURE2DMS`.
  * - Si no, se usa `TEXTURE2D` con `MipSlice = 0`.
  */
HRESULT DepthStencilView::init(Device& device, Texture& depthStencil, DXGI_FORMAT format) {
    if (!device.m_device) {
        LOG_ERROR("DepthStencilView", "init", "Device is null.");
        return E_POINTER;
    }
    if (!depthStencil.m_texture) {
        LOG_ERROR("DepthStencilView", "init", "Texture is null.");
        return E_POINTER;
    }

    // Obtener descripción de la textura para decidir formato y tipo de vista
    D3D11_TEXTURE2D_DESC texDesc{};
    depthStencil.m_texture->GetDesc(&texDesc);

    D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
    dsvDesc.Format = (format == DXGI_FORMAT_UNKNOWN) ? texDesc.Format : format;

    if (texDesc.SampleDesc.Count > 1) {
        // MSAA: vista para textura multisampleada
        dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;
    }
    else {
        // Sin MSAA: vista normal con primer mip
        dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
        dsvDesc.Texture2D.MipSlice = 0;
    }

    // Crear la vista DSV
    HRESULT hr = device.m_device->CreateDepthStencilView(
        depthStencil.m_texture,
        &dsvDesc,
        &m_depthStencilView
    );

    if (FAILED(hr)) {
        LOG_ERROR("DepthStencilView", "init",
            ("Failed to create depth stencil view. HRESULT: " + std::to_string(hr)).c_str());
        return hr;
    }

    return S_OK;
}

/**
 * @brief Limpia el contenido del Depth Stencil View.
 * @param deviceContext Contexto de dispositivo D3D11.
 *
 * @note
 * - `ClearDepthStencilView` se llama normalmente al inicio de cada frame.
 * - Aquí se limpia tanto la profundidad (a 1.0f) como el stencil (a 0).
 */
void DepthStencilView::render(DeviceContext& deviceContext) {
    if (!deviceContext.m_deviceContext) {
        LOG_ERROR("DepthStencilView", "render", "Device context is null.");
        return;
    }
    if (!m_depthStencilView) {
        LOG_ERROR("DepthStencilView", "render", "DepthStencilView is null.");
        return;
    }

    // Limpia profundidad y stencil antes de renderizar
    deviceContext.m_deviceContext->ClearDepthStencilView(
        m_depthStencilView,
        D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL,
        1.0f,
        0
    );
}

/**
 * @brief Sobrecarga que permite especificar la dimensión de la vista explícitamente.
 * @param device        Dispositivo D3D11.
 * @param depthStencil  Textura de profundidad.
 * @param format        Formato DXGI del DSV.
 * @param viewDimension Dimensión del view (TEXTURE2D, TEXTURE2DMS, etc.).
 * @return HRESULT S_OK si ok.
 */
HRESULT DepthStencilView::init(Device& device, Texture& depthStencil,
                                DXGI_FORMAT format, D3D11_DSV_DIMENSION viewDimension) {
    if (!device.m_device) {
        LOG_ERROR("DepthStencilView", "init", "Device is null.");
        return E_POINTER;
    }
    if (!depthStencil.m_texture) {
        LOG_ERROR("DepthStencilView", "init", "Texture is null.");
        return E_POINTER;
    }

    D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
    dsvDesc.Format        = format;
    dsvDesc.ViewDimension = viewDimension;
    if (viewDimension == D3D11_DSV_DIMENSION_TEXTURE2D)
        dsvDesc.Texture2D.MipSlice = 0;

    HRESULT hr = device.m_device->CreateDepthStencilView(
        depthStencil.m_texture, &dsvDesc, &m_depthStencilView);
    if (FAILED(hr)) {
        LOG_ERROR("DepthStencilView", "init",
            ("Failed to create DSV (explicit dim). HRESULT: " + std::to_string(hr)).c_str());
        return hr;
    }
    return S_OK;
}

/**
 * @brief Libera los recursos asociados al Depth Stencil View.
 */
void DepthStencilView::destroy() {
    SAFE_RELEASE(m_depthStencilView);
}
