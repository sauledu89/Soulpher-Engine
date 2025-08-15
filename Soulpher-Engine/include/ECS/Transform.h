#pragma once
#include "Prerequisites.h"
#include "EngineUtilities\Vectors\Vector3.h"
#include "Component.h"

class Transform : public Component {
public:
    /**
     * @brief Constructor que inicializa posici�n, rotaci�n y escala por defecto.
     *
     * @details
     * - La posici�n, rotaci�n y escala se inicializan en `(0,0,0)` y `(1,1,1)` seg�n el valor por defecto de `EU::Vector3`.
     * - La matriz de transformaci�n (`matrix`) comienza como identidad.
     * - El tipo de componente se establece como `ComponentType::TRANSFORM`.
     *
     * @note Para estudiantes:
     * - Este es uno de los componentes m�s comunes en un motor ECS.
     * - Un `Transform` define d�nde y c�mo est� orientada una entidad en el mundo.
     */
    Transform()
        : position(), rotation(), scale(), matrix(),
        Component(ComponentType::TRANSFORM) {
    }

    /**
     * @brief Inicializa el componente de transformaci�n.
     *
     * @note Aqu� se podr�an cargar datos iniciales o configurar la matriz como identidad.
     */
    void init();

    /**
     * @brief Actualiza el estado del objeto Transform.
     * @param deltaTime Tiempo transcurrido desde la �ltima actualizaci�n (en segundos).
     *
     * @note Usualmente este m�todo recalcula la matriz de transformaci�n
     * combinando posici�n, rotaci�n y escala.
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
     * @note Generalmente no es necesario liberar memoria aqu�, ya que es un componente de datos.
     */
    void destroy() {}

    // ==== M�todos de acceso y modificaci�n ====

    /** @brief Obtiene la posici�n actual. */
    const EU::Vector3& getPosition() const { return position; }

    /** @brief Establece una nueva posici�n. */
    void setPosition(const EU::Vector3& newPos) { position = newPos; }

    /** @brief Obtiene la rotaci�n actual. */
    const EU::Vector3& getRotation() const { return rotation; }

    /** @brief Establece una nueva rotaci�n. */
    void setRotation(const EU::Vector3& newRot) { rotation = newRot; }

    /** @brief Obtiene la escala actual. */
    const EU::Vector3& getScale() const { return scale; }

    /** @brief Establece una nueva escala. */
    void setScale(const EU::Vector3& newScale) { scale = newScale; }

    /**
     * @brief Establece posici�n, rotaci�n y escala en una sola llamada.
     * @param newPos Nueva posici�n.
     * @param newRot Nueva rotaci�n.
     * @param newSca Nueva escala.
     *
     * @note �til para inicializar transformaciones r�pidamente.
     */
    void setTransform(const EU::Vector3& newPos,
        const EU::Vector3& newRot,
        const EU::Vector3& newSca);

    /**
     * @brief Traslada la posici�n del objeto.
     * @param translation Vector que representa el desplazamiento en cada eje.
     *
     * @note Similar a `setPosition(getPosition() + translation)`.
     */
    void translate(const EU::Vector3& translation);

private:
    EU::Vector3 position; ///< Posici�n del objeto en coordenadas del mundo.
    EU::Vector3 rotation; ///< Rotaci�n en ejes X, Y, Z (en grados o radianes seg�n convenci�n).
    EU::Vector3 scale;    ///< Escala relativa en X, Y, Z.

public:
    XMMATRIX matrix; ///< Matriz de transformaci�n combinada (posici�n, rotaci�n, escala).
};
