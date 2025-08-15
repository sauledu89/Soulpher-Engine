/**
 * @file Transform.cpp
 * @brief Implementación del componente Transform en el sistema ECS.
 *
 * @details
 * El componente `Transform` almacena y gestiona la **posición**, **rotación** y **escala**
 * de una entidad en el mundo 3D. También calcula la **matriz de transformación** que
 * combina estos tres elementos en el orden adecuado para el renderizado.
 *
 * ---
 * 💡 **Notas para estudiantes**:
 * - La matriz de transformación (`matrix`) se utiliza para pasar la posición,
 *   orientación y tamaño del objeto a la GPU.
 * - El orden de multiplicación es importante: **escala → rotación → traslación**.
 * - Este patrón es común en motores de videojuegos y se basa en la matemática de
 *   transformaciones en gráficos 3D.
 */

#include "ECS/Transform.h"
#include "DeviceContext.h"

 /**
  * @brief Inicializa el transform con valores por defecto.
  *
  * @details
  * - Escala = (1,1,1).
  * - Matriz de transformación = identidad (sin rotación, traslación o escala).
  */
void Transform::init() {
    scale.one();                ///< Escala unitaria.
    matrix = XMMatrixIdentity();///< Matriz inicial (sin transformación).
}

/**
 * @brief Actualiza la matriz de transformación según posición, rotación y escala.
 * @param deltaTime Tiempo transcurrido desde el último frame (no usado aquí, pero útil para animaciones futuras).
 *
 * @details
 * - Calcula las matrices de escala, rotación y traslación.
 * - Las multiplica en el orden: `escala * rotación * traslación`.
 * - Este orden asegura que la rotación y la escala se apliquen respecto al sistema local del objeto.
 */
void Transform::update(float deltaTime) {
    // Matriz de escala
    XMMATRIX scaleMatrix = XMMatrixScaling(scale.x, scale.y, scale.z);

    // Matriz de rotación (en radianes: pitch=X, yaw=Y, roll=Z)
    XMMATRIX rotationMatrix = XMMatrixRotationRollPitchYaw(rotation.x, rotation.y, rotation.z);

    // Matriz de traslación
    XMMATRIX translationMatrix = XMMatrixTranslation(position.x, position.y, position.z);

    // Composición final
    matrix = scaleMatrix * rotationMatrix * translationMatrix;
}

/**
 * @brief Configura la posición, rotación y escala del transform.
 * @param newPos Nueva posición.
 * @param newRot Nueva rotación (en radianes).
 * @param newSca Nueva escala.
 */
void Transform::setTransform(const EU::Vector3& newPos,
    const EU::Vector3& newRot,
    const EU::Vector3& newSca) {
    position = newPos;
    rotation = newRot;
    scale = newSca;
}
