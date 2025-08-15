#pragma once
#include "Prerequisites.h"
#include "Component.h"

class DeviceContext;

/**
 * @file Entity.h
 * @brief Clase base para todas las entidades del motor de juego.
 *
 * @details
 * Una `Entity` es el núcleo del patrón **ECS (Entity-Component System)**:
 * - Contiene una colección de `Component` que definen sus datos y comportamientos.
 * - No implementa lógica directamente, sino que delega en sus componentes.
 * - Funciona como "contenedor" para combinar múltiples aspectos (render, física, audio, IA).
 *
 * @note Para estudiantes:
 * - Las entidades por sí mismas son “vacías”; la funcionalidad se logra a través de componentes.
 * - Esto permite reutilizar código y crear variaciones de entidades simplemente cambiando sus componentes.
 * - Ejemplo: un "enemigo" y un "jugador" pueden compartir el mismo `PhysicsComponent` pero tener distinto `AIComponent`.
 */
class Entity {
public:
    /** @brief Constructor por defecto. */
    Entity() = default;

    /** @brief Destructor virtual por defecto. */
    virtual ~Entity() = default;

    /**
     * @brief Inicializa la entidad.
     *
     * @note Llamar antes de `update()` o `render()` para garantizar que los componentes estén listos.
     */
    virtual void init() = 0;

    /**
     * @brief Actualiza la lógica de la entidad.
     * @param deltaTime Tiempo en segundos desde la última actualización.
     * @param deviceContext Contexto de dispositivo (render u operaciones gráficas).
     *
     * @note Ideal para actualizar todos los componentes asociados (física, animaciones, IA).
     */
    virtual void update(float deltaTime, DeviceContext& deviceContext) = 0;

    /**
     * @brief Renderiza la entidad.
     * @param deviceContext Contexto del dispositivo para operaciones gráficas.
     *
     * @note Normalmente delega a los componentes gráficos de la entidad.
     */
    virtual void render(DeviceContext& deviceContext) = 0;

    /**
     * @brief Destruye la entidad y libera sus recursos.
     *
     * @note Asegura la liberación de memoria GPU/CPU asociada a sus componentes.
     */
    virtual void destroy() = 0;

    /**
     * @brief Agrega un componente a la entidad.
     * @tparam T Tipo de componente (debe heredar de `Component`).
     * @param component Puntero compartido al componente.
     *
     * @code
     * auto transform = EU::MakeShared<TransformComponent>();
     * myEntity.addComponent(transform);
     * @endcode
     */
    template <typename T>
    void addComponent(EU::TSharedPointer<T> component) {
        static_assert(std::is_base_of<Component, T>::value, "T must be derived from Component");
        m_components.push_back(component.template dynamic_pointer_cast<Component>());
    }

    /**
     * @brief Obtiene un componente por tipo.
     * @tparam T Tipo del componente deseado.
     * @return Puntero compartido al componente, o `nullptr` si no se encuentra.
     *
     * @code
     * auto transform = myEntity.getComponent<TransformComponent>();
     * if (transform) { transform->setPosition({0, 0, 0}); }
     * @endcode
     */
    template<typename T>
    EU::TSharedPointer<T> getComponent() {
        for (auto& component : m_components) {
            EU::TSharedPointer<T> specificComponent = component.template dynamic_pointer_cast<T>();
            if (specificComponent) {
                return specificComponent;
            }
        }
        return EU::TSharedPointer<T>();
    }

protected:
    bool m_isActive; ///< Indica si la entidad está activa en el juego.
    int m_id; ///< Identificador único de la entidad.
    std::vector<EU::TSharedPointer<Component>> m_components; ///< Lista de componentes asociados.
};
