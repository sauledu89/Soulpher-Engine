#pragma once
#include "Prerequisites.h"

/**
 * @class DeviceContext
 * @brief Representa el contexto del dispositivo de renderizado.
 *
 * Esta clase encapsula el contexto inmediato del dispositivo Direct3D,
 * que es responsable de generar comandos de renderizado.
 */
    class
    DeviceContext {
    public:
        DeviceContext() = default;
        ~DeviceContext() = default;

        void
            init();

        void
            update();

        void
            render();

        void
            destroy();

        /**
         * @brief Establece las áreas de visualización en el destino de renderizado.
         * @param NumViewports Número de áreas de visualización a establecer.
         * @param pViewports Puntero a una matriz de estructuras D3D11_VIEWPORT.
         */
        void
            RSSetViewports(unsigned int NumViewports, const D3D11_VIEWPORT* pViewports);

        /**
         * @brief Limpia la vista de galería de símbolos de profundidad.
         * @param pDepthStencilView Puntero a la vista de galería de símbolos de profundidad a limpiar.
         * @param ClearFlags Indicadores que especifican qué partes de la vista limpiar.
         * @param Depth Valor de profundidad con el que limpiar.
         * @param Stencil Valor de galería de símbolos con el que limpiar.
         */
        void
            ClearDepthStencilView(ID3D11DepthStencilView* pDepthStencilView,
                unsigned int ClearFlags,
                float Depth,
                UINT8 Stencil);

        /**
         * @brief Limpia la vista de destino de renderizado con un color específico.
         * @param pRenderTargetView Puntero a la vista de destino de renderizado a limpiar.
         * @param ColorRGBA Matriz de cuatro componentes que representa el color con el que limpiar.
         */
        void
            ClearRenderTargetView(ID3D11RenderTargetView* pRenderTargetView,
                const float ColorRGBA[4]);

        /**
         * @brief Establece los destinos de renderizado y la vista de galería de símbolos de profundidad.
         * @param NumViews Número de vistas de destino de renderizado a establecer.
         * @param ppRenderTargetViews Puntero a una matriz de punteros a vistas de destino de renderizado.
         * @param pDepthStencilView Puntero a la vista de galería de símbolos de profundidad.
         */
        void
            OMSetRenderTargets(unsigned int NumViews,
                ID3D11RenderTargetView* const* ppRenderTargetViews,
                ID3D11DepthStencilView* pDepthStencilView);

        /**
         * @brief Establece la topología de primitivas para la etapa de ensamblador de entrada.
         * @param Topology La topología de primitivas a establecer.
         */
        void
            IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY Topology);

        /**
         * @brief Establece los recursos de sombreador de píxeles.
         * @param StartSlot El índice del primer recurso a establecer.
         * @param NumViews El número de recursos a establecer.
         * @param ppShaderResourceViews Puntero a una matriz de vistas de recursos de sombreador.
         */
        void
            PSSetShaderResources(unsigned int StartSlot, unsigned int NumViews,
                ID3D11ShaderResourceView* const* ppShaderResourceViews);

        /**
         * @brief Establece los estados de muestreo para el sombreador de píxeles.
         * @param StartSlot El índice del primer estado de muestreo a establecer.
         * @param NumSamplers El número de estados de muestreo a establecer.
         * @param ppSamplers Puntero a una matriz de estados de muestreo.
         */
        void
            PSSetSamplers(unsigned int StartSlot, unsigned int NumSamplers, ID3D11SamplerState* const* ppSamplers);

        /**
         * @brief Dibuja primitivas indexadas.
         * @param IndexCount El número de índices a dibujar.
         * @param StartIndexLocation La ubicación del primer índice a leer desde el búfer de índices.
         * @param BaseVertexLocation Un valor que se añade a cada índice antes de leer un vértice del búfer de vértices.
         */
        void
            DrawIndexed(unsigned int IndexCount, unsigned int StartIndexLocation, int BaseVertexLocation);

        /**
         * @brief Establece el estado de la galería de símbolos de profundidad.
         * @param pDepthStencilState Puntero al estado de la galería de símbolos de profundidad a establecer.
         * @param StencilRef El valor de referencia de la galería de símbolos.
         */
        void
            OMSetDepthStencilState(ID3D11DepthStencilState* pDepthStencilState, unsigned int StencilRef);

        /**
         * @brief Restablece el estado del contexto del dispositivo a los valores predeterminados.
         */
        void
            ClearState();

        /**
         * @brief Establece el diseño de entrada para la etapa de ensamblador de entrada.
         * @param pInputLayout Puntero al diseño de entrada.
         */
        void
            IASetInputLayout(ID3D11InputLayout* pInputLayout);

        /**
         * @brief Establece un sombreador de vértices en el dispositivo.
         * @param pVertexShader Puntero al sombreador de vértices.
         * @param ppClassInstances Puntero a una matriz de instancias de clase.
         * @param NumClassInstances El número de instancias de clase en la matriz.
         */
        void
            VSSetShader(ID3D11VertexShader* pVertexShader, ID3D11ClassInstance* const* ppClassInstances,
                unsigned int NumClassInstances);

        /**
         * @brief Establece un sombreador de píxeles en el dispositivo.
         * @param pPixelShader Puntero al sombreador de píxeles.
         * @param ppClassInstances Puntero a una matriz de instancias de clase.
         * @param NumClassInstances El número de instancias de clase en la matriz.
         */
        void
            PSSetShader(ID3D11PixelShader* pPixelShader, ID3D11ClassInstance* const* ppClassInstances,
                unsigned int NumClassInstances);

        /**
         * @brief Actualiza un subrecurso con nuevos datos.
         * @param pDstResource Puntero al recurso de destino.
         * @param DstSubresource El índice del subrecurso de destino.
         * @param pDstBox Puntero a un cuadro que define la región del subrecurso de destino a actualizar.
         * @param pSrcData Puntero a los datos de origen en la memoria.
         * @param SrcRowPitch El tamaño de una fila de los datos de origen.
         * @param SrcDepthPitch El tamaño de una porción de profundidad de los datos de origen.
         */
        void
            UpdateSubresource(ID3D11Resource* pDstResource,
                unsigned int DstSubresource,
                const D3D11_BOX* pDstBox,
                const void* pSrcData,
                unsigned int SrcRowPitch,
                unsigned int SrcDepthPitch);

        /**
         * @brief Vincula una matriz de búferes de vértices a la etapa de ensamblador de entrada.
         * @param StartSlot El primer búfer de entrada a vincular.
         * @param NumBuffers El número de búferes de vértices en la matriz.
         * @param ppVertexBuffers Puntero a una matriz de búferes de vértices.
         * @param pStrides Puntero a una matriz de valores de paso; un valor de paso para cada búfer en la matriz de búferes de vértices.
         * @param pOffsets Puntero a una matriz de valores de desplazamiento; un valor de desplazamiento para cada búfer en la matriz de búferes de vértices.
         */
        void
            IASetVertexBuffers(unsigned int StartSlot,
                unsigned int NumBuffers,
                ID3D11Buffer* const* ppVertexBuffers,
                const unsigned int* pStrides,
                const unsigned int* pOffsets);

        /**
         * @brief Establece los búferes de constantes utilizados por la etapa de sombreador de vértices.
         * @param StartSlot Índice en la matriz de base cero del dispositivo para comenzar a establecer los búferes de constantes.
         * @param NumBuffers Número de búferes a establecer.
         * @param ppConstantBuffers Matriz de búferes de constantes.
         */
        void
            VSSetConstantBuffers(unsigned int StartSlot, unsigned int NumBuffers, ID3D11Buffer* const* ppConstantBuffers);

        /**
         * @brief Establece los búferes de constantes utilizados por la etapa de sombreador de píxeles.
         * @param StartSlot Índice en la matriz de base cero del dispositivo para comenzar a establecer los búferes de constantes.
         * @param NumBuffers Número de búferes a establecer.
         * @param ppConstantBuffers Matriz de búferes de constantes.
         */
        void
            PSSetConstantBuffers(unsigned int StartSlot, unsigned int NumBuffers, ID3D11Buffer* const* ppConstantBuffers);

        /**
         * @brief Vincula un búfer de índices a la etapa de ensamblador de entrada.
         * @param pIndexBuffer Puntero a un búfer de índices.
         * @param Format Un DXGI_FORMAT que especifica el formato de los datos en el búfer de índices.
         * @param Offset Desplazamiento (en bytes) desde el inicio del búfer de índices hasta el primer índice a utilizar.
         */
        void
            IASetIndexBuffer(ID3D11Buffer* pIndexBuffer, DXGI_FORMAT Format, unsigned int Offset);

        /**
         * @brief Establece el estado de mezcla para la etapa de fusión de salida.
         * @param pBlendState Puntero a una interfaz de estado de mezcla.
         * @param BlendFactor Matriz de factores de mezcla, uno para cada componente RGBA.
         * @param SampleMask Una máscara de muestra de 32 bits que permite el control de todos los píxeles.
         */
        void
            OMSetBlendState(ID3D11BlendState* pBlendState, const float* BlendFactor, unsigned int SampleMask);

    private:
        ID3D11DeviceContext* m_deviceContext = nullptr;

        friend class SwapChain;
        friend class BaseApp;
};