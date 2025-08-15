/**
 * @file DeviceContext.cpp
 * @brief Implementación del contexto de dispositivo D3D11 y operaciones de pipeline.
 *
 * @details
 * Esta clase actúa como interfaz entre el CPU y la GPU para configurar y ejecutar
 * las etapas del pipeline de renderizado en DirectX 11.
 *
 * El Device Context es responsable de:
 * - Enviar comandos de dibujo (`Draw`, `DrawIndexed`).
 * - Configurar shaders, buffers, texturas y estados gráficos.
 * - Limpiar y preparar los render targets.
 *
 * @note
 * Comprender cómo usar correctamente un Device Context es clave para renderizar
 * de forma eficiente en un motor gráfico.
 */

#include "DeviceContext.h"

 /**
  * @brief Libera el contexto de dispositivo.
  *
  * @note Se debe llamar antes de cerrar la aplicación para evitar fugas de memoria.
  */
void DeviceContext::destroy() {
    SAFE_RELEASE(m_deviceContext);
}

/**
 * @brief Configura los viewports para el rasterizador.
 * @param NumViewports Número de viewports a establecer.
 * @param pViewports Arreglo con las definiciones de cada viewport.
 *
 * @note El viewport define la región de la pantalla donde se dibuja.
 */
void DeviceContext::RSSetViewports(unsigned int NumViewports,
    const D3D11_VIEWPORT* pViewports) {
    if (!pViewports) {
        ERROR("DeviceContext", "RSSetViewports", "pViewports is nullptr");
        return;
    }
    m_deviceContext->RSSetViewports(NumViewports, pViewports);
}

/**
 * @brief Asigna recursos de textura a la etapa de Pixel Shader.
 * @param StartSlot Índice de slot inicial.
 * @param NumViews Número de vistas a asignar.
 * @param ppShaderResourceViews Arreglo de vistas de recursos.
 *
 * @note Aquí se conectan las texturas para que el shader pueda muestrearlas.
 */
void DeviceContext::PSSetShaderResources(unsigned int StartSlot,
    unsigned int NumViews,
    ID3D11ShaderResourceView* const* ppShaderResourceViews) {
    if (!ppShaderResourceViews) {
        ERROR("DeviceContext", "PSSetShaderResources", "ppShaderResourceViews is nullptr");
        return;
    }
    m_deviceContext->PSSetShaderResources(StartSlot, NumViews, ppShaderResourceViews);
}

/**
 * @brief Define el Input Layout para el ensamblador de entrada.
 * @param pInputLayout Layout que describe el formato de los vértices.
 *
 * @note Debe coincidir con la estructura de vértices usada en el Vertex Shader.
 */
void DeviceContext::IASetInputLayout(ID3D11InputLayout* pInputLayout) {
    if (!pInputLayout) {
        ERROR("DeviceContext", "IASetInputLayout", "pInputLayout is nullptr");
        return;
    }
    m_deviceContext->IASetInputLayout(pInputLayout);
}

/**
 * @brief Asigna el Vertex Shader activo.
 * @param pVertexShader Shader de vértices.
 * @param ppClassInstances Instancias de clases para shaders (opcional).
 * @param NumClassInstances Número de instancias.
 */
void DeviceContext::VSSetShader(ID3D11VertexShader* pVertexShader,
    ID3D11ClassInstance* const* ppClassInstances,
    unsigned int NumClassInstances) {
    if (!pVertexShader) {
        ERROR("DeviceContext", "VSSetShader", "pVertexShader is nullptr");
        return;
    }
    m_deviceContext->VSSetShader(pVertexShader, ppClassInstances, NumClassInstances);
}

/**
 * @brief Asigna el Pixel Shader activo.
 */
void DeviceContext::PSSetShader(ID3D11PixelShader* pPixelShader,
    ID3D11ClassInstance* const* ppClassInstances,
    unsigned int NumClassInstances) {
    if (!pPixelShader) {
        ERROR("DeviceContext", "PSSetShader", "pPixelShader is nullptr");
        return;
    }
    m_deviceContext->PSSetShader(pPixelShader, ppClassInstances, NumClassInstances);
}

