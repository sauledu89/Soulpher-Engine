/**
 * @file RenderScene.h
 * @brief Contenedor temporal con todos los elementos visibles del frame actual.
 *
 * @details
 * `RenderScene` es una estructura de datos efímera que se construye cada frame:
 *  1. `BaseApp` (o un SceneGraph futuro) la llena con actores y luces visibles.
 *  2. `ForwardRenderer` la consume en modo solo lectura para ejecutar los passes.
 *  3. Al finalizar el frame se descarta con `clear()`.
 *
 * Nunca posee los objetos a los que apunta; solo almacena punteros/snapshots.
 *
 * @note [GameDev] Este patrón se llama "Frame Data" o "Render Queue" en la
 * literatura de motores. Separa la recopilación de objetos visibles (CPU, lógica
 * de juego) del renderizado (GPU, API gráfica). Unreal Engine tiene un sistema
 * similar llamado "Render Thread" + "Proxy objects" que va un paso más allá
 * separando en threads distintos la simulación y el render.
 *
 * @see ForwardRenderer, RenderObject, LightData
 */

#pragma once
#include "Prerequisites.h"
#include "Rendering/RenderTypes.h"

class Skybox;

/**
 * @class RenderScene
 * @brief Snapshot del frame actual: objetos opacos, transparentes y luces.
 *
 * @details
 * Se construye al inicio de cada frame y se descarta al final.
 * No posee ninguno de los punteros que contiene.
 */
class RenderScene {
public:
    /** @brief Vacía las tres listas para reutilizar el objeto en el siguiente frame. */
    void clear();

public:
    std::vector<RenderObject> opaqueObjects;       ///< Objetos con MaterialDomain::Opaque.
    std::vector<RenderObject> transparentObjects;  ///< Objetos con MaterialDomain::Transparent, ordenados back-to-front.
    std::vector<LightData>    directionalLights;   ///< Luces direccionales activas en la escena.
    Skybox*                   skybox = nullptr;    ///< Skybox de la escena (opcional, puede ser nullptr).
};
