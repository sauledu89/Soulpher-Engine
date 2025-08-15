/**
 * @file Buffer.cpp
 * @brief Gestión de Vertex/Index/Constant Buffers (creación, actualización y enlace).
 *
 * @details
 * Esta clase encapsula un recurso de buffer D3D11 y lo usa en tres modos:
 * - **Vertex Buffer**  (IASetVertexBuffers)
 * - **Index Buffer**   (IASetIndexBuffer)
 * - **Constant Buffer** (VS/PS SetConstantBuffers)
 *
 * 🔹 Consejos para estudiantes:
 * - Usa `D3D11_USAGE_DEFAULT + UpdateSubresource` cuando actualizas poco por frame.
 * - Para actualizar mucho desde CPU, preferir `D3D11_USAGE_DYNAMIC + Map/Unmap`.
 * - `m_stride` = tamaño de elemento (vértice o índice) para VB/IB; para CB guarda su ByteWidth.
 */

#include "Buffer.h"
#include "Device.h"
#include "DeviceContext.h"
#include "MeshComponent.h"

 /**
  * @brief Inicializa un Vertex o Index Buffer a partir de una malla.
  * @param device Dispositivo D3D11.
  * @param mesh   Malla con arrays de vértices/índices.
  * @param bindFlag D3D11_BIND_VERTEX_BUFFER o D3D11_BIND_INDEX_BUFFER.
  * @return HRESULT S_OK si ok.
  *
  * @note Se usa D3D11_USAGE_DEFAULT y UpdateSubresource para subidas puntuales.
  */
HRESULT Buffer::init(Device& device, const MeshComponent& mesh, unsigned int bindFlag) {
    if (!device.m_device) {
        ERROR("Buffer", "init", "Device is null.");
        return E_POINTER;
    }
    if ((bindFlag & D3D11_BIND_VERTEX_BUFFER) && mesh.m_vertex.empty()) {
        ERROR("Buffer", "init", "Vertex buffer is empty");
        return E_INVALIDARG;
    }
    if ((bindFlag & D3D11_BIND_INDEX_BUFFER) && mesh.m_index.empty()) {
        ERROR("Buffer", "init", "Index buffer is empty");
        return E_INVALIDARG;
    }

    D3D11_BUFFER_DESC desc = {};
    D3D11_SUBRESOURCE_DATA data = {};

    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.CPUAccessFlags = 0;
    m_bindFlag = bindFlag;

    if (bindFlag & D3D11_BIND_VERTEX_BUFFER) {
        m_stride = sizeof(SimpleVertex);
        desc.ByteWidth = m_stride * static_cast<unsigned int>(mesh.m_vertex.size());
        desc.BindFlags = (D3D11_BIND_FLAG)bindFlag;
        data.pSysMem = mesh.m_vertex.data();
    }
    else if (bindFlag & D3D11_BIND_INDEX_BUFFER) {
        m_stride = sizeof(unsigned int);
        desc.ByteWidth = m_stride * static_cast<unsigned int>(mesh.m_index.size());
        desc.BindFlags = (D3D11_BIND_FLAG)bindFlag;
        data.pSysMem = mesh.m_index.data();
    }

    return createBuffer(device, desc, &data);
}

/**
 * @brief Inicializa un Constant Buffer vacío.
 * @param device Dispositivo D3D11.
 * @param ByteWidth Tamaño del buffer (múltiplo de 16 bytes recomendado por D3D11).
 * @return HRESULT.
 *
 * @warning D3D11 requiere alineación a 16 bytes para constant buffers.
 */
HRESULT Buffer::init(Device& device, unsigned int ByteWidth) {
    if (!device.m_device) {
        ERROR("Buffer", "init", "Device is null.");
        return E_POINTER;
    }
    if (ByteWidth == 0) {
        ERROR("Buffer", "init", "ByteWidth is zero");
        return E_INVALIDARG;
    }
    m_stride = ByteWidth;

    D3D11_BUFFER_DESC desc = {};
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.ByteWidth = ByteWidth;
    desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    m_bindFlag = desc.BindFlags;

    return createBuffer(device, desc, nullptr);
}