/**
 * @brief Copia datos desde CPU a GPU en un recurso existente.
 * @param pDstResource Recurso destino en GPU.
 * @param DstSubresource Índice de subrecurso.
 * @param pDstBox Caja opcional para actualización parcial.
 * @param pSrcData Puntero a los datos fuente en CPU.
 * @param SrcRowPitch Número de bytes por fila.
 * @param SrcDepthPitch Número de bytes por capa.
 */
void DeviceContext::UpdateSubresource(ID3D11Resource* pDstResource,
    unsigned int DstSubresource,
    const D3D11_BOX* pDstBox,
    const void* pSrcData,
    unsigned int SrcRowPitch,
    unsigned int SrcDepthPitch) {
    if (!pDstResource || !pSrcData) {
        ERROR("DeviceContext", "UpdateSubresource",
            "Invalid arguments: pDstResource or pSrcData is nullptr");
        return;
    }
    m_deviceContext->UpdateSubresource(pDstResource, DstSubresource, pDstBox, pSrcData, SrcRowPitch, SrcDepthPitch);
}

/**
 * @brief Asigna buffers de vértices al Input Assembler.
 */
void DeviceContext::IASetVertexBuffers(unsigned int StartSlot,
    unsigned int NumBuffers,
    ID3D11Buffer* const* ppVertexBuffers,
    const unsigned int* pStrides,
    const unsigned int* pOffsets) {
    if (!ppVertexBuffers || !pStrides || !pOffsets) {
        ERROR("DeviceContext", "IASetVertexBuffers",
            "Invalid arguments: ppVertexBuffers, pStrides, or pOffsets is nullptr");
        return;
    }
    m_deviceContext->IASetVertexBuffers(StartSlot, NumBuffers, ppVertexBuffers, pStrides, pOffsets);
}

/**
 * @brief Asigna el buffer de índices para dibujo indexado.
 */
void DeviceContext::IASetIndexBuffer(ID3D11Buffer* pIndexBuffer,
    DXGI_FORMAT Format,
    unsigned int Offset) {
    if (!pIndexBuffer) {
        ERROR("DeviceContext", "IASetIndexBuffer", "pIndexBuffer is nullptr");
        return;
    }
    m_deviceContext->IASetIndexBuffer(pIndexBuffer, Format, Offset);
}

/**
 * @brief Asigna estados de muestreo para el Pixel Shader.
 */
void DeviceContext::PSSetSamplers(unsigned int StartSlot,
    unsigned int NumSamplers,
    ID3D11SamplerState* const* ppSamplers) {
    if (!ppSamplers) {
        ERROR("DeviceContext", "PSSetSamplers", "ppSamplers is nullptr");
        return;
    }
    m_deviceContext->PSSetSamplers(StartSlot, NumSamplers, ppSamplers);
}

/**
 * @brief Asigna el estado del rasterizador.
 */
void DeviceContext::RSSetState(ID3D11RasterizerState* pRasterizerState) {
    if (!pRasterizerState) {
        ERROR("DeviceContext", "RSSetState", "pRasterizerState is nullptr");
        return;
    }
    m_deviceContext->RSSetState(pRasterizerState);
}

/**
 * @brief Configura el estado de mezcla de color.
 */
void DeviceContext::OMSetBlendState(ID3D11BlendState* pBlendState,
    const float BlendFactor[4],
    unsigned int SampleMask) {
    if (!pBlendState) {
        ERROR("DeviceContext", "OMSetBlendState", "pBlendState is nullptr");
        return;
    }
    m_deviceContext->OMSetBlendState(pBlendState, BlendFactor, SampleMask);
}

/**
 * @brief Asigna render targets y depth stencil para salida de color/profundidad.
 */
