/**
 * @file RenderScene.cpp
 * @brief Implementación de RenderScene — limpieza del snapshot de frame.
 *
 * @details
 * El único método de este archivo es `clear()`, que vacía las tres listas de
 * la escena para reutilizar el objeto en el siguiente frame sin hacer
 * reservas de memoria adicionales.
 *
 * @note [GameDev] Llamar a `clear()` en lugar de destruir y recrear el objeto
 * cada frame aprovecha la memoria ya reservada por los `std::vector` internos
 * (capacidad != tamaño), evitando reallocaciones frecuentes que fragmentan el heap.
 *
 * @see RenderScene, ForwardRenderer
 */

#include "Rendering/RenderScene.h"

/**
 * @brief Vacía las listas de objetos y luces para preparar el siguiente frame.
 *
 * @note No libera la memoria reservada; solo pone size a 0.
 * La capacidad del vector se conserva para reutilizarla en el próximo frame.
 */
void
RenderScene::clear() {
    opaqueObjects.clear();
    transparentObjects.clear();
    lights.clear();
}
