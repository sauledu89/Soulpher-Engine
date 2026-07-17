/**
 * @file LightComponent.h
 * @brief Componente ECS que describe una fuente de luz en la escena ("Light Actor").
 *
 * @details
 * Un `Actor` con un `LightComponent` adjunto (sin necesidad de `MeshComponent`) es lo
 * que `BaseApp` llama internamente un "Light Actor": aparece en el Hierarchy/Inspector
 * igual que cualquier otro actor, con su propio `Transform` para posición/dirección, y
 * cada frame `BaseApp::render()` recorre `m_actors` recolectando el `LightComponent` de
 * cada uno (vía `resolve()`) para llenar `RenderScene::lights`, que
 * `DeferredRenderer::updatePerFrame()` sube al `CBPerFrame` de la GPU.
 *
 * Tipos de luz soportados (ver `LightType` en RenderTypes.h):
 *  - **Directional** — luz solar; ilumina toda la escena desde una dirección (sin posición).
 *  - **Point** — bombilla; irradia en todas las direcciones con atenuación por distancia (`range`).
 *  - **Spot** — foco; cono de luz con ángulo (`spotAngle`) y atenuación por distancia.
 *
 * @note [GameDev] La distinción Directional/Point/Spot es fundamental en cualquier
 * motor 3D moderno. Unreal Engine los llama Directional/Point/Spot Light igual, y en
 * UE5 cada uno es literalmente un "Light Actor" con su propio Light Component — el
 * mismo patrón Actor+Component que se replica aquí. La luz direccional es la más
 * barata de evaluar por píxel (no hay cálculo de distancia ni atenuación), y en este
 * motor es también la única que genera shadow map — igual que en la mayoría de motores
 * indie/educativos, donde sombras de point/spot lights (que requieren 6 shadow maps de
 * cubemap o un shadow map por spot) se dejan fuera por su costo de implementación y
 * de rendimiento.
 *
 * @see DeferredRenderer, LightData, RenderTypes.h, LightGizmoRenderer (íconos visuales)
 */

#pragma once
#include "Prerequisites.h"
#include "ECS/Component.h"
#include "ECS/Transform.h"
#include "Rendering/RenderTypes.h"

class DeviceContext;

/**
 * @class LightComponent
 * @brief Componente ECS que aporta datos de luz a un Actor.
 *
 * @details
 * Adjuntar este componente a un `Actor` (típicamente sin `MeshComponent`: un Light
 * Actor no necesita geometría propia, solo un ícono de editor — ver
 * `LightGizmoRenderer`) hace que `BaseApp::render()` lo recolecte en `RenderScene.lights`
 * cada frame, de donde `DeferredRenderer` lo consume para el lighting pass. Si
 * `castShadow` está habilitado y el tipo es Directional, además participa en el
 * shadow pass (`DeferredRenderer::updateLightMatrices`) desde su propia perspectiva.
 *
 * Este componente NO conoce su posición/dirección en el mundo por sí mismo — solo
 * guarda parámetros "locales" del tipo de luz (color, intensidad, range, spotAngle).
 * La posición/dirección real se obtienen combinando estos datos con el `Transform`
 * del Actor dueño en `resolve()`, llamado una vez por frame por quien construye la
 * escena a renderizar.
 *
 * @note [GameDev] En Unreal Engine 5 este concepto equivale a un "Light Component"
 * dentro de un "Light Actor" (`APointLight`, `ASpotLight`, `ADirectionalLight`, cada
 * uno envolviendo un `ULightComponent`). La separación Actor/Component permite
 * reutilizar el mismo sistema de iluminación para objetos dinámicos (ej. una antorcha
 * que se mueve, una linterna que sigue al jugador) sin duplicar código: cualquier
 * Actor puede convertirse en luz con solo agregarle el componente, exactamente como
 * cualquier Actor puede volverse visible con un `MeshComponent`.
 */
class LightComponent : public Component {
public:
    /** @brief Constructor: inicializa con tipo ECS dedicado LIGHT. */
    LightComponent() : Component(ComponentType::LIGHT) {}

    /** @brief No requiere inicialización GPU. */
    void init()                            override {}
    /** @brief Sin lógica de actualización por defecto. */
    void update(float /*deltaTime*/)       override {}
    /** @brief No emite draw calls directamente. */
    void render(DeviceContext& /*dc*/)     override {}
    /** @brief Sin recursos GPU que liberar. */
    void destroy()                         override {}

    /** @brief Acceso mutable a los parámetros de luz (tipo, color, dirección, etc.). */
    LightData&       getLightData()       { return m_light; }
    /** @brief Acceso de solo lectura a los parámetros de luz. */
    const LightData& getLightData() const { return m_light; }

    /**
     * @brief Combina los parámetros de este componente con la posición/dirección del
     * Transform del Actor dueño, listo para subir a `RenderScene::lights`.
     * @param transform Transform del Actor que posee este LightComponent.
     * @return Copia de `getLightData()` con `position` y `direction` recalculados.
     *
     * @details `direction` se deriva rotando un vector de referencia "hacia abajo"
     * (0,-1,0) por la rotación del Transform — un Directional/Spot Light sin rotación
     * apunta derecho hacia abajo (misma convención que ya usaba la luz global de la
     * escena), y rotar el actor inclina el rayo de forma intuitiva.
     *
     * @note [GameDev] Este patrón — "combinar datos locales de un componente con el
     * Transform de su dueño para obtener el dato final en espacio mundo" — es
     * universal en motores basados en ECS/Component. Es exactamente lo que hace
     * `GetComponentTransform()` en Unreal (combina el Transform del componente con el
     * del Actor padre) o `transform.TransformDirection()` en Unity. La razón de no
     * guardar `position`/`direction` ya resueltos DENTRO del componente es que el
     * Transform puede cambiar cada frame (el usuario arrastra el gizmo, una animación
     * mueve la luz) — recalcular en `resolve()` garantiza que la luz nunca queda
     * "desincronizada" de su propio Transform, al costo de un `XMMatrixRotationRollPitchYaw`
     * por luz por frame (trivial para las ≤8 luces que soporta este motor).
     */
    LightData resolve(const Transform& transform) const {
        LightData resolved = m_light;
        const EU::Vector3& pos = transform.getPosition();
        resolved.position = pos;

        if (resolved.type != LightType::Point) {
            const EU::Vector3& rot = transform.getRotation();
            XMMATRIX rotMatrix = XMMatrixRotationRollPitchYaw(rot.x, rot.y, rot.z);
            XMVECTOR baseDown = XMVectorSet(0.0f, -1.0f, 0.0f, 0.0f);
            XMVECTOR dir = XMVector3Normalize(XMVector3TransformNormal(baseDown, rotMatrix));
            XMFLOAT3 dirF;
            XMStoreFloat3(&dirF, dir);
            resolved.direction = EU::Vector3(dirF.x, dirF.y, dirF.z);
        }
        return resolved;
    }

    /**
     * @brief Habilita o deshabilita la generación de shadow map para esta luz.
     * @param value `true` para que genere sombras.
     *
     * @note Solo aplica a luces Directional en el `ForwardRenderer` actual.
     */
    void setCastShadow(bool value) { m_castShadow = value; }

    /** @brief Devuelve `true` si esta luz genera shadow map. */
    bool canCastShadow() const     { return m_castShadow;  }

private:
    LightData m_light;         ///< Parámetros de la luz (tipo, color, dirección, rango).
    bool      m_castShadow = false; ///< Si es true, el renderer genera shadow map para esta luz.
};
