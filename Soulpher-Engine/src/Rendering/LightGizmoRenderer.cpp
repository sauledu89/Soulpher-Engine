/**
 * @file LightGizmoRenderer.cpp
 * @brief Implementación de LightGizmoRenderer: geometría de líneas + draw calls unlit.
 * @ingroup rendering
 */
#include "Rendering/LightGizmoRenderer.h"
#include "Device.h"
#include "DeviceContext.h"
#include "EngineUtilities/Utilities/LayoutBuilder.h"
#include <cmath>

namespace {
constexpr int kCircleSegments = 24; ///< Segmentos por círculo (esfera/cono).

/**
 * @brief Matriz de orientación que mapea el eje local +Z ("forward" de la plantilla) a una
 * dirección arbitraria en mundo, con Gram-Schmidt para evitar degenerarse cuando la dirección
 * es casi paralela al "up" de referencia. Misma convención de basis vectors que Camera::lookAt.
 */
XMMATRIX BuildOrientationFromForward(const XMFLOAT3& forwardF) {
    XMVECTOR forward = XMVector3Normalize(XMLoadFloat3(&forwardF));
    XMVECTOR worldUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    if (fabsf(XMVectorGetX(XMVector3Dot(forward, worldUp))) > 0.98f) {
        worldUp = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
    }
    XMVECTOR right = XMVector3Normalize(XMVector3Cross(worldUp, forward));
    XMVECTOR up    = XMVector3Cross(forward, right);

    XMMATRIX m;
    m.r[0] = right;
    m.r[1] = up;
    m.r[2] = forward;
    m.r[3] = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
    return m;
}
}

HRESULT LightGizmoRenderer::init(Device& device) {
    LayoutBuilder layout;
    layout.Add("POSITION", DXGI_FORMAT_R32G32B32_FLOAT);

    HRESULT hr = m_shader.init(device, "Gizmo.fx", layout); // reutiliza el shader del gizmo de transformacion
    if (FAILED(hr)) {
        LOG_ERROR("LightGizmoRenderer", "init", "Failed to compile Gizmo.fx");
        return hr;
    }

    hr = m_cbFrame.init(device, sizeof(CBGizmoFrame));
    if (FAILED(hr)) return hr;

    hr = m_cbObject.init(device, sizeof(CBGizmoObject));
    if (FAILED(hr)) return hr;

    // Depth test normal (LESS) sin escritura: los iconos se ocultan detras de geometria opaca,
    // a diferencia del gizmo de transformacion (que siempre esta encima).
    hr = m_depthStencil.init(device, true, D3D11_DEPTH_WRITE_MASK_ZERO, D3D11_COMPARISON_LESS);
    if (FAILED(hr)) return hr;

    hr = m_rasterizer.init(device, D3D11_FILL_SOLID, D3D11_CULL_NONE, false, true);
    if (FAILED(hr)) return hr;

    std::vector<LineVertex> verts;

    verts.clear();
    buildArrowGeometry(verts);
    hr = buildLineMesh(device, m_arrowMesh, verts);
    if (FAILED(hr)) return hr;

    verts.clear();
    buildSphereGeometry(verts);
    hr = buildLineMesh(device, m_sphereMesh, verts);
    if (FAILED(hr)) return hr;

    verts.clear();
    buildConeGeometry(verts);
    hr = buildLineMesh(device, m_coneMesh, verts);
    if (FAILED(hr)) return hr;

    LOG_MESSAGE("LightGizmoRenderer", "init", "OK");
    return S_OK;
}

HRESULT LightGizmoRenderer::buildLineMesh(Device& device, GizmoLineMesh& mesh,
                                           const std::vector<LineVertex>& verts) {
    HRESULT hr = mesh.vb.init(device, verts.data(),
        static_cast<unsigned int>(verts.size()), sizeof(LineVertex), D3D11_BIND_VERTEX_BUFFER);
    if (FAILED(hr)) return hr;

    mesh.vertexCount = static_cast<unsigned int>(verts.size());
    return S_OK;
}

// ── Geometría procedural (espacio unitario, LINELIST: pares de vértices = un segmento) ────────

void LightGizmoRenderer::buildArrowGeometry(std::vector<LineVertex>& verts) {
    // Vara desde el origen hasta +Z, con una punta en V (4 segmentos cortos) cerca de la punta.
    const float len = 1.0f;
    verts.push_back({ XMFLOAT3(0, 0, 0) });
    verts.push_back({ XMFLOAT3(0, 0, len) });

    const float headBack = len - 0.15f;
    const float spread = 0.08f;
    const XMFLOAT3 tip(0, 0, len);
    const XMFLOAT3 sides[4] = {
        { spread, 0, headBack }, { -spread, 0, headBack },
        { 0, spread, headBack }, { 0, -spread, headBack },
    };
    for (const XMFLOAT3& s : sides) {
        verts.push_back({ tip });
        verts.push_back({ s });
    }
}

