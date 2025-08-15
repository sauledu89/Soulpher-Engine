/**
 * @file ModelLoader.cpp
 * @brief Carga y procesamiento de modelos 3D (formatos OBJ y FBX), materiales y texturas.
 *
 * @details
 * Este módulo permite cargar modelos desde archivos OBJ y FBX, procesando su geometría
 * y materiales para que puedan ser renderizados en DirectX 11.
 *
 * **Características clave:**
 * - Conversión de datos de modelos a estructuras optimizadas para GPU.
 * - Procesamiento de coordenadas UV, índices y posiciones de vértices.
 * - Soporte para múltiples mallas dentro de un mismo archivo FBX.
 *
 * @note En motores de videojuegos, el cargador de modelos es fundamental para importar
 *       assets desde herramientas como Blender, Maya o 3ds Max, manteniendo la
 *       compatibilidad con la etapa de renderizado.
 */

#include "ModelLoader.h"
#include "OBJ_Loader.h"

 /**
  * @brief Carga un modelo en formato OBJ y lo convierte a un MeshComponent.
  *
  * @param filePath Ruta al archivo OBJ.
  * @return MeshComponent con los datos de la malla cargada.
  *
  * @note
  * - Solo procesa posición y coordenadas de textura.
  * - Invierte el eje Y de las UV para corregir diferencias entre motores.
  * - Los modelos OBJ no almacenan animaciones, solo mallas estáticas.
  */
MeshComponent ModelLoader::LoadOBJModel(const std::string& filePath) {
    MeshComponent mesh;
    objl::Loader loader;

    if (!loader.LoadFile(filePath)) {
        return mesh;
    }

    mesh.m_name = filePath;

    const unsigned int numVertices = loader.LoadedVertices.size();
    const unsigned int numIndices = loader.LoadedIndices.size();

    mesh.m_vertex.resize(numVertices);
    mesh.m_index = std::move(loader.LoadedIndices);

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

/**
 * @brief Inicializa el administrador de la SDK de FBX.
 *
 * @return true si la inicialización fue exitosa, false si hubo errores.
 *
 * @note
 * - Este paso es obligatorio antes de cargar cualquier modelo FBX.
 * - Crea la escena y configura opciones de entrada/salida.
 */
bool ModelLoader::InitializeFBXManager() {
    lSdkManager = FbxManager::Create();
    if (!lSdkManager) {
        ERROR("ModelLoader", "FbxManager::Create()", "Unable to create FBX Manager!");
        return false;
    }
    else {
        MESSAGE("ModelLoader", "ModelLoader", "Autodesk FBX SDK version " << lSdkManager->GetVersion())
    }

    FbxIOSettings* ios = FbxIOSettings::Create(lSdkManager, IOSROOT);
    lSdkManager->SetIOSettings(ios);

    lScene = FbxScene::Create(lSdkManager, "MyScene");
    if (!lScene) {
        ERROR("ModelLoader", "FbxScene::Create()", "Unable to create FBX Scene!");
        return false;
    }
    else {
        MESSAGE("ModelLoader", "ModelLoader", "FBX Scene created successfully.")
    }
    return true;
}

/**
 * @brief Carga un modelo en formato FBX y procesa su escena.
 *
 * @param filePath Ruta al archivo FBX.
 * @return true si se cargó correctamente, false si hubo fallos.
 *
 * @note
 * - Soporta múltiples mallas y jerarquía de nodos.
 * - Guarda el nombre del modelo cargado.
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
            ERROR("ModelLoader", "FbxImporter::Initialize()",
                "Unable to initialize FBX Importer! Error: " << lImporter->GetStatus().GetErrorString());
            lImporter->Destroy();
            return false;
        }
        else {
            MESSAGE("ModelLoader", "ModelLoader", "FBX Importer initialized successfully.");
        }

        if (!lImporter->Import(lScene)) {
            ERROR("ModelLoader", "FbxImporter::Import()",
                "Unable to import FBX Scene! Error: " << lImporter->GetStatus().GetErrorString());
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

/**
 * @brief Procesa recursivamente un nodo FBX y sus hijos.
 *
 * @param node Nodo actual a procesar.
 *
 * @note
 * - Si el nodo contiene una malla, se procesa y almacena.
 * - Llama a sí mismo para recorrer toda la jerarquía de la escena.
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

/**
 * @brief Procesa una malla FBX, extrayendo posiciones, UVs e índices.
 *
 * @param node Nodo FBX que contiene la malla.
 *
 * @note
 * - Extrae vértices y coordenadas UV (si existen).
 * - Convierte los datos a un MeshComponent.
 */
void ModelLoader::ProcessFBXMesh(FbxNode* node) {
    FbxMesh* mesh = node->GetMesh();
    if (!mesh) return;

    std::vector<SimpleVertex> vertices;
    std::vector<unsigned int> indices;

    for (int i = 0; i < mesh->GetControlPointsCount(); i++) {
        SimpleVertex vertex;
        FbxVector4* controlPoint = mesh->GetControlPoints();
        vertex.Pos = XMFLOAT3((float)controlPoint[i][0],
            (float)controlPoint[i][1],
            (float)controlPoint[i][2]);
        vertices.push_back(vertex);
    }

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
                    if (referenceMode == FbxGeometryElement::eDirect) {
                        uvIndex = controlPointIndex;
                    }
                    else if (referenceMode == FbxGeometryElement::eIndexToDirect) {
                        uvIndex = uvElement->GetIndexArray().GetAt(controlPointIndex);
                    }
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

/**
 * @brief Procesa materiales FBX para extraer texturas difusas.
 *
 * @param material Material FBX a procesar.
 *
 * @note Guarda los nombres de los archivos de textura encontrados.
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
