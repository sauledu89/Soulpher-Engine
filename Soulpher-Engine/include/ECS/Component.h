#pragma once
#include "Prerequisites.h"
class DeviceContext;

/**
 * @file Component.h
 * @brief Clase base abstracta para todos los componentes del motor de juego.
 *
 * @details
 * Esta clase define la **interfaz mínima** que debe implementar cualquier componente
 * en un sistema basado en **ECS (Entity-Component System)**.
 *
 * Un `Component` representa datos o comportamientos que se pueden asociar a una entidad (`Entity`).
 * Por ejemplo:
 * - Componente de Transformación (posición, rotación, escala)
 * - Componente de Render (malla, materiales, shaders)
 * - Componente de Física (colisiones, fuerzas)
 *
 * @note Para estudiantes:
 * - Un `Component` **no** conoce la lógica de otros sistemas; solo maneja su propia responsabilidad.
 * - La lógica que coordina varios componentes vive en los **sistemas** del motor.
 * - Usar interfaces virtuales puras fuerza a que todas las clases hijas implementen los métodos clave (`init`, `update`, `render`, `destroy`).
 */
class Component {
public:
    /** @brief Constructor por defecto. */
    Component() = default;

    /**
     * @brief Constructor que asigna el tipo del componente.
     * @param type Enumeración `ComponentType` que identifica el tipo.
     *
     * @note Esto permite saber en tiempo de ejecución qué tipo de componente es sin usar `dynamic_cast`.
     */
    Component(const ComponentType type) : m_type(type) {}

    /** @brief Destructor virtual por defecto. */
    virtual ~Component() = default;

    /**
     * @brief Inicializa el componente.
     *
     * @note Ideal para cargar recursos, asignar memoria o configurar valores iniciales.
     */
    virtual void init() = 0;

    /**
     * @brief Actualiza el estado del componente.
     * @param deltaTime Tiempo (en segundos) desde la última actualización.
     *
     * @note Ejemplos:
     * - En un `TransformComponent`: interpolar posiciones.
     * - En un `PhysicsComponent`: aplicar gravedad.
     */
    virtual void update(float deltaTime) = 0;

    /**
     * @brief Renderiza el componente (si aplica).
     * @param deviceContext Contexto de dispositivo para emitir draw calls.
     *
     * @note Un componente de render usará esto para dibujar; uno de física podría dejarlo vacío.
     */
    virtual void render(DeviceContext& deviceContext) = 0;

    /**
     * @brief Libera recursos asociados al componente.
     *
     * @note Llamar antes de destruir la entidad para evitar fugas de memoria.
     */
    virtual void destroy() = 0;

    /**
     * @brief Obtiene el tipo del componente.
     * @return Enumeración `ComponentType` que identifica el tipo.
     */
    ComponentType getType() const { return m_type; }

protected:
    ComponentType m_type; ///< Tipo del componente, usado para identificación y casting seguro.
};
