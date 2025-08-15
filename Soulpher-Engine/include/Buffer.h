#pragma once
#include "Prerequisites.h"

class Device;
class DeviceContext;
class MeshComponent;

/**
 * @file Buffer.h
 * @brief Encapsula la creación y gestión de buffers en Direct3D 11.
 *
 * @details
 * Un `Buffer` puede representar:
 * - **Vertex Buffer:** Contiene vértices que definen geometría.
 * - **Index Buffer:** Contiene índices que optimizan el renderizado.
 * - **Constant Buffer:** Contiene datos que cambian por frame o draw call (como matrices de transformación o parámetros de materiales).
 *
 * @note Para estudiantes:
 * - Los buffers son fundamentales para enviar datos de CPU a GPU.
 * - Los **Vertex/Index Buffers** suelen crearse una sola vez y usarse muchas veces.
 * - Los **Constant Buffers** se actualizan frecuentemente en cada frame.
 */
class Buffer {
public:
    /** @brief Constructor por defecto. */
    Buffer() = default;

    /** @brief Destructor por defecto. */
    ~Buffer() = default;

    /**
     * @brief Inicializa un Vertex o Index Buffer a partir de una malla.
     * @param device Dispositivo Direct3D para la creación.
     * @param mesh Datos de la malla (vértices e índices).
     * @param bindFlag Tipo de enlace del buffer (`D3D11_BIND_VERTEX_BUFFER` o `D3D11_BIND_INDEX_BUFFER`).
     * @return `S_OK` si fue exitoso, código de error en caso contrario.
     *
     * @note Los datos de `MeshComponent` se copian a GPU al crear el buffer.
     */
    HRESULT init(Device& device, const MeshComponent& mesh, unsigned int bindFlag);

    /**
     * @brief Inicializa un Constant Buffer vacío.
     * @param device Dispositivo Direct3D.
     * @param ByteWidth Tamaño en bytes del buffer (debe ser múltiplo de 16).
     * @return `S_OK` si fue exitoso, código de error en caso contrario.
     *
     * @note Usado para enviar datos dinámicos a shaders.
     */
    HRESULT init(Device& device, unsigned int ByteWidth);

    /**
     * @brief Actualiza el contenido de un Constant Buffer en memoria GPU.
     * @param deviceContext Contexto del dispositivo.
     * @param pDstResource Recurso destino (el buffer).
     * @param DstSubresource Índice de subrecurso.
     * @param pDstBox Región de destino (opcional, para actualizaciones parciales).
     * @param pSrcData Datos fuente desde CPU.
     * @param SrcRowPitch Tamaño de una fila en bytes.
     * @param SrcDepthPitch Tamaño de una capa de profundidad en bytes.
     *
     * @note Se usa para sobrescribir datos en buffers ya creados sin recrearlos.
     */
    void update(DeviceContext& deviceContext,
        ID3D11Resource* pDstResource,
        unsigned int DstSubresource,
        const D3D11_BOX* pDstBox,
        const void* pSrcData,
        unsigned int SrcRowPitch,
        unsigned int SrcDepthPitch);

    /**
     * @brief Establece el buffer para su uso en el pipeline de render.
     * @param deviceContext Contexto del dispositivo.
     * @param StartSlot Índice inicial del slot donde enlazar el buffer.
     * @param NumBuffers Número de buffers a establecer.
     * @param setPixelShader Si es `true`, también se asigna al Pixel Shader.
     * @param format Formato del índice (solo usado en Index Buffers).
     *
     * @note Este método configura el pipeline para que los shaders puedan leer el contenido del buffer.
     */
    void render(DeviceContext& deviceContext,
        unsigned int StartSlot,
        unsigned int NumBuffers,
        bool setPixelShader = false,
        DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN);

    /**
     * @brief Libera el recurso del buffer.
     *
     * @note Importante para evitar fugas de memoria GPU.
     */
    void destroy();

    /**
     * @brief Crea un buffer de Direct3D 11 con una descripción y datos iniciales.
     * @param device Dispositivo Direct3D.
     * @param desc Descripción (`D3D11_BUFFER_DESC`) que define el tipo de buffer.
     * @param initData Datos iniciales (puede ser `nullptr` para crear un buffer vacío).
     * @return `S_OK` si fue exitoso, código de error en caso contrario.
     *
     * @note Método de bajo nivel usado internamente por `init()`.
     */
    HRESULT createBuffer(Device& device,
        D3D11_BUFFER_DESC& desc,
        D3D11_SUBRESOURCE_DATA* initData);

private:
    ID3D11Buffer* m_buffer = nullptr; ///< Puntero al recurso de buffer en GPU.
    unsigned int m_stride = 0;        ///< Tamaño de cada elemento (en bytes) para Vertex Buffers.
    unsigned int m_offset = 0;        ///< Desplazamiento inicial.
    unsigned int m_bindFlag = 0;      ///< Tipo de enlace del buffer (vertex/index/constant).
};
