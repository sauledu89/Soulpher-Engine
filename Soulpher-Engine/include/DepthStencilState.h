#pragma once    
#include "Prerequisites.h"

class Device;
class DeviceContext;

/**
 * @class DepthStencilState
 * @brief Gestiona el estado de la prueba de profundidad y galería de símbolos.
 *
 * Esta clase encapsula la configuración del estado de profundidad/galería de símbolos,
 * que controla cómo se realizan las pruebas de profundidad y las operaciones de galería de símbolos.
 */
class
    DepthStencilState {
public:
    DepthStencilState() = default;
    ~DepthStencilState() = default;

    HRESULT
        init(Device& device, bool enableDepth = true, bool enableStencil = false);

    void
        update();

    void
        render(DeviceContext& deviceContext, unsigned int stencilRef = 0, bool reset = false);

    void
        destroy();

private:
    ID3D11DepthStencilState* m_depthStencilState = nullptr;
};