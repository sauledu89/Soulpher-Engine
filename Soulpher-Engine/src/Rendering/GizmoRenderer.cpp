/**
 * @file GizmoRenderer.cpp
 * @brief Implementación de GizmoRenderer: geometría procedural + draw calls unlit.
 *
 * @details
 * Toda la geometría se genera en CPU una sola vez (en `init()`) y se sube a GPU como
 * 4 pares de vertex/index buffer estáticos — no hay generación de geometría por frame.
 * Cada handle es un sólido de revolución simple (cilindro, cono, cubo o toro) descrito
 * a mano con senos/cosenos, sin depender de ninguna librería de mallas externa.
 *
 * @note [GameDev] Generar primitivas "a mano" (en vez de cargar un .fbx/.obj) es la
 * técnica estándar para geometría de herramientas de editor: gizmos, iconos de luces,
 * volúmenes de colisión de debug, etc. Motores como Unreal tienen una librería interna
 * de "debug draw" (`DrawDebugSphere`, `DrawDebugCone`...) que hace exactamente esto —
 * generar vértices procedurales en tiempo de ejecución en vez de shipear assets para
 * geometría que nunca aparece en el juego final.
 *
 * @ingroup rendering
 */
#include "Rendering/GizmoRenderer.h"
#include "Device.h"
#include "DeviceContext.h"
#include "EngineUtilities/Utilities/LayoutBuilder.h"
#include <cmath>

namespace {
constexpr int   kRadialSegments = 12;   ///< Segmentos alrededor del cilindro/cono.
constexpr int   kRingSegments   = 32;   ///< Segmentos alrededor del anillo de rotación.
constexpr int   kTubeSegments   = 8;    ///< Segmentos de la sección transversal del anillo (torus).
constexpr float kShaftRadius     = 0.018f;
constexpr float kConeRadius      = 0.06f;
constexpr float kCubeHalfExtent  = 0.07f;
constexpr float kTubeRadius      = 0.02f;
constexpr float kCenterHalfExtent = 0.12f; ///< Handle central de escala uniforme (Axis::All).
}

XMVECTOR GizmoRenderer::axisDirectionVec(Axis axis) {
    switch (axis) {
        case Axis::X: return XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
        case Axis::Y: return XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
        case Axis::Z: return XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
        default:      return XMVectorZero();
    }
}

XMMATRIX GizmoRenderer::axisAlignRotation(Axis axis) {
    switch (axis) {
        case Axis::X: return XMMatrixIdentity();
        case Axis::Y: return XMMatrixRotationZ(XM_PIDIV2);   // +X -> +Y
        case Axis::Z: return XMMatrixRotationY(-XM_PIDIV2);  // +X -> +Z
        default:      return XMMatrixIdentity();
    }
}

HRESULT GizmoRenderer::init(Device& device) {
    LayoutBuilder layout;
    layout.Add("POSITION", DXGI_FORMAT_R32G32B32_FLOAT);

    HRESULT hr = m_shader.init(device, "Gizmo.fx", layout);
    if (FAILED(hr)) {
        LOG_ERROR("GizmoRenderer", "init", "Failed to compile Gizmo.fx");
        return hr;
    }

    hr = m_cbFrame.init(device, sizeof(CBGizmoFrame));
    if (FAILED(hr)) return hr;

    hr = m_cbObject.init(device, sizeof(CBGizmoObject));
    if (FAILED(hr)) return hr;

    // Depth test y write deshabilitados: el gizmo siempre se dibuja encima de la escena.
    hr = m_depthDisabled.init(device, false, D3D11_DEPTH_WRITE_MASK_ZERO, D3D11_COMPARISON_ALWAYS);
    if (FAILED(hr)) return hr;

    hr = m_rasterizer.init(device, D3D11_FILL_SOLID, D3D11_CULL_BACK, false, false);
    if (FAILED(hr)) return hr;

    std::vector<GizmoVertex> verts;
    std::vector<unsigned int> indices;

    verts.clear(); indices.clear();
    buildArrowGeometry(verts, indices);
    hr = buildMesh(device, m_arrowMesh, verts, indices);
    if (FAILED(hr)) return hr;

    verts.clear(); indices.clear();
    buildCubeHandleGeometry(verts, indices);
    hr = buildMesh(device, m_cubeMesh, verts, indices);
    if (FAILED(hr)) return hr;

    verts.clear(); indices.clear();
    buildRingGeometry(verts, indices);
    hr = buildMesh(device, m_ringMesh, verts, indices);
    if (FAILED(hr)) return hr;

    verts.clear(); indices.clear();
    buildCenterHandleGeometry(verts, indices);
    hr = buildMesh(device, m_centerMesh, verts, indices);
    if (FAILED(hr)) return hr;

    LOG_MESSAGE("GizmoRenderer", "init", "OK");
    return S_OK;
}

