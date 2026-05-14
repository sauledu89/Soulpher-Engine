#pragma once
#include "Prerequisites.h"
#include "Buffer.h"
#include "MeshComponent.h"

class Device;

/**
 * @struct Submesh
 * @brief Porcion geometrica renderizable con sus propios buffers GPU.
 *
 * Un Mesh puede estar compuesto de varios Submesh, cada uno con un slot
 * de material propio. Esto permite que un modelo FBX con multiples materiales
 * se renderice en una sola llamada a ProcessFBXMesh sin duplicar el Mesh.
 */
struct Submesh {
    Buffer       vertexBuffer;
    Buffer       indexBuffer;
    unsigned int indexCount   = 0;
    unsigned int startIndex   = 0;
    unsigned int materialSlot = 0;
};

/**
 * @class Mesh
 * @brief Coleccion de Submesh listos para renderizado.
 *
 * Reemplaza el rol del vector crudo de MeshComponent en el nuevo pipeline
 * de ForwardRenderer. Cada Submesh tiene sus propios buffers en GPU.
 */
class Mesh {
public:
    std::vector<Submesh>&       getSubmeshes()       { return m_submeshes; }
    const std::vector<Submesh>& getSubmeshes() const { return m_submeshes; }

    /**
     * @brief Crea un Mesh con buffers GPU a partir de una lista de MeshComponent (datos CPU).
     *
     * Cada MeshComponent se convierte en un Submesh con su propio VB + IB. Los slots
     * de material se asignan por orden de aparicion (0, 1, 2, ...).
     *
     * @param device     Dispositivo Direct3D activo.
     * @param components Lista de mallas CPU producidas por ModelLoader.
     * @return Mesh listo para usar en el ForwardRenderer.
     *         Si alguna submalla falla, se omite; las demas se conservan.
     */
    static Mesh buildFrom(Device& device, const std::vector<MeshComponent>& components);

    void destroy() {
        for (Submesh& sm : m_submeshes) {
            sm.vertexBuffer.destroy();
            sm.indexBuffer.destroy();
        }
        m_submeshes.clear();
    }

private:
    std::vector<Submesh> m_submeshes;
};
