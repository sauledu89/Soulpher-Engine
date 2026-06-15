#pragma once
#include "Prerequisites.h"
class DeviceContext;

/**
 * @file Component.h
 * @brief Clase base abstracta para todos los componentes del motor de juego.
 *
 * @details
 * Esta clase define la **interfaz m�nima** que debe implementar cualquier componente
 * en un sistema basado en **ECS (Entity-Component System)**.
 *
 * Un `Component` representa datos o comportamientos que se pueden asociar a una entidad (`Entity`).
 * Por ejemplo:
 * - Componente de Transformaci�n (posici�n, rotaci�n, escala)
 * - Componente de Render (malla, materiales, shaders)
 * - Componente de F�sica (colisiones, fuerzas)
 *
 * @note Para estudiantes:
 * - Un `Component` **no** conoce la l�gica de otros sistemas; solo maneja su propia responsabilidad.
 * - La l�gica que coordina varios componentes vive en los **sistemas** del motor.
 * - Usar interfaces virtuales puras fuerza a que todas las clases hijas implementen los m�todos clave (`init`, `update`, `render`, `destroy`).
 *
 * @note [GameDev] En Unreal Engine, UActorComponent es el equivalente: una clase base
 * abstracta con BeginPlay / TickComponent. La diferencia es que UE usa el sistema de
 * reflection (UCLASS/UPROPERTY) para serializar componentes al editor y a disco.
 * En este motor, ComponentType enum permite hacer "type-safe downcast" sin dynamic_cast
 * cuando el tipo ya es conocido por diseño (por ejemplo, siempre hay un Transform).
 * En un ECS puro, Component no seria una clase virtual sino una struct de datos plana;
 * el polimorfismo virtual tiene un costo de vtable lookup por llamada que se evita en ECS.
 */
class Component {
public:
    /** @brief Constructor por defecto. */
    Component() = default;

    /**
     * @brief Constructor que asigna el tipo del componente.
     * @param type Enumeraci�n `ComponentType` que identifica el tipo.
     *
     * @note Esto permite saber en tiempo de ejecuci�n qu� tipo de componente es sin usar `dynamic_cast`.
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
     * @param deltaTime Tiempo (en segundos) desde la �ltima actualizaci�n.
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
     * @note Un componente de render usar� esto para dibujar; uno de f�sica podr�a dejarlo vac�o.
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
     * @return Enumeraci�n `ComponentType` que identifica el tipo.
     */
    ComponentType getType() const { return m_type; }

protected:
    ComponentType m_type; ///< Tipo del componente, usado para identificaci�n y casting seguro.
};
