/**
 * @file LayoutBuilder.h
 * @brief Construtor fluent para arreglos de D3D11_INPUT_ELEMENT_DESC.
 *
 * @details
 * Elimina el boilerplate de definir manualmente los arreglos de elementos
 * de Input Layout. Permite encadenar llamadas en lugar de rellenar structs a mano:
 *
 * @code
 * LayoutBuilder builder;
 * auto layout = builder
 *     .Add("POSITION",  DXGI_FORMAT_R32G32B32_FLOAT)
 *     .Add("NORMAL",    DXGI_FORMAT_R32G32B32_FLOAT)
 *     .Add("TEXCOORD",  DXGI_FORMAT_R32G32_FLOAT)
 *     .Get();
 * @endcode
 *
 * @note [GameDev] El Input Layout en DX11 es el equivalente a `glVertexAttribPointer`
 * en OpenGL: le dice a la GPU cómo interpretar los bytes del Vertex Buffer.
 * El stride (bytes por vértice) y los offsets se calculan automáticamente aquí.
 * Un error común es declarar el layout en un orden distinto al del struct de C++,
 * lo que produce datos corrompidos sin ningún error de compilación.
 *
 * @ingroup rendering
 */
#pragma once
#include "Prerequisites.h"

/**
 * @class LayoutBuilder
 * @brief Builder fluent para `std::vector<D3D11_INPUT_ELEMENT_DESC>`.
 *
 * Acumula elementos de input layout llamando sucesivamente a `Add()` o `AddInstance()`.
 * Calcula el offset en bytes automáticamente a partir del formato DXGI de cada campo.
 */
class LayoutBuilder {
public:
    /**
     * @brief Agrega un elemento de vértice (per-vertex data).
     * @param semantic      Nombre del semantic HLSL (ej. "POSITION", "NORMAL").
     * @param format        Formato DXGI del dato (ej. `DXGI_FORMAT_R32G32B32_FLOAT` = float3).
     * @param semanticIndex Índice del semantic si hay múltiples del mismo nombre (ej. TEXCOORD0, TEXCOORD1).
     * @param inputSlot     Slot del Vertex Buffer (0 = primero, default habitual).
     * @return Referencia a `*this` para encadenamiento fluent.
     */
    LayoutBuilder& Add(const char*     semantic,
                        DXGI_FORMAT     format,
                        UINT            semanticIndex = 0,
                        UINT            inputSlot     = 0) {
        D3D11_INPUT_ELEMENT_DESC elem = {};
        elem.SemanticName             = semantic;
        elem.SemanticIndex            = semanticIndex;
        elem.Format                   = format;
        elem.InputSlot                = inputSlot;
        elem.AlignedByteOffset        = D3D11_APPEND_ALIGNED_ELEMENT;
        elem.InputSlotClass           = D3D11_INPUT_PER_VERTEX_DATA;
        elem.InstanceDataStepRate     = 0;
        m_elems.push_back(elem);
        return *this;
    }

    /**
     * @brief Agrega un elemento de instancia (per-instance data).
     * @param semantic          Nombre del semantic HLSL.
     * @param format            Formato DXGI del dato.
     * @param instanceStepRate  Avanza 1 instancia por cada N draw calls (default = 1).
     * @param semanticIndex     Índice del semantic.
     * @param inputSlot         Slot del Vertex Buffer de instancia (generalmente 1).
     * @return Referencia a `*this` para encadenamiento fluent.
     *
     * @note [GameDev] El instancing permite dibujar miles de copias de un mesh
     * con un solo draw call. Cada instancia puede tener su propia matriz mundo,
     * color, etc. almacenada en un segundo Vertex Buffer (el de instancia).
     */
    LayoutBuilder& AddInstance(const char* semantic,
                                DXGI_FORMAT format,
                                UINT        instanceStepRate = 1,
                                UINT        semanticIndex    = 0,
                                UINT        inputSlot        = 1) {
        D3D11_INPUT_ELEMENT_DESC elem = {};
        elem.SemanticName             = semantic;
        elem.SemanticIndex            = semanticIndex;
        elem.Format                   = format;
        elem.InputSlot                = inputSlot;
        elem.AlignedByteOffset        = D3D11_APPEND_ALIGNED_ELEMENT;
        elem.InputSlotClass           = D3D11_INPUT_PER_INSTANCE_DATA;
        elem.InstanceDataStepRate     = instanceStepRate;
        m_elems.push_back(elem);
        return *this;
    }

    /**
     * @brief Devuelve el vector de elementos acumulados.
     * @return `const std::vector<D3D11_INPUT_ELEMENT_DESC>&` con todos los elementos.
     */
    const std::vector<D3D11_INPUT_ELEMENT_DESC>& Get() const { return m_elems; }

    /**
     * @brief Devuelve el número de elementos acumulados.
     * @return Número de elementos como `UINT`.
     */
    UINT Count() const { return static_cast<UINT>(m_elems.size()); }

private:
    std::vector<D3D11_INPUT_ELEMENT_DESC> m_elems; ///< Elementos acumulados.
};