HRESULT GizmoRenderer::buildMesh(Device& device, GizmoMesh& mesh,
                                  const std::vector<GizmoVertex>& verts,
                                  const std::vector<unsigned int>& indices) {
    HRESULT hr = mesh.vb.init(device, verts.data(),
        static_cast<unsigned int>(verts.size()), sizeof(GizmoVertex), D3D11_BIND_VERTEX_BUFFER);
    if (FAILED(hr)) return hr;

    hr = mesh.ib.init(device, indices.data(),
        static_cast<unsigned int>(indices.size()), sizeof(unsigned int), D3D11_BIND_INDEX_BUFFER);
    if (FAILED(hr)) return hr;

    mesh.indexCount = static_cast<unsigned int>(indices.size());
    return S_OK;
}

// ── Geometría procedural (espacio unitario, eje principal = +X) ───────────────────────

void GizmoRenderer::buildArrowGeometry(std::vector<GizmoVertex>& verts, std::vector<unsigned int>& indices) {
    // --- Cilindro (mango): x in [0, kShaftEnd], radio kShaftRadius ---
    const unsigned int shaftBase = static_cast<unsigned int>(verts.size());
    for (int i = 0; i < kRadialSegments; ++i) {
        float t = (float)i / kRadialSegments * XM_2PI;
        float y = cosf(t) * kShaftRadius, z = sinf(t) * kShaftRadius;
        verts.push_back({ XMFLOAT3(0.0f, y, z) });
        verts.push_back({ XMFLOAT3(kShaftEnd, y, z) });
    }
    for (int i = 0; i < kRadialSegments; ++i) {
        unsigned int i0 = shaftBase + i * 2;
        unsigned int i1 = shaftBase + i * 2 + 1;
        unsigned int i2 = shaftBase + ((i + 1) % kRadialSegments) * 2;
        unsigned int i3 = shaftBase + ((i + 1) % kRadialSegments) * 2 + 1;
        indices.insert(indices.end(), { i0, i2, i1,  i1, i2, i3 });
    }

    // --- Cono (punta): base en x=kShaftEnd (radio kConeRadius), ápice en x=kArmLength ---
    const unsigned int coneBaseCenter = static_cast<unsigned int>(verts.size());
    verts.push_back({ XMFLOAT3(kShaftEnd, 0.0f, 0.0f) }); // centro de la base (para tapar el hueco)
    const unsigned int coneApex = static_cast<unsigned int>(verts.size());
    verts.push_back({ XMFLOAT3(kArmLength, 0.0f, 0.0f) });
    const unsigned int coneRingBase = static_cast<unsigned int>(verts.size());
    for (int i = 0; i < kRadialSegments; ++i) {
        float t = (float)i / kRadialSegments * XM_2PI;
        verts.push_back({ XMFLOAT3(kShaftEnd, cosf(t) * kConeRadius, sinf(t) * kConeRadius) });
    }
    for (int i = 0; i < kRadialSegments; ++i) {
        unsigned int a = coneRingBase + i;
        unsigned int b = coneRingBase + (i + 1) % kRadialSegments;
        // Tapa de la base (mirando hacia -X)
        indices.insert(indices.end(), { coneBaseCenter, b, a });
        // Superficie lateral del cono (mirando hacia afuera)
        indices.insert(indices.end(), { a, b, coneApex });
    }
}

