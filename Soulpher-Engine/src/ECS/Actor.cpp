/**
 * @file Actor.cpp
 * @brief Implementación de la clase Actor para el sistema ECS.
 *
 * @details
 * Un Actor es una entidad del motor que contiene componentes y lógica
 * de renderizado y actualización. Este archivo implementa:
 * - Inicialización de recursos gráficos (buffers, estados, shaders).
 * - Actualización de transformaciones y datos de render por frame.
 * - Renderizado del modelo y de su sombra proyectada.
 * - Destrucción y liberación de recursos.
 *
 * ---
 * 💡 **Notas para estudiantes de desarrollo de videojuegos**:
 * - El patrón **ECS (Entity Component System)** separa los datos (componentes) de la lógica.
 * - Aquí, `Transform` define posición, rotación y escala.
 * - `MeshComponent` contiene la geometría a renderizar.
 * - El uso de **constant buffers** permite enviar datos de CPU → GPU cada frame.
 * - El render de sombras se realiza proyectando la geometría en el plano del suelo mediante una matriz de proyección de sombra.
 */

#include "ECS/Actor.h"
#include "MeshComponent.h"
#include "Device.h"
#include "DeviceContext.h"

 /**
  * @brief Constructor de Actor.
  * @param device Referencia al dispositivo de render (DirectX 11).
  *
  * @details
  * - Crea componentes básicos por defecto (`Transform` y `MeshComponent`).
  * - Inicializa buffers y estados gráficos necesarios para renderizar.
  * - Carga el shader para sombras.
  */
Actor::Actor(Device& device) {
    // Crear componentes por defecto
    EU::TSharedPointer<Transform> transform = EU::MakeShared<Transform>();
    addComponent(transform);
    EU::TSharedPointer<MeshComponent> meshComponent = EU::MakeShared<MeshComponent>();
    addComponent(meshComponent);

    HRESULT hr;
    std::string classNameType = "Actor -> " + m_name;

    // Buffer de modelo (mundo + color)
    hr = m_modelBuffer.init(device, sizeof(CBChangesEveryFrame));
    if (FAILED(hr)) {
        ERROR("Actor", classNameType.c_str(), "Failed to create new CBChangesEveryFrame");
    }

    // Estados gráficos
    hr = m_sampler.init(device);
    if (FAILED(hr)) { ERROR("Actor", classNameType.c_str(), "Failed to create new SamplerState"); }

    hr = m_rasterizer.init(device);
    if (FAILED(hr)) { ERROR("Actor", classNameType.c_str(), "Failed to create new Rasterizer"); }

    hr = m_blendstate.init(device);
    if (FAILED(hr)) { ERROR("Actor", classNameType.c_str(), "Failed to create new BlendState"); }

    // Shader para sombras
    hr = m_shaderShadow.CreateShader(device, PIXEL_SHADER, "HybridEngine.fx");
    if (FAILED(hr)) {
        ERROR("Main", "InitDevice",
            ("Failed to initialize Shadow Shader. HRESULT: " + std::to_string(hr)).c_str());
    }

    // Buffers y estados para sombras
    hr = m_shaderBuffer.init(device, sizeof(CBChangesEveryFrame));
    if (FAILED(hr)) {
        ERROR("Main", "InitDevice",
            ("Failed to initialize Shadow Buffer. HRESULT: " + std::to_string(hr)).c_str());
    }

    hr = m_shadowBlendState.init(device);
    if (FAILED(hr)) {
        ERROR("Main", "InitDevice",
            ("Failed to initialize Shadow Blend State. HRESULT: " + std::to_string(hr)).c_str());
    }

    hr = m_shadowDepthStencilState.init(device, true, false);
    if (FAILED(hr)) {
        ERROR("Main", "InitDevice",
            ("Failed to initialize Depth Stencil State. HRESULT: " + std::to_string(hr)).c_str());
    }

    // Posición de la luz para proyección de sombras
    m_LightPos = XMFLOAT4(2.0f, 4.0f, -2.0f, 1.0f);
}

/**
 * @brief Actualiza el actor y sus componentes.
 * @param deltaTime Tiempo transcurrido desde el último frame.
 * @param deviceContext Contexto de dispositivo para enviar datos a la GPU.
 *
 * @details
 * - Llama a `update()` de cada componente.
 * - Actualiza el constant buffer del modelo con la matriz de mundo y el color.
 */
void Actor::update(float deltaTime, DeviceContext& deviceContext) {
    for (auto& component : m_components) {
        if (component) { component->update(deltaTime); }
    }

    m_model.mWorld = XMMatrixTranspose(getComponent<Transform>()->matrix);
    m_model.vMeshColor = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);

    m_modelBuffer.update(deviceContext, nullptr, 0, nullptr, &m_model, 0, 0);
}

