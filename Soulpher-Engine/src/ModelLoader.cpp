/**
 * @file ModelLoader.cpp
 * @brief Carga y procesamiento de modelos 3D (OBJ/FBX), junto con materiales y texturas.
 *
 * @details
 * Este módulo centraliza toda la lógica necesaria para importar modelos 3D
 * desde formatos comunes en videojuegos como:
 *  - **OBJ** (Wavefront): Ligero, ampliamente usado, ideal para geometría estática.
 *  - **FBX** (Autodesk): Complejo, soporta jerarquías, animaciones y materiales avanzados.
 *
 * Las funciones se encargan de:
 *  1. Leer el archivo desde disco.
 *  2. Procesar vértices, índices y coordenadas UV.
 *  3. Extraer información de materiales y texturas.
 *  4. Guardar el resultado en `MeshComponent` para que el motor lo renderice.
 *
 * @note
 * - Para OBJ se utiliza la librería externa `OBJ_Loader`.
 * - Para FBX se emplea el **Autodesk FBX SDK**.
 */

#include "ModelLoader.h"
#include "OBJ_Loader.h"

 // ============================================================================
 //  Carga de modelos OBJ
 // ============================================================================
 /**
  * @brief Carga un modelo en formato OBJ.
  *
  * @param filePath Ruta del archivo OBJ a cargar.
  * @return MeshComponent Malla con vértices, índices y coordenadas UV cargadas.
  *
  * @details
  * - Usa `objl::Loader` para interpretar el formato OBJ.
  * - Convierte los datos a `SimpleVertex` para que el motor pueda procesarlos.
  * - Invierte el eje Y de las coordenadas UV (1 - Y) para ajustarse al sistema DirectX.
  */
MeshComponent ModelLoader::LoadOBJModel(const std::string& filePath) {
    MeshComponent mesh;
    objl::Loader loader;

    if (!loader.LoadFile(filePath)) {
        return mesh; // Devuelve vacío si falla
    }

    mesh.m_name = filePath;

    const unsigned int numVertices = loader.LoadedVertices.size();
    const unsigned int numIndices = loader.LoadedIndices.size();

    // Reservar memoria exacta para evitar reallocaciones
    mesh.m_vertex.resize(numVertices);
    mesh.m_index = std::move(loader.LoadedIndices); // Mover directamente (optimización)

    for (unsigned int i = 0; i < numVertices; ++i) {
        const auto& v = loader.LoadedVertices[i];
        mesh.m_vertex[i] = SimpleVertex{
            { v.Position.X, v.Position.Y, v.Position.Z },
            { v.TextureCoordinate.X, 1.0f - v.TextureCoordinate.Y }
        };
    }

    mesh.m_numVertex = numVertices;
    mesh.m_numIndex = numIndices;

    return mesh;
}

// ============================================================================
//  Inicialización del FBX Manager
// ============================================================================
/**
 * @brief Inicializa el administrador principal del Autodesk FBX SDK.
 * @return true si la inicialización fue exitosa, false en caso contrario.
 *
 * @details
 * - Crea el `FbxManager` que controla toda la interacción con FBX.
 * - Configura `FbxIOSettings` para definir opciones de importación/exportación.
 * - Crea una escena vacía (`FbxScene`) donde se almacenará el modelo importado.
 */
bool ModelLoader::InitializeFBXManager() {
    lSdkManager = FbxManager::Create();
    if (!lSdkManager) {
        ERROR("ModelLoader", "FbxManager::Create()", "Unable to create FBX Manager!");
        return false;
    }
    else {
        MESSAGE("ModelLoader", "ModelLoader", "Autodesk FBX SDK version " << lSdkManager->GetVersion());
    }

    FbxIOSettings* ios = FbxIOSettings::Create(lSdkManager, IOSROOT);
    lSdkManager->SetIOSettings(ios);

    lScene = FbxScene::Create(lSdkManager, "MyScene");
    if (!lScene) {
        ERROR("ModelLoader", "FbxScene::Create()", "Unable to create FBX Scene!");
        return false;
    }
    else {
        MESSAGE("ModelLoader", "ModelLoader", "FBX Scene created successfully.");
    }
    return true;
}

// ============================================================================
//  Carga de modelos FBX
// ============================================================================
/**
 * @brief Carga un modelo en formato FBX.
 *
 * @param filePath Ruta del archivo FBX a cargar.
 * @return true si se cargó correctamente, false si falló.
 *
 * @details
 * 1. Inicializa el SDK de FBX.
 * 2. Crea un `FbxImporter` para leer el archivo.
 * 3. Importa la escena completa a `lScene`.
 * 4. Procesa recursivamente cada nodo (`FbxNode`) para extraer mallas y materiales.
 */
bool ModelLoader::LoadFBXModel(const std::string& filePath) {
    if (InitializeFBXManager()) {
        FbxImporter* lImporter = FbxImporter::Create(lSdkManager, "");
        if (!lImporter) {
            ERROR("ModelLoader", "FbxImporter::Create()", "Unable to create FBX Importer!");
            return false;
        }
        else {
            MESSAGE("ModelLoader", "ModelLoader", "FBX Importer created successfully.");
        }

        if (!lImporter->Initialize(filePath.c_str(), -1, lSdkManager->GetIOSettings())) {
            ERROR("ModelLoader", "FbxImporter::Initialize()", "Unable to initialize FBX Importer! Error: " << lImporter->GetStatus().GetErrorString());
            lImporter->Destroy();
            return false;
        }

        if (!lImporter->Import(lScene)) {
            ERROR("ModelLoader", "FbxImporter::Import()", "Unable to import FBX Scene! Error: " << lImporter->GetStatus().GetErrorString());
            lImporter->Destroy();
            return false;
        }
        else {
            MESSAGE("ModelLoader", "ModelLoader", "FBX Scene imported successfully.");
            modelName = lImporter->GetFileName();
        }

        lImporter->Destroy();

        FbxNode* lRootNode = lScene->GetRootNode();
        if (lRootNode) {
            for (int i = 0; i < lRootNode->GetChildCount(); i++) {
                ProcessFBXNode(lRootNode->GetChild(i));
            }
            return true;
        }
        else {
            ERROR("ModelLoader", "FbxScene::GetRootNode()", "Unable to get root node from FBX Scene!");
            return false;
        }
    }
    return false;
}

