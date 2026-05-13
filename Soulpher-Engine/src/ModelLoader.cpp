/**
 * @file ModelLoader.cpp
 * @brief Carga y procesamiento de modelos 3D (OBJ/FBX), junto con materiales y texturas.
 *
 * @details
 * Este m�dulo centraliza toda la l�gica necesaria para importar modelos 3D
 * desde formatos comunes en videojuegos como:
 *  - **OBJ** (Wavefront): Ligero, ampliamente usado, ideal para geometr�a est�tica.
 *  - **FBX** (Autodesk): Complejo, soporta jerarqu�as, animaciones y materiales avanzados.
 *
 * Las funciones se encargan de:
 *  1. Leer el archivo desde disco.
 *  2. Procesar v�rtices, �ndices y coordenadas UV.
 *  3. Extraer informaci�n de materiales y texturas.
 *  4. Guardar el resultado en `MeshComponent` para que el motor lo renderice.
 *
 * @note
 * - Para OBJ se utiliza la librer�a externa `OBJ_Loader`.
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
  * @return MeshComponent Malla con v�rtices, �ndices y coordenadas UV cargadas.
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
        return mesh; // Devuelve vac�o si falla
    }

    mesh.m_name = filePath;

    const unsigned int numVertices = loader.LoadedVertices.size();
    const unsigned int numIndices = loader.LoadedIndices.size();

    // Reservar memoria exacta para evitar reallocaciones
    mesh.m_vertex.resize(numVertices);
    mesh.m_index = std::move(loader.LoadedIndices); // Mover directamente (optimizaci�n)

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
//  Inicializaci�n del FBX Manager
// ============================================================================
/**
 * @brief Inicializa el administrador principal del Autodesk FBX SDK.
 * @return true si la inicializaci�n fue exitosa, false en caso contrario.
 *
 * @details
 * - Crea el `FbxManager` que controla toda la interacci�n con FBX.
 * - Configura `FbxIOSettings` para definir opciones de importaci�n/exportaci�n.
 * - Crea una escena vac�a (`FbxScene`) donde se almacenar� el modelo importado.
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
 * @return true si se carg� correctamente, false si fall�.
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

        // Convertir unidades de la escena a metros (el engine trabaja en metros).
        // Cubre casos donde el FBX fue exportado en cm, mm u otras unidades.
        FbxSystemUnit sceneUnit = lScene->GetGlobalSettings().GetSystemUnit();
        if (sceneUnit != FbxSystemUnit::m) {
            FbxSystemUnit::m.ConvertScene(lScene);
        }

        // Triangular toda la escena antes de procesar (quads y n-gons a triangulos)
        FbxGeometryConverter converter(lSdkManager);
        converter.Triangulate(lScene, true);

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
 * 3. Extrae �ndices de los pol�gonos.
 * 4. Crea un `MeshComponent` y lo almacena en `meshes`.
 */
void ModelLoader::ProcessFBXMesh(FbxNode* node) {
    FbxMesh* mesh = node->GetMesh();
    if (!mesh) return;

    // Transform global del nodo: coloca cada pieza en su posicion/orientacion correcta en el mundo
    FbxAMatrix globalTransform = node->EvaluateGlobalTransform();
    FbxVector4* controlPoints  = mesh->GetControlPoints();

    FbxGeometryElementUV* uvElement = (mesh->GetElementUVCount() > 0)
        ? mesh->GetElementUV(0) : nullptr;

    FbxGeometryElement::EMappingMode   uvMapping   = FbxGeometryElement::eNone;
    FbxGeometryElement::EReferenceMode uvReference = FbxGeometryElement::eDirect;
    if (uvElement) {
        uvMapping   = uvElement->GetMappingMode();
        uvReference = uvElement->GetReferenceMode();
    }

    std::vector<SimpleVertex> vertices;
    std::vector<unsigned int> indices;

    // Un vertice por polygon-vertex para soportar UVs discontinuas entre poligonos.
    // La triangulacion previa garantiza polySize == 3 siempre.
    int polyVertCounter = 0;
    for (int polyIndex = 0; polyIndex < mesh->GetPolygonCount(); polyIndex++) {
        int polySize = mesh->GetPolygonSize(polyIndex);
        for (int vertIndex = 0; vertIndex < polySize; vertIndex++) {
            int cpIndex = mesh->GetPolygonVertex(polyIndex, vertIndex);

            // Posicion en espacio mundo con transform global aplicado
            FbxVector4 worldPos = globalTransform.MultT(controlPoints[cpIndex]);

            SimpleVertex vertex;
            vertex.Pos = XMFLOAT3((float)worldPos[0], (float)worldPos[1], (float)worldPos[2]);

            // UV: maneja eByControlPoint y eByPolygonVertex x eDirect / eIndexToDirect
            if (uvElement) {
                int uvIndex = -1;

                if (uvMapping == FbxGeometryElement::eByControlPoint) {
                    uvIndex = (uvReference == FbxGeometryElement::eDirect)
                        ? cpIndex
                        : uvElement->GetIndexArray().GetAt(cpIndex);
                }
                else if (uvMapping == FbxGeometryElement::eByPolygonVertex) {
                    uvIndex = (uvReference == FbxGeometryElement::eDirect)
                        ? polyVertCounter
                        : uvElement->GetIndexArray().GetAt(polyVertCounter);
                }

                if (uvIndex >= 0 && uvIndex < uvElement->GetDirectArray().GetCount()) {
                    FbxVector2 uv = uvElement->GetDirectArray().GetAt(uvIndex);
                    vertex.Tex = XMFLOAT2((float)uv[0], 1.0f - (float)uv[1]);
                }
            }

            indices.push_back((unsigned int)vertices.size());
            vertices.push_back(vertex);
            polyVertCounter++;
        }
    }

    // indices ya construidos en el loop anterior (uno por polygon-vertex)

    MeshComponent meshData;
    meshData.m_name      = node->GetName();
    meshData.m_vertex    = vertices;
    meshData.m_index     = indices;
    meshData.m_numVertex = (unsigned int)vertices.size();
    meshData.m_numIndex  = (unsigned int)indices.size();
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