void LightGizmoRenderer::buildSphereGeometry(std::vector<LineVertex>& verts) {
    // 3 circulos ortogonales de radio 1 (planos XY, XZ, YZ) — escalados por 'range' al dibujar.
    auto appendCircle = [&](int axis) {
        for (int i = 0; i < kCircleSegments; ++i) {
            float a0 = (float)i / kCircleSegments * XM_2PI;
            float a1 = (float)(i + 1) / kCircleSegments * XM_2PI;
            XMFLOAT3 p0, p1;
            switch (axis) {
            case 0: p0 = XMFLOAT3(cosf(a0), sinf(a0), 0.0f); p1 = XMFLOAT3(cosf(a1), sinf(a1), 0.0f); break; // XY
            case 1: p0 = XMFLOAT3(cosf(a0), 0.0f, sinf(a0)); p1 = XMFLOAT3(cosf(a1), 0.0f, sinf(a1)); break; // XZ
            default: p0 = XMFLOAT3(0.0f, cosf(a0), sinf(a0)); p1 = XMFLOAT3(0.0f, cosf(a1), sinf(a1)); break; // YZ
            }
            verts.push_back({ p0 });
            verts.push_back({ p1 });
        }
    };
    appendCircle(0);
    appendCircle(1);
    appendCircle(2);
}

void LightGizmoRenderer::buildConeGeometry(std::vector<LineVertex>& verts) {
    // Apice en el origen, base circular en z=1 radio 1 (cono de 45 grados "unitario"). Se
    // escala de forma NO uniforme al dibujar: (tan(spotAngle)*range, tan(spotAngle)*range, range)
    // reproduce el cono real de cualquier angulo/rango sin regenerar geometria por instancia.
    const XMFLOAT3 apex(0, 0, 0);
    for (int i = 0; i < kCircleSegments; ++i) {
        float a0 = (float)i / kCircleSegments * XM_2PI;
        float a1 = (float)(i + 1) / kCircleSegments * XM_2PI;
        verts.push_back({ XMFLOAT3(cosf(a0), sinf(a0), 1.0f) });
        verts.push_back({ XMFLOAT3(cosf(a1), sinf(a1), 1.0f) });
    }
    // 4 lineas del apice a la base (0/90/180/270 grados) para mostrar la silueta del cono.
    for (int k = 0; k < 4; ++k) {
        float a = (float)k / 4.0f * XM_2PI;
        verts.push_back({ apex });
        verts.push_back({ XMFLOAT3(cosf(a), sinf(a), 1.0f) });
    }
}

// ── Render ──────────────────────────────────────────────────────────────────────────

void LightGizmoRenderer::drawInstance(DeviceContext& deviceContext, const Instance& instance) {
    GizmoLineMesh* mesh = nullptr;
    XMMATRIX scaleM = XMMatrixIdentity();
    XMMATRIX orient = XMMatrixIdentity();

    switch (instance.type) {
    case LightType::Directional:
        mesh   = &m_arrowMesh;
        scaleM = XMMatrixScaling(kArrowLength, kArrowLength, kArrowLength);
        orient = BuildOrientationFromForward(instance.direction);
        break;
    case LightType::Point:
        mesh   = &m_sphereMesh;
        scaleM = XMMatrixScaling(instance.range, instance.range, instance.range);
        break; // la esfera es simetrica, no necesita orientacion
    case LightType::Spot: {
        mesh = &m_coneMesh;
        float radius = tanf(instance.spotAngleRad) * instance.range;
        scaleM = XMMatrixScaling(radius, radius, instance.range);
        orient = BuildOrientationFromForward(instance.direction);
        break;
    }
    }
    if (!mesh) return;

    XMMATRIX world = scaleM * orient *
                      XMMatrixTranslation(instance.position.x, instance.position.y, instance.position.z);

    CBGizmoObject obj{};
    XMStoreFloat4x4(&obj.World, XMMatrixTranspose(world));
    obj.Color = instance.color;
    m_cbObject.update(deviceContext, nullptr, 0, nullptr, &obj, 0, 0);
    m_cbObject.render(deviceContext, 1, 1, true);

    deviceContext.IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
    mesh->vb.render(deviceContext, 0, 1);
    deviceContext.Draw(mesh->vertexCount, 0);
}

void LightGizmoRenderer::render(DeviceContext& deviceContext, const XMMATRIX& viewProj,
                                 const std::vector<Instance>& instances) {
    if (instances.empty()) return;

    m_shader.render(deviceContext);
    m_rasterizer.render(deviceContext);
    m_depthStencil.render(deviceContext, 0, false);

    CBGizmoFrame frame{};
    XMStoreFloat4x4(&frame.ViewProj, XMMatrixTranspose(viewProj));
    m_cbFrame.update(deviceContext, nullptr, 0, nullptr, &frame, 0, 0);
    m_cbFrame.render(deviceContext, 0, 1, false);

    for (const Instance& instance : instances) {
        drawInstance(deviceContext, instance);
    }
}

void LightGizmoRenderer::destroy() {
    m_arrowMesh.vb.destroy();
    m_sphereMesh.vb.destroy();
    m_coneMesh.vb.destroy();
    m_cbObject.destroy();
    m_cbFrame.destroy();
    m_depthStencil.destroy();
    m_rasterizer.destroy();
    m_shader.destroy();
}
