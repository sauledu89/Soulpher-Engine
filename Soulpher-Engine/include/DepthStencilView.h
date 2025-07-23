#pragma once
#include "Prerequisites.h"

class Device;
class DeviceContext;
class Texture;

/**
 * @class DepthStencilView
 * @brief Representa una vista de galería de símbolos de profundidad.
 *
 * Esta clase encapsula una vista de un recurso de textura que se puede usar como búfer de profundidad/galería de símbolos.
 */
class
    DepthStencilView {
public:
    DepthStencilView() = default;
    ~DepthStencilView() = default;

    HRESULT
        init(Device& device, Texture& depthStencil, DXGI_FORMAT format);

    void
        update();

    void
        render(DeviceContext& deviceContext);

    void
        destroy();

public:
    ID3D11DepthStencilView* m_depthStencilView = nullptr;
};