void GizmoRenderer::buildCubeHandleGeometry(std::vector<GizmoVertex>& verts, std::vector<unsigned int>& indices) {
    // --- Cilindro (mango): igual que la flecha pero más corto, hasta la base del cubo ---
    const float shaftEnd = 0.80f;
    const unsigned int shaftBase = static_cast<unsigned int>(verts.size());
    for (int i = 0; i < kRadialSegments; ++i) {
        float t = (float)i / kRadialSegments * XM_2PI;
        float y = cosf(t) * kShaftRadius, z = sinf(t) * kShaftRadius;
        verts.push_back({ XMFLOAT3(0.0f, y, z) });
        verts.push_back({ XMFLOAT3(shaftEnd, y, z) });
    }
    for (int i = 0; i < kRadialSegments; ++i) {
        unsigned int i0 = shaftBase + i * 2;
        unsigned int i1 = shaftBase + i * 2 + 1;
        unsigned int i2 = shaftBase + ((i + 1) % kRadialSegments) * 2;
        unsigned int i3 = shaftBase + ((i + 1) % kRadialSegments) * 2 + 1;
        indices.insert(indices.end(), { i0, i2, i1,  i1, i2, i3 });
    }

    // --- Cubo sólido centrado en x = kArmLength - kCubeHalfExtent ---
    const float cx = kArmLength - kCubeHalfExtent;
    const float h = kCubeHalfExtent;
    const XMFLOAT3 c[8] = {
        { cx - h, -h, -h }, { cx + h, -h, -h }, { cx + h,  h, -h }, { cx - h,  h, -h },
        { cx - h, -h,  h }, { cx + h, -h,  h }, { cx + h,  h,  h }, { cx - h,  h,  h },
    };
    const unsigned int cubeBase = static_cast<unsigned int>(verts.size());
    for (const XMFLOAT3& p : c) verts.push_back({ p });

    auto quad = [&](unsigned int a, unsigned int b, unsigned int cIdx, unsigned int d) {
        indices.insert(indices.end(), {
            cubeBase + a, cubeBase + b, cubeBase + cIdx,
            cubeBase + a, cubeBase + cIdx, cubeBase + d });
    };
    quad(0, 1, 2, 3); // -Z
    quad(5, 4, 7, 6); // +Z
    quad(4, 0, 3, 7); // -X
    quad(1, 5, 6, 2); // +X
    quad(3, 2, 6, 7); // +Y
    quad(4, 5, 1, 0); // -Y
}

/**
 * @brief Genera un toro (dona) delgado en el plano YZ, representando el aro de rotación
 * alrededor del eje +X.
 *
 * @details
 * Parametrización estándar de un toro con dos ángulos independientes:
 *  - `theta` recorre el círculo GRANDE (el aro en sí, radio `kRingRadius`) en el plano YZ.
 *  - `phi` recorre el círculo PEQUEÑO (la sección transversal/"grosor del tubo", radio
 *    `kTubeRadius`) alrededor de cada punto del círculo grande.
 * Por cada combinación `(theta, phi)` se genera un vértice; conectar vértices vecinos en
 * ambas direcciones con 2 triángulos por celda cierra la superficie del tubo.
 *
 * @note [GameDev] Esta és la parametrización de toro que se enseña en cualquier curso de
 * gráficos por computadora (la misma que usan Blender/Maya al generar un primitivo
 * "Torus"), aplicada aquí con el eje principal en X en vez del Z convencional para que
 * coincida con la convención "plantilla apunta a +X" del resto de esta clase. Vale la
 * pena notar el costo: `kRingSegments * kTubeSegments` vértices (32×8 = 256 aquí) — para
 * un gizmo eso es trivial, pero es el mismo cálculo de "resolución angular al cuadrado"
 * que hace que las esferas/toros de alta calidad sean notablemente más caras que cajas o
 * conos en un presupuesto de triángulos de escena real.
 */
