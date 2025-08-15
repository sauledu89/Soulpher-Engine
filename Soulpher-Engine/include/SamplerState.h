#pragma once
#include "Prerequisites.h"

class Device;
class DeviceContext;

/**
 * @class SamplerState
 * @brief Administra el estado de muestreo de texturas en Direct3D 11.
 *
 * @details
 * Un **Sampler State** define c�mo un shader accede a las texturas en GPU,
 * controlando aspectos como:
 * - Tipo de filtrado (point, bilinear, trilinear, anisotr�pico).
 * - Direcci�n de muestreo (wrap, clamp, mirror).
 * - Comparaciones y bias para sampling avanzado.
 *
 * Esta clase encapsula:
 * - Creaci�n de un `ID3D11SamplerState` con par�metros de muestreo.
 * - Asignaci�n del muestreador a un slot espec�fico del shader.
 * - Liberaci�n de recursos.
 *
 * @note En motores de videojuegos, cambiar el `SamplerState` puede afectar
 * dr�sticamente la calidad visual de texturas, especialmente en entornos 3D.
 */
class SamplerState {
public:
    SamplerState() = default;  ///< Constructor por defecto.
    ~SamplerState() = default; ///< Destructor por defecto.

    /**
     * @brief Inicializa el estado de muestreo con configuraci�n por defecto.
     * @param device Dispositivo Direct3D donde se crear� el estado.
     * @return HRESULT que indica �xito o error de la operaci�n.
     *
     * @note
     * Un muestreador por defecto suele usar filtrado bilinear
     * y `D3D11_TEXTURE_ADDRESS_WRAP`.
     */
    HRESULT init(Device& device);

    /**
     * @brief Actualiza par�metros internos si es necesario.
     *
     * @note
     * Generalmente no se usa con frecuencia, pero puede ser �til para
     * cambiar filtrado o direcci�n de muestreo en tiempo real.
     */
    void update();

    /**
     * @brief Aplica el estado de muestreo al contexto de dispositivo.
     * @param deviceContext Contexto de dispositivo donde aplicar el estado.
     * @param StartSlot Primer slot de muestreador en el shader.
     * @param NumSamplers N�mero de muestreadores a asignar.
     *
     * @note
     * Este m�todo debe llamarse antes de dibujar, para que el shader
     * use el muestreador configurado.
     */
    void render(DeviceContext& deviceContext,
        unsigned int StartSlot,
        unsigned int NumSamplers);

    /**
     * @brief Libera los recursos asociados.
     *
     * @warning
     * No liberar este recurso causar� fugas de memoria GPU.
     */
    void destroy();

public:
    ID3D11SamplerState* m_sampler = nullptr; ///< Estado de muestreo de Direct3D 11.
};