// ============================================================================
//  Procesamiento recursivo de nodos FBX
// ============================================================================
/**
 * @brief Procesa un nodo FBX y sus hijos recursivamente.
 *
 * @param node Nodo actual de la escena.
 *
 * @details
 * - Si el nodo contiene una malla, se llama a `ProcessFBXMesh`.
 * - Luego se procesan todos sus hijos de forma recursiva.
 */
void ModelLoader::ProcessFBXNode(FbxNode* node) {
    if (node->GetNodeAttribute()) {
        if (node->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eMesh) {
            ProcessFBXMesh(node);
        }
    }

    for (int i = 0; i < node->GetChildCount(); i++) {
        ProcessFBXNode(node->GetChild(i));
    }
}

// ============================================================================
//  Procesamiento de mallas FBX
// ============================================================================
/**
 * @brief Convierte una malla FBX (`FbxMesh`) a un `MeshComponent`.
 *
 * @param node Nodo que contiene la malla.
 *
 * @details
 * 1. Extrae posiciones desde los control points.
 * 2. Extrae coordenadas UV dependiendo del modo de mapeo.
 * 3. Extrae índices de los polígonos.
 * 4. Crea un `MeshComponent` y lo almacena en `meshes`.
 */
void ModelLoader::ProcessFBXMesh(FbxNode* node) {
    FbxMesh* mesh = node->GetMesh();
    if (!mesh) return;

    std::vector<SimpleVertex> vertices;
    std::vector<unsigned int> indices;

    // Posiciones
    for (int i = 0; i < mesh->GetControlPointsCount(); i++) {
        SimpleVertex vertex;
        FbxVector4* controlPoint = mesh->GetControlPoints();
        vertex.Pos = XMFLOAT3((float)controlPoint[i][0], (float)controlPoint[i][1], (float)controlPoint[i][2]);
        vertices.push_back(vertex);
    }

    // Coordenadas UV
    if (mesh->GetElementUVCount() > 0) {
        FbxGeometryElementUV* uvElement = mesh->GetElementUV(0);
        FbxGeometryElement::EMappingMode mappingMode = uvElement->GetMappingMode();
        FbxGeometryElement::EReferenceMode referenceMode = uvElement->GetReferenceMode();
        int polyIndexCounter = 0;

        for (int polyIndex = 0; polyIndex < mesh->GetPolygonCount(); polyIndex++) {
            int polySize = mesh->GetPolygonSize(polyIndex);
            for (int vertIndex = 0; vertIndex < polySize; vertIndex++) {
                int controlPointIndex = mesh->GetPolygonVertex(polyIndex, vertIndex);
                int uvIndex = -1;

                if (mappingMode == FbxGeometryElement::eByControlPoint) {
                    uvIndex = (referenceMode == FbxGeometryElement::eDirect) ?
                        controlPointIndex :
                        uvElement->GetIndexArray().GetAt(controlPointIndex);
                }
                else if (mappingMode == FbxGeometryElement::eByPolygonVertex) {
                    uvIndex = uvElement->GetIndexArray().GetAt(polyIndexCounter++);
                }

                if (uvIndex != -1) {
                    FbxVector2 uv = uvElement->GetDirectArray().GetAt(uvIndex);
                    vertices[controlPointIndex].Tex = XMFLOAT2((float)uv[0], -(float)uv[1]);
                }
            }
        }
    }

    // Índices
    for (int i = 0; i < mesh->GetPolygonCount(); i++) {
        for (int j = 0; j < mesh->GetPolygonSize(i); j++) {
            indices.push_back(mesh->GetPolygonVertex(i, j));
        }
    }

    MeshComponent meshData;
    meshData.m_name = node->GetName();
    meshData.m_vertex = vertices;
    meshData.m_index = indices;
    meshData.m_numVertex = vertices.size();
    meshData.m_numIndex = indices.size();

    meshes.push_back(meshData);
}

// ============================================================================
//  Procesamiento de materiales FBX
// ============================================================================
/**
 * @brief Procesa un material FBX y extrae nombres de texturas difusas.
 *
 * @param material Puntero al material FBX.
 *
 * @details
 * Busca la propiedad `sDiffuse` y agrega los nombres de las texturas
 * encontradas al vector `textureFileNames`.
 */
void ModelLoader::ProcessFBXMaterials(FbxSurfaceMaterial* material) {
    if (material) {
        FbxProperty prop = material->FindProperty(FbxSurfaceMaterial::sDiffuse);
        if (prop.IsValid()) {
            int textureCount = prop.GetSrcObjectCount<FbxTexture>();
            for (int i = 0; i < textureCount; ++i) {
                FbxTexture* texture = FbxCast<FbxTexture>(prop.GetSrcObject<FbxTexture>(i));
                if (texture) {
                    textureFileNames.push_back(texture->GetName());
                }
            }
        }
    }
}