void DeviceContext::OMSetRenderTargets(unsigned int NumViews,
    ID3D11RenderTargetView* const* ppRenderTargetViews,
    ID3D11DepthStencilView* pDepthStencilView) {
    if (!ppRenderTargetViews && !pDepthStencilView) {
        ERROR("DeviceContext", "OMSetRenderTargets",
            "Both ppRenderTargetViews and pDepthStencilView are nullptr");
        return;
    }
    if (NumViews > 0 && !ppRenderTargetViews) {
        ERROR("DeviceContext", "OMSetRenderTargets",
            "ppRenderTargetViews is nullptr, but NumViews > 0");
        return;
    }
    m_deviceContext->OMSetRenderTargets(NumViews, ppRenderTargetViews, pDepthStencilView);
}

/**
 * @brief Configura la topología de primitivas.
 *
 * @note Ejemplos: TRIANGLELIST, LINELIST, POINTLIST.
 */
void DeviceContext::IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY Topology) {
    if (Topology == D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED) {
        ERROR("DeviceContext", "IASetPrimitiveTopology",
            "Topology is D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED");
        return;
    }
    m_deviceContext->IASetPrimitiveTopology(Topology);
}

/**
 * @brief Limpia el Render Target con un color sólido.
 */
void DeviceContext::ClearRenderTargetView(ID3D11RenderTargetView* pRenderTargetView,
    const float ColorRGBA[4]) {
    if (!pRenderTargetView) {
        ERROR("DeviceContext", "ClearRenderTargetView", "pRenderTargetView is nullptr");
        return;
    }
    if (!ColorRGBA) {
        ERROR("DeviceContext", "ClearRenderTargetView", "ColorRGBA is nullptr");
        return;
    }
    m_deviceContext->ClearRenderTargetView(pRenderTargetView, ColorRGBA);
}

/**
 * @brief Limpia el Depth Stencil.
 */
void DeviceContext::ClearDepthStencilView(ID3D11DepthStencilView* pDepthStencilView,
    unsigned int ClearFlags,
    float Depth,
    UINT8 Stencil) {
    if (!pDepthStencilView) {
        ERROR("DeviceContext", "ClearDepthStencilView", "pDepthStencilView is nullptr");
        return;
    }
    if ((ClearFlags & (D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL)) == 0) {
        ERROR("DeviceContext", "ClearDepthStencilView",
            "Invalid ClearFlags: must include D3D11_CLEAR_DEPTH or D3D11_CLEAR_STENCIL");
        return;
    }
    m_deviceContext->ClearDepthStencilView(pDepthStencilView, ClearFlags, Depth, Stencil);
}

/**
 * @brief Asigna Constant Buffers al Vertex Shader.
 */
void DeviceContext::VSSetConstantBuffers(unsigned int StartSlot,
    unsigned int NumBuffers,
    ID3D11Buffer* const* ppConstantBuffers) {
    if (!ppConstantBuffers) {
        ERROR("DeviceContext", "VSSetConstantBuffers", "ppConstantBuffers is nullptr");
        return;
    }
    m_deviceContext->VSSetConstantBuffers(StartSlot, NumBuffers, ppConstantBuffers);
}

/**
 * @brief Asigna Constant Buffers al Pixel Shader.
 */
void DeviceContext::PSSetConstantBuffers(unsigned int StartSlot,
    unsigned int NumBuffers,
    ID3D11Buffer* const* ppConstantBuffers) {
    if (!ppConstantBuffers) {
        ERROR("DeviceContext", "PSSetConstantBuffers", "ppConstantBuffers is nullptr");
        return;
    }
    m_deviceContext->PSSetConstantBuffers(StartSlot, NumBuffers, ppConstantBuffers);
}

/**
 * @brief Dibuja geometría usando índices.
 */
void DeviceContext::DrawIndexed(unsigned int IndexCount,
    unsigned int StartIndexLocation,
    int BaseVertexLocation) {
    if (IndexCount == 0) {
        ERROR("DeviceContext", "DrawIndexed", "IndexCount is zero");
        return;
    }
    m_deviceContext->DrawIndexed(IndexCount, StartIndexLocation, BaseVertexLocation);
}
