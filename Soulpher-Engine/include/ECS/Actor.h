#pragma once
#include "Prerequisites.h"
#include "Entity.h"
#include "Buffer.h"
#include "Texture.h"
#include "Transform.h"
#include "SamplerState.h"
#include "Rasterizer.h"
#include "BlendState.h"
#include "ShaderProgram.h"
#include "DepthStencilState.h"
#include "Rendering/RenderTypes.h"

class Device;
class MeshComponent;

/**
 * @file Actor.h
 * @brief Entidad del sistema ECS que puede ser renderizada en escena.
 *
 * @details
 * Un `Actor` es un tipo especial de `Entity` que representa objetos 3D visibles en el motor.
 * Maneja:
 * - Transformaciones (posici�n, rotaci�n, escala).
 * - Mallas y texturas asociadas.
 * - Buffers de v�rtices e �ndices para el renderizado.
 * - Estados de renderizado y sombreado.
 * - Soporte para sombras (proyecci�n y recepci�n).
 *
 * @note Para estudiantes:
 * - Este patr�n ECS separa los datos (componentes) de la l�gica (sistemas).
 * - Cada `Actor` es un contenedor flexible de componentes que pueden cambiar en tiempo real.
 * - En juegos grandes, esto ayuda a optimizar memoria y permitir entidades con diferentes combinaciones de capacidades.
 */
class Actor : public Entity {
public:
    /** @brief Constructor por defecto. */
    Actor() = default;

    /**
     * @brief Constructor que inicializa el actor con un dispositivo DirectX.
     * @param device Dispositivo DirectX para inicializar recursos gr�ficos.
     *
     * @note Usar este constructor cuando el actor requiera recursos GPU desde su creaci�n.
     */
    Actor(Device& device);

    /** @brief Destructor virtual por defecto. */
    virtual ~Actor() = default;

    /**
     * @brief Activa o desactiva la recepci�n de sombras.
     * @param v `true` para recibir sombras, `false` para ignorarlas.
     *
     * @note Desactivar la recepci�n puede optimizar el render en objetos lejanos o poco relevantes.
     */
    void setReceiveShadow(bool v) { m_receiveShadow = v; }

    /**
     * @brief Consulta si el actor recibe sombras.
     * @return `true` si recibe sombras, `false` en caso contrario.
     */
    bool getReceiveShadow() const { return m_receiveShadow; }

    /** @brief Inicializa el actor. Sobrescribe la inicializaci�n base. */
    void init() override {}

    /**
     * @brief Actualiza la l�gica del actor.
     * @param deltaTime Tiempo en segundos desde la �ltima actualizaci�n.
     * @param deviceContext Contexto de dispositivo usado para operaciones gr�ficas.
     *
     * @note En un motor real, aqu� se podr�an actualizar animaciones, IA o f�sicas.
     */
    void update(float deltaTime, DeviceContext& deviceContext) override;

    /**
     * @brief Renderiza el actor en la escena.
     * @param deviceContext Contexto del dispositivo para enviar draw calls.
     *
     * @note Para estudiantes:
     * - Este m�todo es llamado t�picamente cada frame.
     * - Es un buen lugar para estudiar c�mo se preparan y env�an los datos a la GPU.
     */
    void render(DeviceContext& deviceContext) override;

    /**
     * @brief Libera los recursos asociados al actor.
     *
     * @warning Llamar antes de destruir el actor para evitar fugas de memoria GPU.
     */
    void destroy();

    /**
     * @brief Asigna las mallas al actor.
     * @param device Dispositivo de render para inicializar los buffers.
     * @param meshes Vector de componentes de malla.
     */
    void setMesh(Device& device, std::vector<MeshComponent> meshes);

    /** @brief Obtiene el nombre del actor (referencia mutable, permite edición desde UI). */
    std::string& getName() { return m_name; }

    /** @brief Asigna un nombre al actor. */
    void setName(const std::string& name) { m_name = name; }

    /**
     * @brief Asigna las texturas del actor.
     * @param textures Vector de texturas a usar en el renderizado.
     */
    void setTextures(std::vector<Texture> textures) { m_textures = textures; }

    /**
     * @brief Define si el actor proyecta sombras.
     * @param v `true` para proyectar sombras, `false` para no hacerlo.
     */
    void setCastShadow(bool v) { castShadow = v; }

    /**
     * @brief Consulta si el actor proyecta sombras.
     * @return `true` si proyecta sombras.
     */
    bool canCastShadow() const { return castShadow; }

    /**
     * @brief Renderiza sólo la geometría del actor para el shadow depth pass.
     * @param deviceContext Contexto del dispositivo.
     *
     * @note El ForwardRenderer llama a este método durante el shadow pass.
     *       CBPerFrame (b0) debe estar ya ligado antes de llamarlo.
     */
    void renderDepth(DeviceContext& deviceContext);

    /**
     * @brief Renderiza la sombra plana proyectada en el suelo (sombra fake legacy).
     * @param deviceContext Contexto del dispositivo.
     */
    void renderShadow(DeviceContext& deviceContext);

private:
    // === Geometr�a y texturas ===
    std::vector<MeshComponent> m_meshes;   ///< Mallas que componen el actor.
    std::vector<Texture> m_textures;       ///< Texturas aplicadas.
    std::vector<Buffer> m_vertexBuffers;   ///< Buffers de v�rtices (GPU).
    std::vector<Buffer> m_indexBuffers;    ///< Buffers de �ndices (GPU).

    // === Estados de renderizado ===
    BlendState m_blendstate;               ///< Estado de mezcla para transparencia/opacidad.
    Rasterizer m_rasterizer;               ///< Configuraci�n de rasterizaci�n (culling, fill mode).
    SamplerState m_sampler;                ///< Estado del muestreador de texturas.

    // === Constantes de modelo (b1 = CBPerObject, b2 = CBPerMaterial) ===
    CBPerObject   m_model;          ///< Matriz de mundo (actualizada cada frame).
    Buffer        m_modelBuffer;    ///< Constant buffer slot b1.
    CBPerMaterial m_materialCB;     ///< Parametros de material (color, PBR).
    Buffer        m_materialBuffer; ///< Constant buffer slot b2.

    // === Sombras ===
    ShaderProgram     m_shaderShadow;            ///< Pixel shader de sombra plana.
    Buffer            m_shaderBuffer;            ///< CB slot b1 para el pase de sombra.
    BlendState        m_shadowBlendState;
    DepthStencilState m_shadowDepthStencilState;
    CBPerObject       m_cbShadow;               ///< Mundo proyectado en el suelo.
    XMFLOAT4 m_LightPos;                    ///< Posici�n de la luz que proyecta las sombras.

    // === Metadatos ===
    std::string m_name = "Actor";          ///< Nombre identificador del actor.
    bool castShadow = true;                ///< Indica si el actor proyecta sombras.
    bool m_receiveShadow = true;           ///< Indica si el actor recibe sombras.
};
