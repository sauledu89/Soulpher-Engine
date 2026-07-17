/**
 * @file Mesh.cpp
 * @brief Implementación de Mesh::buildFrom — construcción de geometría GPU desde datos CPU.
 *
 * @details
 * Convierte una lista de `MeshComponent` (datos en RAM producidos por `ModelLoader`)
 * en un `Mesh` con buffers en VRAM listos para el pipeline de render.
 *
 * Cada `MeshComponent` se traduce a un `Submesh` con:
 *  - Un Vertex Buffer (`D3D11_BIND_VERTEX_BUFFER`) de `SimpleVertex`.
 *  - Un Index Buffer (`D3D11_BIND_INDEX_BUFFER`) de `unsigned int`.
 *
 * @note [GameDev] Este archivo es un ejemplo del patrón "Upload to GPU":
 * los datos viven en CPU (MeshComponent) hasta que se deciden subir a VRAM
 * (Mesh::buildFrom). Una vez subidos, en un motor de producción se liberaría
 * la copia CPU para ahorrar RAM. Aquí se conserva por simplicidad.
 *
 * @see Mesh, Submesh, Buffer, MeshComponent
 */

#include "Rendering/Mesh.h"
#include "Device.h"

/**
 * @brief Construye un Mesh con buffers GPU a partir de datos CPU.
 *
 * @param device     Dispositivo Direct3D activo.
 * @param components Lista de mallas CPU (una por submalla / material).
 * @return Mesh con todos los Submesh cargados en VRAM.
 *
 * @details
 * Itera los `MeshComponent`, crea VB + IB para cada uno y los agrupa en `Submesh`.
 * Las submallas que fallen se omiten para no interrumpir la carga del resto del modelo.
 */
Mesh
Mesh::buildFrom(Device& device, const std::vector<MeshComponent>& components) {
    Mesh mesh;
    mesh.m_submeshes.reserve(components.size());

    for (unsigned int slot = 0; slot < static_cast<unsigned int>(components.size()); ++slot) {
        const MeshComponent& mc = components[slot];

        if (mc.m_vertex.empty() || mc.m_index.empty()) {
            continue;
        }

        Submesh sm;
        sm.materialSlot = slot;
        sm.indexCount   = static_cast<unsigned int>(mc.m_index.size());
        sm.startIndex   = 0;

        HRESULT hr = sm.vertexBuffer.init(
            device,
            mc.m_vertex.data(),
            static_cast<unsigned int>(mc.m_vertex.size()),
            sizeof(SimpleVertex),
            D3D11_BIND_VERTEX_BUFFER);

        if (FAILED(hr)) {
            LOG_ERROR("Mesh", "buildFrom",
                ("Failed to create vertex buffer for submesh '" + mc.m_name + "'").c_str());
            continue;
        }

        hr = sm.indexBuffer.init(
            device,
            mc.m_index.data(),
            static_cast<unsigned int>(mc.m_index.size()),
            sizeof(unsigned int),
            D3D11_BIND_INDEX_BUFFER);

        if (FAILED(hr)) {
            LOG_ERROR("Mesh", "buildFrom",
                ("Failed to create index buffer for submesh '" + mc.m_name + "'").c_str());
            sm.vertexBuffer.destroy();
            continue;
        }

        mesh.m_submeshes.push_back(std::move(sm));
    }

    return mesh;
}
