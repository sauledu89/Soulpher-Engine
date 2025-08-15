/**
 * @file DeviceContext.h
 * @brief Encapsula el contexto de dispositivo Direct3D 11 para The Visionary Engine.
 *
 * @details
 * El **DeviceContext** es el encargado de emitir comandos de render y configurar
 * el pipeline gráfico en Direct3D 11.
 *
 * Tipos de contextos en DX11:
 * - **Immediate Context:** Envía comandos directamente a la GPU.
 * - **Deferred Context:** Graba comandos para ser ejecutados posteriormente.
 *
 * Esta clase envuelve un `ID3D11DeviceContext` y expone métodos para:
 * - Configurar estados (rasterizer, blend, depth/stencil).
 * - Asignar shaders, buffers y texturas.
 * - Limpiar buffers.
 * - Ejecutar draw calls.
 *
 * @note Para estudiantes:
 * - El `Device` crea recursos, pero el `DeviceContext` **los usa** para dibujar.
 * - Cambiar estados o recursos frecuentemente puede afectar el rendimiento.
 * - Entender este flujo es clave para optimizar un motor 3D.
 */

#pragma once
#include "Prerequisites.h"

 /**
  * @class DeviceContext
  * @brief Maneja las operaciones de render y configuración del pipeline gráfico en Direct3D 11.
  *
  * @details
  * Proporciona funciones de alto nivel para manipular el estado del pipeline y emitir draw calls.
  * Esto incluye:
  * - Configurar viewports y topología de primitivas.
  * - Establecer shaders y sus recursos.
  * - Configurar render targets y depth/stencil.
  * - Dibujar geometría indexada o no indexada.
  */
class DeviceContext {
public:
    /** @brief Constructor por defecto. */
    DeviceContext() = default;

    /** @brief Destructor por defecto. */
    ~DeviceContext() = default;

    /** @brief Inicializa el contexto (placeholder). */
    void init();

    /** @brief Actualiza el estado del contexto (placeholder). */
    void update();

    /** @brief Ejecuta el render (placeholder). */
    void render();

    /** @brief Libera los recursos asociados al contexto. */
    void destroy();

    // === Configuración del pipeline ===

    /** @brief Configura uno o varios viewports. */
    void RSSetViewports(unsigned int NumViewports, const D3D11_VIEWPORT* pViewports);

    /** @brief Asigna texturas a un pixel shader. */
    void PSSetShaderResources(unsigned int StartSlot,
        unsigned int NumViews,
        ID3D11ShaderResourceView* const* ppShaderResourceViews);

    /** @brief Define el input layout para la etapa de ensamblado de entrada (IA). */
    void IASetInputLayout(ID3D11InputLayout* pInputLayout);

    /** @brief Asigna el vertex shader activo. */
    void VSSetShader(ID3D11VertexShader* pVertexShader,
        ID3D11ClassInstance* const* ppClassInstances,
        unsigned int NumClassInstances);

    /** @brief Asigna el pixel shader activo. */
    void PSSetShader(ID3D11PixelShader* pPixelShader,
        ID3D11ClassInstance* const* ppClassInstances,
        unsigned int NumClassInstances);

    /** @brief Actualiza un recurso de GPU con nuevos datos desde CPU. */
    void UpdateSubresource(ID3D11Resource* pDstResource,
        unsigned int DstSubresource,
        const D3D11_BOX* pDstBox,
        const void* pSrcData,
        unsigned int SrcRowPitch,
        unsigned int SrcDepthPitch);

    /** @brief Asigna uno o más vertex buffers. */
    void IASetVertexBuffers(unsigned int StartSlot,
        unsigned int NumBuffers,
        ID3D11Buffer* const* ppVertexBuffers,
        const unsigned int* pStrides,
        const unsigned int* pOffsets);

    /** @brief Asigna el index buffer activo. */
    void IASetIndexBuffer(ID3D11Buffer* pIndexBuffer,
        DXGI_FORMAT Format,
        unsigned int Offset);

    /** @brief Configura samplers para el pixel shader. */
    void PSSetSamplers(unsigned int StartSlot,
        unsigned int NumSamplers,
        ID3D11SamplerState* const* ppSamplers);

    /** @brief Establece el estado del rasterizador. */
    void RSSetState(ID3D11RasterizerState* pRasterizerState);

    /** @brief Establece el estado de mezcla (blending). */
    void OMSetBlendState(ID3D11BlendState* pBlendState,
        const float BlendFactor[4],
        unsigned int SampleMask);

    /** @brief Asigna render targets y depth stencil al pipeline. */
    void OMSetRenderTargets(unsigned int NumViews,
        ID3D11RenderTargetView* const* ppRenderTargetViews,
        ID3D11DepthStencilView* pDepthStencilView);

    /** @brief Define la topología de primitivas (triángulos, líneas, etc.). */
    void IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY Topology);

    // === Limpieza de buffers ===

    /** @brief Limpia un render target con un color específico. */
    void ClearRenderTargetView(ID3D11RenderTargetView* pRenderTargetView,
        const float ColorRGBA[4]);

    /** @brief Limpia un depth/stencil buffer. */
    void ClearDepthStencilView(ID3D11DepthStencilView* pDepthStencilView,
        unsigned int ClearFlags,
        float Depth,
        UINT8 Stencil);

    // === Constant buffers ===

    /** @brief Asigna constant buffers al vertex shader. */
    void VSSetConstantBuffers(unsigned int StartSlot,
        unsigned int NumBuffers,
        ID3D11Buffer* const* ppConstantBuffers);

    /** @brief Asigna constant buffers al pixel shader. */
    void PSSetConstantBuffers(unsigned int StartSlot,
        unsigned int NumBuffers,
        ID3D11Buffer* const* ppConstantBuffers);

    // === Draw calls ===

    /** @brief Dibuja geometría usando un index buffer. */
    void DrawIndexed(unsigned int IndexCount,
        unsigned int StartIndexLocation,
        int BaseVertexLocation);

public:
    ID3D11DeviceContext* m_deviceContext = nullptr; ///< Puntero al contexto de dispositivo Direct3D 11.
};
