/**
 * @file Texture.cpp
 * @brief Carga/creación, enlace y liberación de texturas 2D (SRV).
 *
 * @details
 * Ofrece tres sobrecargas de init():
 *  - init(Device, const std::string&, ExtensionType): carga desde DDS o PNG.
 *  - init(Device, width, height, Format, BindFlags, sampleCount, qualityLevels): crea una textura vacía.
 *  - init(Device&, Texture&, DXGI_FORMAT): crea una SRV aliasando otra textura existente.
 *
 * Incluye además update(), render() para enlazar como SRV en el PS, y destroy() para liberar recursos.
 */

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "Texture.h"
#include "Device.h"
#include "DeviceContext.h"

 // Helper local (evita macro-collisions con SAFE_RELEASE)
static void SafeRelease(IUnknown*& p) { if (p) { p->Release(); p = nullptr; } }

HRESULT
Texture::init(Device device, const std::string& textureName, ExtensionType extensionType) {
    if (!device.m_device) {
        ERROR("Texture", "init", "Device is null.");
        return E_POINTER;
    }
    if (textureName.empty()) {
        ERROR("Texture", "init", "Texture name cannot be empty.");
        return E_INVALIDARG;
    }

    // Asegúrate de partir sin recursos previos
    if (m_textureFromImg) { m_textureFromImg->Release(); m_textureFromImg = nullptr; }
    if (m_texture) { m_texture->Release();        m_texture = nullptr; }

    HRESULT hr = S_OK;

    switch (extensionType) {
    case DDS: {
        m_textureName = textureName + ".dds";

        // Carga directa a SRV (el recurso subyacente queda referenciado por la SRV)
        hr = D3DX11CreateShaderResourceViewFromFileA(
            device.m_device,
            m_textureName.c_str(),
            nullptr,
            nullptr,
            &m_textureFromImg,
            nullptr
        );

        if (FAILED(hr)) {
            ERROR("Texture", "init",
                ("Failed to load DDS texture. Verify filepath: " + m_textureName).c_str());
            return hr;
        }
        break;
    }

    case PNG: {
        m_textureName = textureName + ".png";
        int width = 0, height = 0, channels = 0;

        // Cargar PNG con stb (forzamos RGBA = 4)
        unsigned char* data = stbi_load(m_textureName.c_str(), &width, &height, &channels, 4);
        if (!data) {
            ERROR("Texture", "init",
                ("Failed to load PNG texture: " + std::string(stbi_failure_reason())).c_str());
            return E_FAIL;
        }

        // Descripción de textura 2D
        D3D11_TEXTURE2D_DESC textureDesc = {};
        textureDesc.Width = width;
        textureDesc.Height = height;
        textureDesc.MipLevels = 1;
        textureDesc.ArraySize = 1;
        textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        textureDesc.SampleDesc.Count = 1;
        textureDesc.SampleDesc.Quality = 0;
        textureDesc.Usage = D3D11_USAGE_DEFAULT;
        textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        textureDesc.CPUAccessFlags = 0;
        textureDesc.MiscFlags = 0;

        // Datos iniciales
        D3D11_SUBRESOURCE_DATA initData = {};
        initData.pSysMem = data;
        initData.SysMemPitch = width * 4;

        // Crear la textura con datos
        hr = device.m_device->CreateTexture2D(&textureDesc, &initData, &m_texture);
        stbi_image_free(data);

        if (FAILED(hr)) {
            ERROR("Texture", "init", "Failed to create texture from PNG data");
            return hr;
        }

        // Crear SRV para la textura
        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = textureDesc.Format;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = 1;
        srvDesc.Texture2D.MostDetailedMip = 0;

        hr = device.m_device->CreateShaderResourceView(m_texture, &srvDesc, &m_textureFromImg);

        // La SRV ya mantiene la referencia al recurso subyacente -> podemos soltar la ID3D11Texture2D local
        SafeRelease(reinterpret_cast<IUnknown*&>(m_texture));

        if (FAILED(hr)) {
            ERROR("Texture", "init", "Failed to create shader resource view for PNG texture");
            return hr;
        }
        break;
    }

    default:
        ERROR("Texture", "init", "Unsupported extension type");
        return E_INVALIDARG;
    }

    return hr;
}

HRESULT
Texture::init(Device device,
    unsigned int width,
    unsigned int height,
    DXGI_FORMAT Format,
    unsigned int BindFlags,
    unsigned int sampleCount,
    unsigned int qualityLevels) {
    if (!device.m_device) {
        ERROR("Texture", "init", "Device is null.");
        return E_POINTER;
    }
    if (width == 0 || height == 0) {
        ERROR("Texture", "init", "Width and height must be greater than 0");
        return E_INVALIDARG;
    }

    // Limpia previos
    SafeRelease(reinterpret_cast<IUnknown*&>(m_texture));
    SafeRelease(reinterpret_cast<IUnknown*&>(m_textureFromImg));

    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = width;
    desc.Height = height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = Format;
    desc.SampleDesc.Count = sampleCount;
    desc.SampleDesc.Quality = qualityLevels;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = BindFlags;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = 0;

    HRESULT hr = device.m_device->CreateTexture2D(&desc, nullptr, &m_texture);
    if (FAILED(hr)) {
        ERROR("Texture", "init",
            ("Failed to create texture with specified params. HRESULT: " + std::to_string(hr)).c_str());
        return hr;
    }

    // Nota: si necesitas SRV/RTV/DSV, créalas externamente con tus clases de View (RenderTargetView/DepthStencilView)
    return S_OK;
}

HRESULT
Texture::init(Device& device, Texture& textureRef, DXGI_FORMAT format) {
    if (!device.m_device) {
        ERROR("Texture", "init", "Device is null.");
        return E_POINTER;
    }
    if (!textureRef.m_texture) {
        ERROR("Texture", "init", "Texture is null.");
        return E_POINTER;
    }

    // Limpia previos
    SafeRelease(reinterpret_cast<IUnknown*&>(m_textureFromImg));

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    srvDesc.Texture2D.MostDetailedMip = 0;

    HRESULT hr = device.m_device->CreateShaderResourceView(
        textureRef.m_texture, &srvDesc, &m_textureFromImg);

    if (FAILED(hr)) {
        ERROR("Texture", "init",
            ("Failed to create shader resource view. HRESULT: " + std::to_string(hr)).c_str());
        return hr;
    }

    return S_OK;
}

void
Texture::update() {
    // no-op (placeholder para streams o actualizaciones futuras)
}

void
Texture::render(DeviceContext& deviceContext,
    unsigned int StartSlot,
    unsigned int NumViews) {
    if (!deviceContext.m_deviceContext) {
        ERROR("Texture", "render", "Device context is null.");
        return;
    }

    // Solo enlaza si hay SRV (el back-buffer no tiene SRV).
    if (m_textureFromImg) {
        // Enlazamos una sola SRV. Si en el futuro manejas varias,
        // prepara un array y pásalo con NumViews.
        ID3D11ShaderResourceView* srv = m_textureFromImg;
        deviceContext.m_deviceContext->PSSetShaderResources(StartSlot, 1, &srv);
    }
}

void
Texture::destroy() {
    // Libera ambos independientemente (no usar else-if)
    SafeRelease(reinterpret_cast<IUnknown*&>(m_textureFromImg));
    SafeRelease(reinterpret_cast<IUnknown*&>(m_texture));
}
