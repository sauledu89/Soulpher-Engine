#pragma once
#include "Prerequisites.h"

class Device;
class DeviceContext;

/**
 * @class Texture
 * @brief Representa una textura en la GPU.
 *
 * Esta clase maneja la creación y gestión de recursos de textura,
 * que pueden ser cargados desde un archivo o creados como un objetivo de renderizado.
 */
class Texture {
public:
    explicit Texture() = default;
    ~Texture() = default;

    HRESULT
        init(Device device,
            const std::string& textureName,
            ExtensionType extensionType);

    HRESULT
        init(Device device,
            unsigned int width,
            unsigned int height,
            DXGI_FORMAT Format,
            unsigned int BindFlags,
            unsigned int sampleCount = 1,
            unsigned int qualityLevels = 0);

    HRESULT
        init(Device& device, Texture& textureRef, DXGI_FORMAT format);

    void
        update();

    void
        render(DeviceContext& deviceContext,
            unsigned int StartSlot,
            unsigned int NumViews);

    void
        destroy();

public:
    ID3D11Texture2D* m_texture = nullptr;
    ID3D11ShaderResourceView* m_textureFromImg = nullptr;
};
