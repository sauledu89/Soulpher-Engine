#pragma once
#include "Prerequisites.h"
#include "ECS\Component.h"

class DeviceContext;

/**
 * @file MeshComponent.h
 * @brief Componente ECS que almacena los datos de una malla 3D.
 *
 * @details
 * Un `MeshComponent` es un **componente de datos** dentro del sistema ECS
 * que contiene:
 * - Lista de vértices (`m_vertex`).
 * - Lista de índices (`m_index`).
 * - Cantidad de vértices e índices.
 *
 * @note
 * - Este componente **no realiza el render por sí mismo**; solo expone la información.
 * - El renderizado real se hace en otra parte del motor (normalmente en un sistema de render o dentro de un `Actor`).
 * - Se asume que los vértices usan la estructura `SimpleVertex`.
 *
 * @par Relación con ECS:
 * - Forma parte de una `Entity` o `Actor`.
 * - Puede combinarse con otros componentes como `Transform` y `Material` para formar un objeto renderizable.
 */
class MeshComponent : public Component {
public:
    /**
     * @brief Constructor por defecto.
     *
     * @note Inicializa contadores de vértices e índices a 0.
     *       El tipo de componente se establece como `ComponentType::MESH`.
     */
    MeshComponent()
        : m_numVertex(0), m_numIndex(0), Component(ComponentType::MESH) {
    }

    /** @brief Destructor virtual por defecto. */
    virtual ~MeshComponent() = default;

    /** @brief Inicializa el componente (actualmente sin implementación). */
    void init() override {}

    /**
     * @brief Actualiza el componente.
     * @param deltaTime Tiempo transcurrido desde la última actualización.
     *
     * @note Por ahora no implementa lógica; en un futuro podría usarse
     *       para animación de vértices o morph targets.
     */
    void update(float deltaTime) override {}

    /**
     * @brief Renderiza el componente.
     * @param deviceContext Contexto de render de Direct3D 11.
     *
     * @note No dibuja directamente; se deja para que sistemas externos usen sus datos.
     */
    void render(DeviceContext& deviceContext) override {}

    /** @brief Libera recursos asociados al componente (actualmente sin implementación). */
    void destroy() override {}

public:
    std::string m_name;                  ///< Nombre identificador de la malla.
    std::vector<SimpleVertex> m_vertex;  ///< Lista de vértices que definen la geometría.
    std::vector<unsigned int> m_index;   ///< Lista de índices que referencian los vértices.
    int m_numVertex;                      ///< Cantidad de vértices.
    int m_numIndex;                       ///< Cantidad de índices.
};
