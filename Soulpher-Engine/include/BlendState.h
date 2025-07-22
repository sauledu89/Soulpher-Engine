#pragma once
#include "Prerequisites.h"

class Device;
class DeviceContext;

/**
 * @class BlendState
 * @brief Gestiona el estado de mezcla para la etapa de fusión de salida.
 *
 * Esta clase encapsula la configuración del estado de mezcla, que controla cómo se combinan los
 * valores de píxeles del render target con los del sombreador de píxeles.
 */
class
    BlendState {
public:
    BlendState() = default;
    ~BlendState() = default;

    HRESULT
        init(Device& device);

    void
        update() {
    };

    void
        render(DeviceContext& deviceContext,
            float* blendFactor = nullptr,
            unsigned int sampleMask = 0xffffffff,
            bool reset = false);

    void
        destroy();

private:
    ID3D11BlendState* m_blendState = nullptr;
};