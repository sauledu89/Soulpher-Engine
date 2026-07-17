/**
 * @file LightGizmoRenderer.h
 * @brief Iconos wireframe de los Light Actors: flecha (Directional), esfera (Point), cono (Spot).
 *
 * @details
 * Reutiliza el shader unlit `Gizmo.fx` (mismo par de cbuffers CBGizmoFrame/CBGizmoObject que
 * GizmoRenderer) pero con topología LINELIST en vez de TRIANGLELIST — es una clase separada
 * porque conceptualmente dibuja algo distinto (íconos informativos de TODAS las luces de la
 * escena, no los handles de edición del actor seleccionado) y usa depth test normal (LESS)
 * para quedar ocultos detrás de geometría opaca, a diferencia del gizmo de transformación que
 * siempre está encima.
 *
 * @see GizmoRenderer, LightComponent, RenderTypes.h (LightType)
 */
#pragma once
#include "Prerequisites.h"
#include "Buffer.h"
#include "ShaderProgram.h"
#include "DepthStencilState.h"
#include "RasterizerState.h"
#include "Rendering/RenderTypes.h"
#include "Rendering/GizmoRenderer.h" // reutiliza CBGizmoFrame / CBGizmoObject
#include <vector>

class Device;
class DeviceContext;

class LightGizmoRenderer {
public:
    /** @brief Snapshot de un Light Actor listo para dibujar este frame. */
    struct Instance {
        LightType type;
        XMFLOAT3  position;
        XMFLOAT3  direction;    ///< Normalizado; usado por Directional/Spot (ignorado en Point).
        float     range;        ///< Point/Spot — radio de la esfera / longitud del cono.
        float     spotAngleRad; ///< Solo Spot — semiángulo del cono, en radianes.
        XMFLOAT4  color;        ///< Color del ícono (resaltado si el actor está seleccionado).
    };

    // Compartidas con el picking en BaseApp — deben coincidir con la geometría/escala usada aquí.
    static constexpr float kArrowLength   = 2.0f;  ///< Directional: longitud fija en unidades de mundo.
    static constexpr float kIconPickScale = 0.03f; ///< Radio de picking = distancia_a_camara * este factor.

    HRESULT init(Device& device);
    void destroy();

    /** @brief Dibuja el ícono de cada instancia sobre el render target actualmente enlazado. */
    void render(DeviceContext& deviceContext, const XMMATRIX& viewProj,
                const std::vector<Instance>& instances);

private:
    struct LineVertex { XMFLOAT3 Pos; };
    struct GizmoLineMesh {
        Buffer vb;
        unsigned int vertexCount = 0;
    };

    HRESULT buildLineMesh(Device& device, GizmoLineMesh& mesh, const std::vector<LineVertex>& verts);

    void buildArrowGeometry(std::vector<LineVertex>& verts);  ///< Directional: apunta a +Z local.
    void buildSphereGeometry(std::vector<LineVertex>& verts); ///< Point: 3 círculos ortogonales, radio 1.
    void buildConeGeometry(std::vector<LineVertex>& verts);   ///< Spot: ápice en origen, base en z=1 radio 1.

    void drawInstance(DeviceContext& deviceContext, const Instance& instance);

    ShaderProgram      m_shader;
    Buffer             m_cbFrame;   ///< b0: CBGizmoFrame (ViewProj).
    Buffer             m_cbObject;  ///< b1: CBGizmoObject (World, Color).
    DepthStencilState  m_depthStencil; ///< Depth enable, write OFF, LESS — se oculta tras geometría opaca.
    RasterizerState    m_rasterizer;

    GizmoLineMesh m_arrowMesh;
    GizmoLineMesh m_sphereMesh;
    GizmoLineMesh m_coneMesh;
};