/**
 * @brief Actualiza el contenido del buffer con datos desde CPU.
 * @param deviceContext Contexto de dispositivo.
 * @param pDstResource  (No usado; se mantiene por compatibilidad).
 * @param DstSubresource Subrecurso destino (normalmente 0).
 * @param pDstBox Región de actualización (nullptr = todo el recurso).
 * @param pSrcData Datos fuente.
 * @param SrcRowPitch Tamaño de fila (para texturas; 0 para buffers).
 * @param SrcDepthPitch Tamaño de profundidad (para texturas; 0 para buffers).
 *
 * @note Usa internamente `UpdateSubresource(m_buffer, ...)`.
 */
void Buffer::update(DeviceContext& deviceContext,
    ID3D11Resource* /*pDstResource*/,
    unsigned int DstSubresource,
    const D3D11_BOX* pDstBox,
    const void* pSrcData,
    unsigned int SrcRowPitch,
    unsigned int SrcDepthPitch) {
    if (!m_buffer) {
        ERROR("Buffer", "update", "m_buffer is null.");
        return;
    }
    if (!pSrcData) {
        ERROR("Buffer", "update", "pSrcData is null.");
        return;
    }

    deviceContext.m_deviceContext->UpdateSubresource(
        m_buffer, DstSubresource, pDstBox, pSrcData, SrcRowPitch, SrcDepthPitch
    );
}

/**
 * @brief Enlaza el buffer según su tipo al pipeline.
 * @param deviceContext Contexto de dispositivo.
 * @param StartSlot Slot inicial (VB/CB).
 * @param NumBuffers Número de buffers.
 * @param setPixelShader Para CB: también enlaza a PS si true.
 * @param format Formato de índice (IB): ej. DXGI_FORMAT_R32_UINT.
 */
void Buffer::render(DeviceContext& deviceContext,
    unsigned int StartSlot,
    unsigned int NumBuffers,
    bool setPixelShader,
    DXGI_FORMAT format) {
    if (!deviceContext.m_deviceContext) {
        ERROR("Buffer", "render", "DeviceContext is nullptr.");
        return;
    }
    if (!m_buffer) {
        ERROR("Buffer", "render", "m_buffer is null.");
        return;
    }

    switch (m_bindFlag) {
    case D3D11_BIND_VERTEX_BUFFER:
        deviceContext.m_deviceContext->IASetVertexBuffers(
            StartSlot, NumBuffers, &m_buffer, &m_stride, &m_offset
        );
        break;

    case D3D11_BIND_CONSTANT_BUFFER:
        deviceContext.m_deviceContext->VSSetConstantBuffers(
            StartSlot, NumBuffers, &m_buffer
        );
        if (setPixelShader) {
            deviceContext.m_deviceContext->PSSetConstantBuffers(
                StartSlot, NumBuffers, &m_buffer
            );
        }
        break;

    case D3D11_BIND_INDEX_BUFFER:
        deviceContext.m_deviceContext->IASetIndexBuffer(
            m_buffer, format, m_offset
        );
        break;

    default:
        ERROR("Buffer", "render", "Unsupported BindFlag");
        break;
    }
}

/**
 * @brief Libera el recurso de GPU.
 */
void Buffer::destroy() {
    SAFE_RELEASE(m_buffer);
}

/**
 * @brief Crea el buffer D3D11 con la descripción y datos iniciales dados.
 * @param device Dispositivo D3D11.
 * @param desc   Descriptor del buffer.
 * @param initData Datos iniciales (opcional).
 * @return HRESULT.
 */
HRESULT Buffer::createBuffer(Device& device,
    D3D11_BUFFER_DESC& desc,
    D3D11_SUBRESOURCE_DATA* initData) {
    if (!device.m_device) {
        ERROR("Buffer", "createBuffer", "Device is nullptr");
        return E_POINTER;
    }

    HRESULT hr = device.CreateBuffer(&desc, initData, &m_buffer);
    if (FAILED(hr)) {
        ERROR("Buffer", "createBuffer", "Failed to create buffer");
        return hr;
    }
    return S_OK;
}
