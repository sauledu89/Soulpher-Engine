#pragma once
#include "Prerequisites.h"

class Device;
class DeviceContext;
class MeshComponent;

/**
 * @class Buffer
 * @brief Representa un búfer de datos en la GPU.
 *
 * Esta clase se utiliza para crear y gestionar búferes de vértices, índices y constantes
 * que son utilizados por el pipeline de renderizado.
 */
class
    Buffer {
public:
    Buffer() = default;
    ~Buffer() = default;

    // Inicializa Vertex e Index Buffers 
    HRESULT
        init(Device& device, const MeshComponent& mesh, unsigned int bindFlag);

    // Inicializa Constant Buffers
    HRESULT
        init(Device& device, unsigned int ByteWidth);

    // Actualiza los constant buffers a nivel logico
    void
        update(DeviceContext& deviceContext,
            ID3D11Resource* pDstResource,
            unsigned int DstSubresource,
            const D3D11_BOX* pDstBox,
            const void* pSrcData,
            unsigned int SrcRowPitch,
            unsigned int SrcDepthPitch);

    // Actualiza en render el Vertex, Index y Constant Buffer
    void
        render(DeviceContext& deviceContext,
            unsigned int StartSlot,
            unsigned int NumBuffers,
            bool setPixelShader = false,
            DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN);


    void
        destroy();

    HRESULT
        createBuffer(Device& device,
            D3D11_BUFFER_DESC& desc,
            D3D11_SUBRESOURCE_DATA* initData);


private:
    ID3D11Buffer* m_buffer = nullptr;
    unsigned int m_stride = 0;
    unsigned int m_offset = 0;
    unsigned int m_bindFlag = 0;
};