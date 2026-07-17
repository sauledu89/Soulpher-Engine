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
            XMFLOAT3(v.Position.X, v.Position.Y, v.Position.Z),
            XMFLOAT3(0.0f, 1.0f, 0.0f),   // Normal — OBJ loader no la expone; placeholder
            XMFLOAT3(1.0f, 0.0f, 0.0f),   // Tangent
            XMFLOAT3(0.0f, 0.0f, 1.0f),   // Bitangent
            XMFLOAT2(v.TextureCoordinate.X, 1.0f - v.TextureCoordinate.Y)
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
        LOG_ERROR("ModelLoader", "FbxManager::Create()", "Unable to create FBX Manager!");
        return false;
    }
    else {
        LOG_MESSAGE("ModelLoader", "ModelLoader", "Autodesk FBX SDK version " + std::string(lSdkManager->GetVersion()));
    }

    FbxIOSettings* ios = FbxIOSettings::Create(lSdkManager, IOSROOT);
    lSdkManager->SetIOSettings(ios);

    lScene = FbxScene::Create(lSdkManager, "MyScene");
    if (!lScene) {
        LOG_ERROR("ModelLoader", "FbxScene::Create()", "Unable to create FBX Scene!");
        return false;
    }
    else {
        LOG_MESSAGE("ModelLoader", "ModelLoader", "FBX Scene created successfully.");
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
            LOG_ERROR("ModelLoader", "FbxImporter::Create()", "Unable to create FBX Importer!");
            return false;
        }
        else {
            LOG_MESSAGE("ModelLoader", "ModelLoader", "FBX Importer created successfully.");
        }

        if (!lImporter->Initialize(filePath.c_str(), -1, lSdkManager->GetIOSettings())) {
            LOG_ERROR("ModelLoader", "FbxImporter::Initialize()", "Unable to initialize FBX Importer! Error: " + std::string(lImporter->GetStatus().GetErrorString()));
            lImporter->Destroy();
            return false;
        }

        if (!lImporter->Import(lScene)) {
            LOG_ERROR("ModelLoader", "FbxImporter::Import()", "Unable to import FBX Scene! Error: " + std::string(lImporter->GetStatus().GetErrorString()));
            lImporter->Destroy();
            return false;
        }
        else {
            LOG_MESSAGE("ModelLoader", "ModelLoader", "FBX Scene imported successfully.");
            modelName = lImporter->GetFileName();
        }

        lImporter->Destroy();

        // Triangular toda la escena (quads y n-gons → triangulos).
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
            LOG_ERROR("ModelLoader", "FbxScene::GetRootNode()", "Unable to get root node from FBX Scene!");
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

    // Generar normales si el mesh no las tiene (garantia antes de leer elementos).
    if (mesh->GetElementNormalCount() == 0) {
        mesh->GenerateNormals(true, true);
    }

    // Identificar el primer UV set para generacion de tangentes.
    const char* uvSetName = nullptr;
    {
        FbxStringList uvSets;
        mesh->GetUVSetNames(uvSets);
        if (uvSets.GetCount() > 0) {
            uvSetName = uvSets[0];
        }
    }

    // Generar tangentes/bitangentes si no existen.
    if (mesh->GetElementTangentCount() == 0 && uvSetName) {
        mesh->GenerateTangentsData(uvSetName);
    }

    // -- Elementos de geometria --
    const FbxGeometryElementUV*       uvElem  = mesh->GetElementUVCount()       > 0 ? mesh->GetElementUV(0)       : nullptr;
    const FbxGeometryElementTangent*  tanElem = mesh->GetElementTangentCount()  > 0 ? mesh->GetElementTangent(0)  : nullptr;
    const FbxGeometryElementBinormal* binElem = mesh->GetElementBinormalCount() > 0 ? mesh->GetElementBinormal(0) : nullptr;

    // -- Transforms de nodo --
    // globalTransform lleva cada pieza (cabeza, pies, manos...) a su posicion en escena,
    // ensamblando correctamente el modelo. El Actor::Transform posiciona el conjunto en el mundo.
    // normalTransform = (globalTransform^{-1})^T corrige normales ante escalados no uniformes.
    FbxAMatrix globalTransform = node->EvaluateGlobalTransform();
    FbxAMatrix normalTransform = globalTransform.Inverse().Transpose();

    // Helper: leer elemento UV por control-point o polygon-vertex index.
    auto readUV = [&](int cpIdx, int pvIdx) -> FbxVector2 {
        if (!uvElem) return FbxVector2(0.0, 0.0);
        using E = FbxGeometryElement;
        if (uvElem->GetMappingMode() == E::eByControlPoint) {
            int idx = (uvElem->GetReferenceMode() == E::eIndexToDirect)
                ? uvElem->GetIndexArray().GetAt(cpIdx) : cpIdx;
            return uvElem->GetDirectArray().GetAt(idx);
        }
        // eByPolygonVertex
        int idx = (uvElem->GetReferenceMode() == E::eIndexToDirect)
            ? uvElem->GetIndexArray().GetAt(pvIdx) : pvIdx;
        return uvElem->GetDirectArray().GetAt(idx);
    };

    // Helper: leer elemento FbxVector4 (tangente o bitangente).
    auto readV4 = [&](auto* elem, int cpIdx, int pvIdx) -> FbxVector4 {
        if (!elem) return FbxVector4(0.0, 0.0, 0.0, 0.0);
        using E = FbxGeometryElement;
        if (elem->GetMappingMode() == E::eByControlPoint) {
            int idx = (elem->GetReferenceMode() == E::eIndexToDirect)
                ? elem->GetIndexArray().GetAt(cpIdx) : cpIdx;
            return elem->GetDirectArray().GetAt(idx);
        }
        int idx = (elem->GetReferenceMode() == E::eIndexToDirect)
            ? elem->GetIndexArray().GetAt(pvIdx) : pvIdx;
        return elem->GetDirectArray().GetAt(idx);
    };

    std::vector<SimpleVertex> vertices;
    std::vector<unsigned int> indices;
    vertices.reserve(static_cast<size_t>(mesh->GetPolygonCount()) * 3);
    indices.reserve(static_cast<size_t>(mesh->GetPolygonCount()) * 3);

    for (int p = 0; p < mesh->GetPolygonCount(); ++p) {
        const int polySize = mesh->GetPolygonSize(p);
        for (int v = 0; v < polySize; ++v) {
            const int cpIndex = mesh->GetPolygonVertex(p, v);
            const int pvIndex = mesh->GetPolygonVertexIndex(p) + v;

            SimpleVertex out{};

            // Posicion: globalTransform ensambla la pieza en el espacio del modelo.
            FbxVector4 localPos = mesh->GetControlPointAt(cpIndex);
            FbxVector4 worldPos = globalTransform.MultT(localPos);
            out.Pos = XMFLOAT3((float)worldPos[0], (float)worldPos[1], (float)worldPos[2]);

            // Normal por polygon-vertex (mas precisa que por control point) + normalTransform.
            FbxVector4 N(0.0, 1.0, 0.0, 0.0);
            mesh->GetPolygonVertexNormal(p, v, N);
            FbxVector4 worldN = normalTransform.MultT(N);
            worldN.Normalize();
            out.Normal = XMFLOAT3((float)worldN[0], (float)worldN[1], (float)worldN[2]);

            // UV: GetTextureUVIndex es mas robusto para eByPolygonVertex directo.
            if (uvElem && uvSetName) {
                int uvIdx = mesh->GetTextureUVIndex(p, v);
                FbxVector2 uv = (uvIdx >= 0)
                    ? uvElem->GetDirectArray().GetAt(uvIdx)
                    : readUV(cpIndex, pvIndex);
                out.Tex = XMFLOAT2((float)uv[0], 1.0f - (float)uv[1]);
            }

            // Tangente y bitangente transformadas igual que la normal.
            if (tanElem) {
                FbxVector4 T  = readV4(tanElem, cpIndex, pvIndex);
                FbxVector4 wT = normalTransform.MultT(T);
                out.Tangent = XMFLOAT3((float)wT[0], (float)wT[1], (float)wT[2]);
            }
            if (binElem) {
                FbxVector4 B  = readV4(binElem, cpIndex, pvIndex);
                FbxVector4 wB = normalTransform.MultT(B);
                out.Bitangent = XMFLOAT3((float)wB[0], (float)wB[1], (float)wB[2]);
            }

            indices.push_back(static_cast<unsigned int>(vertices.size()));
            vertices.push_back(out);
        }
    }

    // -- Detectar y corregir winding en geometria espejada --
    // Un determinante de escala negativo en el globalTransform indica geometria reflejada.
    {
        FbxVector4 S = globalTransform.GetS();
        const bool mirrored = (S[0] * S[1] * S[2]) < 0.0;
        if (mirrored) {
            for (size_t i = 0; i + 2 < indices.size(); i += 3) {
                std::swap(indices[i + 1], indices[i + 2]);
            }
        }
    }

    // -- Generar tangentes por CPU si el FBX no las tenia --
    if (!tanElem) {
        auto add3 = [](XMFLOAT3& a, const XMFLOAT3& b) {
            a.x += b.x; a.y += b.y; a.z += b.z;
        };
        for (size_t i = 0; i + 2 < indices.size(); i += 3) {
            SimpleVertex& v0 = vertices[indices[i]];
            SimpleVertex& v1 = vertices[indices[i + 1]];
            SimpleVertex& v2 = vertices[indices[i + 2]];

            XMFLOAT3 e1 = { v1.Pos.x - v0.Pos.x, v1.Pos.y - v0.Pos.y, v1.Pos.z - v0.Pos.z };
            XMFLOAT3 e2 = { v2.Pos.x - v0.Pos.x, v2.Pos.y - v0.Pos.y, v2.Pos.z - v0.Pos.z };
            float du1 = v1.Tex.x - v0.Tex.x, dv1 = v1.Tex.y - v0.Tex.y;
            float du2 = v2.Tex.x - v0.Tex.x, dv2 = v2.Tex.y - v0.Tex.y;
            float denom = du1 * dv2 - du2 * dv1;
            float r = (fabsf(denom) < 1e-8f) ? 0.0f : 1.0f / denom;

            XMFLOAT3 T = {
                (e1.x * dv2 - e2.x * dv1) * r,
                (e1.y * dv2 - e2.y * dv1) * r,
                (e1.z * dv2 - e2.z * dv1) * r
            };
            XMFLOAT3 B = {
                (e2.x * du1 - e1.x * du2) * r,
                (e2.y * du1 - e1.y * du2) * r,
                (e2.z * du1 - e1.z * du2) * r
            };
            add3(v0.Tangent, T); add3(v1.Tangent, T); add3(v2.Tangent, T);
            add3(v0.Bitangent, B); add3(v1.Bitangent, B); add3(v2.Bitangent, B);
        }
    }

    // -- Gram-Schmidt: ortogonalizar T respecto a N y recalcular B --
    for (auto& v : vertices) {
        // Normalizar N
        float nLen = sqrtf(v.Normal.x * v.Normal.x + v.Normal.y * v.Normal.y + v.Normal.z * v.Normal.z);
        if (nLen > 1e-6f) {
            v.Normal.x /= nLen; v.Normal.y /= nLen; v.Normal.z /= nLen;
        } else {
            v.Normal = XMFLOAT3(0.0f, 1.0f, 0.0f);
        }
        // T = normalize(T - dot(T,N)*N)
        float dTN = v.Tangent.x * v.Normal.x + v.Tangent.y * v.Normal.y + v.Tangent.z * v.Normal.z;
        v.Tangent.x -= dTN * v.Normal.x;
        v.Tangent.y -= dTN * v.Normal.y;
        v.Tangent.z -= dTN * v.Normal.z;
        float tLen = sqrtf(v.Tangent.x * v.Tangent.x + v.Tangent.y * v.Tangent.y + v.Tangent.z * v.Tangent.z);
        if (tLen > 1e-6f) {
            v.Tangent.x /= tLen; v.Tangent.y /= tLen; v.Tangent.z /= tLen;
        }
        // B = cross(N, T) con handedness preservado
        XMFLOAT3 Bcalc = {
            v.Normal.y * v.Tangent.z - v.Normal.z * v.Tangent.y,
            v.Normal.z * v.Tangent.x - v.Normal.x * v.Tangent.z,
            v.Normal.x * v.Tangent.y - v.Normal.y * v.Tangent.x
        };
        float hand = (Bcalc.x * v.Bitangent.x + Bcalc.y * v.Bitangent.y + Bcalc.z * v.Bitangent.z) < 0.0f
            ? -1.0f : 1.0f;
        v.Bitangent = { Bcalc.x * hand, Bcalc.y * hand, Bcalc.z * hand };
    }

    MeshComponent meshData;
    meshData.m_name      = node->GetName();
    meshData.m_vertex    = std::move(vertices);
    meshData.m_index     = std::move(indices);
    meshData.m_numVertex = static_cast<int>(meshData.m_vertex.size());
    meshData.m_numIndex  = static_cast<int>(meshData.m_index.size());
    meshes.push_back(std::move(meshData));
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
