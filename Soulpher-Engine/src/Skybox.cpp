/**
 * @file Skybox.cpp
 * @brief Implementación de Skybox: cubo unitario + panorámica equirectangular .dds.
 * @ingroup rendering
 */
#include "EngineUtilities/Utilities/Skybox.h"
#include "Device.h"
#include "DeviceContext.h"
#include "EngineUtilities/Utilities/Camera.h"
#include "EngineUtilities/Utilities/LayoutBuilder.h"

namespace {
/** @brief Slot de textura de la panorámica en el PS — t0-t3 (G-buffer) y t6 (shadow map) ya están en uso. */
constexpr unsigned int kPanoramaSlot = 7;

struct SkyboxVertex { XMFLOAT3 Pos; };

/** @brief Texture::init(DDS) agrega ".dds" internamente; hay que quitarlo antes de llamarlo. */
std::string StripDdsExtension(const std::string& path) {
    const std::string kExt = ".dds";
    if (path.size() >= kExt.size() &&
        path.compare(path.size() - kExt.size(), kExt.size(), kExt) == 0) {
        return path.substr(0, path.size() - kExt.size());
    }
    return path;
}
}

/**
 * @copydoc Skybox::init
 * @details Compila `Skybox.hlsl`, sube el cubo unitario (8 vértices, 36 índices) y carga
 * la panorámica vía `Texture::init(..., DDS)` — que usa un parser de DDS propio, NO
 * `D3DX11CreateShaderResourceViewFromFileA`.
 *
 * @note [GameDev] Ese detalle no es un capricho: `D3DX11CreateShaderResourceViewFromFileA`
 * (la utilidad de carga de texturas del DirectX SDK de 2010, que usa el resto de este
 * motor) provoca un acceso a memoria inválido al parsear archivos DDS con cabecera
 * extendida DX10 + formato BC7 — una combinación que no existía cuando esa SDK se
 * publicó (BC7 es de DirectX 11, DX10 header es más viejo pero poco usado en la época).
 * Es un ejemplo real de por qué los motores en producción migran su pipeline de carga
 * de assets lejos de utilidades "legacy" congeladas: el contenido moderno (texturas
 * comprimidas con BC7, generadas por herramientas actuales) puede dejar de ser
 * compatible con código de hace más de una década sin ningún aviso hasta que crashea.
 */
