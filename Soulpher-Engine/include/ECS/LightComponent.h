#pragma once
#include "Prerequisites.h"
#include "ECS/Component.h"
#include "Rendering/RenderTypes.h"

class DeviceContext;

/**
 * @class LightComponent
 * @brief Componente ECS que describe una fuente de luz en la escena.
 *
 * Un Actor puede tener un LightComponent para participar en el pipeline
 * de iluminacion del ForwardRenderer. Si castShadow == true, el renderer
 * generara un shadow map desde la perspectiva de esta luz (directional only).
 */
class LightComponent : public Component {
public:
    LightComponent() : Component(ComponentType::NONE) {}

    void init()                            override {}
    void update(float /*deltaTime*/)       override {}
    void render(DeviceContext& /*dc*/)     override {}
    void destroy()                         override {}

    LightData&       getLightData()       { return m_light; }
    const LightData& getLightData() const { return m_light; }

    void setCastShadow(bool value) { m_castShadow = value; }
    bool canCastShadow() const     { return m_castShadow;  }

private:
    LightData m_light;
    bool      m_castShadow = false;
};
