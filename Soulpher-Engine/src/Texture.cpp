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
 *
 * @note [GameDev] DDS es el formato preferido para texturas en motores DX porque ya viene
 * comprimido en GPU (BC1/BC3/BC7) y con mipmaps pre-generados: una sola llamada a
 * D3DX11CreateShaderResourceViewFromFileA crea la SRV completa. PNG/JPG requieren
 * descomprimir en CPU (stb_image) y subir como RGBA no comprimido, lo que consume mas
 * VRAM. En produccion, los assets PNG/JPG se pre-convierten a DDS durante el Cook/Build.
 * La textura del shadow map usa el init(width, height, format, flags) con formato TYPELESS
 * para que pueda ser usada como DSV (write) y SRV (read) simultaneamente.
 */

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "Texture.h"
#include "Device.h"
#include "DeviceContext.h"
#include <fstream>
#include <vector>

 // Helper local (evita macro-collisions con SAFE_RELEASE)
static void SafeRelease(IUnknown*& p) { if (p) { p->Release(); p = nullptr; } }

namespace {
// =============================================================================
// Loader de DDS minimo y propio (sin D3DX11, sin DirectXTK).
//
// D3DX11CreateShaderResourceViewFromFileA (la SDK de junio 2010 usada en el resto
// del motor) tiene soporte roto para el header extendido DX10 con formatos BC7:
// produce un acceso a memoria invalido (0xC0000005) dentro de D3DX11_43.dll al
// parsear ese tipo de archivo, sin importar si se pide como cubemap o Texture2D
// plana. En vez de depender de esa funcion para DDS, este loader parsea el header
// DDS/DX10 a mano y construye la textura directamente via CreateTexture2D +
// CreateShaderResourceView — soporta DX10 header (cualquier DXGI_FORMAT, con
// pitch calculado para BC1-BC7 comprimidos) con un unico elemento (ArraySize=1).
// =============================================================================
#pragma pack(push, 1)
struct DdsPixelFormat {
    UINT size;
    UINT flags;
    UINT fourCC;
    UINT rgbBitCount;
    UINT rBitMask;
    UINT gBitMask;
    UINT bBitMask;
    UINT aBitMask;
};

struct DdsHeader {
    UINT size;
    UINT flags;
    UINT height;
    UINT width;
    UINT pitchOrLinearSize;
    UINT depth;
    UINT mipMapCount;
    UINT reserved1[11];
    DdsPixelFormat pixelFormat;
    UINT caps;
    UINT caps2;
    UINT caps3;
    UINT caps4;
    UINT reserved2;
};

struct DdsHeaderDxt10 {
    DXGI_FORMAT dxgiFormat;
    UINT resourceDimension;
    UINT miscFlag;
    UINT arraySize;
    UINT miscFlags2;
};
#pragma pack(pop)

constexpr UINT kDdsMagic     = 0x20534444; // "DDS "
constexpr UINT kFourCcDx10   = 0x30315844; // "DX10"
constexpr UINT kDdpfFourCC   = 0x00000004; // DDS_PIXELFORMAT.flags: DDPF_FOURCC

/** @brief Bytes por bloque 4x4 para los formatos block-compressed (BC1-BC7). 0 si no es BC. */
unsigned int BytesPerBlock(DXGI_FORMAT format) {
    switch (format) {
    case DXGI_FORMAT_BC1_TYPELESS: case DXGI_FORMAT_BC1_UNORM: case DXGI_FORMAT_BC1_UNORM_SRGB:
    case DXGI_FORMAT_BC4_TYPELESS: case DXGI_FORMAT_BC4_UNORM: case DXGI_FORMAT_BC4_SNORM:
        return 8;
    case DXGI_FORMAT_BC2_TYPELESS: case DXGI_FORMAT_BC2_UNORM: case DXGI_FORMAT_BC2_UNORM_SRGB:
    case DXGI_FORMAT_BC3_TYPELESS: case DXGI_FORMAT_BC3_UNORM: case DXGI_FORMAT_BC3_UNORM_SRGB:
    case DXGI_FORMAT_BC5_TYPELESS: case DXGI_FORMAT_BC5_UNORM: case DXGI_FORMAT_BC5_SNORM:
    case DXGI_FORMAT_BC6H_TYPELESS: case DXGI_FORMAT_BC6H_UF16: case DXGI_FORMAT_BC6H_SF16:
    case DXGI_FORMAT_BC7_TYPELESS: case DXGI_FORMAT_BC7_UNORM: case DXGI_FORMAT_BC7_UNORM_SRGB:
        return 16;
    default:
        return 0;
    }
}

/** @brief bits-por-pixel de respaldo para formatos NO comprimidos (fallback razonable si no se reconoce). */
unsigned int BitsPerPixelFallback(DXGI_FORMAT format) {
    switch (format) {
    case DXGI_FORMAT_R32G32B32A32_FLOAT: case DXGI_FORMAT_R32G32B32A32_UINT:
        return 128;
    case DXGI_FORMAT_R16G16B16A16_FLOAT: case DXGI_FORMAT_R16G16B16A16_UNORM:
        return 64;
    case DXGI_FORMAT_R8G8B8A8_UNORM: case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
    case DXGI_FORMAT_B8G8R8A8_UNORM: case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
    case DXGI_FORMAT_R32_FLOAT: case DXGI_FORMAT_R32_UINT:
        return 32;
    case DXGI_FORMAT_R8_UNORM:
        return 8;
    default:
        return 32;
    }
}

/**
 * @brief Carga un .dds con header extendido DX10 y crea la textura + SRV directamente.
 * @details Solo soporta ArraySize=1 (suficiente para una panorámica/textura simple; con
 * ArraySize>1 solo se usa el primer elemento). Requiere el header DX10 — DDS "clasicos"
 * (sin extensión DX10) no están soportados por este loader minimo.
 */
HRESULT LoadDdsTexture(Device& device, const std::string& path,
                        ID3D11Texture2D*& outTexture, ID3D11ShaderResourceView*& outSRV) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        LOG_ERROR("Texture", "LoadDdsTexture", ("No se pudo abrir el archivo: " + path).c_str());
        return E_FAIL;
    }

    std::streamsize fileSize = file.tellg();
    if (fileSize < static_cast<std::streamsize>(4 + sizeof(DdsHeader))) {
        LOG_ERROR("Texture", "LoadDdsTexture", "Archivo demasiado pequeño para ser un DDS valido.");
        return E_FAIL;
    }
    file.seekg(0, std::ios::beg);

    std::vector<unsigned char> data(static_cast<size_t>(fileSize));
    if (!file.read(reinterpret_cast<char*>(data.data()), fileSize)) {
        LOG_ERROR("Texture", "LoadDdsTexture", "Fallo al leer el archivo DDS.");
        return E_FAIL;
    }

    UINT magic = 0;
    memcpy(&magic, data.data(), sizeof(UINT));
    if (magic != kDdsMagic) {
        LOG_ERROR("Texture", "LoadDdsTexture", "Magic 'DDS ' invalido.");
        return E_FAIL;
    }

    DdsHeader header{};
    memcpy(&header, data.data() + 4, sizeof(DdsHeader));
    if (header.size != 124) {
        LOG_ERROR("Texture", "LoadDdsTexture", "DDS_HEADER.size invalido.");
        return E_FAIL;
    }

    size_t offset = 4 + sizeof(DdsHeader);
    bool isDx10 = (header.pixelFormat.flags & kDdpfFourCC) != 0 &&
                  header.pixelFormat.fourCC == kFourCcDx10;
    if (!isDx10) {
        LOG_ERROR("Texture", "LoadDdsTexture",
            "DDS sin header extendido DX10 no soportado por este loader minimo.");
        return E_NOTIMPL;
    }
    if (offset + sizeof(DdsHeaderDxt10) > data.size()) {
        LOG_ERROR("Texture", "LoadDdsTexture", "Archivo truncado: falta el header DX10.");
        return E_FAIL;
    }

    DdsHeaderDxt10 dx10{};
    memcpy(&dx10, data.data() + offset, sizeof(DdsHeaderDxt10));
    offset += sizeof(DdsHeaderDxt10);

    const DXGI_FORMAT format = dx10.dxgiFormat;
    const UINT width    = header.width;
    const UINT height   = header.height;
    const UINT mipCount = (header.mipMapCount == 0) ? 1 : header.mipMapCount;
    const unsigned int blockBytes = BytesPerBlock(format);

    std::vector<D3D11_SUBRESOURCE_DATA> subresources;
    subresources.reserve(mipCount);

    size_t cursor = offset;
    UINT w = width, h = height;
    for (UINT mip = 0; mip < mipCount; ++mip) {
        UINT rowPitch, slicePitch;
        if (blockBytes > 0) {
            UINT blocksWide = (std::max)(1u, (w + 3) / 4);
            UINT blocksHigh = (std::max)(1u, (h + 3) / 4);
            rowPitch   = blocksWide * blockBytes;
            slicePitch = rowPitch * blocksHigh;
        } else {
            UINT bpp   = BitsPerPixelFallback(format);
            rowPitch   = (w * bpp + 7) / 8;
            slicePitch = rowPitch * h;
        }

        if (cursor + slicePitch > data.size()) {
            LOG_ERROR("Texture", "LoadDdsTexture", "DDS truncado: faltan datos de un nivel de mip.");
            return E_FAIL;
        }

        D3D11_SUBRESOURCE_DATA sub{};
        sub.pSysMem          = data.data() + cursor; // valido: 'data' vive hasta CreateTexture2D (mas abajo)
        sub.SysMemPitch      = rowPitch;
        sub.SysMemSlicePitch = slicePitch;
        subresources.push_back(sub);

        cursor += slicePitch;
        w = (std::max)(1u, w / 2);
        h = (std::max)(1u, h / 2);
    }

    D3D11_TEXTURE2D_DESC desc{};
    desc.Width          = width;
    desc.Height         = height;
    desc.MipLevels      = mipCount;
    desc.ArraySize      = 1;
    desc.Format         = format;
    desc.SampleDesc.Count = 1;
    desc.Usage          = D3D11_USAGE_DEFAULT;
    desc.BindFlags      = D3D11_BIND_SHADER_RESOURCE;
    desc.MiscFlags      = 0;

    HRESULT hr = device.m_device->CreateTexture2D(&desc, subresources.data(), &outTexture);
    if (FAILED(hr)) {
        LOG_ERROR("Texture", "LoadDdsTexture",
            ("CreateTexture2D fallo. hr=" + std::to_string(hr)).c_str());
        return hr;
    }

    hr = device.m_device->CreateShaderResourceView(outTexture, nullptr, &outSRV);
    if (FAILED(hr)) {
        LOG_ERROR("Texture", "LoadDdsTexture",
            ("CreateShaderResourceView fallo. hr=" + std::to_string(hr)).c_str());
        outTexture->Release();
        outTexture = nullptr;
        return hr;
    }

    return S_OK;
}
} // namespace

