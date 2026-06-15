/**
 * @file LightComponent.h
 * @brief Componente ECS que describe una fuente de luz en la escena.
 *
 * @details
 * Un `LightComponent` adjunta a un `Actor` lo convierte en una fuente
 * de luz que el `ForwardRenderer` puede consumir durante el render.
 *
 * Tipos de luz soportados (ver `LightType` en RenderTypes.h):
 *  - **Directional** — luz solar; ilumina toda la escena desde una dirección.
 *  - **Point** — bombilla; irradia en todas las direcciones con atenuación por distancia.
 *  - **Spot** — foco; cono de luz con ángulo y atenuación.
 *
 * @note [GameDev] La distinción Directional/Point/Spot es fundamental en cualquier
 * motor 3D moderno. Unreal Engine los llama Directional/Point/Spot Light igual.
 * La luz direccional es la más barata porque no necesita cálculo de distancia.
 * En este engine, solo la luz direccional genera shadow map.
 *
 * @see ForwardRenderer, LightData, RenderTypes.h
 */

#pragma once
#include "Prerequisites.h"
#include "ECS/Component.h"
#include "Rendering/RenderTypes.h"

class DeviceContext;

/**
 * @class LightComponent
 * @brief Componente ECS que aporta datos de luz a un Actor.
 *
 * @details
 * Adjuntar este componente a un Actor hace que el `ForwardRenderer` lo tenga
 * en cuenta al construir la iluminación del frame. Si `castShadow` está
 * habilitado y el tipo es Directional, el renderer generará un shadow map
 * desde la perspectiva de esta luz.
 *
 * @note [GameDev] En Unreal Engine 5 este concepto equivale a "Light Component"
 * dentro de un "Light Actor". La separación Actor/Component permite reutilizar
 * el mismo sistema de iluminación para objetos dinámicos (ej. una antorcha que
 * se mueve) sin duplicar código.
 */
class LightComponent : public Component {
public:
    /** @brief Constructor: inicializa con tipo NONE (sin categoría ECS dedicada aún). */
    LightComponent() : Component(ComponentType::NONE) {}

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
