#pragma once
#include "Prerequisites.h"
#include "Component.h"

class DeviceContext;

/**
 * @file Entity.h
 * @brief Clase base para todas las entidades del motor de juego.
 *
 * @details
 * Una `Entity` es el n�cleo del patr�n **ECS (Entity-Component System)**:
 * - Contiene una colecci�n de `Component` que definen sus datos y comportamientos.
 * - No implementa l�gica directamente, sino que delega en sus componentes.
 * - Funciona como "contenedor" para combinar m�ltiples aspectos (render, f�sica, audio, IA).
 *
 * @note Para estudiantes:
 * - Las entidades por s� mismas son �vac�as�; la funcionalidad se logra a trav�s de componentes.
 * - Esto permite reutilizar c�digo y crear variaciones de entidades simplemente cambiando sus componentes.
 * - Ejemplo: un "enemigo" y un "jugador" pueden compartir el mismo `PhysicsComponent` pero tener distinto `AIComponent`.
 *
 * @note [GameDev] En Unreal Engine, el equivalente es UObject/AActor: todo objeto del juego
 * hereda de UObject (identity, reflection, garbage collection) y AActor anade spawn/destroy,
 * componentes y ticks. Unity usa GameObject + Component, igual que este ECS.
 * La diferencia clave con un ECS puro (como DOTS de Unity o Flecs) es que aqui la entidad
 * tiene logica virtual (update/render virtuales). En ECS puro las entidades son solo IDs
 * numericos sin datos ni metodos, y los sistemas procesan arrays de componentes en batch
 * para maximizar cache coherency (Structure of Arrays vs Array of Structures).
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
     * @note Llamar antes de `update()` o `render()` para garantizar que los componentes est�n listos.
     */
    virtual void init() = 0;

    /**
     * @brief Actualiza la l�gica de la entidad.
     * @param deltaTime Tiempo en segundos desde la �ltima actualizaci�n.
     * @param deviceContext Contexto de dispositivo (render u operaciones gr�ficas).
     *
     * @note Ideal para actualizar todos los componentes asociados (f�sica, animaciones, IA).
     */
    virtual void update(float deltaTime, DeviceContext& deviceContext) = 0;

    /**
     * @brief Renderiza la entidad.
     * @param deviceContext Contexto del dispositivo para operaciones gr�ficas.
     *
     * @note Normalmente delega a los componentes gr�ficos de la entidad.
     */
    virtual void render(DeviceContext& deviceContext) = 0;

    /**
     * @brief Destruye la entidad y libera sus recursos.
     *
     * @note Asegura la liberaci�n de memoria GPU/CPU asociada a sus componentes.
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
    bool m_isActive; ///< Indica si la entidad est� activa en el juego.
    int m_id; ///< Identificador �nico de la entidad.
    std::vector<EU::TSharedPointer<Component>> m_components; ///< Lista de componentes asociados.
};
