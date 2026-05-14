#pragma once
#include "Prerequisites.h"
#include "Rendering/RenderTypes.h"

class ShaderProgram;
class Rasterizer;
class DepthStencilState;
class SamplerState;

/**
 * @class Material
 * @brief Estado fijo de render compartido entre instancias de material.
 *
 * Un Material apunta a los recursos GPU que son identicos para todos los
 * objetos que usen el mismo tipo de superficie: shader, rasterizer,
 * depth-stencil, sampler, dominio y modo de mezcla.
 *
 * Los valores concretos por objeto (texturas, parametros PBR) viven en
 * MaterialInstance, que referencia a este Material.
 */
class Material {
public:
    void setShader            (ShaderProgram*    shader) { m_shader            = shader; }
    void setRasterizer        (Rasterizer*        state) { m_rasterizer        = state;  }
    void setDepthStencilState (DepthStencilState* state) { m_depthStencilState = state;  }
    void setSamplerState      (SamplerState*      state) { m_samplerState      = state;  }
    void setDomain            (MaterialDomain    domain) { m_domain            = domain; }
    void setBlendMode         (BlendMode           mode) { m_blendMode         = mode;   }

    ShaderProgram*    getShader()            const { return m_shader;            }
    Rasterizer*       getRasterizer()        const { return m_rasterizer;        }
    DepthStencilState* getDepthStencilState() const { return m_depthStencilState; }
    SamplerState*     getSamplerState()      const { return m_samplerState;      }
    MaterialDomain    getDomain()            const { return m_domain;            }
    BlendMode         getBlendMode()         const { return m_blendMode;         }

private:
    ShaderProgram*    m_shader            = nullptr;
    Rasterizer*       m_rasterizer        = nullptr;
    DepthStencilState* m_depthStencilState = nullptr;
    SamplerState*     m_samplerState      = nullptr;
    MaterialDomain    m_domain            = MaterialDomain::Opaque;
    BlendMode         m_blendMode         = BlendMode::Opaque;
};
