#pragma once
#include "Prerequisites.h"
#include "MeshComponent.h"
#include "fbxsdk.h"

/**
 * @file ModelLoader.h
 * @brief Carga y procesa modelos 3D en formatos OBJ y FBX — pipeline de assets de Soulpher-Engine.
 *
 * @details
 * `ModelLoader` se encarga de:
 * - Leer archivos OBJ y FBX.
 * - Extraer geometr�a (v�rtices, �ndices) y materiales.
 * - Cargar informaci�n de texturas.
 * - Generar `MeshComponent` listos para su uso en el renderizado.
 *
 * @note Para estudiantes:
 * - Los modelos OBJ son simples: solo contienen geometr�a y referencias a materiales (MTL).
 * - Los modelos FBX son m�s complejos: pueden incluir jerarqu�as de nodos, animaciones, esqueleto, materiales y m�ltiples UVs.
 * - Este loader se enfoca en obtener **mallas est�ticas** listas para usar, pero se puede ampliar para animaciones.
 *
 * @note [GameDev] El FBX SDK de Autodesk (2020.3.7) es el estandar de facto para
 * intercambio de assets 3D entre DCC tools (Maya, Blender, 3ds Max) y motores.
 * ProcessFBXMesh hace triangulacion automatica, correccion UV (flip V) y ensambla la
 * transformacion global del nodo para importar modelos con transformaciones embebidas.
 * La TBN (Tangent-Bitangent-Normal) se genera con Gram-Schmidt para ortogonalizar la
 * base tangente, necesaria para los normal maps en el pixel shader.
 * En Unreal Engine el pipeline equivalente es el FBX Import con el Interchange plugin
 * (UE5+). Los motores de produccion cachean los assets en un formato binario propio
 * (como el .uasset de UE) para evitar el costo de procesar el FBX en cada startup.
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
     * @return Un `MeshComponent` con la geometr�a cargada.
     *
     * @note
     * - No soporta animaciones ni jerarqu�as, solo mallas est�ticas.
     * - Busca un archivo `.mtl` asociado para materiales.
     */
    MeshComponent LoadOBJModel(const std::string& filePath);

    /**
     * @brief Inicializa el administrador de FBX SDK.
     * @return `true` si la inicializaci�n fue exitosa.
     *
     * @note El FBX SDK debe estar inicializado antes de cargar modelos FBX.
     */
    bool InitializeFBXManager();

    /**
     * @brief Carga un modelo en formato FBX.
     * @param filePath Ruta del archivo FBX.
     * @return `true` si el modelo se carg� correctamente.
     *
     * @note
     * - Puede contener m�ltiples nodos y mallas.
     * - Requiere haber llamado antes a `InitializeFBXManager()`.
     */
    bool LoadFBXModel(const std::string& filePath);

    /**
     * @brief Procesa recursivamente un nodo de la escena FBX.
     * @param node Puntero al nodo FBX.
     *
     * @note Recorre jerarqu�as para encontrar mallas y materiales.
     */
    void ProcessFBXNode(FbxNode* node);

    /**
     * @brief Procesa una malla asociada a un nodo FBX.
     * @param node Puntero al nodo que contiene la malla.
     *
     * @note Extrae v�rtices, normales, UVs e �ndices para generar un `MeshComponent`.
     */
    void ProcessFBXMesh(FbxNode* node);

    /**
     * @brief Procesa los materiales asociados a un modelo FBX.
     * @param material Puntero al material FBX.
     *
     * @note Busca texturas difusas, especulares, normales, etc., y las a�ade a la lista `textureFileNames`.
     */
    void ProcessFBXMaterials(FbxSurfaceMaterial* material);

    /**
     * @brief Obtiene los nombres de archivos de texturas extra�dos.
     * @return Vector con rutas o nombres de archivos de textura.
     */
    std::vector<std::string> GetTextureFileNames() const { return textureFileNames; }

private:
    FbxManager* lSdkManager = nullptr; ///< Administrador principal del FBX SDK.
    FbxScene* lScene = nullptr;        ///< Escena FBX cargada en memoria.
    std::vector<std::string> textureFileNames; ///< Lista de texturas encontradas durante el procesamiento.

public:
    std::string modelName; ///< Nombre del modelo cargado.
    std::vector<MeshComponent> meshes; ///< Lista de mallas extra�das del modelo.
};
