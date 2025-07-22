#pragma once
#include "Prerequisites.h"

class Device;
class DeviceContext;

/**
 * @class InputLayout
 * @brief Define el formato de los datos de v�rtice de entrada.
 *
 * Esta clase describe c�mo los datos de v�rtice del b�fer de entrada se asignan a los registros de entrada del sombreador de v�rtices.
 */
class
    InputLayout {
public:
    InputLayout() = default;
    ~InputLayout() = default;

    HRESULT
        init(Device& device,
            std::vector<D3D11_INPUT_ELEMENT_DESC>& Layout,
            ID3DBlob* VertexShaderData);

    void
        update();

    void
        render(DeviceContext& deviceContext);

    void
        destroy();

public:
    ID3D11InputLayout* m_inputLayout = nullptr;
};