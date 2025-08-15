#pragma once
#include "Prerequisites.h"

class Device;
class DeviceContext;

/**
 * @class SamplerState
 * @brief Administra el estado de muestreo de texturas en Direct3D 11.
 *
 * @details
 * Un **Sampler State** define cómo un shader accede a las texturas en GPU,
 * controlando aspectos como:
 * - Tipo de filtrado (point, bilinear, trilinear, anisotrópico).
 * - Dirección de muestreo (wrap, clamp, mirror).
 * - Comparaciones y bias para sampling avanzado.
 *
 * Esta clase encapsula:
 * - Creación de un `ID3D11SamplerState` con parámetros de muestreo.
 * - Asignación del muestreador a un slot específico del shader.
 * - Liberación de recursos.
 *
 * @note En motores de videojuegos, cambiar el `SamplerState` puede afectar
 * drásticamente la calidad visual de texturas, especialmente en entornos 3D.
 */
class SamplerState {
public:
    SamplerState() = default;  ///< Constructor por defecto.
    ~SamplerState() = default; ///< Destructor por defecto.

    /**
     * @brief Inicializa el estado de muestreo con configuración por defecto.
     * @param device Dispositivo Direct3D donde se creará el estado.
     * @return HRESULT que indica éxito o error de la operación.
     *
     * @note
     * Un muestreador por defecto suele usar filtrado bilinear
     * y `D3D11_TEXTURE_ADDRESS_WRAP`.
     */
    HRESULT init(Device& device);

    /**
     * @brief Actualiza parámetros internos si es necesario.
     *
     * @note
     * Generalmente no se usa con frecuencia, pero puede ser útil para
     * cambiar filtrado o dirección de muestreo en tiempo real.
     */
    void update();

    /**
     * @brief Aplica el estado de muestreo al contexto de dispositivo.
     * @param deviceContext Contexto de dispositivo donde aplicar el estado.
     * @param StartSlot Primer slot de muestreador en el shader.
     * @param NumSamplers Número de muestreadores a asignar.
     *
     * @note
     * Este método debe llamarse antes de dibujar, para que el shader
     * use el muestreador configurado.
     */
    void render(DeviceContext& deviceContext,
        unsigned int StartSlot,
        unsigned int NumSamplers);

    /**
     * @brief Libera los recursos asociados.
     *
     * @warning
     * No liberar este recurso causará fugas de memoria GPU.
     */
    void destroy();

public:
    ID3D11SamplerState* m_sampler = nullptr; ///< Estado de muestreo de Direct3D 11.
};
