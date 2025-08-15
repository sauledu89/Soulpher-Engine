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

class Device;
class MeshComponent;

/**
 * @file Actor.h
 * @brief Entidad del sistema ECS que puede ser renderizada en escena.
 *
 * @details
 * Un `Actor` es un tipo especial de `Entity` que representa objetos 3D visibles en el motor.
 * Maneja:
 * - Transformaciones (posición, rotación, escala).
 * - Mallas y texturas asociadas.
 * - Buffers de vértices e índices para el renderizado.
 * - Estados de renderizado y sombreado.
 * - Soporte para sombras (proyección y recepción).
 *
 * @note Para estudiantes:
 * - Este patrón ECS separa los datos (componentes) de la lógica (sistemas).
 * - Cada `Actor` es un contenedor flexible de componentes que pueden cambiar en tiempo real.
 * - En juegos grandes, esto ayuda a optimizar memoria y permitir entidades con diferentes combinaciones de capacidades.
 */
class Actor : public Entity {
public:
    /** @brief Constructor por defecto. */
    Actor() = default;

    /**
     * @brief Constructor que inicializa el actor con un dispositivo DirectX.
     * @param device Dispositivo DirectX para inicializar recursos gráficos.
     *
     * @note Usar este constructor cuando el actor requiera recursos GPU desde su creación.
     */
    Actor(Device& device);

    /** @brief Destructor virtual por defecto. */
    virtual ~Actor() = default;

    /**
     * @brief Activa o desactiva la recepción de sombras.
     * @param v `true` para recibir sombras, `false` para ignorarlas.
     *
     * @note Desactivar la recepción puede optimizar el render en objetos lejanos o poco relevantes.
     */
    void setReceiveShadow(bool v) { m_receiveShadow = v; }

    /**
     * @brief Consulta si el actor recibe sombras.
     * @return `true` si recibe sombras, `false` en caso contrario.
     */
    bool getReceiveShadow() const { return m_receiveShadow; }

    /** @brief Inicializa el actor. Sobrescribe la inicialización base. */
    void init() override {}

    /**
     * @brief Actualiza la lógica del actor.
     * @param deltaTime Tiempo en segundos desde la última actualización.
     * @param deviceContext Contexto de dispositivo usado para operaciones gráficas.
     *
     * @note En un motor real, aquí se podrían actualizar animaciones, IA o físicas.
     */
    void update(float deltaTime, DeviceContext& deviceContext) override;

    /**
     * @brief Renderiza el actor en la escena.
     * @param deviceContext Contexto del dispositivo para enviar draw calls.
     *
     * @note Para estudiantes:
     * - Este método es llamado típicamente cada frame.
     * - Es un buen lugar para estudiar cómo se preparan y envían los datos a la GPU.
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

    /** @brief Obtiene el nombre del actor. */
    std::string getName() { return m_name; }

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
     * @brief Renderiza la sombra del actor (depth pass).
     * @param deviceContext Contexto del dispositivo.
     *
     * @note Suele usarse en un pase previo para mapas de sombras.
     */
    void renderShadow(DeviceContext& deviceContext);

private:
    // === Geometría y texturas ===
    std::vector<MeshComponent> m_meshes;   ///< Mallas que componen el actor.
    std::vector<Texture> m_textures;       ///< Texturas aplicadas.
    std::vector<Buffer> m_vertexBuffers;   ///< Buffers de vértices (GPU).
    std::vector<Buffer> m_indexBuffers;    ///< Buffers de índices (GPU).

    // === Estados de renderizado ===
    BlendState m_blendstate;               ///< Estado de mezcla para transparencia/opacidad.
    Rasterizer m_rasterizer;               ///< Configuración de rasterización (culling, fill mode).
    SamplerState m_sampler;                ///< Estado del muestreador de texturas.

    // === Constantes de modelo ===
    CBChangesEveryFrame m_model;           ///< Constantes que cambian cada frame (transformaciones).
    Buffer m_modelBuffer;                  ///< Buffer de constantes para el modelo.

    // === Sombras ===
    ShaderProgram m_shaderShadow;          ///< Shader para renderizado de sombras.
    Buffer m_shaderBuffer;                 ///< Buffer asociado al shader de sombras.
    BlendState m_shadowBlendState;         ///< Estado de mezcla usado en el pase de sombras.
    DepthStencilState m_shadowDepthStencilState; ///< Estado de profundidad para sombras.
    CBChangesEveryFrame m_cbShadow;        ///< Buffer de constantes para sombras.
    XMFLOAT4 m_LightPos;                    ///< Posición de la luz que proyecta las sombras.

    // === Metadatos ===
    std::string m_name = "Actor";          ///< Nombre identificador del actor.
    bool castShadow = true;                ///< Indica si el actor proyecta sombras.
    bool m_receiveShadow = true;           ///< Indica si el actor recibe sombras.
};