void GizmoRenderer::buildRingGeometry(std::vector<GizmoVertex>& verts, std::vector<unsigned int>& indices) {
    const unsigned int base = static_cast<unsigned int>(verts.size());
    for (int i = 0; i < kRingSegments; ++i) {
        float theta = (float)i / kRingSegments * XM_2PI;
        float cy = cosf(theta), cz = sinf(theta);
        for (int j = 0; j < kTubeSegments; ++j) {
            float phi = (float)j / kTubeSegments * XM_2PI;
            float radial = kRingRadius + kTubeRadius * cosf(phi);
            float xOff   = kTubeRadius * sinf(phi);
            verts.push_back({ XMFLOAT3(xOff, radial * cy, radial * cz) });
        }
    }
    for (int i = 0; i < kRingSegments; ++i) {
        int iNext = (i + 1) % kRingSegments;
        for (int j = 0; j < kTubeSegments; ++j) {
            int jNext = (j + 1) % kTubeSegments;
            unsigned int a = base + i * kTubeSegments + j;
            unsigned int b = base + iNext * kTubeSegments + j;
            unsigned int c = base + iNext * kTubeSegments + jNext;
            unsigned int d = base + i * kTubeSegments + jNext;
            indices.insert(indices.end(), { a, b, c,  a, c, d });
        }
    }
}

void GizmoRenderer::buildCenterHandleGeometry(std::vector<GizmoVertex>& verts, std::vector<unsigned int>& indices) {
    // Cubo pequeño centrado exactamente en el origen — sin mango, a diferencia de los handles
    // por eje. axisAlignRotation(Axis::All) es identidad, así que queda anclado al centro del gizmo.
    const float h = kCenterHalfExtent;
    const XMFLOAT3 c[8] = {
        { -h, -h, -h }, { h, -h, -h }, { h,  h, -h }, { -h,  h, -h },
        { -h, -h,  h }, { h, -h,  h }, { h,  h,  h }, { -h,  h,  h },
    };
    const unsigned int base = static_cast<unsigned int>(verts.size());
    for (const XMFLOAT3& p : c) verts.push_back({ p });

    auto quad = [&](unsigned int a, unsigned int b, unsigned int cIdx, unsigned int d) {
        indices.insert(indices.end(), {
            base + a, base + b, base + cIdx,
            base + a, base + cIdx, base + d });
    };
    quad(0, 1, 2, 3); // -Z
    quad(5, 4, 7, 6); // +Z
    quad(4, 0, 3, 7); // -X
    quad(1, 5, 6, 2); // +X
    quad(3, 2, 6, 7); // +Y
    quad(4, 5, 1, 0); // -Y
}

// ── Render ──────────────────────────────────────────────────────────────────────────

/**
 * @brief Orienta, escala, traslada y dibuja una malla plantilla ya cargada en GPU.
 * @details `World = Scale * AxisRotation * Translation` (orden fila-vector, igual
 * convención SRT que usa `Transform::update()` para actores normales). Se transpone
 * antes de subir porque HLSL espera matrices column-major por defecto.
 */
void GizmoRenderer::drawMesh(DeviceContext& deviceContext, GizmoMesh& mesh,
                              const XMFLOAT3& center, float scale,
                              Axis axis, const XMFLOAT4& color) {
    XMMATRIX world = axisAlignRotation(axis) *
                     XMMatrixScaling(scale, scale, scale) *
                     XMMatrixTranslation(center.x, center.y, center.z);

    CBGizmoObject obj{};
    XMStoreFloat4x4(&obj.World, XMMatrixTranspose(world));
    obj.Color = color;
    m_cbObject.update(deviceContext, nullptr, 0, nullptr, &obj, 0, 0);
    m_cbObject.render(deviceContext, 1, 1, true);

    deviceContext.IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    mesh.vb.render(deviceContext, 0, 1);
    mesh.ib.render(deviceContext, 0, 1, false, DXGI_FORMAT_R32_UINT);
    deviceContext.DrawIndexed(mesh.indexCount, 0, 0);
}

