#include "ModelLoader.h"
#include "MeshComponent.h"
#include "Prerequisites.h"

void
ModelLoader::init() {
}

void
ModelLoader::update() {
}

void
ModelLoader::render() {
}

void
ModelLoader::destroy() {
}

MeshComponent
ModelLoader::Load(const std::string& filename) {
    MeshComponent combinedMesh;
    objl::Loader loader;

    if (loader.LoadFile(filename)) {
        if (loader.LoadedMeshes.empty()) {
            ERROR("ModelLoader", "Load", "El archivo .obj no contiene mallas.");
            return combinedMesh;
        }

        // Combinar todas las mallas en una
        for (const auto& current_mesh : loader.LoadedMeshes) {
            unsigned int vertex_offset = static_cast<unsigned int>(combinedMesh.m_vertex.size());
            for (const auto& objlVertex : current_mesh.Vertices) {
                SimpleVertex vertex;
                vertex.Pos = XMFLOAT3(objlVertex.Position.X, objlVertex.Position.Y, objlVertex.Position.Z);
                vertex.Tex = XMFLOAT2(objlVertex.TextureCoordinate.X, 1.0f - objlVertex.TextureCoordinate.Y);
                combinedMesh.m_vertex.push_back(vertex);
            }
            for (unsigned int index : current_mesh.Indices) {
                combinedMesh.m_index.push_back(index + vertex_offset);
            }
        }

        if (!combinedMesh.m_vertex.empty()) {
            XMFLOAT3 minPos = combinedMesh.m_vertex[0].Pos;
            XMFLOAT3 maxPos = combinedMesh.m_vertex[0].Pos;
            for (const auto& vertex : combinedMesh.m_vertex) {
                minPos.x = min(minPos.x, vertex.Pos.x);
                minPos.y = min(minPos.y, vertex.Pos.y);
                minPos.z = min(minPos.z, vertex.Pos.z);
                maxPos.x = max(maxPos.x, vertex.Pos.x);
                maxPos.y = max(maxPos.y, vertex.Pos.y);
                maxPos.z = max(maxPos.z, vertex.Pos.z);
            }

            XMFLOAT3 center;
            center.x = (minPos.x + maxPos.x) / 2.0f;
            center.y = (minPos.y + maxPos.y) / 2.0f;
            center.z = (minPos.z + maxPos.z) / 2.0f;

            for (auto& vertex : combinedMesh.m_vertex) {
                vertex.Pos.x -= center.x;
                vertex.Pos.y -= center.y;
                vertex.Pos.z -= center.z;
            }
        }

        combinedMesh.m_numVertex = static_cast<int>(combinedMesh.m_vertex.size());
        combinedMesh.m_numIndex = static_cast<int>(combinedMesh.m_index.size());
        combinedMesh.m_name = "CombinedModel";
    }
    else {
        ERROR("ModelLoader", "Load", "No se pudo cargar el archivo del modelo .obj.");
    }

    return combinedMesh;
}