#pragma once
#include "Prerequisites.h"

/**
 * @file Texture.h
 * @class Texture
 * @brief Representa una textura en DirectX 11.
 *
 * Esta clase encapsula el uso de una textura 2D, la cual puede ser cargada desde un archivo de imagen
 * (como DDS, PNG, JPG) o creada en tiempo de ejecuci�n para utilizarse como render target o depth buffer.
 * Adem�s, mantiene una ShaderResourceView que permite que la textura sea accedida por los shaders.
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
 * El prop�sito principal de esta clase es simplificar el proceso de creaci�n,
 * uso y destrucci�n de texturas 2D dentro del motor gr�fico. Adicionalmente,
 * permite su vinculaci�n directa al pipeline gr�fico para uso en shaders.
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
     * Este m�todo interpreta el tipo de imagen con base en su extensi�n y la convierte
     * en un recurso v�lido de DirectX 11 que puede ser muestreado por los shaders.
     *
     * @param device Dispositivo Direct3D responsable de crear el recurso.
     * @param textureName Ruta completa o nombre del archivo a cargar.
     * @param extensionType Tipo de extensi�n (DDS, PNG, JPG).
     * @return HRESULT C�digo de �xito o error.
     */
    HRESULT init(Device device,
        const std::string& textureName,
        ExtensionType extensionType);

    /**
     * @brief Crea una textura vac�a para uso como render target, depth buffer u otros usos internos.
     *
     * Esta versi�n es �til cuando se necesita una textura din�mica, como en postprocesado o efectos especiales.
     *
     * @param device Dispositivo de renderizado.
     * @param width Ancho en p�xeles.
     * @param height Alto en p�xeles.
     * @param Format Formato de la textura (por ejemplo, DXGI_FORMAT_R8G8B8A8_UNORM).
     * @param BindFlags Banderas de uso: render target, shader resource, etc.
     * @param sampleCount N�mero de muestras para MSAA (por defecto 1).
     * @param qualityLevels Niveles de calidad para MSAA (por defecto 0).
     * @return HRESULT C�digo de �xito o error.
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
     * Este m�todo permite reutilizar una textura creada previamente y generar una vista
     * de recurso que los shaders puedan leer.
     *
     * @param device Dispositivo Direct3D.
     * @param textureRef Referencia a la textura fuente.
     * @param format Formato deseado para la vista.
     * @return HRESULT C�digo de �xito o error.
     */
    HRESULT init(Device& device, Texture& textureRef, DXGI_FORMAT format);

    /**
     * @brief M�todo de actualizaci�n por cuadro. Placeholder no implementado.
     */
    void update();

    /**
     * @brief Enlaza la textura al pipeline para que pueda ser usada en los shaders.
     *
     * @param deviceContext Contexto de renderizado activo.
     * @param StartSlot Slot inicial en el que se vincular� la textura.
     * @param NumViews N�mero de vistas consecutivas a asignar.
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
    // Recursos p�blicos que representan la textura base y su vista asociada
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
