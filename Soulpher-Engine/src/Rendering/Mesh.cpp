#include "Rendering/Mesh.h"
#include "Device.h"

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
            ERROR("Mesh", "buildFrom",
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
            ERROR("Mesh", "buildFrom",
                ("Failed to create index buffer for submesh '" + mc.m_name + "'").c_str());
            sm.vertexBuffer.destroy();
            continue;
        }

        mesh.m_submeshes.push_back(std::move(sm));
    }

    return mesh;
}
