/**
 * @file Texture.h
 * @brief Encapsula la creación, gestión y uso de texturas en Direct3D 11.
 *
 * @details
 * Una **textura** en un motor gráfico es un mapa de bits o imagen que se aplica a modelos 3D
 * o superficies 2D para darles detalle visual. En Direct3D 11, se representan con
 * **ID3D11Texture2D** y pueden tener vistas asociadas como:
 * - **SRV (Shader Resource View)** → para leer en shaders.
 * - **RTV (Render Target View)** → para dibujar sobre ellas.
 * - **DSV (Depth Stencil View)** → para usarlas como buffers de profundidad.
 *
 * 🔹 **Conceptos clave para estudiantes**:
 * - Las texturas pueden venir de un archivo (PNG, JPG, DDS...).
 * - También pueden crearse vacías para usarse como objetivos de render o buffers auxiliares.
 * - Se pueden inicializar como "alias" de otra textura ya existente (ej. back-buffer).
 */

#pragma once
#include "Prerequisites.h"

class Device;
class DeviceContext;

/**
 * @class Texture
 * @brief Administra texturas Direct3D 11 (desde archivo, vacías o alias).
 *
 * @note Parte fundamental en el renderizado: sin texturas, el resultado
 * sería solo formas sólidas con colores planos.
 */
class Texture {
public:
    /** @brief Constructor por defecto. */
    Texture() = default;

    /** @brief Destructor por defecto. */
    ~Texture() = default;

    /**
     * @brief Inicializa la textura cargándola desde un archivo de imagen.
     * @param device Dispositivo Direct3D usado para la creación.
     * @param textureName Ruta del archivo de textura.
     * @param extensionType Tipo de extensión de la imagen (para elegir el método de carga).
     * @return HRESULT que indica éxito o error de la operación.
     *
     * @note Ideal para cargar texturas difusas, normales o especulares desde disco.
     */
    HRESULT init(Device device,
        const std::string& textureName,
        ExtensionType extensionType);

    /**
     * @brief Crea una textura 2D vacía (generalmente para render targets o depth buffers).
     * @param device Dispositivo Direct3D usado para la creación.
     * @param width Ancho de la textura en píxeles.
     * @param height Alto de la textura en píxeles.
     * @param Format Formato de píxeles (ej. DXGI_FORMAT_R8G8B8A8_UNORM).
     * @param BindFlags Banderas de uso (RTV, DSV, SRV, etc.).
     * @param sampleCount Número de muestras para MSAA (por defecto 1).
     * @param qualityLevels Niveles de calidad para MSAA (por defecto 0).
     * @return HRESULT que indica éxito o error de la operación.
     *
     * @note Este método es muy usado para crear **framebuffers personalizados**.
     */
    HRESULT init(Device device,
        unsigned int width,
        unsigned int height,
        DXGI_FORMAT Format,
        unsigned int BindFlags,
        unsigned int sampleCount = 1,
        unsigned int qualityLevels = 0);

    /**
     * @brief Inicializa la textura como alias de otra textura ya existente.
     * @param device Dispositivo Direct3D.
     * @param textureRef Textura de referencia.
     * @param format Formato de la vista.
     * @return HRESULT que indica éxito o error de la operación.
     *
     * @note Útil para acceder a recursos como el back-buffer del swap chain.
     */
    HRESULT init(Device& device, Texture& textureRef, DXGI_FORMAT format);

    /**
     * @brief Adjunta un recurso nativo de textura existente.
     * @param tex Puntero a la textura ID3D11Texture2D.
     *
     * @note Usado típicamente para enlazar el back-buffer.
     */
    void attach(ID3D11Texture2D* tex) { SAFE_RELEASE(m_texture); m_texture = tex; }

    /// @return Puntero nativo a la textura 2D.
    ID3D11Texture2D* raw() const { return m_texture; }

    /// @return Vista SRV (Shader Resource View) si la textura fue creada para ser leída en shaders.
    ID3D11ShaderResourceView* srv() const { return m_textureFromImg; }

    /** @brief Actualiza parámetros internos si es necesario. */
    void update();

    /**
     * @brief Enlaza la textura como SRV al Pixel Shader.
     * @param deviceContext Contexto Direct3D.
     * @param StartSlot Primer slot donde se asignará.
     * @param NumViews Número de vistas a enlazar.
     *
     * @note Es el paso final para que un shader pueda usar esta textura.
     */
    void render(DeviceContext& deviceContext, unsigned int StartSlot, unsigned int NumViews);

    /** @brief Libera todos los recursos asociados a la textura. */
    void destroy();

public:
    ID3D11Texture2D* m_texture = nullptr;                 ///< Recurso de textura 2D.
    ID3D11ShaderResourceView* m_textureFromImg = nullptr; ///< Vista SRV asociada (si aplica).
    std::string m_textureName;                            ///< Nombre o ruta del archivo original.
};
