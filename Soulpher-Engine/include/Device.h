#pragma once
#include "Prerequisites.h"

/**
 * @file Device.h
 * @class Device
 * @brief Encapsula el dispositivo principal de Direct3D 11 (ID3D11Device).
 *
 * La clase `Device` se encarga de crear todos los recursos gr�ficos necesarios
 * para el motor, incluyendo texturas, buffers, shaders, layouts y estados.
 *
 * Su objetivo es centralizar las llamadas a la API de Direct3D, aislando el
 * resto del motor de detalles bajos del hardware o la API de renderizado.
 *
 * @note En motores reales como Unreal o Unity, una clase similar controla la creaci�n
 * y administraci�n de recursos gr�ficos en m�ltiples plataformas.
 */
class Device {
public:
    /**
     * @brief Constructor por defecto.
     */
    Device() = default;

    /**
     * @brief Destructor por defecto.
     */
    ~Device() = default;

    /**
     * @brief Inicializa el dispositivo si se requiere l�gica personalizada.
     */
    void init();

    /**
     * @brief Actualiza el estado del dispositivo por cuadro (sin uso por defecto).
     */
    void update();

    /**
     * @brief Renderiza acciones espec�ficas del dispositivo (vac�o por defecto).
     */
    void render();

    /**
     * @brief Libera todos los recursos asociados al dispositivo.
     */
    void destroy();

    // ======================= M�TODOS DE CREACI�N DE RECURSOS =======================

    /**
     * @brief Crea una vista de renderizado desde un recurso (RenderTargetView).
     *
     * @param pResource Recurso base (t�picamente una textura).
     * @param pDesc Descripci�n de la vista (formato, dimensi�n, etc.).
     * @param ppRTView Salida: puntero a la vista creada.
     * @return HRESULT C�digo de �xito o error.
     */
    HRESULT CreateRenderTargetView(ID3D11Resource* pResource,
        const D3D11_RENDER_TARGET_VIEW_DESC* pDesc,
        ID3D11RenderTargetView** ppRTView);

    /**
     * @brief Crea una textura 2D.
     *
     * @param pDesc Descripci�n de la textura (tama�o, formato, uso).
     * @param pInitialData Datos iniciales opcionales (por ejemplo, un mapa de bits cargado).
     * @param ppTexture2D Salida: puntero a la textura creada.
     * @return HRESULT C�digo de �xito o error.
     */
    HRESULT CreateTexture2D(const D3D11_TEXTURE2D_DESC* pDesc,
        const D3D11_SUBRESOURCE_DATA* pInitialData,
        ID3D11Texture2D** ppTexture2D);

    /**
     * @brief Crea una vista de profundidad y stencil (DepthStencilView).
     *
     * @param pResource Recurso base (como textura de profundidad).
     * @param pDesc Descripci�n de la vista.
     * @param ppDepthStencilView Salida: puntero a la vista creada.
     * @return HRESULT C�digo de �xito o error.
     */
    HRESULT CreateDepthStencilView(ID3D11Resource* pResource,
        const D3D11_DEPTH_STENCIL_VIEW_DESC* pDesc,
        ID3D11DepthStencilView** ppDepthStencilView);

    /**
     * @brief Crea un shader de v�rtices.
     *
     * @param pShaderBytecode Bytecode compilado del shader.
     * @param BytecodeLength Tama�o del bytecode en bytes.
     * @param pClassLinkage (Opcional) Soporte para herencia de shaders.
     * @param ppVertexShader Salida: puntero al shader creado.
     * @return HRESULT C�digo de �xito o error.
     */
    HRESULT CreateVertexShader(const void* pShaderBytecode,
        unsigned int BytecodeLength,
        ID3D11ClassLinkage* pClassLinkage,
        ID3D11VertexShader** ppVertexShader);