/**
 * @copydoc GizmoRenderer::render
 * @details Implementación: selecciona la malla plantilla según `mode`, y para cada uno
 * de los 3 ejes mundiales resuelve un color final en 3 niveles — color base del eje,
 * aclarado si está en hover, o amarillo sólido si se está arrastrando — antes de
 * delegar el dibujo real a `drawMesh()`.
 *
 * @note [GameDev] Esta jerarquía de 3 estados visuales (reposo/hover/activo) es el
 * mismo patrón de "affordance" que cualquier UI, de botones HTML a los gizmos de
 * Unreal/Unity: el color le dice al usuario, antes de que haga clic, exactamente qué
 * va a agarrar (hover) y, mientras arrastra, qué está agarrando ahora mismo (drag) —
 * sin este feedback, mover un objeto en 3D a ciegas con el mouse sería muy propenso a
 * error (agarrar el eje equivocado sin darse cuenta).
 */
void GizmoRenderer::render(DeviceContext& deviceContext,
                            const XMMATRIX& viewProj,
                            const XMFLOAT3& center,
                            float screenScale,
                            Mode mode,
                            Axis hoverAxis,
                            Axis dragAxis) {
    if (mode == Mode::None) return;

    m_shader.render(deviceContext);
    m_rasterizer.render(deviceContext);
    m_depthDisabled.render(deviceContext, 0, false);

    CBGizmoFrame frame{};
    XMStoreFloat4x4(&frame.ViewProj, XMMatrixTranspose(viewProj));
    m_cbFrame.update(deviceContext, nullptr, 0, nullptr, &frame, 0, 0);
    m_cbFrame.render(deviceContext, 0, 1, false);

    GizmoMesh* mesh = (mode == Mode::Translate) ? &m_arrowMesh
                     : (mode == Mode::Scale)     ? &m_cubeMesh
                                                  : &m_ringMesh;

    const XMFLOAT4 kColors[3] = {
        XMFLOAT4(0.85f, 0.20f, 0.20f, 1.0f), // X - rojo
        XMFLOAT4(0.25f, 0.80f, 0.25f, 1.0f), // Y - verde
        XMFLOAT4(0.25f, 0.45f, 0.95f, 1.0f), // Z - azul
    };
    const Axis kAxes[3] = { Axis::X, Axis::Y, Axis::Z };

    for (int i = 0; i < 3; ++i) {
        XMFLOAT4 c = kColors[i];
        if (kAxes[i] == dragAxis) {
            c = XMFLOAT4(1.0f, 0.85f, 0.15f, 1.0f); // amarillo: arrastrando
        } else if (kAxes[i] == hoverAxis) {
            c = XMFLOAT4(c.x * 0.5f + 0.5f, c.y * 0.5f + 0.5f, c.z * 0.5f + 0.5f, 1.0f); // aclarado: hover
        }
        drawMesh(deviceContext, *mesh, center, screenScale, kAxes[i], c);
    }

    // Handle central de escala uniforme — solo en Mode::Scale, no tiene eje ni rotación propia.
    if (mode == Mode::Scale) {
        XMFLOAT4 centerColor = XMFLOAT4(0.85f, 0.85f, 0.85f, 1.0f); // gris claro neutro
        if (dragAxis == Axis::All) {
            centerColor = XMFLOAT4(1.0f, 0.85f, 0.15f, 1.0f); // amarillo: arrastrando
        } else if (hoverAxis == Axis::All) {
            centerColor = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f); // blanco: hover
        }
        drawMesh(deviceContext, m_centerMesh, center, screenScale, Axis::All, centerColor);
    }
}

void GizmoRenderer::destroy() {
    m_arrowMesh.vb.destroy();  m_arrowMesh.ib.destroy();
    m_cubeMesh.vb.destroy();   m_cubeMesh.ib.destroy();
    m_ringMesh.vb.destroy();   m_ringMesh.ib.destroy();
    m_centerMesh.vb.destroy(); m_centerMesh.ib.destroy();
    m_cbObject.destroy();
    m_cbFrame.destroy();
    m_depthDisabled.destroy();
    m_rasterizer.destroy();
    m_shader.destroy();
}