HRESULT
Texture::init(Device device, const std::string& textureName, ExtensionType extensionType) {
    if (!device.m_device) {
        LOG_ERROR("Texture", "init", "Device is null.");
        return E_POINTER;
    }
    if (textureName.empty()) {
        LOG_ERROR("Texture", "init", "Texture name cannot be empty.");
        return E_INVALIDARG;
    }

    // Asegúrate de partir sin recursos previos
    if (m_textureFromImg) { m_textureFromImg->Release(); m_textureFromImg = nullptr; }
    if (m_texture) { m_texture->Release();        m_texture = nullptr; }

    HRESULT hr = S_OK;

    switch (extensionType) {
    case DDS: {
        m_textureName = textureName + ".dds";

        // Loader propio (ver LoadDdsTexture arriba) — D3DX11CreateShaderResourceViewFromFileA
        // crashea con DDS de header DX10 + BC7 (formato de este proyecto), asi que se evita
        // por completo para DDS.
        hr = LoadDdsTexture(device, m_textureName, m_texture, m_textureFromImg);

        if (FAILED(hr)) {
            LOG_ERROR("Texture", "init",
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
            LOG_ERROR("Texture", "init",
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
            LOG_ERROR("Texture", "init", "Failed to create texture from PNG data");
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
            LOG_ERROR("Texture", "init", "Failed to create shader resource view for PNG texture");
            return hr;
        }
        break;
    }

    case JPG: {
        m_textureName = textureName + ".jpg";
        int width = 0, height = 0, channels = 0;

        unsigned char* data = stbi_load(m_textureName.c_str(), &width, &height, &channels, 4);
        if (!data) {
            LOG_ERROR("Texture", "init",
                ("Failed to load JPG texture: " + std::string(stbi_failure_reason())).c_str());
            return E_FAIL;
        }

        D3D11_TEXTURE2D_DESC textureDesc = {};
        textureDesc.Width              = width;
        textureDesc.Height             = height;
        textureDesc.MipLevels          = 1;
        textureDesc.ArraySize          = 1;
        textureDesc.Format             = DXGI_FORMAT_R8G8B8A8_UNORM;
        textureDesc.SampleDesc.Count   = 1;
        textureDesc.SampleDesc.Quality = 0;
        textureDesc.Usage              = D3D11_USAGE_DEFAULT;
        textureDesc.BindFlags          = D3D11_BIND_SHADER_RESOURCE;
        textureDesc.CPUAccessFlags     = 0;
        textureDesc.MiscFlags          = 0;

        D3D11_SUBRESOURCE_DATA initData = {};
        initData.pSysMem     = data;
        initData.SysMemPitch = width * 4;

        hr = device.m_device->CreateTexture2D(&textureDesc, &initData, &m_texture);
        stbi_image_free(data);

        if (FAILED(hr)) {
            LOG_ERROR("Texture", "init", "Failed to create texture from JPG data");
            return hr;
        }

        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format                    = textureDesc.Format;
        srvDesc.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels       = 1;
        srvDesc.Texture2D.MostDetailedMip = 0;

        hr = device.m_device->CreateShaderResourceView(m_texture, &srvDesc, &m_textureFromImg);
        SafeRelease(reinterpret_cast<IUnknown*&>(m_texture));

        if (FAILED(hr)) {
            LOG_ERROR("Texture", "init", "Failed to create shader resource view for JPG texture");
            return hr;
        }
        break;
    }

    default:
        LOG_ERROR("Texture", "init", "Unsupported extension type");
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
        LOG_ERROR("Texture", "init", "Device is null.");
        return E_POINTER;
    }
    if (width == 0 || height == 0) {
        LOG_ERROR("Texture", "init", "Width and height must be greater than 0");
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
        LOG_ERROR("Texture", "init",
            ("Failed to create texture with specified params. HRESULT: " + std::to_string(hr)).c_str());
        return hr;
    }

    // Nota: si necesitas SRV/RTV/DSV, créalas externamente con tus clases de View (RenderTargetView/DepthStencilView)
    return S_OK;
}

HRESULT
Texture::init(Device& device, Texture& textureRef, DXGI_FORMAT format) {
    if (!device.m_device) {
        LOG_ERROR("Texture", "init", "Device is null.");
        return E_POINTER;
    }
    if (!textureRef.m_texture) {
        LOG_ERROR("Texture", "init", "Texture is null.");
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
        LOG_ERROR("Texture", "init",
            ("Failed to create shader resource view. HRESULT: " + std::to_string(hr)).c_str());
        return hr;
    }

    return S_OK;
}

HRESULT
Texture::initCheckerboard(Device device, unsigned int size, unsigned int checkSize) {
    if (!device.m_device) {
        LOG_ERROR("Texture", "initCheckerboard", "Device is null.");
        return E_POINTER;
    }
    if (size == 0 || checkSize == 0) {
        LOG_ERROR("Texture", "initCheckerboard", "size and checkSize must be greater than 0");
        return E_INVALIDARG;
    }

    // Limpia previos
    SafeRelease(reinterpret_cast<IUnknown*&>(m_texture));
    SafeRelease(reinterpret_cast<IUnknown*&>(m_textureFromImg));
    m_textureName = "checkerboard://magenta-black";

    // Buffer RGBA en memoria: magenta (255,0,255) y "negro" alternados en bloques de checkSize.
    // NOTA: el cuadro oscuro usa (8,8,8) y no (0,0,0) puro a propósito. Este albedo pasa por
    // el pipeline PBR-lit del DeferredRenderer (albedo * luz * NdotL); un albedo (0,0,0) da
    // siempre (0,0,0) sin importar la iluminación, lo que contra el fondo oscuro del viewport
    // se ve idéntico a "transparente" aunque el cuadro sí se está dibujando (opaco). Con un
    // gris casi-negro sí refleja algo de luz y se distingue del fondo.
    std::vector<unsigned char> pixels(static_cast<size_t>(size) * size * 4);
    for (unsigned int y = 0; y < size; ++y) {
        for (unsigned int x = 0; x < size; ++x) {
            bool magenta = ((x / checkSize) + (y / checkSize)) % 2 == 0;
            unsigned char* p = &pixels[(static_cast<size_t>(y) * size + x) * 4];
            p[0] = magenta ? 255 : 8;  // R
            p[1] = magenta ? 0   : 8;  // G
            p[2] = magenta ? 255 : 8;  // B
            p[3] = 255;                  // A
        }
    }

    D3D11_TEXTURE2D_DESC textureDesc = {};
    textureDesc.Width              = size;
    textureDesc.Height             = size;
    textureDesc.MipLevels          = 1;
    textureDesc.ArraySize          = 1;
    textureDesc.Format             = DXGI_FORMAT_R8G8B8A8_UNORM;
    textureDesc.SampleDesc.Count   = 1;
    textureDesc.SampleDesc.Quality = 0;
    textureDesc.Usage              = D3D11_USAGE_DEFAULT;
    textureDesc.BindFlags          = D3D11_BIND_SHADER_RESOURCE;
    textureDesc.CPUAccessFlags     = 0;
    textureDesc.MiscFlags          = 0;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem     = pixels.data();
    initData.SysMemPitch = size * 4;

    HRESULT hr = device.m_device->CreateTexture2D(&textureDesc, &initData, &m_texture);
    if (FAILED(hr)) {
        LOG_ERROR("Texture", "initCheckerboard", "Failed to create checkerboard texture. HRESULT: " + std::to_string(hr));
        return hr;
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format                    = textureDesc.Format;
    srvDesc.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels       = 1;
    srvDesc.Texture2D.MostDetailedMip = 0;

    hr = device.m_device->CreateShaderResourceView(m_texture, &srvDesc, &m_textureFromImg);
    SafeRelease(reinterpret_cast<IUnknown*&>(m_texture));

    if (FAILED(hr)) {
        LOG_ERROR("Texture", "initCheckerboard", "Failed to create shader resource view for checkerboard texture. HRESULT: " + std::to_string(hr));
        return hr;
    }

    LOG_MESSAGE("Texture", "initCheckerboard", "Default checkerboard texture created (" + std::to_string(size) + "x" + std::to_string(size) + ")");
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
        LOG_ERROR("Texture", "render", "Device context is null.");
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
