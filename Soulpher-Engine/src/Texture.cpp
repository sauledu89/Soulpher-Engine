// #define STB_IMAGE_IMPLEMENTATION
// #include "stb_image.h" // Comentado por ahora. Se usaría para cargar imágenes externas (como PNG o JPG).

#include "Texture.h"
#include "Device.h"
#include "DeviceContext.h"

/**
 * @brief Inicializa una textura cargada desde un archivo de imagen.
 *
 * Actualmente no está implementado (E_NOTIMPL). Normalmente aquí se cargarían
 * texturas con bibliotecas como stb_image, WIC o DirectXTex.
 */
HRESULT Texture::init(Device device,
    const std::string& textureName,
    ExtensionType extensionType)
{
    return E_NOTIMPL; // Not implemented
}

/**
 * @brief Crea una textura vacía (en blanco), útil para render targets, depth buffers, etc.
 *
 * Esta función configura los parámetros de la textura como tamaño, formato, MSAA y bind flags.
 *
 * @param device Dispositivo en el que se creará la textura.
 * @param width Ancho de la textura.
 * @param height Alto de la textura.
 * @param format Formato de píxeles (ej. DXGI_FORMAT_R8G8B8A8_UNORM).
 * @param BindFlags Bandera que define el uso de la textura (ej. como render target o recurso de shader).
 * @param sampleCount Número de muestras para antialiasing (MSAA).
 * @param qualityLevels Nivel de calidad de MSAA soportado.
 * @return HRESULT indicando éxito o error.
 */
HRESULT Texture::init(Device device, unsigned int width,
    unsigned int height, DXGI_FORMAT format,
    unsigned int BindFlags, unsigned int sampleCount,
    unsigned int qualityLevels)
{
    if (!device.m_device) {
        ERROR("Texture", "init", "Device is not initialized / is null.");
        return E_FAIL;
    }
    if (width == 0 || height == 0) {
        ERROR("Texture", "init", "Width and height must be greater than 0.");
        return E_FAIL;
    }

    // Configuramos los parámetros básicos de la textura.
    D3D11_TEXTURE2D_DESC desc;
    memset(&desc, 0, sizeof(desc));
    desc.Width = width;
    desc.Height = height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = format;

    // Forzamos el uso de MSAA con 4 muestras por ahora.
    sampleCount = 4;
    UINT sampleQuality = 0;
    device.m_device->CheckMultisampleQualityLevels(DXGI_FORMAT_R8G8B8A8_UNORM, sampleCount, &sampleQuality);
    if (sampleQuality > 0) sampleQuality -= 1; // Los niveles son de 0 a N-1
    desc.SampleDesc.Count = sampleCount;
    desc.SampleDesc.Quality = sampleQuality;

    // Indicamos que esta textura es de uso general (no dinámica).
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = BindFlags;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = 0;

    // Creamos la textura 2D.
    HRESULT hr = device.CreateTexture2D(&desc, nullptr, &m_texture);
    if (FAILED(hr)) {
        ERROR("Texture", "init", ("Failed to create texture with specified parameters. HRESULT: " + std::to_string(hr)).c_str());
        return hr;
    }

    return S_OK;
}

/**
 * @brief Crea una Shader Resource View desde una textura ya existente.
 *
 * Esto permite que la textura pueda ser usada dentro de shaders como un recurso de lectura.
 *
 * @param device Dispositivo donde se crea la vista.
 * @param textureRef Textura base desde la cual se genera la vista.
 * @param format Formato de la vista.
 * @return HRESULT indicando éxito o error.
 */
HRESULT Texture::init(Device& device,
    Texture& textureRef,
    DXGI_FORMAT format)
{
    if (!device.m_device) {
        ERROR("Texture", "init", "Device is not initialized / is null.");
        return E_FAIL;
    }
    if (!textureRef.m_texture) {
        ERROR("Texture", "init", "Texture reference is not initialized / is null.");
        return E_FAIL;
    }

    // Definimos los parámetros para crear la ShaderResourceView
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
    srvDesc.Format = format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    srvDesc.Texture2D.MostDetailedMip = 0;

    HRESULT hr = device.m_device->CreateShaderResourceView(textureRef.m_texture, &srvDesc, &m_textureFromImg);

    if (FAILED(hr)) {
        ERROR("Texture", "init", ("Failed to create shader resource view from texture reference. HRESULT: " + std::to_string(hr)).c_str());
        return hr;
    }
    return S_OK;
}

/**
 * @brief Enlaza esta textura al shader como recurso visual.
 *
 * Esto permite que la textura sea leída por el pixel shader, por ejemplo para efectos o texturizado.
 *
 * @param deviceContext Contexto del dispositivo.
 * @param StartSlot Slot inicial del shader donde se vinculará la textura.
 * @param NumViews Número de vistas a asignar.
 */
void Texture::render(DeviceContext& deviceContext,
    unsigned int StartSlot,
    unsigned int NumViews)
{
    if (!deviceContext.m_deviceContext) {
        ERROR("Texture", "render", "Device context is not initialized / is null.");
        return;
    }

    // Para asegurarnos de que no se superpongan texturas antiguas, primero liberamos el slot.
    if (m_textureFromImg) {
        ID3D11ShaderResourceView* nullSRV[] = { nullptr };
        deviceContext.m_deviceContext->PSSetShaderResources(StartSlot, NumViews, nullSRV);
        deviceContext.m_deviceContext->PSSetShaderResources(StartSlot, NumViews, &m_textureFromImg);
    }
}

/**
 * @brief Libera los recursos asociados a la textura.
 */
void Texture::destroy()
{
    if (m_texture != nullptr) {
        SAFE_RELEASE(m_texture);
    }
    if (m_textureFromImg != nullptr) {
        SAFE_RELEASE(m_textureFromImg);
    }
}