    /**
     * @brief Crea un Input Layout que define c�mo se leen los v�rtices desde un buffer.
     *
     * @param pInputElementDescs Arreglo con la descripci�n de cada atributo del v�rtice.
     * @param NumElements N�mero de elementos en el layout.
     * @param pShaderBytecodeWithInputSignature Bytecode que contiene la firma del shader.
     * @param BytecodeLength Tama�o del bytecode.
     * @param ppInputLayout Salida: puntero al layout creado.
     * @return HRESULT C�digo de �xito o error.
     */
    HRESULT CreateInputLayout(const D3D11_INPUT_ELEMENT_DESC* pInputElementDescs,
        unsigned int NumElements,
        const void* pShaderBytecodeWithInputSignature,
        unsigned int BytecodeLength,
        ID3D11InputLayout** ppInputLayout);

    /**
     * @brief Crea un shader de p�xeles.
     *
     * @param pShaderBytecode Bytecode compilado del shader.
     * @param BytecodeLength Tama�o del bytecode.
     * @param pClassLinkage (Opcional) Enlace de clases para shaders din�micos.
     * @param ppPixelShader Salida: puntero al shader creado.
     * @return HRESULT C�digo de �xito o error.
     */
    HRESULT CreatePixelShader(const void* pShaderBytecode,
        unsigned int BytecodeLength,
        ID3D11ClassLinkage* pClassLinkage,
        ID3D11PixelShader** ppPixelShader);

    /**
     * @brief Crea un buffer (vertex, index o constant).
     *
     * @param pDesc Descripci�n del buffer.
     * @param pInitialData Datos iniciales opcionales.
     * @param ppBuffer Salida: puntero al buffer creado.
     * @return HRESULT C�digo de �xito o error.
     */
    HRESULT CreateBuffer(const D3D11_BUFFER_DESC* pDesc,
        const D3D11_SUBRESOURCE_DATA* pInitialData,
        ID3D11Buffer** ppBuffer);

    /**
     * @brief Crea un estado de muestreo (Sampler State) para texturas.
     *
     * @param pSamplerDesc Descripci�n del muestreo (filtros, direcci�n, etc.).
     * @param ppSamplerState Salida: puntero al estado creado.
     * @return HRESULT C�digo de �xito o error.
     */
    HRESULT CreateSamplerState(const D3D11_SAMPLER_DESC* pSamplerDesc,
        ID3D11SamplerState** ppSamplerState);

    /**
     * @brief Crea un estado de mezcla (Blend State) para transparencia.
     *
     * @param pBlendStateDesc Descripci�n del modo de mezcla.
     * @param ppBlendState Salida: puntero al estado creado.
     * @return HRESULT C�digo de �xito o error.
     */
    HRESULT CreateBlendState(const D3D11_BLEND_DESC* pBlendStateDesc,
        ID3D11BlendState** ppBlendState);

    /**
     * @brief Crea un estado de profundidad y stencil.
     *
     * @param pDepthStencilDesc Descripci�n de configuraci�n del z-buffer y stencil.
     * @param ppDepthStencilState Salida: puntero al estado creado.
     * @return HRESULT C�digo de �xito o error.
     */
    HRESULT CreateDepthStencilState(const D3D11_DEPTH_STENCIL_DESC* pDepthStencilDesc,
        ID3D11DepthStencilState** ppDepthStencilState);

    /**
     * @brief Crea un estado de rasterizado.
     *
     * Controla c�mo se dibujan los tri�ngulos (relleno s�lido, l�neas, backface culling, etc.).
     *
     * @param pRasterizerDesc Descripci�n de la pol�tica de rasterizado.
     * @param ppRasterizerState Salida: puntero al estado creado.
     * @return HRESULT C�digo de �xito o error.
     */
    HRESULT CreateRasterizerState(const D3D11_RASTERIZER_DESC* pRasterizerDesc,
        ID3D11RasterizerState** ppRasterizerState);

    /**
     * @brief Dispositivo principal de Direct3D 11.
     *
     * Utilizado para todas las operaciones de creaci�n de recursos gr�ficos.
     *
     * @note Esta interfaz representa la conexi�n con la GPU a nivel l�gico.
     */
    ID3D11Device* m_device = nullptr;
};
