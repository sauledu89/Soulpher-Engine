#pragma once
#include "Prerequisites.h"
#include "Rendering/RenderTypes.h"

class Material;
class DeviceContext;
class Texture;

/**
 * @class MaterialInstance
 * @brief Instancia concreta de un Material con texturas y parametros PBR propios.
 *
 * Permite reutilizar un mismo Material (shader + estados) con distintas
 * combinaciones de mapas de textura y valores PBR por objeto.
 *
 * Slots de textura (deben coincidir con los registros t# del pixel shader):
 *   t0 = Albedo
 *   t1 = Normal
 *   t2 = Metallic
 *   t3 = Roughness
 *   t4 = AO
 *   t5 = Emissive
 *   t6 = Shadow map  (lo asigna ForwardRenderer, no esta clase)
 */
class MaterialInstance {
public:
    void setMaterial  (Material* material)  { m_material  = material;  }
    void setAlbedo    (Texture*  texture)   { m_albedo    = texture;   }
    void setNormal    (Texture*  texture)   { m_normal    = texture;   }
    void setMetallic  (Texture*  texture)   { m_metallic  = texture;   }
    void setRoughness (Texture*  texture)   { m_roughness = texture;   }
    void setAO        (Texture*  texture)   { m_ao        = texture;   }
    void setEmissive  (Texture*  texture)   { m_emissive  = texture;   }

    Material* getMaterial()  const { return m_material;  }
    Texture*  getAlbedo()    const { return m_albedo;    }
    Texture*  getNormal()    const { return m_normal;    }
    Texture*  getMetallic()  const { return m_metallic;  }
    Texture*  getRoughness() const { return m_roughness; }
    Texture*  getAO()        const { return m_ao;        }
    Texture*  getEmissive()  const { return m_emissive;  }

    MaterialParams&       getParams()       { return m_params; }
    const MaterialParams& getParams() const { return m_params; }

    /**
     * @brief Enlaza todas las texturas no nulas en el Pixel Shader.
     * @param deviceContext Contexto de dispositivo activo.
     */
    void bindTextures(DeviceContext& deviceContext) const;

private:
    Material*      m_material  = nullptr;
    Texture*       m_albedo    = nullptr;
    Texture*       m_normal    = nullptr;
    Texture*       m_metallic  = nullptr;
    Texture*       m_roughness = nullptr;
    Texture*       m_ao        = nullptr;
    Texture*       m_emissive  = nullptr;
    MaterialParams m_params;
};
