#include "Rendering/MaterialInstance.h"
#include "DeviceContext.h"
#include "Texture.h"

void
MaterialInstance::bindTextures(DeviceContext& deviceContext) const {
    if (!deviceContext.m_deviceContext) {
        return;
    }

    // Cada slot se enlaza de forma independiente para no bloquear slots
    // cuya textura no existe en esta instancia.
    auto bind = [&](Texture* tex, unsigned int slot) {
        ID3D11ShaderResourceView* srv = tex ? tex->srv() : nullptr;
        if (srv) {
            deviceContext.m_deviceContext->PSSetShaderResources(slot, 1, &srv);
        }
    };

    bind(m_albedo,    0);
    bind(m_normal,    1);
    bind(m_metallic,  2);
    bind(m_roughness, 3);
    bind(m_ao,        4);
    bind(m_emissive,  5);
    // Slot 6 es reservado para el shadow map; lo asigna ForwardRenderer.
}
