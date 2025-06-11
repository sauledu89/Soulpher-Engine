#pragma once
#include "Prerequisites.h"

/**
 * @file Texture.h
 * @class Texture
 * @brief Representa una textura en DirectX 11.
 *
 * Esta clase encapsula el uso de una textura 2D, la cual puede ser cargada desde un archivo de imagen
 * (como DDS, PNG, JPG) o creada en tiempo de ejecución para utilizarse como render target o depth buffer.
 * Además, mantiene una ShaderResourceView que permite que la textura sea accedida por los shaders.
 */

 //--------------------------------------------------------------------------------------
 // Forward declarations para evitar dependencias circulares
 //--------------------------------------------------------------------------------------
class Device;
class DeviceContext;

/**
 * @class Texture
 * @brief Clase que encapsula el recurso de textura de Direct3D.
 *
 * El propósito principal de esta clase es simplificar el proceso de creación,
 * uso y destrucción de texturas 2D dentro del motor gráfico. Adicionalmente,
 * permite su vinculación directa al pipeline gráfico para uso en shaders.
 */
class Texture {
public:
    /**
     * @brief Constructor por defecto.
     */
    Texture() = default;

    /**
     * @brief Destructor por defecto.
     */
    ~Texture() = default;

    /**
     * @brief Carga una textura desde un archivo externo.
     *
     * Este método interpreta el tipo de imagen con base en su extensión y la convierte
     * en un recurso válido de DirectX 11 que puede ser muestreado por los shaders.
     *
     * @param device Dispositivo Direct3D responsable de crear el recurso.
     * @param textureName Ruta completa o nombre del archivo a cargar.
     * @param extensionType Tipo de extensión (DDS, PNG, JPG).
     * @return HRESULT Código de éxito o error.
     */
    HRESULT init(Device device,
        const std::string& textureName,
        ExtensionType extensionType);

    /**
     * @brief Crea una textura vacía para uso como render target, depth buffer u otros usos internos.
     *
     * Esta versión es útil cuando se necesita una textura dinámica, como en postprocesado o efectos especiales.
     *
     * @param device Dispositivo de renderizado.
     * @param width Ancho en píxeles.
     * @param height Alto en píxeles.
     * @param Format Formato de la textura (por ejemplo, DXGI_FORMAT_R8G8B8A8_UNORM).
     * @param BindFlags Banderas de uso: render target, shader resource, etc.
     * @param sampleCount Número de muestras para MSAA (por defecto 1).
     * @param qualityLevels Niveles de calidad para MSAA (por defecto 0).
     * @return HRESULT Código de éxito o error.
     */
    HRESULT init(Device device,
        unsigned int width,
        unsigned int height,
        DXGI_FORMAT Format,
        unsigned int BindFlags,
        unsigned int sampleCount = 1,
        unsigned int qualityLevels = 0);

    /**
     * @brief Crea una ShaderResourceView desde una textura existente.
     *
     * Este método permite reutilizar una textura creada previamente y generar una vista
     * de recurso que los shaders puedan leer.
     *
     * @param device Dispositivo Direct3D.
     * @param textureRef Referencia a la textura fuente.
     * @param format Formato deseado para la vista.
     * @return HRESULT Código de éxito o error.
     */
    HRESULT init(Device& device, Texture& textureRef, DXGI_FORMAT format);

    /**
     * @brief Método de actualización por cuadro. Placeholder no implementado.
     */
    void update();

    /**
     * @brief Enlaza la textura al pipeline para que pueda ser usada en los shaders.
     *
     * @param deviceContext Contexto de renderizado activo.
     * @param StartSlot Slot inicial en el que se vinculará la textura.
     * @param NumViews Número de vistas consecutivas a asignar.
     */
    void render(DeviceContext& deviceContext,
        unsigned int StartSlot,
        unsigned int NumViews);

    /**
     * @brief Libera todos los recursos de memoria asociados a esta textura.
     */
    void destroy();

public:
    //----------------------------------------------------------------------------------
    // Recursos públicos que representan la textura base y su vista asociada
    //----------------------------------------------------------------------------------

    /**
     * @brief Recurso de textura 2D creado con DirectX.
     *
     * Contiene los datos en crudo de la imagen o render target.
     */
    ID3D11Texture2D* m_texture = nullptr;

    /**
     * @brief Vista de la textura utilizada en shaders.
     *
     * Permite que el shader pueda acceder a esta textura como entrada.
     */
    ID3D11ShaderResourceView* m_textureFromImg = nullptr;
};
