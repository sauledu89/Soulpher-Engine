/**
 * @file Mesh.h
 * @brief Geometría GPU lista para renderizado, compuesta de submallas.
 *
 * @details
 * `Mesh` es la representación **en GPU** de la geometría de un modelo.
 * Se construye a partir de la representación CPU (`MeshComponent`) producida
 * por `ModelLoader`, subiendo los datos de vértices e índices a `Buffer` objects.
 *
 * Un mismo modelo FBX con múltiples materiales genera varias `Submesh`, cada
 * una con su propio vertex buffer, index buffer y slot de material.
 *
 * @note [GameDev] La separación CPU/GPU es fundamental en gráficos en tiempo real:
 *  - **CPU side** (`MeshComponent`): datos editables en RAM (posiciones, UVs, índices).
 *  - **GPU side** (`Mesh` / `Submesh`): datos en VRAM listos para el pipeline.
 * Una vez subidos a GPU, los datos CPU pueden descartarse para ahorrar RAM.
 * En este motor los `MeshComponent` se mantienen por conveniencia, pero en
 * producción se liberarían tras crear el `Mesh`.
 *
 * @see MeshComponent, Buffer, ForwardRenderer
 */

#pragma once
#include "Prerequisites.h"
#include "Buffer.h"
#include "MeshComponent.h"

class Device;

/**
 * @struct Submesh
 * @brief Porción geométrica renderizable con sus propios buffers GPU.
 *
 * @details
 * Un `Mesh` puede estar compuesto de varios `Submesh`, cada uno con un slot
 * de material propio. Esto permite que un modelo FBX con múltiples materiales
 * se renderice correctamente sin duplicar la geometría.
 *
 * @note [GameDev] En Unreal Engine este concepto se llama "LOD Section".
 * En Unity, cada "submesh" de un `Mesh` equivale a un rango de índices con
 * un material distinto. La idea es la misma: un modelo = N submallas = N materiales.
 */
struct Submesh {
    Buffer       vertexBuffer;               ///< Buffer de vértices en GPU.
    Buffer       indexBuffer;                ///< Buffer de índices en GPU.
    unsigned int indexCount   = 0;           ///< Número de índices a dibujar.
    unsigned int startIndex   = 0;           ///< Offset de inicio en el index buffer.
    unsigned int materialSlot = 0;           ///< Slot de material asignado (índice en el array de texturas del actor).
    XMFLOAT4X4   localTransform = {          ///< Transformación local del submesh (identidad por defecto).
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };
};

/**
 * @class Mesh
 * @brief Colección de `Submesh` listos para renderizado en GPU.
 *
 * @details
 * Reemplaza el rol del vector crudo de `MeshComponent` en el pipeline del
 * `ForwardRenderer`. Cada `Submesh` tiene sus propios buffers en VRAM.
 */
class Mesh {
public:
    /** @return Referencia mutable al vector de submallas. */
    std::vector<Submesh>&       getSubmeshes()       { return m_submeshes; }
    /** @return Referencia de solo lectura al vector de submallas. */
    const std::vector<Submesh>& getSubmeshes() const { return m_submeshes; }

    /**
     * @brief Construye un `Mesh` con buffers GPU a partir de datos CPU.
     * @param device     Dispositivo Direct3D activo.
     * @param components Lista de mallas CPU producidas por `ModelLoader`.
     * @return `Mesh` listo para usar en el `ForwardRenderer`.
     *
     * @details
     * Cada `MeshComponent` se convierte en un `Submesh` con su propio VB + IB.
     * Los slots de material se asignan por orden de aparición (0, 1, 2, …).
     * Si alguna submalla falla al crear sus buffers, se omite; las demás se conservan.
     */
    static Mesh buildFrom(Device& device, const std::vector<MeshComponent>& components);

    /**
     * @brief Libera todos los buffers GPU de las submallas.
     *
     * @warning Llamar antes de destruir el `Device` para evitar fugas de memoria GPU.
     */
    void destroy() {
        for (Submesh& sm : m_submeshes) {
            sm.vertexBuffer.destroy();
            sm.indexBuffer.destroy();
        }
        m_submeshes.clear();
    }

private:
    std::vector<Submesh> m_submeshes; ///< Submallas que componen el mesh.
};
