#pragma once
#include "Prerequisites.h"
#include "EngineUtilities\Vectors\Vector3.h"
#include "Component.h"

class Transform : public Component {
public:
    /**
     * @brief Constructor que inicializa posición, rotación y escala por defecto.
     *
     * @details
     * - La posición, rotación y escala se inicializan en `(0,0,0)` y `(1,1,1)` según el valor por defecto de `EU::Vector3`.
     * - La matriz de transformación (`matrix`) comienza como identidad.
     * - El tipo de componente se establece como `ComponentType::TRANSFORM`.
     *
     * @note Para estudiantes:
     * - Este es uno de los componentes más comunes en un motor ECS.
     * - Un `Transform` define dónde y cómo está orientada una entidad en el mundo.
     */
    Transform()
        : position(), rotation(), scale(), matrix(),
        Component(ComponentType::TRANSFORM) {
    }

    /**
     * @brief Inicializa el componente de transformación.
     *
     * @note Aquí se podrían cargar datos iniciales o configurar la matriz como identidad.
     */
    void init();

    /**
     * @brief Actualiza el estado del objeto Transform.
     * @param deltaTime Tiempo transcurrido desde la última actualización (en segundos).
     *
     * @note Usualmente este método recalcula la matriz de transformación
     * combinando posición, rotación y escala.
     */
    void update(float deltaTime) override;

    /**
     * @brief Renderiza el objeto Transform.
     * @param deviceContext Contexto del dispositivo de renderizado.
     *
     * @note En este componente normalmente no se dibuja nada, pero se deja
     * implementado para cumplir con la interfaz de `Component`.
     */
    void render(DeviceContext& deviceContext) override {}

    /**
     * @brief Destruye el objeto Transform y libera recursos.
     *
     * @note Generalmente no es necesario liberar memoria aquí, ya que es un componente de datos.
     */
    void destroy() {}

    // ==== Métodos de acceso y modificación ====

    /** @brief Obtiene la posición actual. */
    const EU::Vector3& getPosition() const { return position; }

    /** @brief Establece una nueva posición. */
    void setPosition(const EU::Vector3& newPos) { position = newPos; }

    /** @brief Obtiene la rotación actual. */
    const EU::Vector3& getRotation() const { return rotation; }

    /** @brief Establece una nueva rotación. */
    void setRotation(const EU::Vector3& newRot) { rotation = newRot; }

    /** @brief Obtiene la escala actual. */
    const EU::Vector3& getScale() const { return scale; }

    /** @brief Establece una nueva escala. */
    void setScale(const EU::Vector3& newScale) { scale = newScale; }

    /**
     * @brief Establece posición, rotación y escala en una sola llamada.
     * @param newPos Nueva posición.
     * @param newRot Nueva rotación.
     * @param newSca Nueva escala.
     *
     * @note Útil para inicializar transformaciones rápidamente.
     */
    void setTransform(const EU::Vector3& newPos,
        const EU::Vector3& newRot,
        const EU::Vector3& newSca);

    /**
     * @brief Traslada la posición del objeto.
     * @param translation Vector que representa el desplazamiento en cada eje.
     *
     * @note Similar a `setPosition(getPosition() + translation)`.
     */
    void translate(const EU::Vector3& translation);

private:
    EU::Vector3 position; ///< Posición del objeto en coordenadas del mundo.
    EU::Vector3 rotation; ///< Rotación en ejes X, Y, Z (en grados o radianes según convención).
    EU::Vector3 scale;    ///< Escala relativa en X, Y, Z.

public:
    XMMATRIX matrix; ///< Matriz de transformación combinada (posición, rotación, escala).
};
