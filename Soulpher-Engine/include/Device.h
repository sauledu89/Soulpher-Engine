/**
 * @file Device.h
 * @brief Encapsula la creación y gestión del dispositivo Direct3D 11.
 *
 * @details
 * El `Device` es el objeto central de Direct3D 11 encargado de:
 * - Crear recursos gráficos (texturas, buffers, shaders, estados…).
 * - Comunicarse con la GPU para la preparación de estos recursos.
 *
 * @note Para estudiantes:
 * - Este objeto **no dibuja nada directamente**, solo crea recursos.
 * - Para enviar comandos de renderizado se usa `DeviceContext`.
 * - En DX11, el `ID3D11Device` es thread-safe para creación de recursos,
 *   pero el `ID3D11DeviceContext` no lo es.
 */

#pragma once
#include "Prerequisites.h"

 /**
  * @class Device
  * @brief Encapsula un `ID3D11Device` y métodos para crear recursos gráficos en Direct3D 11.
  *
  * @details
  * Esta clase abstrae las funciones de creación de recursos de Direct3D 11:
  * - Render Target Views (RTV)
  * - Depth Stencil Views (DSV)
  * - Shaders (vertex y pixel)
  * - Buffers (vertex, index, constant)
  * - Estados de renderizado (sampler, blend, depth/stencil, rasterizer)
  *
  * @note El patrón usado aquí es **wrapper de API**, lo que simplifica el código
  *       de alto nivel y permite centralizar el manejo de errores.
  */
class Device {
public:
    /** @brief Constructor por defecto. */
    Device() = default;

    /** @brief Destructor por defecto. */
    ~Device() = default;

    /**
     * @brief Inicializa el dispositivo Direct3D 11.
     *
     * @note Normalmente se invoca junto con la creación de un `SwapChain`.
     */
    void init();

    /** @brief Actualiza el estado del dispositivo (sin implementación por ahora). */
    void update();

    /** @brief Renderiza contenido usando el dispositivo (placeholder). */
    void render();

    /** @brief Libera los recursos asociados al dispositivo. */
    void destroy();

    // ==== Creación de recursos Direct3D ====

    /** @brief Crea una vista de renderizado (Render Target View, RTV). */
    HRESULT CreateRenderTargetView(ID3D11Resource* pResource,
        const D3D11_RENDER_TARGET_VIEW_DESC* pDesc,
        ID3D11RenderTargetView** ppRTView);

    /** @brief Crea una textura 2D. */
    HRESULT CreateTexture2D(const D3D11_TEXTURE2D_DESC* pDesc,
        const D3D11_SUBRESOURCE_DATA* pInitialData,
        ID3D11Texture2D** ppTexture2D);

    /** @brief Crea una vista de depth/stencil (Depth Stencil View, DSV). */
    HRESULT CreateDepthStencilView(ID3D11Resource* pResource,
        const D3D11_DEPTH_STENCIL_VIEW_DESC* pDesc,
        ID3D11DepthStencilView** ppDepthStencilView);

    /** @brief Crea un vertex shader a partir de bytecode compilado. */
    HRESULT CreateVertexShader(const void* pShaderBytecode,
        unsigned int BytecodeLength,
        ID3D11ClassLinkage* pClassLinkage,
        ID3D11VertexShader** ppVertexShader);

    /** @brief Crea un input layout para el pipeline de entrada de vértices. */
    HRESULT CreateInputLayout(const D3D11_INPUT_ELEMENT_DESC* pInputElementDescs,
        unsigned int NumElements,
        const void* pShaderBytecodeWithInputSignature,
        unsigned int BytecodeLength,
        ID3D11InputLayout** ppInputLayout);

    /** @brief Crea un pixel shader a partir de bytecode compilado. */
    HRESULT CreatePixelShader(const void* pShaderBytecode,
        unsigned int BytecodeLength,
        ID3D11ClassLinkage* pClassLinkage,
        ID3D11PixelShader** ppPixelShader);

    /** @brief Crea un buffer de Direct3D 11 (vertex, index o constant). */
    HRESULT CreateBuffer(const D3D11_BUFFER_DESC* pDesc,
        const D3D11_SUBRESOURCE_DATA* pInitialData,
        ID3D11Buffer** ppBuffer);

    /** @brief Crea un estado de muestreo (Sampler State). */
    HRESULT CreateSamplerState(const D3D11_SAMPLER_DESC* pSamplerDesc,
        ID3D11SamplerState** ppSamplerState);

    /** @brief Crea un estado de mezcla (Blend State). */
    HRESULT CreateBlendState(const D3D11_BLEND_DESC* pBlendStateDesc,
        ID3D11BlendState** ppBlendState);

    /** @brief Crea un estado de profundidad/stencil (Depth Stencil State). */
    HRESULT CreateDepthStencilState(const D3D11_DEPTH_STENCIL_DESC* pDepthStencilDesc,
        ID3D11DepthStencilState** ppDepthStencilState);

    /** @brief Crea un estado de rasterizado (Rasterizer State). */
    HRESULT CreateRasterizerState(const D3D11_RASTERIZER_DESC* pRasterizerDesc,
        ID3D11RasterizerState** ppRasterizerState);

public:
    ID3D11Device* m_device = nullptr; ///< Puntero al dispositivo Direct3D 11.
};
