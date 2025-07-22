/**
 * @file DeviceContext.h
 * @brief Declaración de la clase DeviceContext.
 */

#pragma once
#include "Prerequisites.h"

 /**
  * @class DeviceContext
  * @brief Encapsula el contexto de dispositivo de Direct3D 11.
  *
  * Esta clase representa el "command list" activo de Direct3D 11, utilizado para emitir
  * instrucciones de renderizado al pipeline gráfico. Aquí se establecen viewports,
  * buffers, estados de render y se limpian las superficies de dibujo.
  *
  * @note En motores gráficos, cada frame se construye a través del contexto del dispositivo.
  * Controlar correctamente este objeto es clave para renderizado eficiente y correcto.
  */
class DeviceContext {
public:
    DeviceContext() = default;
    ~DeviceContext() = default;

    void init();
    void update();
    void render();
    void destroy();

    void RSSetViewports(unsigned int NumViewports, const D3D11_VIEWPORT* pViewports);
    void ClearDepthStencilView(ID3D11DepthStencilView* pDepthStencilView, unsigned int ClearFlags, float Depth, UINT8 Stencil);
    void ClearRenderTargetView(ID3D11RenderTargetView* pRenderTargetView, const float ColorRGBA[4]);
    void OMSetRenderTargets(unsigned int NumViews, ID3D11RenderTargetView* const* ppRenderTargetViews, ID3D11DepthStencilView* pDepthStencilView);

    // Declaraciones (implementadas en .cpp)
    void IASetInputLayout(ID3D11InputLayout* pInputLayout);
    void VSSetShader(ID3D11VertexShader* pShader, ID3D11ClassInstance* const* ppClassInstances, UINT NumClassInstances);
    void PSSetShader(ID3D11PixelShader* pShader, ID3D11ClassInstance* const* ppClassInstances, UINT NumClassInstances);
    void OMSetBlendState(ID3D11BlendState* pBlendState, const FLOAT BlendFactor[4], UINT SampleMask);
    void OMSetDepthStencilState(ID3D11DepthStencilState* pDepthStencilState, UINT StencilRef);
    void VSSetConstantBuffers(UINT StartSlot, UINT NumBuffers, ID3D11Buffer* const* ppConstantBuffers);
    void PSSetConstantBuffers(UINT StartSlot, UINT NumBuffers, ID3D11Buffer* const* ppConstantBuffers);
    void UpdateSubresource(ID3D11Resource* pDstResource, UINT DstSubresource, const D3D11_BOX* pDstBox, const void* pSrcData, UINT SrcRowPitch, UINT SrcDepthPitch);
    void IASetIndexBuffer(ID3D11Buffer* pIndexBuffer, DXGI_FORMAT Format, UINT Offset);
    void IASetVertexBuffers(UINT StartSlot, UINT NumBuffers, ID3D11Buffer* const* ppVertexBuffers, const UINT* pStrides, const UINT* pOffsets);

public:
    /**
     * @brief Contexto de dispositivo de Direct3D 11.
     *
     * Esta interfaz permite emitir comandos al pipeline gráfico:
     * dibujar geometría, limpiar buffers, cambiar estados, etc.
     */
    ID3D11DeviceContext* m_deviceContext = nullptr;
};
