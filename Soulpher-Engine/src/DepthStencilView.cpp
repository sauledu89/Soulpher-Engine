/**
 * @file DepthStencilView.cpp
 * @brief Implementación de la clase DepthStencilView.
 *
 * Esta clase encapsula la creación, uso y destrucción de una DepthStencilView en Direct3D 11.
 * Permite definir un buffer de profundidad y máscara de stencil que regula qué píxeles se dibujan
 * según su profundidad y otras reglas de stencil.
 */

#include "DepthStencilView.h"
#include "Device.h"
#include "DeviceContext.h"
#include "Texture.h"

 /**
  * @brief Inicializa la vista de profundidad y stencil a partir de una textura.
  *
  * Esta función configura el descriptor de la DSV (`D3D11_DEPTH_STENCIL_VIEW_DESC`) usando MSAA
  * (modo `TEXTURE2DMS`) y lo utiliza para crear una ID3D11DepthStencilView.
  *
  * @param device Objeto que contiene el ID3D11Device.
  * @param depthStencil Textura que se usará como z-buffer/stencil.
  * @param format Formato de la vista, como DXGI_FORMAT_D24_UNORM_S8_UINT.
  * @return HRESULT Resultado de la operación (S_OK si fue exitosa).
  */
HRESULT DepthStencilView::init(Device& device, Texture& depthStencil, DXGI_FORMAT format) {
    if (!device.m_device) {
        ERROR("DepthStencilView", "init", "Device is null.");
        return E_POINTER;
    }

    if (!depthStencil.m_texture) {
        ERROR("DepthStencilView", "init", "Texture is null.");
        return E_FAIL;
    }

    // Descriptor de la DSV
    D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
    memset(&descDSV, 0, sizeof(descDSV));
    descDSV.Format = format;
    descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS; // MSAA activado
    descDSV.Texture2D.MipSlice = 0;

    // Crear la vista
    HRESULT hr = device.m_device->CreateDepthStencilView(
        depthStencil.m_texture,
        &descDSV,
        &m_depthStencilView);

    if (FAILED(hr)) {
        ERROR("DepthStencilView", "init",
            ("Failed to create depth stencil view. HRESULT: " + std::to_string(hr)).c_str());
        return hr;
    }

    return S_OK;
}

/**
 * @brief Limpia el buffer de profundidad y stencil antes del renderizado.
 *
 * Este paso es esencial para evitar que se mezclen valores de profundidad de frames anteriores.
 *
 * @param deviceContext Contexto gráfico para emitir el comando de limpieza.
 */
void DepthStencilView::render(DeviceContext& deviceContext) {
    if (!deviceContext.m_deviceContext) {
        ERROR("DepthStencilView", "render", "Device context is null.");
        return;
    }

    deviceContext.m_deviceContext->ClearDepthStencilView(
        m_depthStencilView,
        D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL,
        1.0f, // Profundidad máxima (lejos)
        0     // Valor de stencil
    );
}

/**
 * @brief Libera la memoria ocupada por la DepthStencilView.
 *
 * Evita fugas de memoria al destruir la interfaz COM.
 */
void DepthStencilView::destroy() {
    SAFE_RELEASE(m_depthStencilView);
}