HRESULT Skybox::init(Device& device, const std::string& ddsPath) {
    LayoutBuilder layout;
    layout.Add("POSITION", DXGI_FORMAT_R32G32B32_FLOAT);

    HRESULT hr = m_shader.init(device, "Skybox.hlsl", layout);
    if (FAILED(hr)) {
        LOG_ERROR("Skybox", "init", "Failed to compile Skybox.hlsl");
        return hr;
    }

    hr = m_cbSkybox.init(device, sizeof(CBSkybox));
    if (FAILED(hr)) return hr;

    // Cubo unitario (-1..1), sin compartir vértices entre caras (no hace falta: sin normales/UV,
    // 8 vértices bastan). El winding coincide con el resto del motor (misma convención que las
    // cajas de GizmoRenderer) — no importa mucho cuál sea exactamente, porque el rasterizer
    // usa CULL_NONE (ver más abajo) y dibuja ambas caras sin importar su orientación.
    const SkyboxVertex verts[8] = {
        { XMFLOAT3(-1.0f, -1.0f, -1.0f) }, { XMFLOAT3( 1.0f, -1.0f, -1.0f) },
        { XMFLOAT3( 1.0f,  1.0f, -1.0f) }, { XMFLOAT3(-1.0f,  1.0f, -1.0f) },
        { XMFLOAT3(-1.0f, -1.0f,  1.0f) }, { XMFLOAT3( 1.0f, -1.0f,  1.0f) },
        { XMFLOAT3( 1.0f,  1.0f,  1.0f) }, { XMFLOAT3(-1.0f,  1.0f,  1.0f) },
    };
    const unsigned int indices[36] = {
        0, 1, 2,  0, 2, 3, // -Z
        5, 4, 7,  5, 7, 6, // +Z
        4, 0, 3,  4, 3, 7, // -X
        1, 5, 6,  1, 6, 2, // +X
        3, 2, 6,  3, 6, 7, // +Y
        4, 5, 1,  4, 1, 0, // -Y
    };
    m_cubeIndexCount = 36;

    hr = m_cubeVB.init(device, verts, 8, sizeof(SkyboxVertex), D3D11_BIND_VERTEX_BUFFER);
    if (FAILED(hr)) return hr;

    hr = m_cubeIB.init(device, indices, 36, sizeof(unsigned int), D3D11_BIND_INDEX_BUFFER);
    if (FAILED(hr)) return hr;

    // El .dds disponible es una panorámica equirectangular 2:1 (no un cubemap de 6 caras),
    // así que se carga como Texture2D normal — el PS hace la proyección esférica.
    hr = m_panorama.init(device, StripDdsExtension(ddsPath), DDS);
    if (FAILED(hr)) {
        LOG_ERROR("Skybox", "init", ("Failed to load skybox panorama: " + ddsPath).c_str());
        return hr;
    }

    // La cámara está siempre DENTRO del cubo. CULL_NONE evita depender de acertar a mano la
    // direccion exacta del winding en left-handed (con CULL_FRONT, si el winding no es el
    // esperado, TODAS las caras quedan culled y el skybox desaparece por completo) — para un
    // cubo cerrado y convexo con la camara siempre adentro no hay overdraw real que evitar.
    hr = m_rasterizer.init(device, D3D11_FILL_SOLID, D3D11_CULL_NONE, false, true);
    if (FAILED(hr)) return hr;

    // Depth enable + LESS_EQUAL + write OFF: solo visible donde no hay geometría más cercana,
    // y nunca sobreescribe el depth del geometry pass.
    hr = m_depthStencil.init(device, true, D3D11_DEPTH_WRITE_MASK_ZERO, D3D11_COMPARISON_LESS_EQUAL);
    if (FAILED(hr)) return hr;

    hr = m_sampler.init(device);
    if (FAILED(hr)) return hr;

    m_initialized = true;
    LOG_MESSAGE("Skybox", "init", "OK (" + ddsPath + ")");
    return S_OK;
}

/**
 * @copydoc Skybox::render
 * @details El vertex shader (`Skybox.hlsl`) fuerza `output.pos = clipPos.xyww` — al
 * dividir por `w` en la etapa de rasterización, esto da `NDC.z = w/w = 1.0` siempre,
 * es decir, la profundidad máxima posible. Combinado con `DepthStencilState` en
 * `LESS_EQUAL`, cualquier píxel que ya tenga un objeto opaco más cerca (profundidad
 * < 1.0) gana automáticamente; el cubo del skybox solo "gana" el test de profundidad
 * donde no hay nada más — sin necesitar ningún cálculo de "¿hay geometría aquí?" en
 * el pixel shader.
 */
void Skybox::render(DeviceContext& deviceContext, const Camera& camera) {
    if (!m_initialized) return;

    m_shader.render(deviceContext);
    m_rasterizer.render(deviceContext);
    m_depthStencil.render(deviceContext, 0, false);

    // Vista sin traslación: el cubo nunca se aleja de la cámara, solo rota con ella.
    XMFLOAT4X4 viewNoTranslationF = camera.GetViewNoTranslation();
    XMMATRIX viewProj = XMLoadFloat4x4(&viewNoTranslationF) * camera.getProj();

    CBSkybox cb{};
    XMStoreFloat4x4(&cb.ViewProj, XMMatrixTranspose(viewProj));
    m_cbSkybox.update(deviceContext, nullptr, 0, nullptr, &cb, 0, 0);
    m_cbSkybox.render(deviceContext, 0, 1, false);

    deviceContext.PSSetShaderResources(kPanoramaSlot, 1, &m_panorama.m_textureFromImg);
    m_sampler.render(deviceContext, 0, 1);

    deviceContext.IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_cubeVB.render(deviceContext, 0, 1);
    m_cubeIB.render(deviceContext, 0, 1, false, DXGI_FORMAT_R32_UINT);
    deviceContext.DrawIndexed(m_cubeIndexCount, 0, 0);
}

void Skybox::destroy() {
    m_panorama.destroy();
    m_sampler.destroy();
    m_depthStencil.destroy();
    m_rasterizer.destroy();
    m_cubeIB.destroy();
    m_cubeVB.destroy();
    m_cbSkybox.destroy();
    m_shader.destroy();
    m_initialized = false;
}
