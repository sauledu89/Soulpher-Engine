#pragma once
#include "Prerequisites.h"
#include "Rendering/RenderTypes.h"

/**
 * @class RenderScene
 * @brief Contenedor temporal con los elementos visibles para el frame actual.
 *
 * BaseApp (o el SceneGraph) llena esta estructura cada frame antes de llamar
 * a ForwardRenderer::render(). El renderer la consume de forma de solo lectura
 * y la descarta al final del frame. Nunca posee los objetos apuntados.
 */
class RenderScene {
public:
    void clear();

public:
    std::vector<RenderObject> opaqueObjects;
    std::vector<RenderObject> transparentObjects;
    std::vector<LightData>    directionalLights;
};
