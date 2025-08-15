#pragma once
#include "Prerequisites.h"
#include "MeshComponent.h"
#include "fbxsdk.h"

/**
 * @file ModelLoader.h
 * @brief Carga y procesa modelos 3D en formatos OBJ y FBX para el motor The Visionary.
 *
 * @details
 * `ModelLoader` se encarga de:
 * - Leer archivos OBJ y FBX.
 * - Extraer geometría (vértices, índices) y materiales.
 * - Cargar información de texturas.
 * - Generar `MeshComponent` listos para su uso en el renderizado.
 *
 * @note Para estudiantes:
 * - Los modelos OBJ son simples: solo contienen geometría y referencias a materiales (MTL).
 * - Los modelos FBX son más complejos: pueden incluir jerarquías de nodos, animaciones, esqueleto, materiales y múltiples UVs.
 * - Este loader se enfoca en obtener **mallas estáticas** listas para usar, pero se puede ampliar para animaciones.
 */
class ModelLoader {
public:
    /** @brief Constructor por defecto. */
    ModelLoader() = default;

    /** @brief Destructor por defecto. */
    ~ModelLoader() = default;

    /**
     * @brief Carga un modelo en formato OBJ.
     * @param filePath Ruta del archivo OBJ.
     * @return Un `MeshComponent` con la geometría cargada.
     *
     * @note
     * - No soporta animaciones ni jerarquías, solo mallas estáticas.
     * - Busca un archivo `.mtl` asociado para materiales.
     */
    MeshComponent LoadOBJModel(const std::string& filePath);

    /**
     * @brief Inicializa el administrador de FBX SDK.
     * @return `true` si la inicialización fue exitosa.
     *
     * @note El FBX SDK debe estar inicializado antes de cargar modelos FBX.
     */
    bool InitializeFBXManager();

    /**
     * @brief Carga un modelo en formato FBX.
     * @param filePath Ruta del archivo FBX.
     * @return `true` si el modelo se cargó correctamente.
     *
     * @note
     * - Puede contener múltiples nodos y mallas.
     * - Requiere haber llamado antes a `InitializeFBXManager()`.
     */
    bool LoadFBXModel(const std::string& filePath);

    /**
     * @brief Procesa recursivamente un nodo de la escena FBX.
     * @param node Puntero al nodo FBX.
     *
     * @note Recorre jerarquías para encontrar mallas y materiales.
     */
    void ProcessFBXNode(FbxNode* node);

    /**
     * @brief Procesa una malla asociada a un nodo FBX.
     * @param node Puntero al nodo que contiene la malla.
     *
     * @note Extrae vértices, normales, UVs e índices para generar un `MeshComponent`.
     */
    void ProcessFBXMesh(FbxNode* node);

    /**
     * @brief Procesa los materiales asociados a un modelo FBX.
     * @param material Puntero al material FBX.
     *
     * @note Busca texturas difusas, especulares, normales, etc., y las añade a la lista `textureFileNames`.
     */
    void ProcessFBXMaterials(FbxSurfaceMaterial* material);

    /**
     * @brief Obtiene los nombres de archivos de texturas extraídos.
     * @return Vector con rutas o nombres de archivos de textura.
     */
    std::vector<std::string> GetTextureFileNames() const { return textureFileNames; }

private:
    FbxManager* lSdkManager = nullptr; ///< Administrador principal del FBX SDK.
    FbxScene* lScene = nullptr;        ///< Escena FBX cargada en memoria.
    std::vector<std::string> textureFileNames; ///< Lista de texturas encontradas durante el procesamiento.

public:
    std::string modelName; ///< Nombre del modelo cargado.
    std::vector<MeshComponent> meshes; ///< Lista de mallas extraídas del modelo.
};
