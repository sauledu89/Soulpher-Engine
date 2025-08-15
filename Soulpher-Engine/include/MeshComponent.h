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
 * - Lista de v�rtices (`m_vertex`).
 * - Lista de �ndices (`m_index`).
 * - Cantidad de v�rtices e �ndices.
 *
 * @note
 * - Este componente **no realiza el render por s� mismo**; solo expone la informaci�n.
 * - El renderizado real se hace en otra parte del motor (normalmente en un sistema de render o dentro de un `Actor`).
 * - Se asume que los v�rtices usan la estructura `SimpleVertex`.
 *
 * @par Relaci�n con ECS:
 * - Forma parte de una `Entity` o `Actor`.
 * - Puede combinarse con otros componentes como `Transform` y `Material` para formar un objeto renderizable.
 */
class MeshComponent : public Component {
public:
    /**
     * @brief Constructor por defecto.
     *
     * @note Inicializa contadores de v�rtices e �ndices a 0.
     *       El tipo de componente se establece como `ComponentType::MESH`.
     */
    MeshComponent()
        : m_numVertex(0), m_numIndex(0), Component(ComponentType::MESH) {
    }

    /** @brief Destructor virtual por defecto. */
    virtual ~MeshComponent() = default;

    /** @brief Inicializa el componente (actualmente sin implementaci�n). */
    void init() override {}

    /**
     * @brief Actualiza el componente.
     * @param deltaTime Tiempo transcurrido desde la �ltima actualizaci�n.
     *
     * @note Por ahora no implementa l�gica; en un futuro podr�a usarse
     *       para animaci�n de v�rtices o morph targets.
     */
    void update(float deltaTime) override {}

    /**
     * @brief Renderiza el componente.
     * @param deviceContext Contexto de render de Direct3D 11.
     *
     * @note No dibuja directamente; se deja para que sistemas externos usen sus datos.
     */
    void render(DeviceContext& deviceContext) override {}

    /** @brief Libera recursos asociados al componente (actualmente sin implementaci�n). */
    void destroy() override {}

public:
    std::string m_name;                  ///< Nombre identificador de la malla.
    std::vector<SimpleVertex> m_vertex;  ///< Lista de v�rtices que definen la geometr�a.
    std::vector<unsigned int> m_index;   ///< Lista de �ndices que referencian los v�rtices.
    int m_numVertex;                      ///< Cantidad de v�rtices.
    int m_numIndex;                       ///< Cantidad de �ndices.
};