/**
 * @brief Renderiza el actor y su sombra (si aplica).
 */
void Actor::render(DeviceContext& deviceContext) {
    if (canCastShadow()) { renderShadow(deviceContext); }

    m_blendstate.render(deviceContext);
    m_rasterizer.render(deviceContext);
    m_sampler.render(deviceContext, 0, 1);

    deviceContext.IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    for (unsigned int i = 0; i < m_meshes.size(); i++) {
        m_vertexBuffers[i].render(deviceContext, 0, 1);
        m_indexBuffers[i].render(deviceContext, 0, 1, false, DXGI_FORMAT_R32_UINT);

        m_modelBuffer.render(deviceContext, 2, 1, true);

        if (m_textures.size() > 0 && i < m_textures.size()) {
            m_textures[i].render(deviceContext, 0, 1);
        }

        deviceContext.DrawIndexed(m_meshes[i].m_numIndex, 0, 0);
    }
}

/**
 * @brief Libera recursos gráficos asociados al actor.
 */
void Actor::destroy() {
    for (auto& vb : m_vertexBuffers) { vb.destroy(); }
    for (auto& ib : m_indexBuffers) { ib.destroy(); }
    for (auto& tex : m_textures) { tex.destroy(); }

    m_modelBuffer.destroy();
    m_rasterizer.destroy();
    m_blendstate.destroy();
    m_sampler.destroy();
}

/**
 * @brief Configura la geometría del actor desde un conjunto de mallas.
 */
void Actor::setMesh(Device& device, std::vector<MeshComponent> meshes) {
    m_meshes = meshes;
    HRESULT hr;
    for (auto& mesh : m_meshes) {
        Buffer vb;
        hr = vb.init(device, mesh, D3D11_BIND_VERTEX_BUFFER);
        if (FAILED(hr)) { ERROR("Actor", "setMesh", "Failed to create new vertexBuffer"); }
        else { m_vertexBuffers.push_back(vb); }

        Buffer ib;
        hr = ib.init(device, mesh, D3D11_BIND_INDEX_BUFFER);
        if (FAILED(hr)) { ERROR("Actor", "setMesh", "Failed to create new indexBuffer"); }
        else { m_indexBuffers.push_back(ib); }
    }
}

/**
 * @brief Renderiza la sombra proyectada del actor en el suelo.
 *
 * @details
 * - Calcula una matriz de proyección de sombra usando la posición de la luz.
 * - Actualiza el constant buffer específico para sombras.
 * - Usa un pixel shader especial (`m_shaderShadow`) que dibuja en negro semitransparente.
 */
void Actor::renderShadow(DeviceContext& deviceContext) {
    auto t = getComponent<Transform>();
    auto pos = t->getPosition();
    auto yaw = t->getRotation().y;
    auto scl = t->getScale();

    XMMATRIX Mscale = XMMatrixScaling(scl.x, scl.y, scl.z);
    XMMATRIX Myaw = XMMatrixRotationY(yaw);
    XMMATRIX Mtrans = XMMatrixTranslation(pos.x, pos.y, pos.z);
    XMMATRIX worldYaw = Mscale * Myaw * Mtrans;

    float Lx = m_LightPos.x, Ly = m_LightPos.y, Lz = m_LightPos.z;
    float invLy = 1.0f / Ly;

    XMMATRIX S = XMMATRIX(
        1.0f, -Lx * invLy, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, -Lz * invLy, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    );

    XMMATRIX worldShadow = worldYaw * S;

    m_cbShadow.mWorld = XMMatrixTranspose(worldShadow);
    m_cbShadow.vMeshColor = XMFLOAT4(0, 0, 0, 0.5f);
    m_shaderBuffer.update(deviceContext, nullptr, 0, nullptr, &m_cbShadow, 0, 0);
    m_shaderBuffer.render(deviceContext, 2, 1, true);

    float blendFactor[4] = { 0.f, 0.f, 0.f, 0.f };
    m_shaderShadow.render(deviceContext, PIXEL_SHADER);
    m_shadowBlendState.render(deviceContext, blendFactor, 0xffffffff);
    m_shadowDepthStencilState.render(deviceContext, 0);

    deviceContext.IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    for (size_t i = 0; i < m_meshes.size(); ++i) {
        m_vertexBuffers[i].render(deviceContext, 0, 1);
        m_indexBuffers[i].render(deviceContext, 0, 1, false, DXGI_FORMAT_R32_UINT);
        deviceContext.DrawIndexed(m_meshes[i].m_numIndex, 0, 0);
    }
}
