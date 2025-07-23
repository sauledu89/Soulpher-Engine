#pragma once 
#include "OBJ_Loader.h"
#include "Prerequisites.h"

class MeshComponent;

class ModelLoader {
public:
    ModelLoader() = default;
    ~ModelLoader() = default;

    void
        init();

    void
        update();

    void
        render();

    void
        destroy();

    /**
     * @brief Carga un modelo desde un archivo .obj y devuelve sus datos.
     * @param filename La ruta completa al archivo del modelo .obj.
     * @return Un objeto MeshComponent que contiene la geometría del modelo.
     */
    MeshComponent
        Load(const std::string& filename);
};