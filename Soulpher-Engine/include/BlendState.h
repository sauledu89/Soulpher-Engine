#pragma once
#include "Prerequisites.h"

class Device;
class DeviceContext;

/**
 * @class BlendState
 * @brief Gestiona el estado de mezcla para la etapa de fusi�n de salida.
 *
 * Esta clase encapsula la configuraci�n del estado de mezcla, que controla c�mo se combinan los
 * valores de p�xeles del render target con los del sombreador de p�xeles.
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